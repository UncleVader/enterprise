#ifndef __IB_WORKER_POOL_H__
#define __IB_WORKER_POOL_H__

// ibWorkerPool — task dispatcher with per-session sequential semantics.
//
// Submitting tasks for the same session enforces FIFO order; tasks for
// different sessions execute on different workers in parallel. The
// interpreter state (ibSession::GetPUState) is automatically visible
// to every task because workers bind their session via ibSessionScope
// before draining the queue — no explicit save/restore needed since
// state is owned by the session itself.
//
// Concrete backends:
//   - ibWorkerPoolHeadless (in this directory): N threads, per-session
//     queue + lease. A task that waits on a person (Await) is a fiber
//     pinned to its worker; the OS thread keeps serving other sessions.
//     Suitable for wenterprise-server.exe and the future oes-server.exe
//     compute server.
//   - ibWorkerPoolGUI (frontend/session/workerPoolGUI.{h,cpp}): wraps
//     wxTheApp's CallAfter so tasks run on the wx main thread. Implemented,
//     but NOT auto-installed — the desktop still runs script on the wx main
//     thread directly. The session arg is unused (desktop = one session).
//
// See docs/worker-pool-tls-audit.md (TLS migration prerequisite — done)
// and docs/compute-server-tiering.md (architectural roadmap).

#include "backend/backend.h"

#include <functional>
#include <future>
#include <memory>

class ibSession;

class BACKEND_API ibWorkerPool {
public:
	using Task = std::function<void()>;

	virtual ~ibWorkerPool() = default;

	// Schedule a task to run with `session` bound on the worker thread.
	// Returns a future that fulfils after the task runs or holds the
	// exception thrown by the task. Per-session order is FIFO; tasks
	// for distinct sessions run in parallel up to the worker count.
	virtual std::future<void> Submit(ibSession* session, Task task) = 0;

	// Convenience: submit + wait. Throws if the task threw.
	void RunOnSession(ibSession* session, Task task) {
		Submit(session, std::move(task)).get();
	}

	// Remove a session's queue and any internal bookkeeping the pool
	// holds for it. Caller must guarantee no in-flight tasks remain
	// (typical pattern: drain via RunOnSession, then DropSession). Used
	// at session teardown so the pool's per-session map doesn't leak
	// stale entries pointing at destroyed sessions.
	virtual void DropSession(ibSession* session) = 0;

	// (No cancel here. Stopping what a session is doing is the session's own command — ibSession::Cancel —
	//  because the session is what knows everything that is doing it: its thread, its connection, its tenants.
	//  A pool only runs tasks.)

	// Drain queues, signal workers to stop, join. Idempotent. Pending
	// tasks at the time of Stop run to completion before workers exit
	// — the pool acts as an actor-system shutdown, not a force-kill.
	// A fiber parked in Await is resumed so it can unwind; it does not
	// stay parked across shutdown.
	virtual void Stop() = 0;

	// Park the calling task until `done` returns true. Cancellation
	// (ibRunCancelled on the task's session, or Stop) throws
	// ibBackendInterruptException on the waiting task's stack, before
	// `done` is consulted again and before any task queued behind the
	// waiter is run under it.
	//
	// On the headless pool the waiter is a stackful fiber pinned to the
	// worker that started it, and the OS thread is free to run other
	// sessions. Wake resumes that session's fiber on its home thread so
	// it can re-check `done`. A question asked from a task that itself
	// ran while an outer question was waiting is a nested Await on the
	// same fiber: the outer frame cannot return before the inner one.
	//
	// Called from a task this pool is running. The base implementation
	// throws — the GUI pool does not park fibers (desktop questions are
	// modal on the UI thread, and that pool is out of this change).
	virtual void Await(std::function<bool()> done);
	virtual void Wake(ibSession* session);
};

#endif
