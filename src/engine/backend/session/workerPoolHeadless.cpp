#include "workerPoolHeadless.h"

#include "fiber.h"

#include "backend/session/session.h"   // ibSessionScope
#include "backend/backend_exception.h" // ibBackendException
#include "backend/compiler/procUnitState.h" // ibRunCancelled
#include "backend/diagnostics/journal.h"

#include <wx/log.h>

#include <chrono>
#include <stdexcept>

namespace {

// Which session this worker is leasing. Submit uses it to spot a
// reentrant call and run that task inline. It is fiber-scoped: while
// this fiber is parked the thread runs someone else, and the value has
// to come back as it was when the fiber resumes.
thread_local ibSession* tl_currentLease = nullptr;

// Idle worker self-exits after this much inactivity, unless it's one
// of the last kMinIdle survivors which stay alive for fast response
// on the next Submit. A worker with a parked fiber is not idle in the
// sense that matters for shutdown: it must stay to resume that fiber.
constexpr auto       kIdleTimeout = std::chrono::seconds(30);
constexpr std::size_t kMinIdle    = 1;

// How often Stop() says out loud that it is still waiting. Not a
// deadline — the wait is unbounded by design; see Stop().
constexpr auto       kStopWaitReport = std::chrono::seconds(5);

// A cancel lives on the session, not on this condition variable, so a
// worker blocked in the wait does not hear it. While a fiber is parked
// the wait is short enough that the script still unwinds without anyone
// calling Wake. A person thinking is not on this timescale.
constexpr auto       kCancelSlice = std::chrono::milliseconds(50);

void LogWorkerException(const wxString& location)
{
	// Reraise to identify the type without losing the original exception_ptr —
	// outer catch(...) keeps current_exception() valid across this nested
	// rethrow, so set_exception below still gets the same one.
	try { throw; }
	catch (const ibBackendException& e) {
		ibJournalWarning(wxT("session.worker"),wxT("%s: ibBackendException: %s"),
		             location, e.GetErrorDescription());
	}
	catch (const std::exception& e) {
		ibJournalWarning(wxT("session.worker"),wxT("%s: std::exception: %s"),
		             location, wxString::FromUTF8(e.what()));
	}
	catch (...) {
		ibJournalWarning(wxT("session.worker"),wxT("%s: unknown exception"), location);
	}
}

} // namespace

thread_local std::vector<ibWorkerPoolHeadless::ibParked> ibWorkerPoolHeadless::tl_parked;
thread_local ibWorkerPoolHeadless::ibSessionQueue* ibWorkerPoolHeadless::tl_currentQueue = nullptr;

void ibWorkerPoolHeadless::RegisterFiberLocals()
{
	// Once per process, and before any fiber snapshot. The lambdas sit
	// directly in this member so they can name the private queue type;
	// a lambda nested in another lambda would not. The other slots
	// register themselves at static init, which has already run by the
	// time a pool exists.
	static std::mutex gate;
	static std::atomic<bool> done{ false };
	if (done.load(std::memory_order_acquire))
		return;
	std::lock_guard<std::mutex> lk(gate);
	if (done.load(std::memory_order_relaxed))
		return;
	ibFiberLocals::RegisterTrivial<ibSession*>(
		[](void* dst) { *static_cast<ibSession**>(dst) = tl_currentLease; },
		[](const void* src) { tl_currentLease = *static_cast<ibSession* const*>(src); });
	ibFiberLocals::RegisterTrivial<ibSessionQueue*>(
		[](void* dst) { *static_cast<ibSessionQueue**>(dst) = tl_currentQueue; },
		[](const void* src) { tl_currentQueue = *static_cast<ibSessionQueue* const*>(src); });
	done.store(true, std::memory_order_release);
}

ibWorkerPoolHeadless::ibWorkerPoolHeadless(std::size_t maxWorkers)
	: m_maxWorkers(maxWorkers > 0 ? maxWorkers : 1)
{
	RegisterFiberLocals();
	// Lazy spawn — no workers at construction. The first Submit kicks
	// the first worker into existence; load growth spawns more up to
	// m_maxWorkers; idle ones eventually self-exit via timeout.
}

ibWorkerPoolHeadless::~ibWorkerPoolHeadless()
{
	Stop();
}

