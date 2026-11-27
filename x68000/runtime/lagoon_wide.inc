static const uint32_t *x_pixels(void);
static int x_width(void);

/* The original raster sits at a stable world origin in each canvas. */
static uint32_t lagoon_wide_frame[X_MAX_WIDTH * LAGOON_MAX_VIEW_H];
static bool lagoon_zoom_ready;

static SDL_Rect lagoon_world_view(void) {
    static const SDL_Rect modes[] = {
        {-LAGOON_FLANK, 0, LAGOON_WIDE, LAGOON_VIEW}, {-156, -32, 568, 320}, {-214, -64, 684, 384}
    };
    /* A quick-state load occurs inside host_frame, after the epoch hook. */
    if (!lagoon_zoom_ready && core.get_memory_data && core.get_memory_size)
        lagoon_zoom_ready = lagoon_scene_view(core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM),
                                              core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM)).area != 0;
    return modes[lagoon_zoom_ready ? lagoon_zoom_mode : 0];
}

static struct {
    LagoonWorld world;
    LagoonExtendedActors actors;
    bool valid, favored;
} lagoon_before_picture;

static struct {
    uint32_t pixels[X_MAX_WIDTH * LAGOON_MAX_VIEW_H];
    uint32_t center[LAGOON_VIEW * LAGOON_VIEW];
    int width;
    unsigned area;
    uint64_t frame, inverted_palette_hash;
    bool valid;
} lagoon_scene_backdrop;
static uint64_t lagoon_last_door_frame;

static void lagoon_invalidate_view(void) {
    lagoon_before_picture.valid = false;
    lagoon_before_picture.favored = false;
    lagoon_scene_backdrop.valid = false;
    lagoon_last_door_frame = 0;
}

static void lagoon_capture_world_epoch(void) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    size_t ram_size = core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    lagoon_before_picture.valid = lagoon_world_begin(&lagoon_before_picture.world, ram, ram_size);
    /* A restored dialogue can precede the first terrain admission. Keep the
     * selected canvas from that first frame; an absent world epoch must not
     * briefly reset a valid in-game scene to the 1x canvas. */
    lagoon_zoom_ready |= lagoon_before_picture.valid || lagoon_scene_view(ram, ram_size).area != 0;
    if (!lagoon_before_picture.valid && lagoon_scene_view(ram, ram_size).door)
        lagoon_last_door_frame = input_frame;
    if (lagoon_before_picture.valid)
        lagoon_capture_extended_actors(&lagoon_before_picture.world, ram_size, &lagoon_before_picture.actors);
}

static bool lagoon_has_wide_canvas(void) {
    /* The original prologue also uses256x256. It is a square film, not a
     * gameplay world with extra margins. Warm resets also retain old RAM. */
    if (width != LAGOON_VIEW || height != LAGOON_VIEW || lagoon_boot.active) return false;
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    size_t size = core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    return ram && size > LAGOON_BASE + 0xeccc && lagoon_word(ram, 0) == 0x6100 &&
        lagoon_word(ram, 2) == 0x5efe && lagoon_word(ram, 4) == 0x6100 &&
        lagoon_word(ram, 6) == 0x5bd4 &&
        (lagoon_zoom_ready || lagoon_scene_view(ram, size).area != 0);
}

static int x_height(void);

static bool lagoon_default_window_size(int usable_w, int usable_h, int *w, int *h) {
    if (usable_w <= 96 || usable_h <= 96) return false;
    int max_w = usable_w - 64, max_h = usable_h - 64;
    *w = SDL_min(1280, max_w);
    *h = *w * 9 / 16;
    if (*h > max_h) { *h = max_h; *w = *h * 16 / 9; }
    return *w > 0 && *h > 0;
}

