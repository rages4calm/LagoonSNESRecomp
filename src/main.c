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
#include "desktop/host_api.h"
#include "game_rtl.h"
#include "lagoon_native_backend69.h"
#include "lagoon_idle_poll.h"
#include "snes/cart.h"
#include "snesrecomp_rom_identity.h"  /* generated from rom_identity.txt */
#include "lagoon_qol.h"               /* opt-in quality-of-life plugins */
#include "lagoon_widescreen.h"
#include "lagoon_frame_probe.h"
#include "lagoon_map.h"
#include "lagoon_scene_renderer.h"
#include "lagoon_follower.h"
#include "lagoon_quest_guidance.h"
#include "lagoon_settings.h"
#include "lagoon_sword_visual.h"
#include "lagoon_journal.h"
#include "lagoon_item_guide.h"
#include "lagoon_return_runtime.h"
#include "lagoon_achievements.h"
#include "lagoon_host_path.h"
#include "lagoon_achievement_ui.h"
#include "lagoon_achievement_sound.h"
#include "lagoon_easter_egg.h"
#include "lagoon_easter_collision.h"
#include "lagoon_achievement_ending.h"
#include "lagoon_wide_ai.h"
#include "lagoon_boss_hud.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include "config.h"
#include "common_rtl.h"
#include "snes_overlay_draw.h"
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
static int achievement_session, achievement_diagnostic;
static uint64_t achievement_generation;
static int achievement_ending_display;
static uint32_t cameo_laugh_until;
static void LagoonHost_Generation(void) {
    uint64_t generation=RtlStateGeneration();
    if(generation!=achievement_generation){
        LagoonAchievements_OnLoaded(generation);
        LagoonEasterEgg_Observe(NULL,0,generation,0);
        LagoonAchievementEnding_Reset(generation);
        achievement_generation=generation;achievement_ending_display=0;
    }
}
static void LagoonHost_EndingOpcode(CpuState *cpu,uint32_t pc) {
    if(RtlSpeculativeFrame())return;
    LagoonHost_Generation();
    if(cpu==&g_cpu&&LagoonAchievementEnding_Observe(cpu,pc,RtlStateGeneration())){
        achievement_ending_display=1;
        LagoonAchievements_CompletedEnding(RtlStateGeneration());
        const char *report=getenv("LAGOON_ACHIEVEMENT_ENDING_REPORT");
        if(report&&!strcmp(report,"1"))
            fprintf(stderr,"[ending-witness] completed pc=%06X generation=%llu sp=%04X counter=%u\n",
                    pc,(unsigned long long)RtlStateGeneration(),cpu->S,cpu->ram[0x1099]);
    }
}
static int present(const char *key){const char *p=getenv(key);return p&&*p&&strcmp(p,"0");}
/* Opt-in host evidence only: the bridge reports real compiled-body entries.
 * No CPU/stack mutation, injected events, or change to admission policy. */
