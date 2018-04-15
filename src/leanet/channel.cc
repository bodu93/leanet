#include <leanet/channel.h>

#include <sys/poll.h>
#include <assert.h>

#include <leanet/eventloop.h>

using namespace leanet;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLERR;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
	: loop_(loop),
		fd_(fd),
		events_(kNoneEvent),
		revents_(kNoneEvent),
		index_(-1)
{ }

void Channel::handleEvent() {
	// TODO
	if (events_ & POLLIN) {
		if (readCallback_) readCallback_();
	}
	if (events_ & POLLERR) {
		if (errorCallback_) errorCallback_();
	}
	if (events_ & POLLOUT) {
		if (writeCallback_) writeCallback_();
	}
}

void Channel::update() {
	assert(loop_);
	loop_->updateChannel(this);
}
