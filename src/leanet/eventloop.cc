#include <leanet/eventloop.h>

#include <sys/poll.h>
#include <assert.h>
#include <stdlib.h>

#include <leanet/logger.h>

using namespace leanet;

__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop()
	: looping_(false),
		threadId_(currentThread::tid()) {
	if (t_loopInThisThread) {
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
							<< " exists in this thread " << threadId_;
	} else {
		t_loopInThisThread = this;
	}
}

EventLoop::~EventLoop() {
	assert(!looping_);
	t_loopInThisThread = 0;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
	return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
						<< " was created in threadId_ = " << threadId_
						<< ", current thread id = " << currentThread::tid();
}

void EventLoop::loop() {
	assert(!looping_);
	assertInLoopThread();

	looping_ = true;

	::poll(NULL, 0, 5 * 1000);

	looping_ = false;
}
