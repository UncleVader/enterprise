#include "backend/sheetFormat/sheetFormatXlsx.h"
#include "backend/utils/zipBudget.h"

#include <gtest/gtest.h>

#include <wx/init.h>
#include <wx/mstream.h>
#include <wx/zipstrm.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

struct WxReady {
	WxReady()
	{
		static wxInitializer init;
		(void)init;
	}
};

std::vector<uint8_t> ZipOf(const std::vector<std::pair<std::string, std::string>>& entries, int level = 9)
{
	wxMemoryOutputStream out;
	{
		wxZipOutputStream zip(out, level);
		for (const auto& entry : entries) {
			zip.PutNextEntry(wxString::FromUTF8(entry.first));
			zip.Write(entry.second.data(), entry.second.size());
		}
		zip.Close();
	}

	std::vector<uint8_t> bytes(static_cast<size_t>(out.GetLength()));
	if (!bytes.empty())
		out.CopyTo(bytes.data(), bytes.size());
	return bytes;
}

wxMemoryInputStream Open(const std::vector<uint8_t>& bytes)
{
	return wxMemoryInputStream(bytes.data(), bytes.size());
}

std::string TextOf(const std::vector<uint8_t>& bytes)
{
	return std::string(bytes.begin(), bytes.end());
}

// The uncompressed-size field is a claim. Rewrite it for one entry in both the
// local header and the central directory, and clear the data-descriptor bit so
// the claim is the size the reader is shown.
bool PatchUncompressedSize(std::vector<uint8_t>& zip, const std::string& name, uint32_t size)
{
	auto read16 = [&](size_t at) -> uint16_t {
		return static_cast<uint16_t>(zip[at] | (zip[at + 1] << 8));
	};
	auto write32 = [&](size_t at, uint32_t value) {
		zip[at] = static_cast<uint8_t>(value & 0xff);
		zip[at + 1] = static_cast<uint8_t>((value >> 8) & 0xff);
		zip[at + 2] = static_cast<uint8_t>((value >> 16) & 0xff);
		zip[at + 3] = static_cast<uint8_t>((value >> 24) & 0xff);
	};

	int patched = 0;
	for (size_t at = 0; at + 30 < zip.size(); ++at) {
		const bool local = zip[at] == 0x50 && zip[at + 1] == 0x4b && zip[at + 2] == 0x03 && zip[at + 3] == 0x04;
		const bool central = zip[at] == 0x50 && zip[at + 1] == 0x4b && zip[at + 2] == 0x01 && zip[at + 3] == 0x02;
		if (!local && !central)
			continue;

		const size_t nameLenAt = local ? at + 26 : at + 28;
		const size_t nameAt = local ? at + 30 : at + 46;
		if (nameAt >= zip.size())
			break;

		const uint16_t nameLen = read16(nameLenAt);
		const uint16_t extraLen = read16(nameLenAt + 2);
		if (nameAt + nameLen > zip.size())
			continue;

		const std::string got(reinterpret_cast<const char*>(&zip[nameAt]), nameLen);
		if (got != name)
			continue;

		write32(local ? at + 22 : at + 24, size);
		const size_t flagsAt = local ? at + 6 : at + 8;
		zip[flagsAt] = static_cast<uint8_t>(zip[flagsAt] & ~0x08);
		++patched;
		at = nameAt + nameLen + extraLen;
	}
	return patched >= 2;
}

std::string SheetXml(const std::string& text, int cells = 1)
{
	std::string xml = "<?xml version=\"1.0\"?><worksheet><sheetData><row r=\"1\">";
	for (int cell = 0; cell < cells; ++cell)
		xml += "<c r=\"A1\" t=\"inlineStr\"><is><t>" + text + "</t></is></c>";
	xml += "</row></sheetData></worksheet>";
	return xml;
}

std::vector<std::pair<std::string, std::string>> Workbook(const std::string& sheet)
{
	return {
		{ "xl/workbook.xml",
		  "<?xml version=\"1.0\"?>"
		  "<workbook xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\">"
		  "<sheets><sheet name=\"S\" sheetId=\"1\" r:id=\"rId1\"/></sheets></workbook>" },
		{ "xl/_rels/workbook.xml.rels",
		  "<?xml version=\"1.0\"?>"
		  "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
		  "<Relationship Id=\"rId1\" Type=\"worksheet\" Target=\"worksheets/sheet1.xml\"/>"
		  "</Relationships>" },
		{ "xl/worksheets/sheet1.xml", sheet }
	};
}

