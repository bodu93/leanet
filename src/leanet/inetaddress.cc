#include <leanet/inetaddress.h>

#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>

#include <leanet/sockets.h>
#include <leanet/logger.h>

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

using namespace leanet;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) {
	if (ipv6) {
		::bzero(&addr6_, sizeof(addr6_));
		addr6_.sin6_family = AF_INET6;
		in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ip;
		addr6_.sin6_port = sockets::hostToNet16(port);
	} else {
		::bzero(&addr_, sizeof(addr_));
		addr_.sin_faimly = AF_INET;
		in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = sockets::hostToNet32(ip);
		addr_.sin_port = sockets::hostToNet16(port);
	}
}

InetAddress::InetAddress(StringArg ip, uint16_t port, bool ipv6) {
	if (ipv6) {
		::bzero(&addr6_, sizeof(addr6_));
		sockets::fromIpPort(ip.c_str(), port, &addr6_);
	} else {
		::bzero(&addr_, sizeof(addr_));
		sockets::fromIpPort(ip.c_str(), port, &addr_);
	}
}

string InetAddress::ip() const {
	char buf[64] = "";
	sockets::toIp(buf, sizeof(buf), getSockAddr());
	return buf;
}

uint16_t InetAddress::port() const {
	assert(family() == AF_INET);
	return sockets::netToHost16(portNetOrder());
}

string InetAddress::ipPort() const {
	char buf[64] = "";
	sockets::toIpPort(buf, sizeof(buf), getSockAddr());
	return buf;
}

uint32_t InetAddress::ipNetOrder() const {
	assert(family() == AF_INET);
	return addr_.sin_addr.s_addr;
}

static __thread char resolveBuffer[64 * 2014];
bool InetAddress::resolve(StringArg hostname, InetAddress* result) {
	assert(result);
	return false;
}
