#include "app.h"

#include "config.h"
#include "datemath.h"
#include "display.h"
#include "exchange.h"
#include "hw.h"
#include "store.h"
#include "ui/renderer.h"
#include "generated/reflections.h"

namespace app {

// ---------------------------------------------------------------------------
// screens and modes
// ---------------------------------------------------------------------------
enum Mode : uint8_t {
  M_CAROUSEL,       // KEY1/KEY3 move between the four main screens
  M_FRIENDS,        // picking a friend in the list
  M_FRIEND_CARD,    // one friend
  M_MENU,           // picking a setting
  M_EDIT_DATE,
  M_EDIT_TEXT,
};

static const uint8_t CAROUSEL_COUNT = 4;  // main, reflection, friends, menu
static int S_MAIN, S_ANNIV, S_REFLECT, S_FRIENDS, S_MENU, S_FRIEND, S_EDIT_DATE, S_EDIT_TEXT, S_ADDED, S_RPS;

static Mode mode = M_CAROUSEL;
static uint8_t carousel = 0;
static int reflectPage = 0, reflectPages = 1;
static int friendSel = -1, menuSel = -1;
static bool confirmDelete = false, confirmClear = false;
static bool sessionDirty = false;  // a partial refresh happened since the last full one

// sorted view of friends: soonest anniversary first
static int order[MAX_FRIENDS];
static int32_t orderToday = -1;

// rock-paper-scissors / friend-added pop-ups
static Friend popupFriend;
static uint8_t rpsMine, rpsTheirs;

// ---------------------------------------------------------------------------
// editors
// ---------------------------------------------------------------------------
enum EditTarget : uint8_t { E_SOBER, E_TODAY, E_NAME, E_PHONE };
static EditTarget editTarget;
static int editField;
static int editY, editM, editD, editH;
static char editBuf[PHONE_LEN + 2];
static int editPos;
static int editChoice = -1;  // -1 editing; else the Save / Cancel (/ Space) option highlighted
static const char NAME_CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.-'";
static const char PHONE_CHARS[] = " 0123456789";

static const char* editChars() { return editTarget == E_NAME ? NAME_CHARS : PHONE_CHARS; }
static int editMax() { return editTarget == E_NAME ? NAME_LEN : PHONE_LEN; }

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static int32_t today() { return hw::today(); }

static void sortFriends() {
  int32_t t = today();
  for (int i = 0; i < store::friendCount; i++) order[i] = i;
  // insertion sort by days until the next anniversary
  for (int i = 1; i < store::friendCount; i++) {
    int k = order[i];
    int32_t dk = nextAnniversary(store::friends[k].soberDay, t);
    int j = i - 1;
    while (j >= 0 && nextAnniversary(store::friends[order[j]].soberDay, t) > dk) {
      order[j + 1] = order[j];
      j--;
    }
    order[j + 1] = k;
  }
  orderToday = t;
}

static void formatPhone(const char* digits, char* out, size_t n) {
  size_t len = strlen(digits);
  if (len == 10) snprintf(out, n, "%.3s-%.3s-%.4s", digits, digits + 3, digits + 6);
  else if (len == 11 && digits[0] == '1') snprintf(out, n, "1-%.3s-%.3s-%.4s", digits + 1, digits + 4, digits + 7);
  else if (len == 7) snprintf(out, n, "%.3s-%.4s", digits, digits + 3);
  else snprintf(out, n, "%s", digits);
}

static void formatUntil(int32_t days, char* out, size_t n) {
  if (days == 0) snprintf(out, n, "today");
  else if (days == 1) snprintf(out, n, "tomorrow");
  else if (days < 60) snprintf(out, n, "%ld days", (long)days);
  else snprintf(out, n, "%ld mos", (long)((days + 15) / 30));
}

// Years a friend will have at their next anniversary (shown as a reminder).
static int yearsAtNext(int32_t soberDay, int32_t t) {
  int32_t a = nextAnniversary(soberDay, t);
  return civilFromDays(a).y - civilFromDays(soberDay).y;
}

static int yearsNow(int32_t soberDay, int32_t t) {
  int y, m, d;
  diffYmd(soberDay, t, y, m, d);
  return y;
}

struct Sober {
  int32_t days;
  int y, m, d;
};

static Sober sober() {
  Sober s;
  int32_t t = today();
  s.days = t - store::settings.soberDay;
  if (s.days < 0) s.days = 0;
  diffYmd(store::settings.soberDay, t, s.y, s.m, s.d);
  return s;
}

static int treeStage(const Sober& s) {
  if (s.y >= 1) return 16 + (s.y > 10 ? 10 : s.y) - 1;  // youth tree 1-4 years, mature 5-10
  if (s.m >= 1) return 5 + s.m - 1;                       // months 1-11
  if (s.days <= 3) return 0;
  if (s.days <= 6) return 1;
  if (s.days <= 11) return 2;
  if (s.days <= 14) return 3;
  return 4;
}

// Is today a celebration? Fills number and unit for the medallion.
static bool milestone(const Sober& s, int& num, const char*& unit) {
  if (s.days == 30 || s.days == 60 || s.days == 90) { num = s.days; unit = "DAYS"; return true; }
  if (s.y == 0 && (s.m == 6 || s.m == 9) && s.d == 0) { num = s.m; unit = "MONTHS"; return true; }
  if (s.y >= 1 && s.m == 0 && s.d == 0) { num = s.y; unit = s.y == 1 ? "YEAR" : "YEARS"; return true; }
  return false;
}

static int nextFriend() {  // index into store::friends of the soonest anniversary, or -1
  if (orderToday != today()) sortFriends();
  return store::friendCount ? order[0] : -1;
}

// ---------------------------------------------------------------------------
// values for the screens
// ---------------------------------------------------------------------------
static const char* MENU_LABELS[] = {"Sober date", "Today", "Name", "Phone", "Friends", "Clear friends", "Battery", "Version", "Back"};
static const int MENU_COUNT = 9;
static const int MENU_BACK = 8;

class Data : public UiData {
 public:
  int item = -1;        // current list row
  const char* list = nullptr;

