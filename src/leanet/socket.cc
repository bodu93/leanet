#include "socket.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <stdio.h>

#include "inetaddress.h"
#include "sockets.h"
#include "logger.h"

using namespace leanet;

Socket::~Socket() {
	sockets::close(sockfd_);
}

bool Socket::getTcpInfo(struct tcp_info* info) const {
	socklen_t len = static_cast<socklen_t>(sizeof(*info));
	::bzero(info, len);
	return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, info, &len) == 0;
}

bool Socket::getTcpInfoString(char* buf, int len) const {
	struct tcp_info info;
	bool ok = getTcpInfo(&info);
	if (ok) {
		snprintf(buf, len, "unrecovered=%u "
				"rto=%u ato=%u snd_mss=%u rcv_mss=%u "
				"lost=%u retrans=%u rtt=%u rttvar=%u "
				"sshthresh=%u cwnd=%u total_retrans=%u",
				info.tcpi_retransmits,
				info.tcpi_rto,
				info.tcpi_ato,
				info.tcpi_snd_mss,
				info.tcpi_rcv_mss,
				info.tcpi_lost,
				info.tcpi_retrans,
				info.tcpi_rtt,
				info.tcpi_rttvar,
				info.tcpi_snd_ssthresh,
				info.tcpi_snd_cwnd,
				info.tcpi_total_retrans);
	}
	return ok;
}

void Socket::bindAddress(const InetAddress& addr) {
	sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen() {
	sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr) {
	struct sockaddr_in6 addr;
	::bzero(&addr, sizeof(addr));
	int connfd = sockets::accept(sockfd_, &addr);
	if (connfd >= 0) {
		peeraddr->setSockAddrInet6(addr);
	}
	return connfd;
}

void Socket::shutdownWrite() {
	sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on) {
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
	if (ret < 0) {
		LOG_SYSERR << "TCP_NODELAY set error";
	}
}

void Socket::setReuseAddr(bool on) {
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if (ret < 0) {
		LOG_SYSERR << "SO_REUSEADDR set error";
	}
}

void Socket::setReusePort(bool on) {
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
	if (ret < 0) {
		LOG_SYSERR << "SO_REUSEPORT set error";
	}
}

void Socket::setKeepAlive(bool on) {
	int optval = on ? 1 : 0;
	int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
	if (ret < 0) {
		LOG_SYSERR << "SO_KEEPALIVE set error";
	}
}
