#ifndef __VALUE_XDTO_H__
#define __VALUE_XDTO_H__

#include "backend/compiler/value.h"

#include <vector>

// XDTOFactory is ФабрикаXDTO. There is no package and no schema: Type(namespace, name)
// records those two strings, and Create makes an object whose fields are whatever a
// script writes. A field that has not been written reads as Undefined.

void ibValueXDTOFactory_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);
void ibValueXDTOType_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);
void ibValueXDTODataObject_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueXDTOType : public ibValueStaticMembers<&ibValueXDTOType_BindNames> {
public:
	ibValueXDTOType() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true) {}
	ibValueXDTOType(const wxString& uri, const wxString& name);

	virtual bool Init() override { return false; }

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);

	const wxString& NamespaceURI() const { return m_uri; }
	const wxString& Name() const { return m_name; }

private:
	wxString m_uri;
	wxString m_name;
};

class BACKEND_API ibValueXDTODataObject : public ibValueStaticMembers<&ibValueXDTODataObject_BindNames> {
public:
	ibValueXDTODataObject();
	explicit ibValueXDTODataObject(const ibValue& type);

	virtual bool Init() override { return false; }

	virtual long GetNProps() const override;
	virtual long FindProp(const ibString& strPropName) const override;
	virtual const ibString& GetPropName(const long lPropNum) const override;
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal) override;
	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal) override;
	virtual bool IsPropReadable(const long lPropNum) const override;
	virtual bool IsPropWritable(const long lPropNum) const override;

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray) override;

	const ibValue& TypeValue() const { return m_type; }

private:
	bool Known(long index) const { return index >= 0 && index < static_cast<long>(m_names.size()); }

	ibValue m_type;
	mutable std::vector<ibString> m_names;
	mutable std::vector<ibValue> m_values;
};

class BACKEND_API ibValueXDTOFactory : public ibValueStaticMembers<&ibValueXDTOFactory_BindNames> {
public:
	ibValueXDTOFactory() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true) {}

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray) override;
};

#endif