static void lagoon_frame_start(void *window, void *renderer) {
    bool fresh = !ca.started;
    if (fresh) lagoon_zoom_mode = lagoon_load_zoom();
    /* A saved window is the player's preference. Never replace it, and do
     * not change an existing fullscreen setting while improving defaults. */
    FILE *saved = fresh ? fopen(player_config, "rb") : NULL;
    bool has_saved_window = saved != NULL;
    if (saved) fclose(saved);
    x_frame_start(window, renderer);
    if (fresh && !has_saved_window && window &&
        !(SDL_GetWindowFlags((SDL_Window *)window) & SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_Rect usable;
        int display = SDL_GetWindowDisplayIndex((SDL_Window *)window), w, h;
        if (display >= 0 && !SDL_GetDisplayUsableBounds(display, &usable) &&
            lagoon_default_window_size(usable.w, usable.h, &w, &h)) {
            SDL_SetWindowMinimumSize((SDL_Window *)window, SDL_min(400, w), SDL_min(340, h));
            SDL_SetWindowSize((SDL_Window *)window, w, h);
            SDL_SetWindowPosition((SDL_Window *)window, usable.x + (usable.w-w)/2, usable.y + (usable.h-h)/2);
        }
    }
}

static int lagoon_picture_width(void) {
    return lagoon_has_wide_canvas() ? lagoon_world_view().w : x_width();
}

static int lagoon_picture_height(void) {
    return lagoon_has_wide_canvas() ? lagoon_world_view().h : x_height();
}

static bool lagoon_opening_rect(int canvas_w, int canvas_h, int native_w, int native_h, int mode, SDL_Rect *rect) {
    if (canvas_w < 1 || canvas_h < 1 || native_w < 1 || native_h < 1) return false;
    int w = canvas_w, h = canvas_h;
    if ((int64_t)w * native_h > (int64_t)h * native_w) w = (int)((int64_t)h * native_w / native_h);
    else h = (int)((int64_t)w * native_h / native_w);
    if (mode == 1) {
        int scale = SDL_min(canvas_w/native_w, canvas_h/native_h);
        if (scale >= 1) { w = native_w*scale; h = native_h*scale; }
    }
    *rect = (SDL_Rect){(canvas_w-w)/2, (canvas_h-h)/2, w, h};
    return true;
}

static bool lagoon_picture_rect(int canvas_w, int canvas_h, int mode, SDL_Rect *rect) {
    if (!lagoon_has_wide_canvas()) {
        if ((lagoon_boot.active && lagoon_boot.stage != LAGOON_BOOT_GAME) || !lagoon_zoom_ready)
            return lagoon_opening_rect(canvas_w, canvas_h, x_width(), x_height(), mode, rect);
        return x_game_rect(canvas_w, canvas_h, mode, rect);
    }
    if (canvas_w < 1 || canvas_h < 1) return false;
    SDL_Rect view = lagoon_world_view();
    int scale = SDL_min(canvas_w / view.w, canvas_h / view.h);
    int w = canvas_w, h = canvas_h;
    if (mode != 2 && !view.y && scale >= 1) {
        w = view.w * scale;
        h = view.h * scale;
    } else if ((int64_t)w * view.h > (int64_t)h * view.w) {
        w = h * view.w / view.h;
    } else {
        h = w * view.h / view.w;
    }
    *rect = (SDL_Rect){(canvas_w - w) / 2, (canvas_h - h) / 2, w, h};
    return true;
}

static bool lagoon_near_color(uint32_t a, uint32_t b) {
    return abs((int)(a >> 16 & 255u) - (int)(b >> 16 & 255u)) <= 4 &&
           abs((int)(a >> 8 & 255u) - (int)(b >> 8 & 255u)) <= 4 &&
           abs((int)(a & 255u) - (int)(b & 255u)) <= 4;
}

static unsigned lagoon_terrain_agreement(const LagoonWorld *world, const uint32_t *picture,
                                         const bool *visible_rows) {
    unsigned match = 0, lit = 0, checked = 0;
    for (int y = 5; y < LAGOON_VIEW; y += 11) {
        if (visible_rows && !visible_rows[y]) continue;
        for (int x = 3; x < LAGOON_VIEW; x += 9) {
            uint32_t native = picture[y*X_MAX_WIDTH + x] & 0x00ffffffu;
            uint32_t sample = lagoon_world_pixel(world, x, y);
            checked++;
            lit += native != 0;
            match += lagoon_near_color(native, sample);
        }
    }
    return lit >= 32 ? match * 100 / checked : 0;
}

static bool lagoon_try_staggered_world(LagoonWorld *world, const LagoonWorld *previous,
                                      const uint32_t *picture, const bool *visible_rows) {
    /* The Castle updates foreground patterns on frames whose picture can
     * still use the previous scroll. Reuse only a genuine adjacent camera,
     * and require stronger pixel agreement than the standard selector. */
    int dx = world->origin_x - previous->origin_x;
    int dy = (int)world->y[0] - (int)previous->y[0];
    if (world->room_left != previous->room_left || world->room_right != previous->room_right ||
        world->bg != previous->bg || world->graphics != previous->graphics ||
        world->parallax != previous->parallax || abs(dx) + abs(dy) > 3 || (!dx && !dy))
        return false;
    world->origin_x = previous->origin_x;
    world->origin_y = previous->origin_y;
    memcpy(world->x, previous->x, sizeof world->x);
    memcpy(world->y, previous->y, sizeof world->y);
    if (lagoon_terrain_agreement(world, picture, visible_rows) < 90) return false;
    /* Preserve actor geometry from the matching camera. */
    world->part_count = previous->part_count;
    memcpy(world->parts, previous->parts, world->part_count * sizeof world->parts[0]);
    world->staggered = true;
    return true;
}

static uint64_t lagoon_palette_hash(bool inverted) {
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned i = 0; i < 512; ++i) {
        unsigned word = i < 256 ? TextPal[i] : GrphPal[i - 256];
        if (inverted) word ^= 0xffffu;
        hash = (hash ^ (word & 255u)) * UINT64_C(1099511628211);
        hash = (hash ^ (word >> 8)) * UINT64_C(1099511628211);
    }
    return hash;
}

