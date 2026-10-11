#ifndef __VALUE_OPEN_FORM_H__
#define __VALUE_OPEN_FORM_H__

#include "backend/compiler/value.h"
#include "backend/createRequest.h"

// The form OpenForm returns when this process has no window. Closing it is what
// runs the notify. The backend does not create a window and does not run an event loop.
class BACKEND_API ibValueOpenedForm : public ibValueDynamicMembers {
public:
	ibValueOpenedForm();
	virtual ibClassID GetClassType() const override { return 0; }
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal) override;
	virtual bool CallAsFunc(const long lMethodNum, ibValue& pvarRetValue, ibValue** paParams, const long lSizeArray) override;

	void Remember(const ibFormRequest& request, const ibValue& notify);
	// The cancel a form module's beforeClose would set. This close is refused.
	void RefuseNextClose() { m_refuse = true; }
	bool IsClosed() const { return m_closed; }
	const ibFormRequest& Request() const { return m_request; }

private:
	void Fill(ibMemberTable& helper) const;
	bool CloseForm();
	void Forget();

	ibFormRequest m_request;
	ibValue m_notify;
	ibValue m_result;
	bool m_refuse = false;
	bool m_closed = false;
};

#endif
