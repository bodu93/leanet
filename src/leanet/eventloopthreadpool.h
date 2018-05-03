#ifndef LEANET_EVENTLOOPTHREADPOOL_H
#define LEANET_EVENTLOOPTHREADPOOL_H

#include <memory>
#include <functional>
#include <vector>
#include <string>

#include "noncopyable.h"

namespace leanet {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool: noncopyable {
public:
	typedef std::function<void (EventLoop*)> ThreadInitCallback;

	EventLoopThreadPool(EventLoop* baseLoop, const std::string& name);
	~EventLoopThreadPool();

	void setThreadNum(int numThreads)
	{ numThreads_ = numThreads; }

	void start(const ThreadInitCallback& cb);
	bool started() const
	{ return started_; }

	EventLoop* getNextLoop();

private:
	EventLoop* baseLoop_;
	const std::string name_;
	bool started_;
	int numThreads_;
	int next_;

	std::vector<std::shared_ptr<EventLoopThread>> threads_;
	std::vector<EventLoop*> loops_;
};

}

#endif
