#ifndef _BINARY_SCRIPT_H__
#define _BINARY_SCRIPT_H__

#include "backend/compiler/value.h"

// Base64 and string/bytes conversion for a script. `encoding` is null, or empty, when the
// caller left the TextEncoding out — that means UTF-8, and no byte-order mark is written.
ibValue ibBase64String(const ibValue& data);
ibValue ibBase64Value(const ibValue& text);
ibValue ibGetBinaryDataFromString(const ibValue& text, const ibValue* encoding);
ibValue ibGetStringFromBinaryData(const ibValue& data, const ibValue* encoding);

#endif
