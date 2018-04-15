#ifndef LEANET_EVENTLOOP_H
#define LEANET_EVENTLOOP_H

#include <leanet/noncopyable.h>
#include <leanet/currentthread.h>

namespace leanet {

class EventLoop: noncopyable {
public:
	EventLoop();
	~EventLoop();

	void loop();

	void assertInLoopThread() {
		if (!isInLoopThread()) {
			abortNotInLoopThread();
		}
	}

	bool isInLoopThread() const {
		return threadId_ == currentThread::tid();
	}

	static EventLoop* getEventLoopOfCurrentThread();

private:
	void abortNotInLoopThread();

	bool looping_; // atomic
	const uint64_t threadId_;
};

} // namespace leanet

#endif
