/* Included by LagoonExtra's X68000 adapter. Reads game RAM and paints a paused map. */
#include "lagoon_atlas_data.inc"
#include "lagoon_ui_font.generated.h"

#define LAGOON_AREA_COUNT ((int)(sizeof lagoon_areas / sizeof *lagoon_areas))
#define LAGOON_MAP_SIZE 512
#define LAGOON_MAP_DETAIL 4
#define LAGOON_BASE 0x7efc0
#include "lagoon_exploration.inc"

enum {
    LAGOON_MAP, LAGOON_ROUTES, LAGOON_ITEMS, LAGOON_JOURNAL, LAGOON_TROPHIES, LAGOON_CONTROLS,
    LAGOON_SETTINGS, LAGOON_PAGES
};
enum { LAGOON_VOLUME, LAGOON_MUTE, LAGOON_VIEW_ZOOM, LAGOON_QUIT, LAGOON_SETTINGS_ROWS };
static const char *const lagoon_zoom_labels[] = {"1x", "1.25x", "1.5x"};
static int lagoon_zoom_mode;
int lagoon_load_zoom(void);
void lagoon_save_zoom(int zoom);

static void lagoon_adjust_zoom(int step) {
    lagoon_zoom_mode = (lagoon_zoom_mode + step + 3) % 3;
    lagoon_save_zoom(lagoon_zoom_mode);
}

int lagoon_sound_volume(void);
void lagoon_set_sound_volume(int volume);
bool rrhk_sound_muted(void);
bool rrhk_sound_toggle_mute(void);

enum { LAGOON_DOORWAY_LIMIT = 64 };

typedef struct { LagoonPosition at, last; int target; } LagoonDoorway;

static struct {
    LagoonDoorway doors[LAGOON_DOORWAY_LIMIT];
    int door_count;
    bool open, consume_pad_release, confirm_quit, quit_selected, trigger_held, zoom_trigger_held;
    int settings_row;
    int area, previous, pending, shown, zoom, pan_x, pan_y, page, route_cursor;
    LagoonPosition at;
    SDL_Texture *texture;
    int painted;
} lagoon_chart;

static const LagoonRoute *lagoon_route_at(int area, int number) {
    for (unsigned i = 0; i < lagoon_explored.route_count; ++i)
        if (lagoon_explored.routes[i].from == area && number-- == 0) return &lagoon_explored.routes[i];
    return NULL;
}

static const char *lagoon_area_name(int area) {
    return lagoon_valid_area(area) ? lagoon_area_labels[area] : "Unknown area";
}

static int lagoon_byte(const uint8_t *ram, int offset) {
    return ram[(LAGOON_BASE + offset) ^ 1]; /* PX68K stores RAM in host word order. */
}

static int lagoon_word(const uint8_t *ram, int offset) {
    return lagoon_byte(ram, offset) << 8 | lagoon_byte(ram, offset + 1);
}

#include "lagoon_journal.inc"

static void lagoon_observe_area(int area, LagoonPosition at) {
    if (!lagoon_valid_area(area) || !lagoon_valid_position(at)) {
        lagoon_chart.area = lagoon_chart.pending = 0;
        return;
    }
    if (area != lagoon_chart.area && lagoon_chart.pending != area) {
        lagoon_chart.pending = area;
        return;
    }
    bool discovered = !lagoon_explored.visits[area].x;
    bool new_route = lagoon_record_route(lagoon_chart.previous, area, lagoon_chart.at);
    LagoonPosition last = lagoon_explored.visits[area];
    if (new_route || last.x != at.x || last.y != at.y) lagoon_exploration_dirty = true;
    lagoon_explored.visits[area] = at;
    lagoon_chart.previous = lagoon_chart.area = area;
    lagoon_chart.pending = 0;
    lagoon_chart.at = at;
    if (discovered || new_route) lagoon_save_history();
}

static void lagoon_note_position(void) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (ram && core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) >= LAGOON_BASE + 0xeccc + 2 &&
        lagoon_word(ram, 0xe968) == 0x4279 && lagoon_word(ram, 0xe96a) == 0x00e8 &&
        lagoon_word(ram, 0xe96c) == 0x002a && lagoon_byte(ram, 0x4035) == 0) {
        lagoon_observe_area(lagoon_byte(ram, 0xeccc), (LagoonPosition){lagoon_word(ram, 0x402e), lagoon_word(ram, 0x4030)});
        if (lagoon_chart.area) lagoon_read_journal(ram);
    } else {
        lagoon_chart.area = lagoon_chart.pending = 0;
    }
}

/* The original field dispatcher walks 16-byte records at $442E. Read
 * ordinary map doors from the current room while pausing the guest. These
 * marks never become visits or achievement progress. */
static void lagoon_read_doorways(void) {
    lagoon_chart.door_count = 0;
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) <= LAGOON_BASE + 0xeccc ||
        lagoon_byte(ram, 0xeccc) != lagoon_chart.area || lagoon_byte(ram, 0x4035)) return;
    for (int slot = 0; slot < LAGOON_DOORWAY_LIMIT; ++slot) {
        int entry = 0x442e + 16 * slot;
        int target = lagoon_byte(ram, entry);
        int x = lagoon_byte(ram, entry + 1), y = lagoon_byte(ram, entry + 2);
        if (!(x | y)) break;
        if (!target) { ++slot; continue; }
        if (x >= 128 || y >= 128 || !lagoon_valid_area(target)) continue;
        LagoonPosition at = {16*x + 8, 16*y + 8};
        int i = 0;
        while (i < lagoon_chart.door_count) {
            LagoonDoorway *door = &lagoon_chart.doors[i];
            int distance = abs(at.x - door->last.x) + abs(at.y - door->last.y);
            if (door->target == target && distance <= 16) {
                door->at = (LagoonPosition){(door->at.x + at.x) / 2, (door->at.y + at.y) / 2};
                door->last = at;
                break;
            }
            ++i;
        }
        if (i == lagoon_chart.door_count)
            lagoon_chart.doors[lagoon_chart.door_count++] = (LagoonDoorway){at, at, target};
    }
}

static void lagoon_include_position(SDL_Rect *bounds, LagoonPosition at) {
    if (!lagoon_valid_position(at)) return;
    int x = at.x / 16 * 4, y = at.y / 16 * 4;
    SDL_Rect around = {.x = SDL_max(0, x - 8), .y = SDL_max(0, y - 8)};
    around.w = SDL_min(LAGOON_MAP_SIZE, x + 12) - around.x;
    around.h = SDL_min(LAGOON_MAP_SIZE, y + 12) - around.y;
    SDL_Rect merged;
    SDL_UnionRect(bounds, &around, &merged);
    *bounds = merged;
}

static SDL_Rect lagoon_map_bounds(int area) {
    SDL_Rect bounds = lagoon_area_bounds[area];
    // Include routes and the player's last position even on undecorated terrain.
    if (area == lagoon_chart.area)
        for (int i = 0; i < lagoon_chart.door_count; ++i)
            lagoon_include_position(&bounds, lagoon_chart.doors[i].at);
    lagoon_include_position(&bounds, lagoon_explored.visits[area]);
    for (unsigned i = 0; i < lagoon_explored.route_count; ++i)
        if (lagoon_explored.routes[i].from == area)
            lagoon_include_position(&bounds, lagoon_explored.routes[i].at);
    return bounds;
}

static void lagoon_recenter(void) {
    SDL_Rect bounds = lagoon_map_bounds(lagoon_chart.shown);
    LagoonPosition at = lagoon_explored.visits[lagoon_chart.shown];
    lagoon_chart.pan_x = at.x / 4 - bounds.w / lagoon_chart.zoom / 2;
    lagoon_chart.pan_y = at.y / 4 - bounds.h / lagoon_chart.zoom / 2;
    lagoon_chart.route_cursor = 0;
}

