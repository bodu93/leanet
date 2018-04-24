#include <leanet/eventloop.h>

#include <sys/poll.h>
#include <assert.h>
#include <stdlib.h>

#include <leanet/logger.h>
#include <leanet/channel.h>
#include <leanet/poller.h>

using namespace leanet;

__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop()
	: looping_(false),
		quit_(false),
		threadId_(currentThread::tid()),
		poller_(new Poller(this)),
		activeChannels_() {
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

void EventLoop::loop() {
	assert(!looping_);
	assertInLoopThread();

	looping_ = true;
	quit_ = false;

	while (!quit_) {
		activeChannels_.clear();
		poller_->poll(kPollTimeMs, &activeChannels_);
		for (ChannelList::iterator iter = activeChannels_.begin();
				 iter != activeChannels_.end();
				 ++iter) {
			(*iter)->handleEvent();
		}
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::updateChannel(Channel* channel) {
	assertInLoopThread();
	poller_->updateChannel(channel);
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
	return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
	LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
						<< " was created in threadId_ = " << threadId_
						<< ", current thread id = " << currentThread::tid();
}

