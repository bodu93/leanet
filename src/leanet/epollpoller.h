#ifndef LEANET_EPOLLER_H
#define LEANET_EPOLLER_H

#include <vector>
#include "noncopyable.h"

struct epoll_event;

namespace leanet {

class EPollPoller;

class EPollPoller: noncopyable {
public:
	EPollPoller(EventLoop* loop);
	~EPollPoller();

	Timestamp poll(int timeoutMs, ChannelList* activeChannels);

	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);

private:
	static const int kInitEventListSize = 16;

	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
	void update(int operation, Channel* channel);

	typedef std::vector<struct epoll_event> EventList;

	EventLoop* loop_;
	int epollfd_;
	EventList events_;
};

}

#endif
