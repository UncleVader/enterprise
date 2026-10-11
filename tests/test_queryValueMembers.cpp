////////////////////////////////////////////////////////////////////////////
//  Isolation test — do the L4 runtime value objects expose their method
//  surface via the generic ibValue introspection (GetNMethods / FindMethod)
//  the designer autocomplete reads? Constructs the EMPTY (AST-less) variants
//  — no DB, no session — so this is a pure member-table check.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include "backend/backend_exception.h"
#include "backend/system/value/valueQuery.h"

namespace {
bool HasMethod(const ibValue& v, const wxString& name)
{
	for (long i = 0; i < v.GetNMethods(); ++i)
		if (v.GetMethodName(i) == name) return true;
	return false;
}
}

// QueryResult (what Execute() returns) must surface Select — the autocomplete after `q.Execute().`
TEST(QueryValueMembers, QueryResult_Exposes_Select)
{
	ibValueQueryResult res;   // empty / AST-less
	EXPECT_GE(res.GetNMethods(), 1);
	EXPECT_TRUE(HasMethod(res, wxT("Select")));
	EXPECT_GE(res.FindMethod(wxT("Select")), 0);
}

// QuerySelect must surface the cursor methods (Level is a METHOD now, Field is gone).
TEST(QueryValueMembers, QuerySelect_Exposes_CursorMethods)
{
	ibValueQuerySelect sel;   // empty / AST-less
	EXPECT_GE(sel.GetNMethods(), 6);
	EXPECT_GE(sel.FindMethod(wxT("Next")),        0);
	EXPECT_GE(sel.FindMethod(wxT("Reset")),       0);
	EXPECT_GE(sel.FindMethod(wxT("HasChildren")), 0);
	EXPECT_GE(sel.FindMethod(wxT("Select")),      0);
	EXPECT_GE(sel.FindMethod(wxT("Total")),       0);
	EXPECT_GE(sel.FindMethod(wxT("Level")),       0);   // now a method
	EXPECT_EQ(sel.FindMethod(wxT("Field")),       wxNOT_FOUND);   // removed (direct attribute access)
}

// Query.Text is the text the query runs. A script sets it after New Query(), and reads back what
// was set. A text that is not a query is refused and the previous text stays.
TEST(QueryValueMembers, QueryText_IsReadAndWritten)
{
	ibValueQueryExec query;
	ASSERT_TRUE(query.Init(nullptr, 0));

	const long text = query.FindProp(wxT("Text"));
	const long manager = query.FindProp(wxT("TempTablesManager"));
	ASSERT_GE(text, 0);
	ASSERT_GE(manager, 0);
	EXPECT_NE(text, manager);

	ibValue empty;
	ASSERT_TRUE(query.GetPropVal(text, empty));
	EXPECT_TRUE(empty.GetString().IsEmpty());

	ibValue written;
	written.SetString(wxT("SELECT Code FROM Catalog.Products"));
	ASSERT_TRUE(query.SetPropVal(text, written));

	ibValue read;
	ASSERT_TRUE(query.GetPropVal(text, read));
	EXPECT_EQ(read.GetString(), wxT("SELECT Code FROM Catalog.Products"));

	ibValue nonsense;
	nonsense.SetString(wxT("this is not a query"));
	EXPECT_THROW(query.SetPropVal(text, nonsense), ibBackendException);

	ibValue still;
	ASSERT_TRUE(query.GetPropVal(text, still));
	EXPECT_EQ(still.GetString(), wxT("SELECT Code FROM Catalog.Products"));
}
