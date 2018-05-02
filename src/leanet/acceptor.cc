#include "acceptor.h"

#include "sockets.h"
#include "inetaddress.h"
#include "eventloop.h"

using namespace leanet;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
	: loop_(loop),
		acceptSocket_(sockets::createNonblockingOrDie(AF_INET)),
		acceptChannel_(loop, acceptSocket_.fd()),
		listenning(false)
{
	acceptSocket_.setReuseAddr(true);
	acceptSocket_.bindAddress(listenAddr);

	acceptChannel_.setReadCallback(&Acceptor::handleRead, this);
}

void Acceptor::listen() {
	loop_->assertInLoopThread();
	listenning_ = true;
	acceptSocket_.listen();
	acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
	loop_->assertInLoopThread();
	InetAddress peeraddr;
	int connfd = acceptSocket_.accept(&peeraddr);
	if (connfd >= 0) {
		if (newConnectionCallback_) {
			newConnectionCallback_(connfd, peeraddr);
		} else {
			sockets::close(connfd);
		}
	}
}
