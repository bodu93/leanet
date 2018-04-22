#ifndef LEANET_STRING_VIEW_H
#define LEANET_STRING_VIEW_H

#include <string.h> // strlen

#include <leanet/types.h>

namespace leanet {

// a proxy of string
class StringView {
private:
	const char* data_;
	size_t length_;

public:
	StringView()
		: data_(NULL), length_(0)
	{ }

	StringView(const char* cstr)
		: data_(cstr),
			length_(static_cast<size_t>(strlen(cstr)))
	{ }

	StringView(const string& str)
		: data_(str.c_str()),
			length_(str.size())
	{ }

	StringView(const char* data, size_t len)
		: data_(data),
			length_(len)
	{ }

	// implicit copy control members are fine

	const char* data() const
	{ return data_; }

	size_t size() const
	{ return length_; }

	const char* begin() const
	{ return data(); }

	const char* end() const
	{ return begin() + size(); }

	string toString() const {
		return string(data_, length_);
	}
};

} // namespace leanet

#endif
