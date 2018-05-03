#ifndef LEANET_CHANNEL_H
#define LEANET_CHANNEL_H

#include "noncopyable.h"
#include "timestamp.h"
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
	typedef std::function<void (Timestamp)> ReadEventCallback;

	explicit Channel(EventLoop* loop, int fd);
	~Channel();

	void handleEvent(Timestamp receiveTime);
	void setReadCallback(const ReadEventCallback& cb)
	{ readCallback_ = cb; }
	void setWriteCallback(const EventCallback& cb)
	{ writeCallback_ = cb; }
	void setCloseCallback(const EventCallback& cb)
	{ closeCallback_ = cb; }
	void setErrorCallback(const EventCallback& cb)
	{ errorCallback_ = cb; }

	int fd() const { return fd_; }

	int interestedEvents() const
	{ return interestedEvents_; }

	void setReceivedEvents(int revents)
	{ receivedEvents_ = revents; }

	// read forever, so no enableReading() there...
	void enableReading() {
		interestedEvents_ |= kReadEvent;
		update();
	}

	void enableWriting() {
		interestedEvents_ |= kWriteEvent;
		update();
	}
	// level-triggered, so we need disableWriting()
	void disableWriting() {
		interestedEvents_ &= ~kWriteEvent;
		update();
	}

	void disableAll() {
		interestedEvents_ = kNoneEvent;
		update();
	}

	bool isNoneEvent() const {
		return interestedEvents_ == kNoneEvent;
	}

	bool isWriting() const {
		return events_ & kWriteEvent;
	}

	void remove();

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

	bool eventHanding_;
	ReadEventCallback readCallback_;
	EventCallback writeCallback_;
	EventCallback closeCallback_;
	EventCallback errorCallback_;
};

} // namespae leanet

#endif
