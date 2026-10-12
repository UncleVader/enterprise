////////////////////////////////////////////////////////////////////////////
//	Description : the headers on a web response, and the nonce the
//	              client's inline style and script need so a page can
//	              still run under them.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webGuardHeaders.h"

TEST(WebGuardHeaders, ADocumentGainsANonceOnItsStyleAndScript)
{
	const std::string html =
		"<style>body{margin:0}</style><script>var x=1;</script>";
	const std::string out = ibWebDocumentWithNonce(html, "abc");
	EXPECT_NE(out.find("<style nonce=\"abc\">"), std::string::npos);
	EXPECT_NE(out.find("<script nonce=\"abc\">"), std::string::npos);
	EXPECT_EQ(out.find("unsafe-inline"), std::string::npos);
}

TEST(WebGuardHeaders, ATagThatAlreadyHasANonceIsLeftAlone)
{
	const std::string html = "<script nonce=\"keep\">var x=1;</script>";
	EXPECT_EQ(ibWebDocumentWithNonce(html, "abc"), html);
}

TEST(WebGuardHeaders, ALeadingScriptDoesNotKeepTheRestAnonymous)
{
	// The caption dictionary is its own <script>, ahead of the client.
	// Stamping only the first tag leaves the program script with no
	// nonce, and script-src then refuses to run it.
	const std::string html =
		"<script nonce=\"keep\">window.OES={};</script>"
		"<style>body{margin:0}</style>"
		"<script>var x=1;</script>";
	const std::string nonce(32, 'a');
	const std::string out = ibWebDocumentWithNonce(html, nonce);
	EXPECT_NE(out.find("<script nonce=\"keep\">"), std::string::npos);
	EXPECT_NE(out.find("<style nonce=\"" + nonce + "\">"), std::string::npos);
	EXPECT_NE(out.find("<script nonce=\"" + nonce + "\">var x=1;</script>"), std::string::npos);
	const std::size_t stamp = std::string(" nonce=\"").size() + nonce.size() + 1;
	EXPECT_EQ(out.size(), html.size() + stamp * 2);
}

TEST(WebGuardHeaders, AQuoteInTheNonceIsNotWrittenIntoTheDocument)
{
	const std::string html = "<script>var x=1;</script>";
	EXPECT_EQ(ibWebDocumentWithNonce(html, "a\"b"), html);
}

TEST(WebGuardHeaders, ThePolicyNamesTheNonceAndRefusesInline)
{
	const ibWebGuardHeaders headers = ibWebGuardHeadersFor("abc");
	EXPECT_NE(headers.contentSecurityPolicy.find("default-src 'self'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("object-src 'none'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("base-uri 'none'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("frame-ancestors 'none'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("script-src 'self' 'nonce-abc'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("style-src 'self' 'nonce-abc'"), std::string::npos);
	EXPECT_NE(headers.contentSecurityPolicy.find("img-src 'self' data:"), std::string::npos);
	EXPECT_EQ(headers.contentSecurityPolicy.find("unsafe-inline"), std::string::npos);
	EXPECT_EQ(headers.contentSecurityPolicy.find("Strict-Transport-Security"), std::string::npos);
	EXPECT_EQ(headers.contentTypeOptions, "nosniff");
	EXPECT_EQ(headers.referrerPolicy, "same-origin");
	EXPECT_EQ(headers.openerPolicy, "same-origin");
	EXPECT_EQ(headers.cacheControl, "no-store");
}

TEST(WebGuardHeaders, AResponseThatAlreadySetCacheControlKeepsIt)
{
	EXPECT_TRUE(ibWebUseDefaultCacheControl(false));
	EXPECT_FALSE(ibWebUseDefaultCacheControl(true));
}

TEST(WebGuardHeaders, ANonceIsThirtyTwoHexDigits)
{
	const std::string nonce = ibWebNonce();
	ASSERT_EQ(nonce.size(), 32u);
	for (char c : nonce)
		EXPECT_NE(std::string("0123456789abcdef").find(c), std::string::npos) << c;
}
