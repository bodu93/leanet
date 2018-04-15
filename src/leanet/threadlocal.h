#ifndef LEANET_THREADLOCAL_H
#define LEANET_THREADLOCAL_H

#include <pthread.h>
#include <leanet/noncopyable.h>

namespace leanet {

template<typename T>
class ThreadLocal: noncopyable {
public:
	ThreadLocal() {
		pthread_key_create(&key_, &ThreadLocal::destructor);
	}

	~ThreadLocal() {
		pthread_key_delete(&key_);
	}

	T& value() {
		T* perThreadValue = static_cast<T*>(pthread_getspecific(key_));
		if (!perThreadValue) {
			perThreadValue = new T();
			pthread_setspecific(key_, perThreadValue);
		}
		assert(perThreadValue);

		return *perThreadValue;
	}

private:
	static void destructor(void* arg) {
		T* data = static_cast<T*>(arg);
		delete data;
	}

	pthread_key_t key_;
};

}

#endif
