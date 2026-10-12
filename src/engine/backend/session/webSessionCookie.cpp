#include "backend/session/webSessionCookie.h"

std::string ibWebSessionCookie(const std::string& id)
{
	// HttpOnly keeps document.cookie from seeing the id. SameSite=Strict
	// keeps a cross-site request from carrying it. Path=/ is the whole
	// site, which is the only path the client uses.
	static const char* kFlags = "; Path=/; HttpOnly; SameSite=Strict";
	if (id.empty())
		return std::string("oes_session=") + kFlags + "; Max-Age=0";
	return "oes_session=" + id + kFlags;
}
