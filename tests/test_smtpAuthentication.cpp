// SMTPAuthenticationMode is СпособSMTPАутентификации. Without it,
// SMTPAuthenticationMode.Login is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"

TEST(SMTPAuthenticationMode, LoginPlainAndDefaultAreMembers) {
	const ibValue mode = ibValue::CreateObject(wxT("SMTPAuthenticationMode"));
	EXPECT_GE(mode.FindProp(wxT("None")), 0);
	EXPECT_GE(mode.FindProp(wxT("Plain")), 0);
	EXPECT_GE(mode.FindProp(wxT("Login")), 0);
	EXPECT_GE(mode.FindProp(wxT("CramMD5")), 0);
	EXPECT_GE(mode.FindProp(wxT("Default")), 0);
	EXPECT_EQ(mode.FindProp(wxT("NTLM")), wxNOT_FOUND);
}

TEST(SMTPAuthenticationMode, TheNameCompiles) {
	// A host binds EnumManager as a transparent scope (moduleManager.cpp). That is
	// the road a script takes to SMTPAuthenticationMode.Login; the name is not a global.
	const ibValue enums = ibValue::CreateObject(wxT("EnumManager"));
	ASSERT_GE(enums.FindProp(wxT("SMTPAuthenticationMode")), 0);

	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction valueSystem;
	cc.AddContextVariable(wxT("System"), &valueSystem, true);
	cc.AddContextVariable(wxT("EnumManager"), enums, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var mode;\n")
			wxT("    mode = SMTPAuthenticationMode.Login;\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
