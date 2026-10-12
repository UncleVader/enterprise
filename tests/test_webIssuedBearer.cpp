////////////////////////////////////////////////////////////////////////////
//	Description : a web session id the server minted. An id it did not
//	              mint is not a session. Login replaces the bearer and
//	              keeps the registry guid. ?sid= does not select one.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webIssuedBearer.h"

namespace {

bool IsHex(const std::string& text)
{
	for (char c : text)
		if (std::string("0123456789abcdef").find(c) == std::string::npos)
			return false;
	return !text.empty();
}

} // namespace

TEST(WebIssuedBearer, AMintedBearerIsNotTheRegistryGuid)
{
	ibWebIssuedBearers issued;
	const std::string bearer = issued.Mint();
	const std::string guid = issued.RegistryGuid(bearer);

	EXPECT_EQ(bearer.size(), 64u);
	EXPECT_TRUE(IsHex(bearer));
	EXPECT_EQ(guid.size(), 36u);
	EXPECT_NE(guid, bearer);
	EXPECT_EQ(guid.find('-'), 8u);
	EXPECT_TRUE(issued.WasIssued(bearer));
	EXPECT_FALSE(issued.WasIssued(guid));
	EXPECT_FALSE(issued.WasIssued("11111111-2222-3333-4444-555555555555"));
}

TEST(WebIssuedBearer, LoginRotatesTheBearerAndKeepsTheRegistryGuid)
{
	ibWebIssuedBearers issued;
	const std::string before = issued.Mint();
	const std::string guid = issued.RegistryGuid(before);
	const std::string after = issued.Rotate(before);

	EXPECT_NE(after, before);
	EXPECT_EQ(after.size(), 64u);
	EXPECT_FALSE(issued.WasIssued(before));
	EXPECT_TRUE(issued.WasIssued(after));
	EXPECT_EQ(issued.RegistryGuid(after), guid);
	EXPECT_EQ(issued.RegistryGuid(before), "");
	EXPECT_EQ(issued.Rotate("not-issued"), "");
}

TEST(WebIssuedBearer, ARevokedBearerIsNotIssued)
{
	ibWebIssuedBearers issued;
	const std::string bearer = issued.Mint();
	issued.Revoke(bearer);
	EXPECT_FALSE(issued.WasIssued(bearer));
	EXPECT_EQ(issued.RegistryGuid(bearer), "");
}

TEST(WebIssuedBearer, AQuerySidDoesNotSelectASession)
{
	const std::string bearer(64, 'a');
	EXPECT_EQ(ibWebPresentedSessionId("", bearer, ""), "");
	EXPECT_EQ(ibWebPresentedSessionId("", bearer, "oes_session=from-cookie"), "from-cookie");
	EXPECT_EQ(ibWebPresentedSessionId("from-header", bearer, "oes_session=from-cookie"),
		"from-header");
	EXPECT_EQ(ibWebPresentedSessionId("", "", "theme=dark; oes_session=abc; Path=/"), "abc");
}

TEST(WebIssuedBearer, ThePageAndTheLoginBodyCarryTheMintedBearer)
{
	ibWebIssuedBearers issued;
	const std::string bearer = issued.Mint();
	const std::string page = ibWebClientPage(
		"<meta name=\"oes-session\" content=\"OES_SID\">", bearer);
	EXPECT_NE(page.find(bearer), std::string::npos);
	EXPECT_EQ(page.find("OES_SID"), std::string::npos);
	EXPECT_EQ(page.find(issued.RegistryGuid(bearer)), std::string::npos);

	EXPECT_EQ(ibWebLoginAdopted(bearer), std::string("{\"id\":\"") + bearer + "\"}\n");
	EXPECT_EQ(ibWebLoginAdopted(issued.RegistryGuid(bearer)), "");
	EXPECT_EQ(ibWebClientPage("<meta content=\"OES_SID\">", "not-a-bearer"),
		"<meta content=\"OES_SID\">");
}
