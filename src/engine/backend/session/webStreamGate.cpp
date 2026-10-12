#include "backend/session/webStreamGate.h"

#include <mutex>
#include <unordered_map>

struct ibWebStreamGate::Counts {
	std::mutex mutex;
	std::unordered_map<std::string, std::size_t> perSession;
};

ibWebStreamGate::ibWebStreamGate(std::size_t perSession, std::size_t global)
	: m_perSessionCap(perSession == 0 ? 1 : perSession)
	, m_globalCap(global == 0 ? 1 : global)
	, m_counts(new Counts())
{
}

ibWebStreamGate::~ibWebStreamGate() = default;

ibWebStreamGate::Answer ibWebStreamGate::TryOpen(const std::string& sessionId, bool live)
{
	if (sessionId.empty())
		return Answer::Empty;
	if (!live)
		return Answer::NotLive;

	std::lock_guard<std::mutex> lock(m_counts->mutex);
	const auto it = m_counts->perSession.find(sessionId);
	const std::size_t n = it == m_counts->perSession.end() ? 0 : it->second;
	if (n >= m_perSessionCap)
		return Answer::SessionCap;
	if (m_open >= m_globalCap)
		return Answer::GlobalCap;

	if (it == m_counts->perSession.end())
		m_counts->perSession.emplace(sessionId, 1);
	else
		++it->second;
	++m_open;
	return Answer::Open;
}

void ibWebStreamGate::Close(const std::string& sessionId)
{
	std::lock_guard<std::mutex> lock(m_counts->mutex);
	const auto it = m_counts->perSession.find(sessionId);
	if (it == m_counts->perSession.end() || it->second == 0 || m_open == 0)
		return;
	--it->second;
	--m_open;
	if (it->second == 0)
		m_counts->perSession.erase(it);
}

std::size_t ibWebStreamGate::OpenCount() const
{
	std::lock_guard<std::mutex> lock(m_counts->mutex);
	return m_open;
}
