#include "app.h"

#include "config.h"
#include "datemath.h"
#include "display.h"
#include "exchange.h"
#include "hw.h"
#include "store.h"
#include "ui/renderer.h"
#include "generated/reflections.h"
#include "generated/gnome_notes.h"

namespace app {

// ---------------------------------------------------------------------------
// screens and modes
// ---------------------------------------------------------------------------
enum Mode : uint8_t {
  M_CAROUSEL,       // KEY1/KEY3 move between the four main screens
  M_FRIENDS,        // picking a friend in the list
  M_FRIEND_CARD,    // one friend
  M_MENU,           // picking a setting
  M_CONFIRM,        // "Are you sure?" before the sobriety date editor or a factory reset
  M_INTRO,          // Pip's first-time setup (fresh flash or factory reset)
  M_SYSINFO,        // battery, version and device ID, from Settings
  M_HOWTO,          // Friends > Add friend: how to make a friend (hold left + right, devices head to head)
  M_EDIT_DATE,
  M_EDIT_TEXT,
};

static const uint8_t CAROUSEL_COUNT = 5;  // main, gnome, reflection, friends, menu
static int S_MAIN, S_ANNIV, S_REFLECT, S_FRIENDS, S_MENU, S_FRIEND, S_EDIT_DATE, S_EDIT_TEXT, S_ADDED, S_RPS, S_GNOME, S_CONFIRM, S_INTRO, S_SYSINFO, S_SEARCH, S_HOWTO;

static Mode mode = M_CAROUSEL;
static uint8_t carousel = 0;
static int reflectPage = 0, reflectPages = 1;
static int friendSel = -1, menuSel = -1;
static bool cardDelete = false;  // friend card: Delete highlighted (else Back)
static bool sessionDirty = false;  // a partial refresh happened since the last full one

// sorted view of friends: soonest anniversary first
static int order[MAX_FRIENDS];
static int32_t orderToday = -1;

// rock-paper-scissors / friend-added pop-ups
static Friend popupFriend;
static uint8_t rpsMine, rpsTheirs;

// Add friend (enter + up held): the radio looks for another Sobergotchi while the search screen is up
static bool searching = false;    // the search screen is showing, the buttons are held and the radio is on
static bool searchNone = false;   // the search ended without finding anyone

// ---------------------------------------------------------------------------
// editors
// ---------------------------------------------------------------------------
enum EditTarget : uint8_t { E_SOBER, E_TODAY, E_NAME, E_PHONE };
static EditTarget editTarget;
static int editField;
static int editY, editM, editD, editH;
static char editBuf[PHONE_LEN + 2];
static int editChoice = -1;  // date editor: -1 editing; else the Save / Cancel option highlighted
static bool confirmYes = false;  // on an "Are you sure?" question: Yes highlighted (starts on Cancel)
enum ConfirmWhat : uint8_t { C_SOBER, C_RESET, C_DELETE };
static ConfirmWhat confirmWhat = C_SOBER;
static bool confirmAgain = false;  // factory reset: the second, "really?" question

// Pip's first-time setup: a page per step; the name, sober date and phone pages open the usual editors.
enum IntroStep : uint8_t { I_HELLO, I_NAME, I_SOBER, I_FRIENDS, I_PHONE, I_DONE, I_COUNT };
static int introStep = I_HELLO;
static bool inIntro() { return !store::settings.setupDone; }

// Name / phone keyboard: up/down step through every key, then the actions under them (wrapping round); enter types
// the highlighted key or does the action. '^' is the Aa key (flips the case of the next letter).
static const char* const NAME_ROWS[] = {"ABCDE", "FGHIJ", "KLMNO", "PQRST", "UVWXY", "Z.-'^"};
static const char* const PHONE_ROWS[] = {"12345", "67890"};
enum KbAction : uint8_t { A_SPACE, A_DELETE, A_SAVE, A_CANCEL };
static const char* const KB_ACTION_LABELS[] = {"Space", "Delete", "Save", "Cancel"};
static int kbSel = 0;    // highlighted key, counted across the rows; past the last key, the actions
static bool kbShift = false;

static int editMax() { return editTarget == E_NAME ? NAME_LEN : PHONE_LEN; }
static bool kbName() { return editTarget == E_NAME; }
static int kbRows() { return kbName() ? 6 : 2; }
static const char* kbRowKeys(int r) { return r < 0 || r >= kbRows() ? "" : kbName() ? NAME_ROWS[r] : PHONE_ROWS[r]; }
static int kbActions() { return kbName() ? 4 : 3; }
static KbAction kbAction(int i) { return (KbAction)(kbName() ? i : i + 1); }  // the phone has no Space
static int kbKeys() { int k = 0; for (int r = 0; r < kbRows(); r++) k += strlen(kbRowKeys(r)); return k; }
static int kbItems() { return kbKeys() + kbActions(); }
static char kbKeyAt(int sel) {  // the key at a position, or 0 past the keys
  for (int r = 0; r < kbRows(); r++) {
    const int len = strlen(kbRowKeys(r));
    if (sel < len) return kbRowKeys(r)[sel];
    sel -= len;
  }
  return 0;
}
static int kbIndex(int r, int c) { int k = c; for (int i = 0; i < r; i++) k += strlen(kbRowKeys(i)); return k; }
// The character a key types: letters are capitalised at the start of each word, unless Aa flipped it.
static char kbChar(char key) {
  if (key < 'A' || key > 'Z') return key;
  int len = strlen(editBuf);
  bool upper = len == 0 || editBuf[len - 1] == ' ' || editBuf[len - 1] == '-';
  if (kbShift) upper = !upper;
  return upper ? key : key - 'A' + 'a';
}

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

// The sky changes every day: the same all day, a new one tomorrow. Order matches art/sky (tools/make_tree_ink.py).
enum Sky : uint8_t { SKY_CLEAR, SKY_FAIR, SKY_MORNING, SKY_PARTLY, SKY_BIRDS, SKY_OVERCAST, SKY_WINDY,
                     SKY_DRIZZLE, SKY_RAIN, SKY_STORM, SKY_SNOW, SKY_FOG, SKY_RAINBOW, SKY_COUNT };

static uint32_t dayHash(int32_t day, uint32_t salt) {
  uint32_t h = (uint32_t)day * 2654435761u + salt * 0x9E3779B9u + 0x5bd1e995u;
  h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15;
  return h;
}

static int skyPick(int32_t day, uint32_t salt) {
  const uint32_t h = dayHash(day, salt);
  // chances out of 100: mostly gentle days, some rain, the odd storm or rainbow
  static const uint8_t chance[SKY_COUNT] = {13, 17, 13, 13, 10, 8, 6, 8, 4, 2, 0, 3, 3};
  int r = h % 100, sky = SKY_FAIR;
  for (int i = 0; i < SKY_COUNT; i++) {
    if (r < chance[i]) { sky = i; break; }
    r -= chance[i];
  }
  const int month = civilFromDays(day).m;
  if (WINTER_SNOW && (month == 12 || month <= 2) && (sky == SKY_DRIZZLE || sky == SKY_RAIN || sky == SKY_STORM)) sky = SKY_SNOW;
  return sky;
}

static int skyFor(int32_t day) {
  int sky = skyPick(day, 0);
  if (sky == skyPick(day - 1, 0)) sky = skyPick(day, 1);  // avoid the same sky two days running
  return sky;
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

// The gnome: what he is doing today and the note he has, if any. Pose order matches art/gnome (tools/make_gnome.py).
enum Pose : uint8_t { P_STAND, P_WAVE, P_WATER, P_RAKE, P_READ, P_TEA, P_NAP, P_LANTERN, P_UMBRELLA, P_CHEER, P_COUNT };
struct Gnome { const GnomeNote* note; int years; int pose; };

static Gnome gnomeToday() {
  const int32_t t = today();
  const Sober s = sober();
  const int sky = skyFor(t);
  const uint32_t h = dayHash(t, 7);
  const bool wet = sky == SKY_DRIZZLE || sky == SKY_RAIN || sky == SKY_STORM || sky == SKY_SNOW;
  Gnome g = {nullptr, s.y, P_STAND};
  bool cheer = false;
  if (s.y >= 1 && s.m == 0 && s.d == 0) {            // a yearly birthday
    g.note = &GNOME_YEARS[s.y <= 15 ? s.y - 1 : 15];  // one note per year to fifteen, matching what the world gains
    cheer = true;
  } else if (s.y == 0 && s.m >= 1 && s.d == 0) {     // a monthly milestone in the first year
    g.note = &GNOME_MONTHS[s.m - 1];
    cheer = true;
  } else if (s.days <= 30) {                         // a note every day for the first thirty
    g.note = &GNOME_DAILY[s.days];
    cheer = s.days > 0 && (s.days % 7 == 0 || s.days % 10 == 0);
  } else if (wet || sky == SKY_FOG) {                // a gentle word for hard weather
    static const uint8_t any[4] = {0, 1, 3, 5};
    g.note = &GNOME_RAINY[sky == SKY_STORM ? 2 : sky == SKY_FOG ? 4 : any[h % 4]];
  }
  if (cheer) g.pose = P_CHEER;
  else if (wet) g.pose = P_UMBRELLA;
  else if (sky == SKY_FOG || sky == SKY_OVERCAST) g.pose = (h & 1) ? P_LANTERN : P_TEA;
  else if (sky == SKY_WINDY) g.pose = P_RAKE;
  else {
    static const uint8_t fair[8] = {P_STAND, P_WAVE, P_WATER, P_RAKE, P_READ, P_TEA, P_NAP, P_WATER};
    g.pose = fair[(h >> 3) % 8];
  }
  return g;
}

// From five years the gnome has a house in the tree, and each day the picture is one of three views. The years
// decide what exists (the house grows until ten; then a bridge, a well and a friend's cottage arrive); the day decides
// the view. Frame numbers match art/world and art/home (tools/make_world.py write_world_art).
enum View : uint8_t { V_WIDE, V_MIDDLE, V_CLOSE };
static const int POSE_NONE = P_COUNT;   // blank gnome layer: he is drawn in the home layer instead
static const int TREE_BLANK = 26;       // blank tree layer: the world layers take over
static const int BRIDGE_YEAR = 11, WELL_YEAR = 13, COTTAGE_YEAR = 15;
struct World { int tree; int world; int home; int pose; };

static World worldToday() {
  const Sober s = sober();
  const Gnome g = gnomeToday();
  World w = {treeStage(s), 0, 0, g.pose};
  if (s.y < 5) return w;
  const int32_t t = today();
  const int sky = skyFor(t);
  const bool wet = sky == SKY_DRIZZLE || sky == SKY_RAIN || sky == SKY_STORM || sky == SKY_SNOW;
  int view = (int)(dayHash(t, 13) % 3);
  if (view == (int)(dayHash(t - 1, 13) % 3)) view = (view + 1) % 3;   // a change of scene from yesterday
  const bool arrival = (s.y == BRIDGE_YEAR || s.y == WELL_YEAR || s.y == COTTAGE_YEAR) && s.m == 0 && s.d < 7;
  if (g.pose == P_CHEER || arrival) view = V_WIDE;     // celebrations and new arrivals: show the whole place
  else if (wet) view = V_CLOSE;                        // wet days: everyone indoors, rain on the windows
  const int level = (s.y > 10 ? 10 : s.y) - 4;          // house size 1-6
  const int household = s.y < 7 ? 0 : s.y == 7 ? 1 : 2; // gnome alone, with the cat, with partner and dog
  const int pick = (int)(dayHash(t, 17) % 3);
  w.tree = TREE_BLANK;
  w.pose = POSE_NONE;
  if (view == V_WIDE) {
    const int era = s.y >= COTTAGE_YEAR ? 3 : s.y >= WELL_YEAR ? 2 : s.y >= BRIDGE_YEAR ? 1 : 0;
    w.world = era ? 6 + era : level;
    w.home = 10 + 2 * (era ? 2 + era : household) + (pick & 1);
  } else if (view == V_MIDDLE) {
    w.world = 9 + level;
    w.home = household ? 21 + household : 0;
    w.pose = g.pose;
  } else {
    // the weather drawn in the windows: fine, rain, storm, snow, grey; the robin has the attic window from ten years
    const int pane = (sky == SKY_DRIZZLE || sky == SKY_RAIN) ? 1 : sky == SKY_STORM ? 2 : sky == SKY_SNOW ? 3
                   : (sky == SKY_OVERCAST || sky == SKY_FOG) ? 4 : 0;
    w.world = 16 + 2 * pane + (s.y >= 10 ? 1 : 0);
    w.home = 1 + household * 3 + pick;
  }
  return w;
}

// Copies src, putting the number v where it says {n}.
static void fillN(const char* src, int v, char* out, size_t n) {
  const char* p = strstr(src, "{n}");
  if (!p) snprintf(out, n, "%s", src);
  else snprintf(out, n, "%.*s%d%s", (int)(p - src), src, v, p + 3);
}

static int nextFriend() {  // index into store::friends of the soonest anniversary, or -1
  if (orderToday != today()) sortFriends();
  return store::friendCount ? order[0] : -1;
}

// ---------------------------------------------------------------------------
// values for the screens
// ---------------------------------------------------------------------------
// Settings has a User settings submenu; rows are named by item so both lists share one screen.
enum MenuItem : uint8_t { MI_SOBER, MI_TODAY, MI_NAME, MI_PHONE, MI_USER, MI_SYSINFO, MI_RESET, MI_BACK };
static const char* MENU_LABELS[] = {"Sober date", "Today", "Name", "Phone", "User settings", "System info", "Factory reset", "Back"};
static const uint8_t MENU_TOP[] = {MI_USER, MI_SYSINFO, MI_RESET, MI_BACK};
static const uint8_t MENU_USER[] = {MI_NAME, MI_PHONE, MI_SOBER, MI_TODAY, MI_BACK};
static bool menuUser = false;  // in the User settings submenu
static int menuCount() { return menuUser ? sizeof MENU_USER : sizeof MENU_TOP; }
static uint8_t menuItem(int i) { return menuUser ? MENU_USER[i] : MENU_TOP[i]; }

class Data : public UiData {
 public:
  int item = -1;        // current list row
  const char* list = nullptr;

  void wrapPages(int pages) override { reflectPages = pages; }

  int listCount(const char* name) override {
    // first row "Add friend", then the friends, last row "Back"; no friends: the empty page (enter adds one)
    if (!strcmp(name, "friends")) return store::friendCount ? store::friendCount + 2 : 0;
    if (!strcmp(name, "menu")) return menuCount();
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
    if (!strcmp(key, "tree.stage")) return num(worldToday().tree);
    if (!strcmp(key, "sky.index")) return num(skyFor(t));
    if (!strcmp(key, "gnome.pose")) return num(gnomeToday().pose);
    if (!strcmp(key, "gnome.home")) return num(worldToday().pose);
    if (!strcmp(key, "world.index")) return num(worldToday().world);
    if (!strcmp(key, "home.index")) return num(worldToday().home);
    if (!strncmp(key, "note.", 5)) {
      const Gnome g = gnomeToday();
      const char* k = key + 5;
      if (!strcmp(k, "any")) return flag(g.note != nullptr);
      if (!strcmp(k, "short")) { if (g.note) fillN(g.note->brief, g.years, out, n); return; }
      if (!strcmp(k, "full")) return fillN(g.note ? g.note->full : GNOME_ACTIVITY[g.pose], g.years, out, n);
      return;
    }

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
    if (!strcmp(key, "menu.title")) return str(menuUser ? "User settings" : "Settings");
    if (!strcmp(key, "confirm.yes")) return flag(confirmYes);
    if (!strcmp(key, "confirm.title"))
      return str(confirmWhat == C_RESET ? "Factory reset"
                 : confirmWhat == C_DELETE ? "Delete friend" : "Sobriety date");
    if (!strcmp(key, "confirm.text")) {
      if (confirmWhat == C_DELETE) {
        snprintf(out, n, "Delete %s from your friends? This can't be undone.", popupFriend.name);
        return;
      }
      return str(confirmWhat == C_SOBER ? "Are you sure you want to change your sobriety date?"
                 : confirmAgain ? "Really erase everything? This can't be undone."
                                : "Erase your name, date, phone and friends, and start over?");
    }
    if (!strcmp(key, "confirm.ok"))
      return str(confirmWhat == C_DELETE ? "Yes, delete" : confirmWhat == C_SOBER ? "Yes, change it"
                 : confirmAgain ? "Yes, erase all" : "Yes, erase it");
    if (!strcmp(key, "search.title")) {
      return str(searchNone ? "No one found" : "Looking...");
    }
    if (!strcmp(key, "search.text")) {
      if (searchNone) return str("Put your Sobergotchis head to head and both hold enter and up again.");
      return str("Keep holding while your friend holds theirs. Let go to stop.");
    }
    if (!strncmp(key, "sys.", 4)) {
      const char* k = key + 4;
      if (!strcmp(k, "battery")) {
        int cv = (int)(hw::batteryVolts() * 100 + 0.5f);  // no float printf on this core
        snprintf(out, n, "%d%%  \177  %d.%02d V", hw::batteryPercent(), cv / 100, cv % 100);
        return;
      }
      if (!strcmp(k, "power")) return str(hw::charging() ? "Charging" : "On battery");
      if (!strcmp(k, "version")) return str(FIRMWARE_VERSION);
      if (!strcmp(k, "friends")) return num(store::friendCount);
      if (!strcmp(k, "id")) { snprintf(out, n, "%08lX", (unsigned long)store::settings.uid); return; }
      return;
    }
    if (!strncmp(key, "intro.", 6)) return introValue(key + 6, out, n);

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
      if (!strcmp(k, "del")) return flag(cardDelete);
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
    if (!strncmp(key, "kb.", 3)) return kbValue(key + 3, out, n);
  }

 private:
  // Pip's setup pages: intro.title, intro.text, intro.pose, intro.step
  void introValue(const char* k, char* out, size_t n) {
    static const char* const TITLE[I_COUNT] = {"Hello!", "Your name", "Sobriety date", "Friends", "Your phone", "All set"};
    static const uint8_t POSE[I_COUNT] = {P_WAVE, P_STAND, P_READ, P_TEA, P_LANTERN, P_CHEER};
    const char* name = store::settings.name;
    if (!strcmp(k, "title")) { snprintf(out, n, "%s", TITLE[introStep]); return; }
    if (!strcmp(k, "pose")) { snprintf(out, n, "%d", POSE[introStep]); return; }
    if (!strcmp(k, "step")) { snprintf(out, n, "%d/%d", introStep + 1, (int)I_COUNT); return; }
    if (strcmp(k, "text")) return;
    switch (introStep) {
      case I_HELLO:
        snprintf(out, n, "Hello there, friend. I'm Pip. I look after the little garden in here, and I'd love to keep "
                         "you company, one day at a time.");
        break;
      case I_NAME: snprintf(out, n, "Let's get to know each other. What would you like me to call you?"); break;
      case I_SOBER:
        snprintf(out, n, "It's good to meet you, %s. When did your sobriety begin? I'll count every day from there.", name);
        break;
      case I_FRIENDS:
        snprintf(out, n, "To make a friend, put your Sobergotchis head to head and both hold the enter and up "
                         "buttons until your friend is added.");
        break;
      case I_PHONE:
        snprintf(out, n, "What number should your friends call? If you'd rather not share one, pick Cancel to skip.");
        break;
      default:
        snprintf(out, n, "That's everything, %s. Our garden grows with every sober day. Come and visit me any time.", name);
        break;
    }
  }

  void itemValue(const char* k, const char* fmt, char* out, size_t n, int32_t t) {
    if (list && !strcmp(list, "menu")) return menuValue(k, out, n);
    if (item == 0 || item == store::friendCount + 1) {  // the "Add friend" and "Back" rows
      if (!strcmp(k, "name")) snprintf(out, n, "%s", item ? "Back" : "Add friend");
      else if (!strcmp(k, "back")) snprintf(out, n, "1");  // plain row: no date badge
      else if (!strcmp(k, "add")) snprintf(out, n, "%s", item ? "" : "1");
      return;
    }
    if (item < 1 || item > store::friendCount) return;
    if (orderToday != t) sortFriends();
    const Friend& f = store::friends[order[item - 1]];
    int32_t a = nextAnniversary(f.soberDay, t);
    if (!strcmp(k, "name")) snprintf(out, n, "%s", f.name);
    else if (!strcmp(k, "date")) formatDate(f.soberDay, fmt, out, n);
    else if (!strcmp(k, "years")) snprintf(out, n, "%d", yearsAtNext(f.soberDay, t));
    else if (!strcmp(k, "in")) formatUntil(a - t, out, n);
    else if (!strcmp(k, "phone")) formatPhone(f.phone, out, n);
    else if (!strcmp(k, "today")) snprintf(out, n, "%s", a == t ? "1" : "");
  }

  void menuValue(const char* k, char* out, size_t n) {
    if (item < 0 || item >= menuCount()) return;
    const uint8_t it = menuItem(item);
    if (!strcmp(k, "label")) { snprintf(out, n, "%s", MENU_LABELS[it]); return; }
    if (strcmp(k, "value")) return;
    switch (it) {
      case MI_SOBER: formatDate(store::settings.soberDay, "MMM d, yyyy", out, n); break;
      case MI_TODAY: formatDate(today(), "MMM d, yyyy", out, n); break;
      case MI_NAME: snprintf(out, n, "%s", store::settings.name); break;
      case MI_PHONE: if (store::settings.phone[0]) formatPhone(store::settings.phone, out, n); else snprintf(out, n, "not set"); break;
      case MI_USER: snprintf(out, n, "name, phone, dates"); break;
      case MI_SYSINFO: snprintf(out, n, "battery, version"); break;
      case MI_RESET: snprintf(out, n, "start over with Pip"); break;
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
      if (k[0] == 's') { snprintf(out, n, "%s", (mode == M_EDIT_DATE && editChoice < 0 && editField == f) ? "1" : ""); return; }
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
      const char* h = "";
      char opts[48];
      if (editChoice >= 0) {
        static const char* O[] = {"Save", "Cancel"};
        int m = 0;
        for (int i = 0; i < 2; i++)
          m += snprintf(opts + m, sizeof opts - m, i == editChoice ? "%s[%s]" : "%s %s ", i ? " " : "", O[i]);
        if (k[4] == '1') h = opts;
        else if (k[4] == '2') h = "\202 \203 choose  \177  \201 ok";
      } else {
        if (k[4] == '1') h = "\202 more  less \203";
        else if (k[4] == '2') h = "\201 next";
      }
      snprintf(out, n, "%s", h);
      return;
    }
    if (!strcmp(k, "text")) {
      char shown[24];
      if (kbName()) snprintf(shown, sizeof shown, "%s", editBuf);
      else formatPhone(editBuf, shown, sizeof shown);
      snprintf(out, n, "%s_", shown);
    }
  }

  // The keyboard: kb.v<r> row shown, kb.c<r><c> key label, kb.h<r><c> key highlighted, kb.a<i> action label,
  // kb.ah<i> action highlighted.
  void kbValue(const char* k, char* out, size_t n) {
    auto flag = [&](bool b) { snprintf(out, n, "%s", b ? "1" : ""); };
    const int r = k[1] == 'h' ? k[2] - '0' : k[1] - '0';
    if (k[0] == 'v') return flag(r < kbRows());
    if (k[0] == 'c') {
      const int c = k[2] - '0';
      const char* keys = kbRowKeys(r);
      if (c >= (int)strlen(keys)) return;
      if (keys[c] == '^') snprintf(out, n, "Aa");
      else snprintf(out, n, "%c", kbChar(keys[c]));
      return;
    }
    if (k[0] == 'h') { const int c = k[2] - '0'; return flag(c < (int)strlen(kbRowKeys(r)) && kbSel == kbIndex(r, c)); }
    if (k[0] == 'a' && k[1] == 'h') { const int i = k[2] - '0'; return flag(i < kbActions() && kbSel == kbKeys() + i); }
    if (k[0] == 'a') { const int i = k[1] - '0'; if (i < kbActions()) snprintf(out, n, "%s", KB_ACTION_LABELS[kbAction(i)]); }
  }

  void hint(char* out, size_t n) {
    if (searching) { snprintf(out, n, "radio on while held"); return; }
    if (searchNone) { snprintf(out, n, "\201 back"); return; }
    switch (mode) {
      case M_HOWTO: snprintf(out, n, "\201 back"); break;
      case M_FRIENDS: snprintf(out, n, "\202 \203 move  \177  \201 %s", friendSel == 0 ? "add" : "open"); break;
      case M_MENU: snprintf(out, n, "\202 \203 move  \177  \201 change"); break;
      case M_CONFIRM: snprintf(out, n, "\202 \203 choose  \177  \201 ok"); break;
      case M_INTRO:
        if (introStep == I_HELLO) snprintf(out, n, "\201 next");
        else snprintf(out, n, "\202 back  \177  \201 %s", introStep == I_DONE ? "begin" : "next");
        break;
      case M_EDIT_TEXT: snprintf(out, n, "\202 \203 move  \177  \201 pick"); break;
      default:
        if (carousel == 3) snprintf(out, n, store::friendCount ? "\201 choose a friend" : "\201 add a friend");
        else if (carousel == 4) snprintf(out, n, "\201 choose a setting");
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
    case M_CONFIRM: return S_CONFIRM;
    case M_INTRO: return S_INTRO;
    case M_SYSINFO: return S_SYSINFO;
    case M_HOWTO: return S_HOWTO;
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
    case 1: return S_GNOME;
    case 2: return S_REFLECT;
    case 3: return S_FRIENDS;
    default: return S_MENU;
  }
}

static uint32_t draws = 0;  // every screen drawn; lets loop() tell whether a key changed anything

static void draw(int screenId, bool full) {
  draws++;
  display::show(screenId, data, full);
}

static void show(bool full) {
  if (hw::batteryStale()) hw::readBattery();
  int id = currentScreen();
  int pagesBefore = reflectPages;
  draw(id, full);
  if (id == S_REFLECT && reflectPages != pagesBefore && reflectPages > 1) draw(id, false);  // page count now known
  if (!full) sessionDirty = true;
}

// A pop-up for POPUP_MS or until enter. If enter + up are still held (Add friend), it waits for them to be let go
// first, so letting go doesn't close it.
static void showPopup(int screenId) {
  draw(screenId, false);
  sessionDirty = true;
  if (hw::comboHeld()) {
    while (hw::comboHeld()) delay(20);
    hw::flushKeys();
  }
  uint32_t t0 = millis();
  while (millis() - t0 < POPUP_MS) {
    Event e = hw::wait(POPUP_MS - (millis() - t0));
    if (e == EV_KEY2) break;  // enter closes the pop-up early; up / down only ever select
  }
}

// ---------------------------------------------------------------------------
// actions
// ---------------------------------------------------------------------------
static void startEditDate(EditTarget target) {
  editTarget = target;
  editField = 0;
  editChoice = -1;
  int32_t day = target == E_SOBER && !inIntro() ? store::settings.soberDay : today();  // setup starts from today
  Ymd d = civilFromDays(day);
  editY = d.y; editM = d.m; editD = d.d;
  editH = (hw::now() % 86400UL) / 3600UL;
  mode = M_EDIT_DATE;
}

static void startEditText(EditTarget target) {
  editTarget = target;
  memset(editBuf, 0, sizeof editBuf);
  if (!inIntro()) strncpy(editBuf, target == E_NAME ? store::settings.name : store::settings.phone, editMax());
  kbSel = 0;
  kbShift = false;
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

static bool saveEditText() {  // false when a name was left empty (nothing saved)
  int len = strlen(editBuf);
  while (len > 0 && editBuf[len - 1] == ' ') editBuf[--len] = 0;
  if (editTarget == E_NAME) {
    if (!len) return false;
    strncpy(store::settings.name, editBuf, NAME_LEN + 1);
  } else {
    char digits[PHONE_LEN + 1];
    int m = 0;
    for (int i = 0; editBuf[i] && m < PHONE_LEN; i++)
      if (editBuf[i] >= '0' && editBuf[i] <= '9') digits[m++] = editBuf[i];
    digits[m] = 0;
    strncpy(store::settings.phone, digits, PHONE_LEN + 1);
  }
  store::saveSettings();
  return true;
}

// Where an editor goes when it closes: back to Settings, or during setup on to Pip's next page.
static void editClosed(bool saved) {
  if (!inIntro()) { mode = M_MENU; return; }
  mode = M_INTRO;
  if (saved || editTarget == E_PHONE) introStep++;  // the phone may be skipped with Cancel
}

static void startIntro() {
  introStep = I_HELLO;
  mode = M_INTRO;
}

static void finishIntro() {
  store::settings.setupDone = 1;
  store::settings.clockSecs = hw::now();
  store::saveSettings();
  sortFriends();
  mode = M_CAROUSEL;
}

// ---------------------------------------------------------------------------
// Friends > Add friend
// ---------------------------------------------------------------------------
static bool isFriend(uint32_t uid) {
  for (int i = 0; i < store::friendCount; i++)
    if (store::friends[i].uid == uid) return true;
  return false;
}

// Back to the friends list after adding (or not), on the given friend's row if there is one.
static void friendsBack(uint32_t uid) {
  if (!store::friendCount) { mode = M_CAROUSEL; carousel = 3; return; }
  mode = M_FRIENDS;
  if (friendSel < 0) friendSel = 1;
  if (friendSel > store::friendCount + 1) friendSel = store::friendCount + 1;
  for (int i = 0; i < store::friendCount; i++)
    if (uid && store::friends[order[i]].uid == uid) friendSel = i + 1;
}

// Add friend mode, from enter + up held for ADD_FRIEND_HOLD_MS on any screen. The radio is on only while both are
// held (at most ADD_FRIEND_HOLD_MAX_MS), and ADD_FRIEND_SWAP_MS after someone is heard it turns off and they are added. A new friend is saved straight away ("Friend added!"); a friend we already have gets their card
// refreshed and a game of rock-paper-scissors.
static void addFriend() {
  searching = true;
  searchNone = false;
  draw(S_SEARCH, false);
  sessionDirty = true;
  exchange::start(store::settings, ADD_FRIEND_MIN_RSSI);
  LOG("add friend: radio on while held");
  const uint32_t t0 = millis();
  uint32_t foundAt = 0;
  while (hw::comboHeld() && millis() - t0 < ADD_FRIEND_HOLD_MAX_MS) {
    delay(20);
    if (!foundAt && exchange::found()) foundAt = millis();  // keep the radio on a moment so they hear us too
    if (foundAt && millis() - foundAt >= ADD_FRIEND_SWAP_MS) break;  // done: added while still held
  }
  exchange::finish();
  LOG("add friend: radio off (%s)", hw::comboHeld() ? "still held" : "let go");
  if (!hw::comboHeld()) hw::flushKeys();  // letting go can bounce: don't let that close the pop-up that comes next
  const exchange::Result r = exchange::best();
  searching = false;
  if (!r.found) {
    searchNone = true;
    showPopup(S_SEARCH);
    searchNone = false;
    friendsBack(0);
    return;
  }
  LOG("add friend: found %s", r.card.name);
  popupFriend = r.card;
  rpsMine = r.myThrow;
  rpsTheirs = r.theirThrow;
  const bool isNew = store::upsertFriend(r.card);  // no question: starting Add friend says you want to
  sortFriends();
  showPopup(isNew ? S_ADDED : S_RPS);
  friendsBack(r.card.uid);
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
    case M_INTRO:
      if (e == EV_KEY1 && introStep > I_HELLO) introStep--;
      else if (e == EV_KEY2) {  // enter only: down never moves on
        switch (introStep) {
          case I_NAME: startEditText(E_NAME); break;
          case I_SOBER: startEditDate(E_SOBER); break;
          case I_PHONE: startEditText(E_PHONE); break;
          case I_DONE: finishIntro(); break;
          default: introStep++; break;
        }
      }
      break;

    case M_CAROUSEL:
      if (e == EV_KEY1) { carousel = (carousel + CAROUSEL_COUNT - 1) % CAROUSEL_COUNT; reflectPage = 0; }
      else if (e == EV_KEY3) { carousel = (carousel + 1) % CAROUSEL_COUNT; reflectPage = 0; }
      else if (e == EV_KEY2) {
        if (carousel == 2) reflectPage = (reflectPage + 1) % (reflectPages ? reflectPages : 1);
        else if (carousel == 3 && store::friendCount) { mode = M_FRIENDS; friendSel = 1; }  // on the first friend
        else if (carousel == 3) mode = M_HOWTO;                                              // no friends yet
        else if (carousel == 4) { mode = M_MENU; menuUser = false; menuSel = 0; }
      }
      break;

    case M_SYSINFO:
      if (e == EV_KEY2) mode = M_MENU;  // enter: back to Settings (up / down do nothing here)
      break;

    case M_HOWTO:
      if (e == EV_KEY2) friendsBack(0);  // enter: back to Friends (on Add friend)
      break;

    case M_FRIENDS:  // rows: 0 Add friend, 1..friendCount the friends, friendCount + 1 Back
      if (e == EV_KEY1 && friendSel > 0) friendSel--;
      else if (e == EV_KEY3 && friendSel < store::friendCount + 1) friendSel++;
      else if (e == EV_KEY2 && friendSel == 0) mode = M_HOWTO;
      else if (e == EV_KEY2 && friendSel > store::friendCount) mode = M_CAROUSEL;
      else if (e == EV_KEY2) {
        popupFriend = store::friends[order[friendSel - 1]];
        cardDelete = false;  // Back highlighted
        mode = M_FRIEND_CARD;
      }
      break;

    case M_FRIEND_CARD:  // up / down choose Back or Delete; enter does it, and Delete asks first
      if (e == EV_KEY1 || e == EV_KEY3) cardDelete = !cardDelete;
      else if (e == EV_KEY2 && !cardDelete) mode = M_FRIENDS;
      else if (e == EV_KEY2) {
        mode = M_CONFIRM;
        confirmWhat = C_DELETE;
        confirmYes = false;  // Cancel highlighted
      }
      break;

    case M_MENU:
      if (e == EV_KEY1 && menuSel > 0) menuSel--;
      else if (e == EV_KEY3 && menuSel < menuCount() - 1) menuSel++;
      else if (e == EV_KEY2) {
        switch (menuItem(menuSel)) {
          case MI_SOBER: mode = M_CONFIRM; confirmWhat = C_SOBER; confirmYes = false; break;
          case MI_TODAY: startEditDate(E_TODAY); break;
          case MI_NAME: startEditText(E_NAME); break;
          case MI_PHONE: startEditText(E_PHONE); break;
          case MI_USER: menuUser = true; menuSel = 0; break;
          case MI_SYSINFO: mode = M_SYSINFO; break;
          case MI_RESET: mode = M_CONFIRM; confirmWhat = C_RESET; confirmAgain = false; confirmYes = false; break;
          case MI_BACK:
            if (menuUser) { menuUser = false; menuSel = 0; }  // back up to Settings, on User settings
            else mode = M_CAROUSEL;
            break;
          default: break;
        }
      }
      break;

    case M_CONFIRM:
      if (e == EV_KEY1 || e == EV_KEY3) confirmYes = !confirmYes;
      else if (e == EV_KEY2) {
        if (confirmWhat == C_DELETE) {
          if (confirmYes) {
            store::deleteFriend(order[friendSel - 1]);
            sortFriends();
            if (friendSel > store::friendCount) friendSel = store::friendCount;
            mode = store::friendCount ? M_FRIENDS : M_CAROUSEL;
          } else {
            cardDelete = false;
            mode = M_FRIEND_CARD;  // Cancel: back to their card, on Back
          }
        } else if (!confirmYes) mode = M_MENU;
        else if (confirmWhat == C_SOBER) startEditDate(E_SOBER);
        else if (!confirmAgain) { confirmAgain = true; confirmYes = false; }  // ask twice, Cancel highlighted again
        else {
          store::factoryReset();
          sortFriends();
          menuSel = -1;
          menuUser = false;
          startIntro();
        }
      }
      break;

    case M_EDIT_DATE: {
      int fields = editTarget == E_TODAY ? 4 : 3;
      if (editChoice >= 0) {  // after the last field: Save / Cancel
        if (e == EV_KEY1 || e == EV_KEY3) editChoice ^= 1;
        else if (e == EV_KEY2) {
          const bool save = editChoice == 0;
          if (save) saveEditDate();
          editChoice = -1;
          editClosed(save);
        }
      } else if (e == EV_KEY1) stepDate(+1);  // up
      else if (e == EV_KEY3) stepDate(-1);    // down
      else if (e == EV_KEY2 && ++editField >= fields) editChoice = 0;
      break;
    }

    case M_EDIT_TEXT: {
      int len = strlen(editBuf);
      if (e == EV_KEY1) kbSel = (kbSel + kbItems() - 1) % kbItems();
      else if (e == EV_KEY3) kbSel = (kbSel + 1) % kbItems();
      else if (e == EV_KEY2 && kbSel < kbKeys()) {  // type the key; stay on it so a double letter is one more press
        const char key = kbKeyAt(kbSel);
        if (key == '^') kbShift = !kbShift;
        else if (len < editMax()) { editBuf[len] = kbChar(key); kbShift = false; }
      } else if (e == EV_KEY2) {
        switch (kbAction(kbSel - kbKeys())) {
          case A_SPACE: if (len > 0 && len < editMax() && editBuf[len - 1] != ' ') editBuf[len] = ' '; break;
          case A_DELETE: if (len > 0) editBuf[len - 1] = 0; kbShift = false; break;
          case A_SAVE: editClosed(saveEditText()); break;
          case A_CANCEL: editClosed(false); break;
        }
      }
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// main loop
// ---------------------------------------------------------------------------
static int32_t shownDay = -1;

// Everything a screen is drawn from, so a key that changes none of it (enter on the home screen, say) leaves the
// panel alone: redrawing the same picture with a partial refresh still drives the ink and turns it grey.
struct UiState {
  uint32_t draws;
  int mode, carousel, reflectPage, friendSel, menuSel, introStep;
  int editTarget, editField, editY, editM, editD, editH, editChoice, kbSel, confirmWhat;
  bool cardDelete, menuUser, confirmYes, confirmAgain, kbShift, searching, searchNone;
  int friendCount;
  char editBuf[PHONE_LEN + 2];
};

static void snapshot(UiState& u) {
  memset(&u, 0, sizeof u);  // zero the padding too, so memcmp sees only the fields
  u.draws = draws;
  u.mode = mode; u.carousel = carousel; u.reflectPage = reflectPage; u.friendSel = friendSel; u.menuSel = menuSel;
  u.introStep = introStep;
  u.editTarget = editTarget; u.editField = editField; u.editY = editY; u.editM = editM; u.editD = editD;
  u.editH = editH; u.editChoice = editChoice; u.kbSel = kbSel; u.confirmWhat = confirmWhat;
  u.cardDelete = cardDelete; u.menuUser = menuUser; u.confirmYes = confirmYes; u.confirmAgain = confirmAgain;
  u.kbShift = kbShift; u.searching = searching; u.searchNone = searchNone;
  u.friendCount = store::friendCount;
  memcpy(u.editBuf, editBuf, sizeof editBuf);
}

// Handle a key; true if it changed something on screen (false: leave the panel alone).
static bool keyChanged(Event e) {
  UiState before, after;
  snapshot(before);
  handleKey(e);
  snapshot(after);
  if (memcmp(&before, &after, sizeof before) != 0) return true;
  LOG("key %d: nothing changed, no redraw", (int)e);
  return false;
}

// Full refresh only at power-up and the midnight redraw; partial otherwise (Scott saw ghosting right after full ones).
static void goHome(bool full) {
  mode = M_CAROUSEL;
  carousel = 0;
  menuUser = false;
  reflectPage = 0;
  cardDelete = false;
  show(full);
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
  S_CONFIRM = ui::screenByName("confirm");
  S_EDIT_TEXT = ui::screenByName("edit-text");
  S_ADDED = ui::screenByName("added");
  S_RPS = ui::screenByName("rps");
  S_GNOME = ui::screenByName("gnome");
  S_INTRO = ui::screenByName("intro");
  S_SYSINFO = ui::screenByName("sysinfo");
  S_SEARCH = ui::screenByName("search");
  S_HOWTO = ui::screenByName("howto");

  // Clock: the newest of the saved time and the firmware build time.
  uint32_t built = hw::buildTime();
  hw::setNow(store::settings.clockSecs > built ? store::settings.clockSecs : built);
  sortFriends();
  if (inIntro()) {  // fresh flash: Pip says hello and asks for the name, sober date and phone
    startIntro();
    show(true);
    sessionDirty = false;
    display::sleep();
    return;
  }
  goHome(true);
}

void loop() {
  if (inIntro()) {  // setup waits for the buttons: no idle timeout, no friend taps until Pip knows who you are
    Event e = hw::wait(IDLE_TIMEOUT_MS);
    if (e == EV_TIMEOUT) { display::sleep(); return; }
    if (e == EV_ADD_FRIEND) { while (hw::comboHeld()) delay(20); hw::flushKeys(); return; }  // not during setup
    if (!keyChanged(e)) return;
    if (mode == M_CAROUSEL) goHome(true);  // setup finished: a clean full refresh onto the home screen
    else show(false);
    return;
  }
  bool home = mode == M_CAROUSEL && carousel == 0 && !sessionDirty;
  // At home: sleep until midnight. Elsewhere: until the idle timeout.
  uint32_t timeout = home ? hw::secondsToMidnight() * 1000UL + 2000UL : IDLE_TIMEOUT_MS;
  Event e = hw::wait(timeout);
  LOG("event %d", (int)e);

  if (e == EV_TIMEOUT) {
    const bool newDay = today() != shownDay;
    if (!home || newDay) {
      // midnight (full refresh) or the end of a button session (partial): save the clock, redraw home
      store::settings.clockSecs = hw::now();
      store::saveSettings();
      goHome(newDay);
    }
    return;
  }
  if (e == EV_ADD_FRIEND) {  // enter + up held: Add friend from any screen, radio on while they are held
    addFriend();
    show(false);
    return;
  }
  if (keyChanged(e)) show(false);
}

}  // namespace app
