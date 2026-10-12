#include <gtest/gtest.h>

#include "backend/fileBaseSecondProcess.h"

TEST(FileBaseSecondProcess, AServerConnectionIsNotAFile)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxEmptyString, true).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxEmptyString, false).IsEmpty());
}

TEST(FileBaseSecondProcess, AShareMayBeOpenedBesideThisProcess)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("\\\\server\\share\\demo.fdb"), true).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("//server/share/demo.fdb"), true).IsEmpty());
}

TEST(FileBaseSecondProcess, ALocalFileHeldHereIsRefused)
{
	const wxString why = ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), true);
	EXPECT_TRUE(why.Contains(wxT("second process cannot open it")));
	EXPECT_TRUE(why.Contains(wxT("single-process")));
	EXPECT_TRUE(why.Contains(wxT("network share")));

	const wxString windows = ibSecondProcessOnLocalFile(wxT("C:\\data\\demo.fdb"), true);
	EXPECT_TRUE(windows.Contains(wxT("single-process")));
}

TEST(FileBaseSecondProcess, ALocalFileThisProcessDoesNotHoldIsAllowed)
{
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("/var/lib/oes/demo.fdb"), false).IsEmpty());
	EXPECT_TRUE(ibSecondProcessOnLocalFile(wxT("C:\\data\\demo.fdb"), false).IsEmpty());
}
