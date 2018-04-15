#ifndef LEANET_BLOCKINGQUEUE_H
#define LEANET_BLOCKINGQUEUE_H

#include <leanet/mutex.h>
#include <leanet/condition.h>
#include <assert.h>
#include <deque>

namespace leanet {

template<typename T>
class BlockingQueue {
private:
	// use mutex_ in constant member functions, so we add the 'mutable' keyword
	mutable Mutex mutex_;
	Condition notEmpty_;
	// @guardby mutex_
	std::deque<T> queue_;

public:
	BlockingQueue()
		: mutex_(),
			notEmpty_(mutex_),
			queue_()
	{ }

	void put(const T& x) {
		MutexLock guard(mutex_);
		queue_.push_back(x);
		// once we put a element, we wake up notEmpty_
		// which waited in get()
		notEmpty_.wakeAll();
	}

	T get() const {
		MutexLock guard(mutex_);
		// blocked when queue is empty
		while (queue_.empty()) {
			notEmpty_.wait();
		}
		assert(!queue_.empty());
		const T& x = queue_.front();
		queue_.pop_front();
		return x;
	}

	size_t size() const {
		MutexLock guard(mutex_);
		return queue_.size();
	}
};

} // namespace leanet

#endif
