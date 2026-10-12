// =============================================================================
// /stream admission. A session that is not signed in takes no slot and is
// not told to ping. Caps are per session and for the whole process.
// =============================================================================

#include <gtest/gtest.h>

#include "backend/session/webStreamGate.h"

TEST(WebStreamGate, ALiveSessionOpensAndCloses)
{
	ibWebStreamGate gate(2, 4);
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("a", true));
	EXPECT_EQ(1u, gate.OpenCount());
	gate.Close("a");
	EXPECT_EQ(0u, gate.OpenCount());
	gate.Close("a");
	EXPECT_EQ(0u, gate.OpenCount());
}

TEST(WebStreamGate, AnUnsignedSessionTakesNoSlot)
{
	ibWebStreamGate gate(2, 4);
	EXPECT_EQ(ibWebStreamGate::Answer::Empty, gate.TryOpen("", true));
	EXPECT_EQ(ibWebStreamGate::Answer::NotLive, gate.TryOpen("anon", false));
	EXPECT_EQ(ibWebStreamGate::Answer::NotLive, gate.TryOpen("gone", false));
	EXPECT_EQ(0u, gate.OpenCount());
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("signed-in", true));
}

TEST(WebStreamGate, CapsRefuseAndACloseFreesTheSlot)
{
	ibWebStreamGate gate(2, 3);
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("a", true));
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("a", true));
	EXPECT_EQ(ibWebStreamGate::Answer::SessionCap, gate.TryOpen("a", true));
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("b", true));
	EXPECT_EQ(ibWebStreamGate::Answer::GlobalCap, gate.TryOpen("c", true));
	EXPECT_EQ(3u, gate.OpenCount());

	gate.Close("a");
	EXPECT_EQ(ibWebStreamGate::Answer::Open, gate.TryOpen("c", true));
	EXPECT_EQ(3u, gate.OpenCount());
}

TEST(WebStreamTick, ADeadSessionClosesInsteadOfPinging)
{
	EXPECT_EQ(ibWebStreamTick::Close, ibWebStreamOnWait(false, 1, 1));
	EXPECT_EQ(ibWebStreamTick::Close, ibWebStreamOnWait(false, 4, 1));
	EXPECT_EQ(ibWebStreamTick::Ping, ibWebStreamOnWait(true, 7, 7));
	EXPECT_EQ(ibWebStreamTick::Push, ibWebStreamOnWait(true, 8, 7));
}

TEST(WebStreamGate, ThePoolKeepsWorkersForOrdinaryRequests)
{
	EXPECT_EQ(1u, ibWebStreamGlobalCap(1));
	EXPECT_EQ(1u, ibWebStreamGlobalCap(2));
	EXPECT_EQ(6u, ibWebStreamGlobalCap(8));
	EXPECT_EQ(1u, ibWebStreamQueueLimit(0));
	EXPECT_EQ(8u, ibWebStreamQueueLimit(8));
}