void ibWorkerPoolHeadless::TrySpawnWorker()
{
	std::lock_guard<std::mutex> lk(m_workersMtx);
	// Re-check inside the lock so two concurrent Submits don't both
	// spawn past the cap.
	if (m_aliveWorkers.load(std::memory_order_acquire) >= m_maxWorkers)
		return;
	if (m_stop.load(std::memory_order_acquire))
		return;
	m_aliveWorkers.fetch_add(1, std::memory_order_acq_rel);
	// Detached: the thread takes care of its own lifetime; Stop() waits
	// on m_stopCv until m_aliveWorkers reaches 0. Avoids tracking handles
	// in a vector that has to be cleaned up when workers self-exit.
	std::thread(&ibWorkerPoolHeadless::WorkerLoop, this).detach();
}

std::future<void> ibWorkerPoolHeadless::Submit(ibSession* session, Task task)
{
	auto promise = std::make_shared<std::promise<void>>();
	auto future  = promise->get_future();

	// Pool already stopped — reject submission with a ready exception
	// future rather than enqueuing a task no one will ever run.
	if (m_stop.load(std::memory_order_acquire)) {
		promise->set_exception(std::make_exception_ptr(
			std::runtime_error("worker pool is stopped")));
		return future;
	}

	// Reentrant Submit on the same session running on this fiber —
	// run inline. The fiber is already leasing this session and would
	// otherwise wait forever for itself to release. A parked fiber has
	// its lease saved away, so a submit from another session on this
	// same OS thread does not look reentrant.
	if (tl_currentLease != nullptr && tl_currentLease == session) {
		try {
			task();
			promise->set_value();
		}
		catch (...) {
			LogWorkerException(wxT("worker pool reentrant task"));
			promise->set_exception(std::current_exception());
		}
		return future;
	}

	{
		std::unique_lock<std::mutex> lk(m_mtx);
		auto& slot = m_sessions[session];
		if (!slot) slot = std::make_unique<ibSessionQueue>();
		slot->tasks.push_back({ std::move(task), std::move(promise) });
	}
	m_cv.notify_all();

	// Lazy spawn. If no worker is currently idle and we're below the
	// cap, kick a new one into existence. m_idleWorkers is incremented
	// in WorkerLoop right before the CV wait and decremented on wake-up
	// — so "idle == 0" means every alive worker is busy on a session.
	// A parked question does not count as busy: its fiber is suspended
	// and the thread is back in this wait.
	if (m_idleWorkers.load(std::memory_order_acquire) == 0
	 && m_aliveWorkers.load(std::memory_order_acquire) < m_maxWorkers) {
		TrySpawnWorker();
	}

	return future;
}

void ibWorkerPoolHeadless::DropSession(ibSession* session)
{
	std::unique_lock<std::mutex> lk(m_mtx);
	auto it = m_sessions.find(session);
	if (it == m_sessions.end())
		return;
	// A LEASED QUEUE IS NOT OURS TO ERASE — a fiber is standing on it right
	// now, parked or running, and the teardown that called us usually runs
	// from inside one of its tasks. Record the drop; the home thread erases
	// the queue when the fiber has unwound and the lease is released.
	if (it->second && it->second->leased.load(std::memory_order_acquire)) {
		it->second->dropped = true;
		return;
	}
	m_sessions.erase(it);
}

void ibWorkerPoolHeadless::Wake(ibSession* session)
{
	if (session == nullptr)
		return;
	{
		std::lock_guard<std::mutex> lk(m_mtx);
		auto it = m_sessions.find(session);
		if (it == m_sessions.end() || !it->second)
			return;
		it->second->m_wake = true;
	}
	// The home thread is the one that can switch to the fiber. Every
	// worker wakes, and the one that parked it notices. notify_all rather
	// than notify_one: the idle worker that holds the fiber may not be
	// the one a single wake would pick.
	m_cv.notify_all();
}

bool ibWorkerPoolHeadless::ShouldInterrupt(ibSession* session) const
{
	if (m_stop.load(std::memory_order_acquire))
		return true;
	return session != nullptr && ibRunCancelled(session->RunState());
}

bool ibWorkerPoolHeadless::HasRunnableParkedLocked() const
{
	for (const ibParked& parked : tl_parked) {
		if (parked.queue == nullptr)
			continue;
		if (parked.queue->m_wake || !parked.queue->tasks.empty())
			return true;
		if (m_stop.load(std::memory_order_acquire))
			return true;
		if (parked.session != nullptr && ibRunCancelled(parked.session->RunState()))
			return true;
	}
	return false;
}

