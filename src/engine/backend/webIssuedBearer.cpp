#include "backend/webIssuedBearer.h"

#include <cctype>
#include <random>

namespace {

void FillRandom(unsigned char* bytes, std::size_t count)
{
	std::random_device rd;
	for (std::size_t i = 0; i < count; ) {
		unsigned int value = rd();
		for (int part = 0; part < 4 && i < count; ++part, ++i) {
			bytes[i] = static_cast<unsigned char>(value & 0xffu);
			value >>= 8;
		}
	}
}

std::string Hex(const unsigned char* bytes, std::size_t count)
{
	static const char* digits = "0123456789abcdef";
	std::string out(count * 2, '0');
	for (std::size_t i = 0; i < count; ++i) {
		out[i * 2]     = digits[bytes[i] >> 4];
		out[i * 2 + 1] = digits[bytes[i] & 0x0f];
	}
	return out;
}

std::string RandomBearer()
{
	unsigned char bytes[32];
	FillRandom(bytes, sizeof(bytes));
	return Hex(bytes, sizeof(bytes));
}

std::string RandomRegistryGuid()
{
	unsigned char bytes[16];
	FillRandom(bytes, sizeof(bytes));
	bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40);
	bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);
	const std::string hex = Hex(bytes, sizeof(bytes));
	return hex.substr(0, 8) + "-" + hex.substr(8, 4) + "-"
		+ hex.substr(12, 4) + "-" + hex.substr(16, 4) + "-"
		+ hex.substr(20, 12);
}

bool IsLowerHex(const std::string& text, std::size_t size)
{
	if (text.size() != size)
		return false;
	for (char c : text) {
		const unsigned char u = static_cast<unsigned char>(c);
		if (!std::isxdigit(u) || std::isupper(u))
			return false;
	}
	return true;
}

} // namespace

std::string ibWebIssuedBearers::Mint()
{
	for (;;) {
		const std::string bearer = RandomBearer();
		if (m_live.find(bearer) != m_live.end())
			continue;
		m_live.emplace(bearer, Entry{ RandomRegistryGuid() });
		return bearer;
	}
}

bool ibWebIssuedBearers::WasIssued(const std::string& bearer) const
{
	return m_live.find(bearer) != m_live.end();
}

std::string ibWebIssuedBearers::RegistryGuid(const std::string& bearer) const
{
	const auto it = m_live.find(bearer);
	return it == m_live.end() ? std::string() : it->second.registryGuid;
}

std::string ibWebIssuedBearers::Rotate(const std::string& bearer)
{
	const auto it = m_live.find(bearer);
	if (it == m_live.end())
		return std::string();
	const std::string guid = it->second.registryGuid;
	m_live.erase(it);
	for (;;) {
		const std::string neu = RandomBearer();
		if (m_live.find(neu) != m_live.end())
			continue;
		m_live.emplace(neu, Entry{ guid });
		return neu;
	}
}

void ibWebIssuedBearers::Revoke(const std::string& bearer)
{
	m_live.erase(bearer);
}

std::string ibWebPresentedSessionId(const std::string& header,
	const std::string& querySid,
	const std::string& cookie)
{
	if (!querySid.empty() && header.empty() && cookie.empty())
		return std::string();
	if (!header.empty())
		return header;

	const auto pos = cookie.find("oes_session=");
	if (pos == std::string::npos)
		return std::string();
	const std::size_t start = pos + 12;
	const std::size_t end = cookie.find(';', start);
	if (end == std::string::npos)
		return cookie.substr(start);
	return cookie.substr(start, end - start);
}

std::string ibWebClientPage(const std::string& html, const std::string& bearer)
{
	static const char* kMarker = "OES_SID";
	if (!IsLowerHex(bearer, 64))
		return html;
	const auto at = html.find(kMarker);
	if (at == std::string::npos)
		return html;
	std::string page = html;
	page.replace(at, 7, bearer);
	return page;
}

std::string ibWebLoginAdopted(const std::string& bearer)
{
	if (!IsLowerHex(bearer, 64))
		return std::string();
	return std::string("{\"id\":\"") + bearer + "\"}\n";
}
