#include "valueInternetMail.h"

void ibValueInternetMailProfile_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("InternetMailProfile()"));

	helper.AppendProp(wxT("SMTPServerAddress"));
	helper.AppendProp(wxT("SMTPPort"));
	helper.AppendProp(wxT("SMTPUser"));
	helper.AppendProp(wxT("SMTPPassword"));
	helper.AppendProp(wxT("SMTPAuthentication"));
	helper.AppendProp(wxT("SMTPUseSSL"));
	helper.AppendProp(wxT("POP3ServerAddress"));
	helper.AppendProp(wxT("POP3Port"));
	helper.AppendProp(wxT("POP3User"));
	helper.AppendProp(wxT("POP3Password"));
	helper.AppendProp(wxT("POP3UseSSL"));
	helper.AppendProp(wxT("IMAPServerAddress"));
	helper.AppendProp(wxT("IMAPPort"));
	helper.AppendProp(wxT("Timeout"));
}

ibValueInternetMailProfile::ibValueInternetMailProfile()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
{
}

bool ibValueInternetMailProfile::Init()
{
	return true;
}

bool ibValueInternetMailProfile::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case enSMTPServerAddress: pvarPropVal = m_smtpServer; return true;
	case enSMTPPort:          pvarPropVal = m_smtpPort; return true;
	case enSMTPUser:          pvarPropVal = m_smtpUser; return true;
	case enSMTPPassword:      pvarPropVal = m_smtpPassword; return true;
	case enSMTPAuthentication: pvarPropVal = m_smtpAuthentication; return true;
	case enSMTPUseSSL:        pvarPropVal = m_smtpUseSSL; return true;
	case enPOP3ServerAddress: pvarPropVal = m_pop3Server; return true;
	case enPOP3Port:          pvarPropVal = m_pop3Port; return true;
	case enPOP3User:          pvarPropVal = m_pop3User; return true;
	case enPOP3Password:      pvarPropVal = m_pop3Password; return true;
	case enPOP3UseSSL:        pvarPropVal = m_pop3UseSSL; return true;
	case enIMAPServerAddress: pvarPropVal = m_imapServer; return true;
	case enIMAPPort:          pvarPropVal = m_imapPort; return true;
	case enTimeout:           pvarPropVal = m_timeout; return true;
	}
	return false;
}

bool ibValueInternetMailProfile::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	switch (lPropNum) {
	case enSMTPServerAddress: m_smtpServer = varPropVal.GetString(); return true;
	case enSMTPPort:          m_smtpPort = varPropVal.GetInteger(); return true;
	case enSMTPUser:          m_smtpUser = varPropVal.GetString(); return true;
	case enSMTPPassword:      m_smtpPassword = varPropVal.GetString(); return true;
	case enSMTPAuthentication: m_smtpAuthentication = varPropVal; return true;
	case enSMTPUseSSL:        m_smtpUseSSL = varPropVal.GetBoolean(); return true;
	case enPOP3ServerAddress: m_pop3Server = varPropVal.GetString(); return true;
	case enPOP3Port:          m_pop3Port = varPropVal.GetInteger(); return true;
	case enPOP3User:          m_pop3User = varPropVal.GetString(); return true;
	case enPOP3Password:      m_pop3Password = varPropVal.GetString(); return true;
	case enPOP3UseSSL:        m_pop3UseSSL = varPropVal.GetBoolean(); return true;
	case enIMAPServerAddress: m_imapServer = varPropVal.GetString(); return true;
	case enIMAPPort:          m_imapPort = varPropVal.GetInteger(); return true;
	case enTimeout:           m_timeout = varPropVal.GetInteger(); return true;
	}
	return false;
}

VALUE_TYPE_REGISTER(ibValueInternetMailProfile, "InternetMailProfile", value_to_clsid("VL_IMPRF"));
