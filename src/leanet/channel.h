#ifndef LEANET_CHANNEL_H
#define LEANET_CHANNEL_H

#include <leanet/noncopyable.h>
#include <functional>

namespace leanet {

class EventLoop;

//
// A selectable I/O channel.
//
// This class doesn't own the file descriptor
// The file descriptor could be a socket, an eventfd, a timerfd, or
// a signalfd
class Channel: noncopyable {
public:
	typedef std::function<void ()> EventCallback;

	explicit Channel(EventLoop* loop, int fd);

	void handleEvent();
	void setReadCallback(const EventCallback& cb)
	{ readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{ writeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{ errorCallback_ = cb; }

	int fd() const { return fd_; }

	int interestedEvents() const
	{ return interestedEvents_; }

	void setReceivedEvents(int revents)
	{ receivedEvents_ = revents; }

	bool isNoneEvent() const
	{ return interestedEvents_ == kNoneEvent; }

	void enableReading() {
		interestedEvents_ |= kReadEvent;
		update();
	}

	// void enableWriting() {
	// interestedEvents_ |= kWriteEvent;
	// update();
	// }
	//
	// void disableWriting() {
	// interestedEvents_ &= ~kWriteEvent;
	// update();
	// }
	//
	// void disableAll() {
	// interestedEvents_ = kNoneEvent;
	// update();
	// }

	// for poller
	int index() { return index_; }
	void setIndex(int idx) { index_ = idx; }
	EventLoop* ownerLoop() { return loop_; }

private:
	void update();

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;
	const int fd_;

	int interestedEvents_;
	int receivedEvents_;

	int index_; // used by poller

	EventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback errorCallback_;
};

} // namespae leanet

#endif
