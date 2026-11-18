#include "backend/compiler/enumUnit.h"
#include "backend/compiler/typeCtor.h"

// Unscoped, and only in this file: a scoped enum does not register
// (enumUnit.h), and these names must not leak into other translation units.
// ИспользованиеByteOrderMark: whether a writer puts a BOM at the start of a text.
enum ibByteOrderMarkUsage
{
	ibByteOrderMarkUsage_Auto = 0,
	ibByteOrderMarkUsage_Use,
	ibByteOrderMarkUsage_DontUse
};

class ibValueEnumByteOrderMarkUsage : public ibValueEnumeration<ibByteOrderMarkUsage> {
public:
	ibValueEnumByteOrderMarkUsage() : ibValueEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(ibByteOrderMarkUsage_Auto,    wxT("Auto"),    wxT("Auto"));
		AddEnumeration(ibByteOrderMarkUsage_Use,     wxT("Use"),     wxT("Use"));
		AddEnumeration(ibByteOrderMarkUsage_DontUse, wxT("DontUse"), wxT("DontUse"));
	}
};

ENUM_TYPE_REGISTER(ibValueEnumByteOrderMarkUsage, "ByteOrderMarkUsage", enum_to_clsid("EN_BOMU"));