  void wrapPages(int pages) override { reflectPages = pages; }

  int listCount(const char* name) override {
    if (!strcmp(name, "friends")) return store::friendCount ? store::friendCount + 1 : 0;  // last row is "Back"
    if (!strcmp(name, "menu")) return MENU_COUNT;
    return 0;
  }
  void setListItem(const char* name, int index) override {
    list = name;
    item = index;
  }

  void value(const char* key, const char* fmt, char* out, size_t n) override {
    out[0] = 0;
    const int32_t t = today();
    auto num = [&](long v) {
      if (fmt && !strcmp(fmt, ",") && v >= 1000) snprintf(out, n, "%ld,%03ld", v / 1000, v % 1000);
      else snprintf(out, n, "%ld", v);
    };
    auto flag = [&](bool b) { snprintf(out, n, "%s", b ? "1" : ""); };
    auto str = [&](const char* s) { snprintf(out, n, "%s", s); };

    if (!strncmp(key, "item.", 5)) return itemValue(key + 5, fmt, out, n, t);

    if (!strcmp(key, "today")) return formatDate(t, fmt, out, n);
    if (!strcmp(key, "user.name")) return str(store::settings.name);
    if (!strcmp(key, "user.phone")) return formatPhone(store::settings.phone, out, n);
    if (!strcmp(key, "screen.index")) return num(carousel);
    if (!strcmp(key, "battery.pct")) return num(hw::batteryPercent());
    if (!strcmp(key, "battery.charging")) return flag(hw::charging());
    if (!strcmp(key, "hint")) return hint(out, n);

    if (!strncmp(key, "sober.", 6)) {
      Sober s = sober();
      const char* k = key + 6;
      if (!strcmp(k, "date")) return formatDate(store::settings.soberDay, fmt, out, n);
      if (!strcmp(k, "days")) return num(s.days);
      if (!strcmp(k, "short")) {
        if (s.y) snprintf(out, n, "%dy %dm %dd", s.y, s.m, s.d);
        else if (s.m) snprintf(out, n, "%dm %dd", s.m, s.d);
        else snprintf(out, n, "%ld day%s", (long)s.days, s.days == 1 ? "" : "s");
        return;
      }
      int mnum = 0;
      const char* unit = "";
      milestone(s, mnum, unit);
      if (!strcmp(k, "num")) return num(mnum);
      if (!strcmp(k, "unit")) return str(unit);
    }
    if (!strcmp(key, "tree.stage")) return num(treeStage(sober()));

    if (!strncmp(key, "next.", 5)) {
      int i = nextFriend();
      const char* k = key + 5;
      if (!strcmp(k, "any")) return flag(i >= 0);
      if (i < 0) return;
      const Friend& f = store::friends[i];
      int32_t a = nextAnniversary(f.soberDay, t);
      if (!strcmp(k, "today")) return flag(a == t && yearsAtNext(f.soberDay, t) >= 1);
      if (!strcmp(k, "name")) return str(f.name);
      if (!strcmp(k, "years")) return num(yearsAtNext(f.soberDay, t));
      if (!strcmp(k, "date")) return formatDate(a, fmt, out, n);
      if (!strcmp(k, "in")) return formatUntil(a - t, out, n);
    }

    if (!strcmp(key, "friends.any")) return flag(store::friendCount > 0);
    if (!strcmp(key, "friends.count")) return num(store::friendCount);
    if (!strcmp(key, "friends.sel")) return num(mode == M_FRIENDS ? friendSel : -1);
    if (!strcmp(key, "menu.sel")) return num(mode == M_MENU ? menuSel : -1);

    if (!strncmp(key, "reflect.", 8)) {
      const Reflection& r = REFLECTIONS[dayOfYear366(t) % REFLECTION_COUNT];
      const char* k = key + 8;
      if (!strcmp(k, "title")) return str(r.title);
      if (!strcmp(k, "text")) return str(r.text);
      if (!strcmp(k, "page")) return num(reflectPage);
      if (!strcmp(k, "more")) {
        if (reflectPages > 1) snprintf(out, n, "%d/%d", reflectPage + 1, reflectPages);
        return;
      }
    }

    if (!strncmp(key, "friend.", 7)) {
      const Friend& f = popupFriend;
      const char* k = key + 7;
      if (!strcmp(k, "name")) return str(f.name);
      if (!strcmp(k, "date")) return formatDate(f.soberDay, fmt, out, n);
      if (!strcmp(k, "years")) return num(yearsNow(f.soberDay, t));
      if (!strcmp(k, "phone")) return formatPhone(f.phone, out, n);
      if (!strcmp(k, "in")) return formatUntil(nextAnniversary(f.soberDay, t) - t, out, n);
      if (!strcmp(k, "confirm")) return flag(confirmDelete);
    }

    if (!strncmp(key, "rps.", 4)) {
      const char* k = key + 4;
      if (!strcmp(k, "name")) return str(popupFriend.name);
      if (!strcmp(k, "mine")) return num(rpsMine - 1);
      if (!strcmp(k, "theirs")) return num(rpsTheirs - 1);
      if (!strcmp(k, "result")) {
        if (rpsMine == rpsTheirs) return str("A tie!");
        bool win = (rpsMine == 1 && rpsTheirs == 3) || (rpsMine == 2 && rpsTheirs == 1) || (rpsMine == 3 && rpsTheirs == 2);
        return str(win ? "You win!" : "They win!");
      }
    }

    if (!strncmp(key, "edit.", 5)) return editValue(key + 5, out, n);
  }

