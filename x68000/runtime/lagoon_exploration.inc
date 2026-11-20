/* Profile data, separate from X68000 floppy saves and machine snapshots. */
#define LAGOON_ROUTE_LIMIT 128
#define LAGOON_WORLD_SIZE 2048

typedef struct { int x, y; } LagoonPosition;
typedef struct { int from, to; LagoonPosition at; } LagoonRoute;
typedef struct {
    LagoonPosition visits[LAGOON_AREA_COUNT];
    LagoonRoute routes[LAGOON_ROUTE_LIMIT];
    unsigned route_count;
} LagoonExploration;

static LagoonExploration lagoon_explored;
static wchar_t lagoon_history_path[PATH_CAP];
static bool lagoon_exploration_dirty;
static const uint8_t lagoon_history_magic[] = "LGAATLS1";

static bool lagoon_valid_area(int area) {
    return area > 0 && area < LAGOON_AREA_COUNT && lagoon_areas[area];
}

static bool lagoon_valid_position(LagoonPosition at) {
    return at.x > 0 && at.x < LAGOON_WORLD_SIZE && at.y >= 0 && at.y < LAGOON_WORLD_SIZE;
}

static bool lagoon_record_route(int from, int to, LagoonPosition at) {
    if (!lagoon_valid_area(from) || from == to || lagoon_explored.route_count >= LAGOON_ROUTE_LIMIT)
        return false;
    for (unsigned i = 0; i < lagoon_explored.route_count; ++i) {
        const LagoonRoute *route = &lagoon_explored.routes[i];
        if (route->from == from && route->to == to &&
            abs(route->at.x - at.x) < 20 && abs(route->at.y - at.y) < 20) return false;
    }
    lagoon_explored.routes[lagoon_explored.route_count++] = (LagoonRoute){from, to, at};
    return true;
}

static LagoonPosition lagoon_read_position(const uint8_t *bytes) {
    return (LagoonPosition){bytes[0]*256 + bytes[1], bytes[2]*256 + bytes[3]};
}

static bool lagoon_read_history(FILE *file, LagoonExploration *history) {
    uint8_t data[9 + LAGOON_AREA_COUNT*4 + LAGOON_ROUTE_LIMIT*6 + 1];
    const size_t prefix = 9 + LAGOON_AREA_COUNT*4;
    size_t size = fread(data, 1, sizeof data, file);
    if (ferror(file) || size < prefix || memcmp(data, lagoon_history_magic, 8) ||
        data[8] > LAGOON_ROUTE_LIMIT || size != prefix + data[8]*6)
        return false;
    LagoonExploration decoded = {0};
    for (int area = 0; area < LAGOON_AREA_COUNT; ++area) {
        LagoonPosition at = lagoon_read_position(data + 9 + area*4);
        if (!at.x && !at.y) continue;
        if (!lagoon_valid_area(area) || !lagoon_valid_position(at)) return false;
        decoded.visits[area] = at;
    }
    decoded.route_count = data[8];
    for (unsigned i = 0; i < decoded.route_count; ++i) {
        const uint8_t *row = data + prefix + i*6;
        LagoonRoute route = {row[0], row[1], lagoon_read_position(row + 2)};
        if (route.from == route.to || !lagoon_valid_area(route.from) ||
            !lagoon_valid_area(route.to) || !lagoon_valid_position(route.at) ||
            !decoded.visits[route.from].x || !decoded.visits[route.to].x) return false;
        decoded.routes[i] = route;
    }
    *history = decoded;
    return true;
}

static bool lagoon_safe_data_file(const wchar_t *path) {
#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(path);
    return attributes == INVALID_FILE_ATTRIBUTES
        ? GetLastError() == ERROR_FILE_NOT_FOUND
        : !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
#else
    (void)path;
    return true;
#endif
}

static bool lagoon_game_data_path(wchar_t *output, const char *filename) {
    const char *suffix = strrchr(player_config, '/');
    char path[PATH_CAP];
    output[0] = 0;
    if (!suffix || strcmp(suffix, "/player.cfg")) return false;
    int length = snprintf(path, sizeof path, "%.*s/%s", (int)(suffix - player_config), player_config, filename);
    if (length > 0 && length < (int)sizeof path && rr_from_utf8(path, output, PATH_CAP) &&
        lagoon_safe_data_file(output)) return true;
    output[0] = 0;
    return false;
}

static void lagoon_open_history(void) {
    memset(&lagoon_explored, 0, sizeof lagoon_explored);
    lagoon_exploration_dirty = false;
    if (!lagoon_game_data_path(lagoon_history_path, "atlas-history.bin")) return;
    FILE *file = rr_fopen(lagoon_history_path, L"rb");
    if (!file) {
        if (errno != ENOENT) lagoon_history_path[0] = 0;
        return;
    }
    if (!lagoon_read_history(file, &lagoon_explored)) lagoon_history_path[0] = 0;
    fclose(file);
}

static void lagoon_write_position(uint8_t *output, LagoonPosition at) {
    output[0] = (uint8_t)(at.x >> 8);
    output[1] = (uint8_t)at.x;
    output[2] = (uint8_t)(at.y >> 8);
    output[3] = (uint8_t)at.y;
}

static bool lagoon_save_history(void) {
    if (!lagoon_exploration_dirty) return true;
    if (!lagoon_history_path[0] || !lagoon_safe_data_file(lagoon_history_path)) return false;
    wchar_t temporary[PATH_CAP];
    int length = swprintf(temporary, PATH_CAP, L"%ls.tmp-%lu", lagoon_history_path, rr_process_id());
    if (length < 0 || length >= PATH_CAP) return false;
    FILE *file = rr_fopen(temporary, L"wbx");
    if (!file) return false;
    uint8_t data[9 + LAGOON_AREA_COUNT*4 + LAGOON_ROUTE_LIMIT*6];
    memcpy(data, lagoon_history_magic, 8);
    data[8] = (uint8_t)lagoon_explored.route_count;
    const size_t prefix = 9 + LAGOON_AREA_COUNT*4;
    for (int area = 0; area < LAGOON_AREA_COUNT; ++area)
        lagoon_write_position(data + 9 + area*4, lagoon_explored.visits[area]);
    for (unsigned i = 0; i < lagoon_explored.route_count; ++i) {
        const LagoonRoute *route = &lagoon_explored.routes[i];
        uint8_t *row = data + prefix + i*6;
        row[0] = (uint8_t)route->from;
        row[1] = (uint8_t)route->to;
        lagoon_write_position(row + 2, route->at);
    }
    size_t size = prefix + (size_t)lagoon_explored.route_count*6;
    bool written = fwrite(data, 1, size, file) == size && rr_flush_file(file);
    if (fclose(file)) written = false;
    if (!written || !rr_replace(temporary, lagoon_history_path)) {
        rr_remove(temporary);
        return false;
    }
    lagoon_exploration_dirty = false;
    return true;
}
