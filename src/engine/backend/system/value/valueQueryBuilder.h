#ifndef __VALUE_QUERY_BUILDER_H__
#define __VALUE_QUERY_BUILDER_H__

#include "backend/compiler/value.h"

// QueryBuilder is ПостроительЗапроса. New QueryBuilder was "Constructor not found".
// Text and Parameters are what a server module sets; Execute runs them as a Query.

void ibValueQueryBuilder_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueQueryBuilder : public ibValueStaticMembers<&ibValueQueryBuilder_BindNames> {
	enum Prop { enText = 0, enParameters };
	enum Func { enExecute = 0, enGetQuery, enFillSettings };
public:
	ibValueQueryBuilder();

	virtual bool Init();
	virtual bool Init(ibValue** paParams, const long lSizeArray);

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);
	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal);

	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray);
	virtual bool CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray);

	const wxString& GetText() const { return m_text; }

private:
	ibValue MakeQuery() const;

	wxString m_text;
	ibValue  m_parameters;
};

#endif
