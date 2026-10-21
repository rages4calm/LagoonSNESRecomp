/*
 * Lagoon — per-game runtime glue.
 *
 * THIS FILE IS THE PORT. Everything else the scaffold produced is layout,
 * build wiring, and packaging; this is where the actual work happens.
 *
 * The framework does not, and cannot, drive an arbitrary SNES game on its
 * own. Two responsibilities always land on the game host:
 *
 *   1. Deciding what "one frame" means for THIS title, and returning from
 *      run_frame() at that boundary. A real ROM's reset vector never
 *      returns — it enters a main loop that waits on vblank — so somebody
 *      has to choose the yield point.
 *   2. Delivering NMI/IRQ at the hardware edge (see
 *      snesrecomp/docs/FRAME_MODEL_HOSTS.md).
 *
 * A new port starts on the framework's beam-aligned driver
 * (runner/src/beam_frame_driver.h): one frame is one PPU field, cut at the
 * beam's vblank edge; interrupts -- raster splits and a coprocessor's IRQ
 * line -- are taken where the beam latches them; parked time still runs the
 * beam, APU and coprocessors; and the field is rasterized from the raster
 * journal with real HDMA. It is the framework's, so a fix to it reaches this
 * port on a submodule pull.
 *
 * This file used to hold its own copy of a simpler driver: frames measured
 * from wherever the CPU last stopped, NMI delivered at an arbitrary beam
 * position, the field drawn with end-of-frame register state. A game that
 * uploads in vblank between INIDISP=$80 and $0F flickered black, and one
 * that chains raster splits fell apart. Replace these calls only for what
 * the driver does not do; MetalWarriorsSNESRecomp's src/mw_rtl.c and
 * GundamWingEndlessDuelSNESRecomp's src/game_rtl.c are developed examples of
 * title-specific drivers.
 */

#include "game_rtl.h"
#include "lagoon_native_recovery.h"
#include "lagoon_menu_close.h"
#include "lagoon_menu_dma.h"
#include "lagoon_soundtrack.h"
#include "lagoon_state.h"
#include "lagoon_follower.h"
#include "lagoon_widescreen.h"
#include "lagoon_campaign_debug.h"

#include "beam_frame_driver.h"
#include "common_rtl.h"

void GameRunOneFrame(void)
{
    LagoonFollower_BeforeFrame();
    LagoonNativeRecovery_BeforeFrame();
    LagoonMenuClose_BeforeFrame();
    LagoonMenuDma_BeforeFrame();
    LagoonCampaignDebug_BeforeFrame();
    snes_beam_frame_driver_run_frame();
}

void GameDrawPpuFrame(void)
{
    snes_beam_frame_driver_draw_ppu_frame_observed(LagoonWide_ObserveRaster, NULL);
}

void GameSessionReset(void)
{
    /* Rematch / soft-return: clear anything that must not survive a new
     * session. recomp-ai-rules/NETPLAY.md §3 — sticky state that "has always
     * been fine" is the usual desync culprit, because single-player never
     * re-enters the boot path twice in one process. */
    LagoonFollower_Discard();
    snes_beam_frame_driver_reset();
    LagoonSoundtrack_Reset();
}

/* In-memory rollback must invalidate disposable escort authority. While an
 * escort quest is active, decline speculative frames so a six-step trail is
 * not repeatedly discarded by run-ahead. File save schema remains unchanged. */
static size_t GameExecBound(void) { return (g_ram[0x4e1]&3)?0:1; }
static size_t GameExecSave(void *out,size_t capacity) {
    if(!out||!capacity)return 0;
    *(uint8_t *)out=0;return 1;
}
static int GameExecLoad(const void *in,size_t size) {
    if(!in||size!=1||*(const uint8_t *)in)return 0;
    LagoonFollower_Discard();return 1;
}

const RtlGameInfo kGameInfo = {
    .title = "lagoon",
    .initialize = NULL,
    .run_frame = &GameRunOneFrame,
    .draw_ppu_frame = &GameDrawPpuFrame,
    .save_name_prefix = "save",
    .session_reset = &GameSessionReset,
    .hardware_reset = &GameSessionReset,
    .state_save_extra = &LagoonState_SaveExtra,
    .state_load_extra = &LagoonState_LoadExtra,
    .on_state_loaded = &LagoonState_OnLoaded,
    .snapshot_preflight = &LagoonState_Preflight,
    .snapshot_guard_identity = &LagoonState_Identity,
    .exec_state_bound = GameExecBound,
    .exec_state_save = GameExecSave,
    .exec_state_load = GameExecLoad,
};
