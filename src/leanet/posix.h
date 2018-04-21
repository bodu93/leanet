#ifndef LEANET_POSIX_H
#define LEANET_POSIX_H

#include <pthread.h>

// a middle tier to capture posix calls used in this library
namespace posix {
	// pthread apis
	int pthread_equal(pthread_t lhs, pthread_t rhs);
	pthread_t pthread_self();
	int pthread_create(
			pthread_t* tidp,
			const pthread_attr_t* attr,
			void* (*start_rtn)(void*),
			void* arg);
	int pthread_join(pthread_t thread, void** rval_ptr);
	int pthread_detach(pthread_t tid);

	int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
	int pthread_mutex_destroy(pthread_mutex_t* mutex);
	int pthread_mutex_lock(pthread_mutex_t* mutex);
	int pthread_mutex_trylock(pthread_mutex_t* mutex);
	int pthread_mutex_unlock(pthread_mutex_t* mutex);

	int pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr);
	int pthread_cond_destroy(pthread_cond_t* cond);
	int pthread_cond_signal(pthread_cond_t* cond);
	int pthread_cond_broadcast(pthread_cond_t* cond);
	int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex);

}

#endif
