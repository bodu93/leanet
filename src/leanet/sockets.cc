#include <leanet/sockets.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <strings.h> // bzero
#include <stdio.h> // snprintf

#include <leanet/types.h>
#include <leanet/logger.h>

using namespace leanet;

namespace {

#if VALGRIND || defined (NO_ACCEPT4)
void setNonBlockAndCloseOnExec(int sockfd) {
	// non blocking: non-inheritable of accept(2) under Linux
	int flags = ::fcntl(sockfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	int ret = ::fcntl(sockfd, F_SETFL, flags);
	if (ret < 0) {
		LOG_SYSFATAL << "::fcntl";
	}

	// close on exec: non-inheritable of accept(2) under Linux
	flags = ::fcntl(sockfd, F_GETFD, 0);
	flags |= FD_CLOEXEC;
	ret = ::fcntl(sockfd, F_SETFD, flags);
	if (ret < 0) {
		LOG_SYSFATAL << "::fcntl";
	}
}
#endif

}

namespace sockets {

// socket apis
int createNonblockingOrDie(sa_family_t family) {
#if VALGRIND
	int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (sockfd < 0) {
		LOG_SYSFATAL << "sockets::createNonblockingorDie";
	}
	setNonBlockAndCloseOnExec(sockfd);
#else
	int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (sockfd < 0) {
		LOG_SYSFATAL << "sockets::createNonblockingOrDie";
	}
#endif
	return sockfd;
}

void bindOrDie(int sockfd, const struct sockaddr* addr) {
	int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
	if (ret < 0) {
		LOG_SYSFATAL << "sockets::bindOrDie";
	}
}

void listenOrDie(int sockfd) {
	int ret = ::listen(sockfd, SOMAXCONN);
	if (ret < 0) {
		LOG_SYSFATAL << "sockets::listenOrDie";
	}
}

int accept(int sockfd, struct sockaddr_in6* addr) {
	socklen_t addrlen = static_cast<socklen_t>(sizeof(*addr));
#if VALGRIND || defined (NO_ACCEPT4)
	int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
	setNonBlockAndCloseOnExec(connfd);
#else
	int connfd = ::accept4(sockfd, sockaddr_cast(addr),
												 &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
	if (connfd < 0) {
		int savedErrno = errno;
		LOG_SYSERR << "sockets::accept";
		switch (savedErrno) {
			case EAGAIN:
			case ECONNABORTED:
			case EINTR:
			case EPROTO:
			case EPERM:
			case EMFILE:
				errno = savedErrno;
				break;
			case EBADF:
			case EFAULT:
			case EINVAL:
			case ENFILE:
			case ENOBUFS:
			case ENOMEM:
			case ENOTSOCK:
			case EOPNOTSUPP:
				LOG_FATAL << "unexcepted error of ::accept(or 4) " << savedErrno;
				break;
			default:
				LOG_FATAL << "unknown error of ::accept(or 4) " << savedErrno;
				break;
		}
	}
	return connfd;
}

int connect(int sockfd, const struct sockaddr* addr) {
	return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

void shutdownWrite(int sockfd) {
	if (::shutdown(sockfd, SHUT_WR) < 0) {
		LOG_SYSERR << "sockets::shutdownwrite";
	}
}

//
// IPv4 socket address
// struct in_addr {
// 		in_addr_t  s_addr;
// };
//
// struct sockaddr_in {
// 		uint8_t					sin_len;
// 		sa_family_t			sin_family;
// 		in_port_t				sin_port;
//
// 		struct in_addr	sin_addr;
//
// 		char						sin_zero[8];
// };
//
// IPv6 socket address
// struct in6_addr {
// 		uint_8_t s6_addr[16];
// };
//
// struct sockaddr_in6 {
// 		uint8_t					sin6_len;
// 		sa_family_t			sin6_family;
// 		in_port_t				sin6_port;
//
// 		uint32_t				sin6_flowinfo;
// 		struct in6_addr	sin6_addr;
//
// 		uint32_t				sin6_scope_id;
// };
//
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr) {
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr) {
	return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
	return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
	return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr) {
	return static_cast<const struct sockaddr_in6*>(implicit_cast<const void*>(addr));
}

//
// struct sockaddr {
// 		uint8_t				sa_len;
// 		sa_family_t		sa_family;
// 		char					sa_data[14];
// };
//
void toIp(char* buf, size_t size, const struct sockaddr* addr) {
	if (addr->sa_family == AF_INET) {
		assert(size >= INET_ADDRSTRLEN);
		const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
		::inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
	} else if (addr->sa_family == AF_INET6) {
		assert(size >= INET6_ADDRSTRLEN);
		const struct sockaddr_in6* addr6 = sockaddr_in6_cast(addr);
		::inet_ntop(AF_INET6, &addr6->sin6_addr, buf, static_cast<socklen_t>(size));
	}
}

void toIpPort(char* buf, size_t size, const struct sockaddr* addr) {
	toIp(buf, size, addr);
	size_t end = ::strlen(buf);
	const struct sockaddr_in* addr4 = sockaddr_in_cast(addr);
	uint16_t port = netToHost16(addr4->sin_port);
	assert(size > end);
	snprintf(buf+end, size-end, ":%u", port);
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNet16(port);
	if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
		LOG_SYSERR << "sockets::fromIpPort";
	}
}

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
	addr->sin6_family = AF_INET6;
	addr->sin6_port = hostToNet16(port);
	if (::inet_pton(AF_INET6, ip, &addr->sin6_addr) <= 0) {
		LOG_SYSERR << "sockets::fromIpPort";
	}
}

int getSocketError(int sockfd) {
	int optval;
	socklen_t optlen = static_cast<socklen_t>(sizeof(optval));

	if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &opelen) < 0) {
		return errno;
	} else {
		return optval;
	}
}

