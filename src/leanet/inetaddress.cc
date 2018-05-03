#include "inetaddress.h"

#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>

#include "sockets.h"
#include "logger.h"

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

using namespace leanet;

InetAddress::InetAddress(uint16_t port, bool loopbackOnly, bool ipv6) {
	if (ipv6) {
		::bzero(&addr6_, sizeof(addr6_));
		addr6_.sin6_family = AF_INET6;
		in6_addr ipaddr = loopbackOnly ? in6addr_loopback : in6addr_any;
		addr6_.sin6_addr = ipaddr;
		addr6_.sin6_port = sockets::hostToNet16(port);
	} else {
		::bzero(&addr_, sizeof(addr_));
		addr_.sin_family = AF_INET;
		in_addr_t ipaddr = loopbackOnly ? kInaddrLoopback : kInaddrAny;
		addr_.sin_addr.s_addr = sockets::hostToNet32(ipaddr);
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

static __thread char t_resolveBuffer[64 * 2014];
// bool InetAddress::resolve(
// 		StringArg hostname,
// 		StringArg servicename,
// 		InetAddress* result) {
// 	assert(result);
//
// 	struct addrinfo hint;
// 	::bzero(&hint, sizeof(hint));
// 	hint.ai_flags = AI_CANONNAME;
// 	hint.ai_family = AF_INET;
//
// 	struct addrinfo* ailist = NULL;
// 	int ret = ::getaddrinfo(
// 			hostname.c_str(),
// 			servicename.c_str(),
// 			&hint,
// 			&ailist);
// 	if (ret == 0 && ailist != NULL) {
// 		for (struct addrinfo* aip = ailist;
// 				 aip != NULL;
// 				 aip = aip->ai_next) {
// 			if (aip->ai_family == AF_INET) {
// 				result->addr_ = *(sockets::sockaddr_in_cast(aip->ai_addr));
// 				break;
// 			}
// 		}
// 		freeaddrinfo(ailist);
// 	} else {
// 		if (ret) {
// 			LOG_SYSERR << "InetAddress::resolve";
// 		}
// 		return false;
// 	}
//
// 	return true;
// }

bool InetAddress::resolve(StringArg hostname, InetAddress* result) {
	assert(result);
	Unused(hostname);
	Unused(result);

	struct hostent hent;
	::bzero(&hent, sizeof(hent));
	struct hostent* he = NULL;
	int herrno = 0;
	int ret = gethostbyname_r(hostname.c_str(), &hent,
			t_resolveBuffer, sizeof(t_resolveBuffer), &he, &herrno);
	if (ret == 0 && he != NULL) {
		assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
		result->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
		return true;
	} else {
		if (ret) {
			LOG_SYSERR << "InetAddress::resolve";
		}
		return false;
	}
	return false;
}
