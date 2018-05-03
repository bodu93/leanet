#ifndef LEANET_EVENTLOOPTHREAD_H
#define LEANET_EVENTLOOPTHREAD_H

#include <string>
#include <functional>

#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"
#include "thread.h"

namespace leanet {

class EventLoop;

class EventLoopThread: noncopyable {
public:
	typedef std::function<void (EventLoop*)> ThreadInitCallback;

	explicit EventLoopThread(
			const ThreadInitCallback& cb = ThreadInitCallback(),
			const std::string& name = std::string());
	~EventLoopThread();

	EventLoop* startLoop();

private:
	void threadFunc();

	EventLoop* loop_;
	bool exiting_;
	Thread thread_;
	Mutex mutex_;
	Condition cond_;
	ThreadInitCallback callback_;
};

}

#endif