 private:
  void itemValue(const char* k, const char* fmt, char* out, size_t n, int32_t t) {
    if (list && !strcmp(list, "menu")) return menuValue(k, out, n);
    if (item == store::friendCount) {  // the "Back" row
      if (!strcmp(k, "name")) snprintf(out, n, "Back");
      else if (!strcmp(k, "back")) snprintf(out, n, "1");
      return;
    }
    if (item < 0 || item >= store::friendCount) return;
    if (orderToday != t) sortFriends();
    const Friend& f = store::friends[order[item]];
    int32_t a = nextAnniversary(f.soberDay, t);
    if (!strcmp(k, "name")) snprintf(out, n, "%s", f.name);
    else if (!strcmp(k, "date")) formatDate(f.soberDay, fmt, out, n);
    else if (!strcmp(k, "years")) snprintf(out, n, "%d", yearsAtNext(f.soberDay, t));
    else if (!strcmp(k, "in")) formatUntil(a - t, out, n);
    else if (!strcmp(k, "phone")) formatPhone(f.phone, out, n);
    else if (!strcmp(k, "today")) snprintf(out, n, "%s", a == t ? "1" : "");
  }

  void menuValue(const char* k, char* out, size_t n) {
    if (!strcmp(k, "label")) { snprintf(out, n, "%s", MENU_LABELS[item]); return; }
    if (strcmp(k, "value")) return;
    switch (item) {
      case 0: formatDate(store::settings.soberDay, "MMM d, yyyy", out, n); break;
      case 1: formatDate(today(), "MMM d, yyyy", out, n); break;
      case 2: snprintf(out, n, "%s", store::settings.name); break;
      case 3: if (store::settings.phone[0]) formatPhone(store::settings.phone, out, n); else snprintf(out, n, "not set"); break;
      case 4: snprintf(out, n, "%d", store::friendCount); break;
      case 5: snprintf(out, n, "%s", confirmClear ? "press \201 again to clear all" : ""); break;
      case 6: {
        int cv = (int)(hw::batteryVolts() * 100 + 0.5f);  // no float printf on this core
        snprintf(out, n, "%d%%  \177  %d.%02d V%s", hw::batteryPercent(), cv / 100, cv % 100, hw::charging() ? "  \177  charging" : "");
        break;
      }
      case 7: snprintf(out, n, "%s", FIRMWARE_VERSION); break;
    }
  }

