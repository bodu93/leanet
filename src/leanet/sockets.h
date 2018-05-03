#ifndef LEANET_SOCKETS_H
#define LEANET_SOCKETS_H

#include <unistd.h> // ssize_t
#include <stdint.h>
#include <netinet/in.h>

namespace sockets {
// socket apis
//
int createNonblockingOrDie(sa_family_t family);
int connect(int sockfd, const struct sockaddr* addr);
void bindOrDie(int sockfd, const struct sockaddr* addr);
void listenOrDie(int sockfd);
int accept(int sockfd, struct sockaddr_in6* addr);
void shutdownWrite(int sockfd);

void toIpPort(char* buf, size_t size, const struct sockaddr* addr);
void toIp(char* buf, size_t size, const struct sockaddr* addr);

void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
void fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

int getSocketError(int sockfd);

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in6* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
const struct sockaddr_in6* sockaddr_in6_cast(const struct sockaddr* addr);

struct sockaddr_in6 getLocalAddr(int sockfd);
struct sockaddr_in6 getPeerAddr(int sockfd);
bool isSelfConnected(int sockfd);

// posix apis
ssize_t read(int fd, void* data, size_t len);
ssize_t write(int fd, const void* data, size_t len);
ssize_t readv(int fd, const struct iovec* iov, int iovcnt);
void close(int fd);

uint64_t netToHost64(uint64_t n);
uint64_t hostToNet64(uint64_t n);
uint32_t netToHost32(uint32_t n);
uint32_t hostToNet32(uint32_t n);
uint16_t netToHost16(uint16_t n);
uint16_t hostToNet16(uint16_t n);

}

#endif // LEANET_SOCKETS_H
