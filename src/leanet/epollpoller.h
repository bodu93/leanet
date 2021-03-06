#ifndef LEANET_EPOLLER_H
#define LEANET_EPOLLER_H

#include <vector>
#include <map>
#include "noncopyable.h"

struct epoll_event;

namespace leanet {

class EventLoop;
class Channel;
class Timestamp;

class EPollPoller: noncopyable {
public:
	typedef std::vector<Channel*> ChannelList;

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
	typedef std::map<int, Channel*> ChannelMap;

	EventLoop* loop_;
	int epollfd_;
	ChannelMap channels_;
	EventList events_;
};

}

#endif
