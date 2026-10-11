#include "valueDataHashing.h"

#include "backend/backend_exception.h"
#include "backend/system/hashFunction.h"
#include "backend/system/value/valueBinaryData.h"
#include "core/clsid.h"

#include <mbedtls/md.h>

#include <cstring>
#include <vector>

struct ibDataHashState {
	mbedtls_md_context_t ctx;
	bool hmac = false;
	bool live = false;
	bool taken = false;
	size_t size = 0;
	std::vector<unsigned char> sum;

	ibDataHashState() { mbedtls_md_init(&ctx); }
	~ibDataHashState()
	{
		if (live)
			mbedtls_md_free(&ctx);
	}
};

namespace {

mbedtls_md_type_t MdOf(ibHashFunction algorithm)
{
	switch (algorithm) {
	case ibHashFunction_MD5: return MBEDTLS_MD_MD5;
	case ibHashFunction_SHA1: return MBEDTLS_MD_SHA1;
	case ibHashFunction_SHA256: return MBEDTLS_MD_SHA256;
	}
	return MBEDTLS_MD_NONE;
}

ibHashFunction AlgorithmOf(const ibValue& value)
{
	const ibValue* const held = value.GetRef();
	if (held->GetType() != ibValueTypes::TYPE_ENUM || held->GetClassType() != enum_to_clsid("EN_HASH"))
		ibBackendCoreException::Error(_("DataHashing: the algorithm is a HashFunction value"));
	const ibHashFunction algorithm = held->ConvertToEnumValue<ibHashFunction>();
	if (algorithm < ibHashFunction_MD5 || algorithm > ibHashFunction_SHA256)
		ibBackendCoreException::Error(_("DataHashing: the algorithm is a HashFunction value"));
	return algorithm;
}

std::vector<unsigned char> BytesOf(const ibValue& value)
{
	if (const ibValueBinaryData* const data = dynamic_cast<const ibValueBinaryData*>(value.GetRef())) {
		std::vector<unsigned char> out(data->GetLength());
		if (!out.empty())
			std::memcpy(out.data(), data->GetBuffer().GetData(), out.size());
		return out;
	}
	if (value.GetRef()->GetType() == ibValueTypes::TYPE_STRING) {
		const wxScopedCharBuffer utf8 = wxString(value.GetString()).utf8_str();
		if (utf8.data() == nullptr || utf8.length() == 0)
			return {};
		return std::vector<unsigned char>(
			reinterpret_cast<const unsigned char*>(utf8.data()),
			reinterpret_cast<const unsigned char*>(utf8.data()) + utf8.length());
	}
	ibBackendCoreException::Error(_("DataHashing: Append takes a BinaryData or a string"));
	return {};
}

}

void ibValueDataHashing_BindNames(ibValue::ibMemberTable& helper, const ibValue* /*ctx*/)
{
	helper.AppendConstructor(1, wxT("DataHashing(algorithm : HashFunction)"));
	helper.AppendConstructor(2, wxT("DataHashing(algorithm : HashFunction, key : BinaryData | string)"));
	helper.AppendProp(wxT("HashSum"), true, false, wxNOT_FOUND);
	helper.AppendProc(wxT("Append"), 1, wxT("Append(data : BinaryData | string)"));
}

ibValueDataHashing::ibValueDataHashing()
	: ibValueStaticMembers(ibValueTypes::TYPE_VALUE, true)
{
}

ibValueDataHashing::~ibValueDataHashing() = default;

bool ibValueDataHashing::Init()
{
	return false;
}

bool ibValueDataHashing::Init(ibValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1 || lSizeArray > 2 || paParams == nullptr)
		return false;

	const bool hmac = lSizeArray == 2;
	const mbedtls_md_info_t* const info = mbedtls_md_info_from_type(MdOf(AlgorithmOf(*paParams[0])));
	if (info == nullptr)
		ibBackendCoreException::Error(_("DataHashing: this algorithm is not available"));

	std::vector<unsigned char> key;
	if (hmac) {
		const ibValue* const held = paParams[1]->GetRef();
		const bool text = held->GetType() == ibValueTypes::TYPE_STRING;
		const bool binary = dynamic_cast<const ibValueBinaryData*>(held) != nullptr;
		if (!text && !binary)
			ibBackendCoreException::Error(_("DataHashing: the key is a BinaryData or a string"));
		key = BytesOf(*paParams[1]);
	}

	auto state = std::make_unique<ibDataHashState>();
	if (mbedtls_md_setup(&state->ctx, info, hmac ? 1 : 0) != 0)
		ibBackendCoreException::Error(_("DataHashing: this algorithm is not available"));
	state->live = true;
	state->hmac = hmac;
	state->size = mbedtls_md_get_size(info);

	const unsigned char* const keyBytes = key.empty() ? reinterpret_cast<const unsigned char*>("") : key.data();
	const int started = hmac
		? mbedtls_md_hmac_starts(&state->ctx, keyBytes, key.size())
		: mbedtls_md_starts(&state->ctx);
	if (started != 0)
		ibBackendCoreException::Error(_("DataHashing: this algorithm is not available"));

	m_state = std::move(state);
	return true;
}

void ibValueDataHashing::Append(const ibValue& data)
{
	if (m_state == nullptr || !m_state->live)
		ibBackendCoreException::Error(_("DataHashing: there is no algorithm"));
	if (m_state->taken)
		ibBackendCoreException::Error(_("DataHashing: the sum is already taken"));

	const std::vector<unsigned char> bytes = BytesOf(data);
	if (bytes.empty())
		return;
	const int added = m_state->hmac
		? mbedtls_md_hmac_update(&m_state->ctx, bytes.data(), bytes.size())
		: mbedtls_md_update(&m_state->ctx, bytes.data(), bytes.size());
	if (added != 0)
		ibBackendCoreException::Error(_("DataHashing: the bytes could not be added"));
}

void ibValueDataHashing::TakeSum()
{
	if (m_state == nullptr || !m_state->live)
		ibBackendCoreException::Error(_("DataHashing: there is no algorithm"));
	if (m_state->taken)
		return;
	m_state->sum.resize(m_state->size);
	const int finished = m_state->hmac
		? mbedtls_md_hmac_finish(&m_state->ctx, m_state->sum.data())
		: mbedtls_md_finish(&m_state->ctx, m_state->sum.data());
	if (finished != 0)
		ibBackendCoreException::Error(_("DataHashing: the sum could not be taken"));
	m_state->taken = true;
}

bool ibValueDataHashing::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	if (lPropNum != enHashSum)
		return false;
	TakeSum();
	pvarPropVal = new ibValueBinaryData(m_state->sum.data(), m_state->sum.size());
	return true;
}

bool ibValueDataHashing::CallAsProc(const long lMethodNum, ibValue** paParams, const long lSizeArray)
{
	if (lMethodNum != enAppend || lSizeArray < 1 || paParams == nullptr)
		return false;
	Append(*paParams[0]);
	return true;
}

VALUE_TYPE_REGISTER(ibValueDataHashing, "DataHashing", value_to_clsid("VL_DHASH"));
