#ifndef LEANET_TIMERID_H
#define LEANET_TIMERID_H

#include "copyable.h"

namespace leanet {

class Timer;
// friend declaration is not forward declaration
class TimerQueue;

class TimerId: public copyable {
public:
	TimerId()
		: timer_(NULL),
			sequence_(0)
	{ }

	TimerId(TimerId* timer, int64_t seq)
		: timer_(timer),
			sequence_(seq)
	{ }

	// implicit copy-control members are okay

	friend class TimerQueue;

private:
	Timer* timer_;
	int64_t sequence_;
};

}

#endif
