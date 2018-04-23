#ifndef LEANET_TIMEZONE_H
#define LEANET_TIMEZONE_H

#include <time.h>
#include <memory> // std::shared_ptr
#include <leanet/copyable.h>

namespace leanet {

// timezone for 1970~2030
class TimeZone: public copyable {
public:
	explicit TimeZone(const char* zonefile);
	TimeZone(int eastOfUTC, const char* tzname);
	TimeZone() { }

	// implicit copy-control members are okay

	bool valid() const
	{ return static_cast<bool>(data_); }

	// localtime(3): struct tm* localtime(const time_t* clock)
	struct tm toLocalTime(time_t secondsSinceEpoch) const;
	// mktime(3): time_t mktime(struct tm* timeptr)
	time_t fromLocalTime(const struct tm&) const;

	// gmtime(3): struct tm* gmtime(const time_t* clock)
	static struct tm toUtcTime(time_t secondsSinceEpoch, bool yday = false);
	// timegm(3): time_t timegm(const struct tm* timeptr)
	static time_t fromUtcTime(const struct tm&);
	static time_t fromUtcTime(int year,
														int month,
														int day,
														int hour,
														int minute,
														int seconds);

	struct Data;

private:
	std::shared_ptr<Data> data_;
};

} // namespace leanet

#endif
