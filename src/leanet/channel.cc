#include "channel.h"

#include <assert.h>
#include <poll.h>

#include "logger.h"
#include "eventloop.h"

using namespace leanet;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLERR;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
	: loop_(loop),
		fd_(fd),
		interestedEvents_(kNoneEvent),
		receivedEvents_(kNoneEvent),
		index_(-1),
		eventHanding_(false)
{ }

Channel::~Channel() {
	assert(!eventHanding_);
}

void Channel::handleEvent(Timestamp receiveTime) {
	eventHanding_ = true;

	if (receivedEvents_ & POLLNVAL) {
		LOG_WARN << "Channel::handleEvent() POLLNVAL";
	}
	if ((receivedEvents_ & POLLHUP) && !(receivedEvents_ & POLLIN)) {
		LOG_WARN << "Channel::handleEvent() POLLHUP";
		if (closeCallback_) closeCallback_();
	}
	if (receivedEvents_ & (POLLERR | POLLNVAL)) {
		if (errorCallback_) errorCallback_();
	}
	// on macOS, there is no POLLRDHUP,
	// and on Linux, POLLRDHUP set the macro _GNU_SOURCE
	// if (receivedEvents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
	if (receivedEvents_ & (POLLIN | POLLPRI)) {
		if (readCallback_) readCallback_(receiveTime);
	}
	if (receivedEvents_ & POLLOUT) {
		if (writeCallback_) writeCallback_();
	}

	eventHanding_ = false;
}

void Channel::update() {
	assert(loop_);
	loop_->updateChannel(this);
}