void ibWorkerPoolHeadless::RunTask(ibSessionTask& item)
{
	try {
		item.task();
		item.promise->set_value();
	}
	catch (...) {
		// Always log — PostWork drops the future, so without this
		// log the exception is silently swallowed (timer-driven
		// script bugs would mask). RunOnWorker callers will see
		// double-logging when they handle the rethrown exception
		// themselves; that's an acceptable cost for the safety win.
		LogWorkerException(wxT("worker pool task"));
		item.promise->set_exception(std::current_exception());
	}
}

void ibWorkerPoolHeadless::DrainLease(ibSession* /*session*/, ibSessionQueue* q)
{
	// Top-level drain. A task that Await threw out of has already left
	// the stack by the time we get back here, so a barrier queued behind
	// it runs now — not under the waiter. Cancel is intentionally not
	// checked: Stop's contract is that queued work still runs, and the
	// cancel belongs to the task that was waiting.
	for (;;) {
		ibSessionTask item;
		{
			std::unique_lock<std::mutex> lk(m_mtx);
			if (q->tasks.empty())
				return;
			item = std::move(q->tasks.front());
			q->tasks.pop_front();
		}
		RunTask(item);
		// item dies HERE, inside the lease — the task's closure may own
		// the session, and teardown must see tl_currentLease still set.
	}
}

void ibWorkerPoolHeadless::DrainArrived(ibSession* session, ibSessionQueue* q)
{
	for (;;) {
		// BEFORE the pop. A teardown submitted after cancel must stay in
		// the queue so the lease drain runs it once Await has unwound,
		// not on this fiber frame underneath the waiting script.
		if (ShouldInterrupt(session))
			return;
		ibSessionTask item;
		{
			std::unique_lock<std::mutex> lk(m_mtx);
			if (ShouldInterrupt(session) || q->tasks.empty())
				return;
			item = std::move(q->tasks.front());
			q->tasks.pop_front();
		}
		RunTask(item);
	}
}

void ibWorkerPoolHeadless::Await(std::function<bool()> done)
{
	ibFiber* const self = ibFiber::Current();
	if (self == nullptr || self->IsScheduler())
		throw std::logic_error("ibWorkerPoolHeadless::Await called off a pool fiber");
	ibSession* const session = tl_currentLease;
	ibSessionQueue* const q = tl_currentQueue;
	if (session == nullptr || q == nullptr)
		throw std::logic_error("ibWorkerPoolHeadless::Await called without a session lease");

	for (;;) {
		if (ShouldInterrupt(session))
			ibBackendInterruptException::Error();
		DrainArrived(session, q);
		if (ShouldInterrupt(session))
			ibBackendInterruptException::Error();
		if (!done || done())
			return;

		tl_parked.push_back(ibParked{ session, q, self });
		bool park = true;
		{
			std::lock_guard<std::mutex> lk(m_mtx);
			q->m_parked = true;
			// A wake or a task that landed while we were deciding, or a
			// cancel that landed in the same window: don't switch away.
			// The flag is consumed so a stale wake cannot spin the loop.
			if (q->m_wake || !q->tasks.empty() || ShouldInterrupt(session)) {
				q->m_wake = false;
				q->m_parked = false;
				park = false;
			}
		}
		if (!park) {
			tl_parked.pop_back();
			continue;
		}

		self->SwitchTo(ibFiber::Scheduler());

		{
			std::lock_guard<std::mutex> lk(m_mtx);
			q->m_parked = false;
		}
	}
}

void ibWorkerPoolHeadless::LeaseEntry(void* raw)
{
	std::unique_ptr<ibLeaseArgs> args(static_cast<ibLeaseArgs*>(raw));
	ibWorkerPoolHeadless* const pool = args->pool;
	ibSession* const session = args->session;
	ibSessionQueue* const q = args->queue;
	args.reset();

	// The scope's previous-binding lives on THIS stack. The map slot it
	// writes is per OS thread, so the fiber snapshot (registered from
	// session.cpp) is what puts the binding back when we resume — the
	// scope destructor only runs when the lease actually ends.
	ibSessionScope scope(session);
	struct ClearLease {
		~ClearLease()
		{
			tl_currentLease = nullptr;
			tl_currentQueue = nullptr;
		}
	} clear;
	tl_currentLease = session;
	tl_currentQueue = q;
	pool->DrainLease(session, q);
}