static bool lagoon_admit_palette_flash(LagoonWorld *world, const uint32_t *picture, unsigned area) {
    /* $10DE2/$10DFC invert all 512 graphics/text palette words. Palette zero
     * becomes opaque white in the original compositor. Recognize the actual
     * complete palette change, not a guessed RGB transform of the cached map. */
    if (!lagoon_scene_backdrop.valid || lagoon_scene_backdrop.area != area ||
        world->area != area || lagoon_scene_backdrop.width != lagoon_world_view().w ||
        input_frame < lagoon_scene_backdrop.frame || input_frame - lagoon_scene_backdrop.frame > 3 ||
        world->graphics_colors[0] != 0xffffu ||
        lagoon_palette_hash(false) != lagoon_scene_backdrop.inverted_palette_hash) return false;
    uint32_t color = lagoon_world_argb(world->graphics_colors[0]);
    unsigned uniform = 0, checked = 0;
    for (int y = 5; y < LAGOON_VIEW; y += 11) {
        if (y >= LAGOON_HUD_TOP && y < LAGOON_HUD_END) continue;
        for (int x = 3; x < LAGOON_VIEW; x += 9) {
            uniform += lagoon_near_color(picture[y*X_MAX_WIDTH+x], color);
            ++checked;
        }
    }
    if (uniform * 100 < checked * 90) return false;
    world->palette_flash = true;
    world->flash_color = color;
    return true;
}

static bool lagoon_select_world(const uint32_t *picture, unsigned area, bool modal,
                                LagoonWorld *world, bool *earlier, const bool *visible_rows) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    const bool current = lagoon_world_begin(world, ram, core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM));
    const LagoonWorld *previous = &lagoon_before_picture.world;
    const bool same_area = !modal && lagoon_before_picture.valid && previous->area == area;
    unsigned score = current ? lagoon_terrain_agreement(world, picture, visible_rows) : 0;
    const unsigned old_score = same_area ? lagoon_terrain_agreement(previous, picture, visible_rows) : 0;
    if (current && !modal && score < 72 && lagoon_admit_palette_flash(world, picture, area)) return true;
    if (current && same_area && score < 72 && old_score < 72 &&
        lagoon_try_staggered_world(world, previous, picture, visible_rows)) {
        *earlier = lagoon_before_picture.favored = true;
        return true;
    }
    *earlier = old_score > score || (same_area && old_score == score && lagoon_before_picture.favored);
    if (*earlier) {
        *world = *previous;
        score = old_score;
    }
    lagoon_before_picture.favored = *earlier;
    return score >= 72;
}

static uint32_t lagoon_shade_color(uint32_t color, unsigned shade) {
    unsigned r = (color >> 16 & 255u) * shade / 256;
    unsigned g = (color >> 8 & 255u) * shade / 256;
    unsigned b = (color & 255u) * shade / 256;
    return 0xff000000u | r << 16 | g << 8 | b;
}

static unsigned lagoon_color_energy(uint32_t color) {
    return (color >> 16 & 255u) + (color >> 8 & 255u) + (color & 255u);
}

typedef struct { int shade, dx, dy; } LagoonBackdropMatch;

static uint32_t lagoon_saved_color(int x, int y, SDL_Rect view, LagoonBackdropMatch match) {
    x = SDL_clamp(x + match.dx, 0, view.w - 1);
    y = SDL_clamp(y + match.dy, 0, view.h - 1);
    return lagoon_shade_color(lagoon_scene_backdrop.pixels[y*X_MAX_WIDTH + x], (unsigned)match.shade);
}

