#include "valueDataCompression.h"

#include "backend/backend_exception.h"

enum
{
	eCompressionLevel
};

void ibValueDataCompression_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(0, wxT("DataCompression()"));
	helper.AppendConstructor(1, wxT("DataCompression(level : number)"));
	helper.AppendProp(wxT("CompressionLevel"));
}

bool ibValueDataCompression::Init()
{
	m_level = -1;
	return true;
}

bool ibValueDataCompression::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray != 1 || paParams[0] == nullptr)
		return false;
	const int level = paParams[0]->GetInteger();
	// -1 is the platform default. 0 is stored as-is. 1..9 are zlib levels.
	if (level < -1 || level > 9)
		ibBackendCoreException::Error(_("DataCompression: level %d is outside -1..9"), level);
	m_level = level;
	return true;
}

bool ibValueDataCompression::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (lPropNum != eCompressionLevel)
		return false;
	pvarPropVal = ibNumber(static_cast<long long>(m_level));
	return true;
}

VALUE_TYPE_REGISTER(ibValueDataCompression, "DataCompression", value_to_clsid("VL_DCMPR"));
