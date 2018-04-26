#ifndef LEANET_TIMERQUEUE_H
#define LEANET_TIMERQUEUE_H

#include <utility>
#include <vector>
#include <set>

#include <leanet/callbacks.h>
#include <leanet/noncopyable.h>
#include <leanet/timestamp.h>

namespace leanet {

class EventLoop;
class Timer;
class TimerId;

class TimerQueue: noncopyable {
public:
	TimerQueue(EventLoop* loop);
	~TimerQueue();

	TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
	void cancelTimer(TimerId timerid);

private:
	// FIXME: use unique_ptr<Timer> instead of raw pointers.
	typedef std::pair<Timestamp, Timer*> Entry;
	typedef std::set<Entry> TimerList;
	typedef std::pair<Timer*, int64_t> ActiveTimer;
	typedef std::set<ActiveTimer> ActiveTimerSet;

	void addTimerInLoop(Timer* timer);
	void cancelTimerInLoop(TimerId timerid);

	// called when timerfd alarms
	void handleRead();

	// move out all expired timers
	std::vector<Entry> getExpired(Timestamp now);
	// readd timers which have repeat_ property
	void reset(const std::vector<Entry>& expired, Timestamp now);
	bool insert(Timer* timer);

	EventLoop* loop_;
	const int timerfd_; // Linux timerfd since 2.6.25
	Channel timerfdChannel_;
	// Timer list sorted by expiration
	TimerList timers_;

	// for cancel()
	ActiveTimerSet activeTimer_;
	bool callingExpiredTimers_; // atomic
	ActiveTimerSet cancelingTimers_;
};

}

#endif
