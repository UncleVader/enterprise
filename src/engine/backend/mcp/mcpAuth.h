#ifndef _IB_MCP_AUTH_H_
#define _IB_MCP_AUTH_H_

/////////////////////////////////////////////////////////////////////////////
// The bearer check the MCP listener asks of every request, without a socket.
// An empty expected token never matches. The token itself is compared in full.
/////////////////////////////////////////////////////////////////////////////

#include "backend/backend.h"

#include <wx/string.h>

BACKEND_API bool ibMcpBearerAuthorised(const wxString& authorizationHeader,
	const wxString& expectedToken);

#endif
