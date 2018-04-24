#ifndef LEANET_INETADDRESS_H
#define LEANET_INETADDRESS_H

#include <netinet/in.h>

#include <leanet/copyable.h>
#include <leanet/stringview.h>

//
// namespace member forward declarations
//
namespace sockets {
	const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
}

namespace leanet {

class InetAddress: public copyable {
public:
	explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);
	InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

	explicit InetAddress(const struct sockaddr_in& addr)
		: addr_(addr)
	{ }
	explicit InetAddress(const struct sockaddr_in6& addr6)
		: addr6_(addr6)
	{ }

	// ipv4
	sa_family_t family() const { return addr_.sin_family; }
	string ip() const;
	uint16_t port() const;
	string ipPort() const;

	const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
	void setSockAddrInet6(const struct sockaddr_in6& addr) { addr6_ = addr; }

	uint32_t ipNetOrder() const;
	// ipv4
	uint16_t portNetOrder() const { return addr_.sin_port; }

	// static bool resolve(StringArg hostname, StringArg servicename,
	// 										InetAddress* result);
	static bool resolve(StringArg hostname, InetAddress* result);

private:
	union {
		struct sockaddr_in addr_;
		struct sockaddr_in6 addr6_;
	};
};

}

#endif
