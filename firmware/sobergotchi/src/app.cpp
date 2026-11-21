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
#include "prayers.h"

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
  M_LANDING,        // the title page before Pip's setup: any key starts it
  M_INTRO,          // Pip's first-time setup (fresh flash or factory reset)
  M_SYSINFO,        // battery, version and device ID, from Settings
  M_HOWTO,          // Friends > Add friend: how to make a friend (hold left + right, devices head to head)
  M_EDIT_DATE,
  M_EDIT_TEXT,
  M_PATH,           // picking a row on Your path
  M_UNLOCK,         // Pip's "Something new!" when a Your path item opens (shown instead of the home screen)
  M_CHECKIN,        // daily check-in: one thing just for today
  M_PICK,           // a list to tick: Gratitude jar, Your word, Share your story
  M_PRAYER,         // a Step prayer, a page at a time
  M_STORY,          // a story: yours or a friend's
  M_TILES,          // Tree tiles, the sliding puzzle
};

static const uint8_t CAROUSEL_COUNT = 6;  // main, gnome, reflection, friends, Your path, menu
static const uint8_t C_PATH = 4, C_MENU = 5;
static int S_MAIN, S_ANNIV, S_REFLECT, S_FRIENDS, S_MENU, S_FRIEND, S_EDIT_DATE, S_EDIT_TEXT, S_ADDED, S_RPS, S_GNOME, S_CONFIRM, S_INTRO, S_SYSINFO, S_SEARCH, S_HOWTO, S_LANDING, S_CALM,
    S_PATH, S_UNLOCK, S_LOCKED, S_PIP, S_CHECKIN, S_PICK, S_PRAYER, S_STORY, S_LEAF, S_TILES;

static Mode mode = M_CAROUSEL;
static uint8_t carousel = 0;
static int reflectPage = 0, reflectPages = 1;
static int friendSel = -1, menuSel = -1;
static int cardSel = 0;  // friend card: 0 Back, 1 Delete, 2 Their story (only when they shared one)
static bool sessionDirty = false;  // a partial refresh happened since the last full one

// sorted view of friends: soonest anniversary first
static int order[MAX_FRIENDS];
static int32_t orderToday = -1;

// rock-paper-scissors / friend-added pop-ups
static Friend popupFriend;
static uint8_t rpsMine, rpsTheirs;

// Add friend (enter + up held): the radio looks for another Sobergotchi while the search screen is up
static bool searching = false;    // the search screen is showing, the buttons are held and the radio is on

// The calm screen (down held): the circle's size (0 small .. 4 full) and the line under it
static int calmFrame = 0;
static const char* calmSay = "";

// ---------------------------------------------------------------------------
// editors
// ---------------------------------------------------------------------------
enum EditTarget : uint8_t { E_SOBER, E_TODAY, E_NAME, E_PHONE, E_WORD, E_THANK };  // E_WORD / E_THANK: write your own
static EditTarget editTarget;
static int editField;
static int editY, editM, editD, editH;
static char editBuf[OWN_LEN + 2];  // OWN_LEN is the longest (name 12, phone 15)
static int editChoice = -1;  // date editor: -1 editing; else the Save / Cancel option highlighted
static bool confirmYes = false;  // on an "Are you sure?" question: Yes highlighted (starts on Cancel)
enum ConfirmWhat : uint8_t { C_SOBER, C_RESET, C_DELETE };
static ConfirmWhat confirmWhat = C_SOBER;
static bool confirmAgain = false;  // factory reset: the second, "really?" question

// Pip's first-time setup: a page per step; the name, sober date and phone pages open the usual editors.
enum IntroStep : uint8_t { I_HELLO, I_BUTTONS, I_NAME, I_SOBER, I_FRIENDS, I_PHONE, I_DONE, I_COUNT };
static int introStep = I_HELLO;
static bool introBack = false;  // setup: the < Back button highlighted (else Next)
static uint32_t introStartMs = 0;  // millis() when a key on the landing page started setup
static bool inIntro() { return !store::settings.setupDone; }

// Name / phone keyboard: up/down step through every key, then the actions under them (wrapping round); enter types
// the highlighted key or does the action. '^' is the Aa key (flips the case of the next letter).
static const char* const NAME_ROWS[] = {"ABCDE", "FGHIJ", "KLMNO", "PQRST", "UVWXY", "Z.-'^"};
static const char* const PHONE_ROWS[] = {"12345", "67890"};
enum KbAction : uint8_t { A_SPACE, A_DELETE, A_SAVE, A_CANCEL };
static const char* const KB_ACTION_LABELS[] = {"Space", "Delete", "Save", "Cancel"};
static int kbSel = 0;    // highlighted key, counted across the rows; past the last key, the actions
static bool kbShift = false;

static int editMax() { return editTarget == E_NAME ? NAME_LEN : editTarget == E_PHONE ? PHONE_LEN : OWN_LEN; }
static bool kbName() { return editTarget != E_PHONE; }  // letters: name and write-your-own
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
  // names: a capital at each word; write-your-own: only the first letter (Aa for more)
  bool upper = len == 0 || (editTarget == E_NAME && (editBuf[len - 1] == ' ' || editBuf[len - 1] == '-'));
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

// ---------------------------------------------------------------------------
// Your path: things that open as the days with Pip go by (counted from the day setup was finished, not the sober
// date, so every new owner has something to look forward to). The calm screen is there from day one.
// ---------------------------------------------------------------------------
enum PathItem : uint8_t { U_CALM, U_CHECKIN, U_LEAF, U_THANKS, U_WORD, U_TILES, U_STEP3, U_STORY, U_STEP7, U_STEP11, U_COUNT };
struct PathInfo { const char* name; uint16_t at; const char* text; };
static const PathInfo PATH_ITEMS[U_COUNT] = {
  {"Calm breathing", 0, "Hold down on any screen to breathe with Pip."},
  {"Daily check-in", 7, "Each day, choose one thing to do just for today."},
  {"Leaf catch", 30, "A little game. Catch the leaves as they fall."},
  {"Gratitude jar", 60, "Each night, put three thank-yous in the jar."},
  {"Your word", 90, "Choose your own word for a power greater than you."},
  {"Tree tiles", 180, "Slide the pieces to put the tree back together."},
  {"Third Step prayer", 270, "A prayer for turning it over, here when you want it."},
  {"Share your story", 365, "Pick three lines to tell your story to friends."},
  {"Seventh Step prayer", 365, "A prayer for letting go, here when you want it."},
  {"Eleventh Step prayer", 365, "A prayer for peace, here when you want it."},
};

