#ifndef LEANET_MUTEX_H
#define LEANET_MUTEX_H

#include <pthread.h>
#include "noncopyable.h"

namespace leanet {

class Condition;
class Mutex: noncopyable {
public:
	Mutex() {
		pthread_mutex_init(&mutex_, NULL);
	}

	~Mutex() {
		pthread_mutex_destroy(&mutex_);
	}

	void lock() {
		pthread_mutex_lock(&mutex_);
	}

	void unlock() {
		pthread_mutex_unlock(&mutex_);
	}

private:
	friend class Condition;
	pthread_mutex_t* getMutex() {
		return &mutex_;
	}

	pthread_mutex_t mutex_;
};

class MutexLock: noncopyable {
public:
	explicit MutexLock(Mutex& mutex)
		: mutex_(mutex) {
		mutex_.lock();
	}

	~MutexLock() {
		mutex_.unlock();
	}

private:
	Mutex& mutex_;
};

}

#endif
