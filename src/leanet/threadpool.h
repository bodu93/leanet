#ifndef LEANET_THREADPOOL_H
#define LEANET_THREADPOOL_H

#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <memory>

#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"

namespace leanet {

class Thread;
class ThreadPool: noncopyable {
public:
	typedef std::function<void ()> Task;

	explicit ThreadPool(const std::string& name = std::string("ThreadPool"));
	~ThreadPool();

	void setMaxQueueSize(int maxSize)
	{ maxQueueSize_ = maxSize; }
	void setThreadInitCallback(const Task& cb)
	{ threadInitCallback_ = cb; }

	void start(int numThreads);
	void stop();

	const std::string& name() const
	{ return name_; }

	size_t queueSize() const;

	void run(const Task& task);

private:
	bool isFull() const;
	void runInThread();
	Task take();

	mutable Mutex mutex_;
	Condition notEmpty_;
	Condition notFull_;

	std::string name_;
	Task threadInitCallback_;
	std::vector<std::shared_ptr<Thread>> threads_;
	std::deque<Task> queue_;
	size_t maxQueueSize_;
	bool running_;
};

}

#endif
