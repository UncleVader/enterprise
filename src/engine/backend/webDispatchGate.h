#ifndef __WEB_DISPATCH_GATE_H__
#define __WEB_DISPATCH_GATE_H__

#include "backend/backend.h"

#include <string>

// Whether a web control may run this request.
//
// reachable: the control and every parent window are enabled and shown.
// valueLocked: the form is view-only, or this control will not take a
// new value (text edit mode off, a read-only checkbox).
// kind: the HandleRequest kind (click, text, toggle, buttonClear, ...).
//
// A locked control may still click, open, select, or fetch. It may not
// be given a new value. An unreachable control accepts nothing.
BACKEND_API bool ibWebDispatchAllowed(bool reachable, bool valueLocked,
	const std::string& kind);

// The fire routes return this body when the gate refuses, so the HTTP
// layer can answer 403 instead of an empty form.
BACKEND_API const char* ibWebDispatchRefusedBody();

#endif
