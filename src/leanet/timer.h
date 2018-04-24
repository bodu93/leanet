#ifndef LEANET_TIMER_H
#define LEANET_TIMER_H

#include <leanet/noncopyable.h>
#include <leanet/atomic.h>
#include <leanet/timestamp.h>
#include <leanet/callbacks.h>

namespace leanet {

class Timer: noncopyable {
public:
	Timer(const TimerCallback& cb, Timestamp when, double interval)
		: callback_(cb),
			expiration_(when),
			interval_(interval_),
			repeat_(interval > 0.0),
			sequence_(numCreated_.incrementAndGet())
	{ }

	void run() const {
		callback_();
	}

	Timestamp expiration() const { return expiration_; }
	bool repeat() const { return repeat_; }
	int64_t sequence() const { return sequence_; }
	static int64_t numCreated() { return numCreated_; }

	void restart(Timestamp now);

private:
	const TimerCallback callback_;
	Timestamp expiration_;
	const double interval_;
	const bool repeat_;
	const int64_t sequence_;

	static AtomicInt64 numCreated_;
};

}

#endif
