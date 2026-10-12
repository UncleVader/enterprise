// =============================================================================
// sheet_import / sheet_export may only name a file inside the exchange folder.
// A path outside it, a `..` that leaves it, and a folder whose name merely
// starts with the exchange folder's name are refused.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/mcp/mcpSheetPath.h"

#include <wx/filefn.h>
#include <wx/filename.h>

namespace {

struct ExchangeDir {
	wxString path;

	ExchangeDir()
	{
		wxFileName dir = wxFileName::DirName(wxFileName::GetTempDir());
		dir.AppendDir(wxT("oes-exchange-path-test"));
		path = dir.GetFullPath();
		if (wxFileName::DirExists(path))
			wxFileName::Rmdir(path);
		dir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	}

	~ExchangeDir()
	{
		if (wxFileName::DirExists(path))
			wxFileName::Rmdir(path);
	}
};

} // namespace

TEST(SheetExchangePath, AFileInsideTheFolderIsAllowed)
{
	ExchangeDir exchange;
	ASSERT_TRUE(wxFileName::DirExists(exchange.path));

	wxFileName file(exchange.path, wxT("invoice.xlsx"));
	wxString refusal;
	EXPECT_TRUE(ibMcpSheetPathAllowed(file.GetFullPath(), exchange.path, refusal)) << refusal.ToStdString();
}

TEST(SheetExchangePath, APathOutsideIsRefused)
{
	ExchangeDir exchange;
	ASSERT_TRUE(wxFileName::DirExists(exchange.path));

	wxFileName outside(wxFileName::GetTempDir(), wxT("secret.xlsx"));
	wxString refusal;
	EXPECT_FALSE(ibMcpSheetPathAllowed(outside.GetFullPath(), exchange.path, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("outside the exchange folder")));
}

TEST(SheetExchangePath, ADotDotAndAPrefixLookalikeAreRefused)
{
	ExchangeDir exchange;
	ASSERT_TRUE(wxFileName::DirExists(exchange.path));

	wxString refusal;
	wxString root = exchange.path;
	if (!root.EndsWith(wxFileName::GetPathSeparator()))
		root += wxFileName::GetPathSeparator();
	const wxString escape = root + wxT("..") + wxFileName::GetPathSeparator() + wxT("secret.xlsx");
	EXPECT_FALSE(ibMcpSheetPathAllowed(escape, exchange.path, refusal));

	wxFileName lookalike = wxFileName::DirName(wxFileName::GetTempDir());
	lookalike.AppendDir(wxT("oes-exchange-path-test-extra"));
	wxFileName decoy(lookalike.GetFullPath(), wxT("invoice.xlsx"));
	refusal.clear();
	EXPECT_FALSE(ibMcpSheetPathAllowed(decoy.GetFullPath(), exchange.path, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("outside the exchange folder")));
}

TEST(SheetExchangePath, NoExchangeFolderRefusesEveryPath)
{
	wxString refusal;
	EXPECT_FALSE(ibMcpSheetPathAllowed(wxT("/tmp/invoice.xlsx"), wxEmptyString, refusal));
	EXPECT_NE(wxNOT_FOUND, refusal.Find(wxT("no exchange folder")));
}
