/* Live Lagoon tile planes. Included after the Atlas guest-memory helpers. */
extern uint8_t TVRAM[0x80000];
extern uint8_t BGCHR8[8 * 8 * 256], BGCHR16[16 * 16 * 256], Sprite_Regs[0x800];
extern uint8_t BG_Regs[0x12], CRTC_Regs[0x30], VCReg0[2], VCReg1[2], VCReg2[2];
extern uint16_t TextPal[256], GrphPal[256];

enum { LAGOON_VIEW = 256, LAGOON_FLANK = 100, LAGOON_WIDE = 456,
       LAGOON_MAX_VIEW_W = 684, LAGOON_MAX_VIEW_H = 384,
       LAGOON_HUD_TOP = 212, LAGOON_HUD_END = 253 };

static int lagoon_sprite_coordinate(unsigned word) {
    int value = (int)((word - 16u) & 1023u);
    return value >= 512 ? value - 1024 : value;
}

typedef struct {
    int16_t x, y;
    uint16_t attributes;
    uint8_t priority;
    uint8_t texels[256];
} LagoonWorldPart;

typedef struct {
    const uint8_t *ram;
    int origin_x, origin_y, room_left, room_right, room_top, room_bottom;
    unsigned area;
    unsigned x[3], y[3];
    bool bg, graphics, parallax, staggered, palette_flash;
    uint32_t flash_color;
    uint16_t bg_colors[256], graphics_colors[256];
    uint8_t bg_texels[sizeof BGCHR8];
    unsigned part_count;
    LagoonWorldPart parts[128];
} LagoonWorld;

static unsigned lagoon_world_sprite_word(const uint8_t *part, unsigned offset) {
    return part[offset] | (unsigned)part[offset + 1] << 8;
}

static void lagoon_world_sample_parts(LagoonWorld *world) {
    for (int slot = 0; slot < 128; ++slot) {
        const uint8_t *reg = Sprite_Regs + slot * 8;
        unsigned priority = reg[6] & 3u;
        int x = lagoon_sprite_coordinate(lagoon_world_sprite_word(reg, 0));
        int y = lagoon_sprite_coordinate(lagoon_world_sprite_word(reg, 2));
        const bool native = x >= 0 && x <= 240 && y >= 0 && y <= 240;
        const bool under_hud = lagoon_zoom_mode && y + 16 > LAGOON_HUD_TOP && y < LAGOON_HUD_END;
        if (!priority || (native && !under_hud) ||
            x <= (256 - LAGOON_MAX_VIEW_W) / 2 - 16 || x >= (256 + LAGOON_MAX_VIEW_W) / 2 ||
            y <= (256 - LAGOON_MAX_VIEW_H) / 2 - 16 || y >= (256 + LAGOON_MAX_VIEW_H) / 2)
            continue;
        LagoonWorldPart *part = &world->parts[world->part_count++];
        part->x = (int16_t)x;
        part->y = (int16_t)y;
        part->attributes = (uint16_t)lagoon_world_sprite_word(reg, 4);
        part->priority = (uint8_t)priority;
        memcpy(part->texels, BGCHR16 + (part->attributes & 255u)*256u, sizeof part->texels);
    }
}

