////////////////////////////////////////////////////////////////////////////
//	Description : the web dispatcher refuses a control that is disabled,
//	              hidden, or not accepting a new value.
////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <string>

#include "backend/webDispatchGate.h"

TEST(WebDispatchGate, AReachableWritableControlAcceptsAValue)
{
	EXPECT_TRUE(ibWebDispatchAllowed(true, false, "text"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, false, "toggle"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, false, "buttonClear"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, false, "click"));
}

TEST(WebDispatchGate, AnUnreachableControlAcceptsNothing)
{
	EXPECT_FALSE(ibWebDispatchAllowed(false, false, "click"));
	EXPECT_FALSE(ibWebDispatchAllowed(false, false, "text"));
	EXPECT_FALSE(ibWebDispatchAllowed(false, false, "toggle"));
	EXPECT_FALSE(ibWebDispatchAllowed(false, true, "fetch"));
}

TEST(WebDispatchGate, ALockedControlRefusesANewValueAndAllowsACommand)
{
	EXPECT_FALSE(ibWebDispatchAllowed(true, true, "text"));
	EXPECT_FALSE(ibWebDispatchAllowed(true, true, "toggle"));
	EXPECT_FALSE(ibWebDispatchAllowed(true, true, "buttonClear"));
	EXPECT_FALSE(ibWebDispatchAllowed(true, true, "cell"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, true, "click"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, true, "buttonOpen"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, true, "buttonSelect"));
	EXPECT_TRUE(ibWebDispatchAllowed(true, true, "fetch"));
}

TEST(WebDispatchGate, TheRefusalBodyIsNotAnEmptyForm)
{
	const std::string body = ibWebDispatchRefusedBody();
	EXPECT_EQ(body, "{\"refused\":true}");
	EXPECT_NE(body, "{}");
}
