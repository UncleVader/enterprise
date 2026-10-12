#ifndef __WEB_SESSION_H__
#define __WEB_SESSION_H__

// Per-session runtime context for the web frontend.
//
// Owns everything that must be isolated between concurrent web users:
// the module manager's main-module instance, the ibProcUnit bytecode
// interpreter, the logical main frame holding "open" documents, and any
// session-scoped variables. The metadata itself (compiled modules,
// form descriptors, catalog schemas, ...) lives on the singleton
// activeMetaData and is shared across sessions.
//
// Lifecycle mirrors the desktop BeforeRun / RunDatabase / Close flow:
//
//   ctor → OnInit(user, password)  [authorises + CreateMainModule]
//        │
//        │   ... HandleRequest(...) on every HTTP hit ...
//        │
//        └─ OnExit()                [DestroyMainModule + cleanup]
//     dtor

#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>

#include <wx/string.h>

#include "backend/session/sessionHolder.h"

#include "webAppBorrow.h"

class ibWebApplication;
class ibSession;

// Keeps one ibWebApplication alive for the caller. Released in the
// destructor, which is what lets ibWebSession::OnExit finish tearing
// the application down. Move-only: the borrow count is acquired once,
// in the constructing PinApp(), and released once.
class ibWebAppPin {
public:
	ibWebAppPin() = default;
	ibWebAppPin(ibWebAppPin&& other) noexcept
		: m_app(std::move(other.m_app))
		, m_borrow(std::move(other.m_borrow))
	{
	}
	ibWebAppPin& operator=(ibWebAppPin&& other) noexcept
	{
		if (this != &other) {
			Release();
			m_app    = std::move(other.m_app);
			m_borrow = std::move(other.m_borrow);
		}
		return *this;
	}
	ibWebAppPin(const ibWebAppPin&)            = delete;
	ibWebAppPin& operator=(const ibWebAppPin&) = delete;
	~ibWebAppPin() { Release(); }

	ibWebApplication* get() const { return m_app.get(); }
	explicit operator bool() const { return m_app != nullptr; }

private:
	friend class ibWebSession;

	ibWebAppPin(std::shared_ptr<ibWebApplication> app,
		std::shared_ptr<ibWebAppBorrow> borrow)
		: m_app(std::move(app))
		, m_borrow(std::move(borrow))
	{
		if (m_app && m_borrow)
			m_borrow->Acquire();
	}

	void Release()
	{
		if (m_borrow) {
			m_borrow->Release();
			m_borrow.reset();
		}
		m_app.reset();
	}

	std::shared_ptr<ibWebApplication> m_app;
	std::shared_ptr<ibWebAppBorrow>   m_borrow;
};

class ibWebSession {
public:
	ibWebSession(wxString id, wxString user);
	~ibWebSession();

	// Lifecycle:
	//   OnInit()  — lightweight: reads the configuration name, registers
	//               the session in the manager. Runs with no user
	//               identity yet and does NOT start a runtime.
	//   Login()   — called after the user authenticates (POST /login or
	//               auto-login when sys_user is empty). Sets the user
	//               identity, then spins up the ibWebApplication which
	//               builds the main frame and calls CreateMainModule.
	//   OnExit()  — tears the runtime down.
	bool OnInit();
	bool Login(const wxString& user, const wxString& password);
	void OnExit();

	bool IsAuthenticated() const { return m_app != nullptr; }

	const wxString& Id()         const { return m_id; }
	const wxString& User()       const { return m_user; }
	const wxString& ConfigName() const { return m_configName; }
	// Raw pointer. Valid only while the caller holds the ibWebAppPin from
	// PinApp() (or while OnExit has not yet passed its borrow wait).
	// Handing this out from under the session-map lock and using it after
	// the lock is gone is a use-after-free: OnExit destroys the application.
	ibWebApplication*  App()        const { return m_app.get(); }
	// Copy of the application plus a borrow that OnExit waits on. Empty
	// when the session is not logged in, or when teardown has started.
	ibWebAppPin PinApp() const;
	// Raw ibSession pointer from the ticket — for HTTP handlers that need
	// to pin ibSessionScope on their worker thread so ibSession::Current()
	// resolves for moduleManager->GetProcUnit() etc. Nullptr before Login.
	class ibSession* Session() const;

	// Milliseconds since epoch at last request — used later for idle
	// timeout eviction in the session manager.
	std::int64_t LastActiveMs() const;
	void         Touch();

private:
	wxString m_id;
	wxString m_user;
	wxString m_configName;

	mutable std::mutex m_mutex;
	std::int64_t       m_lastActiveMs;
	bool               m_initialized = false;

	// Lifecycle serializer — locks Login and OnExit on the same instance
	// against each other. Rapid F5 races SessionManager::Login (holds a
	// keeper shared_ptr) against SessionManager::Destroy (also holds a
	// keeper) on the same ibWebSession; without this the two ran
	// concurrently and Destroy's m_session.reset() freed the ibSession
	// while Login's worker thread was still about to start with
	// the app still pointing at it. Recursive in case future code
	// calls OnExit re-entrantly from the same thread (dtor → OnExit →
	// internal teardown). Distinct from m_mutex which only guards the
	// activity timestamp accessors.
	mutable std::recursive_mutex m_lifecycleMutex;

	// Per-session "app" — owns the module manager, future frame, etc.
	// Created lazily in OnInit so a failed authentication does not
	// leave a half-initialised app behind. Shared so a handler can
	// hold the object (via ibWebAppPin) across OnExit's wait; OnExit
	// still destroys it, but only after every pin has released.
	std::shared_ptr<ibWebApplication> m_app;
	std::shared_ptr<ibWebAppBorrow>   m_borrow;

	// Set at the start of OnExit. PinApp() and Login() refuse once this
	// is set, so teardown does not gain new borrowers while it waits
	// for the ones already in flight.
	bool m_closing = false;

	// Watch, not ownership. The web main window (ibWebFrame) owns this
	// tab's session, exactly as the desktop main window owns its own —
	// so tearing down m_app destroys the frame, which releases the
	// holder, which ends the session. This watch just lets us observe it
	// in between, and answers empty once the frame is gone.
	ibSessionWatch m_session;
};

#endif
