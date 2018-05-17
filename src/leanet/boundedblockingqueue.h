#ifndef LEANET_BOUNDEDBLOCKINGQUEUE_H
#define LEANET_BOUNDEDBLOCKINGQUEUE_H

#include "mutex.h"
#include "condition.h"
#include <assert.h>
#include <deque>

namespace leanet {

template<typename T>
class BoundedBlockingQueue {
public:
	static const size_t DEFAULTCAPACITY = 8;
	explicit BoundedBlockingQueue(size_t max_cap = DEFAULTCAPACITY)
		: mutex_(),
			notEmpty_(mutex_),
			notFull_(mutex_),
			maxCapacity_(max_cap)
	{ }

	void put(const T& x) {
		MutexLock guard(mutex_);
		while (queue_.size() >= maxCapacity_) {
			notFull_.wait();
		}
		assert(queue_.size() < maxCapacity_);
		queue_.push_back(x);
		notEmpty_.wakeAll();
	}

	T get() {
		MutexLock guard(mutex_);
		while (queue_.empty()) {
			notEmpty_.wait();
		}
		assert(queue_.size() > 0);
		const T& x = queue_.front();
		queue_.pop_front();
		notFull_.wakeAll();
		return x;
	}

	size_t size() const {
		MutexLock guard(mutex_);
		return queue_.size();
	}
private:
	mutable Mutex mutex_;
	Condition notEmpty_;
	Condition notFull_;
	size_t maxCapacity_;
	// @guardby mutex_
	std::deque<T> queue_;
};

} // namespace leanet

#endif
