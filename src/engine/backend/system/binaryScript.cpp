#include "binaryScript.h"

#include "backend/backend_exception.h"
#include "backend/compiler/enumUnit.h"   // ConvertToEnumValue needs the enumeration variant complete
#include "backend/system/value/valueBinaryData.h"
#include "backend/system/value/valueTextFile.h"
#include "core/clsid.h"

#include <wx/base64.h>

namespace {

const ibValueBinaryData* AsBinary(const ibValue& value)
{
	return dynamic_cast<const ibValueBinaryData*>(value.GetRef());
}

ibTextEncoding EncodingOf(const ibValue* encoding)
{
	if (encoding == nullptr || encoding->GetType() == ibValueTypes::TYPE_EMPTY)
		return ibTextEncoding_UTF8;
	const ibValue* const held = encoding->GetRef();
	if (held->GetType() != ibValueTypes::TYPE_ENUM
		|| held->GetClassType() != enum_to_clsid("EN_TXEN"))
		ibBackendCoreException::Error(_("the encoding is a TextEncoding value"));
	const ibTextEncoding value = held->ConvertToEnumValue<ibTextEncoding>();
	if (value < ibTextEncoding_UTF8 || value > ibTextEncoding_System)
		ibBackendCoreException::Error(_("the encoding is a TextEncoding value"));
	return value;
}

}

ibValue ibBase64String(const ibValue& data)
{
	const ibValueBinaryData* const bytes = AsBinary(data);
	if (bytes == nullptr)
		ibBackendCoreException::Error(_("Base64String takes a BinaryData"));
	if (bytes->GetLength() == 0)
		return wxString();
	return wxBase64Encode(bytes->GetBuffer().GetData(), bytes->GetBuffer().GetDataLen());
}

ibValue ibBase64Value(const ibValue& text)
{
	if (text.GetRef()->GetType() != ibValueTypes::TYPE_STRING)
		ibBackendCoreException::Error(_("Base64Value takes a string"));
	const wxString source = text.GetString();
	if (source.empty())
		return new ibValueBinaryData();

	size_t posError = wxCONV_FAILED;
	const wxScopedCharBuffer ascii = source.utf8_str();
	const wxMemoryBuffer decoded = wxBase64Decode(ascii.data(), ascii.length(),
		wxBase64DecodeMode_Strict, &posError);
	if (posError != wxCONV_FAILED)
		ibBackendCoreException::Error(_("Base64Value: the text is not base64"));
	return new ibValueBinaryData(decoded.GetData(), decoded.GetDataLen());
}

ibValue ibGetBinaryDataFromString(const ibValue& text, const ibValue* encoding)
{
	if (text.GetRef()->GetType() != ibValueTypes::TYPE_STRING)
		ibBackendCoreException::Error(_("GetBinaryDataFromString takes a string"));
	const ibTextEncoding which = EncodingOf(encoding);
	const wxString source = text.GetString();
	if (source.empty())
		return new ibValueBinaryData();

	const std::unique_ptr<wxMBConv> conv = ibCreateTextConv(which);
	size_t length = 0;
	const wxCharBuffer bytes = conv->cWC2MB(source.wc_str(), source.length(), &length);
	if (bytes.data() == nullptr || length == 0 || length == wxCONV_FAILED)
		ibBackendCoreException::Error(_("GetBinaryDataFromString: the text cannot be written in this encoding"));
	return new ibValueBinaryData(bytes.data(), length);
}

ibValue ibGetStringFromBinaryData(const ibValue& data, const ibValue* encoding)
{
	const ibValueBinaryData* const bytes = AsBinary(data);
	if (bytes == nullptr)
		ibBackendCoreException::Error(_("GetStringFromBinaryData takes a BinaryData"));
	wxString text;
	if (!ibTextFromBytes(bytes->GetBuffer().GetData(), bytes->GetLength(), EncodingOf(encoding), text))
		ibBackendCoreException::Error(_("GetStringFromBinaryData: the bytes are not text in this encoding"));
	return text;
}
