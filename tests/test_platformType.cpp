// PlatformType is ТипПлатформы. A port names PlatformType.Windows_x86_64; without the
// enumeration that name is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"

TEST(PlatformType, Windows_x86_64IsAMember) {
	const ibValue kind = ibValue::CreateObject(wxT("PlatformType"));
	EXPECT_GE(kind.FindProp(wxT("Windows_x86_64")), 0);
	EXPECT_GE(kind.FindProp(wxT("Linux_x86_64")), 0);
	EXPECT_GE(kind.FindProp(wxT("MacOS_ARM64")), 0);
	EXPECT_EQ(kind.FindProp(wxT("NotAPlatform")), wxNOT_FOUND);
}

TEST(PlatformType, TheNameCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction valueSystem;
	cc.AddContextVariable(wxT("System"), &valueSystem, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var kind;\n")
			wxT("    kind = PlatformType.Windows_x86_64;\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
