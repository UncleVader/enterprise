#ifndef __FILE_BASE_SECOND_PROCESS_H__
#define __FILE_BASE_SECOND_PROCESS_H__

#include "backend/backend.h"

// The ServerMode value from a firebird.conf text. Blank lines and comments
// are skipped. The last assignment wins, which is how the shipped file
// comments out the Super default and then sets SuperClassic. Empty when
// the file does not say.
BACKEND_API wxString ibFirebirdServerMode(const wxString& confText);

// A second process on a local file this process holds. Empty unless Firebird
// is in Super mode (ThreadedDedicated), which opens the file for itself.
// SuperClassic — what the shipped engine uses — and Classic share a local
// file, so a designer can start the thick client on the same base. An empty
// serverMode is not a refusal: an unreadable conf must not block that launch.
// An empty path is a server connection. A UNC/SMB path uses leader mode.
// `heldStandalone` is this process's current Firebird role.
BACKEND_API wxString ibSecondProcessOnLocalFile(const wxString& filePath, bool heldStandalone,
	const wxString& serverMode);

// ServerMode from this process's _fb/firebird.conf, or empty when that
// folder was not found. Used by the launch sites; tests pass the text.
BACKEND_API wxString ibConfiguredFirebirdServerMode();

#endif
