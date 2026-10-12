#ifndef __WEB_APP_BORROW_H__
#define __WEB_APP_BORROW_H__

// Count of in-flight borrowers of one web application.
//
// Session teardown (ibWebSession::OnExit) destroys the application. A
// handler that looked the application up and then dropped the session-map
// lock must keep a borrow outstanding until it is done with the pointer.
// OnExit calls WaitIdle() and only then tears the application down, so the
// pointer stays alive for the whole borrow.
//
// The counter is the whole mechanism: Acquire on the way out of the map,
// Release when the handler's pin dies, WaitIdle before the application is
// destroyed. No wxWidgets types — the unit test drives it directly.

#include <condition_variable>
#include <mutex>

class ibWebAppBorrow {
public:
	void Acquire()
	{
		std::lock_guard<std::mutex> lock(m_mu);
		++m_held;
	}

	void Release()
	{
		std::lock_guard<std::mutex> lock(m_mu);
		if (m_held > 0)
			--m_held;
		if (m_held == 0)
			m_cv.notify_all();
	}

	// Block until every Acquire has been matched by Release.
	void WaitIdle()
	{
		std::unique_lock<std::mutex> lock(m_mu);
		++m_waiting;
		m_cv.wait(lock, [this] { return m_held == 0; });
		--m_waiting;
	}

	int Held() const
	{
		std::lock_guard<std::mutex> lock(m_mu);
		return m_held;
	}

	int Waiting() const
	{
		std::lock_guard<std::mutex> lock(m_mu);
		return m_waiting;
	}

private:
	mutable std::mutex      m_mu;
	std::condition_variable m_cv;
	int                     m_held    = 0;
	int                     m_waiting = 0;
};

#endif
