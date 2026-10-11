// WindowsClientCertificate is СертификатКлиентаWindows. Without it,
// New WindowsClientCertificate is "Constructor not found", and
// WindowsCertificateSelectMode.Auto is "var is not found".

#include <gtest/gtest.h>

#include <string>

#include "backend/backend_exception.h"
#include "backend/compiler/compileCode.h"
#include "backend/compiler/value.h"
#include "backend/system/systemManager.h"

namespace {

ibValue Member(const wxChar* name)
{
	ibValue modes = ibValue::CreateObject(wxT("WindowsCertificateSelectMode"));
	ibValue member;
	EXPECT_TRUE(modes.GetPropVal(modes.FindProp(name), member));
	return member;
}

wxString ModeName(ibValue& certificate)
{
	ibValue mode;
	EXPECT_TRUE(certificate.GetPropVal(certificate.FindProp(wxT("SelectionMode")), mode));
	return mode.GetString();
}

} // namespace

TEST(WindowsClientCertificate, AutoAndChooseAreTheSelection) {
	const ibValue modes = ibValue::CreateObject(wxT("WindowsCertificateSelectMode"));
	EXPECT_GE(modes.FindProp(wxT("Auto")), 0);
	EXPECT_GE(modes.FindProp(wxT("Choose")), 0);
	EXPECT_EQ(modes.FindProp(wxT("Select")), wxNOT_FOUND);

	ibValue plain = ibValue::CreateObject(wxT("WindowsClientCertificate"));
	ASSERT_TRUE(plain.IsReference());
	EXPECT_EQ(ModeName(plain), wxT("Auto"));

	ibValue choose = Member(wxT("Choose"));
	ibValue* params = &choose;
	ibValue asked = ibValue::CreateObject(wxT("WindowsClientCertificate"), &params, 1);
	EXPECT_EQ(ModeName(asked), wxT("Choose"));

	ibValue number(ibNumber(0));
	params = &number;
	try {
		ibValue::CreateObject(wxT("WindowsClientCertificate"), &params, 1);
		FAIL() << "a number is not a selection mode";
	}
	catch (const ibBackendException& err) {
		EXPECT_NE(err.GetErrorDescription().find(wxT("WindowsCertificateSelectMode")), wxString::npos)
			<< err.GetErrorDescription().ToStdString();
	}
}

TEST(WindowsClientCertificate, TheConstructorCompiles) {
	const ibValue enums = ibValue::CreateObject(wxT("EnumManager"));
	ASSERT_GE(enums.FindProp(wxT("WindowsCertificateSelectMode")), 0);

	ibCompileCode cc(wxT("test"), wxT("memory"), false);
	ibValueSystemFunction valueSystem;
	cc.AddContextVariable(wxT("System"), &valueSystem, true);
	cc.AddContextVariable(wxT("EnumManager"), enums, true);
	try {
		ASSERT_TRUE(cc.Compile(
			wxT("Procedure Check()\n")
			wxT("    var cert;\n")
			wxT("    cert = New WindowsClientCertificate;\n")
			wxT("    cert = New WindowsClientCertificate(WindowsCertificateSelectMode.Auto);\n")
			wxT("    cert = New WindowsClientCertificate(WindowsCertificateSelectMode.Choose);\n")
			wxT("EndProcedure\n")));
	}
	catch (const ibBackendException& err) {
		FAIL() << err.GetErrorDescription().ToStdString();
	}
}
