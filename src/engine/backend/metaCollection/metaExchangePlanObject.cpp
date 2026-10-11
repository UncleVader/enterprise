#include "metaExchangePlanObject.h"

#include "backend/metaData.h"
#include "backend/metaCtor.h"
#include "backend/objCtor.h"
#include "backend/backend_exception.h"
#include "backend/compiler/typeCtor.h"
#include "backend/system/value/valueArray.h"
#include "backend/system/value/valueMap.h"

#include <map>

enum
{
	eCode,
	eDescription,
	eSentNo,
	eReceivedNo,
	eDeletionMark
};

enum
{
	eThisNode,
	eEmptyRef,
	eFindByCode,
	eSelect
};

enum
{
	eMasterNode,
	eSetMasterNode
};

namespace {

void ibValueExchangePlanNode_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendProp(wxT("Code"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("Description"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("SentNo"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("ReceivedNo"), true, false, wxNOT_FOUND);
	helper.AppendProp(wxT("DeletionMark"), true, false, wxNOT_FOUND);
}

// A node that was named, or an empty one. Nothing behind it is stored.
class ibValueExchangePlanNode : public ibValueStaticMembers<&ibValueExchangePlanNode_BindNames> {
public:
	explicit ibValueExchangePlanNode(const wxString& code = wxString())
		: ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true), m_code(code) {}

	virtual bool IsEmpty() const override { return m_code.empty(); }
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal) override;

	const wxString& Code() const { return m_code; }

private:
	wxString m_code;
};

bool ibValueExchangePlanNode::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case eCode:
		pvarPropVal = m_code;
		return true;
	case eDescription:
		pvarPropVal = wxString();
		return true;
	case eSentNo:
	case eReceivedNo:
		pvarPropVal = ibNumber(static_cast<long long>(0));
		return true;
	case eDeletionMark:
		pvarPropVal = false;
		return true;
	}
	return false;
}

wxString& MasterCode(const ibMetaData* meta)
{
	static std::map<const ibMetaData*, wxString> codes;
	return codes[meta];
}

wxString CodeOf(const ibValue& value)
{
	if (value.GetType() == ibValueTypes::TYPE_EMPTY)
		return wxString();
	if (value.GetType() == ibValueTypes::TYPE_STRING)
		return wxString(value.GetString());
	const ibValueExchangePlanNode* const node = dynamic_cast<const ibValueExchangePlanNode*>(value.GetRef());
	if (node == nullptr)
		ibBackendCoreException::Error(_("ExchangePlans: SetMasterNode takes a node, a code, or Undefined"));
	return node->Code();
}

class ibValueExchangePlanManager : public ibValueDynamicMembers {
public:
	explicit ibValueExchangePlanManager(ibValueMetaObjectExchangePlan* meta = nullptr)
		: ibValueDynamicMembers(ibValueTypes::TYPE_VALUE, true), m_meta(meta)
	{
		m_members.Bind(this, &ibValueExchangePlanManager::Fill);
	}

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray) override;
	virtual wxString GetClassName() const override;

private:
	void Fill(ibMemberTable& helper) const
	{
		helper.AppendFunc(wxT("ThisNode"), 0, wxT("ThisNode()"));
		helper.AppendFunc(wxT("EmptyRef"), 0, wxT("EmptyRef()"));
		helper.AppendFunc(wxT("FindByCode"), 1, wxT("FindByCode(code : string)"));
		helper.AppendFunc(wxT("Select"), 0, wxT("Select()"));
	}

	ibValueMetaObjectExchangePlan* m_meta;
};

class ibCtorExchangePlanManager : public ibCtorMetaValueType {
public:
	explicit ibCtorExchangePlanManager(ibValueMetaObjectExchangePlan* meta) : m_meta(meta)
	{
		m_classType = manager_to_clsid(meta->GetMetaID(), clsid_metaclass(meta->GetClassType()));
	}
	virtual wxString GetClassName() const override { return m_meta->GetClassName() + prefixManager + m_meta->GetName(); }
	virtual ibClassID GetClassType() const override { return m_classType; }
	virtual ibValue CreateObject() const override { return new ibValueExchangePlanManager(m_meta); }
	virtual const ibValueMetaObject* GetMetaObject() const override { return m_meta; }
	virtual ibCtorObjectMetaType GetMetaTypeCtor() const override { return ibCtorObjectMetaType::ibCtorObjectMetaType_Manager; }

private:
	ibClassID m_classType = 0;
	ibValueMetaObjectExchangePlan* m_meta;
};

