#include <gtest/gtest.h>

#include "backend/webLaunchReport.h"

TEST(WebLaunchReport, NotSpawnedNamesTheStartAndNotTheSignInWait)
{
	const wxString msg = ibWebLaunchFailure(ibWebLaunchFacts{});
	EXPECT_EQ(wxString(wxT("wenterprise-server could not be started.")), msg);
}

TEST(WebLaunchReport, AManifestUrlIsSuccess)
{
	ibWebLaunchFacts facts;
	facts.spawned = true;
	facts.manifestReady = true;
	facts.running = true;
	facts.errText = wxT("ignored once it is up");
	EXPECT_TRUE(ibWebLaunchFailure(facts).IsEmpty());
}

TEST(WebLaunchReport, AnEarlyExitCarriesStatusStderrAndTheAuthWait)
{
	ibWebLaunchFacts facts;
	facts.spawned = true;
	facts.manifestReady = false;
	facts.running = false;
	facts.exitStatus = 1;
	facts.errText = wxT("  Failed to open the database: locked\n");

	EXPECT_EQ(
		wxString(wxT("wenterprise-server exited with status 1 before it accepted a connection. "
			"Sign-in waits up to 20 seconds and the process exits if it does not finish.\n"
			"Failed to open the database: locked")),
		ibWebLaunchFailure(facts));
}

TEST(WebLaunchReport, ASignalStatusIsTheNumberWxReports)
{
	ibWebLaunchFacts facts;
	facts.spawned = true;
	facts.exitStatus = -9;

	const wxString msg = ibWebLaunchFailure(facts);
	EXPECT_TRUE(msg.Contains(wxT("status -9")));
	EXPECT_TRUE(msg.Contains(wxT("20 seconds")));
	EXPECT_TRUE(msg.Find(wxT('\n')) == wxNOT_FOUND);
}

TEST(WebLaunchReport, StillRunningWithoutAManifestNamesTheAuthWait)
{
	ibWebLaunchFacts facts;
	facts.spawned = true;
	facts.running = true;
	facts.exitStatus = 0;
	facts.errText = wxT("   ");

	EXPECT_EQ(
		wxString(wxT("wenterprise-server did not start accepting connections. "
			"Sign-in waits up to 20 seconds and the process exits if it does not finish.")),
		ibWebLaunchFailure(facts));
}

TEST(WebLaunchReport, ManifestUrlIsTheValueOnThatLine)
{
	EXPECT_EQ(
		wxString(wxT("http://127.0.0.1:49152/base")),
		ibWebManifestUrl(wxT("host=127.0.0.1\nport=49152\nurl=http://127.0.0.1:49152/base\n")));
	EXPECT_EQ(
		wxString(wxT("http://127.0.0.1:9/x")),
		ibWebManifestUrl(wxT("url=  http://127.0.0.1:9/x\r\n")));
	EXPECT_TRUE(ibWebManifestUrl(wxT("host=127.0.0.1\n")).IsEmpty());
	EXPECT_TRUE(ibWebManifestUrl(wxT("url=   \n")).IsEmpty());
	EXPECT_TRUE(ibWebManifestUrl(wxEmptyString).IsEmpty());
}
