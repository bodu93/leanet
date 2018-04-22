#ifndef LEANET_BUFFER_H
#define LEANET_BUFFER_H

#include <assert.h>
#include <string.h>

#include <utility> // std::swap since C++11
#include <algorithm>
#include <vector>

#include <leanet/copyable.h>
#include <leanet/sockets.h>
#include <leanet/stringview.h>

namespace leanet {

//
// two usages:
// 1. as an input buffer
// at host endpoint: read begin with readerIndex to app buffer
// at net endpoint: ssize_t len = ::read(sockfd, writerIndex, writableBytes())
// 									advance writer index to len
// 2. as an output buffer
// at host endpoint: ssize_t len = ::write(sockfd, readerIndex, readableBytes())
// 									 advance reader index to len
// at net endpoint: append data from writerIndex
//

class Buffer: public copyable {
public:
	static const size_t kCheapPrepend = 8;
	static const size_t kInitialSize = 1024; // 1k

	explicit Buffer(size_t initialSize = kInitialSize)
		: buffer_(kCheapPrepend + initialSize),
			readerIndex_(kCheapPrepend),
			writerIndex_(kCheapPrepend)
	{
		assert(readableBytes() == 0);
		assert(writableBytes() == initialSize);
		assert(prependableBytes() == kCheapPrepend);
	}

	// implicit copy-control members are okay

	void swap(Buffer& other) {
		buffer_.swap(other.buffer_);
		std::swap(readerIndex_, other.readerIndex_);
		std::swap(writerIndex_, other.writerIndex_);
	}

	void prepend(const void* data, size_t len) {
		assert(len <= prependableBytes());
		readerIndex_ -= len;
		const char* d = static_cast<const char*>(data);
		std::copy(d, d + len, begin() + readerIndex_());
	}

	size_t prependableBytes() const {
		return readerIndex_;
	}

	char* readBegin() {
		return begin() + readerIndex_;
	}

	const char* readBegin() {
		return begin() + readerIndex_;
	}

	size_t readableBytes() const {
		return writerIndex_ - readerIndex_;
	}

	void readAdvanceAll() {
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	void readAdvance(size_t len) {
		std::size_t gap = std::min(len, readableBytes());
		readerIndex_ += gap;
		if (readerIndex_ == writerIndex_) {
			readAdanceAll();
		}
	}

	char* writeBegin() {
		return begin() + writerIndex_;
	}

	const char* writeBegin() const {
		return begin() + writerIndex_;
	}

	size_t writableBytes() const {
		return buffer_.size() - writerIndex_;
	}

	void writeAdvance(size_t len) {
		size_t gap = std::min(len, writableBytes());
		writerIndex_ += gap;
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		writeAdvance(len);
	}

	// like append but data from fd
	ssize_t readFd(int fd, int* savedErrno);

	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {
			extend(len);
		}
		assert(writableBytes() >= len);
	}

private:
	char* begin() {
		return &*buffer_.begin();
	}

	const char* begin() {
		return &*buffer_.begin();
	}

	void extend(size_t len) {
		/* no more buffer in buffer_ */
		if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
			buffer_.resize(writerIndex_ + len);
		} else { /* just need to move ahead */
			assert(kCheapPrepend < readerIndex_);
			size_t readable = readableBytes();
			/* move ahead */
			std::copy(begin() + readerIndex_,
								begin() + writerIndex_,
								begin() + kCheapPrepend);
			readerIndex_ = kCheapPrepend;
			writerIndex_ = readerIndex_ + readable;
			assert(readable == readableBytes());
		}
	}

	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;

	static const char kCRLF[];
};

#endif // LEANET_BUFFER_H
