#ifndef __VALUE_NOTIFY_DESCRIPTION_H__
#define __VALUE_NOTIFY_DESCRIPTION_H__

#include "backend/compiler/value.h"

void ibValueNotifyDescription_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

// A procedure to call when a dialog has an answer. The procedure is named,
// the module is the object that has it, and the extra value is passed after
// the answer. Question() calls it. OpenForm does not: nothing in the backend
// opens a form and then waits for it to close.
class BACKEND_API ibValueNotifyDescription : public ibValueStaticMembers<&ibValueNotifyDescription_BindNames>
{
public:
	ibValueNotifyDescription();
	virtual ~ibValueNotifyDescription() {}

	virtual bool Init(ibValue** paParams, const long lSizeArray) override;
	virtual ibString GetString() const override { return m_procedure; }

	virtual bool SetPropVal(const long lPropNum, const ibValue& varPropVal) override;
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal) override;

	// Calls module.procedure(result, additionalParameters).
	void Call(const ibValue& result);

private:
	wxString m_procedure;
	ibValue  m_module;
	ibValue  m_additional;
};

#endif
