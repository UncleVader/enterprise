// =============================================================================
// The MCP bearer check, and the compare it uses.
//
// A plain == stops at the first byte that differs, so a watcher learns how much
// of a token matched. The check walks the whole token. These tests pin the
// answers: a match, a mismatch, a different length, a missing scheme, a blank
// expected token.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/mcp/mcpAuth.h"
#include "backend/utils/constTime.h"

TEST(ConstTimeEqual, SameBytesMatch)
{
	const uint8_t a[] = { 1, 2, 3, 4 };
	const uint8_t b[] = { 1, 2, 3, 4 };
	EXPECT_TRUE(ibConstTimeEqual(a, b, 4));
}

TEST(ConstTimeEqual, ADifferenceAnywhereRefuses)
{
	const uint8_t a[] = { 1, 2, 3, 4 };
	const uint8_t first[] = { 9, 2, 3, 4 };
	const uint8_t last[] = { 1, 2, 3, 9 };
	EXPECT_FALSE(ibConstTimeEqual(a, first, 4));
	EXPECT_FALSE(ibConstTimeEqual(a, last, 4));
}

TEST(ConstTimeEqual, StringsMatchOnlyWhenTheBytesDo)
{
	EXPECT_TRUE(ibConstTimeEqual(wxString(wxT("token")), wxString(wxT("token"))));
	EXPECT_FALSE(ibConstTimeEqual(wxString(wxT("token")), wxString(wxT("tokEn"))));
	EXPECT_FALSE(ibConstTimeEqual(wxString(wxT("token")), wxString(wxT("tokenX"))));
	EXPECT_FALSE(ibConstTimeEqual(wxString(wxT("tokenX")), wxString(wxT("token"))));
	EXPECT_TRUE(ibConstTimeEqual(wxString(), wxString()));
}

TEST(McpBearer, AMatchingBearerIsAccepted)
{
	EXPECT_TRUE(ibMcpBearerAuthorised(wxT("Bearer secret-token"), wxT("secret-token")));
	EXPECT_TRUE(ibMcpBearerAuthorised(wxT("Bearer  secret-token  "), wxT("secret-token")));
}

TEST(McpBearer, AWrongTokenIsRefused)
{
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Bearer secret-token"), wxT("secret-tokEn")));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Bearer secret"), wxT("secret-token")));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Bearer secret-token-extra"), wxT("secret-token")));
}

TEST(McpBearer, TheSchemeIsRequiredAndABlankTokenRefuses)
{
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("secret-token"), wxT("secret-token")));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Basic secret-token"), wxT("secret-token")));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Bearer secret-token"), wxEmptyString));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxEmptyString, wxT("secret-token")));
	EXPECT_FALSE(ibMcpBearerAuthorised(wxT("Bearer "), wxEmptyString));
}
