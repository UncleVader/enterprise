#include "backend/webCatalogRoute.h"

#include <cstring>

bool ibWebPathNamesConfiguration(const std::string& path)
{
	static const char* kSuffixes[] = {
		"/menu",
		"/forms",
		"/diag/ctors",
		"/functions",
		"/interfaces",
	};
	for (const char* suffix : kSuffixes) {
		const std::size_t n = std::strlen(suffix);
		if (path.size() >= n && path.compare(path.size() - n, n, suffix) == 0)
			return true;
	}
	return false;
}
