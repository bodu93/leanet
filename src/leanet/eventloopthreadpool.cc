#include "eventloopthreadpool.h"
#include "types.h"
#include "eventloop.h"
#include "thread.h"
#include "threadpool.h"
#include "eventloopthread.h"

#include <assert.h>
#include <stdio.h> // snprintf

using namespace leanet;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
	: baseLoop_(baseLoop),
		name_(name),
		started_(false),
		numThreads_(0),
		next_(0)
{ }

EventLoopThreadPool::~EventLoopThreadPool() {
	// a stack variable, don't delete it
}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
	assert(!started_);
	baseLoop_->assertInLoopThread();

	started_ = true;

	for (int i = 0; i < numThreads_; ++i) {
		char buf[name_.size() + 32];
		snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
		std::shared_ptr<EventLoopThread> loopThread =
			std::make_shared<EventLoopThread>(cb, std::string(buf));
		threads_.push_back(loopThread);
		loops_.push_back(loopThread->startLoop());
	}

	if (numThreads_ == 0 && cb) {
		cb(baseLoop_);
	}
}

EventLoop* EventLoopThreadPool::getNextLoop() {
	baseLoop_->assertInLoopThread();
	assert(started_);

	EventLoop* loop = baseLoop_;

	if (!loops_.empty()) {
		// round-robin
		loop = loops_[next_];
		++next_;
		if (implicit_cast<size_t>(next_) >= loops_.size()) {
			next_ = 0;
		}
	}

	return loop;
}

