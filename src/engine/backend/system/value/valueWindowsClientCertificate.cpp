#include "valueWindowsClientCertificate.h"

#include "backend/backend_exception.h"
#include "backend/compiler/typeCtor.h"

enum
{
	eSelectionMode
};

namespace {

const ibClassID kSelectMode = enum_to_clsid("EN_WCSEL");

ibValue SelectMember(const wxChar* name)
{
	ibValue modes = ibValue::CreateObject(wxT("WindowsCertificateSelectMode"));
	ibValue member;
	const long idx = modes.FindProp(name);
	if (idx == wxNOT_FOUND || !modes.GetPropVal(idx, member))
		ibBackendCoreException::Error(_("WindowsClientCertificate: WindowsCertificateSelectMode.%s is not a member"), wxString(name));
	return member;
}

bool IsSelectMode(const ibValue& value)
{
	return value.GetType() == ibValueTypes::TYPE_ENUM && value.GetClassType() == kSelectMode;
}

} // namespace

void ibValueWindowsClientCertificate_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("WindowsClientCertificate()"));
	helper.AppendConstructor(1, wxT("WindowsClientCertificate(selection : WindowsCertificateSelectMode)"));
	helper.AppendProp(wxT("SelectionMode"), true, false, wxNOT_FOUND);
}

bool ibValueWindowsClientCertificate::Init()
{
	m_selection = SelectMember(wxT("Auto"));
	return true;
}

bool ibValueWindowsClientCertificate::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray != 1 || paParams[0] == nullptr)
		return false;
	if (!IsSelectMode(*paParams[0]))
		ibBackendCoreException::Error(_("WindowsClientCertificate: the selection is a WindowsCertificateSelectMode value"));
	m_selection = *paParams[0];
	return true;
}

bool ibValueWindowsClientCertificate::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (lPropNum != eSelectionMode)
		return false;
	pvarPropVal = m_selection;
	return true;
}

VALUE_TYPE_REGISTER(ibValueWindowsClientCertificate, "WindowsClientCertificate", value_to_clsid("VL_WCCRT"));
