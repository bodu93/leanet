#ifndef LEANET_COUNTDOWNLATCH_H
#define LEANET_COUNTDOWNLATCH_H

#include <assert.h>
#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"

namespace leanet {

class CountdownLatch: noncopyable {
public:
	explicit CountdownLatch(int count)
		: count_(count),
			mutex_(),
			cond_(mutex_)
	{ }

	void wait() {
		MutexLock guard(mutex_);
		while (count_ > 0) {
			cond_.wait();
		}
		assert(count_ == 0);
	}

	void countDown() {
		MutexLock guard(mutex_);
		--count_;
		if (count_ == 0) {
			cond_.wakeAll();
		}
	}

	int getCount() const {
		MutexLock guard(mutex_);
		return count_;
	}

private:
	int count_;
	// use mutable for constant member method(lock and unlock are non-const)
	mutable Mutex mutex_;
	Condition cond_;
};

} // namespace leanet

#endif