static bool lagoon_world_begin(LagoonWorld *world, const uint8_t *ram, size_t ram_size) {
    if (!ram || ram_size < 0xd5810 ||
        lagoon_word(ram, 0xe968) != 0x4279 || lagoon_word(ram, 0xe96a) != 0x00e8 ||
        lagoon_word(ram, 0xe96c) != 0x002a || lagoon_byte(ram, 0x4035) ||
        lagoon_byte(ram, 0x4d2f) || !lagoon_byte(ram, 0xeccc) || lagoon_byte(ram, 0xeccc) > 50)
        return false;

    /* Compatible 8px BG and paired 256-color graphics planes, with Lagoon's
       normal priority order. Other CRTC layouts are presented by the core. */
    unsigned graph_enable = VCReg2[1] & 0x0f;
    bool bg = (VCReg2[1] & 0x40) && (BG_Regs[8] & 2) && (BG_Regs[9] & 1);
    if (VCReg0[1] != 1 || VCReg1[0] != 0x12 || VCReg1[1] != 0xe4 || VCReg2[0] ||
        (graph_enable && graph_enable != 3 && graph_enable != 12 && graph_enable != 15) ||
        (!bg && !graph_enable))
        return false;

    int origin = (int16_t)lagoon_word(ram, 0xde80);
    int left = (int16_t)lagoon_word(ram, 0xde84);
    int right = (int16_t)lagoon_word(ram, 0xde88);
    if (left < 0 || origin < left || origin > right || right > 2048 - 256)
        return false;

    LagoonWorld result = {0};
    result.ram = ram;
    result.area = (unsigned)lagoon_byte(ram, 0xeccc);
    result.origin_x = origin;
    result.origin_y = (int16_t)lagoon_word(ram, 0xde82);
    result.room_top = (int16_t)lagoon_word(ram, 0xde86);
    result.room_bottom = (int16_t)lagoon_word(ram, 0xde8a) + LAGOON_VIEW;
    /* The legacy view only samples the original scanlines. Expand vertically
     * where the inventory of loaded terrain has valid room bounds. */
    if (result.room_top < 0 || result.room_top > result.origin_y || result.room_bottom > 2048 ||
        result.room_bottom < result.origin_y + LAGOON_VIEW) {
        result.room_top = result.origin_y;
        result.room_bottom = result.origin_y + LAGOON_VIEW;
    }
    result.room_left = left;
    result.room_right = right + 256;
    result.bg = bg;
    result.graphics = (graph_enable & 3) != 0;
    result.parallax = (graph_enable & 12) != 0;
    for (int layer = 0; layer < 3; ++layer) {
        result.x[layer] = lagoon_word(ram, 0xde98 + layer * 4) & 2047u;
        result.y[layer] = lagoon_word(ram, 0xde9a + layer * 4) & 2047u;
    }
    /* Check the same scroll latched by the renderer. This rejects the
       frames between loading the new map and restoring its world planes. */
    if (bg && ((BG_Regs[0x11] & 3) ||
        (BG_Regs[0x11] & 0x14) != (CRTC_Regs[0x29] & 0x14) ||
        BG_Regs[0x0d] != CRTC_Regs[5] + 4 || BG_Regs[0x0f] != CRTC_Regs[0x0d] ||
        (((unsigned)BG_Regs[0] << 8) | BG_Regs[1]) != (result.x[0] & 511u) ||
        (((unsigned)BG_Regs[2] << 8) | BG_Regs[3]) != (result.y[0] & 511u)))
        return false;
    for (int layer = 1; layer < 3; ++layer) {
        if (!(graph_enable & (layer == 1 ? 3 : 12))) continue;
        int scroll = 0x18 + (layer - 1) * 8;
        if ((((unsigned)CRTC_Regs[scroll] << 8) | CRTC_Regs[scroll + 1]) != (result.x[layer] & 511u) ||
            (((unsigned)CRTC_Regs[scroll + 2] << 8) | CRTC_Regs[scroll + 3]) != (result.y[layer] & 511u))
            return false;
    }
    result.part_count = 0;
    memcpy(result.bg_colors, TextPal, sizeof result.bg_colors);
    memcpy(result.graphics_colors, GrphPal, sizeof result.graphics_colors);
    if (bg) {
        memcpy(result.bg_texels, BGCHR8, sizeof result.bg_texels);
        lagoon_world_sample_parts(&result);
    }
    *world = result;
    return true;
}

static uint8_t lagoon_tvram_byte(unsigned offset) {
    return TVRAM[offset ^ 1u];
}

