/* Expanded-view rendering for live actors. The guest keeps its original actor
 * slots, simulation, cull and OAM limits. */
enum { LAGOON_EXTENDED_PARTS = 256 };

typedef struct {
    int16_t x, y;
    uint16_t attributes;
    uint8_t priority;
    bool enemy;
    uint8_t packed[128];
} LagoonExtendedPart;

typedef struct {
    unsigned count;
    LagoonExtendedPart parts[LAGOON_EXTENDED_PARTS];
} LagoonExtendedActors;

static const uint8_t *lagoon_actor_span(const LagoonWorld *world, size_t ram_size,
                                        uint32_t address, size_t length) {
    /* The 68000 exposes big-endian words. Both backing stores are word-swapped. */
    if (address & 1u || length & 1u) return NULL;
    if (address >= 0xe00000 && address < 0xe80000 && length <= 0xe80000 - address)
        return TVRAM + address - 0xe00000;
    if (address < ram_size && length <= ram_size - address)
        return world->ram + address;
    return NULL;
}

static unsigned lagoon_actor_word(const uint8_t *p) {
    return ((unsigned)p[1] << 8) | p[0];
}

static uint32_t lagoon_actor_long(const uint8_t *p) {
    return (uint32_t)lagoon_actor_word(p) << 16 | lagoon_actor_word(p + 2);
}

static unsigned lagoon_actor_priority(int x, int y, unsigned half_x, unsigned half_y) {
    /* Same foreground mask and collision-footprint test as $FC12–$FC56. */
    unsigned first = (unsigned)(x - (int)half_x) / 16;
    unsigned last = (unsigned)(x + (int)half_x) / 16;
    unsigned row = (unsigned)(y + (int)half_y) & ~15u;
    for (unsigned cell = first; cell <= last; ++cell)
        if (lagoon_tvram_byte(0x75420 + row + cell/8) & (1u << (cell & 7u)))
            return 1;
    return 3;
}

static bool lagoon_has_crossing_part(const LagoonWorld *world, int x, int y,
                                     unsigned attributes, unsigned priority) {
    for (unsigned i = 0; i < world->part_count; ++i) {
        const LagoonWorldPart *part = &world->parts[i];
        if (part->x == x && part->y == y && part->priority == priority &&
            (part->attributes & 0xff00u) == attributes) return true;
    }
    return false;
}

