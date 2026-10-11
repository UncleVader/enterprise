#include <gtest/gtest.h>

#include <cstring>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/system/systemManager.h"
#include "backend/system/systemManagerEnum.h"
#include "backend/system/value/valueBinaryData.h"

namespace {

const ibValueBinaryData* BytesOf(const ibValue& value)
{
	return dynamic_cast<const ibValueBinaryData*>(value.GetRef());
}

ibValue Call(const wxString& name, ibValue** args, long count)
{
	ibValueSystemFunction sys;
	const long method = sys.FindMethod(name);
	EXPECT_GE(method, 0) << name.ToStdString();
	ibValue ret;
	EXPECT_TRUE(sys.CallAsFunc(method, ret, args, count)) << name.ToStdString();
	return ret;
}

}

TEST(BinaryScript, Base64RoundTrip) {
	const char raw[] = "abc";
	ibValue binary(new ibValueBinaryData(raw, 3));
	ibValue* arg = &binary;
	const ibValue encoded = Call(wxT("Base64String"), &arg, 1);
	EXPECT_EQ(encoded.GetString(), wxT("YWJj"));

	ibValue text(wxT("YWJj"));
	arg = &text;
	const ibValue decoded = Call(wxT("Base64Value"), &arg, 1);
	const ibValueBinaryData* const bytes = BytesOf(decoded);
	ASSERT_NE(bytes, nullptr);
	ASSERT_EQ(bytes->GetLength(), 3u);
	EXPECT_EQ(std::memcmp(bytes->GetBuffer().GetData(), raw, 3), 0);

	ibValue emptyBinary(new ibValueBinaryData());
	arg = &emptyBinary;
	EXPECT_TRUE(Call(wxT("Base64String"), &arg, 1).GetString().empty());
	ibValue emptyText(wxT(""));
	arg = &emptyText;
	const ibValue emptyDecoded = Call(wxT("Base64Value"), &arg, 1);
	ASSERT_NE(BytesOf(emptyDecoded), nullptr);
	EXPECT_EQ(BytesOf(emptyDecoded)->GetLength(), 0u);
}

TEST(BinaryScript, TextRoundTrip) {
	ibValue text(wxT("abc"));
	ibValue* arg = &text;
	const ibValue utf8 = Call(wxT("GetBinaryDataFromString"), &arg, 1);
	const ibValueBinaryData* const bytes = BytesOf(utf8);
	ASSERT_NE(bytes, nullptr);
	ASSERT_EQ(bytes->GetLength(), 3u);
	EXPECT_EQ(std::memcmp(bytes->GetBuffer().GetData(), "abc", 3), 0);

	ibValue* back = const_cast<ibValue*>(&utf8);
	EXPECT_EQ(Call(wxT("GetStringFromBinaryData"), &back, 1).GetString(), wxT("abc"));

	ibValue letter(wxT("A"));
	ibValue encoding = ibValue::CreateEnumObject<ibValueEnumTextEncoding>(ibTextEncoding_UTF16);
	ibValue* wide[] = { &letter, &encoding };
	const ibValue utf16 = Call(wxT("GetBinaryDataFromString"), wide, 2);
	const ibValueBinaryData* const wideBytes = BytesOf(utf16);
	ASSERT_NE(wideBytes, nullptr);
	ASSERT_EQ(wideBytes->GetLength(), 2u);
	const unsigned char* const raw = static_cast<const unsigned char*>(wideBytes->GetBuffer().GetData());
	EXPECT_EQ(raw[0], 0x41);
	EXPECT_EQ(raw[1], 0x00);
	ibValue* wideBack[] = { const_cast<ibValue*>(&utf16), &encoding };
	EXPECT_EQ(Call(wxT("GetStringFromBinaryData"), wideBack, 2).GetString(), wxT("A"));
}

TEST(BinaryScript, Refusals) {
	ibValueSystemFunction sys;
	ibValue text(wxT("not-binary"));
	ibValue* arg = &text;
	ibValue ret;
	try {
		sys.CallAsFunc(sys.FindMethod(wxT("Base64String")), ret, &arg, 1);
		FAIL() << "a string is not binary";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("BinaryData")));
	}

	ibValue broken(wxT("!!!!"));
	arg = &broken;
	try {
		sys.CallAsFunc(sys.FindMethod(wxT("Base64Value")), ret, &arg, 1);
		FAIL() << "that text is not base64";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("base64")));
	}

	ibValue number(ibNumber(1));
	ibValue* badEnc[] = { &text, &number };
	try {
		sys.CallAsFunc(sys.FindMethod(wxT("GetBinaryDataFromString")), ret, badEnc, 2);
		FAIL() << "a number is not an encoding";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("TextEncoding")));
	}

	const unsigned char junk[] = { 0xFF };
	ibValue junkData(new ibValueBinaryData(junk, 1));
	arg = &junkData;
	try {
		sys.CallAsFunc(sys.FindMethod(wxT("GetStringFromBinaryData")), ret, &arg, 1);
		FAIL() << "those bytes are not UTF-8";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("not text")));
	}
}

TEST(BinaryScript, TheNameCompiles) {
	ibValueSystemFunction sys;
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	cc.AddContextVariable(wxT("System"), &sys, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check() {\n")
			wxT("    var text;\n")
			wxT("    text = GetStringFromBinaryData(GetBinaryDataFromString(\"abc\"));\n")
			wxT("    text = Base64String(Base64Value(\"YWJj\"));\n")
			wxT("}\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