static unsigned lagoon_backdrop_score(const uint32_t *picture, LagoonBackdropMatch match, int first_row) {
    unsigned agrees = 0, checked = 0;
    for (int y = first_row; y < LAGOON_VIEW; y += 11) {
        for (int x = 3; x < LAGOON_VIEW; x += 9) {
            int sx = SDL_clamp(x + match.dx, 0, LAGOON_VIEW - 1);
            int sy = SDL_clamp(y + match.dy, 0, LAGOON_VIEW - 1);
            uint32_t color = lagoon_scene_backdrop.center[sy*LAGOON_VIEW + sx];
            agrees += lagoon_near_color(picture[y*X_MAX_WIDTH + x], lagoon_shade_color(color, (unsigned)match.shade));
            checked++;
        }
    }
    return agrees * 100 / checked;
}

static LagoonBackdropMatch lagoon_match_backdrop(const uint32_t *picture, LagoonSceneView scene, bool short_gap) {
    LagoonBackdropMatch match = {-1, 0, 0};
    const bool same_area = scene.area == lagoon_scene_backdrop.area;
    if (!lagoon_scene_backdrop.valid || lagoon_scene_backdrop.width != lagoon_world_view().w || (!same_area && !scene.door) ||
        (!scene.door && !scene.modal && !short_gap)) return match;
    if (short_gap && (input_frame < lagoon_scene_backdrop.frame ||
        input_frame - lagoon_scene_backdrop.frame > 3)) return match;
    unsigned before = 0, after = 0;
    if (scene.door) {
        for (int y = 5; y < LAGOON_VIEW; y += 11) {
            for (int x = 3; x < LAGOON_VIEW; x += 9) {
                before += lagoon_color_energy(lagoon_scene_backdrop.center[y*LAGOON_VIEW + x]);
                after += lagoon_color_energy(picture[y*X_MAX_WIDTH + x]);
            }
        }
        if (!before) return match;
    }
    /* Doors fade the same backdrop out. Text overlays keep its palette. */
    match.shade = scene.door ? (int)SDL_min(256u, (after * 256u + before / 2) / before) : 256;
    /* Original shop and house text can cover most of the center. Align to
     * the gameplay strip below the boxes before reusing the town margins. */
    const int first_row = scene.dialogue ? 214 : 5;
    unsigned best = lagoon_backdrop_score(picture, match, first_row);
    if ((scene.door || scene.dialogue || short_gap) && best < 90 && match.shade > 0) {
        for (int d = -3; d <= 3; ++d) {
            if (!d) continue;
            LagoonBackdropMatch horizontal = {match.shade, d, 0};
            unsigned score = lagoon_backdrop_score(picture, horizontal, first_row);
            if (score > best) { best = score; match = horizontal; }
            LagoonBackdropMatch vertical = {match.shade, 0, d};
            score = lagoon_backdrop_score(picture, vertical, first_row);
            if (score > best) { best = score; match = vertical; }
        }
    }
    if (best < (scene.dialogue ? 80u : scene.modal ? 68u : short_gap ? 85u : 72u)) match.shade = -1;
    return match;
}

typedef struct {
    unsigned crossing, extended, enemy, hud, restored;
    bool backdrop, masked_world, live, staggered;
} LagoonWideFrameInfo;

extern uint8_t TextDrawWork[1024 * 1024];
extern uint32_t TextScrollX, TextScrollY;

static const uint32_t *lagoon_native_row(int y, SDL_Rect view) {
    return lagoon_wide_frame + (y - view.y)*X_MAX_WIDTH - view.x;
}