static unsigned lagoon_world_quadrant(unsigned x, unsigned y) {
    return ((x & 8u) >> 2) | ((y & 8u) >> 3);
}

static unsigned lagoon_world_bg_index(const LagoonWorld *world, unsigned x, unsigned y) {
    unsigned cell = lagoon_tvram_byte(0x6d420 + (y >> 4) * 128u + (x >> 4));
    unsigned location = 0x6c400 + cell * 8u + lagoon_world_quadrant(x, y) * 2u;
    unsigned attribute = lagoon_tvram_byte(location);
    unsigned pattern = lagoon_tvram_byte(location + 1);
    unsigned tile_x = (x ^ (attribute & 0x40 ? 7u : 0u)) & 7u;
    unsigned tile_y = (y ^ (attribute & 0x80 ? 7u : 0u)) & 7u;
    unsigned pixel = world->bg_texels[pattern * 64u + tile_y * 8u + tile_x] & 15u;
    return pixel ? ((attribute & 15u) << 4) + pixel : 0u;
}

static unsigned lagoon_world_graphics_index(const LagoonWorld *world, unsigned layer, unsigned x, unsigned y) {
    const unsigned cell_offset = (y >> 4) * 128u + (x >> 4);
    unsigned pattern;
    if (layer == 1) {
        unsigned cell = lagoon_tvram_byte(0x71420 + cell_offset);
        unsigned location = 0x6cc10 + cell * 8u + lagoon_world_quadrant(x, y) * 2u;
        pattern = ((unsigned)lagoon_tvram_byte(location) << 8) | lagoon_tvram_byte(location + 1);
    } else {
        unsigned cell = world->ram[(0xd1810 + cell_offset) ^ 1u];
        unsigned location = 0xd1000 + cell * 8u + lagoon_world_quadrant(x, y) * 2u;
        pattern = ((unsigned)world->ram[location ^ 1u] << 8) | world->ram[(location + 1) ^ 1u];
    }
    /* $EC66 expands each 4bpp tile to 8bpp and applies its palette group. */
    unsigned graphics = layer == 1 ? 0x8000 : 0x10000;
    return lagoon_tvram_byte(graphics + (pattern & 0x7fc0u) + (y & 7u) * 8u + (x & 7u));
}

static uint32_t lagoon_world_argb(unsigned word) {
    unsigned intensity = word & 1u;
    unsigned r = (word >> 5 & 62u) | intensity, g = (word >> 10 & 62u) | intensity, b = (word & 62u) | intensity;
    return 0xff000000u | (((r << 2) | (r >> 4)) << 16) | (((g << 2) | (g >> 4)) << 8) | (b << 2) | (b >> 4);
}

static bool lagoon_world_contains(const LagoonWorld *world, int x, int y) {
    int world_x = world->origin_x + x, world_y = world->origin_y + y;
    return world_x >= world->room_left && world_x < world->room_right &&
        world_y >= world->room_top && world_y < world->room_bottom;
}

static bool lagoon_inside_native(int x, int y) {
    return (unsigned)x < LAGOON_VIEW && (unsigned)y < LAGOON_VIEW;
}

static uint32_t lagoon_world_pixel(const LagoonWorld *world, int screen_x, int screen_y) {
    /* The guest's global palette strobe also colors its normally transparent
     * backdrop. This is a screen effect, including outside finite map bounds. */
    if (world->palette_flash) return world->flash_color;
    if (!lagoon_world_contains(world, screen_x, screen_y))
        return 0xff000000u;

    unsigned bg = world->bg ? lagoon_world_bg_index(world, (world->x[0] + (unsigned)screen_x) & 2047u,
                                                    (world->y[0] + (unsigned)screen_y) & 2047u) : 0u;
    unsigned index = 0;
    if (!bg && world->graphics)
        index = lagoon_world_graphics_index(world, 1, (world->x[1] + (unsigned)screen_x) & 2047u,
                                             (world->y[1] + (unsigned)screen_y) & 2047u);
    if (!bg && !index && world->parallax)
        index = lagoon_world_graphics_index(world, 2, (world->x[2] + (unsigned)screen_x) & 2047u,
                                             (world->y[2] + (unsigned)screen_y) & 2047u);
    unsigned word = bg ? world->bg_colors[bg] : world->graphics_colors[index];
    return lagoon_world_argb(word);
}


