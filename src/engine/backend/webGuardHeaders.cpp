#include "backend/webGuardHeaders.h"

#include <random>

std::string ibWebNonce()
{
	std::random_device rd;
	std::uniform_int_distribution<int> dist(0, 15);
	static const char* hex = "0123456789abcdef";
	std::string nonce(32, '0');
	for (char& c : nonce)
		c = hex[dist(rd)];
	return nonce;
}

namespace {

std::string WithNonceOnTag(const std::string& html, const char* tag,
	const std::string& nonce)
{
	const std::string insert = " nonce=\"" + nonce + "\"";
	const std::size_t tagLen = std::char_traits<char>::length(tag);
	std::string out = html;
	std::size_t search = 0;
	while (search < out.size()) {
		const auto pos = out.find(tag, search);
		if (pos == std::string::npos)
			return out;
		const auto end = out.find('>', pos);
		if (end == std::string::npos)
			return out;
		const auto nonceAt = out.find("nonce=", pos);
		if (nonceAt != std::string::npos && nonceAt < end) {
			search = end + 1;
			continue;
		}
		out.insert(pos + tagLen, insert);
		search = end + 1 + insert.size();
	}
	return out;
}

} // namespace

std::string ibWebDocumentWithNonce(const std::string& html, const std::string& nonce)
{
	if (nonce.empty() || nonce.find('"') != std::string::npos)
		return html;
	return WithNonceOnTag(WithNonceOnTag(html, "<style", nonce), "<script", nonce);
}

ibWebGuardHeaders ibWebGuardHeadersFor(const std::string& nonce)
{
	ibWebGuardHeaders headers;
	headers.contentSecurityPolicy =
		"default-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'";
	if (!nonce.empty()) {
		headers.contentSecurityPolicy += "; script-src 'self' 'nonce-";
		headers.contentSecurityPolicy += nonce;
		headers.contentSecurityPolicy += "'; style-src 'self' 'nonce-";
		headers.contentSecurityPolicy += nonce;
		headers.contentSecurityPolicy += "'; img-src 'self' data:";
	}
	headers.contentTypeOptions = "nosniff";
	headers.referrerPolicy = "same-origin";
	headers.openerPolicy = "same-origin";
	headers.cacheControl = "no-store";
	return headers;
}

bool ibWebUseDefaultCacheControl(bool alreadySet)
{
	return !alreadySet;
}
