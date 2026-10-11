#ifndef __VALUE_DATA_COMPRESSION_H__
#define __VALUE_DATA_COMPRESSION_H__

#include "backend/compiler/value.h"

// DataCompression is СжатиеДанных: the level a value storage is asked to use.
// New DataCompression() and New DataCompression(level), level -1..9.

void ibValueDataCompression_BindNames(ibValue::ibMemberTable& helper, const ibValue* ctx);

class BACKEND_API ibValueDataCompression : public ibValueStaticMembers<&ibValueDataCompression_BindNames> {
public:
	ibValueDataCompression() : ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true), m_level(-1) {}

	virtual bool Init();
	virtual bool Init(ibValue** paParams, const long lSizeArray);

	virtual bool GetPropVal(const long lPropNum, ibValue& pvarPropVal);

	int GetLevel() const { return m_level; }

private:
	int m_level;
};

#endif
