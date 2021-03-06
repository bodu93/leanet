#include "eventloopthread.h"
#include "thread.h"
#include "eventloop.h"

using namespace leanet;

EventLoopThread::EventLoopThread(
		const ThreadInitCallback& cb,
		const std::string& name)
	: loop_(NULL),
		exiting_(false),
		thread_(std::bind(&EventLoopThread::threadFunc, this), name),
		mutex_(),
		cond_(mutex_),
		callback_(cb)
{ }

EventLoopThread::~EventLoopThread() {
	exiting_ = true;
	if (loop_ != NULL) {
		loop_->quit();
		thread_.join();
	}
}

EventLoop* EventLoopThread::startLoop() {
	assert(!thread_.started());
	thread_.start();

	{
	MutexLock lock(mutex_);
	while (loop_ == NULL) {
		cond_.wait();
	}
	}

	return loop_;
}

void EventLoopThread::threadFunc() {
	EventLoop loop;

	if (callback_) {
		callback_(&loop);
	}

	{
	MutexLock lock(mutex_);
	loop_ = &loop;
	cond_.wakeAll();
	}

	loop.loop();
	loop_ = NULL;
}
