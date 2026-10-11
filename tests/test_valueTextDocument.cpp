// TextDocument is ТекстовыйДокумент. Without the type, New TextDocument is
// "Constructor not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueTextDocument.h"

namespace {

ibValue Call(ibValue& doc, const wxString& name, ibValue* a = nullptr, ibValue* b = nullptr)
{
	const long method = doc.FindMethod(name);
	EXPECT_GE(method, 0) << name.ToStdString();
	ibValue* params[2] = { a, b };
	const long count = b != nullptr ? 2 : (a != nullptr ? 1 : 0);
	ibValue ret;
	EXPECT_TRUE(doc.CallAsFunc(method, ret, count ? params : nullptr, count));
	return ret;
}

} // namespace

TEST(TextDocument, LinesAreCountedFromOne) {
	ibValue doc = ibValue::CreateObject(wxT("TextDocument"));
	ASSERT_TRUE(doc.IsReference());
	EXPECT_EQ(Call(doc, wxT("LineCount")).GetInteger(), 0);

	ibValue text(wxT("a\r\nb\n"));
	Call(doc, wxT("SetText"), &text);
	EXPECT_EQ(Call(doc, wxT("LineCount")).GetInteger(), 2);
	EXPECT_EQ(Call(doc, wxT("GetText")).GetString(), wxString(wxT("a\nb")));

	ibValue one(ibNumber(1));
	ibValue two(ibNumber(2));
	EXPECT_EQ(Call(doc, wxT("GetLine"), &one).GetString(), wxString(wxT("a")));
	EXPECT_EQ(Call(doc, wxT("GetLine"), &two).GetString(), wxString(wxT("b")));

	ibValue extra(wxT("c"));
	Call(doc, wxT("AddLine"), &extra);
	Call(doc, wxT("AddLine"));
	EXPECT_EQ(Call(doc, wxT("LineCount")).GetInteger(), 4);
	ibValue four(ibNumber(4));
	EXPECT_EQ(Call(doc, wxT("GetLine"), &four).GetString(), wxString());

	ibValue inserted(wxT("x"));
	Call(doc, wxT("InsertLine"), &two, &inserted);
	EXPECT_EQ(Call(doc, wxT("GetLine"), &two).GetString(), wxString(wxT("x")));

	ibValue replaced(wxT("y"));
	Call(doc, wxT("ReplaceLine"), &two, &replaced);
	EXPECT_EQ(Call(doc, wxT("GetLine"), &two).GetString(), wxString(wxT("y")));

	Call(doc, wxT("DeleteLine"), &two);
	EXPECT_EQ(Call(doc, wxT("GetLine"), &two).GetString(), wxString(wxT("b")));

	ibValue zero(ibNumber(0));
	ibValue* bad = &zero;
	ibValue ret;
	EXPECT_THROW(doc.CallAsFunc(doc.FindMethod(wxT("GetLine")), ret, &bad, 1), ibBackendException);

	Call(doc, wxT("Clear"));
	EXPECT_EQ(Call(doc, wxT("LineCount")).GetInteger(), 0);
	EXPECT_TRUE(doc.IsEmpty());
}

TEST(TextDocument, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var doc;\n")
			wxT("    doc = New TextDocument;\n")
			wxT("    doc.SetText(\"a\");\n")
			wxT("    doc.AddLine(\"b\");\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
