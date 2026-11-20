#include "valueQueryBuilder.h"

#include "backend/system/value/valueMap.h"

void ibValueQueryBuilder_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("QueryBuilder()"));
	helper.AppendConstructor(1, wxT("QueryBuilder(text : string)"));

	helper.AppendProp(wxT("Text"));
	helper.AppendProp(wxT("Parameters"));

	helper.AppendFunc(wxT("Execute"), wxT("Execute()"));
	helper.AppendFunc(wxT("GetQuery"), wxT("GetQuery()"));
	helper.AppendProc(wxT("FillSettings"), wxT("FillSettings()"));
}

ibValueQueryBuilder::ibValueQueryBuilder()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE)
{
	m_parameters = new ibValueStructure();
}

bool ibValueQueryBuilder::Init()
{
	return true;
}

bool ibValueQueryBuilder::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray != 1 || paParams[0] == nullptr)
		return false;
	m_text = paParams[0]->GetString();
	return true;
}

bool ibValueQueryBuilder::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum) {
	case enText:
		pvarPropVal = m_text;
		return true;
	case enParameters:
		pvarPropVal = m_parameters;
		return true;
	}
	return false;
}

bool ibValueQueryBuilder::SetPropVal(const long lPropNum, const ibValue& varPropVal)
{
	if (lPropNum != enText)
		return false;
	m_text = varPropVal.GetString();
	return true;
}

ibValue ibValueQueryBuilder::MakeQuery() const
{
	ibValue query;
	if (m_text.IsEmpty()) {
		query = ibValue::CreateObject(wxT("Query"));
	}
	else {
		ibValue text(m_text);
		ibValue* arg = &text;
		query = ibValue::CreateObject(wxT("Query"), &arg, 1);
	}

	const long setParameter = query.FindMethod(wxT("SetParameter"));
	ibValue* store = m_parameters.GetRef();
	if (store == nullptr || setParameter < 0)
		return query;

	const long count = store->GetNProps();
	for (long i = 0; i < count; ++i) {
		ibValue name(store->GetPropName(i));
		ibValue value;
		if (!store->GetPropVal(i, value))
			continue;
		ibValue* args[] = { &name, &value };
		query.CallAsProc(setParameter, args, 2);
	}
	return query;
}

bool ibValueQueryBuilder::CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** /*paParams*/, const long /*lSizeArray*/)
{
	if (lMethodNum == enGetQuery) {
		pvarRetValue = MakeQuery();
		return true;
	}
	if (lMethodNum != enExecute)
		return false;

	ibValue query = MakeQuery();
	const long execute = query.FindMethod(wxT("Execute"));
	if (execute < 0)
		return false;
	return query.CallAsFunc(execute, pvarRetValue, nullptr, 0);
}

bool ibValueQueryBuilder::CallAsProc(const long lMethodNum, ibValue** /*paParams*/, const long /*lSizeArray*/)
{
	// FillSettings builds the filter and order a form draws. There is no such
	// settings object here; the call is accepted so a module that fills settings
	// and then runs Execute still runs.
	return lMethodNum == enFillSettings;
}

VALUE_TYPE_REGISTER(ibValueQueryBuilder, "QueryBuilder", value_to_clsid("VL_QBLDR"));