ibZipBudget Tight()
{
	ibZipBudget budget = ibZipBudgetForDatabase();
	budget.maxEntryBytes = 4096;
	budget.maxTotalBytes = 8192;
	budget.maxEntries = 32;
	budget.maxMillis = 5000;
	return budget;
}

} // namespace

TEST(ZipBudget, OrdinaryArchiveKeepsConfigUserAndData)
{
	WxReady wx;
	const auto bytes = ZipOf({
		{ "config", "CFG" },
		{ "notes", std::string(80 * 1024, 'Z') },
		{ "user", "USR" },
		{ "data", "DAT" }
	});
	auto input = Open(bytes);

	ibZipBudget budget = Tight();
	// The notes entry is not one of the three the restore reads. Its size is
	// past the per-entry budget and must not be expanded.
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	ASSERT_TRUE(ibReadDatabaseArchive(input, budget, entries, fault));
	ASSERT_EQ(entries.size(), 3u);
	EXPECT_EQ(entries[0].name, wxT("config"));
	EXPECT_EQ(TextOf(entries[0].bytes), "CFG");
	EXPECT_EQ(entries[1].name, wxT("user"));
	EXPECT_EQ(TextOf(entries[1].bytes), "USR");
	EXPECT_EQ(entries[2].name, wxT("data"));
	EXPECT_EQ(TextOf(entries[2].bytes), "DAT");
	EXPECT_EQ(fault, ibZipBudgetFault::None);
}

TEST(ZipBudget, ConfigEntryOverBudgetIsRefusedAndDropped)
{
	WxReady wx;
	const auto bytes = ZipOf({ { "config", std::string(64 * 1024, 'C') } });
	auto input = Open(bytes);
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, Tight(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, UserEntryOverBudgetIsRefusedAndDropped)
{
	WxReady wx;
	const auto bytes = ZipOf({
		{ "config", "CFG" },
		{ "user", std::string(64 * 1024, 'U') }
	});
	auto input = Open(bytes);
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, Tight(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, DataEntryOverBudgetIsRefusedAndDropped)
{
	WxReady wx;
	const auto bytes = ZipOf({
		{ "config", "CFG" },
		{ "data", std::string(64 * 1024, 'D') }
	});
	auto input = Open(bytes);
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, Tight(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, HighlyCompressibleEntryStopsWhenTheDeclaredSizeIsALie)
{
	WxReady wx;
	auto bytes = ZipOf({ { "config", std::string(256 * 1024, 'A') } });
	ASSERT_TRUE(PatchUncompressedSize(bytes, "config", 100));
	auto input = Open(bytes);

	ibZipBudget budget = Tight();
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, budget, entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::ExpandedTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, DeclaredSizePastTheProductionBudgetIsRefused)
{
	WxReady wx;
	auto bytes = ZipOf({ { "config", "CFG" } });
	ASSERT_TRUE(PatchUncompressedSize(bytes, "config", 0x7fffffffu));
	auto input = Open(bytes);

	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, ibZipBudgetForDatabase(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, DuplicateNameIsRefused)
{
	WxReady wx;
	const auto bytes = ZipOf({
		{ "config", "one" },
		{ "config", "two" }
	});
	auto input = Open(bytes);
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, Tight(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DuplicateName);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, TooManyEntriesIsRefused)
{
	WxReady wx;
	std::vector<std::pair<std::string, std::string>> entries;
	for (int at = 0; at < 20; ++at)
		entries.emplace_back("extra" + std::to_string(at), "x");
	entries.emplace_back("config", "CFG");
	const auto bytes = ZipOf(entries);
	auto input = Open(bytes);

	ibZipBudget budget = Tight();
	budget.maxEntries = 8;
	std::vector<ibZipEntryBytes> read;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, budget, read, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TooManyEntries);
	EXPECT_TRUE(read.empty());
}

TEST(ZipBudget, TotalOfAcceptedEntriesIsBounded)
{
	WxReady wx;
	const auto bytes = ZipOf({
		{ "config", std::string(3000, 'C') },
		{ "user", std::string(3000, 'U') },
		{ "data", std::string(3000, 'D') }
	});
	auto input = Open(bytes);
	ibZipBudget budget = Tight();
	budget.maxEntryBytes = 4000;
	budget.maxTotalBytes = 7000;
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, budget, entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TotalTooLarge);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, ZeroTimeBudgetRefusesBeforeExpansion)
{
	WxReady wx;
	const auto bytes = ZipOf({ { "config", std::string(64 * 1024, 'A') } });
	auto input = Open(bytes);
	ibZipBudget budget = Tight();
	budget.maxMillis = 0;
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, budget, entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TimedOut);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, CorruptStreamIsRefused)
{
	WxReady wx;
	const std::string text = "this is not a zip";
	wxMemoryInputStream input(text.data(), text.size());
	std::vector<ibZipEntryBytes> entries;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibReadDatabaseArchive(input, Tight(), entries, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::Corrupt);
	EXPECT_TRUE(entries.empty());
}

TEST(ZipBudget, OrdinaryWorkbookStillReads)
{
	WxReady wx;
	const auto bytes = ZipOf(Workbook(SheetXml("Hello")));
	auto input = Open(bytes);

	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibSheetFormatXlsx format;
	ASSERT_TRUE(format.Read(input, sheet));
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("Hello"));
}

