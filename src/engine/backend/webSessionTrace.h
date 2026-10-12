#ifndef __WEB_SESSION_TRACE_H__
#define __WEB_SESSION_TRACE_H__

#include "backend/backend.h"

#include <string>

// A stderr line about a web session. `sessionId` is taken so the caller
// has it in hand and does not also paste it onto the line. The line
// names the event. It does not contain the id.
// `detail` is a tab count for "reusing" and a tab index for "close-tab".
BACKEND_API std::string ibWebSessionTrace(const std::string& event,
	const std::string& sessionId,
	int detail = -1);

// The body of a successful POST /login. The session id is not in it.
BACKEND_API std::string ibWebLoginOkBody(const std::string& sessionId,
	const std::string& user);

#endif
