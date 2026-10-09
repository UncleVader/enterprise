#ifndef __IB_WORKER_POOL_HEADLESS_H__
#define __IB_WORKER_POOL_HEADLESS_H__

// Headless worker pool — N OS threads serving M sessions with
// per-session sequential dispatch (single in-flight task per session).
//
// Lease semantics: each session has a queue + an atomic "leased" flag.
// A worker claiming a session's queue CAS-flips leased→true and runs
// the lease on a fiber pinned to that worker. Tasks drain in FIFO
// order on that fiber. Await suspends the fiber and returns the OS
// thread to the scheduler; the lease stays held, so no other worker
// picks the session up. Wake / a queued task / cancel resumes the
// fiber on its home thread. Other sessions proceed in parallel on
// whatever workers are free.
//
// Reentrant Submit (a task running on session S calls Submit on the
// same session) runs inline rather than enqueuing — avoids the
// self-deadlock where a worker would queue work behind itself.
//
// Used by wenterprise-server.exe (replaces today's per-session worker
// thread in ibWebApplication) and the future oes-server.exe compute
// server. See docs/compute-server-tiering.md Phase 2 for the bigger
// picture.

#include "workerPool.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class ibFiber;

class BACKEND_API ibWorkerPoolHeadless : public ibWorkerPool {
public:
	// maxWorkers — hard cap on the number of OS threads the pool will
	// ever spawn concurrently. Workers spawn lazily on demand: ctor
	// creates none; the first Submit launches the first worker;
	// subsequent Submits spawn more (up to the cap) when no idle
	// worker is available. Idle workers self-exit after kIdleTimeout
	// of inactivity, except the last kMinIdle which stay alive for
	// low-latency response to the next Submit.
	explicit ibWorkerPoolHeadless(std::size_t maxWorkers);
	~ibWorkerPoolHeadless() override;

	std::future<void> Submit(ibSession* session, Task task) override;
	void              DropSession(ibSession* session) override;
	void              Stop() override;
	void              Await(std::function<bool()> done) override;
	void              Wake(ibSession* session) override;

	// Diagnostics — current worker counts. Useful for /admin endpoints
	// and load tests.
	std::size_t MaxWorkers()   const { return m_maxWorkers; }
	std::size_t AliveWorkers() const { return m_aliveWorkers.load(std::memory_order_acquire); }
	std::size_t IdleWorkers()  const { return m_idleWorkers.load(std::memory_order_acquire); }

private:
	struct ibSessionTask {
		Task                                task;
		std::shared_ptr<std::promise<void>> promise;
	};

	struct ibSessionQueue {
		std::deque<ibSessionTask> tasks;
		std::atomic<bool>         leased { false };
		// DROPPED WHILE LEASED. A session's teardown runs from inside one of its own
		// tasks — the task's closure can own the session holder — so DropSession can
		// arrive while a worker is standing on this very object. Erasing it there is
		// a use-after-free under the pool's own mutex. So the drop is RECORDED here
		// and the worker erases the queue itself when it lets the lease go.
		bool                      dropped { false };

		// The lease fiber, pinned to m_home. Non-null from the moment the
		// fiber is created until it has unwound and the home thread has
		// destroyed it. m_parked / m_wake are guarded by m_mtx; m_fiber and
		// m_home are touched only on the home thread.
		class ibFiber*            m_fiber = nullptr;
		std::thread::id           m_home{};
		bool                      m_parked = false;
		bool                      m_wake = false;
	};

	void WorkerLoop();

	// Spawn a new detached worker thread. Re-checks alive-vs-cap under
	// m_workersMtx to handle the race between two threads racing to
	// spawn; bumps m_aliveWorkers atomically before std::thread::detach.
	void TrySpawnWorker();

	// Find a session with pending tasks not currently leased and CAS
	// the lease in. Returns the session pointer + queue, or {nullptr,
	// nullptr} if no work is available. Must be called with m_mtx held.
	std::pair<ibSession*, ibSessionQueue*> ClaimSessionLocked();

	// A fiber parked on this worker. The vector is per OS thread: fibers
	// never migrate, so the scheduler on the home thread is the only
	// reader and the only writer.
	struct ibParked {
		ibSession*       session = nullptr;
		ibSessionQueue*  queue = nullptr;
		class ibFiber*   fiber = nullptr;
	};
	static thread_local std::vector<ibParked> tl_parked;
	static thread_local ibSessionQueue* tl_currentQueue;

	static void RunTask(ibSessionTask& item);
	static void RegisterFiberLocals();

	// Passed across the fiber entry. A nested type so the translation
	// unit can name the queue (private) without a friend.
	struct ibLeaseArgs {
		ibWorkerPoolHeadless* pool = nullptr;
		ibSession*            session = nullptr;
		ibSessionQueue*       queue = nullptr;
	};
	bool TakeRunnable(ibParked& out);
	static void LeaseEntry(void* raw);
	void        DrainLease(ibSession* session, ibSessionQueue* q);
	// Tasks that arrived while this lease is inside Await. Returns
	// without popping when the session is cancelled, so a teardown
	// barrier queued after the cancel stays in the queue and runs only
	// once the waiting task has unwound.
	void        DrainArrived(ibSession* session, ibSessionQueue* q);
	void        StartLease(ibSession* session, ibSessionQueue* q);
	void        FinishFiber(ibSession* session, ibSessionQueue* q, class ibFiber* fiber);
	bool        ShouldInterrupt(ibSession* session) const;
	// m_mtx must be held. True when a fiber parked on THIS thread should
	// be resumed: it was woken, it has queued tasks, the pool is
	// stopping, or its session was cancelled.
	bool        HasRunnableParkedLocked() const;

	std::size_t              m_maxWorkers;
	std::atomic<bool>        m_stop { false };

	// Worker spawn coordination + join replacement (detached threads).
	std::mutex               m_workersMtx;
	std::atomic<std::size_t> m_aliveWorkers { 0 };
	// Idle-count drives lazy growth: zero idle + below cap = spawn.
	std::atomic<std::size_t> m_idleWorkers  { 0 };
	// Stop() waits on this until m_aliveWorkers reaches 0 (every
	// detached worker has exited).
	std::mutex               m_stopMtx;
	std::condition_variable  m_stopCv;

	// Per-session queue + dispatch.
	mutable std::mutex                                                m_mtx;
	std::condition_variable                                           m_cv;
	std::unordered_map<ibSession*, std::unique_ptr<ibSessionQueue>>   m_sessions;

};

#endif
