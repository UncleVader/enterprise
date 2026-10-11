#ifndef _VALUE_DATA_HASHING_H__
#define _VALUE_DATA_HASHING_H__

#include "backend/compiler/value.h"

#include <memory>

struct ibDataHashState;

void ibValueDataHashing_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

// New DataHashing(HashFunction). A second argument, a BinaryData or a string, makes the sum an HMAC.
// Append adds bytes. HashSum is the digest, and it can be read only once the adding is finished.
class BACKEND_API ibValueDataHashing : public ibValueStaticMembers<&ibValueDataHashing_BindNames> {
	enum Prop { enHashSum };
	enum Func { enAppend };
public:
	ibValueDataHashing();
	~ibValueDataHashing();

	virtual bool Init();
	virtual bool Init(ibValue** paParams, const long lSizeArray);
	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);
	virtual bool CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray);

private:
	void Append(const ibValue& data);
	void TakeSum();

	std::unique_ptr<ibDataHashState> m_state;
};

#endif
