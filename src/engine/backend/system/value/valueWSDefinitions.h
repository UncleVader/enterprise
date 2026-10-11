#ifndef __VALUE_WS_DEFINITIONS_H__
#define __VALUE_WS_DEFINITIONS_H__

#include "backend/compiler/value.h"

// WSDefinitions is WSОпределения: a WSDL document, read when the value is made.
// New WSDefinitions(location) reads a file, or GET on http:// and https://.
// Services is an array of structures: Name, NamespaceURI, and Ports
// (Name, Location). A proxy, a secure-connection object, and Windows
// authentication are refused — they are not applied.

void ibValueWSDefinitions_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueWSDefinitions : public ibValueStaticMembers<&ibValueWSDefinitions_BindNames> {
public:
	ibValueWSDefinitions() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true) {}

	virtual bool Init() { return false; }
	virtual bool Init(ibValue** paParams, const long lSizeArray);

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);

	const wxString& Location() const { return m_location; }
	const ibValue& Services() const { return m_services; }

private:
	wxString m_location;
	ibValue  m_services;
};

#endif
