#include "valueInternetMailSession.h"

#include "backend/backend_exception.h"
#include "backend/system/value/valueArray.h"

#include <wx/base64.h>
#include <wx/socket.h>

#include <memory>

namespace {

bool TakeProp(ibValue& object, const wxString& name, ibValue& out)
{
	const long index = object.FindProp(name);
	if (index < 0)
		return false;
	return object.GetPropVal(index, out);
}

wxString PropString(ibValue& object, const wxString& name)
{
	ibValue value;
	if (!TakeProp(object, name, value))
		return wxString();
	return value.GetString();
}

} // namespace

bool ibValueInternetMailAddress::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (lPropNum == enAddress) { pvarPropVal = m_address; return true; }
	if (lPropNum == enDisplayName) { pvarPropVal = m_displayName; return true; }
	return false;
}

bool ibValueInternetMailAddress::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (lPropNum == enAddress) { m_address = varPropVal.GetString(); return true; }
	if (lPropNum == enDisplayName) { m_displayName = varPropVal.GetString(); return true; }
	return false;
}

void ibValueInternetMailAddress_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("InternetMailAddress()"));
	helper.AppendProp(wxT("Address"));
	helper.AppendProp(wxT("DisplayName"));
}

ibValueInternetMailMessage::ibValueInternetMailMessage()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
{
	m_from = new ibValueInternetMailAddress();
	m_to = new ibValueArray();
	m_texts = new ibValueArray();
}

bool ibValueInternetMailMessage::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case enSubject: pvarPropVal = m_subject; return true;
	case enFrom:    pvarPropVal = m_from; return true;
	case enTo:      pvarPropVal = m_to; return true;
	case enTexts:   pvarPropVal = m_texts; return true;
	}
	return false;
}

bool ibValueInternetMailMessage::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (lPropNum == enSubject) {
		m_subject = varPropVal.GetString();
		return true;
	}
	if (lPropNum == enFrom) {
		ibValueInternetMailAddress* address = nullptr;
		if (m_from.ConvertToValue(address) && address != nullptr)
			address->SetPropVal(ibValueInternetMailAddress::enAddress, varPropVal);
		return true;
	}
	return false;
}

void ibValueInternetMailMessage_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("InternetMailMessage()"));
	helper.AppendProp(wxT("Subject"));
	helper.AppendProp(wxT("From"));
	helper.AppendProp(wxT("To"), true, false, ibValueInternetMailMessage::enTo);
	helper.AppendProp(wxT("Texts"), true, false, ibValueInternetMailMessage::enTexts);
}

// The socket lives here so the header does not mention wx.
struct ibInternetMailLink {
	wxSocketClient socket;
	bool           open = false;

	~ibInternetMailLink() { socket.Close(); }

	wxString ReadLine()
	{
		wxString line;
		char ch = 0;
		while (socket.Read(&ch, 1).LastCount() == 1 && ch != '\n') {
			if (ch != '\r')
				line += wxUniChar(static_cast<unsigned char>(ch));
		}
		if (socket.Error() && line.IsEmpty())
			ibBackendCoreException::Error(_("InternetMail: the server closed the connection"));
		return line;
	}

	int ReadReply(wxString& text)
	{
		text.clear();
		int code = 0;
		for (;;) {
			const wxString line = ReadLine();
			if (line.Length() < 3)
				ibBackendCoreException::Error(_("InternetMail: the server closed the connection"));
			if (!text.IsEmpty())
				text += wxT("\n");
			text += line;
			code = wxAtoi(line.Left(3));
			if (line.Length() < 4 || line[3] != wxT('-'))
				break;
		}
		return code;
	}

	void WriteLine(const wxString& line)
	{
		const wxString data = line + wxT("\r\n");
		const wxScopedCharBuffer bytes = data.utf8_str();
		socket.Write(bytes.data(), bytes.length());
		if (socket.Error())
			ibBackendCoreException::Error(_("InternetMail: the server did not accept the text"));
	}

	void Expect(int want)
	{
		wxString text;
		const int code = ReadReply(text);
		if (code != want)
			ibBackendCoreException::Error(_("InternetMail: the server answered %d (%s)"), code, text);
	}
};

ibValueInternetMail::ibValueInternetMail()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
{
}

ibValueInternetMail::~ibValueInternetMail() = default;

bool ibValueInternetMail::Init()
{
	return true;
}

bool ibValueInternetMail::IsLoggedOn() const
{
	return m_link != nullptr && m_link->open;
}

void ibValueInternetMail_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("InternetMail()"));
	helper.AppendProc(wxT("Logon"), 1, wxT("Logon(profile)"));
	helper.AppendProc(wxT("Send"), 1, wxT("Send(message)"));
	helper.AppendProc(wxT("Logoff"), wxT("Logoff()"));
}

