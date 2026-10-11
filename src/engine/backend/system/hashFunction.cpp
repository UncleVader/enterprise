#include "hashFunction.h"

#include "backend/compiler/enumUnit.h"

class ibValueEnumHashFunction : public ibValueEnumeration<ibHashFunction> {
public:
	virtual void CreateEnumeration() override {
		AddEnumeration(ibHashFunction_MD5, wxT("MD5"), _("MD5"));
		AddEnumeration(ibHashFunction_SHA1, wxT("SHA1"), _("SHA-1"));
		AddEnumeration(ibHashFunction_SHA256, wxT("SHA256"), _("SHA-256"));
	}
};

ENUM_TYPE_REGISTER(ibValueEnumHashFunction, "HashFunction", enum_to_clsid("EN_HASH"));
