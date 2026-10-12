#include "backend/webLaunchReport.h"

wxString ibWebManifestUrl(const wxString& content)
{
	const int pos = content.Find(wxT("url="));
	if (pos == wxNOT_FOUND)
		return wxEmptyString;

	wxString url = content.Mid(pos + 4);
	url = url.BeforeFirst(wxT('\n'));
	url.Trim().Trim(false);
	return url;
}

wxString ibWebLaunchFailure(const ibWebLaunchFacts& facts)
{
	if (!facts.spawned)
		return wxT("wenterprise-server could not be started.");

	if (facts.manifestReady)
		return wxEmptyString;

	wxString msg;
	if (!facts.running) {
		msg = wxString::Format(
			wxT("wenterprise-server exited with status %d before it accepted a connection."),
			facts.exitStatus);
	}
	else {
		msg = wxT("wenterprise-server did not start accepting connections.");
	}

	msg += wxT(" Sign-in waits up to 20 seconds and the process exits if it does not finish.");

	wxString err = facts.errText;
	err.Trim().Trim(false);
	if (!err.IsEmpty()) {
		msg += wxT("\n");
		msg += err;
	}
	return msg;
}