static SDL_Rect lagoon_map_source(void) {
    SDL_Rect bounds = lagoon_map_bounds(lagoon_chart.shown);
    int w = bounds.w / lagoon_chart.zoom, h = bounds.h / lagoon_chart.zoom;
    lagoon_chart.pan_x = SDL_clamp(lagoon_chart.pan_x, bounds.x, bounds.x + bounds.w - w);
    lagoon_chart.pan_y = SDL_clamp(lagoon_chart.pan_y, bounds.y, bounds.y + bounds.h - h);
    return (SDL_Rect){lagoon_chart.pan_x, lagoon_chart.pan_y, w, h};
}

static SDL_Rect lagoon_map_letterbox(SDL_Rect src, SDL_Rect frame) {
    int w = frame.w, h = frame.h;
    if (frame.w * src.h <= frame.h * src.w)
        h = SDL_max(1, frame.w * src.h / src.w);
    else
        w = SDL_max(1, frame.h * src.w / src.h);
    return (SDL_Rect){frame.x + (frame.w - w) / 2, frame.y + (frame.h - h) / 2, w, h};
}

static bool lagoon_map_point(SDL_Rect src, SDL_Rect dst, LagoonPosition at, SDL_Point *point) {
    int x = at.x / 4 - src.x, y = at.y / 4 - src.y;
    if (x < 0 || x >= src.w || y < 0 || y >= src.h) return false;
    point->x = dst.x + x * dst.w / src.w;
    point->y = dst.y + y * dst.h / src.h;
    return true;
}

static void lagoon_close_atlas(void) {
    if (lagoon_chart.texture) SDL_DestroyTexture(lagoon_chart.texture);
    lagoon_chart.texture = NULL;
    lagoon_chart.painted = 0;
    lagoon_chart.open = false;
    lagoon_chart.confirm_quit = false;
    lagoon_chart.consume_pad_release = true;
    loading_holdoff = 16;
}

static bool lagoon_settings_event(const SDL_Event *event, int key, int pad, bool back, bool *running) {
    if (!key && pad < 0) return false;
    const bool left = key == SDL_SCANCODE_LEFT || pad == SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    const bool right = key == SDL_SCANCODE_RIGHT || pad == SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    const bool accept = key == SDL_SCANCODE_RETURN || pad == SDL_CONTROLLER_BUTTON_A;
    if (lagoon_chart.confirm_quit) {
        if (key && event->key.repeat) return true;
        if (key == SDL_SCANCODE_ESCAPE) ca.escape_held = 1;
        if (back || (accept && !lagoon_chart.quit_selected)) lagoon_chart.confirm_quit = false;
        else if (left || right) lagoon_chart.quit_selected = right;
        else if (accept) {
            lagoon_close_atlas();
            *running = false;
        }
        return true;
    }
    if (back) return false;
    if (key == SDL_SCANCODE_UP || pad == SDL_CONTROLLER_BUTTON_DPAD_UP)
        lagoon_chart.settings_row = SDL_max(0, lagoon_chart.settings_row - 1);
    if (key == SDL_SCANCODE_DOWN || pad == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        lagoon_chart.settings_row = SDL_min(LAGOON_SETTINGS_ROWS - 1, lagoon_chart.settings_row + 1);
    if (lagoon_chart.settings_row == LAGOON_VOLUME && (left || right)) {
        int volume = lagoon_sound_volume() + (left ? -5 : 5);
        lagoon_set_sound_volume(SDL_clamp(volume, 0, 200));
    } else if ((!key || !event->key.repeat) && (accept || left || right)) {
        if (lagoon_chart.settings_row == LAGOON_MUTE) rrhk_sound_toggle_mute();
        if (lagoon_chart.settings_row == LAGOON_VIEW_ZOOM) lagoon_adjust_zoom(left ? -1 : 1);
        if (lagoon_chart.settings_row == LAGOON_QUIT && accept) {
            lagoon_chart.confirm_quit = true;
            lagoon_chart.quit_selected = false;
        }
    }
    return true;
}

static bool lagoon_chart_event(const SDL_Event *event, bool *running) {
    RrhkUi *ui = rrhk_ui();
    int key = event->type == SDL_KEYDOWN ? event->key.keysym.scancode : 0;
    int mod = key ? event->key.keysym.mod : 0;
    int pad = event->type == SDL_CONTROLLERBUTTONDOWN ? event->cbutton.button : -1;
    SDL_JoystickID owner = ui->pads[0] ? SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(ui->pads[0])) : -1;
    if (pad >= 0 && (owner < 0 || owner != event->cbutton.which)) pad = -1;
    const bool bumper = pad == SDL_CONTROLLER_BUTTON_LEFTSHOULDER || pad == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    bool trigger = false, zoom_trigger = false;
    if (event->type == SDL_CONTROLLERDEVICEREMOVED)
        lagoon_chart.trigger_held = lagoon_chart.zoom_trigger_held = false;
    if (event->type == SDL_CONTROLLERAXISMOTION && owner >= 0 && event->caxis.which == owner) {
        const bool left = event->caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT;
        const bool right = event->caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
        if (left || right) {
            bool *held = left ? &lagoon_chart.zoom_trigger_held : &lagoon_chart.trigger_held;
            if (event->caxis.value < 10000) *held = false;
            else if (event->caxis.value >= 20000 && !*held) {
                trigger = right;
                zoom_trigger = left;
                *held = true;
            }
        }
    }
    const int atlas_button = ui->machine ? ui->buttons[0][X_ATLAS] : SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    bool toggle = (key && !event->key.repeat &&
        ((key == SDL_SCANCODE_M && (mod & KMOD_CTRL) && !(mod & KMOD_ALT)) ||
         (ui->machine && key == ui->keys[0][X_ATLAS]))) ||
        (pad >= 0 && pad == atlas_button && (!lagoon_chart.open || !bumper));
    const bool settings = trigger || (key == SDL_SCANCODE_COMMA && !event->key.repeat &&
        (mod & KMOD_CTRL) && !(mod & KMOD_ALT));
    if (!lagoon_chart.open) {
        bool zoom_key = key == SDL_SCANCODE_Z && !event->key.repeat &&
            (mod & KMOD_CTRL) && !(mod & KMOD_ALT);
        if ((zoom_trigger || zoom_key) && !ui->menu && !ui->paused && !ui->capturing &&
            input_focus && lagoon_chart.area) {
            lagoon_adjust_zoom(1);
            player_notice(lagoon_zoom_labels[lagoon_zoom_mode]);
            return true;
        }
        if ((toggle || settings) && !ui->menu && !ui->paused && !ui->capturing && input_focus) {
            rrhk_options_observe(event);
            if (!lagoon_chart.area) player_notice("Map available after entering an area");
            else {
                lagoon_chart.open = true;
                lagoon_chart.shown = lagoon_chart.area;
                lagoon_chart.page = settings ? LAGOON_SETTINGS : LAGOON_MAP;
                lagoon_chart.settings_row = LAGOON_VOLUME;
                lagoon_chart.zoom = 1;
                lagoon_read_doorways();
                lagoon_recenter();
                lagoon_save_history();
            }
            return true;
        }
        return x_event(event, running);
    }
    bool back = toggle || key == SDL_SCANCODE_ESCAPE || pad == SDL_CONTROLLER_BUTTON_B;
    int tab = (key == SDL_SCANCODE_TAB && (mod & KMOD_SHIFT)) || pad == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ? -1
        : key == SDL_SCANCODE_TAB || pad == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER || pad == SDL_CONTROLLER_BUTTON_BACK ? 1 : 0;
    if (lagoon_chart.page == LAGOON_SETTINGS && (lagoon_chart.confirm_quit || !tab) &&
        lagoon_settings_event(event, key, pad, back, running)) {
        rrhk_options_observe(event);
        return true;
    }
    if (back) {
        rrhk_options_observe(event);
        if (key == SDL_SCANCODE_ESCAPE) ca.escape_held = 1;
        lagoon_close_atlas();
        return true;
    }
    if (tab) {
        if (!key || !event->key.repeat) {
            lagoon_chart.page = (lagoon_chart.page + tab + LAGOON_PAGES) % LAGOON_PAGES;
            if (lagoon_chart.page == LAGOON_SETTINGS) lagoon_chart.settings_row = LAGOON_VOLUME;
        }
        return true;
    }
    const bool area_page = lagoon_chart.page == LAGOON_MAP || lagoon_chart.page == LAGOON_ROUTES;
    if (settings && (!trigger || !area_page) && !lagoon_chart.confirm_quit) {
        lagoon_chart.page = LAGOON_SETTINGS;
        lagoon_chart.settings_row = LAGOON_VOLUME;
        return true;
    }
    if (lagoon_chart.page == LAGOON_ROUTES) {
        if ((key == SDL_SCANCODE_UP || pad == SDL_CONTROLLER_BUTTON_DPAD_UP) && lagoon_chart.route_cursor)
            --lagoon_chart.route_cursor;
        if ((key == SDL_SCANCODE_DOWN || pad == SDL_CONTROLLER_BUTTON_DPAD_DOWN) &&
            lagoon_route_at(lagoon_chart.shown, lagoon_chart.route_cursor + 1)) ++lagoon_chart.route_cursor;
        if (key == SDL_SCANCODE_RETURN || pad == SDL_CONTROLLER_BUTTON_A) {
            const LagoonRoute *route = lagoon_route_at(lagoon_chart.shown, lagoon_chart.route_cursor);
            if (route) {
                lagoon_chart.shown = route->to;
                lagoon_chart.page = LAGOON_MAP;
                lagoon_chart.zoom = 1;
                lagoon_recenter();
            }
            return true;
        }
    }
    if (lagoon_chart.page == LAGOON_MAP && (key == SDL_SCANCODE_Z || pad == SDL_CONTROLLER_BUTTON_Y)) {
        if (!key || !event->key.repeat) {
            lagoon_chart.zoom = lagoon_chart.zoom == 4 ? 1 : lagoon_chart.zoom * 2;
            lagoon_recenter();
        }
        return true;
    }
    if (key == SDL_SCANCODE_HOME || pad == SDL_CONTROLLER_BUTTON_X) {
        lagoon_chart.shown = lagoon_chart.area;
        lagoon_chart.page = LAGOON_MAP;
        lagoon_chart.zoom = 1;
        lagoon_recenter();
        return true;
    }
    int direction = key == SDL_SCANCODE_PAGEDOWN || trigger ? 1 : key == SDL_SCANCODE_PAGEUP || zoom_trigger ? -1 : 0;
    if (direction && area_page) {
        int area = lagoon_chart.shown;
        do { area = (area + direction + LAGOON_AREA_COUNT) % LAGOON_AREA_COUNT; }
        while (!lagoon_explored.visits[area].x);
        lagoon_chart.shown = area;
        lagoon_chart.zoom = 1;
        lagoon_recenter();
        return true;
    }
    if (lagoon_chart.page == LAGOON_MAP) {
        SDL_Rect src = lagoon_map_source();
        int step_x = SDL_max(1, src.w / 10), step_y = SDL_max(1, src.h / 10);
        if (key == SDL_SCANCODE_LEFT || pad == SDL_CONTROLLER_BUTTON_DPAD_LEFT) lagoon_chart.pan_x -= step_x;
        if (key == SDL_SCANCODE_RIGHT || pad == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) lagoon_chart.pan_x += step_x;
        if (key == SDL_SCANCODE_UP || pad == SDL_CONTROLLER_BUTTON_DPAD_UP) lagoon_chart.pan_y -= step_y;
        if (key == SDL_SCANCODE_DOWN || pad == SDL_CONTROLLER_BUTTON_DPAD_DOWN) lagoon_chart.pan_y += step_y;
    }
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP || event->type == SDL_CONTROLLERBUTTONDOWN ||
        event->type == SDL_CONTROLLERBUTTONUP) {
        rrhk_options_observe(event);
        if (key == SDL_SCANCODE_Q && (mod & KMOD_CTRL) && (mod & KMOD_ALT)) *running = false;
        return true;
    }
    return x_event(event, running);
}

