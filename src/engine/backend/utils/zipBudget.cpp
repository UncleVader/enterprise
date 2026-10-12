#include "backend/utils/zipBudget.h"

#include <wx/log.h>
#include <wx/zipstrm.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>

namespace {

void Release(std::vector<ibZipEntryBytes>& entries)
{
	entries.clear();
	entries.shrink_to_fit();
}

wxString NormalizeZipName(const wxString& name)
{
	wxString key = name;
	key.Replace(wxT("\\"), wxT("/"));
	while (key.StartsWith(wxT("./")))
		key = key.Mid(2);
	if (key.StartsWith(wxT("/")))
		key = key.Mid(1);
	return key;
}

bool AlreadyAccepted(const std::vector<ibZipEntryBytes>& entries, const wxString& key)
{
	for (const ibZipEntryBytes& entry : entries)
		if (entry.name == key)
			return true;
	return false;
}

wxString DatabaseEntryKey(const wxString& name, void*)
{
	if (name == wxT("config") || name == wxT("user") || name == wxT("data"))
		return name;
	return wxString();
}

// Whole megabytes from the environment. Zero, a sign, a suffix and anything
// that does not fit in the byte counter are ignored so a bad value cannot
// turn the budget off or wrap it.
bool ReadMegabytes(const wxString& name, uint64_t& bytes)
{
	wxString raw;
	if (!wxGetEnv(name, &raw))
		return false;
	raw.Trim(true).Trim(false);
	if (raw.empty())
		return false;

	const wxScopedCharBuffer utf8 = raw.ToUTF8();
	if (!utf8)
		return false;

	errno = 0;
	char* end = nullptr;
	const unsigned long long megabytes = std::strtoull(utf8.data(), &end, 10);
	if (end == utf8.data() || *end != '\0' || errno == ERANGE || megabytes == 0)
		return false;

	constexpr unsigned long long kMaxMegabytes =
		std::numeric_limits<uint64_t>::max() / (1024ull * 1024ull);
	if (megabytes > kMaxMegabytes)
		return false;

	bytes = static_cast<uint64_t>(megabytes) * 1024ull * 1024ull;
	return true;
}

} // namespace

ibZipBudget ibZipBudgetForWorkbook()
{
	ibZipBudget budget;
	budget.maxEntryBytes = 64ull * 1024ull * 1024ull;
	budget.maxTotalBytes = 128ull * 1024ull * 1024ull;
	budget.maxEntries = 4096;
	budget.maxMillis = 30000;
	return budget;
}

ibZipBudget ibZipBudgetForDatabase()
{
	ibZipBudget budget;
	budget.maxEntryBytes = 512ull * 1024ull * 1024ull;
	budget.maxTotalBytes = 1024ull * 1024ull * 1024ull;
	budget.maxEntries = 64;
	budget.maxMillis = 120000;

	uint64_t entryBytes = 0;
	const bool hasEntry = ReadMegabytes(wxT("OES_ZIP_DATABASE_ENTRY_MB"), entryBytes);
	if (hasEntry)
		budget.maxEntryBytes = entryBytes;

	uint64_t totalBytes = 0;
	const bool hasTotal = ReadMegabytes(wxT("OES_ZIP_DATABASE_TOTAL_MB"), totalBytes);
	if (hasTotal)
		budget.maxTotalBytes = totalBytes;
	else if (hasEntry && budget.maxTotalBytes < budget.maxEntryBytes)
		budget.maxTotalBytes = budget.maxEntryBytes;

	return budget;
}

ibZipBudget ibZipBudgetForHelp()
{
	ibZipBudget budget;
	budget.maxEntryBytes = 32ull * 1024ull * 1024ull;
	budget.maxTotalBytes = 64ull * 1024ull * 1024ull;
	budget.maxEntries = 8192;
	budget.maxMillis = 30000;
	return budget;
}

