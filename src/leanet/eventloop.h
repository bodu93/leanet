#ifndef LEANET_EVENTLOOP_H
#define LEANET_EVENTLOOP_H

#include <vector>
#include <memory> // std::unique_ptr

#include <leanet/noncopyable.h>
#include <leanet/currentthread.h>

namespace leanet {

class Channel;
class Poller;

class EventLoop: noncopyable {
public:
	EventLoop();
	~EventLoop();

	void loop();

	void updateChannel(Channel* channel);

	void assertInLoopThread() {
		if (!isInLoopThread()) {
			abortNotInLoopThread();
		}
	}

	bool isInLoopThread() const {
		return threadId_ == currentThread::tid();
	}

	static EventLoop* getEventLoopOfCurrentThread();

private:
	static const int kPollTimeMs = 1000;

	void abortNotInLoopThread();

	typedef std::vector<Channel*> ChannelList;

	bool looping_; // atomic
	bool quit_; // atomic
	const uint64_t threadId_;

	std::unique_ptr<Poller> poller_;
	// filled by poller in busy loop.
	ChannelList activeChannels_;
};

} // namespace leanet

#endif
