// XDTOFactory is ФабрикаXDTO. Without it, the name is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"

namespace {

ibValue Factory()
{
	ibValue global = ibValue::CreateObject(wxT("GlobalContextManager"));
	ibValue factory;
	EXPECT_TRUE(global.GetPropVal(global.FindProp(wxT("XDTOFactory")), factory));
	return factory;
}

ibValue Call(ibValue& object, const wxChar* method, ibValue** args, long count)
{
	ibValue result;
	EXPECT_TRUE(object.CallAsFunc(object.FindMethod(method), result, args, count));
	return result;
}

wxString Prop(ibValue& value, const wxChar* name)
{
	ibValue found;
	EXPECT_TRUE(value.GetPropVal(value.FindProp(name), found));
	return wxString(found.GetString());
}

} // namespace

TEST(XDTOFactory, TypeAndCreateRememberWhatWasSaid) {
	ibValue factory = Factory();
	ASSERT_TRUE(factory.IsReference());

	ibValue uri(wxT("http://example/goods"));
	ibValue name(wxT("Order"));
	ibValue* typeArgs[] = { &uri, &name };
	ibValue type = Call(factory, wxT("Type"), typeArgs, 2);
	EXPECT_EQ(Prop(type, wxT("NamespaceURI")), wxT("http://example/goods"));
	EXPECT_EQ(Prop(type, wxT("Name")), wxT("Order"));

	ibValue* createArgs[] = { &type };
	ibValue object = Call(factory, wxT("Create"), createArgs, 1);
	ibValue addr(wxT("ada@example.com"));
	ASSERT_TRUE(object.SetPropVal(object.FindProp(wxT("addr")), addr));
	EXPECT_EQ(Prop(object, wxT("addr")), wxT("ada@example.com"));

	ibValue unread;
	ASSERT_TRUE(object.GetPropVal(object.FindProp(wxT("subject")), unread));
	EXPECT_EQ(unread.GetType(), ibValueTypes::TYPE_EMPTY);

	ibValue again = Call(object, wxT("Type"), nullptr, 0);
	EXPECT_EQ(Prop(again, wxT("Name")), wxT("Order"));

	ibValue empty(wxT(""));
	ibValue* bad[] = { &empty, &name };
	EXPECT_THROW(Call(factory, wxT("Type"), bad, 2), ibBackendException);
	ibValue* notAType[] = { &uri };
	EXPECT_THROW(Call(factory, wxT("Create"), notAType, 1), ibBackendException);
}

TEST(XDTOFactory, TheNameCompiles) {
	ibValue global = ibValue::CreateObject(wxT("GlobalContextManager"));
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	cc.AddContextVariable(wxT("GlobalContextManager"), &global, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var order;\n")
			wxT("    order = XDTOFactory.Create(XDTOFactory.Type(\"http://example/goods\", \"Order\"));\n")
			wxT("    order.addr = \"ada@example.com\";\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