static unsigned lagoon_place_zoom_hud(const uint32_t *picture, const LagoonWorld *world,
                                      SDL_Rect view, bool live, bool cache, LagoonBackdropMatch backdrop,
                                      unsigned *restored) {
    if (!view.y || (!live && backdrop.shade != 256) || !(VCReg2[1] & 0x20u) ||
        (CRTC_Regs[0x29] & 0x1c) == 0x1c) return 0;
    unsigned pixels = 0;
    /* Use the hardware text mask and its displayed color. Keep the original
     * HP/MP art and values, and avoid moving any terrain or sprite pixels. */
    for (int y = LAGOON_HUD_TOP; y < LAGOON_HUD_END; ++y) {
        size_t row = ((TextScrollY + y) & 1023u) * 1024u;
        uint32_t *from = lagoon_wide_frame + (y - view.y)*X_MAX_WIDTH - view.x;
        uint32_t *to = from - view.y*X_MAX_WIDTH;
        for (int x = 0; x < LAGOON_VIEW; ++x) {
            unsigned index = TextDrawWork[row + ((TextScrollX + x) & 1023u)] & 15u;
            uint32_t pixel = picture[y*X_MAX_WIDTH + x];
            /* During the validated hardware palette strobe, displayed text
             * color need not equal the ordinary palette conversion. Its
             * hardware mask still identifies the exact original HUD pixels. */
            if (!index || (!(live && world->palette_flash) &&
                !lagoon_near_color(pixel, lagoon_world_argb(TextPal[index])))) continue;
            from[x] = live ? lagoon_world_hud_underlay(world, x, y, restored) :
                lagoon_saved_color(x - view.x, y - view.y, view, backdrop);
            if (cache)
                lagoon_scene_backdrop.pixels[(y - view.y)*X_MAX_WIDTH + x - view.x] = from[x];
            to[x] = pixel;
            ++pixels;
        }
    }
    return pixels;
}

static bool lagoon_compose_picture(const uint32_t *picture, LagoonWideFrameInfo *info) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    const size_t ram_size = core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    LagoonSceneView scene = lagoon_scene_view(ram, ram_size);
    if (scene.door) lagoon_last_door_frame = input_frame;
    const bool revealing = lagoon_last_door_frame && input_frame >= lagoon_last_door_frame &&
        input_frame - lagoon_last_door_frame <= 2;
    LagoonBackdropMatch backdrop = {-1, 0, 0};
    bool reveal_rows[LAGOON_VIEW], *rows = NULL;
    if (revealing && !scene.modal) {
        unsigned lit = 0;
        for (int y = 0; y < LAGOON_VIEW; ++y) {
            unsigned visible = 0;
            for (int x = 2; x < LAGOON_VIEW; x += 8)
                visible += lagoon_color_energy(picture[y*X_MAX_WIDTH + x]) > 48;
            reveal_rows[y] = visible >= 4;
            lit += reveal_rows[y];
        }
        if (lit == 0) backdrop.shade = 0;
        else if (lit < LAGOON_VIEW - 8) rows = reveal_rows;
    }
    if (backdrop.shade < 0) backdrop = lagoon_match_backdrop(picture, scene, false);

    LagoonWorld world;
    bool earlier = false;
    const bool live = backdrop.shade < 0 && lagoon_select_world(picture, scene.area, scene.modal, &world, &earlier, rows);
    if (!live && backdrop.shade < 0 && !scene.modal && !scene.door && !revealing)
        backdrop = lagoon_match_backdrop(picture, scene, true);
    SDL_Rect view = lagoon_world_view();
    for (int y = view.y; y < view.y + view.h; ++y) {
        uint32_t *row = lagoon_wide_frame + (y - view.y)*X_MAX_WIDTH;
        if ((unsigned)y < LAGOON_VIEW)
            memcpy(row - view.x, picture + y*X_MAX_WIDTH, LAGOON_VIEW * sizeof *row);
        bool visible = !rows || rows[SDL_clamp(y, 0, LAGOON_VIEW - 1)];
        for (int x = view.x; x < view.x + view.w; ++x) {
            if (lagoon_inside_native(x, y)) continue;
            row[x - view.x] = visible && live ? lagoon_world_pixel(&world, x, y)
                : visible && backdrop.shade > 0 ? lagoon_saved_color(x - view.x, y - view.y, view, backdrop)
                : 0xff000000u;
        }
    }

    unsigned extended = 0, crossing = 0, enemy = 0, hud = 0, restored = 0;
    if (live) {
        const LagoonExtendedActors *actors = &lagoon_before_picture.actors;
        if (!earlier) {
            static LagoonExtendedActors current;
            lagoon_capture_extended_actors(&world, ram_size, &current);
            actors = &current;
        }
        extended = lagoon_draw_extended_actors(&world, actors, lagoon_wide_frame, X_MAX_WIDTH, view, &enemy);
        crossing = lagoon_world_draw_crossing_parts(&world, lagoon_wide_frame, X_MAX_WIDTH, view);
    }
    /* A partial arrival can draw only the rows already revealed by the
     * guest. Mask completed sprites along with their scenery on these rows. */
    if (rows) {
        for (int y = view.y; y < view.y + view.h; ++y) {
            if (rows[SDL_clamp(y, 0, LAGOON_VIEW - 1)]) continue;
            uint32_t *row = lagoon_wide_frame + (y - view.y)*X_MAX_WIDTH;
            for (int x = view.x; x < view.x + view.w; ++x)
                if (!lagoon_inside_native(x, y)) row[x - view.x] = 0xff000000u;
        }
    }
    const bool stable_world = live && !world.palette_flash && !scene.modal && !scene.door && !rows;
    if (stable_world) {
        for (int y = 0; y < view.h; ++y)
            memcpy(lagoon_scene_backdrop.pixels + y*X_MAX_WIDTH, lagoon_wide_frame + y*X_MAX_WIDTH,
                view.w * sizeof(uint32_t));
        for (int y = 0; y < LAGOON_VIEW; ++y)
            memcpy(lagoon_scene_backdrop.center + y*LAGOON_VIEW, picture + y*X_MAX_WIDTH,
                LAGOON_VIEW * sizeof(uint32_t));
        lagoon_scene_backdrop.area = world.area;
        lagoon_scene_backdrop.width = view.w;
        lagoon_scene_backdrop.frame = input_frame;
        lagoon_scene_backdrop.inverted_palette_hash = lagoon_palette_hash(true);
        lagoon_scene_backdrop.valid = true;
    }
    if (!scene.modal && !scene.door && !rows && (live || backdrop.shade == 256))
        hud = lagoon_place_zoom_hud(picture, &world, view, live, stable_world, backdrop, &restored);
    if (info) *info = (LagoonWideFrameInfo){
        crossing, extended, enemy, hud, restored, backdrop.shade > 0, live && rows != NULL, live, live && world.staggered
    };
    return live || backdrop.shade >= 0;
}

