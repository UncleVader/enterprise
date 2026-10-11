#include <gtest/gtest.h>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueBinaryData.h"
#include "backend/system/value/valueDataHashing.h"

namespace {

wxString Hex(const ibValue& value)
{
	const ibValueBinaryData* const data = dynamic_cast<const ibValueBinaryData*>(value.GetRef());
	EXPECT_NE(data, nullptr);
	if (data == nullptr)
		return wxString();
	wxString out;
	const unsigned char* const bytes = static_cast<const unsigned char*>(data->GetBuffer().GetData());
	for (size_t i = 0; i < data->GetLength(); ++i)
		out += wxString::Format(wxT("%02x"), static_cast<int>(bytes[i]));
	return out;
}

ibValue Member(const wxString& name)
{
	ibValue owner = ibValue::CreateObject(wxT("HashFunction"));
	const long prop = owner.FindProp(name);
	EXPECT_GE(prop, 0) << name.ToStdString();
	ibValue member;
	EXPECT_TRUE(owner.GetPropVal(prop, member));
	return member;
}

ibValue Make(const wxString& algorithm, ibValue* key = nullptr)
{
	ibValue algo = Member(algorithm);
	ibValue* args[] = { &algo, key };
	return ibValue::CreateObject(wxT("DataHashing"), args, key == nullptr ? 1 : 2);
}

void AppendText(ibValue& hash, const wxString& text)
{
	ibValue chunk(text);
	ibValue* arg = &chunk;
	EXPECT_TRUE(hash.CallAsProc(hash.FindMethod(wxT("Append")), &arg, 1));
}

ibValue Sum(ibValue& hash)
{
	ibValue sum;
	EXPECT_TRUE(hash.GetPropVal(hash.FindProp(wxT("HashSum")), sum));
	return sum;
}

}

TEST(DataHashing, DigestsAndHmac) {
	ibValue sha = Make(wxT("SHA256"));
	AppendText(sha, wxT("abc"));
	EXPECT_EQ(Hex(Sum(sha)),
		wxT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
	EXPECT_EQ(Hex(Sum(sha)),
		wxT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

	ibValue split = Make(wxT("SHA256"));
	AppendText(split, wxT("ab"));
	AppendText(split, wxT("c"));
	EXPECT_EQ(Hex(Sum(split)),
		wxT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));

	ibValue md5 = Make(wxT("MD5"));
	AppendText(md5, wxT("abc"));
	EXPECT_EQ(Hex(Sum(md5)), wxT("900150983cd24fb0d6963f7d28e17f72"));

	ibValue sha1 = Make(wxT("SHA1"));
	AppendText(sha1, wxT("abc"));
	EXPECT_EQ(Hex(Sum(sha1)), wxT("a9993e364706816aba3e25717850c26c9cd0d89d"));

	ibValue empty = Make(wxT("SHA256"));
	EXPECT_EQ(Hex(Sum(empty)),
		wxT("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

	ibValue key(wxT("key"));
	ibValue hmac = Make(wxT("SHA256"), &key);
	AppendText(hmac, wxT("The quick brown fox jumps over the lazy dog"));
	EXPECT_EQ(Hex(Sum(hmac)),
		wxT("f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8"));

	ibValue binary(new ibValueBinaryData("abc", 3));
	ibValue fromBytes = Make(wxT("MD5"));
	ibValue* arg = &binary;
	EXPECT_TRUE(fromBytes.CallAsProc(fromBytes.FindMethod(wxT("Append")), &arg, 1));
	EXPECT_EQ(Hex(Sum(fromBytes)), wxT("900150983cd24fb0d6963f7d28e17f72"));
}

TEST(DataHashing, Refusals) {
	ibValue sha = Make(wxT("SHA256"));
	AppendText(sha, wxT("abc"));
	EXPECT_FALSE(Sum(sha).IsEmpty());
	ibValue more(wxT("d"));
	ibValue* arg = &more;
	try {
		sha.CallAsProc(sha.FindMethod(wxT("Append")), &arg, 1);
		FAIL() << "the sum was already taken";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("already taken")));
	}

	ibValue number(ibNumber(1));
	arg = &number;
	try {
		ibValue::CreateObject(wxT("DataHashing"), &arg, 1);
		FAIL() << "a number is not an algorithm";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("HashFunction")));
	}

	try {
		ibValue::CreateObject(wxT("DataHashing"));
		FAIL() << "an algorithm is required";
	}
	catch (const ibBackendException& err) {
		EXPECT_TRUE(err.GetErrorDescription().Contains(wxT("DataHashing")));
	}
}

TEST(DataHashing, TheNameCompiles) {
	ibValue enums = ibValue::CreateObject(wxT("EnumManager"));
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	cc.AddContextVariable(wxT("EnumManager"), &enums, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check() {\n")
			wxT("    var hash;\n")
			wxT("    var sum;\n")
			wxT("    hash = New DataHashing(HashFunction.SHA256);\n")
			wxT("    hash.Append(\"abc\");\n")
			wxT("    sum = hash.HashSum;\n")
			wxT("}\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
