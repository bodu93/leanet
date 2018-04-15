#include <leanet/thread.h>
#include <unistd.h>
#include <assert.h>
#include <leanet/currentthread.h>

namespace leanet {

namespace currentThread {

__thread uint64_t t_cachedTid = 0;
__thread const char* t_threadName = "unknown";

} // namespace leanet::currentThread

namespace detail {

uint64_t gettid() {
	// under linux
	// return static_cast<pid_t>(::syscall(SYS_gettid));

	// under macosx
	pthread_t tid = pthread_self();
	size_t minsz = std::min(sizeof(tid), sizeof(uint64_t));
	uint64_t id = 0;
	memcpy(&id, &tid, minsz);
	return id;
}

struct ThreadData {
	typedef leanet::Thread::ThreadFunc ThreadFunc;

	ThreadFunc func;
	std::string name;
	CountdownLatch* platch;

	explicit ThreadData(const ThreadFunc& func, const std::string& name, CountdownLatch* latch)
		: func(func),
			name(name),
			platch(latch)
	{ }

	void startThread() {
		// cache thread id, after that we can use isMainThread()
		leanet::currentThread::tid();
		leanet::currentThread::t_threadName = name.empty() ? "leanetThread" : name.c_str();

		platch->countDown();
		platch = NULL;
		func();
		leanet::currentThread::t_threadName = "finished";
	}
};

void* thread_routine(void* thd_arg) {
	ThreadData* data = static_cast<ThreadData*>(thd_arg);
	assert(data);
	data->startThread();
	delete data;
	return (void*)0;
}

} // namespace leanet::detail

namespace currentThread {

void cacheTid() {
	if (t_cachedTid == 0) {
		t_cachedTid = detail::gettid();
	}
}

bool isMainThread() {
	// linux: return tid() == getpid();

	// macosx
	uint64_t id = tid();
	pthread_t pid = pthread_self();
	size_t minsz = std::min(sizeof(id), sizeof(pthread_t));
	return memcmp(&id, &pid, minsz) == 0;
}

} // namespace leanet::currentThread

AtomicInt32 Thread::threadsCreated_;
Thread::Thread(const ThreadFunc& func, const std::string& name)
	: threadFunc_(func),
		threadName_(name),
		started_(false),
		joined_(false),
		latch_(1)
	{
		setDefaultName();
	}

Thread::~Thread() {
	if (started_ && !joined_) {
		// let thread terminated naturally
		pthread_detach(pthreadId_);
	}
}

void Thread::setDefaultName() {
	int count = threadsCreated_.incrementAndGet();
	if (threadName_.empty()) {
		char buf[32];
		snprintf(buf, sizeof(buf), "Thread%d", num);
		threadName_ = buf;
	}
}

void Thread::start() {
	assert(!started_);
	started_ = true;

	detail::ThreadData* data = new detail::ThreadData(threadFunc_, threadName_, &latch_);
	if (pthread_create(&pthreadId_, NULL, &detail::thread_routine, data)) {
		started_ = false;
		delete data;
	} else {
		latch_.wait();
	}
}

int Thread::join() {
	joined_ = true;
	return pthread_join(pthreadId_, NULL);
}

} // namespace leanet
