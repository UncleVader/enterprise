#ifndef _IB_MCP_SHEET_PATH_H_
#define _IB_MCP_SHEET_PATH_H_

/////////////////////////////////////////////////////////////////////////////
// sheet_import and sheet_export may only touch a file inside the base's
// exchange folder. A path anywhere else is refused. The check is here so a
// test can hand it a folder without opening a base.
/////////////////////////////////////////////////////////////////////////////

#include "backend/backend.h"

#include <wx/string.h>

// <base>/exchange, or empty when this process has no base folder.
BACKEND_API wxString ibMcpSheetExchangeDir();

// True when fileName resolves to a file inside exchangeDir. `..`, a prefix
// of the folder name, and a symlink that leaves the folder are refused.
BACKEND_API bool ibMcpSheetPathAllowed(const wxString& fileName, const wxString& exchangeDir,
	wxString& refusal);

#endif
