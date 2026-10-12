#ifndef _IB_ZIP_BUDGET_H_
#define _IB_ZIP_BUDGET_H_

/////////////////////////////////////////////////////////////////////////////
// A ZIP entry is expanded only up to a fixed budget.
//
// The uncompressed size stored in the archive is a claim, not a measurement:
// a tiny deflate stream can advertise a few bytes and then emit megabytes, or
// advertise gigabytes so a reader allocates them up front. Callers count the
// bytes that actually come out of the decompressor and stop when the budget
// is spent. What was collected is dropped, so a refusal does not keep a
// partial buffer.
/////////////////////////////////////////////////////////////////////////////

#include "backend/backend.h"

#include <wx/stream.h>

#include <cstdint>
#include <vector>

struct ibZipBudget {
	uint64_t maxEntryBytes = 0;
	uint64_t maxTotalBytes = 0;
	unsigned maxEntries = 0;
	unsigned maxMillis = 0;
};

// A workbook's parts, a database archive, and a help file are different sizes.
// Each budget is large enough for a real file of that kind and small enough
// that one entry cannot exhaust the process.
BACKEND_API ibZipBudget ibZipBudgetForWorkbook();
BACKEND_API ibZipBudget ibZipBudgetForDatabase();
BACKEND_API ibZipBudget ibZipBudgetForHelp();

enum class ibZipBudgetFault {
	None = 0,
	TooManyEntries,
	DeclaredTooLarge,
	ExpandedTooLarge,
	TotalTooLarge,
	TimedOut,
	Corrupt,
	DuplicateName,
	TooManyXmlMarks,
	TooManyCells,
	TooManySharedStrings
};

struct ibZipEntryBytes {
	wxString name;
	std::vector<uint8_t> bytes;
};

// `keyOf` returns the name under which the entry is kept. An empty return
// skips the entry: it is not expanded, and it does not count toward the byte
// budget. It still counts toward the entry and time budgets. A second accepted
// entry with the same key is a duplicate. Used by the backend readers; the
// test binary goes through those readers.
bool ibCollectZipEntries(
	wxInputStream& input,
	const ibZipBudget& budget,
	wxString (*keyOf)(const wxString& name, void* ctx),
	void* ctx,
	std::vector<ibZipEntryBytes>& entries,
	ibZipBudgetFault& fault,
	wxMBConv& conv = wxConvLocal);

// The config, user and data entries of a database archive, in file order.
// Any other entry is left compressed. On failure `entries` is empty.
BACKEND_API bool ibReadDatabaseArchive(
	wxInputStream& input,
	const ibZipBudget& budget,
	std::vector<ibZipEntryBytes>& entries,
	ibZipBudgetFault& fault);

#endif // _IB_ZIP_BUDGET_H_
