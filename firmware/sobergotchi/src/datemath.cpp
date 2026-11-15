#include "datemath.h"

static const char* const MONTHS[] = {"January", "February", "March", "April", "May", "June", "July",
                                     "August", "September", "October", "November", "December"};
static const char* const DAYS[] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

// Howard Hinnant's algorithms (public domain).
int32_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = (unsigned)(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

Ymd civilFromDays(int32_t z) {
  z += 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = (unsigned)(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int32_t y = (int32_t)yoe + era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned d = doy - (153 * mp + 2) / 5 + 1;
  const unsigned m = mp < 10 ? mp + 3 : mp - 9;
  Ymd r;
  r.y = (int16_t)(y + (m <= 2));
  r.m = (int8_t)m;
  r.d = (int8_t)d;
  return r;
}

int weekdayMon0(int32_t days) {  // 1970-01-01 was a Thursday (3)
  int w = (int)((days % 7 + 7 + 3) % 7);
  return w;
}

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int y, int m) {
  static const uint8_t dim[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return (m == 2 && isLeap(y)) ? 29 : dim[m - 1];
}

void diffYmd(int32_t from, int32_t to, int& years, int& months, int& days) {
  Ymd a = civilFromDays(from), b = civilFromDays(to);
  years = b.y - a.y;
  months = b.m - a.m;
  days = b.d - a.d;
  if (days < 0) {
    months--;
    int pm = b.m - 1, py = b.y;
    if (pm < 1) { pm = 12; py--; }
    days += daysInMonth(py, pm);
  }
  if (months < 0) { years--; months += 12; }
  if (years < 0) { years = months = days = 0; }
}

static int32_t anniversaryIn(Ymd d, int year) {
  int day = d.d;
  if (d.m == 2 && d.d == 29 && !isLeap(year)) day = 28;
  return daysFromCivil(year, d.m, day);
}

int32_t nextAnniversary(int32_t date, int32_t today) {
  Ymd d = civilFromDays(date), t = civilFromDays(today);
  int32_t a = anniversaryIn(d, t.y);
  if (a < today) a = anniversaryIn(d, t.y + 1);
  return a;
}

int dayOfYear366(int32_t days) {
  Ymd d = civilFromDays(days);
  return (int)(daysFromCivil(2000, d.m, d.d) - daysFromCivil(2000, 1, 1));
}

static void append(char* out, size_t n, size_t& len, const char* s) {
  while (*s && len + 1 < n) out[len++] = *s++;
  out[len] = 0;
}

void formatDate(int32_t days, const char* fmt, char* out, size_t n) {
  if (!fmt || !*fmt) fmt = "MMM d, yyyy";
  Ymd d = civilFromDays(days);
  int wd = weekdayMon0(days);
  size_t len = 0;
  out[0] = 0;
  char buf[12];
  while (*fmt) {
    if (!strncmp(fmt, "dddd", 4)) { append(out, n, len, DAYS[wd]); fmt += 4; }
    else if (!strncmp(fmt, "ddd", 3)) { strncpy(buf, DAYS[wd], 3); buf[3] = 0; append(out, n, len, buf); fmt += 3; }
    else if (!strncmp(fmt, "MMMM", 4)) { append(out, n, len, MONTHS[d.m - 1]); fmt += 4; }
    else if (!strncmp(fmt, "MMM", 3)) { strncpy(buf, MONTHS[d.m - 1], 3); buf[3] = 0; append(out, n, len, buf); fmt += 3; }
    else if (!strncmp(fmt, "MM", 2)) { snprintf(buf, sizeof buf, "%02d", d.m); append(out, n, len, buf); fmt += 2; }
    else if (*fmt == 'M') { snprintf(buf, sizeof buf, "%d", d.m); append(out, n, len, buf); fmt += 1; }
    else if (!strncmp(fmt, "dd", 2)) { snprintf(buf, sizeof buf, "%02d", d.d); append(out, n, len, buf); fmt += 2; }
    else if (*fmt == 'd') { snprintf(buf, sizeof buf, "%d", d.d); append(out, n, len, buf); fmt += 1; }
    else if (!strncmp(fmt, "yyyy", 4)) { snprintf(buf, sizeof buf, "%d", d.y); append(out, n, len, buf); fmt += 4; }
    else if (!strncmp(fmt, "yy", 2)) { snprintf(buf, sizeof buf, "%02d", d.y % 100); append(out, n, len, buf); fmt += 2; }
    else { buf[0] = *fmt++; buf[1] = 0; append(out, n, len, buf); }
  }
}
