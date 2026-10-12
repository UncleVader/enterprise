#ifndef __FILE_BASE_SECOND_PROCESS_H__
#define __FILE_BASE_SECOND_PROCESS_H__

#include "backend/backend.h"

// A local file base opened by this process is embedded Firebird in
// standalone mode: one process. A second process on that file exits at
// once. An empty path is a server connection. A UNC/SMB path uses leader
// mode and may be opened beside this one. `heldStandalone` is this
// process's current Firebird role; a process that does not hold the file
// does not refuse.
BACKEND_API wxString ibSecondProcessOnLocalFile(const wxString& filePath, bool heldStandalone);

#endif
