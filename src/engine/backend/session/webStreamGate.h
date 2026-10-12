#ifndef __IB_WEB_STREAM_GATE_H__
#define __IB_WEB_STREAM_GATE_H__

// How many /stream connections may be open at once. Each one occupies a
// worker for as long as the browser holds it, so the global cap stays
// two below the pool and a signed-in session may hold only a few tabs.

#include "backend/backend.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// One EventSource per tab, a few tabs per session.
constexpr std::size_t ibWebStreamPerSessionCap = 4;

// Workers left for /ping and /login. A pool of one or two still allows
// a single stream.
inline std::size_t ibWebStreamGlobalCap(std::size_t poolMax)
{
	if (poolMax <= 2)
		return 1;
	return poolMax - 2;
}

// Queued requests past the pool are refused. Zero would mean "no bound".
inline std::size_t ibWebStreamQueueLimit(std::size_t poolMax)
{
	return poolMax == 0 ? 1 : poolMax;
}

// What the wait just told the stream to do. A session that is not live
// closes; it does not look like an idle timeout.
enum class ibWebStreamTick { Close, Ping, Push };

inline ibWebStreamTick ibWebStreamOnWait(bool live, std::uint64_t seq, std::uint64_t lastSeen)
{
	if (!live)
		return ibWebStreamTick::Close;
	if (seq == lastSeen)
		return ibWebStreamTick::Ping;
	return ibWebStreamTick::Push;
}

class BACKEND_API ibWebStreamGate {
public:
	ibWebStreamGate(std::size_t perSession, std::size_t global);
	~ibWebStreamGate();
	ibWebStreamGate(const ibWebStreamGate&) = delete;
	ibWebStreamGate& operator=(const ibWebStreamGate&) = delete;

	enum class Answer { Open, Empty, NotLive, SessionCap, GlobalCap };

	// `live` is the caller's fact: the session exists and is signed in.
	// A refusal takes no slot.
	Answer TryOpen(const std::string& sessionId, bool live);
	void Close(const std::string& sessionId);
	std::size_t OpenCount() const;

private:
	std::size_t m_perSessionCap;
	std::size_t m_globalCap;
	std::size_t m_open = 0;
	// The map is defined in the cpp so this header stays free of it.
	struct Counts;
	std::unique_ptr<Counts> m_counts;
};

#endif
