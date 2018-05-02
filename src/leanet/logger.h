#ifndef LEANET_LOGGER_H
#define LEANET_LOGGER_H

#include "types.h"
#include "noncopyable.h"
#include "logstream.h"
#include "timestamp.h"

namespace leanet {

class TimeZone;

class Logger: noncopyable {
public:
	enum LogLevel {
		TRACE,
		DEBUG,
		INFO,
		WARN,
		ERROR,
		FATAL,
		NUM_LOG_LEVELS
	};

	class SourceFile {
	public:
		template<int N>
		inline SourceFile(const char (&arr)[N])
			: data(arr),
				size(N - 1) {
			// findlastof
			const char* slash = strrchr(data, '/');
			if (slash) {
				data = slash + 1;
				size -= static_cast<int>(data - arr);
			}
		}

		// c-style string
		explicit SourceFile(const char* filename)
			: data(filename) {
			const char* slash = strrchr(filename, '/');
			if (slash) {
				data = slash + 1;
			}
			size = static_cast<int>(strlen(data));
		}

		const char* data;
		int size;
	};

	Logger(SourceFile file, int line);
	Logger(SourceFile file, int line, LogLevel level);
	Logger(SourceFile file, int line, LogLevel level, const char* func);
	Logger(SourceFile file, int line, bool abort);
	~Logger();

	LogStream& stream() { return impl_.stream_; }

	static LogLevel logLevel();
	static void setLogLevel(LogLevel level);

	typedef void (*LogOutputCallback)(const char* msg, int len);
	typedef void (*LogFlushCallback)();
	static void setOutputCallback(LogOutputCallback);
	static void setFlushCallback(LogFlushCallback);
	static void setTimeZone(const TimeZone& tz);

private:
	class Impl {
	public:
		Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
		void formatTime();
		void finish();

		Timestamp 		time_;
		LogStream 		stream_;
		LogLevel 			level_;
		int 					line_;
		SourceFile 		basename_;
	};

	Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() {
	return g_logLevel;
}

template<typename T>
T* checkNotNull(Logger::SourceFile file, int line, const char* msg, T* p) {
	if (p == NULL) {
		Logger(file, line, Logger::FATAL).stream() << msg;
	}
	return p;
}

#define CHECK_NOTNULL(val) \
	::leanet::checkNotNull(__FILE__, __LINE__, "'" #val "' Must be non-NULL", (val))

const char* strerror_tl(int saved_errno);

#define LOG_TRACE \
	if (leanet::Logger::logLevel() <= leanet::Logger::TRACE) \
		leanet::Logger(__FILE__, __LINE__, leanet::Logger::TRACE, __func__).stream() \

#define LOG_DEBUG \
	if (leanet::Logger::logLevel() <= leanet::Logger::DEBUG) \
		leanet::Logger(__FILE__, __LINE__, leanet::Logger::DEBUG, __func__).stream() \

#define LOG_INFO \
	if (leanet::Logger::logLevel() <= leanet::Logger::INFO) \
		leanet::Logger(__FILE__, __LINE__).stream() \

#define LOG_WARN leanet::Logger(__FILE__, __LINE__, leanet::Logger::WARN).stream()

#define LOG_ERROR leanet::Logger(__FILE__, __LINE__, leanet::Logger::ERROR).stream()

#define LOG_FATAL leanet::Logger(__FILE__, __LINE__, leanet::Logger::FATAL).stream()

#define LOG_SYSERR leanet::Logger(__FILE__, __LINE__, false).stream()

#define LOG_SYSFATAL leanet::Logger(__FILE__, __LINE__, true).stream()

}

#endif
