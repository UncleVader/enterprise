#include "valueWSDefinitions.h"

#include "backend/backend_exception.h"
#include "backend/compiler/typeCtor.h"
#include "backend/system/value/valueArray.h"
#include "backend/system/value/valueMap.h"

#include <cpp-httplib/httplib.h>

#include <memory>
#include <string>

#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/xml/xml.h>

enum
{
	eLocation,
	eServices
};

namespace {

bool Given(ibValue** paParams, long count, long at)
{
	return count > at && paParams[at] != nullptr && paParams[at]->GetType() != ibValueTypes::TYPE_EMPTY;
}

std::string Utf8(const wxString& text)
{
	const wxScopedCharBuffer buffer = text.utf8_str();
	return std::string(buffer.data(), buffer.length());
}

wxString LocalName(const wxString& name)
{
	const int colon = name.Find(wxT(':'));
	return colon == wxNOT_FOUND ? name : name.Mid(colon + 1);
}

bool IsHttp(const wxString& location, bool& secure, wxString& rest)
{
	if (location.StartsWith(wxT("https://"))) {
		secure = true;
		rest = location.Mid(8);
		return true;
	}
	if (location.StartsWith(wxT("http://"))) {
		secure = false;
		rest = location.Mid(7);
		return true;
	}
	return false;
}

// host[:port][/path][?query]. No user-info and no IPv6 literal: those are said as a bad location.
bool SplitHttp(const wxString& rest, bool secure, std::string& host, int& port, std::string& path)
{
	if (rest.empty() || rest.StartsWith(wxT("[")) || rest.Find(wxT('@')) != wxNOT_FOUND)
		return false;
	const int slash = rest.Find(wxT('/'));
	const wxString authority = slash == wxNOT_FOUND ? rest : rest.Left(slash);
	const wxString resource = slash == wxNOT_FOUND ? wxString(wxT("/")) : rest.Mid(slash);
	const int colon = authority.Find(wxT(':'));
	const wxString hostText = colon == wxNOT_FOUND ? authority : authority.Left(colon);
	port = secure ? 443 : 80;
	if (colon != wxNOT_FOUND) {
		long parsed = 0;
		if (!authority.Mid(colon + 1).ToLong(&parsed) || parsed < 1 || parsed > 65535)
			return false;
		port = static_cast<int>(parsed);
	}
	if (hostText.empty())
		return false;
	host = Utf8(hostText);
	path = Utf8(resource);
	return true;
}

wxString Fetch(const wxString& location, bool secure, const std::string& host, int port, const std::string& path,
	const wxString& user, const wxString& password, int timeout)
{
	std::unique_ptr<httplib::ClientImpl> client;
	if (secure) {
#ifdef CPPHTTPLIB_SSL_ENABLED
		client.reset(new httplib::SSLClient(host, port));
#else
		ibBackendCoreException::Error(_("WSDefinitions: this build has no TLS - '%s' cannot be loaded"), location);
		return wxString();
#endif
	}
	else {
		client.reset(new httplib::ClientImpl(host, port));
	}
	client->set_connection_timeout(timeout, 0);
	client->set_read_timeout(timeout, 0);
	client->set_write_timeout(timeout, 0);

	httplib::Headers headers;
	if (!user.empty())
		headers.insert(httplib::make_basic_authentication_header(Utf8(user), Utf8(password)));
	const httplib::Result result = client->Get(path, headers);
	if (!result)
		ibBackendCoreException::Error(_("WSDefinitions: cannot load '%s'"), location);
	if (result->status < 200 || result->status >= 300)
		ibBackendCoreException::Error(_("WSDefinitions: '%s' answered %d"), location, result->status);
	return wxString::FromUTF8(result->body.data(), result->body.size());
}

ibValue ReadDocument(const wxXmlDocument& xml, const wxString& location)
{
	const wxXmlNode* const root = xml.GetRoot();
	if (root == nullptr || !LocalName(root->GetName()).IsSameAs(wxT("definitions")))
		ibBackendCoreException::Error(_("WSDefinitions: '%s' is not a WSDL definitions document"), location);

	const wxString targetNamespace = root->GetAttribute(wxT("targetNamespace"), wxEmptyString);
	ibValueArray* const services = new ibValueArray();
	for (wxXmlNode* serviceNode = root->GetChildren(); serviceNode != nullptr; serviceNode = serviceNode->GetNext()) {
		if (serviceNode->GetType() != wxXML_ELEMENT_NODE || !LocalName(serviceNode->GetName()).IsSameAs(wxT("service")))
			continue;
		ibValueStructure* const service = new ibValueStructure();
		service->Insert(wxT("Name"), serviceNode->GetAttribute(wxT("name"), wxEmptyString));
		service->Insert(wxT("NamespaceURI"), targetNamespace);
		ibValueArray* const ports = new ibValueArray();
		for (wxXmlNode* portNode = serviceNode->GetChildren(); portNode != nullptr; portNode = portNode->GetNext()) {
			if (portNode->GetType() != wxXML_ELEMENT_NODE || !LocalName(portNode->GetName()).IsSameAs(wxT("port")))
				continue;
			wxString address;
			for (wxXmlNode* addressNode = portNode->GetChildren(); addressNode != nullptr; addressNode = addressNode->GetNext()) {
				if (addressNode->GetType() == wxXML_ELEMENT_NODE && LocalName(addressNode->GetName()).IsSameAs(wxT("address"))) {
					address = addressNode->GetAttribute(wxT("location"), wxEmptyString);
					break;
				}
			}
			ibValueStructure* const port = new ibValueStructure();
			port->Insert(wxT("Name"), portNode->GetAttribute(wxT("name"), wxEmptyString));
			port->Insert(wxT("Location"), address);
			ports->Add(port);
		}
		service->Insert(wxT("Ports"), ports);
		services->Add(service);
	}
	return services;
}

ibValue LoadWsdl(const wxString& location, const wxString& user, const wxString& password, int timeout)
{
	wxXmlDocument xml;
	wxLogNull quiet;
	bool secure = false;
	wxString rest;
	if (IsHttp(location, secure, rest)) {
		std::string host;
		std::string path;
		int port = 0;
		if (!SplitHttp(rest, secure, host, port, path))
			ibBackendCoreException::Error(_("WSDefinitions: '%s' is not a WSDL location"), location);
		wxStringInputStream in(Fetch(location, secure, host, port, path, user, password, timeout));
		if (!xml.Load(in))
			ibBackendCoreException::Error(_("WSDefinitions: '%s' is not a WSDL definitions document"), location);
	}
	else if (!xml.Load(location)) {
		ibBackendCoreException::Error(_("WSDefinitions: '%s' is not a WSDL definitions document"), location);
	}
	return ReadDocument(xml, location);
}

} // namespace

