#include "posix.h"

namespace posix {

// pthread apis
//
int pthread_equal(pthread_t lhs, pthread_t rhs) {
	return ::pthread_equal(lhs, rhs);
}

pthread_t pthread_self() {
	return ::pthread_self();
}

int pthread_create(
		pthread_t* tidp,
		const pthread_attr_t* attr,
		void* (*start_rtn)(void*),
		void* arg) {
	return ::pthread_create(tidp, attr, start_rtn, arg);
}

void pthread_exit(void* rval_ptr) {
	::pthread_exit(rval_ptr);
}

int pthread_join(pthread_t thread, void** rval_ptr) {
	return ::pthread_join(thread, rval_ptr);
}

int pthread_detach(pthread_t tid) {
	return ::pthread_detach(tid);
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
	return ::pthread_mutex_init(mutex, attr);
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {
	return ::pthread_mutex_destroy(mutex);
}

int pthread_mutex_lock(pthread_mutex_t* mutex) {
	return ::pthread_mutex_lock(mutex);
}

int pthread_mutex_trylock(pthread_mutex_t* mutex) {
	return ::pthread_mutex_trylock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t* mutex) {
	return ::pthread_mutex_unlock(mutex);
}

int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr) {
	return ::pthread_cond_init(cond, attr);
}

int pthread_cond_destroy(pthread_cond_t* cond) {
	return ::pthread_cond_destroy(cond);
}

int pthread_cond_signal(pthread_cond_t* cond) {
	return ::pthread_cond_signal(cond);
}

int pthread_cond_broadcast(pthread_cond_t* cond) {
	return ::pthread_cond_broadcast(cond);
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
	return ::pthread_cond_wait(cond, mutex);
}

} // namespace posix
