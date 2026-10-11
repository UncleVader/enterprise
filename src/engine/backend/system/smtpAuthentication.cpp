#include "backend/compiler/enumUnit.h"
#include "backend/compiler/typeCtor.h"

// Unscoped, and only in this file: a scoped enum does not register
// (enumUnit.h), and these names must not leak into other translation units.
// СпособSMTPАутентификации: the names a mail profile already writes.
enum ibSmtpAuthenticationMode
{
	ibSmtpAuthenticationMode_None = 0,
	ibSmtpAuthenticationMode_Plain,
	ibSmtpAuthenticationMode_Login,
	ibSmtpAuthenticationMode_CramMD5,
	ibSmtpAuthenticationMode_Default
};

class ibValueEnumSmtpAuthenticationMode : public ibValueEnumeration<ibSmtpAuthenticationMode> {
public:
	ibValueEnumSmtpAuthenticationMode() : ibValueEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(ibSmtpAuthenticationMode_None,    wxT("None"),    wxT("None"));
		AddEnumeration(ibSmtpAuthenticationMode_Plain,   wxT("Plain"),   wxT("Plain"));
		AddEnumeration(ibSmtpAuthenticationMode_Login,   wxT("Login"),   wxT("Login"));
		AddEnumeration(ibSmtpAuthenticationMode_CramMD5, wxT("CramMD5"), wxT("CramMD5"));
		AddEnumeration(ibSmtpAuthenticationMode_Default, wxT("Default"), wxT("Default"));
	}
};

ENUM_TYPE_REGISTER(ibValueEnumSmtpAuthenticationMode, "SMTPAuthenticationMode", enum_to_clsid("EN_SMTP"));