static uint32_t lagoon_rgb(unsigned value) {
    unsigned i = value & 1;
    unsigned r = ((value >> 5 & 62) | i) * 255 / 63;
    unsigned g = ((value >> 10 & 62) | i) * 255 / 63;
    unsigned b = ((value & 62) | i) * 255 / 63;
    return 0xff000000u | r << 16 | g << 8 | b;
}

static void lagoon_scaled_text(SDL_Renderer *renderer, int ox, int oy, int unit,
                              int x, int y, const char *text, uint32_t color, bool title, int scale) {
    int screen_x = ox + x*unit, screen_y = oy + y*unit;
    int width = title ? 20 : 16, height = title ? 20 : 12;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    for (; *text; ++text) {
        unsigned glyph = *text >= 32 && *text <= 126 ? *text - 32 : '?' - 32;
        const unsigned char *alpha = title ? ui_body_alpha[glyph] : ui_small_alpha[glyph];
        for (int py = 0; py < height; ++py) for (int px = 0; px < width; ++px) {
            int index = py*width + px;
            unsigned coverage = (index & 1 ? alpha[index/2] & 15 : alpha[index/2] >> 4) * 17;
            if (!coverage) continue;
            SDL_Rect pixel = {screen_x + (px - 2)*scale, screen_y + py*scale, scale, scale};
            SDL_SetRenderDrawColor(renderer, (uint8_t)(color >> 16), (uint8_t)(color >> 8), (uint8_t)color, (uint8_t)coverage);
            SDL_RenderFillRect(renderer, &pixel);
        }
        screen_x += (title ? ui_body_advance[glyph] : ui_small_advance[glyph])*scale;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static void lagoon_text(SDL_Renderer *renderer, int ox, int oy, int unit,
                        int x, int y, const char *text, uint32_t color, bool title) {
    lagoon_scaled_text(renderer, ox, oy, unit, x, y, text, color, title, SDL_max(1, unit / 2));
}

static unsigned lagoon_map_texel(const unsigned char *pattern, int x, int y) {
    unsigned packed = pattern[y * 4 + x / 2];
    return x & 1 ? packed & 15 : packed >> 4;
}

static bool lagoon_chart_texture(SDL_Renderer *renderer) {
    if (lagoon_chart.painted == lagoon_chart.shown) return true;
    if (!lagoon_chart.texture) {
        lagoon_chart.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, LAGOON_MAP_SIZE * LAGOON_MAP_DETAIL,
            LAGOON_MAP_SIZE * LAGOON_MAP_DETAIL);
        if (!lagoon_chart.texture) return false;
    }
    const unsigned char *data = lagoon_areas[lagoon_chart.shown];
    const unsigned char *patterns = data + 512;
    const unsigned char *metatiles = patterns + 512 * 32;
    const unsigned char *grid = metatiles + 256 * 8;
    const unsigned char *foreground = grid + 128 * 128;
    const unsigned char *foreground_grid = foreground + 256 * 8;
    uint32_t palette[256];
    for (int i = 0; i < 256; ++i) {
        unsigned word = data[2*i] << 8 | data[2*i + 1];
        palette[i] = word ? lagoon_rgb(word) : 0xff10161c;
    }
    uint8_t ground_cells[256][16][16], foreground_cells[256][16][16];
    for (int cell = 0; cell < 256; ++cell) {
        for (int quadrant = 0; quadrant < 4; ++quadrant) {
            const unsigned char *descriptor = metatiles + cell * 8 + quadrant * 2;
            unsigned pattern = (descriptor[0] & 1) << 8 | descriptor[1];
            unsigned group = descriptor[0] & 0xf0;
            const unsigned char *front = foreground + cell * 8 + quadrant * 2;
            int flip_x = front[0] & 0x40 ? 7 : 0, flip_y = front[0] & 0x80 ? 7 : 0;
            int left = (quadrant >> 1) * 8, top = (quadrant & 1) * 8;
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    unsigned texel = lagoon_map_texel(patterns + pattern * 32, x, y);
                    unsigned front_texel = lagoon_map_texel(patterns + front[1] * 32, x ^ flip_x, y ^ flip_y);
                    ground_cells[cell][top + y][left + x] = (uint8_t)(group + texel + 16);
                    foreground_cells[cell][top + y][left + x] = front_texel ?
                        (uint8_t)((front[0] & 15) * 16 + front_texel) : 0;
                }
            }
        }
    }
    void *pixels;
    int pitch;
    if (SDL_LockTexture(lagoon_chart.texture, NULL, &pixels, &pitch)) return false;
    for (int y = 0; y < LAGOON_MAP_SIZE * LAGOON_MAP_DETAIL; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)pixels + (size_t)y * pitch);
        for (int cell_x = 0; cell_x < 128; ++cell_x) {
            int position = y / 16 * 128 + cell_x;
            const uint8_t *ground = ground_cells[grid[position]][y % 16];
            const uint8_t *front = foreground_cells[foreground_grid[position]][y % 16];
            for (int x = 0; x < 16; ++x) row[cell_x * 16 + x] = palette[front[x] ? front[x] : ground[x]];
        }
    }
    SDL_UnlockTexture(lagoon_chart.texture);
    lagoon_chart.painted = lagoon_chart.shown;
    return true;
}

