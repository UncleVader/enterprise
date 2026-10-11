#ifndef __VALUE_WINDOWS_CLIENT_CERTIFICATE_H__
#define __VALUE_WINDOWS_CLIENT_CERTIFICATE_H__

#include "backend/compiler/value.h"

// WindowsClientCertificate is СертификатКлиентаWindows: which way a client
// certificate would be picked from the Windows store. New WindowsClientCertificate()
// asks for Auto. New WindowsClientCertificate(WindowsCertificateSelectMode.Choose)
// asks for the system dialog when more than one certificate fits.
//
// The store is not read. SelectionMode is the request, and nothing else.

void ibValueWindowsClientCertificate_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueWindowsClientCertificate
	: public ibValueStaticMembers<&ibValueWindowsClientCertificate_BindNames> {
public:
	ibValueWindowsClientCertificate() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true) {}

	virtual bool Init();
	virtual bool Init(ibValue** paParams, const long lSizeArray);

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);

	const ibValue& SelectionMode() const { return m_selection; }

private:
	ibValue m_selection;
};

#endif
