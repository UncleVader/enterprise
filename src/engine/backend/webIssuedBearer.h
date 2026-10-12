#ifndef __WEB_ISSUED_BEARER_H__
#define __WEB_ISSUED_BEARER_H__

#include "backend/backend.h"

#include <map>
#include <string>

// The ids this process has minted for the web client. A value that is
// not in the table was not issued here: the registry guid, a guid copied
// out of /admin/locks, a ?sid= from a link. The bearer is 32 random
// bytes, hex (256 bits). The registry guid is a separate dashed UUID
// and is what sys_session and sys_lock store.
class BACKEND_API ibWebIssuedBearers {
public:
	std::string Mint();
	bool WasIssued(const std::string& bearer) const;
	std::string RegistryGuid(const std::string& bearer) const;
	// The old bearer is no longer issued. The new one keeps the same
	// registry guid. Empty when `bearer` was not issued.
	std::string Rotate(const std::string& bearer);
	void Revoke(const std::string& bearer);

private:
	struct Entry {
		std::string registryGuid;
	};
	std::map<std::string, Entry> m_live;
};

// Which session a request may name. `querySid` is ignored: a link
// ?sid=<bearer> does not select a session. The header wins, then the
// oes_session cookie.
BACKEND_API std::string ibWebPresentedSessionId(const std::string& header,
	const std::string& querySid,
	const std::string& cookie);

// The HTML document with its OES_SID marker replaced by the bearer the
// server just minted. A bearer that is not 64 hex digits leaves the
// marker in place.
BACKEND_API std::string ibWebClientPage(const std::string& html,
	const std::string& bearer);

// POST /login body. The client stores `id` and drops the previous bearer.
BACKEND_API std::string ibWebLoginAdopted(const std::string& bearer);

#endif
