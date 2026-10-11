#ifndef __VALUE_INTERNET_MAIL_SESSION_H__
#define __VALUE_INTERNET_MAIL_SESSION_H__

#include "backend/compiler/value.h"

#include <memory>

// InternetMail is ИнтернетПочта. New InternetMail was "Constructor not found".
// Logon speaks SMTP to the server named by the profile. A secure (SSL) logon is refused:
// this session speaks plain SMTP.

struct ibInternetMailLink;

void ibValueInternetMailAddress_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);
void ibValueInternetMailMessage_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);
void ibValueInternetMail_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueInternetMailAddress : public ibValueStaticMembers<&ibValueInternetMailAddress_BindNames> {
public:
	enum { enAddress = 0, enDisplayName };
	ibValueInternetMailAddress() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE) {}

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);
	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);

	const wxString& GetAddress() const { return m_address; }

private:
	wxString m_address;
	wxString m_displayName;
};

class BACKEND_API ibValueInternetMailMessage : public ibValueStaticMembers<&ibValueInternetMailMessage_BindNames> {
public:
	enum { enSubject = 0, enFrom, enTo, enTexts };
	ibValueInternetMailMessage();

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);
	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);

private:
	wxString m_subject;
	ibValue  m_from;
	ibValue  m_to;
	ibValue  m_texts;
};

class BACKEND_API ibValueInternetMail : public ibValueStaticMembers<&ibValueInternetMail_BindNames> {
public:
	enum { enLogon = 0, enSend, enLogoff };
	ibValueInternetMail();
	virtual ~ibValueInternetMail();

	virtual bool Init();
	virtual bool CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray);

	bool IsLoggedOn() const;

private:
	std::unique_ptr<ibInternetMailLink> m_link;
};

#endif
