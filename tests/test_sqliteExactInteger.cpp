// =============================================================================
// SQLite must round-trip an integer that a double cannot hold.
//
// A reference clsid is an integer near 2^60 (the kind sits in the high byte).
// The SQLite binder used to call sqlite3_bind_double, and the reader used to
// call sqlite3_column_int — 53 bits on the way in, 32 on the way out. The
// stored class was a different one, and a register list whose key contains
// that reference could not turn the page. Integers that fit in int64 bind and
// read as int64; a fraction still travels as a double.
// =============================================================================

#include <gtest/gtest.h>

#include <wx/init.h>

#include "backend/clsid.h"
#include "backend/databaseLayer/databaseResultSet.h"
#include "backend/databaseLayer/preparedStatement.h"
#include "backend/databaseLayer/sqllite/sqliteDatabaseLayer.h"
#include "backend/fnumber.h"

namespace {

struct SqliteExactIntegerFix : ::testing::Test {
	wxInitializer wx;
	ibDatabaseLayerSQLite db;

	void SetUp() override {
		if (!wx.IsOk())
			GTEST_SKIP() << "wxBase init failed";
		if (!db.Open(wxT(":memory:")))
			GTEST_SKIP() << "in-memory SQLite open failed";
		db.RunQuery(wxT("CREATE TABLE n (id INTEGER PRIMARY KEY, v BIGINT)"));
	}
	void TearDown() override { db.Close(); }

	void Put(int id, const ibNumber& value) {
		ibPreparedStatement* st = db.PrepareStatement(wxT("INSERT INTO n (id, v) VALUES (?, ?)"));
		ASSERT_NE(st, nullptr);
		st->SetParamInt(1, id);
		st->SetParamNumber(2, value);
		st->RunQuery();
		db.CloseStatement(st);
	}

	ibDatabaseResultSet* Row(int id) {
		ibDatabaseResultSet* rs = db.RunQueryWithResults(
			wxT("SELECT v FROM n WHERE id = %d"), id);
		EXPECT_TRUE(rs != nullptr && rs->Next());
		return rs;
	}
};

} // namespace

// The clsid shape the register list actually stores in _RTRef.
TEST_F(SqliteExactIntegerFix, ReferenceClsidSurvives) {
	const unsigned long long clsid = reference_to_clsid(42);
	ASSERT_GT(clsid, (1ULL << 53)) << "the fixture must sit past the double mantissa";
	Put(1, ibNumber(clsid));

	ibDatabaseResultSet* rs = Row(1);
	ASSERT_NE(rs, nullptr);
	EXPECT_EQ(rs->GetResultLong(1), static_cast<long long>(clsid));
	EXPECT_EQ(rs->GetResultNumber(1), ibNumber(clsid));
	db.CloseResultSet(rs);
}

// 2^53+3 is the first integer a double rounds away. It must come back whole.
TEST_F(SqliteExactIntegerFix, IntegerPastTheDoubleMantissaSurvives) {
	const long long n = (1LL << 53) + 3;
	Put(2, ibNumber(n));

	ibDatabaseResultSet* rs = Row(2);
	ASSERT_NE(rs, nullptr);
	EXPECT_EQ(rs->GetResultLong(1), n);
	EXPECT_EQ(rs->GetResultNumber(1), ibNumber(n));
	db.CloseResultSet(rs);
}

// A fraction is not an int64 bind. 3/2 is exact in a double, so it still
// round-trips on the path this driver has always used for non-integers.
TEST_F(SqliteExactIntegerFix, FractionStillRoundTrips) {
	const ibNumber half = ibNumber(3) / ibNumber(2);
	Put(3, half);

	ibDatabaseResultSet* rs = Row(3);
	ASSERT_NE(rs, nullptr);
	EXPECT_EQ(rs->GetResultNumber(1), half);
	db.CloseResultSet(rs);
}