static int32_t pipDays() {  // whole days since setup was finished
  const int32_t d = today() - (int32_t)store::settings.setupDone + PATH_TEST_DAYS;
  return d < 0 ? 0 : d;
}
static bool isOpen(int i) { return pipDays() >= PATH_ITEMS[i].at; }
static int pendingUnlock() {  // the first opened item whose "Something new!" hasn't been shown, or -1
  for (int i = 0; i < U_COUNT; i++)
    if (PATH_ITEMS[i].at > 0 && isOpen(i) && !(store::path.seen & (1u << i))) return i;
  return -1;
}
static void formatWhen(int at, char* out, size_t n) {  // "30 days with Pip", "6 months with Pip", "1 year with Pip"
  if (at >= 365) snprintf(out, n, "%d year%s with Pip", at / 365, at >= 730 ? "s" : "");
  else if (at >= 180) snprintf(out, n, "%d months with Pip", (at + 15) / 30);
  else snprintf(out, n, "%d days with Pip", at);
}
static void formatOpensIn(int32_t days, char* out, size_t n) {
  if (days <= 1) snprintf(out, n, "opens tomorrow");
  else if (days <= 60) snprintf(out, n, "opens in %ld days", (long)days);
  else snprintf(out, n, "opens in %ld months", (long)((days * 10 + 152) / 304));
}

static int pathSel = -1;      // Your path row (U_COUNT = < Back)
static int unlockItem = 0;    // the item Pip is celebrating
static bool unlockTry = true; // "Try it now" highlighted (else Later)
static int lockItem = 0;      // the locked item asked about
static const char* popTitle = "";  // Pip's note after something is done
static char popText[120];
static int popPose = 9;

// Daily check-in
static const char* const CHECKIN[5] = {"Stay sober", "Call a friend", "Go to a meeting", "Rest and be kind", "Help someone"};
static const char* const CHECKIN_SAY[5] = {
  "One day at a time. I'll be right here with you.",
  "Reaching out takes strength. Who will you call?",
  "Keep coming back. It works if you work it.",
  "Be gentle with yourself today. You're doing well.",
  "We keep what we have by giving it away.",
};
static int ciSel = 0;  // 0-4 the choices, 5 < Back

// Lists to tick (M_PICK)
enum PickKind : uint8_t { K_THANKS, K_WORD, K_STORY };
static const char* const THANKS[12] = {"Being sober today", "A friend", "My family", "A meeting", "My health", "A good meal",
                                       "Fresh air", "A good laugh", "A good sleep", "A second chance", "My sponsor",
                                       "This moment"};
static const char* const WORDS[7] = {"God", "Higher Power", "the Universe", "Spirit", "my group", "Love", "none for now"};
static const uint8_t WORD_NONE = 7;  // 1-based "none for now"
static const uint8_t WORD_OWN = 8;   // 1-based: their own word (store::path.ownWord)
static const int THANK_OWN = 12;     // Gratitude jar row (and bit) for their own thank-you
static const char* const STORY[3][6] = {
  {"I drank to get by.", "I used to escape.", "I felt so alone.", "I lost my way.", "I hid how I felt.", "I couldn't stop."},
  {"I asked for help.", "A friend reached out.", "I found the rooms.", "I hit my bottom.", "I got honest.", "I gave it a try."},
  {"Today I'm grateful.", "Today I have hope.", "I take it day by day.", "I help others now.", "I found some peace.",
   "I came back to life."},
};
static const char* const STORY_ASK[3] = {"What it was like:", "What happened:", "What it's like now:"};
static PickKind pickKind = K_THANKS;
static int pickSel = 0;
static uint16_t pickBits = 0;   // Gratitude jar: ticked rows
static int storyStep = 0;       // Share your story: which line is being picked
static uint8_t storyTmp[3];

// Step prayers, built with Your word when one is chosen
static char prayerBuf[960];
static const char* prayerTitle = "";
static int prayerPage = 0, prayerPages = 1;

// A story on screen
static bool svOwn = true, svBack = false;
static uint8_t svLines[3];
static char svName[NAME_LEN + 1];

// Tree tiles: board[cell] = tile (8 = the gap); tSel picks among the tiles next to the gap, then < Back
static uint8_t board[9];
static int tSel = 0, tMoves = 0;

// Leaf catch
static int leafBasket = 1, leafCaught = 0, leafFallen = 0;
static int8_t leafLane[3], leafRow[3];  // up to three leaves falling (row -1 = none)
static const char* leafSay = "";
static bool leafPlaying = false;

static int pickCount() {
  if (pickKind == K_THANKS) return 12 + 3;  // + Write my own, Put in the jar, < Back
  if (pickKind == K_WORD) return 7 + 2;     // + Write my own, < Back
  return 6 + 1;
}

