#ifndef LEANET_ATOMIC_H
#define LEANET_ATOMIC_H

#include <leanet/noncopyable.h>
#include <stdint.h>

namespace leanet {

namespace detail {

// we can find atomic integers infos at:
// http://www.alexonlinux.com/multithreaded-simple-data-type-access-and-atomic-variables
// https://gcc.gnu.org/onlinedocs/gcc-4.1.2/gcc/Atomic-Builtins.html
// https://en.wikipedia.org/wiki/Barrier_(computer_science)
template<typename T>
class AtomicInteger: noncopyable {
public:
	AtomicInteger()
		: value_(0)
	{ }

	// uncomment if you need copying and assignment
	//
	// AtomicInteger(const AtomicInteger& other)
	// 	: value_(other.value_)
	// { }

	// AtomicInteger& operator=(const AtomicInteger& other) {
	// 	getAndSet(other.get());
	// 	return *this;
	// }

	// *this += x
	void add(T x) {
		getAndAdd(x);
	}

	// *this += 1
	void increment() {
		incrementAndGet();
	}

	// *this -= 1
	void decrement() {
		decrementAndGet();
	}

	T addAndGet(T x) {
		return getAndAdd(x) + x;
	}

	T incrementAndGet() {
		return addAndGet(1);
	}

	T decrementAndGet() {
		return addAndGet(-1);
	}

	T get() {
		return __sync_val_compare_and_swap(&value_, 0, 0);
	}

	T getAndAdd(T x) {
		return __sync_fetch_and_add(&value_, x);
	}

	T getAndSet(T newValue) {
		return __sync_lock_test_and_set(&value_, newValue);
	}

private:
	volatile T value_;
};

} // namespace leanet::detail

typedef detail::AtomicInteger<int32_t> AtomicInt32;
typedef detail::AtomicInteger<int64_t> AtomicInt64;

} // namespace leanet

#endif // LEANET_ATOMIC_H