static const uint32_t *lagoon_picture(void) {
    const uint32_t *native = x_pixels();
    if (!native || !lagoon_has_wide_canvas()) {
        lagoon_invalidate_view();
        return native;
    }
    lagoon_compose_picture(native, NULL);
    return lagoon_wide_frame;
}

static void lagoon_draw_overlays(SDL_Renderer *renderer) {
    lagoon_chart_draw(renderer);
    lagoon_boot_draw(renderer);
}

static uint32_t lagoon_wide_reference_pixel(size_t i) {
    if (pixel_format == RETRO_PIXEL_FORMAT_RGB565) {
        uint16_t v;
        memcpy(&v, last_pixels + 2*i, sizeof v);
        return 0xff000000u | (((v >> 11) & 31) * 255 / 31 << 16) |
            (((v >> 5) & 63) * 255 / 63 << 8) | ((v & 31) * 255 / 31);
    }
    uint32_t pixel;
    memcpy(&pixel, last_pixels + 4*i, sizeof pixel);
    return pixel | 0xff000000u;
}

#ifdef LAGOON_ATLAS_PREVIEW
static bool lagoon_capture_wide(const char *path, bool require_crossing, bool require_extended) {
    if (!lagoon_has_wide_canvas() || !last_pixels || last_pixels_size < LAGOON_VIEW*LAGOON_VIEW*2u) return false;
    uint32_t *center = (uint32_t *)calloc(X_MAX_WIDTH * LAGOON_VIEW, sizeof *center);
    if (!center) return false;
    for (int y = 0; y < LAGOON_VIEW; ++y) for (int x = 0; x < LAGOON_VIEW; ++x)
        center[y*X_MAX_WIDTH + x] = lagoon_wide_reference_pixel((size_t)y*LAGOON_VIEW + x);
    LagoonWideFrameInfo info;
    bool ok = lagoon_compose_picture(center, &info);
    unsigned crossing = info.crossing, extended = info.extended;
    unsigned left = 0, right = 0;
    SDL_Rect view = lagoon_world_view();
    for (int y = 0; y < LAGOON_VIEW; ++y) {
        const uint32_t *actual = lagoon_native_row(y, view);
        if (!info.hud || y < LAGOON_HUD_TOP || y >= LAGOON_HUD_END)
            ok = ok && !memcmp(center + y*X_MAX_WIDTH, actual, LAGOON_VIEW * sizeof *actual);
        for (int x = view.x; x < 0; ++x) left += actual[x] != 0xff000000u;
        for (int x = LAGOON_VIEW; x < view.x + view.w; ++x) right += actual[x] != 0xff000000u;
    }
    free(center);
    if (!ok || left < 1024 || right < 1024 || (require_crossing && crossing < 20) ||
        (require_extended && extended < 20)) {
        fprintf(stderr, "Widescreen qualification failed (admission %d, left %u, right %u, crossing %u, extended %u)\n",
                ok, left, right, crossing, extended);
        return false;
    }
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormatFrom(lagoon_wide_frame,
        view.w, view.h, 32, X_MAX_WIDTH * sizeof(uint32_t), SDL_PIXELFORMAT_ARGB8888);
    bool result = surface && SDL_SaveBMP(surface, path) == 0;
    if (surface) SDL_FreeSurface(surface);
    if (result) fprintf(stderr, "Wide %dx%d verified (%u,%u); %u crossing, %u extended actor, %u enemy and %u HUD pixels\n",
                        view.w, view.h, left, right, crossing, extended, info.enemy, info.hud);
    return result;
}

