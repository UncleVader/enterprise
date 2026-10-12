#include "backend/fileBaseSecondProcess.h"

wxString ibSecondProcessOnLocalFile(const wxString& filePath, bool heldStandalone)
{
	if (filePath.IsEmpty() || !heldStandalone)
		return wxEmptyString;

	// The same test ibFirebirdLeaderMode uses to stay standalone.
	// Anything else is a share, and a second process may attach to it.
	if (filePath.StartsWith(wxT("\\\\")) || filePath.StartsWith(wxT("//")))
		return wxEmptyString;

	return wxT("This process already holds this local file base, and a second process cannot open it. "
		"Embedded Firebird on a local path is single-process. "
		"Use a server base, or a base on a network share, to run the application beside this one.");
}