void ibWorkerPoolHeadless::StartLease(ibSession* session, ibSessionQueue* q)
{
	auto* args = new ibLeaseArgs();
	args->pool = this;
	args->session = session;
	args->queue = q;
	ibFiber* fiber = nullptr;
	try {
		fiber = ibFiber::Create(&ibWorkerPoolHeadless::LeaseEntry, args, ibFiber::kStackReserve);
	}
	catch (...) {
		delete args;
		throw;
	}
	q->m_fiber = fiber;
	q->m_home = std::this_thread::get_id();
	ibFiber::Scheduler()->SwitchTo(fiber);
	if (fiber->Finished())
		FinishFiber(session, q, fiber);
}

void ibWorkerPoolHeadless::FinishFiber(ibSession* session, ibSessionQueue* q, ibFiber* fiber)
{
	if (std::exception_ptr escaped = fiber->TakeException()) {
		try {
			std::rethrow_exception(escaped);
		}
		catch (...) {
			LogWorkerException(wxT("worker pool fiber"));
		}
	}

	bool more = false;
	{
		std::lock_guard<std::mutex> lk(m_mtx);
		q->m_fiber = nullptr;
		q->m_parked = false;
		q->m_wake = false;
		q->leased.store(false);
		more = !q->tasks.empty();
		auto it = m_sessions.find(session);
		if (it != m_sessions.end() && it->second.get() == q
		    && q->dropped && q->tasks.empty()) {
			m_sessions.erase(it);
			more = false;
		}
	}
	if (more)
		m_cv.notify_all();
	// The fiber has unwound (LeaseEntry returned, its scopes destroyed).
	// Only now is the stack free of live objects.
	ibFiber::Destroy(fiber);
}

std::pair<ibSession*, ibWorkerPoolHeadless::ibSessionQueue*>
ibWorkerPoolHeadless::ClaimSessionLocked()
{
	for (auto& kv : m_sessions) {
		ibSessionQueue* q = kv.second.get();
		if (q->tasks.empty()) continue;
		bool expected = false;
		if (q->leased.compare_exchange_strong(expected, true))
			return { kv.first, q };
	}
	return { nullptr, nullptr };
}

bool ibWorkerPoolHeadless::TakeRunnable(ibParked& out)
{
	for (auto it = tl_parked.begin(); it != tl_parked.end(); ++it) {
		bool run = false;
		{
			std::lock_guard<std::mutex> lk(m_mtx);
			ibSessionQueue* q = it->queue;
			if (q == nullptr)
				continue;
			run = q->m_wake || !q->tasks.empty()
				|| m_stop.load(std::memory_order_acquire)
				|| (it->session != nullptr && ibRunCancelled(it->session->RunState()));
			if (run)
				q->m_wake = false;
		}
		if (!run)
			continue;
		out = *it;
		tl_parked.erase(it);
		return true;
	}
	return false;
}

