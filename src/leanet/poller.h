#ifndef LEANET_POLLER_H
#define LEANET_POLLER_H

#include <vector>
#include <map>

#include "noncopyable.h"
#include "timestamp.h"

struct pollfd;

namespace leanet {

class Channel;
class EventLoop;

//
// IO Multiplexing with poll(2).
//

class Poller : noncopyable {
public:
	typedef std::vector<Channel*> ChannelList;

	explicit Poller(EventLoop* loop);
	~Poller();

	Timestamp poll(int timeoutMs, ChannelList* activeChannels);

	void updateChannel(Channel* channel);
	void removeChannel(Channel* channel);

	void assertInLoopThread();

private:
	void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

	typedef std::vector<struct pollfd> PollFdList;
	typedef std::map<int, Channel*> ChannelMap;

	EventLoop* ownerLoop_;
	PollFdList pollfds_;
	ChannelMap channels_;
};

}

#endif
