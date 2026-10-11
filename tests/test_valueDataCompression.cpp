// DataCompression is СжатиеДанных. Without it, New DataCompression is
// "Constructor not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueDataCompression.h"

namespace {

int LevelOf(const ibValue& made)
{
	ibValue level;
	EXPECT_TRUE(made.GetPropVal(made.FindProp(wxT("CompressionLevel")), level));
	return level.GetInteger();
}

} // namespace

TEST(DataCompression, TheLevelIsMinusOneUnlessItIsSaid) {
	const ibValue plain = ibValue::CreateObject(wxT("DataCompression"));
	ASSERT_TRUE(plain.IsReference());
	EXPECT_EQ(LevelOf(plain), -1);

	ibValue nine(ibNumber(9));
	ibValue* params = &nine;
	const ibValue tight = ibValue::CreateObject(wxT("DataCompression"), &params, 1);
	EXPECT_EQ(LevelOf(tight), 9);

	ibValue zero(ibNumber(0));
	params = &zero;
	EXPECT_EQ(LevelOf(ibValue::CreateObject(wxT("DataCompression"), &params, 1)), 0);

	ibValue bad(ibNumber(10));
	params = &bad;
	EXPECT_THROW(ibValue::CreateObject(wxT("DataCompression"), &params, 1), ibBackendException);
}

TEST(DataCompression, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var pack;\n")
			wxT("    pack = New DataCompression;\n")
			wxT("    pack = New DataCompression(9);\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
