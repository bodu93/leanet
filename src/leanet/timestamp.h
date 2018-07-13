#ifndef LEANET_TIMESTAMP_H
#define LEANET_TIMESTAMP_H

#include <stdint.h>
#include <string>
#include <utility> // std::swap since C++11

#include "types.h"
#include "copyable.h"

namespace leanet {

class Timestamp: public copyable {
public:
	Timestamp(): microSecondsFromEpoch_(0) { }
	explicit Timestamp(int64_t microSeconds)
		: microSecondsFromEpoch_(microSeconds)
	{ }
	// implicit copy-control members are fine
	void swap(Timestamp& other) {
		std::swap(microSecondsFromEpoch_, other.microSecondsFromEpoch_);
	}

	// format: "<seconds>.<micro-seconds>"
	string toString() const;
	// calendar time to broken-down time
	string toFormattedString(bool showMicroSeconds = true) const;

	bool valid() const { return microSecondsFromEpoch_ > 0; };

	int64_t microSecondsFromEpoch() const
	{ return microSecondsFromEpoch_; }
	time_t secondsFromEpoch() const
	{ return static_cast<time_t>(microSecondsFromEpoch_ / kMicroSecondsPerSecond); }

	static Timestamp now();
	static Timestamp invalid()
	{ return Timestamp(); }

	static Timestamp fromUnixTime(time_t t)
	{ return fromUnixTime(t, 0); }
	static Timestamp fromUnixTime(time_t t, int microSeconds)
	{ return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microSeconds); }

  /*
   * second
   * millisecond: 1/1000 second
   * microsecond: 1/1000 millisecond
   * nanosecond:  1/1000 microsecond
   */
	static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
	int64_t microSecondsFromEpoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
	return lhs.microSecondsFromEpoch() < rhs.microSecondsFromEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
	return lhs.microSecondsFromEpoch() == rhs.microSecondsFromEpoch();
}

// micro-seconds diff between high and low
inline double timeDifference(Timestamp high, Timestamp low) {
	int64_t diff = high.microSecondsFromEpoch() - low.microSecondsFromEpoch();
	return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
	int64_t detla = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
	return Timestamp(timestamp.microSecondsFromEpoch() + detla);
}

} // namespace leanet

#endif
