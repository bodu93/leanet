#ifndef LEANET_NONCOPYABLE_H
#define LEANET_NONCOPYABLE_H

namespace leanet {

class noncopyable {
public:
	noncopyable() { }

private:
	// disable copy ctor and copy assignment
	noncopyable(const noncopyable&);
	noncopyable& operator=(const noncopyable&);
};

}

#endif
