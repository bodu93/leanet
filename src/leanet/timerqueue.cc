#include <leanet/timerqueue.h>

#include <assert.h>
#include <sys/timerfd.h>

#include <stdint.h>
#include <algorithm>
#include <vector>
#include <iterator>

#include <leanet/logger.h>
#include <leanet/timer.h>
#include <leanet/channel.h>
#include <leanet/eventloop.h>

namespace detail {

int createTimerfd() {
	int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (timerfd < 0) {
		LOG_SYSERR << "timerfd_create";
	}
	return timerfd;
}

}

using namespace leanet;

TimerQueue::TimerQueue(EventLoop* loop)
	: loop_(loop),
		timerfd_(detail::createTimerfd()),
		timerfdChannel_(loop, timerfd_),
		timers_()
{ }

TimerQueue::~TimerQueue() {
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

	return expired;
}
