/* Host bindings for Lagoon's original guest menus. */
#define LAGOON_MENU_BITS (X_BIT(X_ITEMS) | X_BIT(X_EQUIPMENT) | X_BIT(X_SAVE_LOAD))

static struct {
    uint16_t held, pulse;
    unsigned remaining;
    bool require_release;
} lagoon_menu_input;

static void lagoon_release_menu_input(void) {
    lagoon_menu_input.pulse = 0;
    lagoon_menu_input.remaining = 0;
    lagoon_menu_input.require_release = true;
}

static void lagoon_prepare_menu_input(uint16_t pad) {
    const uint16_t held = pad & LAGOON_MENU_BITS;
    const uint16_t pressed = held & ~lagoon_menu_input.held;
    lagoon_menu_input.held = held;
    const RrhkUi *ui = rrhk_ui();
    if (!input_focus || ui->menu || ui->paused || ui->capturing ||
        lagoon_chart.open || lagoon_chart.consume_pad_release) {
        lagoon_release_menu_input();
        return;
    }
    if (lagoon_menu_input.require_release) {
        lagoon_menu_input.require_release = held != 0;
        return;
    }
    if (!pressed || (pressed & (pressed - 1)) || lagoon_menu_input.remaining) return;
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM) < LAGOON_BASE + 0xeccd ||
        lagoon_word(ram, 0xe968) != 0x4279 || lagoon_word(ram, 0xe96a) != 0x00e8 ||
        lagoon_word(ram, 0xe96c) != 0x002a || lagoon_byte(ram, 0x4035) ||
        lagoon_byte(ram, 0x4d2f) || !lagoon_byte(ram, 0xeccc) ||
        lagoon_byte(ram, 0xeccc) > 50) return;

    /* Poll long enough for the X68000 keyboard scan to see a short tap. */
    lagoon_menu_input.pulse = pressed;
    lagoon_menu_input.remaining = 4;
    loading_holdoff = 16;
}

static int16_t lagoon_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    if (!port && device == RETRO_DEVICE_KEYBOARD && lagoon_menu_input.remaining &&
        input_focus && !player_blocked()) {
        if ((id == RETROK_i && lagoon_menu_input.pulse == X_BIT(X_ITEMS)) ||
            (id == RETROK_e && lagoon_menu_input.pulse == X_BIT(X_EQUIPMENT)) ||
            (id == RETROK_F1 && lagoon_menu_input.pulse == X_BIT(X_SAVE_LOAD))) return 1;
    }
    return input_state(port, device, index, id);
}

static bool lagoon_control_event(const SDL_Event *event, bool *running) {
    if ((event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) ||
        rrhk_ui()->menu || rrhk_ui()->paused || rrhk_ui()->capturing) lagoon_release_menu_input();
    return lagoon_chart_event(event, running);
}
#ifdef LAGOON_ATLAS_PREVIEW
static bool x_run_frame(uint16_t p1, uint16_t p2);

static bool lagoon_pad_run(SDL_Joystick *pad, int button, unsigned hold, unsigned wait) {
    if (SDL_JoystickSetVirtualButton(pad, button, 1)) return false;
    SDL_JoystickUpdate();
    for (unsigned i = 0; i < hold; ++i)
        if (!x_run_frame(rrhk_pad_word(0, input_frame), 0)) return false;
    if (SDL_JoystickSetVirtualButton(pad, button, 0)) return false;
    SDL_JoystickUpdate();
    for (unsigned i = 0; i < wait; ++i)
        if (!x_run_frame(rrhk_pad_word(0, input_frame), 0)) return false;
    return true;
}

static bool lagoon_pad_event(SDL_Joystick *pad, int button) {
    SDL_FlushEvents(SDL_CONTROLLERBUTTONDOWN, SDL_CONTROLLERBUTTONUP);
    if (SDL_JoystickSetVirtualButton(pad, button, 1)) return false;
    SDL_JoystickUpdate();
    SDL_Event event;
    bool running = true, found = false;
    while (SDL_PollEvent(&event)) {
        if (event.type != SDL_CONTROLLERBUTTONDOWN || event.cbutton.button != button ||
            event.cbutton.which != SDL_JoystickInstanceID(pad)) continue;
        lagoon_control_event(&event, &running);
        found = true;
    }
    if (SDL_JoystickSetVirtualButton(pad, button, 0)) return false;
    SDL_JoystickUpdate();
    return running && found;
}

void rrhk_options_init(const wchar_t *console);

