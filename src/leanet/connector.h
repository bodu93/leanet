#ifndef LEANET_CONNECTOR_H
#define LEANET_CONNECTOR_H

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "inetaddress.h"

namespace leanet {

class EventLoop;
class Channel;

class Connector
	: noncopyable,
		public std::enable_shared_from_this<Connector>
{
public:
	typedef std::function<void (int sockfd)> NewConnectionCallback;

	explicit Connector(EventLoop* loop, const InetAddress& servAddr);
	~Connector();

	void setNewConnectionCallback(const NewConnectionCallback& cb)
	{ newConnectionCallback_ = cb; }

	// can be called in any thread
	void start();
	// can be called in any thread
	void stop();
	// must be called in loop thread
	void restart();

private:
	enum State { kDisconnected, kConnecting, kConnected };
	static const int kMaxRetryDelayMs = 30*1000;
	static const int kInitRetryDelayMs = 500;

	void setState(State s)
	{ state_ = s; }

	void startInLoop();
	void stopInLoop();

	void connect();
	void connecting(int sockfd);

	void handleWrite();
	void handleError();

	void retry(int sockfd);
	void removeAndResetChannel();
	void resetChannel();

	EventLoop* loop_;
	InetAddress serverAddr_;
	bool connected_;
	State state_;
	std::unique_ptr<Channel> channel_;
	NewConnectionCallback newConnectionCallback_;
	int retryDelayMs_;
};

}

#endif
