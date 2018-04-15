#ifndef LEANET_THREAD_H
#define LEANET_THREAD_H

#include <pthread.h>
#include <assert.h>
#include <string>
#include <functional>

#include <leanet/noncopyable.h>
#include <leanet/atomic.h>
#include <leanet/mutex.h>
#include <leanet/condition.h>
#include <leanet/countdownlatch.h>

namespace leanet {

class Thread: noncopyable {
public:
	typedef std::function<void ()> ThreadFunc;

	explicit Thread(const ThreadFunc& func, const std::string& name = std::string());
	~Thread();

	void start();
	int join();

	bool started() const { return started_; }
	const std::string& name() const { return threadName_; }

	// get threads count that have been created since program started
	static int threadsCreated() { return threadsCreated_.get(); }

private:
	void setDefaultName();

	ThreadFunc threadFunc_;
	std::string threadName_;
	pthread_t pthreadId_;
	bool started_;
	bool joined_;
	CountdownLatch latch_;

	static AtomicInt32 threadsCreated_;
};

}

#endif
