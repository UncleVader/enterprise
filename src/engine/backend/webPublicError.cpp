#include "backend/webPublicError.h"

#include "backend/appData.h"
#include "backend/logger/logger.h"

#include <nlohmann/json.hpp>

#include <random>

std::string ibWebPublicInternalError(const std::string& correlationId)
{
	nlohmann::json body;
	body["error"] = "internal";
	body["id"] = correlationId;
	return body.dump();
}

std::string ibWebCorrelationId()
{
	std::random_device rd;
	std::uniform_int_distribution<int> dist(0, 15);
	static const char* hex = "0123456789abcdef";
	std::string id(32, '0');
	for (char& c : id)
		c = hex[dist(rd)];
	return id;
}

std::string ibWebDatabaseFailureJournal(const ibWebDatabaseFailure& failure)
{
	std::string line = "id=" + failure.correlationId;
	line += " code=";
	line += failure.hasDriverCode ? std::to_string(failure.driverCode) : "none";
	line += " sqlstate=";
	line += failure.sqlState.empty() ? "none" : failure.sqlState;
	line += " retryable=";
	line += failure.retryable ? "true" : "false";
	line += " message=";
	line += failure.message;
	return line;
}

void ibWebRecordDatabaseFailure(const ibWebDatabaseFailure& failure)
{
	const wxString line = wxString::FromUTF8(ibWebDatabaseFailureJournal(failure));
	ibJournalError(wxT("web"), wxT("%s"), line);
	if (ibLogger* logger = ibApplicationInstance::GetLogger(ibApplicationInstance::Get(false)))
		logger->Error(wxT("web"), wxT("database"), line);
}
