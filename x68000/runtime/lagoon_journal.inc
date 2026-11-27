#include <time.h>

enum { LAGOON_ACH_AREAS, LAGOON_ACH_ROUTES, LAGOON_ACH_LEVEL };
static const struct {
    const char *id, *title, *detail;
    int event, target;
} lagoon_awards[] = {
#include "lagoon_achievements_data.inc"
};
#define LAGOON_AWARD_COUNT ((int)(sizeof lagoon_awards / sizeof *lagoon_awards))
#define LAGOON_CHIME_FRAMES 8820

static struct {
    int level, hp, max_hp, mp, max_mp, exp, next_exp, gold, areas;
} lagoon_journal;
static wchar_t lagoon_awards_path[PATH_CAP];
static uint64_t lagoon_unlocks[LAGOON_AWARD_COUNT];
static bool lagoon_unsaved_unlocks[LAGOON_AWARD_COUNT];
static int lagoon_pending_awards[LAGOON_AWARD_COUNT], lagoon_pending_count, lagoon_next_award;
static int lagoon_showing_award = -1;
static Uint64 lagoon_award_until;
static int lagoon_chime_frame = LAGOON_CHIME_FRAMES;

static void lagoon_open_awards(void) {
    if (!lagoon_game_data_path(lagoon_awards_path, "achievements.ini")) return;
    for (int i = 0; i < LAGOON_AWARD_COUNT; ++i) {
        wchar_t key[81], text[24], *end;
        if (!rr_from_utf8(lagoon_awards[i].id, key, 81)) continue;
        rr_ini_get(L"Unlocked", key, L"", text, 24, lagoon_awards_path);
        if (text[0] < L'0' || text[0] > L'9') continue;
        errno = 0;
        uint64_t time_value = wcstoull(text, &end, 10);
        if (!errno && !*end && time_value <= INT64_MAX) lagoon_unlocks[i] = time_value;
    }
}

static void lagoon_save_awards(void) {
    if (!lagoon_awards_path[0] || !lagoon_safe_data_file(lagoon_awards_path)) return;
    for (int i = 0; i < LAGOON_AWARD_COUNT; ++i) {
        if (!lagoon_unsaved_unlocks[i]) continue;
        wchar_t key[81], text[24];
        if (!rr_from_utf8(lagoon_awards[i].id, key, 81) ||
            swprintf(text, 24, L"%llu", (unsigned long long)lagoon_unlocks[i]) < 1) continue;
        if (rr_ini_set(L"Unlocked", key, text, lagoon_awards_path)) lagoon_unsaved_unlocks[i] = false;
    }
}

static void lagoon_check_awards(void) {
    bool awarded = false;
    for (int i = 0; i < LAGOON_AWARD_COUNT; ++i) {
        if (lagoon_unlocks[i]) continue;
        int current = lagoon_awards[i].event == LAGOON_ACH_AREAS ? lagoon_journal.areas
            : lagoon_awards[i].event == LAGOON_ACH_ROUTES ? (int)lagoon_explored.route_count : lagoon_journal.level;
        if (current < lagoon_awards[i].target) continue;
        time_t now = time(NULL);
        lagoon_unlocks[i] = now > 0 ? (uint64_t)now : 1;
        lagoon_unsaved_unlocks[i] = true;
        lagoon_pending_awards[lagoon_pending_count++] = i;
        awarded = true;
    }
    if (awarded) lagoon_save_awards();
}

static void lagoon_read_journal(const uint8_t *ram) {
    int level = lagoon_byte(ram, 0x4cfd), max_hp = lagoon_byte(ram, 0x4cfe);
    if (level < 1 || level > 35 || max_hp == 0 || level != lagoon_byte(ram, 0x4d0d)) return;
    lagoon_journal.level = level;
    lagoon_journal.hp = lagoon_byte(ram, 0x4cff);
    lagoon_journal.max_hp = max_hp;
    lagoon_journal.mp = lagoon_byte(ram, 0x4d01);
    lagoon_journal.max_mp = lagoon_byte(ram, 0x4d00);
    lagoon_journal.exp = lagoon_word(ram, 0x4d04);
    lagoon_journal.next_exp = lagoon_word(ram, 0x4d02);
    lagoon_journal.gold = lagoon_word(ram, 0x4d06);
    lagoon_journal.areas = 0;
    for (int area = 1; area < LAGOON_AREA_COUNT; ++area)
        lagoon_journal.areas += lagoon_explored.visits[area].x != 0;
    lagoon_check_awards();
}

static void lagoon_mix_chime(int16_t *pcm, size_t count) {
    for (size_t i = 0; i + 1 < count && lagoon_chime_frame < LAGOON_CHIME_FRAMES; i += 2) {
        int n = lagoon_chime_frame++, note = n / 4410, age = n % 4410;
        double envelope = SDL_min(age, 144) * (4410 - age) / (144.0 * 4410.0);
        int sound = (int)(1300 * envelope * sin(age * (note ? 0.17564 : 0.14004)));
        for (int channel = 0; channel < 2; ++channel) {
            int mix = pcm[i + channel] + sound;
            pcm[i + channel] = (int16_t)SDL_max(-32768, SDL_min(32767, mix));
        }
    }
}
