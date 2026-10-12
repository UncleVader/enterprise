// =============================================================================
// The Origin header is compared as scheme, host and port. A name that only
// starts with localhost or 127.0.0.1, and the literal "null", are refused.
// An absent header is still allowed: a tool that is not a browser sends none.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/mcp/mcpServer.h"

TEST(McpOrigin, NoHeaderAndThisComputerAreAllowed)
{
	EXPECT_TRUE(ibMcpOriginAllowed(wxString()));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("http://127.0.0.1")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("https://127.0.0.1")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("http://localhost")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("https://localhost")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("HTTP://LocalHost")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("http://127.0.0.1:3737")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("https://localhost:443")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("http://[::1]")));
	EXPECT_TRUE(ibMcpOriginAllowed(wxT("http://[::1]:3737")));
}

TEST(McpOrigin, ALookalikeHostIsRefused)
{
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://localhost.example.com")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("https://localhost.example.com")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.1.example.com")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://localhostevil")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.2")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://[::ffff:127.0.0.1]")));
}

TEST(McpOrigin, NullIsForeign)
{
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("null")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("NULL")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("Null")));
}

TEST(McpOrigin, APathUserOrBadPortIsRefused)
{
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.1/")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://user@127.0.0.1")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.1:99999")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.1:0")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://127.0.0.1:")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://localhost?x=1")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("file://localhost")));
	EXPECT_FALSE(ibMcpOriginAllowed(wxT("http://[::1]:no")));
}
