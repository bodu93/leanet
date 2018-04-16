#include <leanet/logger.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // getenv
#include <string.h> // strerror_r
#include <assert.h>

#include <leanet/currentthread.h>
#include <leanet/timestamp.h>
#include <leanet/timezone.h>

namespace leanet {

__thread char t_errnobuf[512];
const char* strerror_tl(int save_errno) {
	return strerror_r(save_errno, t_errnobuf, sizeof(t_errnobuf));
}

Logger::LogLevel initLogLevel() {
	if (::getenv("LEANET_LOG_TRACE"))
		return Logger::TRACE;
	else if (::getenv("LEANET_LOG_DEBUG"))
		return Logger::DEBUG;
	else
		return Logger::INFO;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char* LogLevelNames[Logger::NUM_LOG_LEVELS] = {
	"TRACE ",
	"DEBUG ",
	"INFO  ",
	"WARN  ",
	"ERROR ",
	"FATAL "
};

class StringHelper {
public:
	StringHelper(const char* str, size_t len)
		: str(str),
			len(len) {
	assert(strlen(str) == len);
	}

	const char* str;
	const size_t len;
};

inline LogStream& operator<<(LogStream& s, StringHelper v) {
	s.append(v.str, v.len);
	return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
	s.append(v.data, v.size);
	return s;
}


Logger::Impl::Impl(
		LogLevel level,
		int old_errno,
		const SourceFile& file,
		int line)
	: time_(Timestamp::now()),
		stream_(),
		level_(level),
		line_(line),
		basename_(file)
{
	// log message format:
	// [current-time] [tid] [log-level] [(errno = err) ] [log-message] [file]
	// [line] \n
	// [func]
	formatTime();
	currentThread::tid();
	stream_ << StringHelper(currentThread::tidString(), currentThread::tidStringLength());
	stream_ << StringHelper(LogLevelNames[level_], 6);
	if (save_errno != 0) {
		stream_ << strerror_tl(save_errno)
						<< " (errno = " << save_errno << ") ";
	}
}

TimeZone g_timezone;
__thread char t_time[32];
__thread time_t t_lastSecond;
void Logger::Impl::formatTime() {
	int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
	int seconds = microSecondsFromEpoch / Timestamp::kMicroSecondsPerSecond;
	int microseconds = microSecondsFromEpoch % Timestamp::kMicroSecondsPerSecond;
	// if output log message in one second, we use t_time
	if (seconds != t_lastSecond) {
		t_lastSecond = seconds;

		struct tm tmt;
		if (g_timezone.valid()) {
			tmt = g_timezone.toLocalTime(seconds);
		} else {
			::gmtime_r(&seconds, &tmt);
		}

		int len = snprintf(t_time, sizeof(t_time),
				"%4d%02d%02d %02d:%02d:%02d",
				tmt.tm_year + 1900,
				tmt.tm_mon + 1,
				tmt.tm_mday,
				tmt.tm_hour,
				tmt.tm_min,
				tmt.tm_sec);
		assert(len == 17);
		(void)len;
	}

	if (g_timezone.valid()) {
		Fmt us("%06d ", microseconds);
		assert(us.length() == 8);
		stream_ << StringHelper(t_time, 17) << StringHelper(us.data(), 8);
	} else {
		Fmt us("%06dZ ", microseconds);
		assert(us.length() == 9);
		stream_ << StringHelper(t_time, 17) << StringHelper(us.data(), 9);
	}
}

void Logger::Impl::finish() {
	stream_ << " - " << basename_ << ':' << line_ << '\n';
}

void defaultOutputCallback(const char* msg, int len) {
	::fwrite(msg, len, 1, stdout);
}

Logger::LogOutputCallback g_output = defaultOutputCallback;
void Logger::setOutputCallback(LogOutputCallback cb) {
	g_output = cb;
}

void defaultFlushCallback() {
	::fflush(stdout);
}

Logger::LogFlushCallback  g_flush  = defaultFlushCallback;
void Logger::setFlushCallback(LogFlushCallback cb) {
	g_flush = cb;
}

Logger::Logger(SourceFile file, int line)
	: impl_(INFO, 0, file, line)
{ }

Logger::Logger(SourceFile file, int line, LogLevel level)
	: impl_(level, 0, file, line)
{ }

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
	: impl_(level, 0, file, line)
{
	impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, bool to_abort)
	: impl_(to_abort?FATAL:ERROR, errno, file, line)
{ }

Logger::~Logger() {
	impl_.finish();
	const LogStream::Buffer& buffer(stream().buffer());
	g_output(buffer.data(), buffer.length());
	if (impl_.level_ == FATAL) {
		g_flush();
		abort();
	}
}

void Logger::setTimeZone(const TimeZone& tz) {
	g_timezone = tz;
}

void Logger::setLogLevel(LogLevel level) {
	g_logLevel = level;
}

}