static void lagoon_chart_tabs(SDL_Renderer *renderer, int ox, int oy, int unit, int y) {
    static const char *const names[] = {"Map", "Routes", "Items", "Journal", "Awards", "Controls", "Settings"};
    for (int page = 0; page < LAGOON_PAGES; ++page) {
        int x = 9 + 34*page;
        bool active = page == lagoon_chart.page;
        lagoon_text(renderer, ox, oy, unit, x, y, names[page], active ? 0xedc977 : 0xbdb8a7, false);
        if (active) rr_menu_box(renderer, ox, oy, unit, x, y + SDL_max(7, 12 * SDL_max(1, unit / 2) / unit),
            28, 1, 237, 201, 119, 255);
    }
}

static void lagoon_chart_draw_routes(SDL_Renderer *renderer, int ox, int oy, int unit) {
    int first = (lagoon_chart.route_cursor / 8)*8;
    rr_menu_box(renderer, ox, oy, unit, 9, 25, 237, 1, 78, 80, 86, 255);
    lagoon_text(renderer, ox, oy, unit, 9, 27, "Trips from this area", 0xedc977, true);
    if (!lagoon_route_at(lagoon_chart.shown, 0)) {
        lagoon_text(renderer, ox, oy, unit, 9, 53, "Routes appear after you travel to another area.", 0xbdb8a7, false);
        return;
    }
    for (int row = 0; row < 8; ++row) {
        int number = first + row;
        const LagoonRoute *route = lagoon_route_at(lagoon_chart.shown, number);
        if (!route) break;
        int y = 49 + row*13;
        if (number == lagoon_chart.route_cursor)
            rr_menu_box(renderer, ox, oy, unit, 9, y-2, 237, 12, 41, 42, 50, 255);
        char line[56];
        snprintf(line, sizeof line, "%d   %s  #%02d", number + 1, lagoon_area_name(route->to), route->to);
        lagoon_text(renderer, ox, oy, unit, 12, y, line, 0xf5f1df, false);
    }
    lagoon_text(renderer, ox, oy, unit, 9, 160, "Up/Down   Select", 0xbdb8a7, false);
    lagoon_text(renderer, ox, oy, unit, 116, 160, "Enter / A   View destination", 0xedc977, false);
}

static void lagoon_chart_draw_items(SDL_Renderer *renderer, int ox, int oy, int unit) {
    static const struct { const char *name, *detail; } items[] = {
        {"Potion", "Restores some HP."}, {"Elixir", "Fully restores HP."},
        {"Light stone", "Restores some MP."}, {"Radiant stone", "Fully restores MP."},
        {"Pearl of life", "Lets you return after defeat."},
        {"Ring of Power", "Raises attack power."}, {"Ring of Defense", "Raises defense."},
        {"Ring of Refuge", "Protects against attacks."}, {"Ring of Recovery", "Restores lost health."},
        {"Ring of Time", "Stops time."}
    };
    rr_menu_box(renderer, ox, oy, unit, 9, 25, 237, 1, 78, 80, 86, 255);
    lagoon_text(renderer, ox, oy, unit, 9, 27, "Item effects", 0xedc977, true);
    for (int i = 0; i < (int)(sizeof items / sizeof *items); ++i) {
        int x = 9 + (i / 5)*119, y = 47 + (i % 5)*23;
        lagoon_text(renderer, ox, oy, unit, x, y, items[i].name, 0xf5f1df, true);
        lagoon_text(renderer, ox, oy, unit, x, y + 10, items[i].detail, 0xbdb8a7, false);
    }
}

static void lagoon_chart_draw_journal(SDL_Renderer *renderer, int ox, int oy, int unit) {
    char value[72];
    rr_menu_box(renderer, ox, oy, unit, 9, 25, 237, 1, 78, 80, 86, 255);
    lagoon_text(renderer, ox, oy, unit, 9, 28, "Nasir", 0xedc977, true);
    if (!lagoon_journal.level) {
        lagoon_text(renderer, ox, oy, unit, 9, 54, "Play a little longer to see Nasir's stats.", 0xbdb8a7, false);
        return;
    }
    snprintf(value, sizeof value, "Level %d", lagoon_journal.level);
    lagoon_text(renderer, ox, oy, unit, 9, 54, value, 0xf5f1df, true);
    snprintf(value, sizeof value, "HP %d / %d", lagoon_journal.hp, lagoon_journal.max_hp);
    lagoon_text(renderer, ox, oy, unit, 9, 70, value, 0xbdb8a7, false);
    snprintf(value, sizeof value, "MP %d / %d", lagoon_journal.mp, lagoon_journal.max_mp);
    lagoon_text(renderer, ox, oy, unit, 9, 82, value, 0xbdb8a7, false);
    snprintf(value, sizeof value, "Gold %d", lagoon_journal.gold);
    lagoon_text(renderer, ox, oy, unit, 128, 54, value, 0xf5f1df, true);
    snprintf(value, sizeof value, "EXP %d", lagoon_journal.exp);
    lagoon_text(renderer, ox, oy, unit, 128, 70, value, 0xbdb8a7, false);
    if (lagoon_journal.level < 35) {
        snprintf(value, sizeof value, "Next level at %d EXP", lagoon_journal.next_exp);
        lagoon_text(renderer, ox, oy, unit, 128, 82, value, 0xbdb8a7, false);
    }
    rr_menu_box(renderer, ox, oy, unit, 9, 98, 237, 1, 78, 80, 86, 255);
    snprintf(value, sizeof value, "%d %s visited  /  %u %s recorded",
        lagoon_journal.areas, lagoon_journal.areas == 1 ? "area" : "areas",
        lagoon_explored.route_count, lagoon_explored.route_count == 1 ? "route" : "routes");
    lagoon_text(renderer, ox, oy, unit, 9, 105, value, 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9, 123, "X68000 area notes", 0xedc977, true);
    /* Regional reminders, rather than inferred quest completion from visits. */
    const char *lines[4] = {
        "Hold Sword while moving into enemies to attack.",
        "Staff and sword together determine your magic.",
        "Routes records exits you have already used.",
        "This edition follows its own story and puzzles."
    };
    if (lagoon_chart.area >= 1 && lagoon_chart.area <= 3) {
        lines[0] = "Visit the bakery, NW neighbor and diviner.";
        lines[1] = "Mayor's wife, church, then NW neighbor again.";
        lines[2] = "Return to the mayor for a letter and money.";
        lines[3] = "Buy and equip sword, armor and shield.";
    } else if (lagoon_chart.area >= 4 && lagoon_chart.area <= 7) {
        lines[0] = "Giles needs help, not your healing Potion.";
        lines[1] = "Climb to Giles and help him.";
        lines[2] = "Then return to Atland's diviner for a blade.";
        lines[3] = "Her blade opens Samson's chamber.";
    } else if (lagoon_chart.area == 8) {
        lines[0] = "Elves guard the way into Voloh.";
        lines[1] = "Visit Nasir's family in Atland for bread.";
        lines[2] = "Bring the bread back to the elves.";
        lines[3] = "Hold Sword while approaching enemies.";
    } else if (lagoon_chart.area == 9) {
        lines[0] = "Speak to Mathias and the village elder.";
        lines[1] = "Find Zerah at the back of the village.";
        lines[2] = "Continue toward Philips Castle afterwards.";
        lines[3] = "X68000 Voloh has no tablet hunt.";
    }
    for (int i = 0; i < 4; ++i)
        lagoon_text(renderer, ox, oy, unit, 9, 138 + 9*i, lines[i], 0xbdb8a7, false);
}

