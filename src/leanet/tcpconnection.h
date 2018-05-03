#ifndef LEANET_TCPCONNECTION_H
#define LEANET_TCPCONNECTION_H

#include "noncopyable.h"
#include "callbacks.h"
#include "inetaddress.h"
#include "buffer.h"

#include <string>
#include <memory> // std::unique_ptr, std::enable_shared_from_this

namespace leanet {

class EventLoop;
class Socket;
class Channel;

class TcpConnection
	: noncopyable,
		public std::enable_shared_from_this<TcpConnection>
{
public:
	explicit TcpConnection(
			EventLoop* loop,
			int sockfd,
			const std::string& name,
			InetAddress localaddr,
			InetAddress peeraddr);
	~TcpConnection();

	void connectEstablished();
	void connectDestroyed();

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }
	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }
	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }
	void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
	{ highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }
	// internal use only
	void setCloseCallback(const CloseCallback& cb)
	{ closeCallback_ = cb; }

	std::string name() const
	{ return name_; }
	EventLoop* getLoop() const
	{ return loop_; }
	InetAddress localAddress() const
	{ return localAddr_; }
	InetAddress peerAddress() const
	{ return peerAddr_; }
	bool connected() const
	{ return state_ == kConnected; }
	bool disconnected() const
	{ return state_ == kDisconnected; }

	void send(const void* data, size_t len);
	void send(const std::string& message);
	// shutdown(SHUT_WR)
	void shutdown();

	void setTcpNoDelay(bool on);
	void setKeepAlive(bool on);

private:
	enum State { kConnecting, kConnected, kDisconnecting, kDisconnected };
	void setState(State s) { state_ = s; }

	void handleRead(Timestamp receiveTime);
	void handleWrite();
	void handleClose();
	void handleError();

	void sendInLoop(const std::string& message);
	void shutdownInLoop();

	EventLoop* loop_;
	std::string name_;
	State state_;

	std::unique_ptr<Socket> socket_;
	std::unique_ptr<Channel> channel_;

	InetAddress localAddr_;
	InetAddress peerAddr_;

	// void (const TcpConnectionPtr&)
	ConnectionCallback connectionCallback_;
	// void (const TcpConnectionPtr&, Buffer*, Timestamp)
	MessageCallback messageCallback_;
	// void (const TcpConnectionPtr&)
	CloseCallback closeCallback_;
	// void (const TcpConnectionPtr&)
	WriteCompleteCallback writeCompleteCallback_;
	// void (const TcpConnectionPtr&, size_t)
	size_t highWaterMark_;
	HighWaterMarkCallback highWaterMarkCallback_;

	Buffer inputBuffer_;
	Buffer outputBuffer_;
};

}

#endif // LEANET_TCPCONNECTION_H
