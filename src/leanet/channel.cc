#include <leanet/channel.h>

#include <assert.h>
#include <sys/poll.h>

#include <leanet/logger.h>
#include <leanet/eventloop.h>

using namespace leanet;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLERR;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
	: loop_(loop),
		fd_(fd),
		interestedEvents_(kNoneEvent),
		receivedEvents_(kNoneEvent),
		index_(-1)
{ }

void Channel::handleEvent() {
	if (receivedEvents_ & POLLNVAL) {
		LOG_WARN << "Channel::handle_event() POLLNVAL";
	}
	if (receivedEvents_ & (POLLERR | POLLNVAL)) {
		if (errorCallback_) errorCallback_();
	}
	// if (receivedEvents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
	if (receivedEvents_ & (POLLIN | POLLPRI)) {
		if (readCallback_) readCallback_();
	}
	if (receivedEvents_ & POLLOUT) {
		if (writeCallback_) writeCallback_();
	}
}

void Channel::update() {
	assert(loop_);
	loop_->updateChannel(this);
}