/* Report every guest frame in a route without retaining pixel dumps. */
static void lagoon_trace_view(FILE *trace) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    LagoonSceneView scene = lagoon_scene_view(ram, core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM));
    static bool observed_room;
    observed_room |= scene.area != 0;
    if (!observed_room || !ram || !last_pixels) return;
    int x = lagoon_word(ram, 0x402e), y = lagoon_word(ram, 0x4030);
    unsigned ink[3] = {0}, vertical[2] = {0}, match = 0, kind = 0, enemy = 0, hud = 0, restored = 0;
    SDL_Rect view = lagoon_world_view();
    if (lagoon_has_wide_canvas()) {
        static uint32_t picture[X_MAX_WIDTH * LAGOON_VIEW];
        for (int row = 0; row < LAGOON_VIEW; ++row)
            for (int col = 0; col < LAGOON_VIEW; ++col)
                picture[row*X_MAX_WIDTH + col] = lagoon_wide_reference_pixel((size_t)row*LAGOON_VIEW + col);
        LagoonWideFrameInfo info;
        lagoon_compose_picture(picture, &info);
        kind = info.staggered ? 3 : info.live ? 1 : info.backdrop ? 2 : 0;
        enemy = info.enemy;
        hud = info.hud;
        restored = info.restored;
        for (int row = view.y; row < view.y + view.h; row += 4) {
            const uint32_t *raster = lagoon_native_row(row, view);
            if ((unsigned)row >= LAGOON_VIEW) {
                for (int col = view.x; col < view.x + view.w; col += 4)
                    vertical[row >= LAGOON_VIEW] += lagoon_color_energy(raster[col]) > 48;
                continue;
            }
            for (int col = view.x; col < view.x + view.w; col += 4) {
                int band = col < 0 ? 0 : col < LAGOON_VIEW ? 1 : 2;
                ink[band] += lagoon_color_energy(raster[col]) > 48;
            }
            if (!hud || row < LAGOON_HUD_TOP || row >= LAGOON_HUD_END)
                match += memcmp(picture + row*X_MAX_WIDTH, raster, LAGOON_VIEW * sizeof(uint32_t)) != 0;
        }
    }
    int actor_x = 0, actor_y = 0, actor_hp = 0, pose = -1;
    if (scene.area) {
        for (unsigned slot = 1; slot < 32; ++slot) {
            int actor = 0x402e + (int)slot*16;
            if (lagoon_byte(ram, actor + 7) != 4 || !lagoon_word(ram, actor)) continue;
            actor_x = lagoon_word(ram, actor);
            actor_y = lagoon_word(ram, actor + 2);
            actor_hp = lagoon_word(ram, actor + 4);
            pose = lagoon_byte(ram, 0xf6ca + slot);
            break;
        }
    }
    fprintf(trace, "%u,%u,%d,%d,%u,%u,%d,%d,%u,%u,%u,%u,%u,%d,%x,%x,%x,%x,%d,%d,%u,%u,%d,%d,%d,%d,%u,%u,%u,%d,%d\n",
            frame_number, scene.area, x, y, width, height, scene.door, scene.modal,
            kind, ink[0], ink[1], ink[2], match, lagoon_byte(ram, 0x4035),
            VCReg0[1], VCReg1[0], VCReg1[1], VCReg2[1], lagoon_picture_width(), lagoon_picture_height(), enemy, hud,
            actor_x, actor_y, actor_hp, pose, vertical[0], vertical[1], restored,
            lagoon_byte(ram, 0x403c), lagoon_byte(ram, 0x4cfe));
}