static void lagoon_chart_draw_awards(SDL_Renderer *renderer, int ox, int oy, int unit) {
    rr_menu_box(renderer, ox, oy, unit, 9, 25, 237, 1, 78, 80, 86, 255);
    for (int i = 0; i < LAGOON_AWARD_COUNT; ++i) {
        int y = 29 + i*17;
        lagoon_text(renderer, ox, oy, unit, 9, y, lagoon_awards[i].title, 0xf5f1df, true);
        lagoon_text(renderer, ox, oy, unit, 9, y + 8, lagoon_awards[i].detail, 0xbdb8a7, false);
        lagoon_text(renderer, ox, oy, unit, 199, y + 2, lagoon_unlocks[i] ? "Unlocked" : "Locked",
            lagoon_unlocks[i] ? 0xedc977 : 0xbdb8a7, false);
    }
}

/* The same button names used by RetroRecomp's binding picker. */
extern const char *rrhk_pad_binding_name(int value);

static void lagoon_chart_draw_controls(SDL_Renderer *renderer, int ox, int oy, int unit) {
    static const struct { int action; const char *title, *key; } rows[] = {
        {X_A, "Hold sword", "S"}, {X_B, "Jump", "D"},
        {X_ITEMS, "Open Items", "I"}, {X_EQUIPMENT, "Open Equipment", "E"},
        {X_SAVE_LOAD, "Open Save / Load", "F1"}, {X_ATLAS, "Open the Atlas", "Ctrl+M"}
    };
    const RrhkUi *ui = rrhk_ui();
    rr_menu_box(renderer, ox, oy, unit, 9, 25, 237, 1, 78, 80, 86, 255);
    lagoon_text(renderer, ox, oy, unit, 9, 31, "In-game shortcuts", 0xedc977, true);
    lagoon_text(renderer, ox, oy, unit, 134, 46, "Keyboard", 0xbdb8a7, false);
    lagoon_text(renderer, ox, oy, unit, 197, 46, "Controller", 0xbdb8a7, false);
    for (int row = 0; row < (int)(sizeof rows / sizeof *rows); ++row) {
        int action = rows[row].action, y = 57 + 11*row;
        int button = ui->machine ? ui->buttons[0][action] : x_actions[action].pad[0];
        const char *binding = rrhk_pad_binding_name(button);
        if (!binding) binding = "Unbound";
        lagoon_text(renderer, ox, oy, unit, 9, y, rows[row].title, 0xf5f1df, false);
        lagoon_text(renderer, ox, oy, unit, 134, y, rows[row].key, 0xf5f1df, false);
        lagoon_text(renderer, ox, oy, unit, 197, y, binding, 0xf5f1df, false);
    }
    lagoon_text(renderer, ox, oy, unit, 9, 134, "Open Settings", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 134, 134, "Ctrl+,", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 197, 134, "RT", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9, 147, "Cycle view zoom", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 134, 147, "Ctrl+Z", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 197, 147, "LT", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9, 165, "Hold Sword and move into enemies to attack.", 0xedc977, false);
}

static void lagoon_draw_award_toast(SDL_Renderer *renderer) {
    if (rrhk_ui()->menu || rrhk_ui()->paused) return;
    Uint64 now = SDL_GetTicks64();
    if (now >= lagoon_award_until) {
        lagoon_showing_award = -1;
        if (lagoon_next_award >= lagoon_pending_count) return;
        lagoon_showing_award = lagoon_pending_awards[lagoon_next_award++];
        lagoon_award_until = now + 2900;
        lagoon_chime_frame = 0;
    }
    if (lagoon_showing_award < 0) return;
    int ox, oy, unit;
    rr_menu_layout(renderer, &ox, &oy, &unit);
    rr_menu_box(renderer, ox, oy, unit, 151, 5, 98, 24, 113, 113, 106, 255);
    rr_menu_box(renderer, ox, oy, unit, 152, 6, 96, 22, 8, 9, 13, 255);
    lagoon_text(renderer, ox, oy, unit, 156, 9, "Achievement unlocked", 0xedc977, false);
    lagoon_text(renderer, ox, oy, unit, 156, 18, lagoon_awards[lagoon_showing_award].title, 0xf5f1df, false);
}

static int lagoon_map_label_width(const char *label) {
    int width = 0;
    for (; *label; ++label) {
        unsigned glyph = *label >= 32 && *label <= 126 ? *label - 32 : '?' - 32;
        width += ui_small_advance[glyph];
    }
    return width;
}

static void lagoon_map_label(SDL_Renderer *renderer, int ox, int oy, int unit,
                            int x, int y, int available, const char *name) {
    char label[32];
    snprintf(label, sizeof label, "%s", name);
    int limit = (available * unit - 2) / SDL_max(1, unit / 2);
    if (lagoon_map_label_width(label) > limit) {
        size_t prefix = strlen(label) - 2;
        while (prefix > 0) {
            memcpy(label + --prefix, "...", 4);
            if (lagoon_map_label_width(label) <= limit) break;
        }
    }
    lagoon_text(renderer, ox, oy, unit, x, y, label, 0xf5f1df, false);
}

static void lagoon_map_player_marker(SDL_Renderer *renderer, int x, int y, int unit) {
    int radius = 4 * unit;
    SDL_SetRenderDrawColor(renderer, 8, 9, 13, 255);
    for (int dy = -radius; dy <= radius; ++dy) {
        int half = radius - abs(dy);
        SDL_RenderDrawLine(renderer, x - half, y + dy, x + half, y + dy);
    }
    radius -= unit;
    SDL_SetRenderDrawColor(renderer, 250, 246, 220, 255);
    for (int dy = -radius; dy <= radius; ++dy) {
        int half = radius - abs(dy);
        SDL_RenderDrawLine(renderer, x - half, y + dy, x + half, y + dy);
    }
}

