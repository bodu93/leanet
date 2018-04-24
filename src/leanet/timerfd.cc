#include <leanet/timerfd.h>

#include <unistd.h> // pipe

#include <leanet/logger.h>

using namespace leanet;

TimerFd::TimerFd()
	: timing_(false) {
	if (::pipe(fd_) == -1) {
		LOG_FATAL << "TimerFd not avaliable.";
	}
}

TimerFd::~TimerFd() {
	::close(fd_[1]);
	::close(fd_[0]);
}

int TimerFd::getFd() const {
	return fd_[0];
}

void TimerFd::setTime(Timestamp when, double interval) {
	// TODO
}
