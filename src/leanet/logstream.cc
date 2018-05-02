#include "logstream.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

namespace leanet {

namespace detail {

template<size_t SIZE> void FixedBuffer<SIZE>::cookieStart() { }
template<size_t SIZE> void FixedBuffer<SIZE>::cookieEnd() { }
template class FixedBuffer<leanet::LogStream::kSmallBufferSize>;
//template class FixedBuffer<leanet::LogStream::kLargeBufferSize>;

const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
const char digitsHex[] = "0123456789ABCDEF";

template<typename T>
size_t convert(char buf[], T value) {
	T i = value;
	char* p = buf;

	do {
		// note: lsd may be negative number,
		// so the digits array is subtle
		int lsd = static_cast<int>(i % 10);
		i /= 10;
		*p++ = zero[lsd];
	} while (i != 0);

	if (value < 0) {
		*p++ = '-';
	}
	*p = '\0';
	std::reverse(buf, p);
	return p - buf;
}

size_t convertHex(char buf[], uintptr_t value) {
	uintptr_t i = value;
	char* p = buf;

	do {
		int lsd = static_cast<int>(i % 16);
		assert(lsd >= 0);
		i /= 16;
		*p++ = digitsHex[lsd];
	} while (i != 0);

	*p = '\0';
	std::reverse(buf, p);
	return p - buf;
}

}

template<typename T>
Fmt::Fmt(const char* fmt, T val) {
	length_ = snprintf(buf_, sizeof(buf_), fmt, val);
	assert(static_cast<size_t>(length_) < sizeof(buf_));
}

// explicit instantiations
template Fmt::Fmt(const char* fmt, char);
template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);
template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

template<typename T>
void LogStream::appendInteger(T val) {
	if (buffer_.avail() >= kMaxNumericSize) {
		size_t len = detail::convert(buffer_.current(), val);
		buffer_.advance(len);
	}
}

LogStream& LogStream::operator<<(short v) {
	*this << static_cast<int>(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned short v) {
	*this << static_cast<unsigned int>(v);
	return *this;
}

LogStream& LogStream::operator<<(int v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned int v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(long long v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(unsigned long long v) {
	appendInteger(v);
	return *this;
}

LogStream& LogStream::operator<<(const void* p) {
	uintptr_t v = reinterpret_cast<uintptr_t>(p);
	if (buffer_.avail() >= kMaxNumericSize) {
		char* buf = buffer_.current();
		buf[0] = '0';
		buf[1] = 'x';
		size_t len = detail::convertHex(buf+2, v);
		buffer_.advance(len);
	}
	return *this;
}

LogStream& LogStream::operator<<(double v) {
	if (buffer_.avail() >= kMaxNumericSize) {
		int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
		buffer_.advance(static_cast<size_t>(len));
	}
	return *this;
}

}