static unsigned lagoon_world_sprite_pixel(const uint8_t *pattern, unsigned attributes, int x, int y) {
    unsigned flipped_x = (unsigned)x ^ (attributes & 0x4000u ? 15u : 0u);
    unsigned flipped_y = (unsigned)y ^ (attributes & 0x8000u ? 15u : 0u);
    return pattern[flipped_y*16u + flipped_x] & 15u;
}

/* Restore the world when the hardware's original text mask moves down.
 * The guest OAM order and BG priority apply under the old HUD as they do
 * to its normal view. Only pixels formerly covered by HUD text use this. */
static uint32_t lagoon_world_hud_underlay(const LagoonWorld *world, int x, int y, unsigned *restored) {
    if (world->bg) {
        for (unsigned slot = 0; slot < world->part_count; ++slot) {
            const LagoonWorldPart *part = &world->parts[slot];
            if ((unsigned)(x - part->x) >= 16 || (unsigned)(y - part->y) >= 16) continue;
            unsigned texel = lagoon_world_sprite_pixel(part->texels, part->attributes, x - part->x, y - part->y);
            if (!texel) continue;
            if (part->priority != 3 && lagoon_world_bg_index(world, (world->x[0] + (unsigned)x) & 2047u,
                                                           (world->y[0] + (unsigned)y) & 2047u))
                break;
            *restored += 1;
            return lagoon_world_argb(world->bg_colors[((part->attributes >> 4) & 0xf0u) + texel]);
        }
    }
    return lagoon_world_pixel(world, x, y);
}

/* Extend the parts already emitted by the guest exactly across the native
 * rectangle. Do not overwrite its original center or change its OAM limits. */
static unsigned lagoon_world_draw_crossing_parts(const LagoonWorld *world, uint32_t *canvas,
                                                 size_t pitch, SDL_Rect view) {
    if (!world->bg) return 0;
    uint32_t claimed[LAGOON_MAX_VIEW_H][(LAGOON_MAX_VIEW_W + 31) / 32] = {{0}};
    unsigned drawn = 0;
    for (unsigned slot = 0; slot < world->part_count; ++slot) {
        const LagoonWorldPart *part = &world->parts[slot];
        int x = part->x, y = part->y;
        if (x >= 0 && x <= 240 && y >= 0 && y <= 240) continue;
        unsigned group = (part->attributes >> 4) & 0xf0u;
        for (int row = SDL_max(y, view.y); row < SDL_min(y + 16, view.y + view.h); ++row) {
            for (int col = SDL_max(x, view.x); col < SDL_min(x + 16, view.x + view.w); ++col) {
                if (lagoon_inside_native(col, row) || !lagoon_world_contains(world, col, row)) continue;
                unsigned index = (unsigned)(col - view.x);
                uint32_t bit = 1u << (index & 31u);
                uint32_t *seen = &claimed[row - view.y][index / 32];
                if (*seen & bit) continue;
                unsigned pixel = lagoon_world_sprite_pixel(part->texels, part->attributes, col - x, row - y);
                if (!pixel) continue;
                *seen |= bit;
                if (part->priority != 3 && lagoon_world_bg_index(world, (world->x[0] + (unsigned)col) & 2047u,
                                                           (world->y[0] + (unsigned)row) & 2047u))
                    continue;
                canvas[(size_t)(row - view.y)*pitch + index] = lagoon_world_argb(world->bg_colors[group + pixel]);
                ++drawn;
            }
        }
    }
    return drawn;
}