static void lagoon_map_route_badge(SDL_Renderer *renderer, int ox, int oy, int unit,
                                   int x, int y, int size, int number) {
    char label[5];
    snprintf(label, sizeof label, "%d", number);
    rr_menu_box(renderer, ox, oy, unit, x - 1, y - 1, size + 2, size + 2, 8, 9, 13, 255);
    rr_menu_box(renderer, ox, oy, unit, x, y, size, size, 237, 201, 119, 255);
    int scale = SDL_max(1, unit / 2);
    int text_x = ox + x * unit + (size * unit - lagoon_map_label_width(label) * scale) / 2;
    int text_y = oy + y * unit + (size * unit - 12 * scale) / 2;
    lagoon_text(renderer, text_x, text_y, unit, 0, 0, label, 0x08090d, false);
}

static void lagoon_map_door_marker(SDL_Renderer *renderer, int x, int y, int unit) {
    SDL_Rect box = {x - 3 * unit, y - 4 * unit, 6 * unit, 8 * unit};
    SDL_SetRenderDrawColor(renderer, 8, 9, 13, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_Rect inner = {box.x + unit, box.y + unit, box.w - 2 * unit, box.h - 2 * unit};
    SDL_SetRenderDrawColor(renderer, 147, 218, 209, 255);
    SDL_RenderDrawRect(renderer, &inner);
}

static void lagoon_chart_draw_map(SDL_Renderer *renderer) {
    int width = 0, height = 0;
    if (SDL_GetRendererOutputSize(renderer, &width, &height) || width < 1 || height < 1) return;
    int unit = SDL_max(1, SDL_min(width / 256, height / 192));
    int span = SDL_clamp(width / unit, 256, 360);
    int depth = SDL_clamp(height / unit, 192, 224);
    int ox = (width - span * unit) / 2, oy = (height - depth * unit) / 2;
    int compact = unit == 1;
    int map_top = compact ? 37 : 28;
    int controls = depth - (compact ? 33 : 21);
    int frame_width = span - 109;
    int frame_height = controls - map_top - 5;
    int key_x = frame_width + 15;
    int badge = compact ? 15 : 8;
    bool current = lagoon_chart.shown == lagoon_chart.area;
    const char *status = current ? "Current area" : "Visited area";
    int status_width = (lagoon_map_label_width(status) * SDL_max(1, unit / 2) + unit - 1) / unit;

    lagoon_text(renderer, ox, oy, unit, 9, 1, lagoon_area_name(lagoon_chart.shown), 0xf5f1df, true);
    lagoon_text(renderer, ox, oy, unit, span - 9 - status_width, compact ? 4 : 6, status, 0xbdb8a7, false);
    lagoon_chart_tabs(renderer, ox, oy, unit, compact ? 21 : 15);
    if (!lagoon_chart_texture(renderer)) {
        rr_menu_text(renderer, ox, oy, unit, 9, map_top, "Map unavailable - Esc to close", 239, 198, 79);
        return;
    }

    SDL_Rect src = lagoon_map_source();
    SDL_Rect frame = {ox + 9 * unit, oy + map_top * unit, frame_width * unit, frame_height * unit};
    SDL_Rect dst = lagoon_map_letterbox(src, frame);
    SDL_Rect terrain = {src.x * LAGOON_MAP_DETAIL, src.y * LAGOON_MAP_DETAIL,
        src.w * LAGOON_MAP_DETAIL, src.h * LAGOON_MAP_DETAIL};
    rr_menu_box(renderer, ox, oy, unit, 8, map_top - 1, frame_width + 2, frame_height + 2, 112, 108, 97, 255);
    SDL_SetRenderDrawColor(renderer, 16, 22, 28, 255);
    SDL_RenderFillRect(renderer, &frame);
    SDL_SetTextureScaleMode(lagoon_chart.texture, dst.w < terrain.w ? SDL_ScaleModeLinear : SDL_ScaleModeNearest);
    SDL_RenderCopy(renderer, lagoon_chart.texture, &terrain, &dst);

    int legend_y = map_top + 7;
    lagoon_map_player_marker(renderer, ox + (key_x + 6) * unit, oy + (legend_y + 5) * unit, unit);
    lagoon_text(renderer, ox, oy, unit, key_x + 15, legend_y,
        current ? "Nasir" : "Last position", 0xf5f1df, false);
    int route_title = legend_y + (compact ? 21 : 16);
    if (current && lagoon_chart.door_count) {
        lagoon_map_door_marker(renderer, ox + (key_x + 6) * unit, oy + (route_title + 5) * unit, unit);
        lagoon_text(renderer, ox, oy, unit, key_x + 15, route_title, "Doorway", 0xf5f1df, false);
        route_title += compact ? 19 : 15;
        SDL_RenderSetClipRect(renderer, &frame);
        for (int i = 0; i < lagoon_chart.door_count; ++i) {
            SDL_Point point;
            if (lagoon_map_point(src, dst, lagoon_chart.doors[i].at, &point))
                lagoon_map_door_marker(renderer, point.x, point.y, unit);
        }
        SDL_RenderSetClipRect(renderer, NULL);
    }
    lagoon_text(renderer, ox, oy, unit, key_x, route_title, "Known exits", 0xedc977, true);
    int route_y = route_title + (compact ? 23 : 17);
    int row_height = badge + 4;
    int list_count = (controls - route_y - (compact ? 14 : 9)) / row_height;
    const LagoonRoute *route = lagoon_route_at(lagoon_chart.shown, 0);
    if (!route)
        lagoon_text(renderer, ox, oy, unit, key_x, route_y, "None recorded", 0xbdb8a7, false);

    SDL_Point player;
    bool show_player = lagoon_map_point(src, dst, lagoon_explored.visits[lagoon_chart.shown], &player);
    for (int number = 1; route; ++number) {
        SDL_Point point;
        if (lagoon_map_point(src, dst, route->at, &point)) {
            int x = SDL_clamp((point.x - ox) / unit - badge / 2, 10, 8 + frame_width - badge);
            int lean = show_player && abs(point.y - player.y) < (badge + 4) * unit &&
                abs(point.x - player.x) < (badge + 4) * unit ? badge + 5 : 0;
            int y = SDL_clamp((point.y - oy) / unit - badge / 2 - lean,
                map_top + 1, map_top + frame_height - badge - 1);
            if (lean && abs(oy + (y + badge / 2) * unit - player.y) < badge * unit)
                y = SDL_min((point.y - oy) / unit + 5, map_top + frame_height - badge - 1);
            SDL_SetRenderDrawColor(renderer, 8, 9, 13, 255);
            SDL_RenderDrawLine(renderer, point.x, point.y, ox + (x + badge / 2) * unit, oy + (y + badge / 2) * unit);
            lagoon_map_route_badge(renderer, ox, oy, unit, x, y, badge, number);
        }
        if (number <= list_count) {
            lagoon_map_route_badge(renderer, ox, oy, unit, key_x + 1, route_y, badge, number);
            lagoon_map_label(renderer, ox, oy, unit, key_x + badge + 5, route_y + (compact ? 1 : 2),
                span - key_x - badge - 14, lagoon_area_name(route->to));
            route_y += row_height;
        } else if (number == list_count + 1) {
            lagoon_text(renderer, ox, oy, unit, key_x, route_y, "See Routes for more", 0xbdb8a7, false);
        }
        route = lagoon_route_at(lagoon_chart.shown, number);
    }
    if (show_player) {
        SDL_RenderSetClipRect(renderer, &frame);
        lagoon_map_player_marker(renderer, player.x, player.y, unit);
        SDL_RenderSetClipRect(renderer, NULL);
    }

    int rail = (span - 18) / 3;
    int next = controls + (compact ? 15 : 8);
    rr_menu_box(renderer, ox, oy, unit, 9, controls - 3, span - 18, 1, 78, 80, 86, 255);
    lagoon_text(renderer, ox, oy, unit, 9, controls, compact ? "Pan: Pad/Arrows" : "Pan: Arrows / D-pad", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9, next, "Zoom: Z / Y", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9 + rail, controls, compact ? "Areas: LT/RT" : "Areas: PgUp/Dn / LT/RT", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9 + rail, next, compact ? "Here: Home / X" : "Current: Home / X", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9 + rail * 2, controls, "Tabs: Tab / LB/RB", 0xf5f1df, false);
    lagoon_text(renderer, ox, oy, unit, 9 + rail * 2, next, "Close: Esc / B", 0xf5f1df, false);
}

static void lagoon_chart_draw_settings(SDL_Renderer *renderer, int ox, int oy, int unit) {
    static const char *const labels[] = {"Volume", "Mute", "View zoom", "Quit to desktop"};
    const int scale = SDL_max(1, (unit + 1) / 2);
    char volume[16];
    snprintf(volume, sizeof volume, "%d%%", lagoon_sound_volume());
    for (int row = 0; row < LAGOON_SETTINGS_ROWS; ++row) {
        int y = 42 + row * 23;
        bool focused = row == lagoon_chart.settings_row;
        if (focused) {
            rr_menu_box(renderer, ox, oy, unit, 9, y - 4, 237, 20, 46, 40, 29, 255);
            rr_menu_box(renderer, ox, oy, unit, 9, y - 4, 2, 20, 237, 201, 119, 255);
        }
        lagoon_scaled_text(renderer, ox, oy, unit, 16, y, labels[row], focused ? 0xedc977 : 0xf5f1df, true, scale);
    }
    lagoon_scaled_text(renderer, ox, oy, unit, 198, 42, volume, 0xf5f1df, true, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 198, 65, rrhk_sound_muted() ? "On" : "Off", 0xf5f1df, true, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 198, 88, lagoon_zoom_labels[lagoon_zoom_mode], 0xf5f1df, true, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 9, 157, "D-pad / Arrows: adjust   A / Enter: select", 0xf5f1df, false, scale);
    if (!lagoon_chart.confirm_quit) return;
    rr_menu_box(renderer, ox, oy, unit, 0, 0, 256, 192, 7, 8, 12, 255);
    rr_menu_box(renderer, ox, oy, unit, 26, 47, 204, 98, 35, 33, 35, 255);
    lagoon_scaled_text(renderer, ox, oy, unit, 36, 57, "Quit Lagoon?", 0xf5f1df, true, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 36, 78, "Unsaved progress will be lost.", 0xf5f1df, false, scale);
    const bool quit = lagoon_chart.quit_selected;
    rr_menu_box(renderer, ox, oy, unit, quit ? 157 : 33, 104, quit ? 62 : 104, 18, 105, 79, 40, 255);
    lagoon_scaled_text(renderer, ox, oy, unit, 40, 109, "Keep playing", quit ? 0xbdb8a7 : 0xffffff, false, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 164, 109, "Quit", quit ? 0xffffff : 0xbdb8a7, false, scale);
    lagoon_scaled_text(renderer, ox, oy, unit, 36, 130, "A / Enter: confirm   B / Esc: back", 0xf5f1df, false, scale);
}

static void lagoon_chart_draw(SDL_Renderer *renderer) {
    if (!lagoon_chart.open) {
        lagoon_draw_award_toast(renderer);
        return;
    }
    SDL_SetRenderDrawColor(renderer, 8, 9, 13, 255);
    SDL_RenderClear(renderer);
    if (lagoon_chart.page == LAGOON_MAP) {
        lagoon_chart_draw_map(renderer);
        return;
    }
    int ox, oy, unit;
    rr_menu_layout(renderer, &ox, &oy, &unit);
    char area[16];
    snprintf(area, sizeof area, "Area %02d", lagoon_chart.shown);
    lagoon_text(renderer, ox, oy, unit, 9, 2, lagoon_area_name(lagoon_chart.shown), 0xf5f1df, true);
    lagoon_text(renderer, ox, oy, unit, 207, 6, area, 0xbdb8a7, false);
    lagoon_chart_tabs(renderer, ox, oy, unit, 15);
    lagoon_text(renderer, ox, oy, unit, 9, 180, "Tabs   Tab / LB/RB", 0xedc977, false);
    lagoon_text(renderer, ox, oy, unit, 169, 180, "Close   Esc / B", 0xbdb8a7, false);
    switch (lagoon_chart.page) {
    case LAGOON_ROUTES:
        lagoon_chart_draw_routes(renderer, ox, oy, unit);
        break;
    case LAGOON_ITEMS:
        lagoon_chart_draw_items(renderer, ox, oy, unit);
        break;
    case LAGOON_JOURNAL:
        lagoon_chart_draw_journal(renderer, ox, oy, unit);
        break;
    case LAGOON_TROPHIES:
        lagoon_chart_draw_awards(renderer, ox, oy, unit);
        break;
    case LAGOON_CONTROLS:
        lagoon_chart_draw_controls(renderer, ox, oy, unit);
        break;
    case LAGOON_SETTINGS:
        lagoon_chart_draw_settings(renderer, ox, oy, unit);
        break;
    }
}

/* Read-only test capture on a bounded headless route. */
#ifdef LAGOON_ATLAS_PREVIEW
static bool x_run_frame(uint16_t p1, uint16_t p2);
static bool lagoon_preview_key(SDL_Scancode key) {
    bool running = true;
    SDL_Event event = {0};
    event.type = SDL_KEYDOWN;
    event.key.keysym.scancode = key;
    if (key == SDL_SCANCODE_M || key == SDL_SCANCODE_COMMA) event.key.keysym.mod = KMOD_CTRL;
    return lagoon_chart_event(&event, &running) && running;
}

static bool lagoon_preview_image(SDL_Renderer *renderer, SDL_Surface *surface, const char *path) {
    lagoon_chart_draw(renderer);
    SDL_RenderPresent(renderer);
    return SDL_SaveBMP(surface, path) == 0;
}

static bool lagoon_preview_settings_size(const char *path, int width, int height) {
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return false;
    SDL_Renderer *renderer = SDL_CreateSoftwareRenderer(surface);
    bool result = renderer && lagoon_preview_image(renderer, surface, path);
    if (renderer) SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    return result;
}

static bool lagoon_check_map_geometry(void) {
    SDL_Rect frame = {4, 8, 580, 580};
    SDL_Rect keep_church = lagoon_area_bounds[3];
    SDL_Point point;
    for (int area = 1; area < LAGOON_AREA_COUNT; ++area) {
        if (!lagoon_valid_area(area)) continue;
        SDL_Rect src = lagoon_area_bounds[area];
        if (src.w <= 0 || src.h <= 0 || src.x < 0 || src.y < 0 ||
            src.x + src.w > LAGOON_MAP_SIZE || src.y + src.h > LAGOON_MAP_SIZE ||
            src.w % 4 || src.h % 4) return false;
        SDL_Rect dst = lagoon_map_letterbox(src, frame);
        int error = abs(dst.w * src.h - dst.h * src.w);
        LagoonPosition center = {(src.x + src.w / 2) * 4, (src.y + src.h / 2) * 4};
        if (error >= SDL_max(src.w, src.h) || !lagoon_map_point(src, dst, center, &point) ||
            point.x != dst.x + dst.w / 2 || point.y != dst.y + dst.h / 2) return false;
    }
    // An observed position in blank terrain extends the crop.
    LagoonPosition outside_art = {2047, 2047};
    lagoon_include_position(&keep_church, outside_art);
    return lagoon_map_point(keep_church, frame, outside_art, &point) &&
        lagoon_area_bounds[1].w < LAGOON_MAP_SIZE &&
        lagoon_area_bounds[3].w < lagoon_area_bounds[3].h &&
        lagoon_area_bounds[7].w == lagoon_area_bounds[7].h &&
        lagoon_area_bounds[7].w < lagoon_area_bounds[3].h;
}

static bool lagoon_preview_arena(SDL_Renderer *renderer, SDL_Surface *surface) {
    // Fixture for the compact arena map. The actual tour only visits Atland and the Church.
    int shown = lagoon_chart.shown, page = lagoon_chart.page;
    LagoonPosition last = lagoon_explored.visits[7];
    lagoon_chart.shown = 7;
    lagoon_chart.page = LAGOON_MAP;
    lagoon_explored.visits[7] = (LagoonPosition){128, 128};
    bool result = lagoon_preview_image(renderer, surface, "samson-atlas-fixture.bmp") &&
        lagoon_chart.painted == 7;
    lagoon_explored.visits[7] = last;
    lagoon_chart.shown = shown;
    lagoon_chart.page = page;
    return result;
}

static bool lagoon_check_later_thresholds(void) {
    static const int cases[][3] = {{2, 4, 3}, {2, 5, 4}, {5, 15, 6}, {10, 30, 8}};
    // Fixture values exercise the catalog in the preview build and stay out of the profile.
    lagoon_awards_path[0] = 0;
    for (unsigned i = 0; i < sizeof cases / sizeof *cases; ++i) {
        lagoon_journal.level = cases[i][0];
        lagoon_journal.areas = cases[i][1];
        lagoon_check_awards();
        lagoon_check_awards();
        if (lagoon_pending_count != cases[i][2]) return false;
    }
    return true;
}

static bool lagoon_chart_preview(const char *path, bool tour) {
    bool running = true, result = false;
    SDL_Event toggle = {0};
    toggle.type = SDL_KEYDOWN;
    toggle.key.keysym.scancode = SDL_SCANCODE_M;
    toggle.key.keysym.mod = KMOD_CTRL;
    lagoon_note_position();
    if (!lagoon_chart_event(&toggle, &running) || !lagoon_chart.open) return false;
    uint64_t before = input_frame;
    if (!x_run_frame(0, 0) || input_frame != before) return false;
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, 1280, 720, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_Renderer *renderer = surface ? SDL_CreateSoftwareRenderer(surface) : NULL;
    if (renderer) {
        rr_menu_init(renderer);
        result = true;
        if (tour) {
            const LagoonRoute *route = lagoon_route_at(1, 0);
            result = lagoon_chart.area == 3 && route && route->to == 3 &&
                lagoon_preview_key(SDL_SCANCODE_PAGEUP) && lagoon_chart.shown == 1;
        }
        result = lagoon_preview_image(renderer, surface, path) && lagoon_chart.painted == lagoon_chart.shown &&
            lagoon_check_map_geometry() && lagoon_chart.door_count > 0 && result;
        int unlocked = 0;
        for (int i = 0; i < LAGOON_AWARD_COUNT; ++i) unlocked += lagoon_unlocks[i] != 0;
        fprintf(stderr, "Atlas area=%d x=%d y=%d routes=%u awards=%d pending=%d level=%d hp=%d mp=%d\n", lagoon_chart.area,
                lagoon_chart.at.x, lagoon_chart.at.y, lagoon_explored.route_count, unlocked,
                lagoon_pending_count, lagoon_journal.level, lagoon_journal.hp, lagoon_journal.mp);
        SDL_Event zoom = toggle;
        zoom.key.keysym.scancode = SDL_SCANCODE_Z;
        zoom.key.keysym.mod = 0;
        result = lagoon_chart_event(&zoom, &running) && lagoon_chart.zoom == 2 && result;
        result = lagoon_chart_event(&zoom, &running) && lagoon_chart.zoom == 4 && result;
        result = lagoon_chart_event(&zoom, &running) && lagoon_chart.zoom == 1 && result;
        if (tour) {
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_chart.page == LAGOON_ROUTES &&
                lagoon_preview_image(renderer, surface, "routes.bmp") && result;
            result = lagoon_preview_key(SDL_SCANCODE_RETURN) && lagoon_chart.page == LAGOON_MAP &&
                lagoon_chart.shown == 3 && lagoon_chart.zoom == 1 &&
                lagoon_preview_image(renderer, surface, "church.bmp") && result;
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_preview_key(SDL_SCANCODE_TAB) &&
                lagoon_chart.page == LAGOON_ITEMS && lagoon_preview_image(renderer, surface, "items.bmp") && result;
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_chart.page == LAGOON_JOURNAL &&
                lagoon_journal.level == 1 && lagoon_journal.max_hp == 18 && lagoon_journal.max_mp == 3 &&
                lagoon_preview_image(renderer, surface, "journal.bmp") && result;
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_chart.page == LAGOON_TROPHIES &&
                unlocked == 2 && lagoon_preview_image(renderer, surface, "awards.bmp") &&
                lagoon_preview_arena(renderer, surface) && result;
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_chart.page == LAGOON_CONTROLS &&
                lagoon_preview_image(renderer, surface, "controls.bmp") && result;
            result = lagoon_preview_key(SDL_SCANCODE_TAB) && lagoon_chart.page == LAGOON_SETTINGS &&
                lagoon_preview_image(renderer, surface, "settings.bmp") &&
                lagoon_preview_settings_size("settings-narrow.bmp", 854, 480) &&
                lagoon_preview_settings_size("settings-ultrawide.bmp", 2560, 1080) && result;
            result = lagoon_preview_key(SDL_SCANCODE_DOWN) && lagoon_preview_key(SDL_SCANCODE_DOWN) &&
                lagoon_preview_key(SDL_SCANCODE_DOWN) && lagoon_preview_key(SDL_SCANCODE_RETURN) && lagoon_chart.confirm_quit &&
                !lagoon_chart.quit_selected && lagoon_preview_image(renderer, surface, "quit.bmp") &&
                lagoon_preview_key(SDL_SCANCODE_RETURN) && !lagoon_chart.confirm_quit && result;
            result = lagoon_preview_key(SDL_SCANCODE_DOWN) && lagoon_preview_key(SDL_SCANCODE_RETURN) &&
                lagoon_preview_key(SDL_SCANCODE_RIGHT) && lagoon_chart.quit_selected && result;
            SDL_Event confirm = {0};
            confirm.type = SDL_KEYDOWN;
            confirm.key.keysym.scancode = SDL_SCANCODE_RETURN;
            bool quit_running = true;
            result = lagoon_chart_event(&confirm, &quit_running) && !quit_running && !lagoon_chart.open && result;
        }
        if (lagoon_chart.open)
            result = lagoon_chart_event(&toggle, &running) && !lagoon_chart.open && result;
        if (tour) {
            int16_t pcm[512] = {0};
            lagoon_draw_award_toast(renderer);
            lagoon_mix_chime(pcm, sizeof pcm / sizeof *pcm);
            bool nonzero = false;
            for (int i = 0; i < 512; i += 2) {
                nonzero |= pcm[i] != 0;
                result = (pcm[i] == pcm[i + 1]) && result;
            }
            result = nonzero && lagoon_next_award == 1 && lagoon_check_later_thresholds() && result;
        }
        rr_menu_shutdown();
        SDL_DestroyRenderer(renderer);
    }
    if (surface) SDL_FreeSurface(surface);
    return result;
}
#endif
