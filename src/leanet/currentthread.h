#ifndef LEANET_CURRENTTHREAD_H
#define LEANET_CURRENTTHREAD_H

#include <stdint.h> // uint64_t

namespace leanet {

namespace currentThread {

extern __thread uint64_t t_cachedTid;
extern __thread const char* t_threadName;

void cacheTid();

inline uint64_t tid() {
	if (t_cachedTid == 0) {
		cacheTid();
	}

	return t_cachedTid;
}

inline const char* name() {
	return t_threadName;
}

bool isMainThread();

} // namespace leanet::currentThread

} // namespace leanet

#endif
