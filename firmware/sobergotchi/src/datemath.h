// Calendar maths on "day numbers" (days since 1970-01-01). No time zones: everything is local time.
#pragma once
#include <Arduino.h>

struct Ymd { int16_t y; int8_t m; int8_t d; };

int32_t daysFromCivil(int y, int m, int d);
Ymd civilFromDays(int32_t days);
int weekdayMon0(int32_t days);              // 0 = Monday ... 6 = Sunday
bool isLeap(int y);
int daysInMonth(int y, int m);
// Whole years, months and days from 'from' to 'to' (to >= from), counted by calendar.
void diffYmd(int32_t from, int32_t to, int& years, int& months, int& days);
// Day number of the next anniversary of 'date' on or after 'today' (Feb 29 -> Feb 28 in other years).
int32_t nextAnniversary(int32_t date, int32_t today);
// Index 0..365 into a 366-entry table (Feb 29 = 59, Mar 1 = 60 every year).
int dayOfYear366(int32_t days);
// Formats with tokens dddd ddd MMMM MMM MM M dd d yyyy yy (same as tools/sober/render.py).
void formatDate(int32_t days, const char* fmt, char* out, size_t n);
