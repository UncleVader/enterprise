#ifndef __WEB_CATALOG_ROUTE_H__
#define __WEB_CATALOG_ROUTE_H__

#include "backend/backend.h"

#include <string>

// True when `path` is a list of the configuration: the menu, the forms,
// the class registry, all functions, or the interfaces. Those answers
// name objects in the base, so they are for a signed-in session. A
// prefix (`/w/Configuration/menu`) still matches. `/`, `/ping`, `/login`
// and `/auth-info` do not.
BACKEND_API bool ibWebPathNamesConfiguration(const std::string& path);

#endif