static uint64_t lagoon_wait_native_entries, lagoon_queue_wrapper_entries, lagoon_dma_request_entries, lagoon_dma_drain_entries, lagoon_redraw_entries, lagoon_scene_entries, lagoon_scene_dispatch_entries;
static void (*lagoon_previous_bounce_hook)(uint32_t,int,int);
static void (*lagoon_previous_pc_hook)(uint32_t,int,int);
static uint64_t lagoon_idle_ops;
static uint64_t lagoon_framewait_ops[4],lagoon_display_entries,lagoon_framewait_entries;
static int lagoon_idle_declared,lagoon_framewait_declared;
static void LagoonHost_IdleOpcode(uint32_t pc,int m,int x) {
    if(pc==0x01814e||pc==0x018151||pc==0x018153)++lagoon_idle_ops;
    if(pc==0x0084e3||pc==0x0084e6)++lagoon_framewait_ops[((m&1)<<1)|(x&1)];
    if(pc==0x0084e9&&m==1&&x==1)++lagoon_display_entries;
    if(pc==0x0084de&&m==1&&x==1)++lagoon_framewait_entries;
    if(lagoon_previous_pc_hook)lagoon_previous_pc_hook(pc,m,x);
}
static void LagoonHost_ConfigureIdlePoll(void) {
    const Cart *cart=g_snes?g_snes->cart:NULL;
    bool enabled=!getenv("LAGOON_IDLE_POLL_PROBE")||present("LAGOON_IDLE_POLL_PROBE");
    bool source=cart&&cart->rom&&cart->type==CART_LOROM&&cart->romSize>=0x8158;
    if(source) {
        for(unsigned i=0;i<10&&source;++i)
            source=cpu_aot_rom_mapping_matches(0x01814eu + i,0x814eu + i,1)!=0;
        for(unsigned i=0;i<3&&source;++i)
            source=cpu_aot_rom_mapping_matches(0x018088+i,0x8088+i,1)&&
                   cpu_aot_rom_mapping_matches(0x008922+i,0x0922+i,1);
        source=source&&LagoonIdle_SourceMatches(cart->rom+0x814e,
                            cart->rom+0x8088,cart->rom+0x0922);
    }
    bool frame_enabled=!getenv("LAGOON_FRAME_WAIT62")||present("LAGOON_FRAME_WAIT62");
    bool frame_source=cart&&cart->rom&&cart->type==CART_LOROM&&cart->romSize>=0x923;
    if(frame_source){
        for(unsigned i=0;i<5&&frame_source;++i)
            frame_source=cpu_aot_rom_mapping_matches(0x0084e3u+i,0x04e3u+i,1)&&
                         cpu_aot_rom_mapping_matches(0x0084deu+i,0x04deu+i,1);
        for(unsigned i=0;i<7&&frame_source;++i)
            frame_source=cpu_aot_rom_mapping_matches(0x00891bu+i,0x091bu+i,1)!=0;
        frame_source=frame_source&&LagoonFrameWait_SourceMatches(cart->rom+0x4e3,
                            cart->rom+0x4de,cart->rom+0x91b);
    }
    /* Revalidate both title-owned declarations on every frame/state load.
     * Clearing a withdrawn declaration also retires its cached loop authority. */
    if((lagoon_idle_declared&&(!enabled||!source))||
       (lagoon_framewait_declared&&(!frame_enabled||!frame_source)))
        interp_bridge_clear_idle_polls();
    lagoon_idle_declared=enabled&&source?interp_bridge_set_idle_poll(0x01814e):0;
    lagoon_framewait_declared=frame_enabled&&frame_source?interp_bridge_set_idle_poll(0x0084e3):0;
}


static void LagoonHost_WaitBounce(uint32_t pc,int m,int x) {
    if(pc==0x0081f7&&m==1&&x==0)++lagoon_wait_native_entries;
    if(pc==0x0081be&&m==1&&x==0)++lagoon_queue_wrapper_entries;
    if(pc==0x00899b&&m==1&&x==0)++lagoon_dma_request_entries;
    if(pc==0x008985&&m==1&&x==0)++lagoon_dma_drain_entries;
    if(pc==0x01a4d6&&m==1&&x==0)++lagoon_redraw_entries, lagoon_scene_entries, lagoon_scene_dispatch_entries;
    if(pc==0x01c643&&m==1&&x==0)++lagoon_scene_dispatch_entries;
    if(pc==0x01c726&&m==1&&x==0)++lagoon_scene_entries, lagoon_scene_dispatch_entries;
    if(lagoon_previous_bounce_hook)lagoon_previous_bounce_hook(pc,m,x);
}
static void LagoonHost_StartSession(void) {
    if(achievement_session)return;
    achievement_session=1;
    achievement_diagnostic=present("SNESRECOMP_SCRIPT")||present("SNESRECOMP_RUN_FRAMES")||
        present("LAGOON_SCRIPT_FIXTURE")||present("LAGOON_CAMPAIGN_WARPS")||
        present("LAGOON_ACHIEVEMENTS_DIAGNOSTIC");
    char path[4096];const char *save=RtlSaveRoot();
    if(LagoonHostPath_IsAbsolute(save,LAGOON_HOST_WINDOWS_PATHS))
        snprintf(path,sizeof path,"%s",save);
    else {
        char *cwd=SDL_GetCurrentDirectory();
        snprintf(path,sizeof path,"%s%s",cwd?cwd:"",save?save:"saves");SDL_free(cwd);
    }
    LagoonAchievements_Init(path,achievement_diagnostic!=0);
    LagoonAchievementUI_Reset();
    achievement_generation=RtlStateGeneration();
    LagoonAchievementSound_Configure(g_config.enable_audio&&!achievement_diagnostic);
    const char *base=SDL_GetBasePath();char laugh_path[4096];
    snprintf(laugh_path,sizeof laugh_path,"%sassets/easter_egg/B1.wav",base?base:"");
    LagoonAchievementSound_LoadLaugh(laugh_path);
}
static int LagoonHost_GameplayReady(void) {
    return g_ppu&&g_ram[0x48f]==0&&g_ram[0x48e]<75&&!g_ram[0x100e]&&
        (g_ram[0x500]&0xc0)==0xc0&&g_ram[0x520]&&!g_ram[0x521]&&g_ppu->inidisp==15;
}
static void LagoonHost_OpenAchievements(void) {
    LagoonMap_Command(LAGOON_MAP_CLOSE);LagoonSettings_Close();LagoonEasterEgg_Close();
    LagoonAchievementUI_Open();
}
/* Bounded opt-in UI integration check; the normal host/input path still opens,
 * draws and closes the panel. No guest time, progress or settings are written. */