static unsigned lagoon_scene_held_doors, lagoon_scene_held_dialogue, lagoon_scene_center_mismatches;
static unsigned lagoon_scene_reveal_frames;

static void lagoon_track_wide_scene(void) {
    if (frame_number < 11100 || !lagoon_has_wide_canvas() || !last_pixels) return;
    static uint32_t picture[X_MAX_WIDTH * LAGOON_VIEW];
    for (int y = 0; y < LAGOON_VIEW; ++y) for (int x = 0; x < LAGOON_VIEW; ++x)
        picture[y*X_MAX_WIDTH + x] = lagoon_wide_reference_pixel((size_t)y*LAGOON_VIEW + x);
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    LagoonSceneView scene = lagoon_scene_view(ram, core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM));
    LagoonWideFrameInfo info;
    lagoon_compose_picture(picture, &info);
    if (info.masked_world) {
        unsigned bright_center = 0;
        for (int y = 5; y < LAGOON_VIEW; y += 11)
            for (int x = 3; x < LAGOON_VIEW; x += 9)
                bright_center += lagoon_color_energy(picture[y*X_MAX_WIDTH + x]) > 48;
        lagoon_scene_reveal_frames += bright_center >= 32 && bright_center < 370;
    }
    if (info.backdrop) {
        lagoon_scene_held_doors += scene.door;
        lagoon_scene_held_dialogue += scene.dialogue;
    }
    if (scene.door || scene.dialogue)
        for (int y = 0; y < LAGOON_VIEW; ++y)
            lagoon_scene_center_mismatches += memcmp(picture + y*X_MAX_WIDTH,
                lagoon_native_row(y, lagoon_world_view()), LAGOON_VIEW * sizeof(uint32_t)) != 0;
}

static unsigned lagoon_motion_lit, lagoon_motion_qualified, lagoon_motion_moved;
static unsigned lagoon_motion_x, lagoon_motion_y;
static bool lagoon_motion_earlier_checked;

static void lagoon_check_earlier_epoch(const uint32_t *picture) {
    /* Simulate the raster leading the register writes on an actual replay
     * frame. Restore the live register and selector preference immediately. */
    LagoonWorld world;
    const bool favored = lagoon_before_picture.favored;
    const uint8_t mode = VCReg0[1];
    bool earlier;
    VCReg0[1] = 0;
    lagoon_motion_earlier_checked = lagoon_select_world(picture, 1, false, &world, &earlier, NULL) && earlier;
    VCReg0[1] = mode;
    lagoon_before_picture.favored = favored;
}

static void lagoon_track_wide_motion(void) {
    if (frame_number < 11100 || !lagoon_has_wide_canvas() || !last_pixels) return;
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    /* Count active world video. Door and menu routines temporarily disable
     * or repurpose these planes without necessarily returning a black image. */
    if (!ram || core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) < LAGOON_BASE + 0xeccd ||
        lagoon_byte(ram, 0xeccc) != 1 || lagoon_byte(ram, 0x4d2f) ||
        VCReg0[1] != 1 || VCReg1[0] != 0x12 || VCReg1[1] != 0xe4 ||
        !(VCReg2[1] & 0x4f)) return;
    static uint32_t samples[X_MAX_WIDTH * LAGOON_VIEW];
    unsigned lit = 0, checked = 0;
    for (int y = 5; y < LAGOON_VIEW; y += 11) {
        for (int x = 3; x < LAGOON_VIEW; x += 9) {
            const uint32_t native = lagoon_wide_reference_pixel((size_t)y*LAGOON_VIEW + x);
            samples[y*X_MAX_WIDTH + x] = native;
            lit += (native & 0xffffffu) != 0;
            checked++;
        }
    }
    if (lit >= checked / 2) {
        unsigned x = lagoon_word(ram, 0xde80), y = lagoon_word(ram, 0xde82);
        lagoon_motion_moved += lagoon_motion_lit && (x != lagoon_motion_x || y != lagoon_motion_y);
        lagoon_motion_x = x;
        lagoon_motion_y = y;
        LagoonWorld world;
        lagoon_motion_lit++;
        bool earlier;
        lagoon_motion_qualified += lagoon_select_world(samples, 1, false, &world, &earlier, NULL);
        if (earlier && !lagoon_motion_earlier_checked) lagoon_check_earlier_epoch(samples);
    }
}
#endif
