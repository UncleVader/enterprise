// GetFunctionalOption is ПолучитьФункциональнуюОпцию. Without the function the
// call is "procedure or function not detected". Functional options are not metadata,
// so the call is answered False.

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"

TEST(FunctionalOption, AnUnknownOptionIsOff) {
	ibValueSystemFunction system;
	const long method = system.FindMethod(wxT("GetFunctionalOption"));
	ASSERT_GE(method, 0);
	EXPECT_EQ(system.GetNParams(method), 2);

	ibValue name(wxT("UseWarehouses"));
	ibValue* params = &name;
	ibValue result;
	ASSERT_TRUE(system.CallAsFunc(method, result, &params, 1));
	EXPECT_FALSE(result.GetBoolean());

	ibValue extra(wxT("ignored"));
	ibValue* both[] = { &name, &extra };
	ASSERT_TRUE(system.CallAsFunc(method, result, both, 2));
	EXPECT_FALSE(result.GetBoolean());
}

TEST(FunctionalOption, TheNameCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction system;
	cc.AddContextVariable(wxT("System"), &system, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var on;\n")
			wxT("    on = GetFunctionalOption(\"UseWarehouses\");\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
