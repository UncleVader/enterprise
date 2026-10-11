// InternetMailProfile is ИнтернетПочтовыйПрофиль. Without it, New InternetMailProfile
// is "Constructor not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/value/valueInternetMail.h"

namespace {

ibValue Prop(ibValue& object, const wxString& name)
{
	ibValue value;
	EXPECT_TRUE(object.GetPropVal(object.FindProp(name), value)) << name.ToStdString();
	return value;
}

void Set(ibValue& object, const wxString& name, const ibValue& value)
{
	ASSERT_TRUE(object.SetPropVal(object.FindProp(name), value)) << name.ToStdString();
}

} // namespace

TEST(InternetMailProfile, TheFieldsALogonIsGiven) {
	ibValue profile = ibValue::CreateObject(wxT("InternetMailProfile"));
	ASSERT_TRUE(profile.IsReference());
	EXPECT_EQ(Prop(profile, wxT("SMTPPort")).GetInteger(), 25);
	EXPECT_EQ(Prop(profile, wxT("POP3Port")).GetInteger(), 110);
	EXPECT_EQ(Prop(profile, wxT("IMAPPort")).GetInteger(), 143);
	EXPECT_EQ(Prop(profile, wxT("Timeout")).GetInteger(), 30);
	EXPECT_FALSE(Prop(profile, wxT("SMTPUseSSL")).GetBoolean());

	Set(profile, wxT("SMTPServerAddress"), ibValue(wxT("smtp.example.com")));
	Set(profile, wxT("SMTPPort"), ibValue(ibNumber(587)));
	Set(profile, wxT("SMTPUser"), ibValue(wxT("ada")));
	Set(profile, wxT("SMTPPassword"), ibValue(wxT("secret")));
	Set(profile, wxT("SMTPUseSSL"), ibValue(true));
	Set(profile, wxT("SMTPAuthentication"), ibValue(wxT("Login")));

	EXPECT_EQ(Prop(profile, wxT("SMTPServerAddress")).GetString(), wxString(wxT("smtp.example.com")));
	EXPECT_EQ(Prop(profile, wxT("SMTPPort")).GetInteger(), 587);
	EXPECT_EQ(Prop(profile, wxT("SMTPUser")).GetString(), wxString(wxT("ada")));
	EXPECT_EQ(Prop(profile, wxT("SMTPPassword")).GetString(), wxString(wxT("secret")));
	EXPECT_TRUE(Prop(profile, wxT("SMTPUseSSL")).GetBoolean());
	EXPECT_EQ(Prop(profile, wxT("SMTPAuthentication")).GetString(), wxString(wxT("Login")));
}

TEST(InternetMailProfile, TheConstructorCompiles) {
	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var profile;\n")
			wxT("    profile = New InternetMailProfile;\n")
			wxT("    profile.SMTPServerAddress = \"smtp.example.com\";\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
