#include "backend/webDispatchGate.h"

bool ibWebDispatchAllowed(bool reachable, bool valueLocked,
	const std::string& kind)
{
	if (!reachable)
		return false;
	if (!valueLocked)
		return true;
	// Open stays live on a read-only field. click is a command, not a
	// value. fetch is a read.
	return kind == "click"
		|| kind == "buttonOpen"
		|| kind == "buttonSelect"
		|| kind == "fetch";
}

const char* ibWebDispatchRefusedBody()
{
	return "{\"refused\":true}";
}
