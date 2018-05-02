#ifndef LEANET_TIMERFD_H
#define LEANET_TIMERFD_H

#include "noncopyable.h"
#include "timestamp.h"

namespace leanet {

//
// timerfd implementation on macOS
// like Linux's timerfd* system calls since
// 2.6.25
//
class TimerFd: noncopyable {
public:
	TimerFd();
	~TimerFd();

	int getFd() const;
	void addTimer(Timestamp when, double interval);

private:
	bool timing_;
	int fd_[2];
};

}

#endif
