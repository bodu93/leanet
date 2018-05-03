#include "threadpool.h"
#include "thread.h"
#include <algorithm> // std::for_each
#include <assert.h>

using namespace leanet;

ThreadPool::ThreadPool(const std::string& name)
	: mutex_(),
		notEmpty_(mutex_),
		notFull_(mutex_),
		name_(name),
		maxQueueSize_(0),
		running_(false)
{ }

ThreadPool::~ThreadPool() {
	if (running_) {
		stop();
	}
}

void ThreadPool::start(int numThreads) {
	assert(!threads_.empty());
	running_ = true;

	for (int i = 0; i < numThreads; ++i) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d", i+1);
		std::shared_ptr<leanet::Thread> thd =
			std::make_shared<leanet::Thread>(std::bind(&ThreadPool::runInThread, this), name_ + buf);
		threads_.push_back(thd);
		thd->start();
	}

	if (numThreads == 0 && threadInitCallback_) {
		threadInitCallback_();
	}
}

void ThreadPool::stop() {
	{
	MutexLock lock(mutex_);
	running_ = false;
	notEmpty_.wakeAll();
	}

	std::for_each(threads_.begin(), threads_.end(),
			std::bind(&Thread::join, std::placeholders::_1));
}

// tasks count in task queue
size_t ThreadPool::queueSize() const {
	MutexLock lock(mutex_);
	return queue_.size();
}

void ThreadPool::run(const Task& task) {
	if (threads_.empty()) {
		task();
	} else {
		MutexLock lock(mutex_);
		while (isFull()) {
			notFull_.wait();
		}
		assert(!isFull());

		queue_.push_back(task);
		notEmpty_.wakeAll();
	}
}

ThreadPool::Task ThreadPool::take() {
	MutexLock lock(mutex_);
	while (queue_.empty() && running_) {
		notEmpty_.wait();
	}

	Task task;
	if (!queue_.empty()) {
		task = queue_.front();
		queue_.pop_front();
		if (maxQueueSize_ > 0) {
			notFull_.wakeAll();
		}
	}

	return task;
}

bool ThreadPool::isFull() const {
	// we are in critical section
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread() {
	if (threadInitCallback_) {
		threadInitCallback_();
	}

	while (running_) {
		Task task = take();
		if (task) {
			task();
		}
	}
}
