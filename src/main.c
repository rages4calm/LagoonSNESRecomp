/*
 * Lagoon — desktop host shim.
 *
 * Identity and hooks, nothing else. The host itself is the framework's
 * (snesrecomp/runner/src/desktop/host_main.h): the pre-boot launcher, ROM
 * resolution and digest checks, config.ini and keybinds.ini, the window and
 * the SDL / OpenGL presenters, audio, gamepads, the in-game save-state
 * browser and rewind filmstrip, the OSD, the pacing clock, crash handlers and
 * the post-mortem report, mod packages, Generate & rebuild, and the netplay
 * barrier when the project is built with it. A fix there reaches this
 * project on a submodule pull; nothing in this file needs to change for it.
 *
 * This file used to BE the host -- a ~700-line copy made at scaffold time --
 * so every project scaffolded before a fix kept the bug. Two ports of the
 * same game, a week apart, had different hosts.
 *
 * Nothing here ever ships a ROM. The player supplies one; the host makes
 * that easy (launcher, then positional argument, then a copy beside the
 * executable, then the rom.cfg cache, then a file picker) and checks what it
 * is handed against the digests in rom_identity.txt.
 *
 * Grow this file only with what is specific to THIS title: a custom
 * presenter (prepare_frame / draw_frame), an SPC player, a pacing rule
 * (keep_pacing_debt), a Mods provider. See SnesDesktopHostGame for the
 * complete list of hooks and what each is for.
 */

#include "host_main.h"
#include "game_rtl.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "lagoon_qol.h"               /* opt-in quality-of-life plugins */
#include "lagoon_widescreen.h"
#include <string.h>

#ifndef __ANDROID__
#define SDL_MAIN_HANDLED 1
#endif
#include "desktop/sdl_compat.h"
#ifdef __ANDROID__
/* SDLActivity loads libmain.so and dlsyms its SDL_main. */
#define main SDL_main
#endif

#ifndef SNES_GAME_VERSION
#define SNES_GAME_VERSION "dev"
#endif

static const SnesDesktopHostGame kGameHost = {
    .display_name        = "Lagoon",
    .window_title        = "Lagoon",
    .region              = SNESRECOMP_ROM_REGION,
    .rom_file            = SNESRECOMP_ROM_FILE,
    .expected_sha256_hex = SNESRECOMP_ROM_EXPECTED_SHA256,
    .expected_crc32_hex  = SNESRECOMP_ROM_EXPECTED_CRC32,
    .game_id             = SNESRECOMP_ROM_GAME_ID,
    .build_version       = SNES_GAME_VERSION,
    .game_info           = &kGameInfo,
    .num_players         = 1,
    /* Battery-backed SRAM shows the launcher's SAVES panel. Leave NULL for a
     * title without one. The path is exe-relative. */
    .sram_path           = "saves/save.srm",
    /* Runs on the loaded image before the machine exists, which is where the
     * quality-of-life features patch ROM data. Nothing happens unless the
     * player has enabled a feature: the faithful game is the default. */
    .on_rom_loaded       = LagoonQol_OnRomLoaded,
    .widescreen_supported = 1,
    .display_aspect_supported = 1,
    .native_widescreen    = 1,
    .after_config        = LagoonWide_AfterConfig,
    .prepare_frame       = LagoonWide_PrepareFrame,
    .begin_sim_frame     = LagoonWide_BeginFrame,
    .end_sim_frame       = LagoonWide_EndFrame,
    .draw_frame          = LagoonWide_DrawFrame,
    .on_reset            = LagoonWide_Reset,
};

#ifndef __ANDROID__
#undef main   /* desktop: keep plain main() even if SDL_main.h remapped it */
#endif
int main(int argc, char **argv)
{
    for (int i = 1; i + 1 < argc; ++i)
        if (!strcmp(argv[i], "--config")) LagoonWide_SetConfigPath(argv[i + 1]);
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}
