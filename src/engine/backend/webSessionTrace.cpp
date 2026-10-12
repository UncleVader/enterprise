#include "backend/webSessionTrace.h"

std::string ibWebSessionTrace(const std::string& event,
	const std::string& /*sessionId*/,
	int detail)
{
	if (event == "created")
		return "[GET /] created a session";
	if (event == "reusing")
		return "[GET /] reusing a session tabCount=" + std::to_string(detail);
	if (event == "unknown")
		return "[GET /] session unknown, creating";
	if (event == "close-tab")
		return "[HTTP] DELETE /tab/" + std::to_string(detail);
	if (event == "sweep-destroy")
		return "[sweep] destroying a session on request";
	if (event == "sweep-idle")
		return "[sweep] evicting an idle session";
	return "[web] " + event;
}

std::string ibWebLoginOkBody(const std::string& /*sessionId*/,
	const std::string& /*user*/)
{
	return "ok\n";
}
