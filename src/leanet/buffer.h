#ifndef LEANET_BUFFER_H
#define LEANET_BUFFER_H

#include <assert.h>
#include <string.h>

#include <utility> // std::swap since C++11
#include <algorithm>
#include <vector>

#include <leanet/types.h>
#include <leanet/copyable.h>
#include <leanet/sockets.h>
#include <leanet/stringview.h>

namespace leanet {

//
// two usages:
// 1. as an input buffer
// at host endpoint: read begin with readerIndex to app buffer
// at net endpoint: ssize_t len = ::read(sockfd, beginWrite(), writableBytes())
// 									hasWritten(len)
// 2. as an output buffer
// at host endpoint: ssize_t len = ::write(sockfd, peek(), readableBytes())
// 									 retrieve(len)
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

	//
	// prepend series functions:
	// append before reader index
	//
	size_t prependableBytes() const {
		return readerIndex_;
	}

	void prepend(const void* data, size_t len) {
		assert(len <= prependableBytes());
		readerIndex_ -= len;
		const char* d = static_cast<const char*>(data);
		std::copy(d, d + len, begin() + readerIndex_);
	}

	void prependInt64(int64_t x) {
		// just bytes, so we use uint64...
		int64_t be64 = sockets::hostToNet64(x);
		prepend(&be64, sizeof(be64));
	}

	void prependInt32(int32_t x) {
		int32_t be32 = sockets::hostToNet32(x);
		prepend(&be32, sizeof(be32));
	}

	void prependInt16(int16_t x) {
		int16_t be16 = sockets::hostToNet16(x);
		prepend(&be16, sizeof(be16));
	}

	void prependInt8(int8_t x) {
		prepend(&x, sizeof(x));
	}

	//
	// read series functions
	//
	size_t readableBytes() const {
		return writerIndex_ - readerIndex_;
	}

	const char* peek() const {
		return begin() + readerIndex_;
	}

	int64_t peekInt64() const {
		assert(readableBytes() >= sizeof(int64_t));
		int64_t i64 = 0;
		::memcpy(&i64, peek(), sizeof(i64));
		return sockets::netToHost64(i64);
	}

	int32_t peekInt32() const {
		assert(readableBytes() >= sizeof(int32_t));
		int32_t i32 = 0;
		::memcpy(&i32, peek(), sizeof(i32));
		return sockets::netToHost32(i32);
	}

	int16_t peekInt16() const {
		assert(readableBytes() >= sizeof(int16_t));
		int16_t i16 = 0;
		::memcpy(&i16, peek(), sizeof(i16));
		return sockets::netToHost16(i16);
	}

	int8_t peekInt8() const {
		assert(readableBytes() >= sizeof(int8_t));
		int8_t x = *peek();
		return x;
	}

	void retrieveAll() {
		readerIndex_ = kCheapPrepend;
		writerIndex_ = kCheapPrepend;
	}

	void retrieve(size_t len) {
		assert(len <= readableBytes());
		if (len < readableBytes()) {
			readerIndex_ += len;
		} else {
			retrieveAll();
		}
	}

	void retrieveInt64() {
		retrieve(sizeof(int64_t));
	}

	void retrieveInt32() {
		retrieve(sizeof(int32_t));
	}

	void retrieveInt16() {
		retrieve(sizeof(int16_t));
	}

	void retrieveInt8() {
		retrieve(sizeof(int8_t));
	}

	int64_t readInt64() {
		int64_t i64 = peekInt64();
		retrieveInt64();
		return i64;
	}

	int32_t readInt32() {
		int32_t i32 = peekInt32();
		retrieveInt32();
		return i32;
	}

	int16_t readInt16() {
		int16_t i16 = peekInt16();
		retrieveInt16();
		return i16;
	}

	int8_t readInt8() {
		int8_t i8 = peekInt8();
		retrieveInt8();
		return i8;
	}

	void retrieveUntil(const char* end) {
		assert(peek() <= end);
		assert(end <= beginWrite());
		retrieve(end - peek());
	}

	string retrieveAllAsString() {
		return retrieveAsString(readableBytes());
	}

	string retrieveAsString(size_t len) {
		assert(len <= readableBytes());
		string ret(peek(), len);
		retrieve(len);
		return ret;
	}

	StringView toStringView() const {
		return StringView(peek(), readableBytes());
	}

	//
	// write series functions
	//
	size_t writableBytes() const {
		return buffer_.size() - writerIndex_;
	}

	char* beginWrite() {
		return begin() + writerIndex_;
	}

	const char* beginWrite() const {
		return begin() + writerIndex_;
	}

	void hasWritten(size_t len) {
		assert(len <= writableBytes());
		writerIndex_ += len;
	}

	void unwrite(size_t len) {
		assert(len <= readableBytes());
		writerIndex_ -= len;
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);
		std::copy(data, data + len, beginWrite());
		hasWritten(len);
	}

	void append(const void* data, size_t len) {
		append(static_cast<const char*>(data), len);
	}

	void append(const StringView& view) {
		append(view.data(), view.size());
	}

	void append(const string& str) {
		append(str.data(), str.size());
	}

	void appendInt64(int64_t x) {
		int64_t be64 = sockets::hostToNet64(x);
		append(&be64, sizeof(be64));
	}

	void appendInt32(int32_t x) {
		int32_t be32 = sockets::hostToNet32(x);
		append(&be32, sizeof(be32));
	}

	void appendInt16(int16_t x) {
		int16_t be16 = sockets::hostToNet16(x);
		append(&be16, sizeof(be16));
	}

	void appendInt8(int8_t x) {
		append(&x, sizeof(x));
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

	const char* begin() const {
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

}

#endif // LEANET_BUFFER_H
