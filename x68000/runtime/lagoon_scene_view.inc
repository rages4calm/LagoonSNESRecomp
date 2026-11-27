/* Preserve the guest raster's mode changes at the original pixel scale.
 * These flags classify the pinned English game's normal door path and the
 * original modal text renderer. They do not change guest state or timing. */
typedef struct {
    unsigned area;
    unsigned door_target;
    bool door;
    bool modal;
    bool dialogue;
} LagoonSceneView;

static bool lagoon_scene_door_stack(const uint8_t *ram, size_t size) {
    if (!core.get_cpu) return false;
    uint32_t regs[18];
    core.get_cpu(regs);
    const uint32_t sp = regs[17];
    const uint32_t pc = regs[0];
    if (pc >= LAGOON_BASE + 0x1312 && pc < LAGOON_BASE + 0x1366) return true;
    /* The main field loop waits for each frame at FD30. A door's two nested
     * ROM-service calls can instead lower SP by over 400 bytes. */
    if ((pc >= LAGOON_BASE + 0xfd30 && pc < LAGOON_BASE + 0xfd4c) ||
        (sp & 1u) || sp >= size) return false;
    const size_t max_bytes = size - sp < 512 ? size - sp : 512;
    for (size_t offset = 0; offset + 4 <= max_bytes; offset += 2) {
        const uint32_t caller = lagoon_stack_long(ram, sp + (uint32_t)offset);
        if (caller == LAGOON_BASE + 0x1318 || caller == LAGOON_BASE + 0x131c ||
            caller == LAGOON_BASE + 0x1334 || caller == LAGOON_BASE + 0x1338 ||
            caller == LAGOON_BASE + 0x1342 || caller == LAGOON_BASE + 0x1348 ||
            caller == LAGOON_BASE + 0x1354 || caller == LAGOON_BASE + 0x1358 ||
            caller == LAGOON_BASE + 0x135e || caller == LAGOON_BASE + 0x1362)
            return true;
    }
    return false;
}

static LagoonSceneView lagoon_scene_view(const uint8_t *ram, size_t size) {
    LagoonSceneView scene = {0};
    if (!ram || size <= LAGOON_BASE + 0x16172 ||
        lagoon_word(ram, 0xe968) != 0x4279 || lagoon_word(ram, 0xe96a) != 0x00e8 ||
        lagoon_word(ram, 0xe96c) != 0x002a)
        return scene;

    const unsigned area = lagoon_byte(ram, 0xeccc);
    if (!area || area > 50) return scene;
    scene.area = area;
    const unsigned target = lagoon_byte(ram, 0x4db0);
    scene.door_target = target <= 50 ? target : 0;
    scene.modal = lagoon_byte(ram, 0x4d2f) != 0;
    scene.dialogue = scene.modal && lagoon_byte(ram, 0x16172) == 1;
    scene.door = !scene.modal && scene.door_target && lagoon_scene_door_stack(ram, size);
    return scene;
}

static bool lagoon_game_fast_forward(void) {
    if (lagoon_boot_fast_forward()) return true;
    if (!input_focus || player_blocked() || lagoon_chart.open) return false;
    if (lagoon_door_waiting()) return true;
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    const size_t size = core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    /* Let disk reads and decompression finish at host speed while the guest's
       normal transfer is on the stack. Retain pacing for disk-retry prompts. */
    return ram && size > LAGOON_BASE + 0x16172 && !lagoon_byte(ram, 0x71d2) &&
        lagoon_scene_view(ram, size).door;
}
