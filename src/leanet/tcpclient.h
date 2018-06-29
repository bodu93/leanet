#ifndef LEANET_TCPCLIENT_H
#define LEANET_TCPCLIENT_H

#include "noncopyable.h"
#include "callbacks.h"
#include "mutex.h"
#include "tcpconnection.h"

namespace leanet {

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class EventLoop;
class TcpClient: noncopyable {
public:
  explicit TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const std::string& name);
	~TcpClient();

	void connect();
	void disconnect();
	void stop();

	TcpConnectionPtr connection() const {
		MutexLock lock(mutex_);
		return connection_;
	}

	EventLoop* getLoop() const { return loop_; }
	bool retry() const { return retry_; }
	void enableRetry() { retry_ = true; }

	const std::string& name() const { return name_; }

	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

	void setWriteCompleteCallback(const WriteCompleteCallback& cb)
	{ writeCompleteCallback_ = cb; }

private:
	// callback registered in Connector::newConnectionCallback_
	void newConnection(int sockfd);
	void removeConnection(const TcpConnectionPtr& conn);

	EventLoop* loop_;
	ConnectorPtr connector_;
	const std::string name_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	WriteCompleteCallback writeCompleteCallback_;
	bool retry_;
	bool connected_;
	int nextConnId_;
	mutable Mutex mutex_;
	TcpConnectionPtr connection_;
};

}

#endif
