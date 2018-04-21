#ifndef LEANET_CIRCULAR_BUFFER_H
#define LEANET_CIRCULAR_BUFFER_H

#include <assert.h>
#include <string>
#include <leanet/stringview.h>
#include <algorithm> // std::copy

namespace leanet {

// a circular buffer
class Buffer {
public:
	static const size_t kInitialSize = 1024;

	explicit Buffer(size_t initialSize = kInitialSize)
		: size_(initialSize + 1),
			buffer_(new char[size_]),
			writerIndex_(0),
			readerIndex_(0)
	{
		assert(readableBytes() == 0);
		assert(writableBytes() == (size_ - 1));
	}

	Buffer(const Buffer& other)
		: size_(other.size_),
			buffer_(new char[other.size_]),
			writerIndex_(other.writerIndex_),
			readerIndex_(other.readerIndex_)
	{
		if (other.needRewind()) {
			std::copy(other.beginRead(),
								other.bufferEnd(),
								beginRead());
			std::copy(other.bufferBegin(),
								other.beginWrite(),
								bufferBegin());
		} else {
			std::copy(other.beginRead(),
								other.beginWrite(),
								beginRead());
		}
	}

	Buffer& operator=(const Buffer& other) {
		Buffer tmp(other);
		tmp.swap(*this);
		return *this;
	}

	~Buffer() {
		delete[] buffer_;
	}

	void swap(Buffer& other) {
		std::swap(size_, other.size_);
		std::swap(buffer_, other.buffer_);
		std::swap(writerIndex_, other.writerIndex_);
		std::swap(readerIndex_, other.readerIndex_);
	}

	size_t readableBytes() const {
		int diff = writerIndex_ - readerIndex_;
		if (diff < 0) {
			diff += size_;
		}
		assert(diff >= 0);
		return diff;
	}

	size_t writableBytes() const {
		return size_ - 1 - readableBytes();
	}

	const char* beginRead() const {
		return buffer_ + readerIndex_;
	}

	const char* beginWrite() const {
		return buffer_ + writerIndex_;
	}

	void append(const char* data, size_t len) {
		ensureWritableBytes(len);

		size_t last = writerIndex_ + len;
		if (last <= size_) {
			std::copy(data,
								data + len,
								beginWrite());
		} else {
			std::copy(data,
								data + size_ - writerIndex_,
								beginWrite());
			std::copy(data + size_ - writerIndex_,
								data + len,
								bufferBegin());
		}
		writeAhead(len);
	}

	void append(const void* data, size_t len) {
		append(static_cast<const char*>(data), len);
	}

	void append(const StringView& view) {
		append(view.data(), view.size());
	}

	void append(const std::string& str) {
		append(str.data(), str.size());
	}

	void retrieveAll() {
		readerIndex_ = 0;
		writerIndex_ = 0;
	}

	void retrieve(size_t len) {
		if (len < readableBytes()) {
			readAhead(len);
		} else {
			retrieveAll();
		}
	}

	// no retrieveAsStringView or retrieveAllAsStringView because of
	// non-continuous buffer
	std::string retrieveAsString(size_t len) {
		len = std::min(readableBytes(), len);
		std::string result;
		result.reserve(len);
		if (needRewind()) {
			result.assign(beginRead(), bufferEnd());
			result.append(bufferBegin(), beginWrite());
		} else {
			result.assign(beginRead(), beginWrite());
		}
		retrieve(len);

		return result;
	}

	std::string retrieveAllAsString() {
		return retrieveAsString(readableBytes());
	}

	void ensureWritableBytes(size_t len) {
		if (writableBytes() < len) {
			extend(len);
		}
		assert(writableBytes() >= len);
	}

	// for debug
	size_t internalSize() const {
		return size_;
	}

private:
	void writeAhead(size_t len) {
		writerIndex_ = (writerIndex_ + len) % size_;
	}

	void readAhead(size_t len) {
		readerIndex_ = (readerIndex_ + len) % size_;
	}

	bool needRewind() const {
		return readerIndex_ > writerIndex_;
	}

	char* beginRead() {
		return buffer_ + readerIndex_;
	}

	char* beginWrite() {
		return buffer_ + writerIndex_;
	}

	const char* bufferBegin() const {
		return buffer_;
	}

	const char* bufferEnd() const {
		return buffer_ + size_;
	}

	char* bufferBegin() {
		return buffer_;
	}

	char* bufferEnd() {
		return buffer_ + size_;
	}

	void extend(size_t len) {
		size_t rlen = readableBytes();
		len += rlen;
		len *= 2;

		Buffer tmp(len);
		if (needRewind()) {
			tmp.append(beginRead(), bufferEnd() - beginRead());
			tmp.append(bufferBegin(), beginWrite() - bufferBegin());
		} else {
			tmp.append(beginRead(), readableBytes());
		}
		tmp.swap(*this);
	}

	size_t size_;
	char*  buffer_;

	size_t writerIndex_;
	size_t readerIndex_;
	static const char kCRLF[];
};

} // namespace leanet

#endif // LEANET_CIRCULAR_BUFFER_H
