#include "tcpclient.h"
#include "logger.h"
#include "sockets.h"
#include "connector.h"
#include "eventloop.h"

#include <assert.h>
#include <stdio.h>

namespace leanet {

namespace detail {

void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
	loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const leanet::ConnectorPtr&) {
	// TODO
}

}

}

using namespace leanet;

TcpClient::TcpClient(EventLoop* loop,
		const InetAddress& serverAddr,
		const std::string& name)
	: loop_(loop),
		connector_(new Connector(loop, serverAddr)),
		name_(name),
		connectionCallback_(defaultConnectionCallback),
		messageCallback_(defaultMessageCallback),
		retry_(false),
		connected_(true),
		nextConnId_(1)
{
	connector_->setNewConnectionCallback(
			std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
	LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector " << connector_.get();
}

TcpClient::~TcpClient() {
	LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector " << connector_.get();

	TcpConnectionPtr conn;
	bool unique = false;
	{
	MutexLock lock(mutex_);
	unique = connection_.unique();
	conn = connection_;
	}

	if (conn) {
		assert(loop_ == conn->getLoop());
		CloseCallback cb = std::bind(
				detail::removeConnection, loop_, std::placeholders::_1);
		loop_->runInLoop(
				std::bind(&TcpConnection::setCloseCallback, conn, cb));
		if (unique) {
			// TODO
			// conn->forceClose();
		}
	} else {
		connector_->stop();
		loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
	}
}

void TcpClient::connect() {
	LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
					 << connector_->serverAddress().ipPort();

	connected_ = true;
	connector_->start();
}

void TcpClient::disconnect() {
	connected_ = false;

	{
	MutexLock lock(mutex_);
	if (connection_) {
		connection_->shutdown();
	}
	}
}

void TcpClient::stop() {
	connected_ = false;
	connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
	loop_->assertInLoopThread();
	InetAddress peerAddr(sockets::getPeerAddr(sockfd));
	char buf[32];
	snprintf(buf, sizeof(buf), ":%s#%d", peerAddr.ipPort().c_str(), nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	TcpConnectionPtr conn = std::make_shared<TcpConnection>(
			loop_,
			sockfd,
			connName,
			localAddr,
			peerAddr);
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(
			std::bind(&TcpClient::removeConnection, this, std::placeholders::_1));
	{
	MutexLock lock(mutex_);
	connection_ = conn;
	}

	conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
	MutexLock lock(mutex_);
	assert(connection_ == conn);
	connection_.reset();
	}

	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connected_) {
		LOG_INFO << "TcpClient::connect[" << name_
						 << "] - Reconnecting to "
						 << connector_->serverAddress().ipPort();
		connector_->restart();
	}
}