TEST(ZipBudget, WorkbookPartOverBudgetLeavesTheDocument)
{
	WxReady wx;
	const auto bytes = ZipOf(Workbook(SheetXml(std::string(64 * 1024, 'Q'))));
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.zip = Tight();
	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, WorkbookWithALyingSheetSizeStopsAtEmittedBytes)
{
	WxReady wx;
	auto bytes = ZipOf(Workbook(SheetXml(std::string(256 * 1024, 'Q'))));
	ASSERT_TRUE(PatchUncompressedSize(bytes, "xl/worksheets/sheet1.xml", 100));
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.zip = Tight();
	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::ExpandedTooLarge);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, ProductionBudgetRefusesAGiganticDeclaredSheet)
{
	WxReady wx;
	auto bytes = ZipOf(Workbook(SheetXml("Hello")));
	ASSERT_TRUE(PatchUncompressedSize(bytes, "xl/worksheets/sheet1.xml", 0x7fffffffu));
	auto input = Open(bytes);

	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, ibXlsxReadLimitsStandard(), fault));
	EXPECT_EQ(fault, ibZipBudgetFault::DeclaredTooLarge);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, SkippedWorkbookPartIsNotExpanded)
{
	WxReady wx;
	auto parts = Workbook(SheetXml("Hello"));
	parts.emplace_back("xl/calcChain.xml", std::string(80 * 1024, 'K'));
	const auto bytes = ZipOf(parts);
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.zip = Tight();
	ibSpreadsheetDescription sheet;
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	ASSERT_TRUE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("Hello"));
}

TEST(ZipBudget, TooManyCellsLeavesTheDocument)
{
	WxReady wx;
	const auto bytes = ZipOf(Workbook(SheetXml("Hello", 3)));
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.maxCells = 2;
	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TooManyCells);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, TooManySharedStringsLeavesTheDocument)
{
	WxReady wx;
	auto parts = Workbook(SheetXml("Hello"));
	parts.emplace_back(
		"xl/sharedStrings.xml",
		"<?xml version=\"1.0\"?><sst><si><t>one</t></si><si><t>two</t></si></sst>");
	const auto bytes = ZipOf(parts);
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.maxSharedStrings = 1;
	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TooManySharedStrings);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, TooManyXmlMarksLeavesTheDocument)
{
	WxReady wx;
	const auto bytes = ZipOf(Workbook(SheetXml("Hello")));
	auto input = Open(bytes);

	ibXlsxReadLimits limits = ibXlsxReadLimitsStandard();
	limits.maxXmlMarks = 3;
	ibSpreadsheetDescription sheet;
	sheet.SetCellValue(0, 0, wxT("keep"));
	ibZipBudgetFault fault = ibZipBudgetFault::None;
	EXPECT_FALSE(ibSheetFormatXlsxRead(input, sheet, limits, fault));
	EXPECT_EQ(fault, ibZipBudgetFault::TooManyXmlMarks);
	EXPECT_EQ(sheet.GetCellValue(0, 0), wxT("keep"));
}

TEST(ZipBudget, XmlCountersStopPastTheBudget)
{
	WxReady wx;
	EXPECT_EQ(ibCountXmlMarks(wxT("<a></a><b>"), 3), 4u);
	EXPECT_EQ(ibCountXmlItems(wxT("<c r=\"A1\"/><col/><c>"), wxT("c"), 1), 2u);
	EXPECT_EQ(ibCountXmlItems(wxT("<si/><size/><si>"), wxT("si"), 5), 2u);
}
