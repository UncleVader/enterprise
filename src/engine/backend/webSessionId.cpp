#include "backend/webSessionId.h"

#include <cctype>
#include <cstring>

namespace {

bool SameMethod(const std::string& method, const char* literal)
{
	const std::size_t n = std::strlen(literal);
	if (method.size() != n)
		return false;
	for (std::size_t i = 0; i < n; ++i) {
		const unsigned char c = static_cast<unsigned char>(method[i]);
		if (std::tolower(c) != static_cast<unsigned char>(literal[i]))
			return false;
	}
	return true;
}

} // namespace

bool ibWebMethodChangesState(const std::string& method)
{
	return SameMethod(method, "post")
		|| SameMethod(method, "put")
		|| SameMethod(method, "patch")
		|| SameMethod(method, "delete");
}

std::string ibWebSessionId(const std::string& header,
	const std::string& querySid,
	const std::string& cookie,
	bool headerOnly)
{
	if (!header.empty())
		return header;
	if (headerOnly)
		return std::string();
	if (!querySid.empty())
		return querySid;

	const auto pos = cookie.find("oes_session=");
	if (pos == std::string::npos)
		return std::string();
	const std::size_t start = pos + 12;
	const std::size_t end = cookie.find(';', start);
	if (end == std::string::npos)
		return cookie.substr(start);
	return cookie.substr(start, end - start);
}
