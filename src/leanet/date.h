#ifndef LEANET_DATE_H
#define LEANET_DATE_H

#include "types.h"
#include "copyable.h"

struct tm;

namespace leanet {

// Date in Gregorian calendar, a POD class
//

class Date: public copyable {
private:
	int julianDateNumber_;

public:
	struct YearMonthDay {
		int year;			// [1900, 2500]
		int month;		// [1, 12]
		int day;			// [1, 31]
	};

	static const int kDaysPerWeek = 7;
	static const int kJulianDayOf1970_01_01;

	Date()
		: julianDateNumber_(0)
	{ }

	Date(int year, int month, int day);

	explicit Date(int julianDayNum)
		: julianDateNumber_(julianDayNum)
	{ }

	explicit Date(const struct tm&);

	// implicit copy-control members are okay

	void swap(Date& other) {
		std::swap(julianDateNumber_, other.julianDateNumber_);
	}

	bool valid() const
	{ return julianDateNumber_ > 0; }

	string toIsoString() const;

	struct YearMonthDay yearMonthDay() const;

	int year() const {
		return yearMonthDay().year;
	}

	int month() const {
		return yearMonthDay().month;
	}

	int day() const {
		return yearMonthDay().day;
	}

	int weekDay() const {
		return (julianDateNumber_ + 1) % kDaysPerWeek;
	}

	int julianDateNumber() const
	{ return julianDateNumber_; }

};

inline bool operator<(Date lhs, Date rhs) {
	return lhs.julianDateNumber() < rhs.julianDateNumber();
}

inline bool operator==(Date lhs, Date rhs) {
	return lhs.julianDateNumber() == rhs.julianDateNumber();
}

} // namespace leanet

#endif
