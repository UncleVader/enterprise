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
	EXPECT_EQ(info->CanonicalName, wxT("en_GB"));

	wxLocale locale;
	const bool set = locale.Init(info->Language);
	EXPECT_TRUE(ibLocaleOpenContinues(set));
	EXPECT_TRUE(ibLocaleOpenContinues(false));
}
