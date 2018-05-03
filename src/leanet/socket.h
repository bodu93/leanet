#ifndef LEANET_SOCKET_H
#define LEANET_SOCKET_H

#include "noncopyable.h"

// on macOS, struct tcp_connection_info is in <netinet/tcp.h>
// struct tcp_connection_info;

// on Linux, struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace leanet {

class InetAddress;

//
// An RAII class of socket file descriptor
//
class Socket: noncopyable {
public:
	explicit Socket(int sockfd)
		: sockfd_(sockfd)
	{ }

	~Socket();

	int fd() const { return sockfd_; }

	// macOS
	// bool getTcpInfo(struct tcp_connection_info*) const;

	bool getTcpInfo(struct tcp_info* info) const;
	bool getTcpInfoString(char* buf, int len) const;

	void bindAddress(const InetAddress& localaddr);
	void listen();

	int accept(InetAddress* peeraddr);

	void shutdownWrite();

	// TCP_NODELAY
	void setTcpNoDelay(bool on);
	// SO_REUSEADDR
	void setReuseAddr(bool on);
	// SO_REUSEPORT
	void setReusePort(bool on);
	// SO_KEEPALIVE
	void setKeepAlive(bool on);

private:
	int sockfd_;
};

}

#endif
