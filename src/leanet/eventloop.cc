#include "eventloop.h"

#include <unistd.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <stdlib.h>

#include "logger.h"
#include "sockets.h"
#include "channel.h"
#include "poller.h"
#include "timer.h"
#include "timerqueue.h"

using namespace leanet;

namespace {

__thread EventLoop* t_loopInThisThread = 0;

int createEventfd() {
	int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (evfd < 0) {
		LOG_SYSERR << "Failed in eventfd";
		abort();
	}
	return evfd;
}

class IgnoreSigpipe {
public:
	IgnoreSigpipe() {
		::signal(SIGPIPE, SIG_IGN);
	}
};

// anonymous namespace variable: ignore pipe signal at beginning
IgnoreSigpipe ignPipe;

}

EventLoop::EventLoop()
	: looping_(false),
		quit_(false),
		callingPendingFunctors_(false),
		threadId_(currentThread::tid()),
		poller_(new Poller(this)),
		activeChannels_(),
		timerQueue_(new TimerQueue(this)),
		wakeupFd_(createEventfd()),
		wakupChannel_(new Channel(this, wakeupFd_)),
		pendingFunctors_()
{
	if (t_loopInThisThread) {
		LOG_FATAL << "Another EventLoop " << t_loopInThisThread
							<< " exists in this thread " << threadId_;
	} else {
		t_loopInThisThread = this;
	}

	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
	assert(!looping_);
	t_loopInThisThread = 0;

	wakeupChannel_->disableAll();
	::close(wakeupFd_);
}

void EventLoop::loop() {
	assert(!looping_);
	assertInLoopThread();

	looping_ = true;
	quit_ = false;

	while (!quit_) {
		//
		// if there no timerfd* apis, we could handle timer callbacks
		// at there(or handle it using another separate timer thread)
		//
		activeChannels_.clear();
		pollReturnedTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
		for (ChannelList::iterator iter = activeChannels_.begin();
				 iter != activeChannels_.end();
				 ++iter) {
			// pollReturnedTime_ is the time of message arival
			(*iter)->handleEvent(pollReturnedTime_);
		}
		doPendingFunctors();
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit() {
	// there is a time window:
	// quit() can be called from other thread(not io thread).
	quit_ = true;
	if (!isInLoopThread()) {
		wakeup();
	}
}

void EventLoop::queueInLoop(const Functor& cb) {
	{
		MutexLock lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	if (!isInLoopThread() || callingPendingFunctors_) {
		wakeup();
	}
}

void EventLoop::runInLoop(const Functor& cb) {
	if (isInLoopThread()) {
		cb();
	} else {
		queueInLoop(cb);
	}
}

void EventLoop::doPendingFunctors() {
	std::vector<Functor> functors;
	callingPendingFunctors_ = true;

	{
	// 1. keep critical section as short as possible:
	// 		swapped with thread-private stack variable!
	// 2. forbid dead lock: functor() could call queueInLoop again...
	MutexLock lock(mutex_);
	functors.swap(pendingFunctors_);
	}

	for (size_t i = 0; i < functors.size(); ++i) {
		functors[i]();
	}
	callingPendingFunctors_ = false;
}

void EventLoop::updateChannel(Channel* channel) {
	assertInLoopThread();
	poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
	assertInLoopThread();
	poller_->removeChannel(channel);
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb) {
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb) {
	Timestamp time(addTime(Timestamp::now(), delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb) {
	Timestamp time(addTime(Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
}

// wakeup poll() call in loop() for calling doPendingFunctors()
void EventLoop::wakeup() {
	uint64_t one = 1;
	ssize_t n = sockets::write(wakeupFd_, &one, sizeof(one));
	if (n != sizeof(one)) {
		LOG_ERROR << "EventLoop::write() write " << n << "(instead of 8) bytes";
	}
}

void EventLoop::handleRead() {
	uint64_t data = 0;
	ssize_t n = sockets::read(wakeupFd_, &data, sizeof(data));
	if (n != sizeof(data)) {
		LOG_ERROR << "EventLoop::handleRead() reads " << n "(instead of 8) bytes";
	}
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
	return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
						<< " was created in threadId_ = " << threadId_
						<< ", current thread id = " << currentThread::tid();
}
