#ifndef LEANET_CONDITION_H
#define LEANET_CONDITION_H

#include <pthread.h>
#include <leanet/noncopyable.h>
#include <leanet/mutex.h>

namespace leanet {

class Condition: noncopyable {
public:
	explicit Condition(Mutex& mutex)
		: mutex_(mutex) {
		pthread_cond_init(&cond_, NULL);
	}

	~Condition() {
		pthread_cond_destroy(&cond_);
	}

	void wait() {
		pthread_cond_wait(&cond_, mutex_.getMutex());
	}

	void wakeOne() {
		pthread_cond_signal(&cond_);
	}

	void wakeAll() {
		pthread_cond_broadcast(&cond_);
	}

private:
	Mutex& mutex_;
	pthread_cond_t cond_;
};

}

#endif