void ibWorkerPoolHeadless::WorkerLoop()
{
	// RAII-guard for the m_aliveWorkers decrement + Stop-cv notify.
	// Pre-2026-05-26 this bookkeeping lived at function tail; an
	// exception escaping the inner try (set_exception OOM, predicate
	// fault, ibSessionScope ctor throwing) bypassed it, the worker
	// died alive-counted, and Stop() blocked forever on the cv. Tying
	// it to a local dtor closes that hole: any path out of WorkerLoop
	// (clean exit, exception, std::terminate after std::set_terminate
	// transforms it back) walks past this and decrements once.
	struct BookkeepingOnExit {
		ibWorkerPoolHeadless* self;
		~BookkeepingOnExit() {
			self->m_aliveWorkers.fetch_sub(1, std::memory_order_acq_rel);
			std::lock_guard<std::mutex> lk(self->m_stopMtx);
			self->m_stopCv.notify_all();
		}
	} bookkeeping{ this };

	try {
	ibFiber::ConvertThread();
	struct ReleaseScheduler {
		~ReleaseScheduler() { ibFiber::ReleaseThread(); }
	} releaseScheduler;

	for (;;) {
		ibParked runnable;
		if (TakeRunnable(runnable)) {
			ibFiber::Scheduler()->SwitchTo(runnable.fiber);
			if (runnable.fiber->Finished())
				FinishFiber(runnable.session, runnable.queue, runnable.fiber);
			continue;
		}

		if (m_stop.load(std::memory_order_acquire) && tl_parked.empty())
			break;

		ibSession*       session = nullptr;
		ibSessionQueue*  q       = nullptr;
		bool             gotWork = false;

		{
			std::unique_lock<std::mutex> lk(m_mtx);
			m_idleWorkers.fetch_add(1, std::memory_order_acq_rel);
			const auto slice = tl_parked.empty() ? kIdleTimeout : kCancelSlice;
			gotWork = m_cv.wait_for(lk, slice, [this, &session, &q]() {
				session = nullptr;
				q = nullptr;
				if (HasRunnableParkedLocked())
					return true;
				if (m_stop.load(std::memory_order_acquire))
					return true;
				auto pair = ClaimSessionLocked();
				session = pair.first;
				q       = pair.second;
				return q != nullptr;
			});
			m_idleWorkers.fetch_sub(1, std::memory_order_acq_rel);
		}

		if (m_stop.load(std::memory_order_acquire) && tl_parked.empty() && q == nullptr)
			break;

		if (!gotWork || q == nullptr) {
			// Idle timeout, or we woke because a parked fiber is runnable
			// (handled at the top of the loop). Self-exit only when this
			// thread holds nothing parked — a parked fiber's home thread
			// is the only thread that can resume it.
			if (!tl_parked.empty())
				continue;
			if (m_aliveWorkers.load(std::memory_order_acquire) > kMinIdle)
				break;
			continue;
		}

		// Bind session for the duration of the lease — on the FIBER, not
		// on this thread stack. Current() and GetPUState() resolve to this
		// session inside every task, and the snapshot puts that binding
		// back after the fiber has been parked under somebody else.
		try {
			StartLease(session, q);
		}
		catch (...) {
			LogWorkerException(wxT("worker pool lease"));
			{
				std::lock_guard<std::mutex> lk(m_mtx);
				q->leased.store(false);
				q->m_fiber = nullptr;
			}
			m_cv.notify_all();
		}
	}
	}
	catch (...) {
		// Last-chance log of an unexpected escape from the inner loop —
		// every individual task is already wrapped above, so reaching
		// here implies a fault in the worker scaffolding itself
		// (ibSessionScope ctor, mutex lock, set_exception OOM). Log,
		// then fall through to BookkeepingOnExit. Without this catch,
		// std::terminate would fire and skip the bookkeeping dtor.
		LogWorkerException(wxT("worker pool loop"));
	}

	// m_aliveWorkers decrement + Stop-cv notify happen in BookkeepingOnExit's
	// dtor — guarantees one notify per WorkerLoop entry regardless of how
	// we leave.
}

void ibWorkerPoolHeadless::Stop()
{
	{
		std::unique_lock<std::mutex> lk(m_mtx);
		m_stop.store(true);
		// CANCEL EVERY KNOWN SESSION. m_stop alone is only read
		// between tasks — a task already running reads nothing, and a task
		// that blocks for minutes (the Firebird maintenance poll) turns
		// this wait into a hang with no way out. The session's cancel
		// is what such a task hears, so shutdown sends it here, before
		// waiting for anyone. Under m_mtx because the queue entry pins
		// nothing beyond the pointer we hold; Cancel takes the connection
		// pool's lock and the job manager's, and neither ever calls back
		// into this pool.
		//
		// A fiber parked in Await hears the same signal on resume
		// (ShouldInterrupt) and throws on its own stack, so Stop does not
		// return while a fiber is still suspended.
		for (auto& kv : m_sessions)
			if (kv.first != nullptr) kv.first->Cancel();
	}
	m_cv.notify_all();

	// Wait for every detached worker to exit. m_aliveWorkers decrements
	// at the end of each WorkerLoop and notifies m_stopCv.
	//
	// Timed, and it keeps waiting — leaving early would let a detached
	// worker run on into session teardown, which is the use-after-free
	// this drain exists to prevent. What the deadline buys is a VOICE:
	// a wait that says nothing is indistinguishable from a deadlock, and
	// reading that difference cost a full-memory dump (2026-08-03: main
	// parked here while a worker sat in the Firebird sweep poll, which
	// was passing nullptr for its cancel token).
	std::unique_lock<std::mutex> lk(m_stopMtx);
	while (!m_stopCv.wait_for(lk, kStopWaitReport, [this]() {
		return m_aliveWorkers.load(std::memory_order_acquire) == 0;
	})) {
		ibJournalWarning(wxT("session.worker"),wxT("worker pool: still waiting on %lu worker(s) after stop"),
		             (unsigned long)m_aliveWorkers.load(std::memory_order_acquire));
	}
}