bool ibValueExchangePlanManager::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray)
{
	if (m_meta == nullptr)
		return false;
	if (lMethodNum == eThisNode || lMethodNum == eEmptyRef) {
		pvarRetValue = new ibValueExchangePlanNode();
		return true;
	}
	if (lMethodNum == eFindByCode) {
		if (lSizeArray < 1 || paParams[0] == nullptr || paParams[0]->GetType() != ibValueTypes::TYPE_STRING)
			ibBackendCoreException::Error(_("ExchangePlan: FindByCode takes a string"));
		pvarRetValue = new ibValueExchangePlanNode();
		return true;
	}
	if (lMethodNum == eSelect) {
		pvarRetValue = new ibValueArray();
		return true;
	}
	return false;
}

wxString ibValueExchangePlanManager::GetClassName() const
{
	if (m_meta != nullptr && m_meta->GetMetaData() != nullptr) {
		const ibCtorMetaValueType* const cls = m_meta->GetMetaData()->GetTypeCtor(
			m_meta, ibCtorObjectMetaType::ibCtorObjectMetaType_Manager);
		if (cls != nullptr)
			return cls->GetClassName();
	}
	return wxT("ExchangePlanManager");
}

class ibValueExchangePlansManager : public ibValueStructure {
public:
	explicit ibValueExchangePlansManager(ibMetaData* meta)
		: ibValueStructure(true), m_meta(meta)
	{
		m_methods.Bind(this, &ibValueExchangePlansManager::Fill);
		if (meta == nullptr)
			return;
		for (const auto object : meta->GetAnyArrayObject(g_metaExchangePlanCLSID)) {
			ibCtorMetaValueType* const so = meta->GetTypeCtor(object, ibCtorObjectMetaType::ibCtorObjectMetaType_Manager);
			if (so == nullptr)
				continue;
			SetAt(object->GetName(), so->CreateObject());
		}
	}

	virtual ibClassID GetClassType() const override { return value_to_clsid("VL_EXPLN"); }
	virtual wxString GetClassName() const override { return wxT("ExchangePlans"); }
	virtual ibString GetString() const override { return wxT("ExchangePlans"); }

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** /*paParams*/, const long /*lSizeArray*/) override;
	virtual bool CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray) override;

protected:
	virtual ibMemberTable* DoGetPMethods() const override { return &m_methods; }

private:
	void Fill(ibMemberTable& helper) const
	{
		helper.AppendFunc(wxT("MasterNode"), 0, wxT("MasterNode()"));
		helper.AppendProc(wxT("SetMasterNode"), 1, wxT("SetMasterNode(node)"));
	}

	ibMetaData* m_meta;
	mutable ibMemberTable m_methods;
};

bool ibValueExchangePlansManager::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** /*paParams*/, const long /*lSizeArray*/)
{
	if (lMethodNum != eMasterNode || m_meta == nullptr)
		return false;
	const wxString code = MasterCode(m_meta);
	if (code.empty())
		pvarRetValue = ibValue();
	else
		pvarRetValue = new ibValueExchangePlanNode(code);
	return true;
}

bool ibValueExchangePlansManager::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
{
	if (lMethodNum != eSetMasterNode)
		return false;
	if (m_meta == nullptr)
		ibBackendCoreException::Error(_("ExchangePlans: there is no configuration to remember a master node"));
	if (lSizeArray < 1 || paParams[0] == nullptr)
		return false;
	MasterCode(m_meta) = CodeOf(*paParams[0]);
	return true;
}

VALUE_TYPE_REGISTER(ibValueExchangePlanNode, "ExchangePlanNode", value_to_clsid("VL_EXNOD"));

} // namespace

bool ibValueMetaObjectExchangePlan::OnBeforeRunMetaObject(int flags)
{
	if (!ibValueMetaObject::OnBeforeRunMetaObject(flags))
		return false;
	if (m_metaData != nullptr && m_metaData->IsConfigOpen()
		&& m_metaData->GetTypeCtor(this, ibCtorObjectMetaType::ibCtorObjectMetaType_Manager) == nullptr)
		m_metaData->RegisterCtor(new ibCtorExchangePlanManager(this));
	return true;
}

bool ibValueMetaObjectExchangePlan::OnDeleteMetaObject()
{
	if (m_metaData != nullptr && m_metaData->GetTypeCtor(this, ibCtorObjectMetaType::ibCtorObjectMetaType_Manager) != nullptr)
		m_metaData->UnRegisterCtor(manager_to_clsid(GetMetaID(), clsid_metaclass(GetClassType())));
	return ibValueMetaObject::OnDeleteMetaObject();
}

ibValue ibExchangePlans(ibMetaData* metaData)
{
	return new ibValueExchangePlansManager(metaData);
}

METADATA_TYPE_REGISTER(ibValueMetaObjectExchangePlan, "ExchangePlan", g_metaExchangePlanCLSID);
