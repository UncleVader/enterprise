// Neutral "en" is wxLANGUAGE_ENGLISH, and that language's canonical name is
// en_GB. wxLocale::Init marks itself initialized before setlocale returns, so
// a second Init asserts in a debug build and returns false. That false used
// to abort opening the database. One attempt, and the open continues.

#include <gtest/gtest.h>

#include "backend/appHost.h"

#include <wx/intl.h>

TEST(LocaleOpen, NeutralEnglishIsBritishAndAFailedInitDoesNotRefuseTheOpen) {
	const wxLanguageInfo* info = wxLocale::FindLanguageInfo(wxT("en"));
	ASSERT_NE(info, nullptr);
	// The short name is "en". The name setlocale is given is the region
	// reference, en_GB, which is why a machine with only en_US fails.
	EXPECT_EQ(info->GetCanonicalWithRegion(), wxString(wxT("en_GB")));

	wxLocale locale;
	const bool set = locale.Init(info->Language);
	EXPECT_TRUE(ibLocaleOpenContinues(set));
	EXPECT_TRUE(ibLocaleOpenContinues(false));
}