void ibValueWSDefinitions_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(1, wxT("WSDefinitions(location : string)"));
	helper.AppendConstructor(7, wxT("WSDefinitions(location : string, user : string, password : string, proxy, timeout : number, secureConnection, useOSAuthentication : boolean)"));
	helper.AppendProp(wxT("Location"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("Services"), true, false, wxNOT_FOUND);
}

bool ibValueWSDefinitions::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1 || lSizeArray > 7 || paParams[0] == nullptr)
		return false;
	if (paParams[0]->GetType() != ibValueTypes::TYPE_STRING)
		ibBackendCoreException::Error(_("WSDefinitions: the location is a string"));
	const wxString location(paParams[0]->GetString());
	if (location.empty())
		ibBackendCoreException::Error(_("WSDefinitions: the WSDL location is empty"));

	wxString user;
	wxString password;
	if (Given(paParams, lSizeArray, 1)) {
		if (paParams[1]->GetType() != ibValueTypes::TYPE_STRING)
			ibBackendCoreException::Error(_("WSDefinitions: the user is a string"));
		user = paParams[1]->GetString();
	}
	if (Given(paParams, lSizeArray, 2)) {
		if (paParams[2]->GetType() != ibValueTypes::TYPE_STRING)
			ibBackendCoreException::Error(_("WSDefinitions: the password is a string"));
		password = paParams[2]->GetString();
	}
	if (Given(paParams, lSizeArray, 3))
		ibBackendCoreException::Error(_("WSDefinitions: an InternetProxy is not used from here"));

	int timeout = 30;
	if (Given(paParams, lSizeArray, 4)) {
		if (paParams[4]->GetType() != ibValueTypes::TYPE_NUMBER)
			ibBackendCoreException::Error(_("WSDefinitions: the timeout is a number of seconds"));
		timeout = paParams[4]->GetInteger();
		if (timeout < 1 || timeout > 120)
			ibBackendCoreException::Error(_("WSDefinitions: the timeout is 1 to 120 seconds, and %d was given"), timeout);
	}
	if (Given(paParams, lSizeArray, 5))
		ibBackendCoreException::Error(_("WSDefinitions: a secure connection is not used from here"));
	if (Given(paParams, lSizeArray, 6)) {
		if (paParams[6]->GetType() != ibValueTypes::TYPE_BOOLEAN || paParams[6]->GetBoolean())
			ibBackendCoreException::Error(_("WSDefinitions: Windows authentication is not used from here"));
	}

	m_location = location;
	m_services = LoadWsdl(location, user, password, timeout);
	return true;
}

bool ibValueWSDefinitions::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case eLocation:
		pvarPropVal = m_location;
		return true;
	case eServices:
		pvarPropVal = m_services;
		return true;
	}
	return false;
}

VALUE_TYPE_REGISTER(ibValueWSDefinitions, "WSDefinitions", value_to_clsid("VL_WSDFN"));