static bool lagoon_test_atlas_tabs(SDL_Joystick *pad) {
    for (int page = 1; page <= LAGOON_PAGES; ++page)
        if (!lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ||
            lagoon_chart.page != page % LAGOON_PAGES) return false;
    for (int page = LAGOON_PAGES - 1; page >= 0; --page)
        if (!lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ||
            lagoon_chart.page != page) return false;
    if (!lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_BACK) || lagoon_chart.page != LAGOON_ROUTES ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) || lagoon_chart.page != LAGOON_MAP)
        return false;

    const LagoonPosition before = lagoon_explored.visits[3];
    lagoon_explored.visits[3] = (LagoonPosition){120, 232};
    SDL_Event trigger = {0};
    trigger.type = SDL_CONTROLLERAXISMOTION;
    trigger.caxis.which = SDL_JoystickInstanceID(pad);
    trigger.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    trigger.caxis.value = 28000;
    bool running = true;
    bool verified = lagoon_control_event(&trigger, &running) && lagoon_chart.page == LAGOON_MAP &&
        lagoon_chart.shown == 3;
    trigger.caxis.value = 0;
    lagoon_control_event(&trigger, &running);
    trigger.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    trigger.caxis.value = 28000;
    verified = lagoon_control_event(&trigger, &running) && lagoon_chart.page == LAGOON_MAP &&
        lagoon_chart.shown == 1 && verified;
    trigger.caxis.value = 0;
    lagoon_control_event(&trigger, &running);
    lagoon_explored.visits[3] = before;
    if (verified && running) fprintf(stderr, "Controller Atlas tabs and room browsing verified\n");
    return verified && running;
}

static bool lagoon_test_settings(SDL_Joystick *pad) {
    rrhk_options_init(L"X68000");
    const int before = lagoon_sound_volume();
    SDL_Event trigger = {0};
    trigger.type = SDL_CONTROLLERAXISMOTION;
    trigger.caxis.which = SDL_JoystickInstanceID(pad);
    trigger.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    trigger.caxis.value = 28000;
    bool running = true;
    if (!lagoon_control_event(&trigger, &running) || !running || !lagoon_chart.open) return false;
    trigger.caxis.value = 0;
    lagoon_control_event(&trigger, &running);
    if (lagoon_chart.page != LAGOON_SETTINGS || lagoon_chart.settings_row != LAGOON_VOLUME ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || lagoon_sound_volume() != before - 5 ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || lagoon_chart.settings_row != LAGOON_MUTE ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_A) || !rrhk_sound_muted() ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        lagoon_chart.settings_row != LAGOON_VIEW_ZOOM ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || lagoon_zoom_mode != 1 ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN) ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_A) || !lagoon_chart.confirm_quit ||
        lagoon_chart.quit_selected || !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_BACK) ||
        lagoon_chart.page != LAGOON_SETTINGS || !lagoon_chart.confirm_quit ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_B) || !lagoon_chart.open || lagoon_chart.confirm_quit ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_B) || lagoon_chart.open ||
        !x_run_frame(0, 0) || !x_run_frame(0, 0)) return false;
    rrhk_load_bindings();
    rrhk_options_init(L"X68000");
    if (lagoon_sound_volume() != before - 5 || !rrhk_sound_muted() || lagoon_load_zoom() != 1) return false;
    trigger.caxis.axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    trigger.caxis.value = 28000;
    if (!lagoon_control_event(&trigger, &running) || !running || lagoon_zoom_mode != 2) return false;
    if (lagoon_control_event(&trigger, &running) && lagoon_zoom_mode != 2) return false;
    trigger.caxis.value = 0;
    lagoon_control_event(&trigger, &running);
    lagoon_zoom_mode = 0;
    fprintf(stderr, "Controller Settings, modal focus, and audio persistence verified\n");
    return true;
}

