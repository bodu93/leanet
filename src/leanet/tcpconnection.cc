#include "tcpconnection.h"

#include "types.h"
#include "callbacks.h"
#include "eventloop.h"
#include "channel.h"
#include "socket.h"
#include "sockets.h"
#include "logger.h"

#include <assert.h>

using namespace leanet;

void defaultConnectionCallback(const TcpConnectionPtr& conn) {
	LOG_TRACE << conn->localAddress().ipPort() << " -> "
						<< conn->peerAddress().ipPort() << " is "
						<< (conn->connected() ? "UP" : "DOWN");
}

void defaultMessageCallback(const TcpConnectionPtr&,
														Buffer* buffer,
														Timestamp) {
	// discard received message
	buffer->retrieveAll();
}

TcpConnection::TcpConnection(
		EventLoop* loop,
		int sockfd,
		const std::string& name,
		InetAddress localaddr,
		InetAddress peeraddr)
	: loop_(loop),
		name_(name),
		state_(kConnecting),
		socket_(new Socket(sockfd)),
		channel_(new Channel(loop, sockfd)),
		localAddr_(localaddr),
		peerAddr_(peeraddr),
		highWaterMark_(64*1024*1024)
{
	socket_->bindAddress(localAddr_);
	// DON'T USE shared_from_this in constructor!
	channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
	channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
	channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
	channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection() {
	assert(state_ == kDisConnected);
}

void TcpConnection::connectEstablished() {
	loop_->assertInLoopThread();
	assert(state == kConnecting);
	setState(kConnected);
	// register in event loop
	channel_->enableReading();
	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
	loop_->assertInLoopThread();
	assert(state_ == kConnected || state_ == kDisconnecting);
	setState(kDisConnected);
	channel_->disableAll();
	connectionCallback_(shared_from_this());
	loop_->removeChannel(channel_->get());
}

void TcpConnection::handleRead(Timestamp receiveTime) {
	int savedErrno = 0;
	ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
	if (n > 0) {
		// actually, messageCallback_ is registered by TcpServer or TcpClient,
		// so it is always not null??
		messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	} else if (n == 0) {
		handleClose();
	} else { // n < 0
		errno = savedErrno;
		LOG_SYSERR << "TcpConnection::handleRead";
		handleError();
	}
}

void TcpConnection::handleWrite() {
	loop_->assertInLoopThread();
	if (channel_->isWriting()) {
		ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
		if (n > 0) {
			outputBuffer_.retrieve(n);
			if (outputBuffer_.readableBytes() == 0) {
				// no more data to be wrote, so we disable writing for disabling a
				// busy loop
				channel_->disableWriting();
				if (writeCompleteCallback_) {
					// drain all readable bytes...
					loop_->queueInLoop(writeCompleteCallback_, shared_from_this());
				}
				if (state_ == kDisconnecting) {
					shutdownInLoop();
				}
			} else {
				// need to write again
			}
		} else {
			LOG_SYSERR << "TcpConnection::handleWrite";
		}
	} else {
		LOG_TRACE << "Connection is down, no more writing";
	}
}

void TcpConnection::handleClose() {
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpConnection::handleClose() state= " << state_;
	assert(state_ == kConnected || state_ == kDisconnecting);
	setState(kDisConnected);
	channel_->disableAll();
	if (closeCallback_) {
		closeCallback_(shared_from_this());
	}
}

void TcpConnection::handleError() {
	//
	// getsockopt(2) will clear pending error on socket.
	// if peer endpoint send RST, call handleError first and then call
	// handleRead in Channel::handleEvent, read(2) will return 0 at that time.
	//
	int err = sockets::getSocketError(channel_->fd());
	LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR= " << err << " " << strerror_tl(err);
}

void TcpConnection::send(const void* message, size_t len) {
	send(std::string(static_cast<const char*>(message), len));
}

void TcpConnection::send(const std::string& message) {
	if (state_ == kConnected) {
		if (loop_->isInLoopThread()) {
			sendInLoop(message);
		} else {
			loop_->runInLoop(std::bind(&TcpConnection::sendInLoop, this, message));
		}
	}
}

void TcpConnection::sendInLoop(const std::string& message) {
	loop_->assertInLoopThread();
	ssize_t nwrote = 0;
	// if channel_->isWriting() == true,
	// indicates that output-buffer has data,
	// so we just append data into output-buffer
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
		nwrote = ::write(channel_->fd(), message.data(), message.size());
		if (nwrote >= 0) {
			// write(2) partial message to socket fd
			if (implicit_cast<size_t>(nwrote) < message.size()) {
				// TODO
			} else if (writeCompleteCallback_) { // nwrote == message.size
				// drain whole message...
				loop_->queueInLoop(writeCompleteCallback_, shared_from_this());
			}
		} else {
			nwrote = 0;
			if (errno != EWOULDBLOCK) {
				LOG_SYSERR << "TcpConnection::sendInLoop";
			}
		}
	}

	assert(nwrote >= 0);
	if (implicit_cast<size_t>(nwrote) < message.size()) {
		outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
		// if (highWaterMarkCallback_) {
		// 	loop_->queueInLoop(std::bind(
		// 				highWaterMarkCallback_,
		// 				shared_from_this(),
		// 				std::placeholders::_1));
		// }
		if (!channel_->isWriting()) {
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown() {
	if (state == kConnected) {
		setState(kDisconnecting);
		loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
	}
}

void TcpConnection::shutdownInLoop() {
	loop_->assertInLoopThread();
	if (!channel_->isWriting()) {
		// not writing or ignoring...
		// ::shutdown(sockfd, SHUT_WR)
		socket_->shutdownWrite();
	}
}

void TcpConnection::setTcpNoDelay(bool on) {
	socket_->setTcpNoDelay(on);
}

void TcpConnection::setKeepAlive(bool on) {
	socket_->setKeepAlive(on);
}