// Tree tiles: the cells next to the gap, in order; option i of them, or -1 for < Back
static int tileOptions(int* cells) {
  int gap = 0, k = 0;
  for (int i = 0; i < 9; i++) if (board[i] == 8) gap = i;
  for (int i = 0; i < 9; i++) {
    const bool next = (i / 3 == gap / 3 && abs(i - gap) == 1) || (i % 3 == gap % 3 && abs(i - gap) == 3);
    if (next) cells[k++] = i;
  }
  return k;
}
static int tileOption(int sel) {
  int cells[4];
  const int k = tileOptions(cells);
  return sel >= 0 && sel < k ? cells[sel] : -1;
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
static const char* MENU_LABELS[] = {"Sober date", "Today", "Name", "Phone", "User settings", "System info", "Factory reset", "< Back"};
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
    if (!strcmp(name, "friends")) return store::friendCount + 2;  // with no friends: just Add friend and Back
    if (!strcmp(name, "menu")) return menuCount();
    if (!strcmp(name, "path")) return U_COUNT + 1;  // + < Back
    if (!strcmp(name, "pick")) return pickCount();
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
    if (!strcmp(key, "calm.frame")) return num(calmFrame);
    if (!strcmp(key, "calm.say")) return str(calmSay);

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
      return str("Looking...");
    }
    if (!strcmp(key, "search.text")) {
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
    if (!strcmp(key, "use.days")) return num(pipDays() + 1);
    if (!strcmp(key, "path.sel")) return num(mode == M_PATH ? pathSel : -1);
    if (!strncmp(key, "unlock.", 7)) {
      const char* k = key + 7;
      const PathInfo& p = PATH_ITEMS[unlockItem];
      if (!strcmp(k, "when")) return formatWhen(p.at, out, n);
      if (!strcmp(k, "name")) return str(p.name);
      if (!strcmp(k, "text")) return str(p.text);
      if (!strcmp(k, "try")) return flag(unlockTry);
      return;
    }
    if (!strcmp(key, "lock.name")) return str(PATH_ITEMS[lockItem].name);
    if (!strcmp(key, "lock.text")) {
      const int32_t left = PATH_ITEMS[lockItem].at - pipDays();
      char date[16];
      formatDate(t + left, "MMM d", date, sizeof date);
      snprintf(out, n, "Pip is still getting this ready. It opens in %ld day%s, on %s.", (long)left, left == 1 ? "" : "s", date);
      return;
    }
    if (!strcmp(key, "pop.title")) return str(popTitle);
    if (!strcmp(key, "pop.text")) return str(popText);
    if (!strcmp(key, "pop.pose")) return num(popPose);
    if (!strncmp(key, "ci.", 3)) {
      const char* k = key + 3;
      if (k[0] == 'l') return str(CHECKIN[(k[1] - '0') % 5]);
      if (k[0] == 's') return flag(ciSel == k[1] - '0');
      if (!strcmp(k, "done")) {
        if (store::path.checkDay == t) snprintf(out, n, "Today: %s", CHECKIN[store::path.checkPick % 5]);
        else str("pick one");
        return;
      }
      return;
    }
    if (!strncmp(key, "pick.", 5)) {
      const char* k = key + 5;
      if (!strcmp(k, "sel")) return num(mode == M_PICK ? pickSel : -1);
      if (!strcmp(k, "title")) return str(pickKind == K_THANKS ? "Gratitude jar" : pickKind == K_WORD ? "Your word" : "Your story");
      if (!strcmp(k, "right")) {
        if (pickKind == K_THANKS) snprintf(out, n, "%u in jar", (unsigned)store::path.thanks);
        else if (pickKind == K_STORY) snprintf(out, n, "%d of 3", storyStep + 1);
        return;
      }
      if (!strcmp(k, "text")) {
        if (pickKind == K_THANKS) return str("Tick up to three things you're thankful for.");
        if (pickKind == K_WORD) return str("What should I call a higher power?");
        return str(STORY_ASK[storyStep]);
      }
      return;
    }
    if (!strncmp(key, "prayer.", 7)) {
      const char* k = key + 7;
      if (!strcmp(k, "title")) return str(prayerTitle);
      if (!strcmp(k, "page")) return num(prayerPage);
      if (!strcmp(k, "more")) { if (prayerPages > 1) snprintf(out, n, "%d/%d", prayerPage + 1, prayerPages); return; }
      if (!strcmp(k, "btn")) return str(prayerPage < prayerPages - 1 ? "More >" : "< Back");
      if (k[0] >= 'a' && k[0] <= 'f' && !k[1]) {  // the prayer in slices that fit a value (150 characters each)
        const size_t at = (k[0] - 'a') * 150, len = strlen(prayerBuf);
        if (at < len) snprintf(out, n, "%.150s", prayerBuf + at);
      }
      return;
    }
    if (!strncmp(key, "sv.", 3)) {
      const char* k = key + 3;
      if (!strcmp(k, "title")) { if (svOwn) str("Your story"); else snprintf(out, n, "%s's story", svName); return; }
      if (k[0] >= 'a' && k[0] <= 'c' && !k[1]) {
        const int line = k[0] - 'a', v = svLines[line];
        if (v >= 1 && v <= 6) str(STORY[line][v - 1]);
        return;
      }
      if (!strcmp(k, "own")) return flag(svOwn);
      if (!strcmp(k, "back")) return flag(svBack);
      return;
    }
    if (!strncmp(key, "tl.", 3)) {
      const char* k = key + 3;
      if (k[0] == 'v') return num(board[(k[1] - '0') % 9]);
      if (k[0] == 's') return flag(tileOption(tSel) == k[1] - '0');
      if (!strcmp(k, "back")) return flag(tileOption(tSel) < 0);
      if (!strcmp(k, "moves")) { snprintf(out, n, "%d move%s", tMoves, tMoves == 1 ? "" : "s"); return; }
      return;
    }
    if (!strncmp(key, "lf.", 3)) {
      const char* k = key + 3;
      if (k[0] == 'c') {
        const int r = k[1] - '0', l = k[2] - '0';
        for (int i = 0; i < 3; i++) if (leafRow[i] == r && leafLane[i] == l) return flag(true);
        return;
      }
      if (k[0] == 'b') return flag(leafBasket == k[1] - '0');
      if (!strcmp(k, "score")) { snprintf(out, n, "%d caught", leafCaught); return; }
      if (!strcmp(k, "say")) return str(leafSay);
      return;
    }

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
      if (!strcmp(k, "del")) return flag(cardSel == 1);
      if (!strcmp(k, "s0")) return flag(cardSel == 0);
      if (!strcmp(k, "s2")) return flag(cardSel == 2);
      if (!strcmp(k, "story")) return flag(f.story[0] && cardSel != 2);
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
    static const char* const TITLE[I_COUNT] = {"Hello!", "Your buttons", "Your name", "Sobriety date", "Friends", "Your phone", "All set"};
    static const uint8_t POSE[I_COUNT] = {P_WAVE, P_WAVE, P_STAND, P_READ, P_TEA, P_LANTERN, P_CHEER};
    const char* name = store::settings.name;
    if (!strcmp(k, "title")) { snprintf(out, n, "%s", TITLE[introStep]); return; }
    if (!strcmp(k, "hasback")) { snprintf(out, n, "%s", introStep > I_HELLO ? "1" : ""); return; }
    if (!strcmp(k, "bsel")) { snprintf(out, n, "%s", introBack ? "1" : ""); return; }
    if (!strcmp(k, "next")) { snprintf(out, n, "%s", introStep == I_DONE ? "Begin >" : "Next >"); return; }
    if (!strcmp(k, "buttons")) { snprintf(out, n, "%s", introStep == I_BUTTONS ? "1" : ""); return; }
    if (!strcmp(k, "pose")) { snprintf(out, n, "%d", POSE[introStep]); return; }
    if (!strcmp(k, "step")) { snprintf(out, n, "%d/%d", introStep + 1, (int)I_COUNT); return; }
    if (strcmp(k, "text")) return;
    switch (introStep) {
      case I_HELLO:
        snprintf(out, n, "Hello there, friend. I'm Pip. I look after the little garden in here, and I'd love to keep "
                         "you company, one day at a time.");
        break;
      case I_BUTTONS:
        snprintf(out, n, "Up and down move around, and enter picks. Hold enter and up together to make a friend.");
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
    if (list && !strcmp(list, "path")) return pathValue(k, out, n);
    if (list && !strcmp(list, "pick")) return pickValue(k, out, n);
    if (item == 0 || item == store::friendCount + 1) {  // the "Add friend" and "Back" rows
      if (!strcmp(k, "name")) snprintf(out, n, "%s", item ? "Back" : "Add friend");
      else if (!strcmp(k, "back")) snprintf(out, n, "1");  // plain row: no date badge
      else if (!strcmp(k, "add")) snprintf(out, n, "%s", item ? "" : "1");
      else if (!strcmp(k, "ret")) snprintf(out, n, "%s", item ? "1" : "");  // Back: a "<" badge
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

  // Your path rows: item.name, item.sub, item.locked / item.open (padlock), item.back
  void pathValue(const char* k, char* out, size_t n) {
    auto flag = [&](bool b) { snprintf(out, n, "%s", b ? "1" : ""); };
    if (item == U_COUNT) {
      if (!strcmp(k, "name")) snprintf(out, n, "Back");
      else if (!strcmp(k, "back")) flag(true);
      return;
    }
    if (item < 0 || item > U_COUNT) return;
    const bool open = isOpen(item);
    if (!strcmp(k, "name")) { snprintf(out, n, "%s", PATH_ITEMS[item].name); return; }
    if (!strcmp(k, "locked")) return flag(!open);
    if (!strcmp(k, "open")) return flag(open);
    if (strcmp(k, "sub")) return;
    if (!open) return formatOpensIn(PATH_ITEMS[item].at - pipDays(), out, n);
    const Path& p = store::path;
    const int32_t t = today();
    switch (item) {
      case U_CALM: snprintf(out, n, "hold down anytime"); return;
      case U_CHECKIN: snprintf(out, n, "%s", p.checkDay == t ? "done today" : "open"); return;
      case U_LEAF: if (p.leafBest) snprintf(out, n, "best %d of %d", p.leafBest, LEAF_COUNT); else snprintf(out, n, "open"); return;
      case U_THANKS: snprintf(out, n, "%u in the jar", (unsigned)p.thanks); return;
      case U_WORD: snprintf(out, n, "%s", p.word == WORD_OWN ? p.ownWord : p.word ? WORDS[p.word - 1] : "open"); return;
      case U_TILES: if (p.tilesBest) snprintf(out, n, "best %u moves", (unsigned)p.tilesBest); else snprintf(out, n, "open"); return;
      case U_STORY: snprintf(out, n, "%s", p.story[0] ? "shared with friends" : "open"); return;
      default: snprintf(out, n, "open"); return;
    }
  }

  // Pick rows: item.label, item.box (empty tick box), item.on (ticked), item.back, item.done
  void pickValue(const char* k, char* out, size_t n) {
    auto flag = [&](bool b) { snprintf(out, n, "%s", b ? "1" : ""); };
    const int count = pickCount();
    if (item < 0 || item >= count) return;
    const bool back = item == count - 1, done = pickKind == K_THANKS && item == count - 2;
    const bool on = !back && !done &&
                    (pickKind == K_THANKS ? (pickBits >> item) & 1
                     : pickKind == K_WORD ? store::path.word == item + 1
                                          : store::path.story[storyStep] == item + 1);
    const bool own = (pickKind == K_THANKS && item == THANK_OWN) || (pickKind == K_WORD && item == WORD_OWN - 1);
    if (!strcmp(k, "label")) {
      if (back) snprintf(out, n, "%s", pickKind == K_STORY && storyStep ? "Back a step" : "Back");
      else if (done) snprintf(out, n, "Put in the jar");
      else if (own) {
        const char* mine = pickKind == K_THANKS ? store::path.ownThank : store::path.ownWord;
        snprintf(out, n, "%s", mine[0] ? mine : "Write my own...");
      } else snprintf(out, n, "%s", pickKind == K_THANKS ? THANKS[item] : pickKind == K_WORD ? WORDS[item] : STORY[storyStep][item]);
      return;
    }
    if (!strcmp(k, "back")) return flag(back);
    if (!strcmp(k, "done")) return flag(done);
    if (!strcmp(k, "on")) return flag(on);
    if (!strcmp(k, "box")) return flag(!back && !done && !on && pickKind != K_STORY);
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
      static const char* T[] = {"Sobriety date", "Today", "Your name", "Your phone", "Your word", "Thankful for"};
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
    if (!strcmp(k, "choose")) { snprintf(out, n, "%s", editChoice >= 0 ? "1" : ""); return; }
    if (!strcmp(k, "csel")) { snprintf(out, n, "%s", editChoice == 1 ? "1" : ""); return; }
    if (!strcmp(k, "has3")) { snprintf(out, n, "%s", editTarget == E_TODAY ? "1" : ""); return; }
    if (!strncmp(k, "hint", 4)) {
      const char* h = "";
      if (editChoice >= 0) {  // Save / Cancel are drawn as buttons
        if (k[4] == '2') h = "\202 \203 choose  \177  \201 ok";
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
    if (leafPlaying) { snprintf(out, n, "\202 \203 move  \177  \201 stop"); return; }
    switch (mode) {
      case M_HOWTO: snprintf(out, n, "\201 back"); break;
      case M_FRIENDS: snprintf(out, n, "\202 \203 move  \177  \201 %s", friendSel == 0 ? "add" : "open"); break;
      case M_MENU: snprintf(out, n, "\202 \203 move  \177  \201 change"); break;
      case M_CONFIRM: snprintf(out, n, "\202 \203 choose  \177  \201 ok"); break;
      case M_EDIT_TEXT: snprintf(out, n, "\202 \203 move  \177  \201 pick"); break;
      case M_PATH: snprintf(out, n, "\202 \203 move  \177  \201 open"); break;
      case M_PICK: snprintf(out, n, "\202 \203 move  \177  \201 %s", pickKind == K_THANKS ? "tick" : "pick"); break;
      case M_TILES: snprintf(out, n, "\202 \203 choose  \177  \201 slide"); break;
      default:
        if (carousel == 3) snprintf(out, n, store::friendCount ? "\201 choose a friend" : "\201 add a friend");
        else if (carousel == C_PATH) snprintf(out, n, "\201 choose");
        else if (carousel == C_MENU) snprintf(out, n, "\201 choose a setting");
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
    case M_LANDING: return S_LANDING;
    case M_INTRO: return S_INTRO;
    case M_SYSINFO: return S_SYSINFO;
    case M_HOWTO: return S_HOWTO;
    case M_EDIT_DATE: return S_EDIT_DATE;
    case M_EDIT_TEXT: return S_EDIT_TEXT;
    case M_FRIENDS: return S_FRIENDS;
    case M_MENU: return S_MENU;
    case M_PATH: return S_PATH;
    case M_UNLOCK: return S_UNLOCK;
    case M_CHECKIN: return S_CHECKIN;
    case M_PICK: return S_PICK;
    case M_PRAYER: return S_PRAYER;
    case M_STORY: return S_STORY;
    case M_TILES: return S_TILES;
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
    case C_PATH: return S_PATH;
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
  if (id == S_PRAYER && reflectPages != prayerPages) { prayerPages = reflectPages; draw(id, false); }
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
  const char* was = target == E_NAME ? store::settings.name : target == E_PHONE ? store::settings.phone
                  : target == E_WORD ? store::path.ownWord : store::path.ownThank;
  if (!inIntro()) strncpy(editBuf, was, editMax());
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
  if (editTarget == E_WORD || editTarget == E_THANK) {  // write your own: kept on Your path
    if (!len) return false;
    strncpy(editTarget == E_WORD ? store::path.ownWord : store::path.ownThank, editBuf, OWN_LEN + 1);
    store::savePath();
    return true;
  }
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
static void wordChosen(int w);
static void editClosed(bool saved) {
  if (editTarget == E_WORD || editTarget == E_THANK) {  // back to the list (a typed word is chosen straight away)
    mode = M_PICK;
    if (saved && editTarget == E_WORD) wordChosen(WORD_OWN);
    else if (saved) pickBits |= 1u << THANK_OWN;
    return;
  }
  if (!inIntro()) { mode = M_MENU; return; }
  mode = M_INTRO;
  if (saved || editTarget == E_PHONE) introStep++;  // the phone may be skipped with Cancel
}

// Fresh flash, factory reset, or setup left unfinished too long: the landing page, setup from the start.
static void startIntro() {
  introStep = I_HELLO;
  introBack = false;
  mode = M_LANDING;
}

// Any key on the landing page: Pip's first page, and the SETUP_TIMEOUT_MS clock starts.
static void beginSetup() {
  introStep = I_HELLO;
  introBack = false;
  introStartMs = millis();
  mode = M_INTRO;
}

static void finishIntro() {
  store::settings.setupDone = (uint32_t)today();  // Your path counts days with Pip from here
  store::path.seen = 0;
  store::settings.clockSecs = hw::now();
  store::saveSettings();
  sortFriends();
  mode = M_CAROUSEL;
}

// ---------------------------------------------------------------------------
// Your path
// ---------------------------------------------------------------------------
static void pipNote(const char* title, int pose) {  // Pip's note (popText filled first), then back to Your path
  popTitle = title;
  popPose = pose;
  showPopup(S_PIP);
  mode = M_PATH;
}

static void buildPrayer(int which) {
  const Prayer& p = PRAYERS[which];
  const uint8_t w = store::path.word;
  char word[24];
  snprintf(word, sizeof word, "%s", w >= 1 && w < WORD_NONE ? WORDS[w - 1]
                                    : w == WORD_OWN && store::path.ownWord[0] ? store::path.ownWord : p.word);
  int len = 0;
  bool start = true;  // capitalise the word at the start of a sentence ("the Universe" -> "The Universe")
  for (const char* c = p.text; *c && len < (int)sizeof prayerBuf - 1;) {
    if (!strncmp(c, "{word}", 6)) {
      for (int i = 0; word[i] && len < (int)sizeof prayerBuf - 1; i++)
        prayerBuf[len++] = (i == 0 && start && word[0] >= 'a' && word[0] <= 'z') ? word[0] - 32 : word[i];
      c += 6;
      start = false;
      continue;
    }
    if (*c == '.' || *c == '!' || *c == '?') start = true;
    else if (*c != ' ') start = false;
    prayerBuf[len++] = *c++;
  }
  prayerBuf[len] = 0;
  prayerTitle = p.title;
  prayerPage = 0;
  prayerPages = 1;
}

static void tilesStart() {
  for (int i = 0; i < 9; i++) board[i] = i;
  uint32_t h = dayHash((int32_t)millis(), 23);
  int last = -1;
  for (int m = 0; m < 80 || board[0] == 0; m++) {  // random slides from the finished picture: always solvable
    int cells[4];
    const int k = tileOptions(cells);
    h = dayHash((int32_t)h, m);
    int c = cells[h % k];
    if (c == last) c = cells[(h / 7 + 1) % k];
    int gap = 0;
    for (int i = 0; i < 9; i++) if (board[i] == 8) gap = i;
    board[gap] = board[c];
    board[c] = 8;
    last = gap;
  }
  tSel = 0;
  tMoves = 0;
  mode = M_TILES;
}

static void tilesSlide() {
  const int c = tileOption(tSel);
  if (c < 0) { mode = M_PATH; return; }  // < Back
  for (int i = 0; i < 9; i++)
    if (board[i] == 8) { board[i] = board[c]; board[c] = 8; break; }
  tMoves++;
  tSel = 0;
  for (int i = 0; i < 9; i++) if (board[i] != i) return;
  store::path.tilesDone++;
  if (!store::path.tilesBest || tMoves < store::path.tilesBest) store::path.tilesBest = tMoves;
  store::savePath();
  snprintf(popText, sizeof popText, "The tree is whole again, in %d moves. Your best is %u.", tMoves,
           (unsigned)store::path.tilesBest);
  draw(S_TILES, false);
  delay(1500);
  pipNote("Solved!", 9);
}

// Leaf catch: leaves fall down three lanes, one row every LEAF_STEP_MS; up / down move the basket, enter stops.
static void leafDraw() { draw(S_LEAF, false); sessionDirty = true; }

static void leafGame() {
  LOG("leaf catch");
  leafBasket = 1; leafCaught = 0; leafFallen = 0; leafSay = "here they come";
  leafPlaying = true;
  for (int i = 0; i < 3; i++) leafRow[i] = -1;
  int spawned = 0, tick = 0;
  uint32_t h = dayHash((int32_t)millis(), 31);
  leafDraw();
  uint32_t next = millis() + LEAF_STEP_MS;
  bool stop = false;
  while (!stop) {
    const int32_t left = (int32_t)(next - millis());
    if (left > 0) {
      const Event e = hw::wait((uint32_t)left);
      if (e == EV_KEY2 || e == EV_KEY2_LONG) stop = true;
      else if (e == EV_KEY1 && leafBasket > 0) { leafBasket--; leafDraw(); }
      else if (e == EV_KEY3 && leafBasket < 2) { leafBasket++; leafDraw(); }
      else if (e == EV_ADD_FRIEND) { while (hw::comboHeld()) delay(20); hw::flushKeys(); }
      else if (e == EV_CALM) { while (hw::downHeld()) delay(20); hw::flushKeys(); }
      continue;
    }
    // one step: everything falls a row; a leaf reaching the basket row is caught or missed
    tick++;
    bool any = false;
    for (int i = 0; i < 3; i++) {
      if (leafRow[i] < 0) continue;
      if (++leafRow[i] >= 5) {
        leafFallen++;
        if (leafLane[i] == leafBasket) { leafCaught++; leafSay = "caught!"; } else leafSay = "missed";
        leafRow[i] = -1;
      } else any = true;
    }
    if (spawned < LEAF_COUNT && tick % 2 == 1) {
      for (int i = 0; i < 3; i++)
        if (leafRow[i] < 0) {
          h = dayHash((int32_t)h, tick);
          leafLane[i] = h % 3;
          leafRow[i] = 0;
          spawned++;
          any = true;
          break;
        }
    }
    if (!any && spawned >= LEAF_COUNT) break;
    leafDraw();
    next += LEAF_STEP_MS;
    if ((int32_t)(millis() - next) > 0) next = millis() + LEAF_STEP_MS;  // slow refresh: don't rush
  }
  if (leafCaught > store::path.leafBest) store::path.leafBest = leafCaught;
  store::savePath();
  snprintf(popText, sizeof popText, "You caught %d of %d leaves. Your best is %d.", leafCaught, leafFallen,
           store::path.leafBest);
  leafPlaying = false;
  pipNote(stop ? "Good game" : "Well done!", 9);
}

static void showStory(bool own, const uint8_t* lines, const char* name) {
  svOwn = own;
  svBack = false;
  memcpy(svLines, lines, 3);
  snprintf(svName, sizeof svName, "%s", name ? name : "");
  mode = M_STORY;
}

static void startPick(PickKind kind) {
  pickKind = kind;
  pickSel = 0;
  if (kind == K_THANKS) pickBits = store::path.thankDay == today() ? store::path.thankBits : 0;
  if (kind == K_WORD && store::path.word) pickSel = store::path.word - 1;
  if (kind == K_STORY) { storyStep = 0; memcpy(storyTmp, store::path.story, 3); }
  mode = M_PICK;
}

static void calm();

// Opens a Your path item (from its row or from "Try it now").
static void openItem(int i) {
  pathSel = i;
  mode = M_PATH;
  switch (i) {
    case U_CALM: calm(); break;
    case U_CHECKIN:
      mode = M_CHECKIN;
      ciSel = store::path.checkDay == today() ? store::path.checkPick : 0;
      break;
    case U_LEAF: leafGame(); break;
    case U_THANKS: startPick(K_THANKS); break;
    case U_WORD: startPick(K_WORD); break;
    case U_TILES: tilesStart(); break;
    case U_STEP3: buildPrayer(0); mode = M_PRAYER; break;
    case U_STEP7: buildPrayer(1); mode = M_PRAYER; break;
    case U_STEP11: buildPrayer(2); mode = M_PRAYER; break;
    case U_STORY:
      if (store::path.story[0]) showStory(true, store::path.story, nullptr);
      else startPick(K_STORY);
      break;
  }
}

static int thanksTicked() {
  int on = 0;
  for (int i = 0; i <= THANK_OWN; i++) if (pickBits >> i & 1) on++;
  return on;
}

static void wordChosen(int w) {  // 1-based; WORD_OWN = the word they typed
  store::path.word = w;
  store::path.seen |= 1u << U_WORD;
  store::savePath();
  if (w == WORD_NONE)
    snprintf(popText, sizeof popText, "That's fine. The prayers stay as they are written, ready when you are.");
  else
    snprintf(popText, sizeof popText, "I'll use \"%s\" in the prayers from now on.",
             w == WORD_OWN ? store::path.ownWord : WORDS[w - 1]);
  pipNote("Your word", 7);
}

static void pickEnter() {
  const int count = pickCount();
  if (pickSel == count - 1) {  // < Back
    if (pickKind == K_STORY && storyStep > 0) { storyStep--; pickSel = storyTmp[storyStep] ? storyTmp[storyStep] - 1 : 0; return; }
    mode = M_PATH;
    return;
  }
  if (pickKind == K_THANKS) {
    if (pickSel == count - 2) {  // Put in the jar
      if (!pickBits) { mode = M_PATH; return; }
      const int32_t t = today();
      const uint16_t before = store::path.thankDay == t ? store::path.thankBits : 0;
      int added = 0;
      for (int i = 0; i <= THANK_OWN; i++) if ((pickBits & ~before) >> i & 1) added++;
      store::path.thanks += added;
      store::path.thankBits = pickBits | before;
      store::path.thankDay = t;
      store::savePath();
      snprintf(popText, sizeof popText, "%u thank-yous in your jar now. Gratitude keeps the garden growing.",
               (unsigned)store::path.thanks);
      pipNote("Thank you", 5);
      return;
    }
    const uint16_t bit = 1u << pickSel;
    if (pickBits & bit) pickBits &= ~bit;
    else if (thanksTicked() < 3) {
      if (pickSel == THANK_OWN) startEditText(E_THANK);  // type it; ticked when saved
      else pickBits |= bit;
    }
    return;
  }
  if (pickKind == K_WORD) {
    if (pickSel == WORD_OWN - 1) startEditText(E_WORD);
    else wordChosen(pickSel + 1);
    return;
  }
  // Share your story: a line per page
  storyTmp[storyStep] = pickSel + 1;
  if (++storyStep < 3) { pickSel = storyTmp[storyStep] ? storyTmp[storyStep] - 1 : 0; return; }
  memcpy(store::path.story, storyTmp, 3);
  store::savePath();
  snprintf(popText, sizeof popText, "Friends you meet will see your story, and you'll see theirs.");
  pipNote("Thank you", 9);
  showStory(true, store::path.story, nullptr);
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
  draw(S_SEARCH, false);
  sessionDirty = true;
  exchange::start(store::settings, store::path.story, ADD_FRIEND_MIN_RSSI);
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
  if (!r.found) return;  // nobody: back to whatever was on screen, no message
  LOG("add friend: found %s", r.card.name);
  popupFriend = r.card;
  rpsMine = r.myThrow;
  rpsTheirs = r.theirThrow;
  const bool isNew = store::upsertFriend(r.card);  // no question: starting Add friend says you want to
  sortFriends();
  showPopup(isNew ? S_ADDED : S_RPS);
  friendsBack(r.card.uid);
}

// ---------------------------------------------------------------------------
// The calm screen: down held for CALM_HOLD_MS on any screen after setup. The circle leads BREATHS slow breaths (in as it
// grows, out as it shrinks, one size every BREATH_STEP_MS), then rests with the prayer for CALM_STAY_MS. Only enter
// closes it (up / down only ever select, and there is nothing to choose here). Partial refreshes only: no flashing.
// ---------------------------------------------------------------------------
static void calmDraw(int frame, const char* say) {
  calmFrame = frame;
  calmSay = say;
  draw(S_CALM, false);
  sessionDirty = true;
}

// Waits until the given millis() time; true if enter was pressed (close). Other keys are let go of and ignored.
static bool calmWait(uint32_t until) {
  for (;;) {
    const int32_t left = (int32_t)(until - millis());
    if (left <= 0) return false;
    const Event e = hw::wait((uint32_t)left);
    if (e == EV_KEY2 || e == EV_KEY2_LONG) return true;
    if (e == EV_ADD_FRIEND) { while (hw::comboHeld()) delay(20); hw::flushKeys(); }
    if (e == EV_CALM) { while (hw::downHeld()) delay(20); hw::flushKeys(); }
  }
}

static void calm() {
  LOG("calm screen");
  calmDraw(0, "breathe in");
  while (hw::downHeld()) delay(20);  // letting go of down must not count as a press
  hw::flushKeys();
  uint32_t next = millis();
  for (int step = 1; step <= BREATHS * 8; step++) {  // 8 steps a breath: sizes 0 1 2 3 4 3 2 1, back to 0
    next += BREATH_STEP_MS;
    if (calmWait(next)) return;
    const int s = step % 8;
    calmDraw(s <= 4 ? s : 8 - s, s < 4 ? "breathe in" : "breathe out");
    if ((int32_t)(millis() - next) > (int32_t)BREATH_STEP_MS) next = millis();  // a slow refresh: don't rush to catch up
  }
  next += BREATH_STEP_MS;
  if (calmWait(next)) return;
  calmDraw(2, "one day at a time");
  calmWait(millis() + CALM_STAY_MS);
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
    case M_LANDING:
      if (e == EV_KEY1 || e == EV_KEY2 || e == EV_KEY3) beginSetup();  // any key
      break;

    case M_INTRO:
      // < Back and Next > buttons: up / down choose (no Back on the first page), enter presses
      if ((e == EV_KEY1 || e == EV_KEY3) && introStep > I_HELLO) introBack = !introBack;
      else if (e == EV_KEY2 && introBack) { introStep--; introBack = false; }
      else if (e == EV_KEY2) {
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
        else if (carousel == 3) { mode = M_FRIENDS; friendSel = store::friendCount ? 1 : 0; }  // first friend, or Add
        else if (carousel == C_PATH) { mode = M_PATH; pathSel = 0; }
        else if (carousel == C_MENU) { mode = M_MENU; menuUser = false; menuSel = 0; }
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
        cardSel = 0;  // Back highlighted
        mode = M_FRIEND_CARD;
      }
      break;

    case M_FRIEND_CARD:  // up / down choose Back or Delete; enter does it, and Delete asks first
      if (e == EV_KEY1 || e == EV_KEY3) {  // Back, Delete, Their story (when shared), round again
        const int count = popupFriend.story[0] ? 3 : 2;
        cardSel = (cardSel + (e == EV_KEY3 ? 1 : count - 1)) % count;
      } else if (e == EV_KEY2 && cardSel == 0) mode = M_FRIENDS;
      else if (e == EV_KEY2 && cardSel == 2) showStory(false, popupFriend.story, popupFriend.name);
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
            mode = M_FRIENDS;  // with none left, on Add friend
          } else {
            cardSel = 0;
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

    case M_PATH:  // rows: the items, then < Back
      if (e == EV_KEY1 && pathSel > 0) pathSel--;
      else if (e == EV_KEY3 && pathSel < U_COUNT) pathSel++;
      else if (e == EV_KEY2 && pathSel == U_COUNT) mode = M_CAROUSEL;
      else if (e == EV_KEY2 && !isOpen(pathSel)) { lockItem = pathSel; showPopup(S_LOCKED); }
      else if (e == EV_KEY2) openItem(pathSel);
      break;

    case M_UNLOCK:  // Try it now / Later
      if (e == EV_KEY1 || e == EV_KEY3) unlockTry = !unlockTry;
      else if (e == EV_KEY2) {
        store::path.seen |= 1u << unlockItem;
        store::savePath();
        if (unlockTry) { carousel = C_PATH; openItem(unlockItem); }
        else {
          mode = M_CAROUSEL;
          carousel = 0;
          const int u = pendingUnlock();  // two opened on the same day: the next one
          if (u >= 0) { mode = M_UNLOCK; unlockItem = u; unlockTry = true; }
        }
      }
      break;

    case M_CHECKIN:  // five choices, then < Back
      if (e == EV_KEY1) ciSel = (ciSel + 5) % 6;
      else if (e == EV_KEY3) ciSel = (ciSel + 1) % 6;
      else if (e == EV_KEY2 && ciSel == 5) mode = M_PATH;
      else if (e == EV_KEY2) {
        store::path.checkPick = ciSel;
        store::path.checkDay = today();
        store::savePath();
        snprintf(popText, sizeof popText, "%s", CHECKIN_SAY[ciSel]);
        pipNote("Just for today", 9);
      }
      break;

    case M_PICK:
      if (e == EV_KEY1) pickSel = (pickSel + pickCount() - 1) % pickCount();
      else if (e == EV_KEY3) pickSel = (pickSel + 1) % pickCount();
      else if (e == EV_KEY2) pickEnter();
      break;

    case M_PRAYER:  // enter turns the page, then goes back
      if (e == EV_KEY2) {
        if (prayerPage < prayerPages - 1) prayerPage++;
        else mode = M_PATH;
      }
      break;

    case M_STORY:
      if (svOwn && (e == EV_KEY1 || e == EV_KEY3)) svBack = !svBack;
      else if (e == EV_KEY2 && !svOwn) { cardSel = 2; mode = M_FRIEND_CARD; }
      else if (e == EV_KEY2 && svBack) mode = M_PATH;
      else if (e == EV_KEY2) startPick(K_STORY);
      break;

    case M_TILES: {
      int cells[4];
      const int k = tileOptions(cells) + 1;  // + < Back
      if (e == EV_KEY1) tSel = (tSel + k - 1) % k;
      else if (e == EV_KEY3) tSel = (tSel + 1) % k;
      else if (e == EV_KEY2) tilesSlide();
      break;
    }

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
  bool menuUser, confirmYes, confirmAgain, kbShift, searching, introBack;
  int friendCount, cardSel;
  int pathSel, unlockItem, ciSel, pickKind, pickSel, storyStep, prayerPage, tSel, tMoves;
  bool unlockTry, svBack, svOwn;
  uint16_t pickBits;
  uint8_t board[9];
  char editBuf[OWN_LEN + 2];
};

static void snapshot(UiState& u) {
  memset(&u, 0, sizeof u);  // zero the padding too, so memcmp sees only the fields
  u.draws = draws;
  u.mode = mode; u.carousel = carousel; u.reflectPage = reflectPage; u.friendSel = friendSel; u.menuSel = menuSel;
  u.introStep = introStep;
  u.editTarget = editTarget; u.editField = editField; u.editY = editY; u.editM = editM; u.editD = editD;
  u.editH = editH; u.editChoice = editChoice; u.kbSel = kbSel; u.confirmWhat = confirmWhat;
  u.cardSel = cardSel; u.menuUser = menuUser; u.confirmYes = confirmYes; u.confirmAgain = confirmAgain;
  u.kbShift = kbShift; u.searching = searching; u.introBack = introBack;
  u.friendCount = store::friendCount;
  u.pathSel = pathSel; u.unlockItem = unlockItem; u.ciSel = ciSel; u.pickKind = pickKind; u.pickSel = pickSel;
  u.storyStep = storyStep; u.prayerPage = prayerPage; u.tSel = tSel; u.tMoves = tMoves;
  u.unlockTry = unlockTry; u.svBack = svBack; u.svOwn = svOwn; u.pickBits = pickBits;
  memcpy(u.board, board, sizeof board);
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
  cardSel = 0;
  const int u = pendingUnlock();  // something opened on Your path: Pip says so instead of the home screen
  if (u >= 0) { mode = M_UNLOCK; unlockItem = u; unlockTry = true; }
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
  S_LANDING = ui::screenByName("landing");
  S_CALM = ui::screenByName("calm");
  S_PATH = ui::screenByName("journey");
  S_UNLOCK = ui::screenByName("unlock");
  S_LOCKED = ui::screenByName("locked");
  S_PIP = ui::screenByName("pip");
  S_CHECKIN = ui::screenByName("checkin");
  S_PICK = ui::screenByName("pick");
  S_PRAYER = ui::screenByName("prayer");
  S_STORY = ui::screenByName("story");
  S_LEAF = ui::screenByName("leaf");
  S_TILES = ui::screenByName("tiles");

  // Clock: the newest of the saved time and the firmware build time.
  uint32_t built = hw::buildTime();
  hw::setNow(store::settings.clockSecs > built ? store::settings.clockSecs : built);
  sortFriends();
  if (store::settings.setupDone == 1) {  // set up before v1.34: Your path counts from today
    store::settings.setupDone = (uint32_t)today();
    store::saveSettings();
  }
  if (inIntro()) {  // fresh flash: the landing page, then Pip says hello and asks for the name, sober date and phone
    startIntro();
    show(true);
    sessionDirty = false;
    display::sleep();
    return;
  }
  goHome(true);
}

void loop() {
  if (inIntro()) {  // setup waits for the buttons, no friend taps until Pip knows who you are
    // The landing page waits as long as it takes. Setup started from it gets SETUP_TIMEOUT_MS to finish,
    // then it is back to the landing page (the next person starts fresh).
    uint32_t timeout = IDLE_TIMEOUT_MS;
    if (mode != M_LANDING) {
      const uint32_t used = millis() - introStartMs;
      if (used >= SETUP_TIMEOUT_MS) {
        LOG("setup not finished in time: back to the landing page");
        startIntro();
        show(true);
        sessionDirty = false;
        display::sleep();
        return;
      }
      if (SETUP_TIMEOUT_MS - used < timeout) timeout = SETUP_TIMEOUT_MS - used;
    }
    Event e = hw::wait(timeout);
    if (e == EV_TIMEOUT) { display::sleep(); return; }
    if (e == EV_ADD_FRIEND) { while (hw::comboHeld()) delay(20); hw::flushKeys(); return; }  // not during setup
    if (e == EV_CALM) {  // no calm screen during setup: a long down is just down (on the landing page, any key)
      while (hw::downHeld()) delay(20);
      hw::flushKeys();
      e = EV_KEY3;
    }
    if (!keyChanged(e)) return;
    if (mode == M_CAROUSEL) goHome(true);  // setup finished: a clean full refresh onto the home screen
    else show(false);
    return;
  }
  bool home = ((mode == M_CAROUSEL && carousel == 0) || mode == M_UNLOCK) && !sessionDirty;
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
  if (e == EV_CALM) {  // down held: the calm screen, then back to whatever was showing
    calm();
    show(false);
    return;
  }
  if (keyChanged(e)) show(false);
}

}  // namespace app