static unsigned menu_test_pumps;
static void LagoonHost_ReturnSelftest(unsigned frame);
static void LagoonHost_MenuSelftest(unsigned frame) {
    static int at=-2;
    if(at==-2){const char *v=getenv("LAGOON_MENU_SELFTEST_FRAME");at=v?atoi(v):-1;}
    if(at<0||frame<(unsigned)at||!LagoonHost_GameplayReady())return;
    at=-1;const char *value=getenv("LAGOON_MENU_SELFTEST_PAGE");int page=value?atoi(value):0;
    if(page<0||page>6)return;
    LagoonMap_Command(LAGOON_MAP_CLOSE);
    if(page==3)LagoonHost_OpenAchievements();
    else {LagoonSettings_Open();for(int i=0;i<(page>=5?4:page==4?3:page);i++)LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);
        if(page>=5){LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);
            LagoonSettings_SetTravelStatus(status.message,status.reason==LAGOON_RETURN_READY);
            if(page==6)LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);}}
    menu_test_pumps=1;
}
/* Opt-in read-only integration observation. Normal sessions do no file I/O. */
static void LagoonHost_TestObserve(unsigned frame) {
    static unsigned last_frame;static uint32_t last_at;
    const char *path=getenv("LAGOON_TEST_OBSERVATION");if(!path||!*path)return;
    if(frame!=~0u)last_frame=frame;
    uint32_t now=SDL_GetTicks();if(now-last_at<100)return;last_at=now;
    FILE *f=fopen(path,"w");if(!f)return;
    fprintf(f,"{\"frame\":%u,\"room\":%u,\"x\":%u,\"y\":%u,\"facing\":%u,\"hp\":%u,\"ready\":%d,\"dialogue\":%d,\"settings\":%d,\"map\":%d,\"achievements\":%d,\"generation\":%llu}\n",
        last_frame,g_ram[0x48e]|g_ram[0x48f]<<8,g_ram[0x502]|g_ram[0x503]<<8,
        g_ram[0x504]|g_ram[0x505]<<8,g_ram[0x50a],g_ram[0x520]|g_ram[0x521]<<8,
        LagoonHost_GameplayReady(),LagoonEasterEgg_IsOpen(),LagoonSettings_IsOpen(),
        LagoonMap_IsOpen(),LagoonAchievementUI_IsOpen(),(unsigned long long)RtlStateGeneration());
    fclose(f);
}
static void LagoonHost_SettingsRequest(void) {
    if(LagoonSettings_TakeAchievementsRequest())LagoonHost_OpenAchievements();
    if(LagoonSettings_TakeReturnRequest()){
        LagoonReturnStatus status;
        if(!LagoonReturnRuntime_Request(&status)){
            LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);
            LagoonSettings_SetTravelStatus(status.message,0);
        }
    }
}
static void LagoonHost_TravelStatus(void){
    LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);
    LagoonSettings_SetTravelStatus(status.message,status.reason==LAGOON_RETURN_READY);
}
static void LagoonHost_ReturnSelftest(unsigned frame){
    static int at=-2;
    if(at==-2){const char*v=getenv("LAGOON_RETURN_SELFTEST_FRAME");at=v?atoi(v):-1;}
    if(at<0||frame<(unsigned)at)return;at=-1;
    LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);
    fprintf(stderr,"[lagoon-return-test] frame=%u reason=%u %s\n",frame,status.reason,status.message);
    LagoonSettings_Open();LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);
    LagoonHost_TravelStatus();LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);
    LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);LagoonHost_SettingsRequest();
    /* A refused request leaves no modal running in an unattended fixture. */
    if(LagoonSettings_IsOpen())LagoonSettings_Close();
}
static void LagoonHost_AfterConfig(void) {
    LagoonWide_AfterConfig();
    LagoonSettings_Configure(lagoon_config_path);
}
static int LagoonHost_Gamepad(int button,int pressed) {
    if(button==kGamepadBtn_L2){
        if(pressed&&!LagoonSettings_IsOpen()&&!LagoonAchievementUI_IsOpen()&&!LagoonEasterEgg_IsOpen())LagoonMap_Command(LAGOON_MAP_TOGGLE);
        return 1;
    }
    if(button==kGamepadBtn_R2){
        if(pressed){
            if(LagoonAchievementUI_IsOpen())LagoonAchievementUI_Close();
            else if(LagoonEasterEgg_IsOpen())LagoonEasterEgg_Close();
            else if(LagoonSettings_IsOpen())LagoonSettings_Close();
            else {LagoonMap_Command(LAGOON_MAP_CLOSE);LagoonSettings_Open();}
        }return 1;
    }return 0;
}
static int LagoonHost_PollOverlay(uint32_t human) {
    LagoonHost_StartSession();LagoonHost_Generation();LagoonHost_SettingsRequest();
    if(LagoonSettings_IsOpen())LagoonHost_TravelStatus();
    LagoonEasterCollision_BeforeFrame();
    if(LagoonAchievementUI_IsOpen())return LagoonAchievementUI_PollOpen(human);
    if(LagoonEasterEgg_IsOpen())return LagoonEasterEgg_PollOpen(human);
    if(!LagoonSettings_IsOpen()&&!LagoonMap_IsOpen()&&!present("LAGOON_EASTER_EGG_DISABLE")&&
       LagoonEasterEgg_TryInteract(human))return LagoonEasterEgg_PollOpen(human);
    return LagoonSettings_IsOpen()?LagoonSettings_PollOpen(human):LagoonMap_PollOpen(human);
}
static void LagoonHost_TickOverlay(uint32_t human,uint32_t ticks) {
    if(LagoonSettings_IsOpen())LagoonHost_TravelStatus();
    LagoonAchievementSound_SetVolume((unsigned)snesrecomp_desktop_get_volume());
    LagoonAchievementUI_PauseToast(1,ticks);
    if(LagoonAchievementUI_IsOpen())LagoonAchievementUI_Tick(human,ticks);
    else if(LagoonEasterEgg_IsOpen())LagoonEasterEgg_Tick(human,ticks);
    else if(LagoonSettings_IsOpen())LagoonSettings_Tick(human,ticks);else LagoonMap_Tick(human,ticks);
    uint64_t generation;if(LagoonEasterEgg_TakeDiscovery(&generation)){
        LagoonAchievements_SecretVisit(generation);
        LagoonAchievementSound_PlayLaugh((unsigned)snesrecomp_desktop_get_volume());
        cameo_laugh_until=ticks+LagoonAchievementSound_LaughDurationMs()+100;
    }
    LagoonHost_SettingsRequest();LagoonHost_TestObserve(~0u);
    if(menu_test_pumps&&++menu_test_pumps>12){LagoonSettings_Close();LagoonAchievementUI_Close();menu_test_pumps=0;}
}
static int LagoonHost_OverlayImage(const uint32_t **pixels,int *width,int *height) {
    if(LagoonAchievementUI_IsOpen())return LagoonAchievementUI_Image(pixels,width,height);
    if(LagoonEasterEgg_IsOpen())return LagoonEasterEgg_Image(pixels,width,height);
    return LagoonSettings_IsOpen()?LagoonSettings_Image(pixels,width,height):LagoonMap_Image(pixels,width,height);
}
static int LagoonHost_CameoBody(const uint8_t *ram,uint64_t generation,int *x,int *y){
    return !present("LAGOON_EASTER_EGG_DISABLE")&&LagoonEasterEgg_CollisionBody(ram,generation,x,y);
}
static void LagoonHost_WideActorOpcode(CpuState *cpu,uint32_t pc) {
    LagoonWideAI_ObserveOpcode(cpu,pc,RtlStateGeneration());
}
static uint8_t LagoonHost_WorldRead(uint32_t address,uint8_t value,
    const CpuWramReadContext *context,const uint8_t *ram) {
    value=LagoonEasterCollision_FilterRead(address,value,context,ram);
    value=LagoonWideAI_FilterRead(address,value,context,ram,RtlStateGeneration());
    return LagoonReturnRuntime_FilterRead(address,value,context,ram);
}
static void LagoonHost_OnRom(const uint8_t *rom,size_t size) {
    /* Authenticate unpatched source before title QoL changes the live image. */
    LagoonNativeBackend69_OnRom(rom,size);
    LagoonMap_Init(rom,size);
    LagoonQuestGuidance_Init(rom,size);
    LagoonJournal_Init(rom,size);
    LagoonItemGuide_Init(rom,size);
    LagoonReturnRuntime_OnRom(rom,size);
    LagoonEasterEgg_Init(rom,size);
    LagoonAchievementEnding_Init(rom,size);
    interp_bridge_add_pre_opcode_hook(0x02e99d,LagoonHost_EndingOpcode);
    interp_bridge_add_pre_opcode_hook(0x02e9bd,LagoonHost_EndingOpcode);
    LagoonQol_OnRomLoaded(rom,size);
    LagoonWideAI_Init(rom,size,&g_cpu);
    LagoonBossHud_Init(rom,size);
    interp_bridge_add_pre_opcode_hook(0x02973a,LagoonHost_WideActorOpcode);
    interp_bridge_add_pre_opcode_hook(0x01ab31,LagoonHost_WideActorOpcode);
    LagoonEasterCollision_Init(rom,size,LagoonHost_CameoBody);
    LagoonQol_SetWorldReadFilter(LagoonHost_WorldRead);
    LagoonFollower_Reset(rom,size);
}
static unsigned long long sword_appends;
static const char *sword_report_path;
static void LagoonHost_SwordReport(void){if(sword_report_path){FILE*f=fopen(sword_report_path,"w");if(f){fprintf(f,"{\"source_qualified_sword_frames\":%llu}\n",sword_appends);fclose(f);}}}
static void LagoonHost_BeginFrame(unsigned frame) {
    static uint64_t travel_tick;
    LagoonReturnRuntime_BeforeFrame(++travel_tick,LagoonMap_IsOpen()||LagoonAchievementUI_IsOpen()||LagoonEasterEgg_IsOpen());
    LagoonSwordVisual_SetRangeCue(LagoonSettings_SwordRangeEnabled());
    static int initialized;if(!initialized){initialized=1;sword_report_path=getenv("LAGOON_SWORD_REPORT");if(sword_report_path&&*sword_report_path)atexit(LagoonHost_SwordReport);}
    LagoonWide_BeginFrame(frame);
    LagoonWideAI_BeginFrame(RtlStateGeneration(),g_ppu?g_ppu->extraLeftRight:0,LagoonHost_GameplayReady());
    LagoonEasterEgg_BeginFrame(RtlStateGeneration());
    if(g_ppu)sword_appends+=LagoonSwordVisual_Prepare(g_ppu,g_rom,0x100000,g_ram,
        LagoonQol_SwordReachEnabled() && (!g_config.widescreen || g_ppu->wsLayerWidenMask==3));
}
static void LagoonHost_AfterFrame(const SnesDesktopHostFrameStats *stats) {
    LagoonHost_StartSession();
    uint64_t generation=RtlStateGeneration();
    LagoonHost_Generation();
    int ready=LagoonHost_GameplayReady();
    LagoonAchievements_Observe(g_ram,0x20000,generation,ready!=0);
    LagoonEasterEgg_Observe(g_ram,0x20000,generation,ready);
    uint32_t ticks=SDL_GetTicks();
    LagoonAchievementSound_SetVolume((unsigned)snesrecomp_desktop_get_volume());
    int notices_visible=(ready||achievement_ending_display)&&!LagoonMap_IsOpen()&&!LagoonSettings_IsOpen()&&
        !LagoonEasterEgg_IsOpen()&&!LagoonAchievementUI_IsOpen();
    LagoonAchievementUI_PauseToast(!notices_visible||LagoonMap_IsOpen()||LagoonSettings_IsOpen()||
        LagoonEasterEgg_IsOpen()||LagoonAchievementUI_IsOpen(),ticks);
    LagoonAchievementUI_Tick(0,ticks);
    size_t notice;
    if(notices_visible&&(int32_t)(ticks-cameo_laugh_until)>=0&&!LagoonAchievementUI_ToastActive()&&LagoonAchievements_PopNotice(&notice)){
        LagoonAchievementUI_StartToast(notice,ticks);
        LagoonAchievementSound_Play((unsigned)snesrecomp_desktop_get_volume());
    }
    LagoonFrameProbe_Record(stats);
    LagoonMap_Observe(g_ram,RtlStateGeneration());
    if(ready&&snesrecomp_host_frame_is_live())
        LagoonSceneRenderer_ArchiveCommit(g_ram[0x48e],generation);
    LagoonReturnRuntime_AfterFrame();
    if((g_ram[0x500]&0xc0)==0xc0&&(g_ram[0x520]||g_ram[0x521]))LagoonSettings_Observe(g_ram,sizeof g_ram);
    else LagoonSettings_Observe(NULL,0);
    LagoonMap_AfterFrame(stats->frame);
    LagoonHost_MenuSelftest(stats->frame);
    LagoonHost_ReturnSelftest(stats->frame);
    LagoonHost_TestObserve(stats->frame);
}
static int LagoonHost_MapKey(int key,int modifiers,int pressed) {
    /* The framework modal callback passes key only; use SDL's live modifier
     * state there as well so the same close chord works while paused. */
    modifiers|=SDL_GetModState();
    if(present("SNESRECOMP_INPUT_TRACE"))fprintf(stderr,"[lagoon-host-key] key=%x modifiers=%x pressed=%d achievements=%d\n",key,modifiers,pressed,LagoonAchievementUI_IsOpen());
    if(key==SDLK_J&&(modifiers&KMOD_CTRL)){
        if(pressed){if(LagoonAchievementUI_IsOpen())LagoonAchievementUI_Close();else LagoonHost_OpenAchievements();}return 1;
    }
    if(LagoonAchievementUI_IsOpen()){
        if(pressed)switch(key){
        case SDLK_ESCAPE:case SDLK_B:LagoonAchievementUI_Close();break;
        case SDLK_UP:case SDLK_KP_8:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_UP);break;
        case SDLK_DOWN:case SDLK_KP_2:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_DOWN);break;
        case SDLK_LEFT:case SDLK_KP_4:case SDLK_PAGEUP:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_PREVIOUS);break;
        case SDLK_RIGHT:case SDLK_KP_6:case SDLK_PAGEDOWN:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_NEXT);break;
        case SDLK_HOME:case SDLK_KP_7:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_FIRST);break;
        case SDLK_END:case SDLK_KP_1:LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_LAST);break;
        default:break;
        }return 1;
    }
    if(LagoonEasterEgg_IsOpen()){
        if(pressed){
            if(key==SDLK_ESCAPE||key==SDLK_B)LagoonEasterEgg_Close();
            else if(key==SDLK_RETURN||key==SDLK_SPACE||key==SDLK_A||key==SDLK_X)LagoonEasterEgg_Tick(0,SDL_GetTicks()),LagoonEasterEgg_Tick(SNES_PAD_A,SDL_GetTicks());
        }return 1;
    }
    /* A separate host key also catches short keyboard taps between fields.
     * The proximity, facing and visible-scene guards still decide eligibility. */
    if(key==SDLK_E&&pressed&&!(modifiers&(KMOD_CTRL|KMOD_ALT))&&
       !LagoonSettings_IsOpen()&&!LagoonMap_IsOpen()&&!present("LAGOON_EASTER_EGG_DISABLE")){
        LagoonHost_Generation();
        if(LagoonEasterEgg_TryInteract(SNES_PAD_Y))return 1;
    }
    if(key==SDLK_COMMA&&(modifiers&KMOD_CTRL)){
        if(pressed){if(LagoonSettings_IsOpen())LagoonSettings_Close();else {LagoonMap_Command(LAGOON_MAP_CLOSE);LagoonSettings_Open();}}return 1;
    }
    if(LagoonSettings_IsOpen()){
        if(pressed)switch(key){
        case SDLK_PAGEUP:LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);break;
        case SDLK_PAGEDOWN:case SDLK_TAB:LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);break;
        case SDLK_ESCAPE:case SDLK_B:LagoonSettings_Command(LAGOON_SETTINGS_CLOSE);break;
        case SDLK_UP:case SDLK_KP_8:LagoonSettings_Command(LAGOON_SETTINGS_UP);break;
        case SDLK_DOWN:case SDLK_KP_2:LagoonSettings_Command(LAGOON_SETTINGS_DOWN);break;
        case SDLK_LEFT:case SDLK_KP_4:LagoonSettings_Command(LAGOON_SETTINGS_LEFT);break;
        case SDLK_RIGHT:case SDLK_KP_6:LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);break;
        case SDLK_RETURN:case SDLK_SPACE:LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);LagoonHost_SettingsRequest();break;
        default:break;
        }return 1;
    }
    int opened=LagoonMap_IsOpen();
    static int m_held;
    if(key==SDLK_M){if(pressed&&!m_held)LagoonMap_Command(LAGOON_MAP_TOGGLE);m_held=pressed;return 1;}
    if(!opened||!pressed)return 0;
    switch(key){
    case SDLK_ESCAPE:case SDLK_B:LagoonMap_Command(LAGOON_MAP_CLOSE);break;
    case SDLK_LEFT:LagoonMap_Command(LAGOON_MAP_PAN_LEFT);break;
    case SDLK_RIGHT:LagoonMap_Command(LAGOON_MAP_PAN_RIGHT);break;
    case SDLK_UP:LagoonMap_Command(LAGOON_MAP_PAN_UP);break;
    case SDLK_DOWN:LagoonMap_Command(LAGOON_MAP_PAN_DOWN);break;
    case SDLK_LEFTBRACKET:case SDLK_PAGEUP:LagoonMap_Command(LAGOON_MAP_PREVIOUS);break;
    case SDLK_RIGHTBRACKET:case SDLK_PAGEDOWN:LagoonMap_Command(LAGOON_MAP_NEXT);break;
    case SDLK_PLUS:case SDLK_EQUALS:case SDLK_A:LagoonMap_Command(LAGOON_MAP_ZOOM_IN);break;
    case SDLK_MINUS:case SDLK_X:LagoonMap_Command(LAGOON_MAP_ZOOM_OUT);break;
    case SDLK_Y:case SDLK_HOME:LagoonMap_Command(LAGOON_MAP_HOME);break;
    case SDLK_0:case SDLK_RETURN:LagoonMap_Command(LAGOON_MAP_FIT);break;
    case SDLK_TAB:LagoonMap_Command(LAGOON_MAP_NEXT_EXITS);break;
    default:break;
    }return 1;
}

