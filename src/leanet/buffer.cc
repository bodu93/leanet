#include "buffer.h"

#include <errno.h>
#include <sys/uio.h> // readv

namespace leanet {

const char Buffer::kCRLF[] = "\r\n";

ssize_t Buffer::readFd(int fd, int* savedErrno) {
	char extrabuf[65536];
	struct iovec iov[2];
	const size_t writable = writableBytes();
	iov[0].iov_base = beginWrite();
	iov[0].iov_len = writable;
	iov[1].iov_base = extrabuf;
	iov[1].iov_len = sizeof(extrabuf);

	const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
	const ssize_t n = sockets::readv(fd, iov, iovcnt);
	if (n < 0) {
		*savedErrno = errno;
	} else if (static_cast<size_t>(n) <= writable) {
		hasWritten(n);
	} else {
		writerIndex_ = buffer_.size();
		append(extrabuf, n - writable);
	}

	return n;
}

}
