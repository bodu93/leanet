#include "timezone.h"

//#include <endian.h>  // be32toh
#include <arpa/inet.h> // ntohs
#include <stdint.h>
#include <stdio.h>
#include <strings.h> // bzero
#include <assert.h>

#include <algorithm> // std::lower_bound
#include <stdexcept> // std::logic_error
#include <string>
#include <vector>

#include "types.h"
#include "noncopyable.h"
#include "date.h"

namespace leanet {

namespace detail {

struct Transition {
	time_t gmtime;
	time_t localtime;
	int localtimeIdx;

	Transition(time_t t, time_t l, int localIdx)
		: gmtime(t), localtime(l), localtimeIdx(localIdx)
	{ }
};

struct Comp {
	bool compareGmt;

	Comp(bool gmt)
		: compareGmt(gmt)
	{ }

	bool operator()(const Transition& lhs, const Transition& rhs) {
		if (compareGmt) {
			return lhs.gmtime < rhs.gmtime;
		} else {
			return lhs.localtime < rhs.localtime;
		}

		return false; // disable warning
	}

	bool equal(const Transition& lhs, const Transition& rhs) const {
		if (compareGmt) {
			return lhs.gmtime == rhs.gmtime;
		} else {
			return lhs.localtime == rhs.localtime;
		}

		return false; // disable warning
	}
};

// see struct ttinfo in timezone/tzfile.h which can find in glibc source archive
//
// struct ttinfo {
// 		long					tt_gmtoff;
// 		int						tt_isdst;
// 		unsigned int	tt_abbrind;
// };
struct Localtime {
	time_t gmtOffset;
	bool isDst;
	int arrbIdx;

	Localtime(time_t offset, bool dst, int arrb)
		: gmtOffset(offset), isDst(dst), arrbIdx(arrb)
	{ }
};

} // namespace leanet::detail

const int kSecondsPerDay = 24 * 60 * 60;

struct TimeZone::Data {
	std::vector<detail::Transition> transitions;
	std::vector<detail::Localtime> localtimes;
	std::vector<string> names;
	string abbreviation;
};

namespace detail {

inline void fillHMS(unsigned seconds, struct tm* utc) {
	utc->tm_sec = seconds % 60;
	unsigned minutes = seconds / 60;
	utc->tm_min = minutes % 60;
	utc->tm_hour = minutes / 60;
}

// A RAII file class used for parsing tzfile
class File: leanet::noncopyable {
public:
	explicit File(const char* fname)
		: fp_(fopen(fname, "rb"))
	{ }

	~File() {
		fclose(fp_);
	}

	bool valid() const { return fp_; }

	string readBytes(size_t n) {
		char buf[n];
		size_t rn = fread(buf, 1, n, fp_);
		if (rn != n) {
			throw std::logic_error("no enough data");
		}
		return string(buf, n);
	}

	int32_t readInt32() {
		int32_t x = 0;
		size_t rn = fread(&x, 1, sizeof(int32_t), fp_);
		if (rn != sizeof(int32_t)) {
			throw std::logic_error("bad int32_t data");
		}
		return ntohl(static_cast<uint32_t>(x));
	}

