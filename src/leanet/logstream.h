#ifndef LEANET_LOGSTREAM_H
#define LEANET_LOGSTREAM_H

#include <assert.h>
#include <string.h> // bzero

#include "types.h"
#include "noncopyable.h"
#include "stringview.h"

namespace leanet {

namespace detail {

template<size_t SIZE>
class FixedBuffer: noncopyable {
private:
	void (*cookie_)();
	char buffer_[SIZE];
	char* cur_;

	const char* end() const {
		return cur_ + SIZE;
	}

	static void cookieStart();
	static void cookieEnd();

public:
	FixedBuffer()
		: cur_(buffer_) {
		setCookie(cookieStart);
	}

	~FixedBuffer() {
		setCookie(cookieEnd);
	}

	void append(const char* data, size_t len) {
		if (avail() > len) {
			::memcpy(cur_, data, len);
			cur_ += len;
		}
	}

	void setCookie(void (*x)()) { cookie_ = x; }

	const char* buffer() const { return buffer_; }
	size_t length() const { return static_cast<size_t>(cur_ - buffer_); }

	char* current() { return cur_; }
	// bytes left length
	size_t avail() const { return static_cast<size_t>(end() - cur_); }
	void advance(size_t n) { cur_ += n; }

	void rewind() { cur_ = buffer_; }
	void bzero() { ::bzero(buffer_, SIZE); }

	string toString() const { return string(buffer_, SIZE); }
	StringView toStringView() const { return StringView(buffer_, SIZE); }
};

}

class Fmt {
public:
	template<typename T>
	Fmt(const char* fmt, T val);

	const char* data() const { return buf_; }
	int length() const { return length_; }

private:
	char buf_[32];
	int length_;
};

// see cfront1.0's stream.h:
// http://www.softwarepreservation.org/projects/c_plus_plus/cfront/release_1.0/src/cfront/incl/stream.h/view
class LogStream: noncopyable {
	typedef LogStream self;
public:
	static const size_t kSmallBufferSize = 4000;
	static const size_t kLargeBufferSize = 4000*1000;
	typedef detail::FixedBuffer<kSmallBufferSize> Buffer;

	// integers
	self& operator<<(bool x) {
		const char* s = x ? "true" : "false";
		buffer_.append(s, strlen(s));
		return *this;
	}

	self& operator<<(short);
	self& operator<<(unsigned short);
	self& operator<<(int);
	self& operator<<(unsigned int);
	self& operator<<(long);
	self& operator<<(unsigned long);
	self& operator<<(long long);
	self& operator<<(unsigned long long);

	self& operator<<(const void*); // variant types

	// float-point numbers
	self& operator<<(float f) {
		return operator<<(static_cast<double>(f));
	}

	self& operator<<(double);

	// character-based variables
	self& operator<<(char c) {
		buffer_.append(&c, 1);
		return *this;
	}

	// c-style string
	self& operator<<(const char* str) {
		if (str) {
			buffer_.append(str, strlen(str));
		} else {
			buffer_.append("(null)", 6);
		}
		return *this;
	}

	// binary buffer
	self& operator<<(const unsigned char* str) {
		return operator<<(reinterpret_cast<const char*>(str));
	}

	self& operator<<(const string& v) {
		buffer_.append(v.c_str(), v.size());
		return *this;
	}

	self& operator<<(const StringView& v) {
		buffer_.append(v.data(), v.size());
		return *this;
	}

	self& operator<<(const Buffer& v) {
		return operator<<(v.toStringView());
	}

	self& operator<<(const Fmt& fmt) {
		buffer_.append(fmt.data(), fmt.length());
		return *this;
	}

	void append(const char* data, size_t len) {
		buffer_.append(data, len);
	}

	const Buffer& buffer() const { return buffer_; }
	void resetBuffer() { buffer_.rewind(); }

private:
	static const size_t kMaxNumericSize = 32;

	template<typename T>
	void appendInteger(T val);

	Buffer buffer_;
};

}

#endif
