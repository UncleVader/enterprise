#include <gtest/gtest.h>

#include "backend/fileBaseSecondProcess.h"

TEST(FileBaseSecondProcess, AServerConnectionIsNotAFile)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxEmptyString, true, wxT("Super")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxEmptyString, false, wxT("Super")).IsEmpty());
}

TEST(FileBaseSecondProcess, AShareMayBeOpenedBesideThisProcess)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("\\\\server\\share\\demo.fdb"), true, wxT("Super")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("//server/share/demo.fdb"), true, wxT("Super")).IsEmpty());
}

TEST(FileBaseSecondProcess, SuperClassicSharesTheLocalFileTheDesignerHolds)
{
	// The shipped engine. A designer on a local file base starts the thick
	// client with app_run debug:true, and both attach.
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true, wxT("SuperClassic")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("C:\\data\\demo.fdb"), true, wxT("superclassic")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true, wxT("Classic")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true, wxT("ThreadedShared")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true, wxEmptyString).IsEmpty());
}

TEST(FileBaseSecondProcess, SuperKeepsALocalFileToItself)
{
	const wxString why = ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true, wxT("Super"));
	EXPECT_TRUE(why.Contains(wxT("Super mode")));
	EXPECT_TRUE(why.Contains(wxT("second process cannot open it")));
	EXPECT_TRUE(why.Contains(wxT("SuperClassic")));

	const wxString alias = ibSecondProcessOnLocalFile(wxT("C:\\data\\demo.fdb"), true, wxT("ThreadedDedicated"));
	EXPECT_TRUE(alias.Contains(wxT("Super mode")));
}

TEST(FileBaseSecondProcess, ALocalFileThisProcessDoesNotHoldIsAllowed)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), false, wxT("Super")).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("C:\\data\\demo.fdb"), false, wxT("Super")).IsEmpty());
}

TEST(FileBaseSecondProcess, TheShippedConfSelectsSuperClassicNotTheCommentedSuper)
{
	const wxString conf =
		wxT("#ServerMode = Super\n")
		wxT("\n")
		wxT("# OES override — SuperClassic instead of the FB 5 default Super.\n")
		wxT("ServerMode = SuperClassic\n");
	EXPECT_EQ(wxString(wxT("SuperClassic")), ibFirebirdServerMode(conf));
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/Users/me/oes-ds-test-conf-mig58/db.fdb"), true,
		ibFirebirdServerMode(conf)).IsEmpty());
}

TEST(FileBaseSecondProcess, AnExclusiveConfIsTheLastAssignment)
{
	const wxString conf =
		wxT("ServerMode = SuperClassic\n")
		wxT("ServerMode = Super # exclusive\n");
	EXPECT_EQ(wxString(wxT("Super")), ibFirebirdServerMode(conf));
}
