////////////////////////////////////////////////////////////////////////////
//	Description : the web session Set-Cookie line. HttpOnly and
//	              SameSite=Strict now; Secure waits for TLS. The tab
//	              client sends X-OES-Session and does not read
//	              document.cookie, so these flags do not change how a
//	              page finds its session.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/session/webSessionCookie.h"

TEST(WebSessionCookie, ASessionId_IsHttpOnlyAndSameSiteStrict)
{
	const std::string id = "11111111-2222-3333-4444-555555555555";
	const std::string cookie = ibWebSessionCookie(id);

	EXPECT_EQ(cookie,
		"oes_session=11111111-2222-3333-4444-555555555555; Path=/; HttpOnly; SameSite=Strict");
	EXPECT_EQ(cookie.find("Secure"), std::string::npos);
	EXPECT_EQ(cookie.find("__Host-"), std::string::npos);

	const std::size_t semi = cookie.find(';');
	ASSERT_NE(semi, std::string::npos);
	EXPECT_EQ(cookie.substr(0, semi), "oes_session=" + id);
}

TEST(WebSessionCookie, AnEmptyId_ClearsTheCookie)
{
	EXPECT_EQ(ibWebSessionCookie(std::string()),
		"oes_session=; Path=/; HttpOnly; SameSite=Strict; Max-Age=0");
}
