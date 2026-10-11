#ifndef __VALUE_INTERNET_MAIL_H__
#define __VALUE_INTERNET_MAIL_H__

#include "backend/compiler/value.h"

// InternetMailProfile is ИнтернетПочтовыйПрофиль. New InternetMailProfile was
// "Constructor not found". The fields a profile is given before logon live here.
// Sending the mail is InternetMail, a separate type.

void ibValueInternetMailProfile_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueInternetMailProfile : public ibValueStaticMembers<&ibValueInternetMailProfile_BindNames> {
	enum Prop {
		enSMTPServerAddress = 0,
		enSMTPPort,
		enSMTPUser,
		enSMTPPassword,
		enSMTPAuthentication,
		enSMTPUseSSL,
		enPOP3ServerAddress,
		enPOP3Port,
		enPOP3User,
		enPOP3Password,
		enPOP3UseSSL,
		enIMAPServerAddress,
		enIMAPPort,
		enTimeout
	};
public:
	ibValueInternetMailProfile();

	virtual bool Init();
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);
	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);

	const wxString& GetSMTPServerAddress() const { return m_smtpServer; }
	int GetSMTPPort() const { return m_smtpPort; }

private:
	wxString m_smtpServer;
	int      m_smtpPort = 25;
	wxString m_smtpUser;
	wxString m_smtpPassword;
	ibValue  m_smtpAuthentication;
	bool     m_smtpUseSSL = false;

	wxString m_pop3Server;
	int      m_pop3Port = 110;
	wxString m_pop3User;
	wxString m_pop3Password;
	bool     m_pop3UseSSL = false;

	wxString m_imapServer;
	int      m_imapPort = 143;
	int      m_timeout = 30;
};

#endif