	uint8_t readUInt8() {
		uint8_t x = 0;
		size_t rn = fread(&x, 1, sizeof(uint8_t), fp_);
		if (rn != sizeof(uint8_t)) {
			throw std::logic_error("bad uint8_t data");
		}
		return x;
	}

private:
	FILE* fp_;
};

// "man tzfile" for detail informations
// simulate functionality of tzset(3)
bool parseTimeZoneFile(const char* fname, struct leanet::TimeZone::Data* data) {
	File f(fname);
	if (f.valid()) {
		try {
			string head = f.readBytes(4);
			if (head != "TZif") {
				throw std::logic_error("bad head");
			}
			string version = f.readBytes(1);
			f.readBytes(15); // skip reserved bytes

			int32_t isgmtcnt = f.readInt32();
			int32_t isstdcnt = f.readInt32();
			int32_t leapcnt = f.readInt32();
			int32_t timecnt = f.readInt32();
			int32_t typecnt = f.readInt32();
			int32_t charcnt = f.readInt32();

			std::vector<int32_t> trans;
			std::vector<int> localtimes;
			trans.reserve(timecnt);
			for(int i=0;i<timecnt;++i) {
				trans.push_back(f.readInt32());
			}

			for(int i=0;i<timecnt;++i) {
				uint8_t l = f.readUInt8();
				localtimes.push_back(l);
			}

			for(int i=0;i<typecnt;++i) {
				int32_t gmoff = f.readInt32();
				uint8_t isdst = f.readUInt8();
				uint8_t abbridx = f.readUInt8();
				data->localtimes.push_back(Localtime(gmoff, isdst, abbridx));
			}

			for(int i=0;i<timecnt;++i) {
				int localidx = localtimes[i];
				time_t localtime = trans[i] + data->localtimes[localidx].gmtOffset;
				data->transitions.push_back(Transition(trans[i], localtime, localidx));
			}

			data->abbreviation = f.readBytes(charcnt);
			Unused(isgmtcnt, isstdcnt, leapcnt);
		} catch (const std::logic_error& e) {
			fprintf(stderr, "%s\n", e.what());
		}
	}

	return true;
}

const Localtime* findLocaltime(const leanet::TimeZone::Data& data, Transition sentry, Comp comp) {
	const Localtime* local = NULL;

	if (data.transitions.empty() || comp(sentry, data.transitions.front())) {
		local = &data.localtimes.front();
	} else {
		std::vector<Transition>::const_iterator iter =
			std::lower_bound(data.transitions.begin(),
											 data.transitions.end(),
											 sentry,
											 comp);
		if (iter != data.transitions.end()) {
			if (!comp.equal(sentry, *iter)) {
				assert(iter != data.transitions.begin());
				--iter;
			}
			local = &data.localtimes[iter->localtimeIdx];
		} else {
			local = &data.localtimes[data.transitions.back().localtimeIdx];
		}
	}

	return local;
}

} // namespace leanet::detail

TimeZone::TimeZone(const char* zonefile)
	: data_(std::make_shared<TimeZone::Data>()) {
	if (!detail::parseTimeZoneFile(zonefile, data_.get())) {
		// release resource
		data_.reset();
	}
}

TimeZone::TimeZone(int eastOfUTC, const char* name)
	: data_(std::make_shared<TimeZone::Data>()) {
	data_->localtimes.push_back(detail::Localtime(eastOfUTC, false, 0));
	data_->abbreviation = name;
}

struct tm TimeZone::toLocalTime(time_t seconds) const {
	struct tm localTime;
	bzero(&localTime, sizeof(localTime));
	assert(data_ != nullptr);
	const Data& data(*data_);

	detail::Transition sentry(seconds, 0, 0);
	// ADL
	const detail::Localtime* local = findLocaltime(data, sentry, detail::Comp(true));

	if (local) {
		time_t localSeconds = seconds + local->gmtOffset;
		gmtime_r(&localSeconds, &localTime);
		localTime.tm_isdst = local->isDst;
		localTime.tm_gmtoff = local->gmtOffset;
		localTime.tm_zone = const_cast<char*>(&data.abbreviation[local->arrbIdx]);
	}

	return localTime;
}

time_t TimeZone::fromLocalTime(const struct tm& localTm) const {
	assert(data_ != NULL);
	const Data& data(*data_);

	struct tm tmp = localTm;
	time_t seconds = timegm(&tmp);
	detail::Transition sentry(0, seconds, 0);
	// ADL
	const detail::Localtime* local = findLocaltime(data, sentry, detail::Comp(false));

	if (localTm.tm_isdst) {
		struct tm tryTm = toLocalTime(seconds - local->gmtOffset);
		if (!tryTm.tm_isdst
				&& tryTm.tm_hour == localTm.tm_hour
				&& tryTm.tm_min == localTm.tm_min) {
			seconds -= 3600;
		}
	}
	return seconds - local->gmtOffset;
}

struct tm TimeZone::toUtcTime(time_t secondsSinceEpoch, bool yday) {
	struct tm utc;
	bzero(&utc, sizeof(utc));
// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wwritable-strings"
	utc.tm_zone = "GMT";
// #pragma GCC diagnostic pop
	int seconds = static_cast<int>(secondsSinceEpoch % kSecondsPerDay);
	int days = static_cast<int>(secondsSinceEpoch / kSecondsPerDay);
	if (seconds < 0) {
		seconds += kSecondsPerDay;
		--days;
	}
	detail::fillHMS(seconds, &utc);
	Date date(days + Date::kJulianDayOf1970_01_01);
	Date::YearMonthDay ymd = date.yearMonthDay();
	utc.tm_year = ymd.year - 1900;
	utc.tm_mon = ymd.month - 1;
	utc.tm_mday = ymd.day;
	utc.tm_wday = date.weekDay();

	if (yday) {
		Date startOfYear(ymd.year, 1, 1);
		utc.tm_yday = date.julianDateNumber() - startOfYear.julianDateNumber();
	}
	return utc;
}

time_t TimeZone::fromUtcTime(const struct tm& utc) {
	return fromUtcTime(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
										 utc.tm_hour, utc.tm_min, utc.tm_sec);
}

time_t TimeZone::fromUtcTime(int year, int month, int day,
														 int hour, int minute, int seconds) {
	Date date(year, month, day);
	int secondsInDay = hour * 3600 + minute * 60 + seconds;
	time_t days = date.julianDateNumber() - Date::kJulianDayOf1970_01_01;
	return days * kSecondsPerDay + secondsInDay;
}

} // namespace leanet
