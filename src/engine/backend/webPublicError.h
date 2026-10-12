#ifndef __WEB_PUBLIC_ERROR_H__
#define __WEB_PUBLIC_ERROR_H__

#include "backend/backend.h"

#include <string>

// What the browser is allowed to see when a database call fails.
// The driver's code, SQLSTATE and message — including the rendered
// statement — stay out of this JSON. `id` is the correlation id the
// journal line uses.
BACKEND_API std::string ibWebPublicInternalError(const std::string& correlationId);

// A new id for one failure. Hex, 32 characters. It is not a secret;
// it only ties the browser's "internal" to the journal line.
BACKEND_API std::string ibWebCorrelationId();

// One database (or other non-user) failure. `hasDriverCode` is false
// when the failure is not a driver exception, and `sqlState` is empty
// in that case. `message` is the text that must not reach the browser.
struct ibWebDatabaseFailure {
	std::string correlationId;
	std::string message;
	std::string sqlState;
	int driverCode = 0;
	bool hasDriverCode = false;
	bool retryable = false;
};

// The line written for operators. It carries the id, the driver code,
// the SQLSTATE and the message the browser does not get.
BACKEND_API std::string ibWebDatabaseFailureJournal(const ibWebDatabaseFailure& failure);

// Write that line to the technology journal and, when this process has
// a logger, to the process log. The technology-journal call is empty in
// a Release build, so the process log is what an operator still has.
// A thread with no base does not throw.
BACKEND_API void ibWebRecordDatabaseFailure(const ibWebDatabaseFailure& failure);

#endif
