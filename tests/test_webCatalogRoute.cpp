////////////////////////////////////////////////////////////////////////////
//	Description : a configuration list is not a public route. The menu,
//	              the forms, the class registry, all functions and the
//	              interfaces name objects in the base.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>

#include "backend/webCatalogRoute.h"

TEST(WebCatalogRoute, AConfigurationListIsNotAPublicRoute)
{
	EXPECT_TRUE(ibWebPathNamesConfiguration("/menu"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/w/Configuration/menu"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/forms"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/diag/ctors"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/x/diag/ctors"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/functions"));
	EXPECT_TRUE(ibWebPathNamesConfiguration("/interfaces"));

	EXPECT_FALSE(ibWebPathNamesConfiguration("/"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/ping"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/login"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/auth-info"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/active"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/admin/locks"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/admin/diag"));
	EXPECT_FALSE(ibWebPathNamesConfiguration(""));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/notmenu"));
	EXPECT_FALSE(ibWebPathNamesConfiguration("/menu/extra"));
}