static void lagoon_capture_extended_actors(const LagoonWorld *world, size_t ram_size,
                                           LagoonExtendedActors *view) {
    view->count = 0;
    if (!world->bg || ram_size < LAGOON_BASE + 0x11f90 ||
        lagoon_word(world->ram, 0xfb82) != world->origin_x) return;

    const int camera_y = (int16_t)lagoon_word(world->ram, 0xfb86);
    const uint32_t table = LAGOON_BASE + 0x11d90;
    uint32_t seen = 0;
    /* Reuse the guest's painter order from $FBB8. */
    for (int ordinal = 0; ordinal < 32; ++ordinal) {
        unsigned slot = lagoon_byte(world->ram, 0xfaea + ordinal);
        if (!slot || slot >= 32 || (seen & (1u << slot))) continue;
        seen |= 1u << slot;
        const int actor = 0x402e + (int)slot*16;
        /* The guest's generic sprite loop ($FB68) has no class test. It
         * admits a nonempty slot with a signed, nonnegative pose. */
        unsigned actor_class = lagoon_byte(world->ram, actor + 7);
        const int x = lagoon_word(world->ram, actor);
        const int y = lagoon_word(world->ram, actor + 2);
        const unsigned half_x = lagoon_word(world->ram, actor + 8);
        const unsigned half_y = lagoon_word(world->ram, actor + 10);
        const int rel_x = x - world->origin_x, rel_y = y - camera_y;
        const unsigned pose = lagoon_byte(world->ram, 0xf6ca + slot);
        if (!x || x >= 2048 || y >= 2048 || pose >= 128 ||
            half_x > 64 || half_y > 64 || x < (int)half_x ||
            x + (int)half_x >= 2048 || y + (int)half_y >= 2048 ||
            rel_y <= -128 || rel_y >= 384 ||
            rel_x < (256 - LAGOON_MAX_VIEW_W) / 2 - 64 ||
            rel_x >= (256 + LAGOON_MAX_VIEW_W) / 2 + 64) continue;

        unsigned kind = lagoon_byte(world->ram, actor + 6);
        const uint8_t *entry = lagoon_actor_span(world, ram_size, table + 2*kind, 2);
        if (!entry) continue;
        const uint8_t *descriptor = lagoon_actor_span(world, ram_size,
            table + (int16_t)lagoon_actor_word(entry), 8);
        if (!descriptor) continue;
        uint32_t poses = lagoon_actor_long(descriptor);
        uint32_t patterns = lagoon_actor_long(descriptor + 4);
        if (poses >= 0xe80000 || patterns >= 0xe80000) continue;

        const uint8_t *offset = lagoon_actor_span(world, ram_size, poses + 2*pose, 2);
        if (!offset) continue;
        uint32_t frame_address = poses + (int16_t)lagoon_actor_word(offset);
        const uint8_t *frame = lagoon_actor_span(world, ram_size, frame_address, 2);
        if (!frame) continue;
        unsigned parts = lagoon_actor_word(frame);
        if (!parts || parts > 32) continue;
        frame = lagoon_actor_span(world, ram_size, frame_address, 2 + parts*8);
        if (!frame) continue;
        unsigned priority = lagoon_actor_priority(x, y, half_x, half_y);
        for (unsigned i = 0; i < parts && view->count < LAGOON_EXTENDED_PARTS; ++i) {
            const uint8_t *record = frame + 2 + i*8;
            int part_x = rel_x + (int16_t)lagoon_actor_word(record) - 16;
            int part_y = rel_y + (int16_t)lagoon_actor_word(record + 2) - 16;
            if (part_x >= 0 && part_x <= 240 && part_y >= 0 && part_y <= 240) continue;
            if (part_x <= (256 - LAGOON_MAX_VIEW_W) / 2 - 16 ||
                part_x >= (256 + LAGOON_MAX_VIEW_W) / 2 ||
                part_y <= (256 - LAGOON_MAX_VIEW_H) / 2 - 16 ||
                part_y >= (256 + LAGOON_MAX_VIEW_H) / 2) continue;
            unsigned tile = lagoon_actor_word(record + 4);
            unsigned attributes = lagoon_actor_word(record + 6);
            if ((tile & 127u) || (attributes & 0x30ffu)) continue;
            /* The anchor can be in range while this part is absent from
             * OAM. Only defer to the guest when it emitted the crossing part. */
            if (part_x > -16 && part_x < 256 && part_y > -16 && part_y < 256 &&
                lagoon_has_crossing_part(world, part_x, part_y, attributes, priority)) continue;
            const uint8_t *pattern = lagoon_actor_span(world, ram_size, patterns + tile, 128);
            if (!pattern) continue;
            LagoonExtendedPart *part = &view->parts[view->count++];
            part->x = (int16_t)part_x;
            part->y = (int16_t)part_y;
            part->attributes = (uint16_t)attributes;
            part->priority = (uint8_t)priority;
            part->enemy = actor_class == 4;
            for (unsigned j = 0; j < sizeof part->packed; ++j)
                part->packed[j] = pattern[j ^ 1u];
        }
    }
}

static unsigned lagoon_extended_texel(const LagoonExtendedPart *part, int x, int y) {
    unsigned column = (unsigned)x ^ (part->attributes & 0x4000u ? 15u : 0u);
    unsigned row = (unsigned)y ^ (part->attributes & 0x8000u ? 15u : 0u);
    unsigned byte = part->packed[(column & 8u)*8u + row*4u + ((column & 7u) >> 1)];
    return column & 1u ? byte & 15u : byte >> 4;
}

static unsigned lagoon_draw_extended_actors(const LagoonWorld *world, const LagoonExtendedActors *view,
                                           uint32_t *canvas, size_t pitch, SDL_Rect bounds, unsigned *enemy_pixels) {
    uint32_t claimed[LAGOON_MAX_VIEW_H][(LAGOON_MAX_VIEW_W + 31) / 32] = {{0}};
    unsigned drawn = 0;
    for (unsigned i = 0; i < view->count; ++i) {
        const LagoonExtendedPart *part = &view->parts[i];
        const unsigned group = (part->attributes >> 4) & 0xf0u;
        int left = SDL_max(part->x, bounds.x);
        int right = SDL_min(part->x + 16, bounds.x + bounds.w);
        for (int row = SDL_max(part->y, bounds.y); row < SDL_min(part->y + 16, bounds.y + bounds.h); ++row) {
            for (int col = left; col < right; ++col) {
                if (lagoon_inside_native(col, row) || !lagoon_world_contains(world, col, row)) continue;
                unsigned index = (unsigned)(col - bounds.x);
                uint32_t bit = 1u << (index & 31u);
                if (claimed[row - bounds.y][index/32] & bit) continue;
                unsigned pixel = lagoon_extended_texel(part, col - part->x, row - part->y);
                if (!pixel) continue;
                claimed[row - bounds.y][index/32] |= bit;
                if (part->priority != 3 && lagoon_world_bg_index(world, (world->x[0] + (unsigned)col) & 2047u,
                                                               (world->y[0] + (unsigned)row) & 2047u))
                    continue;
                canvas[(size_t)(row - bounds.y)*pitch + index] = lagoon_world_argb(world->bg_colors[group + pixel]);
                *enemy_pixels += part->enemy;
                ++drawn;
            }
        }
    }
    return drawn;
}
