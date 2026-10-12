#ifndef __WEB_SESSION_COOKIE_H__
#define __WEB_SESSION_COOKIE_H__

#include "backend/backend.h"

#include <string>

// The Set-Cookie value for the web session. A non-empty id is
// `oes_session=<id>; Path=/; HttpOnly; SameSite=Strict`. An empty id
// is logout: the same attributes plus Max-Age=0, so the browser drops
// it. Secure and the __Host- prefix stay off until the listener speaks
// TLS. The client keeps its tab id in sessionStorage and sends
// X-OES-Session, so script never has to read this cookie.
BACKEND_API std::string ibWebSessionCookie(const std::string& id);

#endif
