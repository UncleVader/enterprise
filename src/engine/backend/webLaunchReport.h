#ifndef __WEB_LAUNCH_REPORT_H__
#define __WEB_LAUNCH_REPORT_H__

#include "backend/backend.h"

// What a caller observed while waiting for wenterprise-server to accept
// connections. ibWebLaunchFailure turns this into the sentence app_run shows.
struct ibWebLaunchFacts {
	bool spawned = false;
	bool manifestReady = false;
	bool running = false;
	int exitStatus = 0;
	wxString errText;
};

// The url= value written into a bind manifest, or empty when it is not there yet.
BACKEND_API wxString ibWebManifestUrl(const wxString& content);

// Empty when the process was spawned and the manifest carried a url.
// Otherwise the reason the launch failed, including an early exit status,
// the child's stderr, and the 20-second sign-in wait.
BACKEND_API wxString ibWebLaunchFailure(const ibWebLaunchFacts& facts);

#endif
