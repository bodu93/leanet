#include <leanet/timer.h>

using namespace leanet;

AtomicInt64 Timer::numCreated_;

void Timer::restart(Timestamp now) {
	if (repeat_) {
		expiration_ = addTime(now, interval_);
	} else {
		expiration_ = Timestamp::invalid();
	}
}
