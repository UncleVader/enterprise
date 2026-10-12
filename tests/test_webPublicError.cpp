////////////////////////////////////////////////////////////////////////////
//	Description : a database failure the web server reports. The browser
//	              gets {"error":"internal","id":"..."} and nothing of the
//	              driver: no SQLSTATE, no native code, no statement. The
//	              journal line keeps those, under the same id.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webPublicError.h"

TEST(WebPublicError, TheBrowserSeesAnIdAndNotTheStatement)
{
	ibWebDatabaseFailure failure;
	failure.correlationId = "abc123";
	failure.message = "column unknown\nThe statement was: SELECT TABLE_SECRET_XZ FROM sales";
	failure.sqlState = "42S22";
	failure.driverCode = 335544569;
	failure.hasDriverCode = true;
	failure.retryable = false;

	const std::string body = ibWebPublicInternalError(failure.correlationId);
	EXPECT_EQ(body, "{\"error\":\"internal\",\"id\":\"abc123\"}");
	EXPECT_EQ(body.find("TABLE_SECRET_XZ"), std::string::npos);
	EXPECT_EQ(body.find("42S22"), std::string::npos);
	EXPECT_EQ(body.find("335544569"), std::string::npos);
	EXPECT_EQ(body.find("sqlstate"), std::string::npos);
	EXPECT_EQ(body.find("message"), std::string::npos);

	const std::string journal = ibWebDatabaseFailureJournal(failure);
	EXPECT_NE(journal.find("id=abc123"), std::string::npos);
	EXPECT_NE(journal.find("code=335544569"), std::string::npos);
	EXPECT_NE(journal.find("sqlstate=42S22"), std::string::npos);
	EXPECT_NE(journal.find("retryable=false"), std::string::npos);
	EXPECT_NE(journal.find("TABLE_SECRET_XZ"), std::string::npos);
	EXPECT_NE(journal.find("The statement was:"), std::string::npos);
}

TEST(WebPublicError, AQuoteInTheIdStaysInsideTheJsonString)
{
	const std::string body = ibWebPublicInternalError("a\"b");
	EXPECT_EQ(body, "{\"error\":\"internal\",\"id\":\"a\\\"b\"}");
	EXPECT_EQ(body.find("message"), std::string::npos);
}

TEST(WebPublicError, AFailureWithoutADriverCodeSaysSoInTheJournal)
{
	ibWebDatabaseFailure failure;
	failure.correlationId = "n1";
	failure.message = "connection dropped";
	const std::string journal = ibWebDatabaseFailureJournal(failure);
	EXPECT_NE(journal.find("code=none"), std::string::npos);
	EXPECT_NE(journal.find("sqlstate=none"), std::string::npos);
	EXPECT_NE(journal.find("retryable=false"), std::string::npos);
	EXPECT_NE(journal.find("connection dropped"), std::string::npos);
}

TEST(WebPublicError, RecordingWithNoBaseDoesNotThrow)
{
	ibWebDatabaseFailure failure;
	failure.correlationId = "bare";
	failure.message = "SELECT TABLE_SECRET_XZ";
	failure.sqlState = "42000";
	failure.hasDriverCode = true;
	failure.driverCode = 1;
	EXPECT_NO_THROW(ibWebRecordDatabaseFailure(failure));
}

TEST(WebPublicError, ACorrelationIdIsThirtyTwoHexDigits)
{
	const std::string id = ibWebCorrelationId();
	ASSERT_EQ(id.size(), 32u);
	for (char c : id)
		EXPECT_NE(std::string("0123456789abcdef").find(c), std::string::npos) << c;
}
