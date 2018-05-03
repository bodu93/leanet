#include "timerqueue.h"

#include <assert.h>
#include <sys/timerfd.h>
#include <strings.h>
#include <stdint.h>

#include <algorithm>
#include <iterator>

#include "logger.h"
#include "timer.h"
#include "timerid.h"
#include "channel.h"
#include "eventloop.h"

namespace {

int createTimerfd() {
	int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (timerfd < 0) {
		LOG_SYSERR << "timerfd_create";
	}
	return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when) {
	int64_t microseconds = when.microSecondsFromEpoch() - Timestamp::now().microSecondsFromEpoch();
	if (microseconds < 100) {
		microseconds = 100;
	}
	struct timespec ts;
	ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
	ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
	return ts;
}

void resetTimerfd(int timerfd, double expiration) {
	struct itimerspec newValue;
	struct itimerspec oldValue;
	::bzero(&newValue, sizeof(newValue));
	::bzero(&oldValue, sizeof(oldValue));
	newValue.it_value = howMuchTimeFromNow(expiration);
	// no interval
	int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
	if (ret) {
		LOG_SYSERR << "timerfd_settime()";
	}
}

// called in TimerQueue::handleRead()
void readTimerfd(int timerfd, Timestamp now) {
	uint64_t times;
	ssize_t n ::read(timerfd, &times, sizeof(times));
	LOG_TRACE << "TimerQueue::handleRead() " << times << " at " << now.toString();
	if (n != sizeof(times)) {
		LOG_ERROR << "TimerQueue::handleRead() reads " << n << "(instead of 8) bytes";
	}
}

}

using namespace leanet;

TimerQueue::TimerQueue(EventLoop* loop)
	: loop_(loop),
		timerfd_(::createTimerfd()),
		timerfdChannel_(loop, timerfd_),
		timers_()
{
	timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
	timerfdChannel_.disableAll();
	// timerfdChannel_.remove();
	::close(timerfd_);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now) {
	Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	// first not less than the key.(that is: >=)
	TimerList::iterator it = timers_.lower_bound(sentry);
	assert(it == timers_.end() || now < it->first);

	std::vector<Entry> expired;
	std::copy(timers_.begin(), it, std::back_inserter(expired));
	timers_.erase(timers_.begin(), it);

	for (std::vector<Entry>::iterator it = expired.begin();
			 it != expired.end();
			 ++it) {
		ActiveTimer timer(it->second, it->second->sequence());
		size_t n = activeTimers_.erase(timer);
		assert(n == 1);
		Unused(n);
	}

	return expired;
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, Timestamp when, double interval) {
	Timer* timer = new Timer(cb, when, interval);
	loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));
	return TimerId(timer, timer->sequence());
}

void TimerQueue::cancelTimer(TimerId timerid) {
	loop_->runInLoop(std::bind(&TimerQueue::cancelTimerInLoop, this, timerid));
}

void TimerQueue::addTimerInLoop(Timer* timer) {
	loop_->assertInLoopThread();
	bool earliestChanged = insert(timer);
	if (earliestChanged) {
		::resetTimerfd(timerfd_, timer_->expiration());
	}
}

void TimerQueue::cancelTimerInLoop(TimerId timerid) {
	loop_->assertInLoopThread();
	ActiveTimer timer(timerid.timer_, timerid.sequence());
	ActiveTimerSet::iterator it = activeTimers_.find(timer);
	if (it != activeTimers_.end()) {
		size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
		assert(n == 1);
		Unused(n);
		delete it->first;
		activeTimers_.erase(it);
	} else if (callingExpiredTimers_) {
		// because we in handleRead(), after all timers' callback are executed,
		// then handleRead() will call reset(), so we add canceled timers into
		// cancelingTimers to exclude them in reset()
		cancelingTimers_.insert(timer);
	}
}


void TimerQueue::handleRead() {
	loop_->assertInLoopThread();
	Timestamp now(Timestamp::now());
	::readTimerfd(timerfd_, now);

	std::vector<Entry>& expired = getExpired(now);

	callingExpiredTimers_ = true;
	cancelingTimers_.clear();
	for (std::vector<Entry>::iterator iter = expired.begin();
			 iter != expired.end();
			 ++iter) {
		iter->second()->run();
	}
	callingExpiredTimers_ = false;

	reset(expired, now);
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {
	for (std::vector<Entry>::const_iterator it = expired.begin();
			 it != expired.end();
			 ++it) {
		ActiveTimer timer(it->second, it->second->sequence());
		if (it->second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()) {
			// add a interval
			it->second->restart(now);
			// insert again
			insert(it->second);
		} else {
			delete it->second;
		}
	}

	Timestamp nextExpire;
	// find first timer expiration in timers_
	if (!timers_.empty()) {
		nextExpire = timers_.begin()->second->expiration();
	}

	if (nextExpire.valid()) {
		::resetTimerfd(timerfd_, nextExpire);
	}
}

bool TimerQueue::insert(Timer* timer) {
	loop_->assertInLoopThread();
	bool earliestChanged = false;
	Timestamp when = timer->expiration();
	TimerList::iterator it = timers_.begin();
	// timers_ is empty or less than first timer...
	if (it == timer_.end() || when < it->first) {
		earlistChanged = true;
	}
	{
	std::pair<TimerList::iterator, bool> result
		= timers_.insert(Entry(when, timer));
	Unused(result);
	}
	{
	std::pair<ActiveTimerSet::iterator, bool> result
		= activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
	Unused(result);
	}

	return earliestChanged;
}
