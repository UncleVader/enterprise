#include "workerPool.h"

#include <stdexcept>

void ibWorkerPool::Await(std::function<bool()> /*done*/)
{
	// The headless pool overrides this. The GUI pool does not park an OS
	// thread on a question — desktop questions are modal on the UI thread —
	// and calling the base from there would block that thread with no
	// fiber to resume. Fail loudly instead of pretending to wait.
	throw std::logic_error("ibWorkerPool::Await is implemented by the headless pool");
}

void ibWorkerPool::Wake(ibSession* /*session*/)
{
	throw std::logic_error("ibWorkerPool::Wake is implemented by the headless pool");
}
