#ifndef __WEB_GUARD_HEADERS_H__
#define __WEB_GUARD_HEADERS_H__

#include "backend/backend.h"

#include <string>

// A per-response token for the inline <style> and <script> in the web
// client. Hex, 32 characters, safe to place inside a CSP nonce and an
// HTML attribute.
BACKEND_API std::string ibWebNonce();

// The document with nonce="..." on the first <style and the first
// <script that do not already carry one. Other text is left as it is.
BACKEND_API std::string ibWebDocumentWithNonce(const std::string& html,
	const std::string& nonce);

// Headers that go on every web response. HSTS is absent: the listener
// speaks plain HTTP. When `nonce` is set, script-src and style-src name
// it, and img-src allows the data: pictures the client draws. There is
// no 'unsafe-inline'.
struct ibWebGuardHeaders {
	std::string contentSecurityPolicy;
	std::string contentTypeOptions;
	std::string referrerPolicy;
	std::string openerPolicy;
	std::string cacheControl;
};

BACKEND_API ibWebGuardHeaders ibWebGuardHeadersFor(const std::string& nonce);

// True when this response has not chosen a Cache-Control of its own.
// An icon's max-age and the event stream's no-cache stay. Everything
// else is no-store.
BACKEND_API bool ibWebUseDefaultCacheControl(bool alreadySet);

#endif