  void editValue(const char* k, char* out, size_t n) {
    static const char* MON[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    if (!strcmp(k, "title")) {
      static const char* T[] = {"Sobriety date", "Today", "Your name", "Your phone"};
      snprintf(out, n, "%s", T[editTarget]);
      return;
    }
    if (k[0] == 'l' || k[0] == 'v' || k[0] == 's') {
      int f = k[1] - '0';
      if (k[0] == 's') { snprintf(out, n, "%s", (mode == M_EDIT_DATE && editField == f) ? "1" : ""); return; }
      if (k[0] == 'l') {
        static const char* L[] = {"Month", "Day", "Year", "Hour"};
        if (f < 3 || editTarget == E_TODAY) snprintf(out, n, "%s", L[f]);
        return;
      }
      switch (f) {
        case 0: snprintf(out, n, "%s", MON[editM - 1]); break;
        case 1: snprintf(out, n, "%d", editD); break;
        case 2: snprintf(out, n, "%d", editY); break;
        case 3:
          if (editTarget == E_TODAY) snprintf(out, n, "%d %s", editH % 12 == 0 ? 12 : editH % 12, editH < 12 ? "AM" : "PM");
          break;
      }
      return;
    }
    if (!strcmp(k, "has3")) { snprintf(out, n, "%s", editTarget == E_TODAY ? "1" : ""); return; }
    if (!strncmp(k, "hint", 4)) {
      const bool text = mode == M_EDIT_TEXT;
      const char* h = "";
      char opts[48];
      if (editChoice >= 0) {
        static const char* O[] = {"Save", "Cancel", "Space"};
        int m = 0;
        for (int i = 0; i < (text ? 3 : 2); i++)
          m += snprintf(opts + m, sizeof opts - m, i == editChoice ? "%s[%s]" : "%s %s ", i ? " " : "", O[i]);
        if (k[4] == '1') h = opts;
        else if (k[4] == '2') h = "\202 \203 choose  \177  \201 ok";
      } else if (text) {
        if (k[4] == '1') h = "\202 \203 change letter";
        else if (k[4] == '2') h = "\201 next letter";
        else h = "pick _ then \201 to finish";
      } else {
        if (k[4] == '1') h = "\202 more  less \203";
        else if (k[4] == '2') h = "\201 next";
      }
      snprintf(out, n, "%s", h);
      return;
    }
    const char* cs = editChars();
    int len = strlen(cs);
    const char* p = strchr(cs, editBuf[editPos] ? editBuf[editPos] : ' ');
    int ci = p ? (int)(p - cs) : 0;
    auto shown = [&](int i) -> char { char c = cs[(i + len) % len]; return c == ' ' ? '_' : c; };
    if (!strcmp(k, "text")) {
      int m = 0;
      for (int i = 0; i < editPos && editBuf[i] && m + 2 < (int)n; i++) out[m++] = editBuf[i];
      out[m++] = '_';
      out[m] = 0;
    } else if (!strcmp(k, "prev")) snprintf(out, n, "%c", shown(ci - 1));
    else if (!strcmp(k, "cur")) snprintf(out, n, "%c", shown(ci));
    else if (!strcmp(k, "next")) snprintf(out, n, "%c", shown(ci + 1));
  }

  void hint(char* out, size_t n) {
    switch (mode) {
      case M_FRIENDS: snprintf(out, n, "\202 \203 move  \177  \201 open"); break;
      case M_MENU: snprintf(out, n, "\202 \203 move  \177  \201 change"); break;
      default:
        if (carousel == 2 && store::friendCount) snprintf(out, n, "\201 choose a friend");
        else if (carousel == 3) snprintf(out, n, "\201 choose a setting");
        break;
    }
  }
};

static Data data;

// ---------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------
static int currentScreen() {
  switch (mode) {
    case M_FRIEND_CARD: return S_FRIEND;
    case M_EDIT_DATE: return S_EDIT_DATE;
    case M_EDIT_TEXT: return S_EDIT_TEXT;
    case M_FRIENDS: return S_FRIENDS;
    case M_MENU: return S_MENU;
    default: break;
  }
  switch (carousel) {
    case 0: {
      int num;
      const char* unit;
      return milestone(sober(), num, unit) ? S_ANNIV : S_MAIN;
    }
    case 1: return S_REFLECT;
    case 2: return S_FRIENDS;
    default: return S_MENU;
  }
}

static void show(bool full) {
  if (hw::batteryStale()) hw::readBattery();  // read before drawing: the battery pin is shared with the display
  int id = currentScreen();
  int pagesBefore = reflectPages;
  display::show(id, data, full);
  if (id == S_REFLECT && reflectPages != pagesBefore && reflectPages > 1) display::show(id, data, false);  // page count now known
  if (!full) sessionDirty = true;
}

static void showPopup(int screenId) {
  display::show(screenId, data, false);
  sessionDirty = true;
  uint32_t t0 = millis();
  while (millis() - t0 < POPUP_MS) {
    Event e = hw::wait(POPUP_MS - (millis() - t0));
    if (e != EV_TIMEOUT && e != EV_REED) break;  // any button closes the pop-up early
  }
}

// ---------------------------------------------------------------------------
// actions
// ---------------------------------------------------------------------------
static void startEditDate(EditTarget target) {
  editTarget = target;
  editField = 0;
  editChoice = -1;
  int32_t day = target == E_SOBER ? store::settings.soberDay : today();
  Ymd d = civilFromDays(day);
  editY = d.y; editM = d.m; editD = d.d;
  editH = (hw::now() % 86400UL) / 3600UL;
  mode = M_EDIT_DATE;
}

static void startEditText(EditTarget target) {
  editTarget = target;
  memset(editBuf, 0, sizeof editBuf);
  strncpy(editBuf, target == E_NAME ? store::settings.name : store::settings.phone, editMax());
  editPos = 0;
  editChoice = -1;
  mode = M_EDIT_TEXT;
}

static void saveEditDate() {
  int32_t day = daysFromCivil(editY, editM, editD);
  if (editTarget == E_SOBER) {
    store::settings.soberDay = day;
  } else {
    uint32_t secs = (uint32_t)day * 86400UL + editH * 3600UL;
    hw::setNow(secs);
    store::settings.clockSecs = secs;
    orderToday = -1;
  }
  store::saveSettings();
  reflectPage = 0;
}

static void saveEditText() {
  int len = strlen(editBuf);
  while (len > 0 && editBuf[len - 1] == ' ') editBuf[--len] = 0;
  if (editTarget == E_NAME) {
    if (len) strncpy(store::settings.name, editBuf, NAME_LEN + 1);
  } else {
    char digits[PHONE_LEN + 1];
    int m = 0;
    for (int i = 0; editBuf[i] && m < PHONE_LEN; i++)
      if (editBuf[i] >= '0' && editBuf[i] <= '9') digits[m++] = editBuf[i];
    digits[m] = 0;
    strncpy(store::settings.phone, digits, PHONE_LEN + 1);
  }
  store::saveSettings();
}

static void stepDate(int dir) {
  switch (editField) {
    case 0: editM = (editM - 1 + dir + 12) % 12 + 1; break;
    case 1: editD += dir; break;
    case 2: editY += dir; if (editY < 1950) editY = 1950; if (editY > 2099) editY = 2099; break;
    case 3: editH = (editH + dir + 24) % 24; break;
  }
  int dim = daysInMonth(editY, editM);
  if (editD < 1) editD = dim;
  if (editD > dim) editD = editField == 1 ? 1 : dim;
}

static void handleKey(Event e) {
  if (e == EV_KEY2_LONG) e = EV_KEY2;  // no hold actions: a long press is just enter
  switch (mode) {
    case M_CAROUSEL:
      if (e == EV_KEY1) { carousel = (carousel + CAROUSEL_COUNT - 1) % CAROUSEL_COUNT; reflectPage = 0; }
      else if (e == EV_KEY3) { carousel = (carousel + 1) % CAROUSEL_COUNT; reflectPage = 0; }
      else if (e == EV_KEY2) {
        if (carousel == 1) reflectPage = (reflectPage + 1) % (reflectPages ? reflectPages : 1);
        else if (carousel == 2 && store::friendCount) { mode = M_FRIENDS; friendSel = 0; }
        else if (carousel == 3) { mode = M_MENU; menuSel = 0; confirmClear = false; }
      }
      break;

    case M_FRIENDS:
      if (e == EV_KEY1 && friendSel > 0) friendSel--;
      else if (e == EV_KEY3 && friendSel < store::friendCount) friendSel++;  // friendCount = the Back row
      else if (e == EV_KEY2 && friendSel >= store::friendCount) mode = M_CAROUSEL;
      else if (e == EV_KEY2 && store::friendCount) {
        popupFriend = store::friends[order[friendSel]];
        confirmDelete = false;
        mode = M_FRIEND_CARD;
      } else if (e == EV_KEY2_LONG) mode = M_CAROUSEL;
      break;

    case M_FRIEND_CARD:
      if (e == EV_KEY3) {
        if (!confirmDelete) confirmDelete = true;
        else {
          store::deleteFriend(order[friendSel]);
          sortFriends();
          if (friendSel >= store::friendCount) friendSel = store::friendCount - 1;
          mode = store::friendCount ? M_FRIENDS : M_CAROUSEL;
        }
      } else if (e == EV_KEY1 || e == EV_KEY2 || e == EV_KEY2_LONG) {
        mode = M_FRIENDS;
      }
      break;

    case M_MENU:
      if (e == EV_KEY1 && menuSel > 0) { menuSel--; confirmClear = false; }
      else if (e == EV_KEY3 && menuSel < MENU_COUNT - 1) { menuSel++; confirmClear = false; }
      else if (e == EV_KEY2_LONG) mode = M_CAROUSEL;
      else if (e == EV_KEY2) {
        switch (menuSel) {
          case 0: startEditDate(E_SOBER); break;
          case 1: startEditDate(E_TODAY); break;
          case 2: startEditText(E_NAME); break;
          case 3: startEditText(E_PHONE); break;
          case 4: carousel = 2; mode = store::friendCount ? M_FRIENDS : M_CAROUSEL; friendSel = 0; break;
          case MENU_BACK: mode = M_CAROUSEL; break;
          case 5:
            if (!confirmClear) confirmClear = true;
            else { store::clearFriends(); sortFriends(); confirmClear = false; }
            break;
          default: break;
        }
      }
      break;

    case M_EDIT_DATE: {
      int fields = editTarget == E_TODAY ? 4 : 3;
      if (editChoice >= 0) {  // after the last field: Save / Cancel
        if (e == EV_KEY1 || e == EV_KEY3) editChoice ^= 1;
        else if (e == EV_KEY2) {
          if (editChoice == 0) saveEditDate();
          editChoice = -1;
          mode = M_MENU;
        }
      } else if (e == EV_KEY1) stepDate(+1);  // up
      else if (e == EV_KEY3) stepDate(-1);    // down
      else if (e == EV_KEY2 && ++editField >= fields) editChoice = 0;
      break;
    }

    case M_EDIT_TEXT: {
      const char* cs = editChars();
      int len = strlen(cs);
      char c = editBuf[editPos] ? editBuf[editPos] : ' ';
      const char* p = strchr(cs, c);
      int ci = p ? (int)(p - cs) : 0;
      if (editChoice >= 0) {  // Save / Cancel / Space
        if (e == EV_KEY1) editChoice = (editChoice + 2) % 3;
        else if (e == EV_KEY3) editChoice = (editChoice + 1) % 3;
        else if (e == EV_KEY2) {
          if (editChoice == 2) {  // keep typing: the blank stays as a space
            if (editPos < editMax()) {
              editBuf[editPos] = ' ';
              if (editPos < editMax() - 1) editPos++;
            }
          } else {
            if (editChoice == 0) {
              for (int i = editPos; i < (int)sizeof editBuf; i++) editBuf[i] = 0;  // drop old text after the cursor
              saveEditText();
            }
            mode = M_MENU;
          }
          editChoice = -1;
        }
      } else if (e == EV_KEY1 || e == EV_KEY3) {
        ci = (ci + (e == EV_KEY3 ? 1 : -1) + len) % len;
        editBuf[editPos] = cs[ci];
        for (int i = 0; i < editPos; i++) if (!editBuf[i]) editBuf[i] = ' ';
      } else if (e == EV_KEY2) {
        if (c == ' ') editChoice = 0;  // enter on a blank: finish
        else if (editPos < editMax() - 1) editPos++;
        else { editPos++; editChoice = 0; }  // full: finish after the last letter
      }
      break;
    }
  }
}

static void friendTap() {
  exchange::Result r = exchange::run(store::settings);
  if (!r.found) return;
  bool isNew = store::upsertFriend(r.card);
  sortFriends();
  popupFriend = r.card;
  rpsMine = r.myThrow;
  rpsTheirs = r.theirThrow;
  showPopup(isNew ? S_ADDED : S_RPS);
}

// ---------------------------------------------------------------------------
// main loop
// ---------------------------------------------------------------------------
static int32_t shownDay = -1;

static void goHome() {
  mode = M_CAROUSEL;
  carousel = 0;
  reflectPage = 0;
  confirmDelete = confirmClear = false;
  show(true);
  sessionDirty = false;
  shownDay = today();
  display::sleep();
}

void begin() {
  S_MAIN = ui::screenByName("main");
  S_ANNIV = ui::screenByName("anniversary");
  S_REFLECT = ui::screenByName("reflection");
  S_FRIENDS = ui::screenByName("friends");
  S_MENU = ui::screenByName("menu");
  S_FRIEND = ui::screenByName("friend");
  S_EDIT_DATE = ui::screenByName("edit-date");
  S_EDIT_TEXT = ui::screenByName("edit-text");
  S_ADDED = ui::screenByName("added");
  S_RPS = ui::screenByName("rps");

  // Clock: the newest of the saved time and the firmware build time.
  uint32_t built = hw::buildTime();
  hw::setNow(store::settings.clockSecs > built ? store::settings.clockSecs : built);
  sortFriends();
  goHome();
}

void loop() {
  bool home = mode == M_CAROUSEL && carousel == 0 && !sessionDirty;
  // At home: sleep until midnight. Elsewhere: until the idle timeout.
  uint32_t timeout = home ? hw::secondsToMidnight() * 1000UL + 2000UL : IDLE_TIMEOUT_MS;
  Event e = hw::wait(timeout);
  LOG("event %d", (int)e);

  if (e == EV_TIMEOUT) {
    if (!home || today() != shownDay) {
      // midnight (or the end of a button session): save the clock, redraw home with a full refresh
      store::settings.clockSecs = hw::now();
      store::saveSettings();
      goHome();
    }
    return;
  }
  if (e == EV_REED) {
    friendTap();
    goHome();
    return;
  }
  handleKey(e);
  show(false);
}

}  // namespace app