bool ibCollectZipEntries(
	wxInputStream& input,
	const ibZipBudget& budget,
	wxString (*keyOf)(const wxString& name, void* ctx),
	void* ctx,
	std::vector<ibZipEntryBytes>& entries,
	ibZipBudgetFault& fault,
	wxMBConv& conv)
{
	Release(entries);
	fault = ibZipBudgetFault::None;

	// A bad archive logs through wx. The caller is refusing the file; a dialog
	// here would stop the process before the refusal could be returned.
	wxLogNull quiet;

	const auto started = std::chrono::steady_clock::now();
	const auto expired = [&]() -> bool {
		const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - started).count();
		return elapsed >= static_cast<std::chrono::milliseconds::rep>(budget.maxMillis);
	};

	if (expired()) {
		fault = ibZipBudgetFault::TimedOut;
		return false;
	}

	// A stream that does not open with the ZIP signature is not an empty
	// archive. wx reports that shape as a successful read of nothing.
	const wxFileOffset start = input.TellI();
	unsigned char signature[2] = {};
	input.Read(signature, 2);
	const size_t signatureBytes = input.LastRead();
	if (start == wxInvalidOffset || input.SeekI(start) == wxInvalidOffset
		|| signatureBytes < 2 || signature[0] != 'P' || signature[1] != 'K') {
		fault = ibZipBudgetFault::Corrupt;
		return false;
	}

	wxZipInputStream zip(input, conv);
	if (!zip.IsOk()) {
		fault = ibZipBudgetFault::Corrupt;
		return false;
	}

	uint64_t total = 0;
	unsigned seen = 0;
	std::vector<uint8_t> chunk(16 * 1024);

	for (;;) {
		if (expired()) {
			fault = ibZipBudgetFault::TimedOut;
			Release(entries);
			return false;
		}

		std::unique_ptr<wxZipEntry> entry(zip.GetNextEntry());
		if (!entry)
			break;

		++seen;
		if (seen > budget.maxEntries) {
			fault = ibZipBudgetFault::TooManyEntries;
			Release(entries);
			return false;
		}

		if (entry->IsDir())
			continue;

		const wxString name = NormalizeZipName(entry->GetInternalName());
		if (name.empty() || keyOf == nullptr)
			continue;

		const wxString key = keyOf(name, ctx);
		if (key.empty())
			continue;

		if (AlreadyAccepted(entries, key)) {
			fault = ibZipBudgetFault::DuplicateName;
			Release(entries);
			return false;
		}

		const wxFileOffset declared = entry->GetSize();
		if (declared > 0) {
			const uint64_t declaredBytes = static_cast<uint64_t>(declared);
			if (declaredBytes > budget.maxEntryBytes) {
				fault = ibZipBudgetFault::DeclaredTooLarge;
				Release(entries);
				return false;
			}
			if (declaredBytes > budget.maxTotalBytes
				|| total > budget.maxTotalBytes - declaredBytes) {
				fault = ibZipBudgetFault::TotalTooLarge;
				Release(entries);
				return false;
			}
		}

		std::vector<uint8_t> body;
		if (declared > 0)
			body.reserve(static_cast<size_t>(declared));

		for (;;) {
			if (expired()) {
				fault = ibZipBudgetFault::TimedOut;
				Release(entries);
				return false;
			}

			zip.Read(chunk.data(), chunk.size());
			const size_t got = zip.LastRead();
			if (got == 0)
				break;

			const uint64_t next = static_cast<uint64_t>(body.size()) + static_cast<uint64_t>(got);
			if (next > budget.maxEntryBytes) {
				fault = ibZipBudgetFault::ExpandedTooLarge;
				Release(entries);
				return false;
			}
			if (next > budget.maxTotalBytes || total > budget.maxTotalBytes - next) {
				fault = ibZipBudgetFault::TotalTooLarge;
				Release(entries);
				return false;
			}

			body.insert(body.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(got));
		}

		if (zip.GetLastError() == wxSTREAM_READ_ERROR) {
			fault = ibZipBudgetFault::Corrupt;
			Release(entries);
			return false;
		}

		total += static_cast<uint64_t>(body.size());
		ibZipEntryBytes kept;
		kept.name = key;
		kept.bytes = std::move(body);
		entries.push_back(std::move(kept));
	}

	return true;
}

bool ibReadDatabaseArchive(
	wxInputStream& input,
	const ibZipBudget& budget,
	std::vector<ibZipEntryBytes>& entries,
	ibZipBudgetFault& fault)
{
	return ibCollectZipEntries(input, budget, &DatabaseEntryKey, nullptr, entries, fault);
}
