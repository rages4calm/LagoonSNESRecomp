/* The bundled English disk retains its original IPL and prologue entry points. */
typedef enum {
    LAGOON_BOOT_LOADING,
    LAGOON_BOOT_LOGO,
    LAGOON_BOOT_PROLOGUE,
    LAGOON_BOOT_GAME,
} LagoonBootStage;

static struct {
    bool active, skip, saw_opening;
    uint64_t first_frame;
    LagoonBootStage stage;
} lagoon_boot;

static void lagoon_boot_reset(void) {
    lagoon_boot.active = true;
    lagoon_boot.skip = false;
    lagoon_boot.saw_opening = false;
    lagoon_boot.first_frame = input_frame;
    lagoon_boot.stage = LAGOON_BOOT_LOADING;
}

static void lagoon_boot_stop(void) {
    lagoon_boot.active = false;
    lagoon_boot.skip = false;
}

static void lagoon_boot_refresh(void) {
    if (!lagoon_boot.active) return;
    /* Fall back to ordinary pacing if a boot is missing its verified program or disk. */
    if (load_start || input_frame - lagoon_boot.first_frame >= 14000) {
        lagoon_boot_stop();
        return;
    }
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) < LAGOON_BASE + 0xeccd) return;
    /* A warm hardware reset leaves old game RAM until the loader overwrites it. */
    if (lagoon_boot.saw_opening && lagoon_word(ram, 0) == 0x6100 && lagoon_word(ram, 2) == 0x5efe &&
        lagoon_word(ram, 4) == 0x6100 && lagoon_word(ram, 6) == 0x5bd4) {
        lagoon_boot.stage = LAGOON_BOOT_GAME;
        int area = lagoon_byte(ram, 0xeccc);
        if (area > 0 && area <= 50 && lagoon_word(ram, 0x402e) && !lagoon_byte(ram, 0x4035))
            lagoon_boot_stop();
    } else if (lagoon_word(ram, 0) == 0x2f09 && lagoon_word(ram, 2) == 0x93c9 &&
               lagoon_word(ram, 4) == 0x303c && lagoon_word(ram, 6) == 0x0081) {
        lagoon_boot.stage = LAGOON_BOOT_PROLOGUE;
        lagoon_boot.saw_opening = true;
    } else if (lagoon_word(ram, 0) == 0x6100 && lagoon_word(ram, 2) == 0x0886 &&
               lagoon_word(ram, 4) == 0x6100 && lagoon_word(ram, 6) == 0x00c6) {
        lagoon_boot.stage = LAGOON_BOOT_LOGO;
        lagoon_boot.saw_opening = true;
    } else {
        lagoon_boot.stage = LAGOON_BOOT_LOADING;
    }
}

static void lagoon_boot_prepare(uint16_t pad) {
    lagoon_boot_refresh();
    if (!lagoon_boot.active || lagoon_boot.stage == LAGOON_BOOT_GAME || !input_focus ||
        player_blocked() || rrhk_ui()->capturing) return;
    const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
    if ((pad & X_BIT(X_ENTER)) || (keyboard && keyboard[SDL_SCANCODE_RETURN]))
        lagoon_boot.skip = true;
}

static bool lagoon_boot_stage_fast_forward(LagoonBootStage stage, bool skip) {
    /* Loading waits can run quickly. The Zoom logo and story are authored
     * animation, and keep their original pacing unless the player skips. */
    return skip || stage == LAGOON_BOOT_LOADING || stage == LAGOON_BOOT_GAME;
}

static bool lagoon_boot_fast_forward(void) {
    lagoon_boot_refresh();
    return x_fast_forward() || (lagoon_boot.active && ca.windowed && input_focus && !player_blocked() &&
        !rrhk_ui()->capturing && lagoon_boot_stage_fast_forward(lagoon_boot.stage, lagoon_boot.skip));
}

static int16_t lagoon_boot_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    /* Both original opening programs check the X68000 I key in their keyboard bitmap.
       Never send it to Lagoon.X, where it would open Items. */
    if (!port && device == RETRO_DEVICE_KEYBOARD && id == RETROK_i &&
        lagoon_boot.active && lagoon_boot.skip && input_focus && !player_blocked() &&
        (lagoon_boot.stage == LAGOON_BOOT_LOGO || lagoon_boot.stage == LAGOON_BOOT_PROLOGUE)) return 1;
    return lagoon_input_state(port, device, index, id);
}

static void lagoon_boot_draw(SDL_Renderer *renderer) {
    if (!lagoon_boot.active || player_blocked()) return;
    int x, y, unit;
    rr_menu_layout(renderer, &x, &y, &unit);
    const char *message = lagoon_boot.stage == LAGOON_BOOT_GAME ?
        "Preparing Atland    Disks change automatically" :
        lagoon_boot.skip ? "Starting Lagoon" : "START / ENTER    Skip the opening";
    rr_menu_plate(renderer, x, y, unit, 172, message, -1);
}

#ifdef LAGOON_ATLAS_PREVIEW
static bool lagoon_check_boot(void) {
    if (input_frame != 0) return false;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) return false;
    int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                         SDL_CONTROLLER_BUTTON_MAX, 0);
    SDL_GameController *controller = index >= 0 ? SDL_GameControllerOpen(index) : NULL;
    bool verified = false;
    if (controller) {
        rrhk_bind(rrhk_adapter());
        rrhk_ui()->pads[0] = controller;
        ca.windowed = true;
        frame_limit = 0;
        lagoon_boot_reset();
        core.set_input_state(lagoon_boot_input_state);
        SDL_Joystick *pad = SDL_GameControllerGetJoystick(controller);
        bool ran = lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_START, 8, 0);
        if (ran && lagoon_boot.skip) {
            while (lagoon_boot.active && input_frame < 3500 && ran) ran = x_run_frame(0, 0);
            unsigned ready_frame = (unsigned)input_frame;
            const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
            if (ran && !lagoon_boot.active && accelerated_frames && ram && lagoon_byte(ram, 0xeccc) == 1 &&
                media.selected[0] == 4 && media.selected[1] == 1 && !lagoon_boot_fast_forward()) {
                int start_x = lagoon_word(ram, 0x402e);
                ran = lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT, 28, 10);
                verified = ran && lagoon_word(ram, 0x402e) < start_x && !lagoon_byte(ram, 0x4d2f);
                if (verified)
                    fprintf(stderr, "Controller boot ready at frame %u, %u accelerated frames, player moved %d pixels\n",
                            ready_frame, accelerated_frames, start_x - lagoon_word(ram, 0x402e));
            }
        }
        if (!verified)
            fprintf(stderr, "Controller boot check failed at frame %" PRIu64 " (stage %d, disks %d/%d, accelerated %u)\n",
                    input_frame, lagoon_boot.stage, media.selected[0], media.selected[1], accelerated_frames);
        core.set_input_state(input_state);
        lagoon_boot_stop();
        ca.windowed = false;
        rrhk_ui()->pads[0] = NULL;
        SDL_GameControllerClose(controller);
    }
    if (index >= 0) SDL_JoystickDetachVirtual(index);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO);
    return verified;
}
#endif
