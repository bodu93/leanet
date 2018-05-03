#include "tcpserver.h"
#include "inetaddress.h"
#include "eventloop.h"
#include "eventloopthread.h"
#include "eventloopthreadpool.h"
#include "acceptor.h"
#include "tcpconnection.h"
#include "logger.h"

#include <stdio.h> // snprintf

using namespace leanet;

TcpServer::TcpServer(
		EventLoop* loop,
		const InetAddress& listenAddr,
		const std::string& name)
	: loop_(loop),
		name_(name),
		acceptor_(new Acceptor(loop, listenAddr)),
		started_(false),
		nextConnId_(0)
{
	acceptor_->setNewConnectionCallback(
			std::bind(&TcpServer::newConnection,
								this,
								std::placeholders::_1,
								std::placeholders::_2));
}

TcpServer::~TcpServer() {
	// TODO
}

void TcpServer::start() {
	assert(!acceptor_->listenning());
	// bind and listen in eventloop...
	loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_));
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
	loop_->assertInLoopThread();
	char buf[64];
	snprintf(buf, sizeof(buf), "#%d", nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;
	LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from " << peerAddr.ipPort();

	// getsockaddr
	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	// single-thread tcpserver
	// TcpConnectionPtr conn = std::make_shared<TcpConnection>(
	// 		loop_, sockfd, connName, localAddr, peerAddr);

	// multi-thread tcpserver
	EventLoop* ioLoop = threadPool_->getNextLoop();
	TcpConnectionPtr conn = std::make_shared<TcpConnection>(
			ioLoop, sockfd, connName, localAddr, peerAddr);
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
	conn->connectEstablished();
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn) {
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) {
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnection [" << name_ << "] - connection " << conn->name();
	size_t n = connections_.erase(conn->name());
	assert(n == 1);
	Unused(n);

	EventLoop* ioLoop = conn->getLoop();
	// do remove in eventloop thread, but why queueInLoop??
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}
