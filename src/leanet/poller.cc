#include "poller.h"
#include "types.h"
#include "logger.h"
#include "channel.h"
#include "eventloop.h"

#include <poll.h>

using namespace leanet;

Poller::Poller(EventLoop* loop)
	: ownerLoop_(loop)
{ }

Poller::~Poller() { }

Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels) {
	int nready = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
	Timestamp now(Timestamp::now());
	if (nready > 0) {
		assert(activeChannels);
		LOG_TRACE << nready << " events happened";
		fillActiveChannels(nready, activeChannels);
	} else if (nready == 0) {
		LOG_TRACE << "nothing happened";
	} else {
		LOG_SYSERR << "Poller::poll() error";
	}
	return now;
}

void Poller::updateChannel(Channel* channel) {
	assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd()
		<< "interested events = " << channel->interestedEvents();
	if (channel->index() < 0) {
		// assert(channel->index() == -1);
		//
		// a new one, add this channel
		assert(channels_.find(channel->fd()) == channels_.end());
		struct pollfd pfd;
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->interestedEvents());
		pfd.revents = 0;
		int idx = static_cast<int>(pollfds_.size());
		pollfds_.push_back(pfd);
		channel->setIndex(idx);
		channels_[pfd.fd] = channel;
	} else {
		// updating this channel
		assert(channels_.find(channel->fd()) != channels_.end());
		struct pollfd& pfd = pollfds_[channel->index()];
		// invariant: -X - 1  == -(-X - 1)
		assert(pfd.fd == -channel->fd() - 1);
		pfd.fd = channel->fd();
		pfd.events = static_cast<short>(channel->interestedEvents());
		pfd.revents = 0;
		if (channel->isNoneEvent()) {
			// ignore fd
			// invariant: -X - 1  == -(-X - 1)
			pfd.fd = -pfd.fd - 1;
		}
	}
}

void Poller::removeChannel(Channel* channel) {
	assertInLoopThread();
	LOG_TRACE << "fd= " << channel->fd();
	assert(channels_.find(channel->fd()) != channels_.end());
	assert(channels_[channel->fd()] == channel);
	// before removeChannel(): channel->disableAll()
	assert(channel->isNoneEvent());

	int idx = channel->index();
	assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
	const struct pollfd& pfd = pollfds_[idx];
	Unused(pfd);
	assert(pfd.fd == -channel->fd() - 1 &&
			pfd.events == channel->interestedEvents());
	size_t n = channels_.erase(channel->fd());
	assert(n == 1);
	Unused(n);
	if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
		pollfds_.pop_back();
	} else {
		int lastChannelFd = pollfds_.back().fd;
		std::iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
		if (lastChannelFd < 0) {
			lastChannelFd = -lastChannelFd - 1;
		}
		channels_[lastChannelFd]->setIndex(idx);
		pollfds_.pop_back();
	}
}



void Poller::assertInLoopThread() {
	ownerLoop_->assertInLoopThread();
}

void Poller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
	assert(activeChannels);

	activeChannels->clear();
	for (PollFdList::const_iterator it = pollfds_.begin();
			 it != pollfds_.end() && (numEvents > 0);
			 ++it) {
		const struct pollfd& pfd = *it;
		//
		// see Linux's poll(2) man page:
		// On success, a positive number is returned; this is the
		// number of structures which have nonzero revents fields.
		if (pfd.revents > 0) {
			--numEvents;
			ChannelMap::const_iterator iter = channels_.find(pfd.fd);
			if (iter != channels_.end()) {
				Channel* channel = iter->second;
				channel->setReceivedEvents(pfd.revents);
				activeChannels->push_back(channel);
			}
		}
	}
}
