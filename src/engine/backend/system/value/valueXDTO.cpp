#include "valueXDTO.h"

#include "backend/backend_exception.h"
#include "backend/compiler/typeCtor.h"

enum
{
	eNamespaceURI,
	eTypeName
};

enum
{
	eFactoryType,
	eFactoryCreate
};

enum
{
	eObjectType
};

const ibClassID kXDTOType = value_to_clsid("VL_XDTOT");

void ibValueXDTOFactory_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendFunc(wxT("Type"), 2, wxT("XDTOFactory.Type(namespaceURI : string, name : string)"));
	helper.AppendFunc(wxT("Create"), 1, wxT("XDTOFactory.Create(type : XDTOType)"));
}

void ibValueXDTOType_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendProp(wxT("NamespaceURI"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("Name"), true, false, wxNOT_FOUND);
}

void ibValueXDTODataObject_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendFunc(wxT("Type"), 0, wxT("XDTODataObject.Type()"));
}

ibValueXDTOType::ibValueXDTOType(const wxString& uri, const wxString& name)
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true)
	, m_uri(uri)
	, m_name(name)
{
}

bool ibValueXDTOType::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case eNamespaceURI:
		pvarPropVal = m_uri;
		return true;
	case eTypeName:
		pvarPropVal = m_name;
		return true;
	}
	return false;
}

ibValueXDTODataObject::ibValueXDTODataObject()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
{
}

ibValueXDTODataObject::ibValueXDTODataObject(const ibValue& type)
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
	, m_type(type)
{
}

long ibValueXDTODataObject::GetNProps() const
{
	return static_cast<long>(m_names.size());
}

long ibValueXDTODataObject::FindProp(const ibString& strPropName) const
{
	if (strPropName.empty())
		return wxNOT_FOUND;
	for (long i = 0; i < static_cast<long>(m_names.size()); ++i) {
		if (stringUtils::CompareString(m_names[static_cast<size_t>(i)], strPropName))
			return i;
	}
	// A get and a set both ask here first, and a missing name is refused before SetPropVal.
	// There is no schema to check the name against, so the first mention opens the field.
	// Until it is written, the value is Undefined.
	m_names.push_back(strPropName);
	m_values.emplace_back();
	return static_cast<long>(m_names.size()) - 1;
}

const ibString& ibValueXDTODataObject::GetPropName(const long lPropNum) const
{
	static const ibString absent;
	if (!Known(lPropNum))
		return absent;
	return m_names[static_cast<size_t>(lPropNum)];
}

bool ibValueXDTODataObject::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (!Known(lPropNum))
		return false;
	pvarPropVal = m_values[static_cast<size_t>(lPropNum)];
	return true;
}

bool ibValueXDTODataObject::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (!Known(lPropNum))
		return false;
	m_values[static_cast<size_t>(lPropNum)] = varPropVal;
	return true;
}

bool ibValueXDTODataObject::IsPropReadable(const long lPropNum) const
{
	return Known(lPropNum);
}

bool ibValueXDTODataObject::IsPropWritable(const long lPropNum) const
{
	return Known(lPropNum);
}

bool ibValueXDTODataObject::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** /*paParams*/, const long /*lSizeArray*/)
{
	if (lMethodNum != eObjectType)
		return false;
	pvarRetValue = m_type;
	return true;
}

bool ibValueXDTOFactory::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	if (lMethodNum == eFactoryType) {
		if (lSizeArray < 2 || paParams[0] == nullptr || paParams[1] == nullptr)
			return false;
		if (paParams[0]->GetType() != ibValueTypes::TYPE_STRING || paParams[1]->GetType() != ibValueTypes::TYPE_STRING)
			ibBackendCoreException::Error(_("XDTOFactory: the namespace and the name are strings"));
		const wxString uri(paParams[0]->GetString());
		const wxString name(paParams[1]->GetString());
		if (uri.empty() || name.empty())
			ibBackendCoreException::Error(_("XDTOFactory: the type needs a namespace and a name"));
		pvarRetValue = new ibValueXDTOType(uri, name);
		return true;
	}
	if (lMethodNum != eFactoryCreate)
		return false;
	if (lSizeArray < 1 || paParams[0] == nullptr)
		return false;
	if (paParams[0]->GetClassType() != kXDTOType)
		ibBackendCoreException::Error(_("XDTOFactory: Create takes an XDTOType"));
	pvarRetValue = new ibValueXDTODataObject(*paParams[0]);
	return true;
}

VALUE_TYPE_REGISTER(ibValueXDTOFactory, "XDTOFactory", value_to_clsid("VL_XDTOF"));
VALUE_TYPE_REGISTER(ibValueXDTOType, "XDTOType", kXDTOType);
VALUE_TYPE_REGISTER(ibValueXDTODataObject, "XDTODataObject", value_to_clsid("VL_XDTOD"));
