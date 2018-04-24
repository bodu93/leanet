#ifndef LEANET_TIMERQUEUE_H
#define LEANET_TIMERQUEUE_H

#include <utility>
#include <set>

#include <leanet/noncopyable.h>
#include <leanet/timestamp.h>
#include <leanet/timerid.h>

namespace leanet {

class EventLoop;

class TimerQueue: noncopyable {
public:
	TimerQueue(EventLoop* loop);
	~TimerQueue();

	TimerId addTimer(const TimerCallback& cb, Timestamp when, double interval);
	//void cancel(TimerId timerid);

private:
	// FIXME: use unique_ptr<Timer> instead of raw pointers.
	typedef std::pair<Timestamp, Timer*> Entry;
	typedef std::set<Entry> TimerList;

	void handleRead();

	std::vector<Entry> getExpired(Timestamp now);
	void reset(const std::vector<Entry>& expired, Timestamp now);
	bool insert(Timer* timer);

	EventLoop* loop_;
	const int timerfd_; // Linux timerfd since 2.6.25
	Channel timerfdChannel_;
	// Timer list sorted by expiration
	TimerList timers_;
};

}

#endif
