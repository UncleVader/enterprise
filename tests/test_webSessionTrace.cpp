////////////////////////////////////////////////////////////////////////////
//	Description : stderr lines and the login body must not carry the
//	              web session id. The id is the argument the caller
//	              already has; the text that leaves the process names
//	              the event only.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webSessionTrace.h"

namespace {

const char* kId = "11111111-2222-3333-4444-555555555555";

void ExpectNoId(const std::string& text)
{
	EXPECT_EQ(text.find(kId), std::string::npos) << text;
}

} // namespace

TEST(WebSessionTrace, PageLoadLinesNameTheEventAndNotTheId)
{
	const std::string created = ibWebSessionTrace("created", kId);
	const std::string reusing = ibWebSessionTrace("reusing", kId, 2);
	const std::string unknown = ibWebSessionTrace("unknown", kId);
	ExpectNoId(created);
	ExpectNoId(reusing);
	ExpectNoId(unknown);
	EXPECT_EQ(created, "[GET /] created a session");
	EXPECT_EQ(reusing, "[GET /] reusing a session tabCount=2");
	EXPECT_EQ(unknown, "[GET /] session unknown, creating");
}

TEST(WebSessionTrace, AClosedTabAndASweepDoNotNameTheSession)
{
	const std::string closed = ibWebSessionTrace("close-tab", kId, 3);
	const std::string destroyed = ibWebSessionTrace("sweep-destroy", kId);
	const std::string idle = ibWebSessionTrace("sweep-idle", kId);
	ExpectNoId(closed);
	ExpectNoId(destroyed);
	ExpectNoId(idle);
	EXPECT_EQ(closed, "[HTTP] DELETE /tab/3");
	EXPECT_EQ(destroyed, "[sweep] destroying a session on request");
	EXPECT_EQ(idle, "[sweep] evicting an idle session");
}

TEST(WebSessionTrace, LoginOkDoesNotReturnTheId)
{
	const std::string body = ibWebLoginOkBody(kId, "Admin");
	EXPECT_EQ(body, "ok\n");
	ExpectNoId(body);
	EXPECT_EQ(body.find("Admin"), std::string::npos);
}
