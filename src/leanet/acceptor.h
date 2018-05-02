#ifndef LEANET_ACCEPTOR_H
#define LEANET_ACCEPTOR_H

#include <functional>

#include "noncopyable.h"
#include "channel.h"
#include "socket.h"
#include "callbacks.h"

namespace leanet {

class EventLoop;
class InetAddress;

class Acceptor: noncopyable {
public:
	typedef std::function<void(int sockfd, const InetAddress&)> NewConnectionCallback;

	Acceptor(EventLoop* loop, const InetAddress& listenAddr);

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	bool listenning() const { return listenning_; }

	void listen();

private:
	void handleRead();

	EventLoop* loop_;
	Socket acceptSocket_;
	Channel acceptChannel_;
	NewConnectionCallback newConnectionCallback_;
	bool listenning_;
};

}

#endif
