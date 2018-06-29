#include <leanet/date.h>
#include <assert.h>
#include <gtest/gtest.h>
#include <stdio.h>

#include <time.h>
#include <sys/time.h>

const int kMonthOfYear = 12;

int isLeapYear(int year) {
  if (year % 400 == 0)
    return 1;
  else if (year % 100 == 0)
    return 0;
  else if (year % 4 == 0)
    return 1;
  else
    return 0;
}

int daysOfMonth(int year, int month) {
  static int days[2][kMonthOfYear+1] =
  {
    { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  };
  return days[isLeapYear(year)][month];
}

using namespace leanet;


TEST(DATE_TEST, DATE_API_TEST) {
  time_t t = ::time(NULL);
  struct tm tmt = *::localtime(&t);

  Date today(tmt);
  Date realToday(2018, 5, 17);

  ASSERT_TRUE(today.valid());
  ASSERT_TRUE(realToday.valid());

  EXPECT_EQ(today, realToday);
  EXPECT_EQ(today.year(), 2018);
  EXPECT_EQ(today.month(), 5);
  EXPECT_EQ(today.day(), 17);

  EXPECT_EQ(today.weekDay(), 4);
}

TEST(DATE_TEST, MONTHOFDAY_TEST) {
  time_t t = ::time(NULL);
  struct tm tmt = *::localtime(&t);

  Date today(tmt);

  int year = today.year();

  EXPECT_FALSE(isLeapYear(year));

  EXPECT_EQ(daysOfMonth(year, today.month()), 31);
}
