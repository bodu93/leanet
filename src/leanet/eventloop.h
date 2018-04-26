#ifndef LEANET_EVENTLOOP_H
#define LEANET_EVENTLOOP_H

#include <vector>
#include <memory> // std::unique_ptr
#include <functional>

#include <leanet/noncopyable.h>
#include <leanet/mutex.h>
#include <leanet/currentthread.h>
#include <leanet/timestamp.h>
#include <leanet/timerid.h>

namespace leanet {

class Channel;
class Poller;

class EventLoop: noncopyable {
public:
	typedef std::function<void()> Functor;

	EventLoop();
	~EventLoop();

	void loop();
	//void quit();

	Timestamp pollReturnedTime() const { return pollReturnedTime_; }


	TimerId runAt(const Timestamp& time, const TimerCallback& cb);
	TimerId runAfter(double delay, const TimerCallback& cb);
	TimerId runEvery(double interval, const TimerCallback& cb);

	void queueInLoop(const Functor& cb);
	void runInLoop(const Functor& cb);

	void assertInLoopThread() {
		if (!isInLoopThread()) {
			abortNotInLoopThread();
		}
	}

	bool isInLoopThread() const {
		return threadId_ == currentThread::tid();
	}

	void wakeup();
	void updateChannel(Channel* channel);
	//void removeChannel(Channel* channel);
	//bool hasChannel(Channel* channel);

	static EventLoop* getEventLoopOfCurrentThread();

private:
	static const int kPollTimeMs = 10000;
	typedef std::vector<Channel*> ChannelList;

	void abortNotInLoopThread();
	void handleRead(); // waked up
	void doPendingFunctors();

	bool looping_; // atomic
	bool quit_; // atomic
	bool callingPendingFunctors_; // atomic
	const uint64_t threadId_;
	Timestamp pollReturnedTime_;

	std::unique_ptr<Poller> poller_;
	// filled by poller in busy loop.
	ChannelList activeChannels_;

	std::unique_ptr<TimerQueue> timerQueue_;

	int wakeupFd_;
	std::unique_ptr<Channel> wakeupChannel_;

	ChannelList activeChannels_;
	mutable Mutex mutex_;
	// @guardedBy mutex_
	std::vector<Functor> pendingFunctors_;
};

} // namespace leanet

#endif
