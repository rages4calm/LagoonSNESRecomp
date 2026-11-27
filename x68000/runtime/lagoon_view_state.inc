/* Pointer-free presentation state in the existing checksummed snapshot.
 * Guest RAM, source identity and core compatibility remain SDK-owned. */
typedef struct {
    uint32_t version, header_size, zoom, ready, valid, area, width, height;
    uint64_t frame, last_door_frame, inverted_palette_hash;
} LagoonViewStateHeader;

static size_t lagoon_view_state_size(void) {
    return sizeof(LagoonViewStateHeader) +
        (LAGOON_MAX_VIEW_W * LAGOON_MAX_VIEW_H + LAGOON_VIEW * LAGOON_VIEW) * sizeof(uint32_t);
}

static bool lagoon_capture_view_state(void *data, size_t size) {
    if (!data || size != lagoon_view_state_size()) return false;
    /* Saving may occur before presentation of this guest frame. Refresh the
     * cache now, including when a headless replay is not drawing a window. */
    if (last_pixels && lagoon_has_wide_canvas() && last_pixels_size >=
        LAGOON_VIEW * LAGOON_VIEW * (pixel_format == RETRO_PIXEL_FORMAT_RGB565 ? 2u : 4u)) {
        /* The window adapter's ARGB buffer is absent in headless runs. Use
         * the core's packed raster in both paths, without invalidating the
         * warm cache merely because no window is presenting it. */
        static uint32_t picture[X_MAX_WIDTH * LAGOON_VIEW];
        for (int y = 0; y < LAGOON_VIEW; ++y)
            for (int x = 0; x < LAGOON_VIEW; ++x)
                picture[y * X_MAX_WIDTH + x] = lagoon_wide_reference_pixel((size_t)y * LAGOON_VIEW + x);
        lagoon_compose_picture(picture, NULL);
    }
    SDL_Rect view = lagoon_world_view();
    LagoonViewStateHeader header = {0};
    header.version = 2;
    header.header_size = sizeof(header);
    header.zoom = (uint32_t)lagoon_zoom_mode;
    header.ready = lagoon_zoom_ready;
    header.valid = lagoon_scene_backdrop.valid && lagoon_scene_backdrop.width == view.w;
    header.area = header.valid ? lagoon_scene_backdrop.area : 0;
    header.width = (uint32_t)view.w;
    header.height = (uint32_t)view.h;
    header.frame = header.valid ? lagoon_scene_backdrop.frame : 0;
    header.inverted_palette_hash = header.valid ? lagoon_scene_backdrop.inverted_palette_hash : 0;
    header.last_door_frame = lagoon_last_door_frame;
    memset(data, 0, size);
    memcpy(data, &header, sizeof(header));
    if (header.valid) {
        uint8_t *pixels = (uint8_t *)data + sizeof(header);
        for (int y = 0; y < view.h; ++y)
            memcpy(pixels + (size_t)y * LAGOON_MAX_VIEW_W * sizeof(uint32_t),
                   lagoon_scene_backdrop.pixels + y * X_MAX_WIDTH, view.w * sizeof(uint32_t));
        memcpy(pixels + LAGOON_MAX_VIEW_W * LAGOON_MAX_VIEW_H * sizeof(uint32_t),
               lagoon_scene_backdrop.center, sizeof(lagoon_scene_backdrop.center));
    }
    return true;
}

static bool lagoon_validate_view_state(const void *data, size_t size) {
    LagoonViewStateHeader header;
    if (!data || size != lagoon_view_state_size()) return false;
    memcpy(&header, data, sizeof(header));
    static const uint32_t widths[] = {456, 568, 684}, heights[] = {256, 320, 384};
    if (header.version != 2 || header.header_size != sizeof(header) ||
        header.zoom > 2 || header.ready > 1 || header.valid > 1 ||
        header.frame > UINT64_C(9007199254740991) ||
        header.last_door_frame > UINT64_C(9007199254740991)) return false;
    unsigned mode = header.ready ? header.zoom : 0;
    if (header.width != widths[mode] || header.height != heights[mode]) return false;
    if (header.valid && (!header.ready || !header.area || header.area > 50 || !header.inverted_palette_hash)) return false;
    if (!header.valid && (header.area || header.frame || header.inverted_palette_hash)) return false;
    return true;
}

static bool lagoon_apply_view_state(const void *data, size_t size) {
    if (!lagoon_validate_view_state(data, size)) return false;
    LagoonViewStateHeader header;
    memcpy(&header, data, sizeof(header));
    lagoon_before_picture.valid = false;
    lagoon_before_picture.favored = false;
    lagoon_zoom_mode = (int)header.zoom;
    lagoon_zoom_ready = header.ready != 0;
    lagoon_last_door_frame = header.last_door_frame;
    lagoon_scene_backdrop.valid = header.valid != 0;
    lagoon_scene_backdrop.area = header.area;
    lagoon_scene_backdrop.width = (int)header.width;
    lagoon_scene_backdrop.frame = header.frame;
    lagoon_scene_backdrop.inverted_palette_hash = header.inverted_palette_hash;
    if (header.valid) {
        const uint8_t *pixels = (const uint8_t *)data + sizeof(header);
        for (unsigned y = 0; y < header.height; ++y)
            memcpy(lagoon_scene_backdrop.pixels + y * X_MAX_WIDTH,
                   pixels + (size_t)y * LAGOON_MAX_VIEW_W * sizeof(uint32_t),
                   header.width * sizeof(uint32_t));
        memcpy(lagoon_scene_backdrop.center,
               pixels + LAGOON_MAX_VIEW_W * LAGOON_MAX_VIEW_H * sizeof(uint32_t),
               sizeof(lagoon_scene_backdrop.center));
    }
    return true;
}