struct sockaddr_in6 getLocalAddr(int sockfd) {
	struct sockaddr_in6 localaddr;
	::bzero(&localaddr, sizeof(localaddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
	if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
		LOG_SYSERR << "sockets::getLocalAddr";
	}
	return localaddr;
}

struct sockaddr_in6 getPeerAddr(int sockfd) {
	struct sockaddr_in6 peeraddr;
	::bzero(&peeraddr, sizeof(peeraddr));
	socklen_t addrlen = static_cast<socklen_t>(sizeof(peeraddr));
	if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
		LOG_SYSERR << "sockets::getPeerAddr";
	}
	return peeraddr;
}

bool isSelfConnected(int sockfd) {
	struct sockaddr_in6 localaddr = getLocalAddr(sockfd);
	struct sockaddr_in6 peeraddr = getPeerAddr(sockfd);
	if (localaddr.sin6_family == AF_INET) {
		const struct sockaddr_in* laddr4 = reinterpret_cast<struct sockaddr_in*>(&localaddr);
		const struct sockaddr_in* raddr4 = reinterpret_cast<struct sockaddr_in*>(&peeraddr);
		return laddr4->sin_port == raddr4->sin_port
			&& laddr4->sin_addr.s_addr == raddr4->sin_addr.s_addr;
	} else if (localaddr.sin6_family == AF_INET6) {
		return localaddr.sin6_port == peeraddr.sin6_port
			&& ::memcmp(&localaddr.sin6_addr, &peeraddr.sin6_addr, sizeof(localaddr.sin6_addr)) == 0;
	} else {
		return false;
	}
}

// posix apis
ssize_t read(int sockfd, void* buf, size_t len) {
	return ::read(sockfd, buf, len);
}

ssize_t write(int sockfd, const void* buf, size_t len) {
	return ::write(sockfd, buf, len);
}

ssize_t readv(int sockfd, const struct iovec* iov, int iovcnt) {
	return ::readv(sockfd, iov, iovcnt);
}

void close(int sockfd) {
	if (::close(sockfd) < 0) {
		LOG_SYSERR << "sockets::close";
	}
}

uint64_t netToHost64(uint64_t n) {
	// TODO
	return 0;
}

uint64_t hostToNet64(uint64_t n) {
	// TODO
	return 0;
}

uint32_t netToHost32(uint32_t n) {
	return ::ntohl(n);
}

uint32_t hostToNet32(uint32_t n) {
	return ::htonl(n);
}

uint16_t netToHost16(uint16_t n) {
	return ::ntohs(n);
}

uint16_t hostToNet16(uint16_t n) {
	return ::htons(n);
}

}