static bool lagoon_test_controller(SDL_Joystick *pad) {
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || lagoon_byte(ram, 0xeccc) != 1) return false;
    const int start_x = lagoon_word(ram, 0x402e), start_y = lagoon_word(ram, 0x4030);
    /* Adventure retains the original 41-sync transition; Expedition is quicker. */
    const unsigned menu_settle_frames = 60;
    if (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, 8, menu_settle_frames) ||
        lagoon_byte(ram, 0x4d2f) != 1 || lagoon_word(ram, 0x70ef) != 0x1d16 ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 10, menu_settle_frames) ||
        lagoon_byte(ram, 0x4d2f)) {
        fprintf(stderr, "Controller Items failed\n");
        return false;
    }
    if (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 8, menu_settle_frames) ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 10, 12) ||
        lagoon_byte(ram, 0x4d2f) != 1 || lagoon_word(ram, 0x70ef) != 0x1c0f ||
        lagoon_word(ram, 0x162f0) != 0x48 ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 10, menu_settle_frames) ||
        lagoon_byte(ram, 0x4d2f)) {
        fprintf(stderr, "Controller Equipment failed\n");
        return false;
    }

    rrhk_ui()->buttons[0][X_EQUIPMENT] = SDL_CONTROLLER_BUTTON_X;
    if (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, 8, 22) ||
        lagoon_byte(ram, 0x4d2f) ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_X, 8, menu_settle_frames) ||
        lagoon_word(ram, 0x70ef) != 0x1c0f ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 10, menu_settle_frames) ||
        lagoon_byte(ram, 0x4d2f)) {
        fprintf(stderr, "Remapped Equipment failed\n");
        return false;
    }
    if (!lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK) || !lagoon_chart.open) {
        fprintf(stderr, "Default Atlas controller binding failed\n");
        return false;
    }
    /* Y zooms the Atlas. The game's original Save / Load stays closed. */
    if (!lagoon_test_atlas_tabs(pad) || !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_Y) || lagoon_chart.zoom != 2 ||
        lagoon_byte(ram, 0x4d2f) ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK) || lagoon_chart.open ||
        !x_run_frame(0, 0) || !x_run_frame(0, 0)) {
        fprintf(stderr, "Atlas controller input isolation failed\n");
        return false;
    }
    if (!lagoon_test_settings(pad)) {
        fprintf(stderr, "Controller Settings failed\n");
        return false;
    }
    rrhk_ui()->buttons[0][X_EQUIPMENT] = SDL_CONTROLLER_BUTTON_X;
    rrhk_ui()->buttons[0][X_ATLAS] = SDL_CONTROLLER_BUTTON_LEFTSTICK;
    if (!lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_RIGHTSTICK) || lagoon_chart.open ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_LEFTSTICK) || !lagoon_chart.open ||
        !lagoon_pad_event(pad, SDL_CONTROLLER_BUTTON_LEFTSTICK) || lagoon_chart.open ||
        !x_run_frame(0, 0) || !x_run_frame(0, 0)) {
        fprintf(stderr, "Remapped Atlas failed\n");
        return false;
    }

    if (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_Y, 10, 45) ||
        lagoon_byte(ram, 0x16172) != 1 || lagoon_byte(ram, 0xc043) != 0 ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 10, 34) ||
        lagoon_byte(ram, 0xc043) != 1 ||
        (lagoon_word(ram, 0xafea) == 1 &&
         (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN, 10, 34) || lagoon_byte(ram, 0xc043) != 1)) ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 14, 210) ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 14, 400) ||
        lagoon_byte(ram, 0x4d2f)) {
        fprintf(stderr, "Controller Save failed (menu %d, mode %d, choice %d)\n",
                lagoon_byte(ram, 0x4d2f), lagoon_byte(ram, 0x16172), lagoon_byte(ram, 0xc043));
        return false;
    }
    if (!lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, 18, 12) ||
        lagoon_word(ram, 0x402e) == start_x ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_Y, 10, 45) ||
        lagoon_byte(ram, 0x16172) != 1 || lagoon_byte(ram, 0xc043) != 0 ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 14, 210) ||
        !lagoon_pad_run(pad, SDL_CONTROLLER_BUTTON_B, 14, 400) ||
        lagoon_byte(ram, 0x4d2f) || lagoon_word(ram, 0x402e) != start_x ||
        lagoon_word(ram, 0x4030) != start_y) {
        fprintf(stderr, "Controller Load failed (position %d,%d; expected %d,%d)\n",
                lagoon_word(ram, 0x402e), lagoon_word(ram, 0x4030), start_x, start_y);
        return false;
    }
    return true;
}

static bool lagoon_check_controls(void) {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)) return false;
    int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                         SDL_CONTROLLER_BUTTON_MAX, 0);
    SDL_GameController *controller = index >= 0 ? SDL_GameControllerOpen(index) : NULL;
    bool verified = false;
    if (controller) {
        SDL_GameControllerEventState(SDL_ENABLE);
        rrhk_bind(rrhk_adapter());
        rrhk_ui()->pads[0] = controller;
        ca.windowed = true;
        frame_limit = 0;
        core.set_input_state(lagoon_input_state);
        verified = lagoon_test_controller(SDL_GameControllerGetJoystick(controller));
        core.set_input_state(input_state);
        ca.windowed = false;
        rrhk_ui()->pads[0] = NULL;
        SDL_GameControllerClose(controller);
    }
    if (index >= 0) SDL_JoystickDetachVirtual(index);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO);
    if (verified) fprintf(stderr, "Controller Items, Equipment, Save and Load verified\n");
    return verified;
}
#endif