bool ibValueInternetMail::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
{
	if (lMethodNum == enLogoff) {
		if (m_link != nullptr && m_link->open) {
			m_link->WriteLine(wxT("QUIT"));
			m_link->socket.Close();
			m_link->open = false;
		}
		m_link.reset();
		return true;
	}

	if (lMethodNum == enLogon) {
		if (lSizeArray < 1 || paParams[0] == nullptr)
			return false;
		ibValue& profile = *paParams[0];

		ibValue ssl;
		if (TakeProp(profile, wxT("SMTPUseSSL"), ssl) && ssl.GetBoolean())
			ibBackendCoreException::Error(_("InternetMail: a secure connection is not sent from here"));

		const wxString host = PropString(profile, wxT("SMTPServerAddress"));
		if (host.IsEmpty())
			ibBackendCoreException::Error(_("InternetMail: the profile has no SMTP server"));

		ibValue portValue;
		const int port = TakeProp(profile, wxT("SMTPPort"), portValue) && portValue.GetInteger() > 0
			? portValue.GetInteger() : 25;
		ibValue timeoutValue;
		const int timeout = TakeProp(profile, wxT("Timeout"), timeoutValue) && timeoutValue.GetInteger() > 0
			? timeoutValue.GetInteger() : 30;
		const wxString user = PropString(profile, wxT("SMTPUser"));
		const wxString password = PropString(profile, wxT("SMTPPassword"));

		wxSocketBase::Initialize();
		auto link = std::make_unique<ibInternetMailLink>();
		link->socket.SetTimeout(timeout);

		wxIPV4address address;
		if (!address.Hostname(host) || !address.Service(port) || !link->socket.Connect(address, true))
			ibBackendCoreException::Error(_("InternetMail: cannot connect to '%s:%d'"), host, port);

		link->Expect(220);
		link->WriteLine(wxT("EHLO oes"));
		link->Expect(250);
		if (!user.IsEmpty()) {
			link->WriteLine(wxT("AUTH LOGIN"));
			link->Expect(334);
			const wxScopedCharBuffer userBytes = user.utf8_str();
			const wxScopedCharBuffer passBytes = password.utf8_str();
			link->WriteLine(wxBase64Encode(userBytes.data(), userBytes.length()));
			link->Expect(334);
			link->WriteLine(wxBase64Encode(passBytes.data(), passBytes.length()));
			link->Expect(235);
		}
		link->open = true;
		m_link = std::move(link);
		return true;
	}

	if (lMethodNum != enSend)
		return false;
	if (lSizeArray < 1 || paParams[0] == nullptr)
		return false;
	if (!IsLoggedOn())
		ibBackendCoreException::Error(_("InternetMail: logon first"));

	ibValue& message = *paParams[0];
	ibValue fromValue;
	if (!TakeProp(message, wxT("From"), fromValue))
		ibBackendCoreException::Error(_("InternetMail: the message has no sender"));
	const wxString from = PropString(fromValue, wxT("Address"));
	if (from.IsEmpty())
		ibBackendCoreException::Error(_("InternetMail: the message has no sender"));

	ibValue toValue;
	if (!TakeProp(message, wxT("To"), toValue))
		ibBackendCoreException::Error(_("InternetMail: the message has no recipient"));
	ibValueArray* recipients = dynamic_cast<ibValueArray*>(toValue.GetRef());
	if (recipients == nullptr || recipients->Count() == 0)
		ibBackendCoreException::Error(_("InternetMail: the message has no recipient"));

	const wxString subject = PropString(message, wxT("Subject"));
	wxString body;
	ibValue textsValue;
	if (TakeProp(message, wxT("Texts"), textsValue)) {
		if (ibValueArray* texts = dynamic_cast<ibValueArray*>(textsValue.GetRef())) {
			for (unsigned int i = 0; i < texts->Count(); ++i) {
				if (i > 0)
					body += wxT("\r\n");
				body += texts->Values()[i].GetString();
			}
		}
	}

	m_link->WriteLine(wxT("MAIL FROM:<") + from + wxT(">"));
	m_link->Expect(250);
	for (const ibValue& recipient : recipients->Values()) {
		wxString address = recipient.GetString();
		ibValue nested;
		if (TakeProp(const_cast<ibValue&>(recipient), wxT("Address"), nested) && !nested.GetString().IsEmpty())
			address = nested.GetString();
		m_link->WriteLine(wxT("RCPT TO:<") + address + wxT(">"));
		m_link->Expect(250);
	}
	m_link->WriteLine(wxT("DATA"));
	m_link->Expect(354);
	m_link->WriteLine(wxT("Subject: ") + subject);
	m_link->WriteLine(wxT("From: ") + from);
	m_link->WriteLine(wxString());
	for (size_t start = 0; start < body.length(); ) {
		const size_t brk = body.find(wxT('\n'), start);
		wxString line = brk == wxString::npos ? body.Mid(start) : body.Mid(start, brk - start);
		if (!line.IsEmpty() && line.Last() == wxT('\r'))
			line.RemoveLast();
		if (line.StartsWith(wxT(".")))
			line.Prepend(wxT("."));
		m_link->WriteLine(line);
		if (brk == wxString::npos)
			break;
		start = brk + 1;
	}
	m_link->WriteLine(wxT("."));
	m_link->Expect(250);
	return true;
}

VALUE_TYPE_REGISTER(ibValueInternetMailAddress, "InternetMailAddress", value_to_clsid("VL_IMADR"));
VALUE_TYPE_REGISTER(ibValueInternetMailMessage, "InternetMailMessage", value_to_clsid("VL_IMMSG"));
VALUE_TYPE_REGISTER(ibValueInternetMail, "InternetMail", value_to_clsid("VL_IMAIL"));
