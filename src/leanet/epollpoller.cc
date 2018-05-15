#include "epollpoller.h"
#include "types.h"
#include "timestamp.h"
#include "logger.h"
#include "channel.h"
#include "eventloop.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

using namespace leanet;

namespace {
const int kNew = -1; // Channel::index_ are initialized with -1
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
	: epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
		events_(kInitEventListSize)
{
	if (epollfd_ < 0) {
		LOG_SYSFATAL << "EPollPoller::EPollPoller";
	}
}

EPollPoller::~EPollPoller() {
	::close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) {
	LOG_TRACE << "fd total count " << channels_.size();
	int numEvents = ::epoll_wait(epollfd_,
															 &*events_.begin(),
															 static_cast<int>(events_.size()),
															 timeoutMs);
	int savedErrno = errno;
	Timestamp now(Timestamp::now());
	if (numEvents > 0) {
		LOG_TRACE << numEvents << " events happened";
		fillActiveChannels(numEvents, activeChannels);
		if (implicit_cast<size_t>(numEvents) == events_.size()) {
			events_.resize(events_.size() * 2);
		}
	} else if (numEvents == 0) {
		LOG_TRACE << "nothing happened";
	} else {
		if (savedErrno != EINTR) {
			errno = savedErrno;
			LOG_SYSERR << "EPollPoller::poll()";
		}
	}

	return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const {
	assert(implicit_cast<size_t>(numEvents) <= events_.size());
	for (int i = 0; i < numEvents; ++i) {
		Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
		channel->setReceivedEvents(events_[i].events);
		activeChannels->push_back(channel);
	}
}

// add, del, mod
void EPollPoller::updateChannel(Channel* channel) {
	loop_->assertInLoopThread();
	const int tag = channel->index();
	if (tag == kNew || tag == kDeleted) {
		int fd = channel->fd();
		if (tag == kNew) {
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		} else {
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd] == channel);
		}

		channel->setIndex(kAdded);
		update(EPOLL_CTL_ADD, channel);
	} else {
		if (channel->isNoneEvent()) {
			update(EPOLL_CTL_DEL, channel);
			channel->setIndex(kDeleted);
		} else {
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

void EPollPoller::removeChannel(Channel* channel) {
	loop_->assertInLoopThread();

	int fd = channel->fd();
	size_t n = channels_.erase(fd);
	Unused(n);
	assert(n == 1);

	int idx = channel->index();
	if (idx == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->setIndex(kNew);
}

void EPollPoller::update(int op, Channel* channel) {
	struct epoll_event event;
	::bzero(&event, sizeof(event));
	event.events = channel->interestedEvents();
	event.data.ptr = channel;
	int fd = channel->fd();

	if (::epoll_ctl(epollfd_, op, fd, &event) < 0) {
		// TODO
	} else {
		// TODO
	}
}
