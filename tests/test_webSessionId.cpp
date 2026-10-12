////////////////////////////////////////////////////////////////////////////
//	Description : which session id a web request may name. A state-
//	              changing method takes X-OES-Session only. A cookie or
//	              ?sid= is enough for a GET, where an image and an
//	              EventSource cannot set a header.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webSessionId.h"

namespace {

const char* kId = "11111111-2222-3333-4444-555555555555";
const char* kOther = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";

} // namespace

TEST(WebSessionId, PostPutPatchAndDeleteChangeState)
{
	EXPECT_TRUE(ibWebMethodChangesState("POST"));
	EXPECT_TRUE(ibWebMethodChangesState("post"));
	EXPECT_TRUE(ibWebMethodChangesState("PUT"));
	EXPECT_TRUE(ibWebMethodChangesState("PATCH"));
	EXPECT_TRUE(ibWebMethodChangesState("DELETE"));
	EXPECT_FALSE(ibWebMethodChangesState("GET"));
	EXPECT_FALSE(ibWebMethodChangesState("HEAD"));
	EXPECT_FALSE(ibWebMethodChangesState("OPTIONS"));
}

TEST(WebSessionId, AStateChangeIgnoresTheCookieAndTheQuery)
{
	const std::string cookie = std::string("oes_session=") + kOther;
	EXPECT_EQ(ibWebSessionId("", kId, cookie, true), "");
	EXPECT_EQ(ibWebSessionId(kId, kOther, cookie, true), kId);
}

TEST(WebSessionId, AReadUsesTheHeaderThenTheQueryThenTheCookie)
{
	const std::string cookie = std::string("theme=dark; oes_session=") + kOther + "; Path=/";
	EXPECT_EQ(ibWebSessionId(kId, "from-query", cookie, false), kId);
	EXPECT_EQ(ibWebSessionId("", "from-query", cookie, false), "from-query");
	EXPECT_EQ(ibWebSessionId("", "", cookie, false), kOther);
	EXPECT_EQ(ibWebSessionId("", "", "", false), "");
}
