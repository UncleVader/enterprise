#ifndef _IB_CONST_TIME_H_
#define _IB_CONST_TIME_H_

/////////////////////////////////////////////////////////////////////////////
// Compare secret bytes without returning at the first difference.
//
// A plain == on a token or a hash tells a watcher how much of the prefix
// matched, because it stops there. These walk every byte they were given.
// Two strings of different length still differ in the result; the walk
// covers the longer of the two so the shorter one is not an early answer.
/////////////////////////////////////////////////////////////////////////////

#include <wx/string.h>

#include <cstddef>
#include <cstdint>

inline bool ibConstTimeEqual(const uint8_t* a, const uint8_t* b, size_t len)
{
	if (len != 0 && (a == nullptr || b == nullptr))
		return false;

	uint8_t diff = 0;
	for (size_t i = 0; i < len; ++i)
		diff |= uint8_t(a[i] ^ b[i]);
	return diff == 0;
}

inline bool ibConstTimeEqual(const wxString& a, const wxString& b)
{
	const wxScopedCharBuffer ab = a.utf8_str();
	const wxScopedCharBuffer bb = b.utf8_str();
	const char* const ap = ab.data();
	const char* const bp = bb.data();
	const size_t an = ap != nullptr ? ab.length() : 0;
	const size_t bn = bp != nullptr ? bb.length() : 0;
	const size_t n = an > bn ? an : bn;

	uint8_t diff = an == bn ? 0 : 1;
	for (size_t i = 0; i < n; ++i) {
		const uint8_t ca = i < an ? uint8_t(ap[i]) : 0;
		const uint8_t cb = i < bn ? uint8_t(bp[i]) : 0;
		diff |= uint8_t(ca ^ cb);
	}
	return diff == 0;
}

#endif
