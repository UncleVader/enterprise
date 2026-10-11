#include "backend/compiler/enumUnit.h"
#include "backend/compiler/typeCtor.h"

// Unscoped, and only in this file: a scoped enum does not register
// (enumUnit.h), and these names must not leak into other translation units.
// СпособВыбораСертификатаWindows.
enum ibWindowsCertificateSelectMode
{
	ibWindowsCertificateSelectMode_Auto = 0,
	ibWindowsCertificateSelectMode_Choose
};

class ibValueEnumWindowsCertificateSelectMode : public ibValueEnumeration<ibWindowsCertificateSelectMode> {
public:
	ibValueEnumWindowsCertificateSelectMode() : ibValueEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(ibWindowsCertificateSelectMode_Auto,   wxT("Auto"),   wxT("Auto"));
		AddEnumeration(ibWindowsCertificateSelectMode_Choose, wxT("Choose"), wxT("Choose"));
	}
};

ENUM_TYPE_REGISTER(ibValueEnumWindowsCertificateSelectMode, "WindowsCertificateSelectMode", enum_to_clsid("EN_WCSEL"));
