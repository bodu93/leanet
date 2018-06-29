#include "connector.h"

#include "channel.h"
#include "eventloop.h"
#include "sockets.h"
#include "logger.h"

#include <algorithm> // std::min
#include <errno.h>
#include <assert.h>

using namespace leanet;

Connector::Connector(EventLoop* loop, const InetAddress& servAddr)
	: loop_(loop),
		serverAddr_(servAddr),
		connected_(false),
		state_(kDisconnected),
		retryDelayMs_(kInitRetryDelayMs)
{ }

Connector::~Connector() {
	assert(!channel_);
}

void Connector::start() {
	connected_ = true;
	loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

// must be called in loop thread
void Connector::restart() {
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connected_ = true;
	startInLoop();
}

void Connector::startInLoop() {
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connected_) {
		connect();
	} else {
		// TODO: log_debug??
	}
}

void Connector::stop() {
	connected_ = false;
	loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
	loop_->assertInLoopThread();
	if (state_ == kConnecting) {
		setState(kDisconnected);
		int sockfd = removeAndResetChannel();
		retry(sockfd);
	}
}

void Connector::connect() {
	int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
	int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
	int savedErrno = (ret == 0) ? 0 : errno;
	// state machine programming
	switch (savedErrno) {
		case 0:
		case EINPROGRESS:
		case EINTR:
		case EISCONN:
			connecting(sockfd);
			break;

		case EAGAIN:
		case EADDRINUSE:
		case EADDRNOTAVAIL:
		case ECONNREFUSED: // server send us a RST
		case ENETUNREACH:
		case EHOSTUNREACH:
		case ETIMEDOUT: // on non-blocking socket, should be this error??
			retry(sockfd);
			break;

		case EACCES:
		case EPERM:
		case EAFNOSUPPORT:
		case EALREADY:
		case EBADF:
		case EFAULT:
		case ENOTSOCK:
			LOG_SYSERR << "connect(2) error in Connector::startInLoop " << savedErrno;
			sockets::close(sockfd);
			break;

		default:
			LOG_SYSERR <<"Unexcepted error in Connector::startInLoop " << savedErrno;
			sockets::close(sockfd);
			break;
	}
}

// EINPROGRESS
void Connector::connecting(int sockfd) {
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->setWriteCallback(
			std::bind(&Connector::handleWrite, this));
	channel_->setErrorCallback(
			std::bind(&Connector::handleError, this));

	channel_->enableWriting();
}

int Connector::removeAndResetChannel() {
  // remove channel_ from Poller::channelList:
  // we don't care about events on it.
	channel_->disableAll();
	channel_->remove();

	int sockfd = channel_->fd();
  // destroy channel_:
	// can't reset channel_ here, because we are inside
	// Channel::handleEvent
	loop_->queueInLoop(std::bind(&Connector::resetChannel, this));
	return sockfd;
}

void Connector::resetChannel() {
	channel_.reset();
}

void Connector::handleWrite() {
	LOG_TRACE << "Connector::handleWrite " << state_;

	if (state_ == kConnecting) {
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		if (err) {
			LOG_WARN << "Connector::handleWrite - SO_ERROR = "
							 << err << " " << strerror_tl(err);
			retry(sockfd);
		} else if (sockets::isSelfConnected(sockfd)) {
			LOG_WARN << "Connector::handleWrite - Self connect";
			retry(sockfd);
		} else {
			setState(kConnected);
			if (connected_) {
				newConnectionCallback_(sockfd);
			} else {
				sockets::close(sockfd);
			}
		}
	} else {
		//
		// scenario: client codes call stop() when we waiting for connect done.
		//
		assert(state_ == kDisconnected);
	}
}

void Connector::handleError() {
	LOG_ERROR << "Connector::handleError state= " << state_;
	if (state_ == kConnecting) {
		int sockfd = removeAndResetChannel();
		int err = sockets::getSocketError(sockfd);
		LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
		retry(sockfd);
	}
}

void Connector::retry(int sockfd) {
	sockets::close(sockfd);
	setState(kDisconnected);
	if (connected_) {
		LOG_INFO << "Connector::retry - Retry connecting to "
						 << serverAddr_.ipPort() << " in "
						 << retryDelayMs_ << " milliseconds. ";
		loop_->runAfter(retryDelayMs_/1000.0,
				std::bind(&Connector::startInLoop, shared_from_this()));
		retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
	} else {
		LOG_DEBUG << "do not connect";
	}
}
