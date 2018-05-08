#include "tcpconnection.h"

#include "types.h"
#include "callbacks.h"
#include "eventloop.h"
#include "channel.h"
#include "socket.h"
#include "sockets.h"
#include "logger.h"

#include <errno.h>
#include <assert.h>

using namespace leanet;

// prototype from "types.h"
void defaultConnectionCallback(const TcpConnectionPtr& conn) {
	LOG_TRACE << conn->localAddress().ipPort() << " -> "
						<< conn->peerAddress().ipPort() << " is "
						<< (conn->connected() ? "UP" : "DOWN");
}

// prototype from "types.h"
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
	assert(state_ == kDisconnected);
}

void TcpConnection::connectEstablished() {
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);
	// register in event loop
	channel_->enableReading();
	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
	loop_->assertInLoopThread();
	assert(state_ == kConnected || state_ == kDisconnecting);
	setState(kDisconnected);
	channel_->disableAll();
	connectionCallback_(shared_from_this());
	loop_->removeChannel(channel_.get());
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
					loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
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
	setState(kDisconnected);
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
	if (state_ == kConnected) {
		if (loop_->isInLoopThread()) {
			sendInLoop(message, len);
		} else {
			loop_->runInLoop(
					std::bind(&TcpConnection::sendInLoop, this, message, len));
		}
	}
}

void TcpConnection::send(const std::string& message) {
	send(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len) {
	loop_->assertInLoopThread();

	ssize_t nwrote = 0;
	size_t remaining = len;
	bool faultError = false;

	// iff no thing in output queue, try writing directly
	if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
		nwrote = ::write(channel_->fd(), data, len);
		if (nwrote >= 0) {
			remaining = len - nwrote;
			if (remaining == 0 && writeCompleteCallback_) {
				loop_->queueInLoop(std::bind(
							writeCompleteCallback_, shared_from_this()));
			}
		} else {
			nwrote = 0;
			if (errno == EWOULDBLOCK) {
				LOG_SYSERR << "TcpConnection::sendInLoop";
				if (errno == EPIPE || errno == ECONNRESET) {
					faultError = true;
				}
			}
		}
	}

	// else we append data to output queue
	assert(remaining <= len);
	if (!faultError && remaining > 0) {
		size_t oldLen = outputBuffer_.readableBytes();
		if (oldLen + remaining >= highWaterMark_
				&& oldLen < highWaterMark_
				&& highWaterMark_) {
			loop_->queueInLoop(std::bind(
						highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
		}
		outputBuffer_.append(static_cast<const char*>(data) + nwrote, remaining);
		if (!channel_->isWriting()) {
			// iff we have written partial data, we interested on writable event
			channel_->enableWriting();
		}
	}
}

void TcpConnection::shutdown() {
	if (state_ == kConnected) {
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
