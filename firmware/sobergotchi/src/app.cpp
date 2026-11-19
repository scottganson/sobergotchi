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
  M_EDIT_DATE,
  M_EDIT_TEXT,
};

static const uint8_t CAROUSEL_COUNT = 5;  // main, gnome, reflection, friends, menu
static int S_MAIN, S_ANNIV, S_REFLECT, S_FRIENDS, S_MENU, S_FRIEND, S_EDIT_DATE, S_EDIT_TEXT, S_ADDED, S_RPS, S_GNOME, S_CONFIRM, S_INTRO;

static Mode mode = M_CAROUSEL;
static uint8_t carousel = 0;
static int reflectPage = 0, reflectPages = 1;
static int friendSel = -1, menuSel = -1;
static bool confirmDelete = false;
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
static int editChoice = -1;  // date editor: -1 editing; else the Save / Cancel option highlighted
static bool confirmYes = false;  // on an "Are you sure?" question: Yes highlighted (starts on Cancel)
enum ConfirmWhat : uint8_t { C_SOBER, C_RESET };
static ConfirmWhat confirmWhat = C_SOBER;

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
static const char* MENU_LABELS[] = {"Sober date", "Today", "Name", "Phone", "Battery", "Version", "Factory reset", "Back"};
static const int MENU_COUNT = 8;
static const int MENU_RESET = 6;
static const int MENU_BACK = 7;

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
    if (!strcmp(key, "confirm.yes")) return flag(confirmYes);
    if (!strcmp(key, "confirm.title")) return str(confirmWhat == C_RESET ? "Factory reset" : "Sobriety date");
    if (!strcmp(key, "confirm.text"))
      return str(confirmWhat == C_RESET ? "Erase your name, date, phone and friends, and start over?"
                                        : "Are you sure you want to change your sobriety date?");
    if (!strcmp(key, "confirm.ok")) return str(confirmWhat == C_RESET ? "Yes, erase it" : "Yes, change it");
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
        snprintf(out, n, "Hold your Sobergotchi near another one to become friends. You'll swap your name, phone "
                         "number and sobriety date.");
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
      case 4: {
        int cv = (int)(hw::batteryVolts() * 100 + 0.5f);  // no float printf on this core
        snprintf(out, n, "%d%%  \177  %d.%02d V%s", hw::batteryPercent(), cv / 100, cv % 100, hw::charging() ? "  \177  charging" : "");
        break;
      }
      case 5: snprintf(out, n, "%s", FIRMWARE_VERSION); break;
      case MENU_RESET: snprintf(out, n, "start over with Pip"); break;
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
    switch (mode) {
      case M_FRIENDS: snprintf(out, n, "\202 \203 move  \177  \201 open"); break;
      case M_MENU: snprintf(out, n, "\202 \203 move  \177  \201 change"); break;
      case M_CONFIRM: snprintf(out, n, "\202 \203 choose  \177  \201 ok"); break;
      case M_INTRO:
        if (introStep == I_HELLO) snprintf(out, n, "\201 next");
        else snprintf(out, n, "\202 back  \177  \201 %s", introStep == I_DONE ? "begin" : "next");
        break;
      case M_EDIT_TEXT: snprintf(out, n, "\202 \203 move  \177  \201 pick"); break;
      default:
        if (carousel == 3 && store::friendCount) snprintf(out, n, "\201 choose a friend");
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

static void show(bool full) {
  if (hw::batteryStale()) hw::readBattery();
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
      else if (e == EV_KEY2 || e == EV_KEY3) {
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
        else if (carousel == 3 && store::friendCount) { mode = M_FRIENDS; friendSel = 0; }
        else if (carousel == 4) { mode = M_MENU; menuSel = 0; }
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
      if (e == EV_KEY1 && menuSel > 0) menuSel--;
      else if (e == EV_KEY3 && menuSel < MENU_COUNT - 1) menuSel++;
      else if (e == EV_KEY2_LONG) mode = M_CAROUSEL;
      else if (e == EV_KEY2) {
        switch (menuSel) {
          case 0: mode = M_CONFIRM; confirmWhat = C_SOBER; confirmYes = false; break;
          case 1: startEditDate(E_TODAY); break;
          case 2: startEditText(E_NAME); break;
          case 3: startEditText(E_PHONE); break;
          case MENU_RESET: mode = M_CONFIRM; confirmWhat = C_RESET; confirmYes = false; break;
          case MENU_BACK: mode = M_CAROUSEL; break;
          default: break;
        }
      }
      break;

    case M_CONFIRM:
      if (e == EV_KEY1 || e == EV_KEY3) confirmYes = !confirmYes;
      else if (e == EV_KEY2) {
        if (!confirmYes) mode = M_MENU;
        else if (confirmWhat == C_SOBER) startEditDate(E_SOBER);
        else {
          store::factoryReset();
          sortFriends();
          menuSel = -1;
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

// Full refresh only at power-up and the midnight redraw; partial otherwise (Scott saw ghosting right after full ones).
static void goHome(bool full) {
  mode = M_CAROUSEL;
  carousel = 0;
  reflectPage = 0;
  confirmDelete = false;
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
    if (e == EV_REED) return;
    handleKey(e);
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
  if (e == EV_REED) {
    friendTap();
    goHome(false);
    return;
  }
  handleKey(e);
  show(false);
}

}  // namespace app