static int LagoonHost_DrawFrame(uint8_t *dst,size_t pitch,const uint8_t *field,int width,int height,double alpha) {
    if(!LagoonWide_DrawFrame(dst,pitch,field,width,height,alpha))
        for(int y=0;y<height;y++)memcpy(dst+(size_t)y*pitch,field+(size_t)y*width*4,(size_t)width*4);
    LagoonBossHud_Draw(dst,pitch,field,(size_t)width*4,(unsigned)width,(unsigned)height,RtlStateGeneration());
    if(!present("LAGOON_EASTER_EGG_DISABLE"))LagoonEasterEgg_Draw(dst,pitch,(unsigned)width,(unsigned)height,SDL_GetTicks(),RtlStateGeneration());
    const uint32_t *image;int iw,ih;
    if(LagoonAchievementUI_ToastImage(&image,&iw,&ih)&&iw<=width&&ih<=height){
        int x=width-iw-4,y=4;if(x<0)x=0;
        for(int row=0;row<ih;row++)memcpy(dst+(size_t)(y+row)*pitch+(size_t)x*4,image+(size_t)row*iw,(size_t)iw*4);
    }
    return 1;
}
static void LagoonHost_EndFrame(const uint8_t *field,unsigned number) {
    LagoonWide_EndFrame(field,number);LagoonEasterEgg_EndFrame(RtlStateGeneration());
    LagoonBossHud_ObserveField(g_ppu,g_ram,RtlStateGeneration(),number,g_snesrecomp_last_hdmaen);
}
static void LagoonHost_Shutdown(void) {
    if (present("LAGOON_NATIVE_WAIT_REPORT")) {
        fprintf(stderr,"[native-wait57] pc=0081F7 mode=M1X0 actual_aot_entries=%llu\n",
            (unsigned long long)lagoon_wait_native_entries);
        fprintf(stderr,"[native-wait57] pc=0081BE mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_queue_wrapper_entries);
        fprintf(stderr,"[native-wait57] pc=00899B mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_dma_request_entries);
        fprintf(stderr,"[native-wait57] pc=008985 mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_dma_drain_entries);
        fprintf(stderr,"[native-wait57] pc=01A4D6 mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_redraw_entries);
    }
    if(present("LAGOON_NATIVE_WAIT_REPORT"))fprintf(stderr,"[native-wait57] pc=01C726 mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_scene_entries);
    if(present("LAGOON_NATIVE_WAIT_REPORT"))fprintf(stderr,"[native-wait57] pc=01C643 mode=M1X0 actual_aot_entries=%llu\n",(unsigned long long)lagoon_scene_dispatch_entries);
    if(present("LAGOON_WIDE_ENTITY_REPORT"))fprintf(stderr,"[lagoon-wide-ai] ai_reads=%llu animation_reads=%llu\n",
        LagoonWideAI_AIReads(),LagoonWideAI_AnimationReads());
    if(getenv("LAGOON_IDLE_POLL_PROBE"))
        fprintf(stderr,"[idle60] declared=%d interpreted_poll_ops=%llu\n",lagoon_idle_declared,(unsigned long long)lagoon_idle_ops);
    if(getenv("LAGOON_IDLE_POLL_PROBE"))
        fprintf(stderr,"[timing61] framewait_ops M0X0=%llu M0X1=%llu M1X0=%llu M1X1=%llu display_entry_M1X1=%llu wait_entry_M1X1=%llu\n",
            (unsigned long long)lagoon_framewait_ops[0],(unsigned long long)lagoon_framewait_ops[1],
            (unsigned long long)lagoon_framewait_ops[2],(unsigned long long)lagoon_framewait_ops[3],
            (unsigned long long)lagoon_display_entries,(unsigned long long)lagoon_framewait_entries);
    if(getenv("LAGOON_IDLE_POLL_PROBE"))fprintf(stderr,
        "[framewait62] declared=%d interpreted_poll_ops=%llu\n",lagoon_framewait_declared,
        (unsigned long long)(lagoon_framewait_ops[0]+lagoon_framewait_ops[1]+lagoon_framewait_ops[2]+lagoon_framewait_ops[3]));
    LagoonMap_Save();LagoonAchievements_Shutdown();LagoonAchievementSound_Shutdown();achievement_session=0;
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
    .before_run_frame    = LagoonHost_ConfigureIdlePoll,
    .prepare_frame       = LagoonWide_PrepareFrame,
    .begin_sim_frame     = LagoonHost_BeginFrame,
    .end_sim_frame       = LagoonHost_EndFrame,
    .draw_frame          = LagoonHost_DrawFrame,
    .on_reset            = LagoonWide_Reset,
    .after_run_frame     = LagoonHost_AfterFrame,
    .on_shutdown         = LagoonHost_Shutdown,
    .title_gamepad_button = LagoonHost_Gamepad,
    .title_overlay_key   = LagoonHost_MapKey,
    .title_overlay_poll_open = LagoonHost_PollOverlay,
    .title_overlay_tick = LagoonHost_TickOverlay,
    .title_overlay_image = LagoonHost_OverlayImage,
    .title_overlay_continue_music = LagoonEasterEgg_IsOpen,
};

#ifndef __ANDROID__
#undef main   /* desktop: keep plain main() even if SDL_main.h remapped it */
#endif
int main(int argc, char **argv)
{
    if (getenv("LAGOON_IDLE_POLL_PROBE")) {
        lagoon_previous_pc_hook = g_interp_bridge_pc_hook;
        g_interp_bridge_pc_hook = LagoonHost_IdleOpcode;
    }
    if (present("LAGOON_NATIVE_WAIT_REPORT")) {
        lagoon_previous_bounce_hook = g_interp_bridge_bounce_hook;
        g_interp_bridge_bounce_hook = LagoonHost_WaitBounce;
    }
    for (int i = 1; i + 1 < argc; ++i)
        if (!strcmp(argv[i], "--config")) {LagoonWide_SetConfigPath(argv[i + 1]);lagoon_config_path=argv[i+1];}
    return snesrecomp_desktop_main(&kGameHost, argc, argv);
}
