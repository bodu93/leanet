#ifndef LEANET_TCPSERVER_H
#define LEANET_TCPSERVER_H

#include <memory> // std::unique_ptr
#include <map>
#include <string>

#include "noncopyable.h"
#include "callbacks.h"

namespace leanet {

class InetAddress;
class EventLoop;
class Acceptor;
class EventLoopThreadPool;

class TcpServer: noncopyable {
public:
	TcpServer(EventLoop* loop,
			const InetAddress& listenAddr,
			const std::string& name);
	~TcpServer();

	void setThreadNum(int numThreads);

	void start();

	// tcp connection UP and DOWN will callback cb
	void setConnectionCallback(const ConnectionCallback& cb)
	{ connectionCallback_ = cb; }

	void setMessageCallback(const MessageCallback& cb)
	{ messageCallback_ = cb; }

private:
	void newConnection(int sockfd, const InetAddress& peerAddr);
	void removeConnection(const TcpConnectionPtr& conn);
	void removeConnectionInLoop(const TcpConnectionPtr& conn);

	// std::string -> TcpConnection
	typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;
	EventLoop* loop_;
	const std::string name_;
	std::unique_ptr<Acceptor> acceptor_;
	std::unique_ptr<EventLoopThreadPool> threadPool_;
	ConnectionCallback connectionCallback_;
	MessageCallback messageCallback_;
	bool started_;
	int nextConnId_; // used to name next connection
	ConnectionMap connections_;
};

}

#endif
