#ifndef __WEB_SESSION_ID_H__
#define __WEB_SESSION_ID_H__

#include "backend/backend.h"

#include <string>

// POST, PUT, PATCH and DELETE change state. A cookie and a ?sid= query
// travel with a cross-site form on their own. A custom header does not:
// the browser will not attach X-OES-Session to a form posted from
// another site, and this server does not answer a cross-origin
// preflight. GET stays open to the cookie and to ?sid=, because an
// image and an EventSource cannot set a header.
BACKEND_API bool ibWebMethodChangesState(const std::string& method);

// The session id a request is allowed to name. `header` is
// X-OES-Session, `querySid` is ?sid=, `cookie` is the Cookie header.
// When `headerOnly` is set, a missing header is an empty id even if
// the cookie or the query names one. A non-empty header always wins.
BACKEND_API std::string ibWebSessionId(const std::string& header,
	const std::string& querySid,
	const std::string& cookie,
	bool headerOnly);

#endif
