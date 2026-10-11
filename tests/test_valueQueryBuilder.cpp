// QueryBuilder is ПостроительЗапроса. Without it, New QueryBuilder is
// "Constructor not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueQueryBuilder.h"

namespace {

ibValue Prop(ibValue& object, const wxString& name)
{
	ibValue value;
	EXPECT_TRUE(object.GetPropVal(object.FindProp(name), value));
	return value;
}

} // namespace

TEST(QueryBuilder, TextParametersAndAnEmptyExecute) {
	ibValue builder = ibValue::CreateObject(wxT("QueryBuilder"));
	ASSERT_TRUE(builder.IsReference());

	ibValue text(wxT("SELECT 1"));
	ASSERT_TRUE(builder.SetPropVal(builder.FindProp(wxT("Text")), text));
	EXPECT_EQ(Prop(builder, wxT("Text")).GetString(), wxString(wxT("SELECT 1")));

	ibValue parameters = Prop(builder, wxT("Parameters"));
	ibValue key(wxT("Day"));
	ibValue day(ibNumber(3));
	ibValue* insert[] = { &key, &day };
	ibValue ignored;
	ASSERT_TRUE(parameters.CallAsFunc(parameters.FindMethod(wxT("Insert")), ignored, insert, 2));
	EXPECT_EQ(Prop(parameters, wxT("Day")).GetInteger(), 3);

	ibValue query;
	ASSERT_TRUE(builder.CallAsFunc(builder.FindMethod(wxT("GetQuery")), query, nullptr, 0));
	EXPECT_GE(query.FindMethod(wxT("Execute")), 0);
	EXPECT_GE(query.FindMethod(wxT("SetParameter")), 0);

	ASSERT_TRUE(builder.CallAsProc(builder.FindMethod(wxT("FillSettings")), nullptr, 0));

	builder.SetPropVal(builder.FindProp(wxT("Text")), ibValue(wxT("")));
	ibValue result;
	ASSERT_TRUE(builder.CallAsFunc(builder.FindMethod(wxT("Execute")), result, nullptr, 0));
	EXPECT_GE(result.FindMethod(wxT("Select")), 0);
}

TEST(QueryBuilder, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var builder;\n")
			wxT("    builder = New QueryBuilder;\n")
			wxT("    builder.Text = \"SELECT 1\";\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
