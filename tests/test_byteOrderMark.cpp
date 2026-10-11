// ByteOrderMarkUsage is ИспользованиеByteOrderMark. Without it, ByteOrderMarkUsage.Use
// is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"

TEST(ByteOrderMarkUsage, UseAutoAndDontUseAreMembers) {
	const ibValue usage = ibValue::CreateObject(wxT("ByteOrderMarkUsage"));
	EXPECT_GE(usage.FindProp(wxT("Auto")), 0);
	EXPECT_GE(usage.FindProp(wxT("Use")), 0);
	EXPECT_GE(usage.FindProp(wxT("DontUse")), 0);
	EXPECT_EQ(usage.FindProp(wxT("Always")), wxNOT_FOUND);
}

TEST(ByteOrderMarkUsage, TheNameCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction valueSystem;
	cc.AddContextVariable(wxT("System"), &valueSystem, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var mark;\n")
			wxT("    mark = ByteOrderMarkUsage.Use;\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
