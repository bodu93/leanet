#include "timestamp.h"
#include <time.h>
#include <inttypes.h> // PRId64
#include <sys/time.h>
#include <utility> // std::swap since C++11

namespace leanet {

string Timestamp::toString() const {
	int64_t seconds = microSecondsFromEpoch_ / kMicroSecondsPerSecond;
	int64_t microseconds = microSecondsFromEpoch_ % kMicroSecondsPerSecond;
	char buf[32] = {0};
	snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64, seconds, microseconds);
	return buf;
}

string Timestamp::toFormattedString(bool showMicroSeconds) const {
	time_t seconds = static_cast<time_t>(microSecondsFromEpoch_ / kMicroSecondsPerSecond);
	struct tm tmt;
	gmtime_r(&seconds, &tmt);

	char buf[32] = {0};
	if (showMicroSeconds) {
		int microseconds = static_cast<int>(microSecondsFromEpoch_ % kMicroSecondsPerSecond);
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
				tmt.tm_year + 1900, // year
				tmt.tm_mon + 1,			// month
				tmt.tm_mday,				// day
				tmt.tm_hour,				// hour
				tmt.tm_min,					// minute
				tmt.tm_sec,					// seconds
				microseconds);			// micro-seconds
	} else {
		snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
				tmt.tm_year + 1900,
				tmt.tm_mon + 1,
				tmt.tm_mday,
				tmt.tm_hour,
				tmt.tm_min,
				tmt.tm_sec);
	}
	return buf;
}

Timestamp Timestamp::now() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t ms = tv.tv_sec * Timestamp::kMicroSecondsPerSecond + tv.tv_usec;
	return Timestamp(ms);
}

}
