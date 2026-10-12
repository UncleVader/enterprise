#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

#include "frontend/web/webAppBorrow.h"

// The session map hands a raw ibWebApplication* out from under its lock.
// Teardown is only safe when it waits until every such borrow has released.
// This is that wait, without standing up a web session.

TEST(WebAppBorrow, WaitIdleReturnsImmediatelyWhenNothingIsHeld)
{
	ibWebAppBorrow gate;
	gate.WaitIdle();
	EXPECT_EQ(gate.Held(), 0);
	EXPECT_EQ(gate.Waiting(), 0);
}

TEST(WebAppBorrow, WaitIdleBlocksUntilTheLastRelease)
{
	auto gate = std::make_shared<ibWebAppBorrow>();
	gate->Acquire();
	gate->Acquire();

	std::atomic<bool> idle{ false };
	std::thread waiter([gate, &idle] {
		gate->WaitIdle();
		idle.store(true, std::memory_order_release);
	});

	while (gate->Waiting() == 0)
		std::this_thread::yield();

	EXPECT_FALSE(idle.load(std::memory_order_acquire));
	EXPECT_EQ(gate->Held(), 2);

	gate->Release();
	EXPECT_FALSE(idle.load(std::memory_order_acquire));
	EXPECT_EQ(gate->Held(), 1);

	gate->Release();
	waiter.join();
	EXPECT_TRUE(idle.load(std::memory_order_acquire));
	EXPECT_EQ(gate->Held(), 0);
	EXPECT_EQ(gate->Waiting(), 0);
}

TEST(WebAppBorrow, TeardownWaitsForThePinAndRejectsALateOne)
{
	// Same shape as ibWebSession::PinApp / OnExit. The closer publishes
	// "closing", waits until the outstanding pin releases, and only then
	// destroys the object. A pin taken after closing is empty and does
	// not add a borrow the closer would have to wait for.
	std::atomic<int> alive{ 0 };
	struct Token {
		std::atomic<int>* counter;
		explicit Token(std::atomic<int>* c) : counter(c) { counter->fetch_add(1); }
		~Token() { counter->fetch_sub(1); }
	};
	struct Shelf {
		std::shared_ptr<ibWebAppBorrow> gate = std::make_shared<ibWebAppBorrow>();
		std::shared_ptr<Token>          obj;
		std::mutex                      mu;
		bool                            closing = false;

		explicit Shelf(std::atomic<int>* counter)
			: obj(std::make_shared<Token>(counter))
		{
		}

		std::shared_ptr<Token> Pin()
		{
			std::lock_guard<std::mutex> lock(mu);
			if (closing || !obj)
				return {};
			gate->Acquire();
			return obj;
		}

		void Unpin(std::shared_ptr<Token>& pin)
		{
			pin.reset();
			gate->Release();
		}

		void Close()
		{
			{
				std::lock_guard<std::mutex> lock(mu);
				closing = true;
			}
			gate->WaitIdle();
			std::lock_guard<std::mutex> lock(mu);
			obj.reset();
		}
	};

	Shelf shelf(&alive);
	auto pin = shelf.Pin();
	ASSERT_TRUE(pin);
	EXPECT_EQ(alive.load(), 1);

	std::thread closer([&shelf] { shelf.Close(); });
	while (shelf.gate->Waiting() == 0)
		std::this_thread::yield();

	auto late = shelf.Pin();
	EXPECT_FALSE(late);
	EXPECT_EQ(shelf.gate->Held(), 1);
	EXPECT_EQ(alive.load(), 1);

	shelf.Unpin(pin);
	closer.join();
	EXPECT_EQ(alive.load(), 0);
}
