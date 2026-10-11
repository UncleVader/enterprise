#include "backend/compiler/enumUnit.h"
#include "backend/compiler/typeCtor.h"

// Unscoped, and only in this file: a scoped enum does not register
// (enumUnit.h), and these names must not leak into other translation units.
// The set is ТипПлатформы: the names a port already writes, including
// PlatformType.Windows_x86_64.
enum ibPlatformType
{
	ibPlatformType_Linux_x86 = 1,
	ibPlatformType_Linux_x86_64,
	ibPlatformType_MacOS_x86,
	ibPlatformType_MacOS_x86_64,
	ibPlatformType_Windows_x86,
	ibPlatformType_Windows_x86_64,
	ibPlatformType_Linux_E2K,
	ibPlatformType_Android_ARM,
	ibPlatformType_Android_ARM_64,
	ibPlatformType_Android_x86,
	ibPlatformType_Android_x86_64,
	ibPlatformType_iOS_ARM,
	ibPlatformType_iOS_ARM_64,
	ibPlatformType_WinRT_ARM,
	ibPlatformType_WinRT_x86,
	ibPlatformType_WinRT_x86_64,
	ibPlatformType_Linux_ARM64,
	ibPlatformType_MacOS_ARM64
};

class ibValueEnumPlatformType : public ibValueEnumeration<ibPlatformType> {
public:
	ibValueEnumPlatformType() : ibValueEnumeration() {}

	virtual void CreateEnumeration() {
		AddEnumeration(ibPlatformType_Linux_x86,       wxT("Linux_x86"),       wxT("Linux_x86"));
		AddEnumeration(ibPlatformType_Linux_x86_64,    wxT("Linux_x86_64"),    wxT("Linux_x86_64"));
		AddEnumeration(ibPlatformType_MacOS_x86,       wxT("MacOS_x86"),       wxT("MacOS_x86"));
		AddEnumeration(ibPlatformType_MacOS_x86_64,    wxT("MacOS_x86_64"),    wxT("MacOS_x86_64"));
		AddEnumeration(ibPlatformType_Windows_x86,     wxT("Windows_x86"),     wxT("Windows_x86"));
		AddEnumeration(ibPlatformType_Windows_x86_64,  wxT("Windows_x86_64"),  wxT("Windows_x86_64"));
		AddEnumeration(ibPlatformType_Linux_E2K,       wxT("Linux_E2K"),       wxT("Linux_E2K"));
		AddEnumeration(ibPlatformType_Android_ARM,     wxT("Android_ARM"),     wxT("Android_ARM"));
		AddEnumeration(ibPlatformType_Android_ARM_64,  wxT("Android_ARM_64"),  wxT("Android_ARM_64"));
		AddEnumeration(ibPlatformType_Android_x86,     wxT("Android_x86"),     wxT("Android_x86"));
		AddEnumeration(ibPlatformType_Android_x86_64,  wxT("Android_x86_64"),  wxT("Android_x86_64"));
		AddEnumeration(ibPlatformType_iOS_ARM,         wxT("iOS_ARM"),         wxT("iOS_ARM"));
		AddEnumeration(ibPlatformType_iOS_ARM_64,      wxT("iOS_ARM_64"),      wxT("iOS_ARM_64"));
		AddEnumeration(ibPlatformType_WinRT_ARM,       wxT("WinRT_ARM"),       wxT("WinRT_ARM"));
		AddEnumeration(ibPlatformType_WinRT_x86,       wxT("WinRT_x86"),       wxT("WinRT_x86"));
		AddEnumeration(ibPlatformType_WinRT_x86_64,    wxT("WinRT_x86_64"),    wxT("WinRT_x86_64"));
		AddEnumeration(ibPlatformType_Linux_ARM64,     wxT("Linux_ARM64"),     wxT("Linux_ARM64"));
		AddEnumeration(ibPlatformType_MacOS_ARM64,     wxT("MacOS_ARM64"),     wxT("MacOS_ARM64"));
	}
};

ENUM_TYPE_REGISTER(ibValueEnumPlatformType, "PlatformType", enum_to_clsid("EN_PLAT"));
