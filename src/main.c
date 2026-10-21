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
#include "lagoon_frame_probe.h"
#include "lagoon_map.h"
#include "lagoon_follower.h"
#include "lagoon_quest_guidance.h"
#include "lagoon_settings.h"
#include "lagoon_sword_visual.h"
#include "snes/ppu.h"
#include "config.h"
#include "common_rtl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

static const char *lagoon_config_path="config.ini";
static void LagoonHost_AfterConfig(void) {
    LagoonWide_AfterConfig();
    LagoonSettings_Configure(lagoon_config_path);
}
static int LagoonHost_Gamepad(int button,int pressed) {
    if(button==kGamepadBtn_L2){
        if(pressed&&!LagoonSettings_IsOpen())LagoonMap_Command(LAGOON_MAP_TOGGLE);
        return 1;
    }
    if(button==kGamepadBtn_R2){
        if(pressed){
            if(LagoonSettings_IsOpen())LagoonSettings_Close();
            else {LagoonMap_Command(LAGOON_MAP_CLOSE);LagoonSettings_Open();}
        }return 1;
    }return 0;
}
static int LagoonHost_PollOverlay(uint32_t human) {
    return LagoonSettings_IsOpen()?LagoonSettings_PollOpen(human):LagoonMap_PollOpen(human);
}
static void LagoonHost_TickOverlay(uint32_t human,uint32_t ticks) {
    if(LagoonSettings_IsOpen())LagoonSettings_Tick(human,ticks);else LagoonMap_Tick(human,ticks);
}
static int LagoonHost_OverlayImage(const uint32_t **pixels,int *width,int *height) {
    return LagoonSettings_IsOpen()?LagoonSettings_Image(pixels,width,height):LagoonMap_Image(pixels,width,height);
}
static void LagoonHost_OnRom(const uint8_t *rom,size_t size) {
    LagoonMap_Init(rom,size);
    LagoonQuestGuidance_Init(rom,size);
    LagoonQol_OnRomLoaded(rom,size);
    LagoonFollower_Reset(rom,size);
}
static unsigned long long sword_appends;
static const char *sword_report_path;
static void LagoonHost_SwordReport(void){if(sword_report_path){FILE*f=fopen(sword_report_path,"w");if(f){fprintf(f,"{\"source_qualified_sword_frames\":%llu}\n",sword_appends);fclose(f);}}}
static void LagoonHost_BeginFrame(unsigned frame) {
    static int initialized;if(!initialized){initialized=1;sword_report_path=getenv("LAGOON_SWORD_REPORT");if(sword_report_path&&*sword_report_path)atexit(LagoonHost_SwordReport);}
    LagoonWide_BeginFrame(frame);
    if(g_ppu)sword_appends+=LagoonSwordVisual_Prepare(g_ppu,g_rom,0x100000,g_ram,
        LagoonQol_SwordReachEnabled() && (!g_config.widescreen || g_ppu->wsLayerWidenMask==3));
}
static void LagoonHost_AfterFrame(const SnesDesktopHostFrameStats *stats) {
    LagoonFrameProbe_Record(stats);
    LagoonMap_Observe(g_ram,RtlStateGeneration());
    LagoonMap_AfterFrame(stats->frame);
}
static int LagoonHost_MapKey(int key,int modifiers,int pressed) {
    if(key==SDLK_COMMA&&(modifiers&KMOD_CTRL)){
        if(pressed){if(LagoonSettings_IsOpen())LagoonSettings_Close();else {LagoonMap_Command(LAGOON_MAP_CLOSE);LagoonSettings_Open();}}return 1;
    }
    if(LagoonSettings_IsOpen()){
        if(pressed)switch(key){
        case SDLK_ESCAPE:case SDLK_B:LagoonSettings_Close();break;
        case SDLK_UP:LagoonSettings_Command(LAGOON_SETTINGS_UP);break;
        case SDLK_DOWN:LagoonSettings_Command(LAGOON_SETTINGS_DOWN);break;
        case SDLK_LEFT:LagoonSettings_Command(LAGOON_SETTINGS_LEFT);break;
        case SDLK_RIGHT:LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);break;
        default:break;
        }return 1;
    }
    int opened=LagoonMap_IsOpen();
    static int m_held;
    if(key==SDLK_M){if(pressed&&!m_held)LagoonMap_Command(LAGOON_MAP_TOGGLE);m_held=pressed;return 1;}
    if(!opened||!pressed)return 0;
    switch(key){
    case SDLK_ESCAPE:case SDLK_B:LagoonMap_Command(LAGOON_MAP_CLOSE);break;
    case SDLK_LEFT:LagoonMap_Command(LAGOON_MAP_PREVIOUS);break;
    case SDLK_RIGHT:LagoonMap_Command(LAGOON_MAP_NEXT);break;
    case SDLK_Y:LagoonMap_Command(LAGOON_MAP_HOME);break;
    default:break;
    }return 1;
}

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
    .on_rom_loaded       = LagoonHost_OnRom,
    .widescreen_supported = 1,
    .display_aspect_supported = 1,
    .native_widescreen    = 1,
    .after_config        = LagoonHost_AfterConfig,
    .prepare_frame       = LagoonWide_PrepareFrame,
    .begin_sim_frame     = LagoonHost_BeginFrame,
    .end_sim_frame       = LagoonWide_EndFrame,
    .draw_frame          = LagoonWide_DrawFrame,
    .on_reset            = LagoonWide_Reset,
    .after_run_frame     = LagoonHost_AfterFrame,
    .on_shutdown         = LagoonMap_Save,
    .title_gamepad_button = LagoonHost_Gamepad,
    .title_overlay_key   = LagoonHost_MapKey,
    .title_overlay_poll_open = LagoonHost_PollOverlay,
    .title_overlay_tick = LagoonHost_TickOverlay,
    .title_overlay_image = LagoonHost_OverlayImage,
};

#ifndef __ANDROID__
#undef main   /* desktop: keep plain main() even if SDL_main.h remapped it */
#endif
int main(int argc, char **argv)
{
    for (int i = 1; i + 1 < argc; ++i)
        if (!strcmp(argv[i], "--config")) {LagoonWide_SetConfigPath(argv[i + 1]);lagoon_config_path=argv[i+1];}
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}
