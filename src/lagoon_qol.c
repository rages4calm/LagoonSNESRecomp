/* lagoon_qol.c — core port improvements and opt-in gameplay features.
 *
 * Optional combat/movement features are OFF by default. Giles first aid and
 * core menu improvements are automatic. These are trusted static plugins: the mod runtime
 * resolves the enabled set before frame 1 and calls what this file registered
 * under the matching plugin id (docs/MOD_PACKAGES.md), and packages themselves
 * carry data only, so the behaviour has to live in the executable.
 *
 * Data changes and shared CPU write filters affect both execution tiers.
 * Camera centring is the one guarded instruction-operand change: its two
 * routines remain explicitly interpreter-only. Runtime AOT conflict probes
 * refuse the feature if a camera entry is compiled; generation checks also
 * enforce the corresponding CFG exclusions. The ROM's own incremental camera
 * and strip upload code then runs unchanged around the new centre thresholds.
 *
 * The const on the ROM pointer is API politeness rather than a promise: the host
 * reads the file into a heap buffer and hands that same buffer to SnesInit after
 * this hook returns, so a patch here is what the guest will read.
 *
 * A patch that cannot find the exact bytes it expects is refused and reported,
 * never applied on top of unknown data: a mod that silently rewrites something
 * else is worse than a mod that does nothing, because it looks like it worked.
 */

#include "lagoon_qol.h"
#include "lagoon_fast_menus.h"

#include "mod_runtime.h"
#include "cpu_state.h"
#include "lagoon_follower.h"
#include "common_rtl.h" /* g_ram: WRAM, $7E:xxxx -> g_ram[addr] */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* getenv: the development hook reads its target from here */
#include <string.h>

#define QOL_PACKAGE_ID "lagoon.quality"

static int g_sword_reach_applied;
int LagoonQol_SwordReachEnabled(void) { return g_sword_reach_applied; }

/* Walking speed.
 *
 * The engine does not carry a walk speed. $02:80EA computes an index from a
 * per-entity direction byte ($050B,x) and a speed-tier selector ($051A,x),
 * `direction * 4 + tier * 16`, and copies a four-byte (vx_lo, vx_hi, vy_lo,
 * vy_hi) record out of the table at $02:8129 into the entity's velocity at
 * $0506/$0508; $02:811C zeroes it. $01:A4EB then adds velocity to the position
 * at $0502/$0504, which is why a position advances only on frames the game
 * chooses to apply it.
 *
 * The table holds six tiers of +-1, 2, 3, 4, 5 and 6 pixels for right, down,
 * left, up. Nasir walks on tier 2, which is +-3; the owner's measurement of
 * "3 px/frame" is that value, and the dumps agree with it: 9 pixels per 7-frame
 * interval in which the direction was held for 6 frames. Tier 4 already holds
 * +-5, so "walking speed 5 px/frame" is this row, not a new number: the four
 * signed magnitudes 3, 3, -3, -3 become 5, 5, -5, -5 only in Nasir's
 * velocity stores. The shared tier-two ROM row stays stock: enemies and NPCs
 * also use it. The supplied BPS likewise preserves tier two and gives Nasir
 * a separate tier-seven row. This filter avoids an instruction patch while
 * preserving the same player-specific effect in both execution paths.
 *
 * Address in the file: bank $02 at LoROM offset 2 << 15, so bus $02:8149 is
 * file offset 0x10149.
 */
#define QOL_WALK_TIER2_OFFSET 0x10149u

static const uint8_t kWalkTier2Stock[16] = {
    0x03, 0x00, 0x00, 0x00, /* right: vx +3, vy 0  */
    0x00, 0x00, 0x03, 0x00, /* down:  vx 0, vy +3  */
    0xFD, 0xFF, 0x00, 0x00, /* left:  vx -3, vy 0  */
    0x00, 0x00, 0xFD, 0xFF, /* up:    vx 0, vy -3  */
};

static int g_walk_speed_applied;

/* Session flags reset at ROM load and enabled by feature activation. */
static int g_camera_center;

/* Instruction patches require interpreter-owned camera paths. Refuse if an
 * entry in either affected range has acquired an AOT variant. The generation
 * contract separately excludes enclosing bodies that cover these operands. */
static int qol_camera_has_aot_conflict(void) {
  /* The main dispatcher patches bank-selector operands at $809D/$80CA.
   * $8158 is a separate actor helper; its native entry cannot cover those
   * earlier operands. Whole-body overlap remains checked from the manifest
   * by check_camera_patch_safety.py before every generated build. */
  static const uint32_t ranges[][2] = {{0x01807A,0x0180CD},{0x0183D8,0x0184A7},{0x0187D0,0x018834}};
  for (unsigned range=0; range<sizeof(ranges)/sizeof(ranges[0]); ++range) {
    for (uint32_t pc=ranges[range][0]; pc<ranges[range][1]; ++pc) {
      for (uint8_t mx=0; mx<4; ++mx) {
        const char *reason=cpu_dispatch_entry_reason(pc,mx);
        if (reason && !strncmp(reason,"compiled_entry",14)) return 1;
      }
    }
  }
  return 0;
}


/* Declared by the package manifest, so the runtime requires it to exist. The
 * ROM patch above is the one that changes the game; this records that the
 * feature resolved, and is the place a RAM-writing feature would set its flag. */
static void qol_walk_speed_activated(void) {}

/* Starting and per-level maximum HP.
 *
 * $01:9208 clamps an entity's HP with `LDA $0520,x; SBC $E140,y; ...; LDA $E140,y;
 * STA $0520,x`, so $01:E140 is a maximum-HP table indexed by a per-entity level
 * term. Read out of the ROM it is a 36-entry byte curve --
 *   0, 10, 17, 23, 28, 36, 43, 54, 63, 68, 75, 89, ... 228, 231, 239, 248, 255
 * whose second entry is exactly the 10 the Start menu shows at level 1, and the
 * next block at $01:E164 (`0, 3, 8, 10, 12, ...`) is the same shape for MP, whose
 * second entry is the 3 the menu shows. So level 1 is entry 1, entry 0 is unused,
 * and the owner's "Nasir starts with 18 HP" is entry 1 plus eight.
 *
 * Opening rebalance sets levels 1 and 2 to 18 and 21; level 3 remains 23.
 * Later levels keep the original curve. This avoids a uniform end-game bonus.
 *
 * File offset: bank $01 at 1 << 15, so bus $01:E140 is file 0xE140 -- the same
 * mapping tools/dis.py uses, and confirmed by the curve and by both menus.
 */
#define QOL_HPTABLE_OFFSET 0xE140u
#define QOL_HPTABLE_LEN 36u
#define QOL_HP_BONUS 8u

static const uint8_t kHpTableStock[QOL_HPTABLE_LEN] = {
    0x00, 0x0A, 0x11, 0x17, 0x1C, 0x24, 0x2B, 0x36, 0x3F, 0x44,
    0x4B, 0x59, 0x60, 0x77, 0x7E, 0x8E, 0x94, 0x99, 0xA0, 0xA9,
    0xAE, 0xB4, 0xB8, 0xBB, 0xC0, 0xC9, 0xD0, 0xD2, 0xD5, 0xDB,
    0xDD, 0xE4, 0xE7, 0xEF, 0xF8, 0xFF,
};

/* Base defence, which the same routine computes one call later.
 *
 * $01:952F: `LDA $052C / ASL / TAY / LDA $0531 / ASL / TAX / LDA $E0F8,y /
 * ADC $E194,x / STA $0526`, then `$0526 += $E1A0[$0532*2]`. So defence is a
 * per-level curve at $01:E0F8 plus a per-class term and an equipment term.
 * Nasir's $0530/$0531/$0532 are all zero at the start, so his defence is
 * simply $E0F8[level], and entry 1 is 5 -- the 5 the menu shows. The STR
 * routine at $01:94FD has the identical shape with its curve at $01:E0B0,
 * whose entry 1 is the 10 the menu shows, and the EXP table at $01:E068
 * (0 10 40 90 170 280 400 560) is what $01:94DA compares against before
 * `INC $052C`. Three tables in one page each matching a menu field is what
 * makes this a measurement.
 *
 * Opening defence is 11 at level 1 and 13 at level 2; level 3 onward is stock.
 */
#define QOL_DEFTABLE_OFFSET 0xE0F8u
#define QOL_STAT_ENTRIES 36u
#define QOL_DEF_BONUS 6u

static const uint16_t kDefTableStock[QOL_STAT_ENTRIES] = {
    0x0000, 0x0005, 0x000A, 0x0010, 0x0013, 0x0017, 0x001A, 0x001F, 0x0024, 0x0028,
    0x002B, 0x0031, 0x0035, 0x0039, 0x003E, 0x0044, 0x004B, 0x004F, 0x0053, 0x005A,
    0x0065, 0x006E, 0x007C, 0x0089, 0x008E, 0x0096, 0x00A1, 0x00AC, 0x00B8, 0x00C1,
    0x00CB, 0x00D4, 0x00DF, 0x00E7, 0x00F3, 0x00FF,
};

static uint16_t qol_wram_read16(uint32_t addr);
static void qol_wram_write16(uint32_t addr, uint16_t value);
static uint8_t qol_filter_write(uint32_t addr, uint8_t value);
static uint8_t qol_filter_read(uint32_t addr, uint8_t value, const CpuWramReadContext *context);
static void qol_mechanics_frame(void);
#define QOL_WRAM_HP 0x0520u /* party block: current HP, proven by damage and by the menu */
#define QOL_WRAM_DEF 0x0526u /* party block: current defence, written as defence by $01:9541 */

static int g_start_hp_applied;
static int g_start_def_applied;
static uint64_t g_qol_state_generation;
static int g_def_written;

/* New-game initialisation is recognised by the freshly cleared party block,
 * not by a timer or by rewriting an existing save's defence. See docs/PLAYING.md. */
static void qol_start_hp_activated(void) {}

static int g_follower_speed, g_recovery_time, g_jump_hold;
static int g_mechanics_registered;
static void qol_register_mechanics(void) {
  if (!g_mechanics_registered) {
    snes_mod_register_frame_callback(qol_mechanics_frame);
    g_mechanics_registered = 1;
  }
}
static int g_recovery_armed, g_recovery_frames;
static uint32_t g_recovery_start_frame;
static uint8_t g_recovery_room;
static const uint8_t *g_recovery_rom;
static size_t g_recovery_rom_size;
static int g_jump_armed, g_jump_holding, g_jump_frames;
static void qol_follower_activated(void) { g_follower_speed = 1; }
static void qol_recovery_activated(void) {
  static const uint8_t eligibility[] = {0xB9,0x01,0x05,0x30,0x11};
  if (!g_recovery_rom || g_recovery_rom_size < 0xBA57 ||
      memcmp(g_recovery_rom + 0xBA52, eligibility, sizeof(eligibility))) {
    fprintf(stderr, "[lagoon-qol] recovery_time refused: unknown hit eligibility code\n");
    return;
  }
  g_recovery_time = 1;
  cpu_set_wram_read_filter_hook(qol_filter_read);
  qol_register_mechanics();
}
static void qol_jump_activated(void) {
  g_jump_hold = 1;
  qol_register_mechanics();
}
static void qol_data_activated(void) {}

/* Giles's original NPC-1 script (05:C491) already owns the Yes/No choice,
 * following/completed guards and the escort transition. Change only its data:
 * after Yes, jump to its existing thanks/escort command at C4AD, bypassing the
 * Healing Pot removal. Nothing is granted or consumed, and existing saves work.
 * Text 20's question ends at the original choice control 0F; text 21 shares its
 * following tail. Guard both spans and their pointers before changing anything.
 * These are interpreted data reads, not instruction operands or native bodies. */
static int qol_encode_dialogue(uint8_t *out, size_t capacity, const char *text) {
  size_t n=0;
  for (; *text; ++text) {
    uint8_t c=(uint8_t)*text;
    if(n==capacity) return -1;
    if(c==' ') c=0xFE;
    else if(c>='a' && c<='z') c=(uint8_t)(c-4);
    else if(c=='!') c=0x2F;
    else if(c=='\'') c=0x5C;
    else if(!((c>='A' && c<='Z') || c=='.' || c==',' || c=='?' ||
              c=='\n' || c=='\f' || c==0x0F)) return -1;
    /* Source uses 0D for a line break; C newline is 0A. */
    out[n++]=c=='\n' ? 0x0D : c;
  }
  return (int)n;
}

static int qol_giles_first_aid(uint8_t *rom, size_t size) {
  static const uint8_t script[]={
    0x05,0x10,0x12,0xB3,0xC4,0x05,0x05,0x0F,0x12,0xB6,0xC4,0x05,
    0x00,0x20,0x14,0x13,0xB9,0xC4,0x05,0x23,0x08,0x20,0x13,0xBA,
    0xC4,0x05,0x07,0x20,0x00,0x21,0x17,0x03,0x0F,0xFF,0x00,0x24,
    0xFF,0x00,0x23,0xFF,0x23,0x00,0x22,0xFF};
  static const char question_stock[]=
    "NASIR!  Is that you?\nI'm not seeing an\nillusion, am I?\f"
    "The demons seem to be afraid\nof the light. They didn't\n"
    "follow me up here..., but\nI got hurt along the way!\f"
    "I wish I had the Healing\nPot which can cure anything\n"
    "immediately. Do you have it,\nNASIR?\x0F";
  static const char thanks_stock[]=
    "Thank you... I'll take it!\fUgh... it's bitter!\nUgh!! Yuck! Yuck...\f"
    "It doesn't hurt so much now!\nI can't go back to the\n"
    "village alone. Take me home!\nDon't leave me here!";
  static const char decline_stock[]=
    "Please! Get the Healing Pot\nfor me! My feet hurt and I\n"
    "can't take even one step!";
  static const char question_new[]=
    "NASIR! I'm hurt.\nCould you dress my wounds\nand help me back home?\x0F";
  static const char thanks_new[]=
    "Thank you for dressing\nmy wounds!\fI can walk now.\n"
    "Please take me home.\nDon't leave me here!";
  static const char decline_new[]=
    "All right. I will wait here.\nPlease come back soon.";
  uint8_t expected[420],expected_decline[81],question[251],thanks[169],decline[81];
  int q=qol_encode_dialogue(expected,251,question_stock);
  int t=qol_encode_dialogue(expected+251,168,thanks_stock);
  int d=qol_encode_dialogue(expected_decline,80,decline_stock);
  if(q!=251 || t!=168 || d!=80) return 0;
  expected[419]=0xFF;
  expected_decline[80]=0xFF;
  int nq=qol_encode_dialogue(question,sizeof(question),question_new);
  int nt=qol_encode_dialogue(thanks,sizeof(thanks)-1,thanks_new);
  int nd=qol_encode_dialogue(decline,sizeof(decline)-1,decline_new);
  if(nq<0 || nt<0 || nd<0 || size<0x2C4BD ||
      memcmp(rom+0x2C491,script,sizeof(script)) ||
      memcmp(rom+0x206B9,"\xE4\x91\xDF\x92\x88\x93",6) ||
      memcmp(rom+0x211E4,expected,sizeof(expected)) ||
      memcmp(rom+0x21388,expected_decline,sizeof(expected_decline))) return 0;
  memset(question+nq,0xFF,sizeof(question)-(size_t)nq);
  memset(thanks+nt,0xFF,sizeof(thanks)-(size_t)nt);
  memset(decline+nd,0xFF,sizeof(decline)-(size_t)nd);
  rom[0x2C4A7]=0x11; /* original unconditional script jump, not CPU code */
  rom[0x2C4A8]=0xAD; /* original thanks + actor/following actions */
  memcpy(rom+0x211E4,question,sizeof(question));
  memcpy(rom+0x212DF,thanks,sizeof(thanks));
  memcpy(rom+0x21388,decline,sizeof(decline));
  return 1;
}

void LagoonQol_ResetTransient(void) {
  g_recovery_armed = g_recovery_frames = 0;
  g_jump_armed = g_jump_holding = g_jump_frames = 0;
  g_def_written = 0;
  g_qol_state_generation = RtlStateGeneration();
}

static void qol_check_timeline(void) {
  if (g_qol_state_generation != RtlStateGeneration())
    LagoonQol_ResetTransient();
}


/* Enemy/boss record+9 gold and+11 EXP are copied by018D25 to0528/052A.
 * Original01B7D9/B7E9 still awards them. Scale only their source data: shops,
 * purchases, inventory, quest rewards and existing save totals are untouched. */
static int qol_reward_ratio(const char *kind,unsigned *numerator,unsigned *denominator) {
  char id[24];unsigned selected=0,n=1,d=1;
  if(!strcmp(kind,"exp")&&snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID,"exp_boost")) {selected++;n=3;d=2;}
  for(unsigned value=2;value<=8;value++) {
    snprintf(id,sizeof id,"%s_%ux",kind,value);
    if(snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID,id)) {selected++;n=value;d=1;}
  }
  if(selected>1) {fprintf(stderr,"lagoon.qol: %s reward multiplier refused, conflicting selections\n",kind);return 0;}
  *numerator=n;*denominator=d;return selected==1;
}
static int qol_patch_rewards(uint8_t *rom,size_t size,int gold,unsigned numerator,unsigned denominator) {
  static const struct {uint32_t xp;uint16_t experience,gold;} table[]={
    {0x29204,2,3},
    {0x29213,1,2},
    {0x29222,3,4},
    {0x29231,4,3},
    {0x29240,5,3},
    {0x2924F,6,5},
    {0x2925E,5,5},
    {0x2926D,4,5},
    {0x2927C,6,3},
    {0x2928B,6,5},
    {0x2929A,7,5},
    {0x292A9,10,10},
    {0x292B8,9,5},
    {0x292C7,32,10},
    {0x292D6,20,10},
    {0x292E5,30,6},
    {0x292F4,22,5},
    {0x29303,30,7},
    {0x29312,35,1},
    {0x29321,40,7},
    {0x29330,52,8},
    {0x2933F,60,8},
    {0x2934E,80,9},
    {0x2935D,80,10},
    {0x2936C,90,5},
    {0x2937B,200,7},
    {0x2938A,120,2},
    {0x29399,160,7},
    {0x293A8,250,8},
    {0x293B7,300,7},
    {0x293C6,350,5},
    {0x293D5,130,7},
    {0x293E4,140,5},
    {0x293F3,150,7},
    {0x29402,350,8},
    {0x29411,400,10},
    {0x29420,0,0},
    {0x2942F,0,0},
    {0x2943E,0,0},
    {0x2946D,64,100},
    {0x2947C,250,500},
    {0x2948B,500,2800},
    {0x2949A,800,300},
    {0x294A9,0,0},
    {0x294B8,0,0},
    {0x294C7,1500,1000},
    {0x294D6,3000,1000},
    {0x294E5,0,0},
    {0x294F4,0,0},
    {0x29503,0,0},
    {0x29512,0,0},
    {0x29521,0,0},
    {0x29530,0,0},
    {0x2953F,2000,1000},
    {0x2954E,0,0},
  };
  if(!rom||size<0x30000||!denominator||numerator>8||denominator>2)return 0;
  for(unsigned i=0;i<sizeof table/sizeof *table;i++) {
    unsigned o=table[i].xp-(gold?2:0),stock=gold?table[i].gold:table[i].experience;
    if((unsigned)(rom[o]|rom[o+1]<<8)!=stock||stock*numerator/denominator>65535)return 0;
  }
  for(unsigned i=0;i<sizeof table/sizeof *table;i++) {
    unsigned o=table[i].xp-(gold?2:0),stock=gold?table[i].gold:table[i].experience;
    unsigned value=stock*numerator/denominator;rom[o]=(uint8_t)value;rom[o+1]=(uint8_t)(value>>8);
  }
  return 1;
}

void LagoonQol_OnRomLoaded(const uint8_t *rom, size_t size) {
  LagoonFastMenus_Reset(rom,size);
  g_walk_speed_applied = 0;
  g_sword_reach_applied = 0;
  g_start_hp_applied = 0;
  g_start_def_applied = 0;
  g_camera_center = 0;
  g_follower_speed = g_recovery_time = g_jump_hold = 0;
  g_mechanics_registered = 0;
  g_recovery_armed = g_recovery_frames = 0;
  g_jump_armed = g_jump_holding = g_jump_frames = 0;
  LagoonQol_ResetTransient();
  cpu_set_wram_write_filter_hook(qol_filter_write);
  cpu_set_wram_read_filter_hook(NULL);
  g_recovery_rom = rom;
  g_recovery_rom_size = size;
  /* Per-session feature state and transient timers have been cleared above. */
  if (!rom) {
    return;
  }
  /* Core menu timing needs scoped interpreter registers and its guarded
   * timer read view even when optional gameplay features are off. */
  cpu_set_wram_read_filter_hook(qol_filter_read);
  if(qol_giles_first_aid((uint8_t *)(uintptr_t)rom,size))
    fprintf(stderr,"[lagoon-qol] Giles first aid: wound dressing, original Yes/No and escort, no potion consumed\n");
  else
    fprintf(stderr,"[lagoon-qol] Giles first aid refused: original quest/text data differs\n");
  /* Each optional patch is guarded independently. Core first aid above is a
   * port improvement even when optional combat/movement features are off. */
  if (size >= QOL_WALK_TIER2_OFFSET + sizeof(kWalkTier2Stock) &&
      snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "walk_speed")) {
    uint8_t *row = (uint8_t *)(uintptr_t)(rom + QOL_WALK_TIER2_OFFSET);
    if (memcmp(row, kWalkTier2Stock, sizeof(kWalkTier2Stock)) != 0) {
      fprintf(stderr,
              "lagoon.qol: walk_speed refused, $02:8149 does not hold the proven "
              "stock row\n");
    } else {
      g_walk_speed_applied = 1;
      fprintf(stderr, "lagoon.qol: walk_speed active, Nasir walks +-5 px; shared tier 2 remains stock\n");
    }
  }
  if (snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "camera_center") && size >= 0x87F8) {
    static const struct { uint32_t offset; uint8_t opcode, stock, center; } changes[] = {
      {0x8407,0xE9,192,128}, /* right screen-space threshold */
      {0x8431,0xA9, 64,128}, /* left screen-space threshold */
      {0x845E,0xE9,176,112}, /* down screen-space threshold */
      {0x8488,0xA9, 80,112}, /* up screen-space threshold */
      {0x87F5,0xE9,112,128}, /* initial camera before complete map/VRAM load */
    };
    /* Reuse the old 35-byte direction dispatcher. Run camera after player
     * movement AND entity collision, then tail-dispatch the original animation
     * with its original caller frame. In particular A0E1 retains its stack:
     * damage paths there can unwind callers nonlocally. X/Y survive strip DMA.
     * Each axis uses actual resolved position, including stopped/blocked frames. */
    static const uint8_t dispatcher_stock[35]={
      0x64,0x40,0x64,0x41,0xAD,0x19,0x05,0x30,0x19,0x5A,0x0A,0xA8,
      0xB9,0xF2,0x83,0x85,0x26,0xB9,0xF3,0x83,0x85,0x27,0x7A,
      0x6C,0x26,0x00,0xFB,0x83,0x52,0x84,0x26,0x84,0x7D,0x84,0x60};
    static const uint8_t dispatcher_center[35]={
      0x5A,0x9B,0xD0,0x16,0xDA,0x64,0x41,
      0x20,0xFB,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7D,0x84,
      0xFA,0xAD,0x19,0x05,0x8D,0x19,0x05,
      0x7A,0x4C,0xB5,0xA7,0xEA,0xEA,0xEA,0xEA,0xEA};
    static const uint8_t early_call[]={0x20,0xD8,0x83};
    static const uint8_t collision_and_animation[]={0x20,0xE1,0xA0,0x20,0xB5,0xA7};
    int matches=!memcmp(rom+0x83D8,dispatcher_stock,sizeof(dispatcher_stock)) &&
        !memcmp(rom+0x809D,early_call,sizeof(early_call)) &&
        !memcmp(rom+0x80C7,collision_and_animation,sizeof(collision_and_animation));
    for (unsigned j=0; j<sizeof(changes)/sizeof(changes[0]); ++j) {
      size_t o=changes[j].offset;
      if (rom[o]!=changes[j].opcode || rom[o+1]!=changes[j].stock || rom[o+2]!=0)
        matches=0;
    }
    if (!matches) {
      fprintf(stderr,"lagoon.qol: camera_center refused, camera instruction guards differ\n");
    } else if (qol_camera_has_aot_conflict()) {
      fprintf(stderr,"lagoon.qol: camera_center refused, affected camera code has an AOT entry\n");
    } else {
      uint8_t *r=(uint8_t *)(uintptr_t)rom;
      for (unsigned j=0; j<sizeof(changes)/sizeof(changes[0]); ++j)
        r[changes[j].offset+1]=changes[j].center;
      memset(r+0x809D,0xEA,3);
      r[0x80CB]=0xD8; r[0x80CC]=0x83;
      memcpy(r+0x83D8,dispatcher_center,sizeof(dispatcher_center));
      g_camera_center=1;
      cpu_set_wram_read_filter_hook(qol_filter_read);
      fprintf(stderr,"lagoon.qol: camera_center applied, native strip streaming retained\n");
    }
  }
  if (size >= QOL_HPTABLE_OFFSET + sizeof(kHpTableStock) &&
      snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "start_hp")) {
    uint8_t *table = (uint8_t *)(uintptr_t)(rom + QOL_HPTABLE_OFFSET);
    if (memcmp(table, kHpTableStock, sizeof(kHpTableStock)) != 0) {
      fprintf(stderr,
              "lagoon.qol: start_hp refused, $01:E140 does not hold the proven "
              "stock level curve\n");
    } else {
      /* Opening-only rebalance: level 3 remains the stock 23 HP. */
      table[1] = 18;
      table[2] = 21;
      g_start_hp_applied = 1;
      fprintf(stderr,
              "lagoon.qol: start_hp applied, level curve is now %u at level 1 and "
              "%u at level 2 (was %u and %u)\n",
              (unsigned)table[1], (unsigned)table[2],
              (unsigned)kHpTableStock[1], (unsigned)kHpTableStock[2]);
    }
    /* Defence is the same feature the owner described in one breath with HP, so
     * it rides the same switch, but it is guarded separately: a ROM whose defence
     * page differs still gets the HP half rather than losing both. */
    if (size >= QOL_DEFTABLE_OFFSET + sizeof(kDefTableStock)) {
      uint8_t *dtable = (uint8_t *)(uintptr_t)(rom + QOL_DEFTABLE_OFFSET);
      int stock_matches = 1;
      for (unsigned i = 0; i < QOL_STAT_ENTRIES; i++) {
        uint16_t v = (uint16_t)(dtable[i * 2] | (dtable[i * 2 + 1] << 8));
        if (v != kDefTableStock[i]) {
          stock_matches = 0;
          break;
        }
      }
      if (!stock_matches) {
        fprintf(stderr,
                "lagoon.qol: start_hp refused the defence curve, $01:E0F8 does not "
                "hold the proven stock values\n");
      } else {
        g_start_def_applied = 1;
        dtable[2] = 11;
        dtable[3] = 0;
        dtable[4] = 13;
        dtable[5] = 0;
        fprintf(stderr,
                "lagoon.qol: start_hp applied, defence is now %u at level 1 and "
                "%u at level 2 (was %u and %u)\n",
                (unsigned)(dtable[2] | (dtable[3] << 8)),
                (unsigned)(dtable[4] | (dtable[5] << 8)),
                (unsigned)kDefTableStock[1], (unsigned)kDefTableStock[2]);
      }
    }
  }
  if (snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "sword_reach") && size >= 0xB710) {
    static const uint8_t stock[32] = {
      0,0,28,0,248,255,12,0,232,255,8,0,0,0,17,0,
      228,255,0,0,244,255,8,0,248,255,24,0,239,255,0,0};
    /* Original four [xmin,xmax,ymin,ymax] rectangles. Keep 38px reach;
     * add only three pixels on each perpendicular edge for lateral cleave.
     * Both the interpreter and native sword code read this shared ROM data. */
    static const uint8_t cleave[32] = {
      0,0,38,0,245,255,15,0,229,255,11,0,0,0,38,0,
      218,255,0,0,241,255,11,0,245,255,27,0,218,255,0,0};
    if (memcmp(rom + 0xB6F0, stock, sizeof(stock)) == 0) {
      uint8_t *r = (uint8_t *)(uintptr_t)rom;
      memcpy(r+0xB6F0,cleave,sizeof(cleave));
      g_sword_reach_applied = 1;
      fprintf(stderr, "lagoon.qol: sword_reach applied, 38px reach and 3px lateral cleave per side\n");
    } else fprintf(stderr, "lagoon.qol: sword_reach refused, stock rectangle differs\n");
  }
  if (snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "magic_power") && size >= 0x131DE) {
    /* Player projectile IDs 1..8; IDs 11 onward are enemy attacks, unchanged.
     * All sixteen selected spell slots are included; zero/no-attack stays zero. */
    static const uint16_t projectile[8] = {70,180,70,156,190,190,200,200};
    static const uint16_t spells[16] = {0,0,240,350,0,200,300,380,0,0,320,400,0,0,350,500};
    int matches = 1;
    for (unsigned j=0;j<8;j++) {
      size_t o=0xFFB9+j*2;
      if ((uint16_t)(rom[o]|rom[o+1]<<8) != projectile[j]) matches=0;
    }
    for (unsigned j=0;j<16;j++) {
      size_t o=0x131BE + j*2;
      if ((uint16_t)(rom[o]|rom[o+1]<<8) != spells[j]) matches=0;
    }
    if (matches) {
      uint8_t *r=(uint8_t *)(uintptr_t)rom;
      for (unsigned j=0;j<24;j++) {
        size_t o=j<8?0xFFB9+j*2:0x131BE + (j-8)*2;
        unsigned old=j<8?projectile[j]:spells[j-8];
        unsigned v=old+old/2;
        r[o]=(uint8_t)v; r[o+1]=(uint8_t)(v>>8);
      }
      fprintf(stderr, "lagoon.qol: magic_power applied, all nonzero player powers +50%%\n");
    } else fprintf(stderr, "lagoon.qol: magic_power refused, stock tables differ\n");
  }

  for(unsigned gold=0;gold<2;gold++) {
    unsigned n,d;const char *kind=gold?"gold":"exp";
    if(qol_reward_ratio(kind,&n,&d)) {
      if(qol_patch_rewards((uint8_t *)(uintptr_t)rom,size,(int)gold,n,d))
        fprintf(stderr,"lagoon.qol: %s rewards applied, %u/%u of original enemy/boss rewards\n",kind,n,d);
      else fprintf(stderr,"lagoon.qol: %s rewards refused, stock reward words differ\n",kind);
    }
  }

}

/* $01:A5AA and $01:BCE2 project entity/projectile world positions to screen;
 * $01:A624/$01:BD2F append their screen ground-Y to six-byte sort records.
 * The camera now runs after collision, so refresh ALREADY queued projections
 * at the explicit no-op store marker before animation. Do not rerun projectile
 * movement/culling, AI, damage, queue insertion, or hardware arithmetic.
 * Validate the entire bounded queue before writing any presentation field. */
static void qol_camera_project_queue(const CpuWramReadContext *c) {
  if (!g_camera_center || !c || !c->interpreter || !c->m || c->x || c->e ||
      (c->pb&0x7F)!=1 || (c->instruction_pc&0x7FFFFF)!=0x0183EF ||
      c->stack>0x1FFB || qol_wram_read16(c->stack+3)!=0x80CC) return;
  unsigned count=g_ram[0x48C];
  if(count>52) return;
  for(unsigned i=0;i<count;++i) {
    unsigned q=0xEC0+i*6, index=qol_wram_read16(q), kind=g_ram[q+4];
    if(kind>5 || (kind==4 ? (index>=0x180 || index%0x18) :
                             (index>=0x840 || index%0x40))) return;
  }
  unsigned cx=qol_wram_read16(0x480),cy=qol_wram_read16(0x482);
  for(unsigned i=0;i<count;++i) {
    unsigned q=0xEC0+i*6,index=qol_wram_read16(q);
    unsigned world=g_ram[q+4]==4 ? 0xD41+index : 0x502+index;
    unsigned screen=g_ram[q+4]==4 ? 0xD49+index : 0x50C+index;
    uint16_t x=(uint16_t)(qol_wram_read16(world)-cx);
    uint16_t y=(uint16_t)(qol_wram_read16(world+2)-cy);
    qol_wram_write16(screen,x); qol_wram_write16(screen+2,y);
    qol_wram_write16(q+2,y);
  }
}

/* Called before canonical WRAM writes by both execution paths. No PC/register
 * assumptions: older generated routines do not keep an architectural PC live. */
static uint8_t qol_filter_write(uint32_t addr, uint8_t value) {
  if(addr==0x23C) value=LagoonFastMenus_Filter(addr,value,cpu_current_wram_context(),g_ram);
  if(addr==0x519 && value==g_ram[addr])
    qol_camera_project_queue(cpu_current_wram_context());
  if (g_recovery_time || g_jump_hold || g_start_hp_applied) qol_check_timeline();
  if (g_start_hp_applied && addr == 0x520 && value == 10 &&
      qol_wram_read16(0x520) == 0 && g_ram[0x500] == 0xC0 &&
      g_ram[0x52C] == 1 && g_ram[0x50B] == 0xFF &&
      qol_wram_read16(0x522) == 0 && qol_wram_read16(0x524) == 0 &&
      qol_wram_read16(0x526) == 0) {
    g_def_written = 1; /* one matching new-game sequence, awaiting DEF */
    return 18;
  }
  if (g_start_def_applied && g_def_written == 1 && addr == 0x526 && value == 5 &&
      qol_wram_read16(0x520) == 18 && qol_wram_read16(0x524) == 10 &&
      qol_wram_read16(0x522) == 3) {
    g_def_written = 2;
    return 11;
  }
  if (g_walk_speed_applied && (addr == 0x506 || addr == 0x508) &&
      (g_ram[0x500] & 0xC0) == 0xC0 && !(g_ram[0x501] & 0xC0) &&
      g_ram[0x51A] == 2 && g_ram[0x50B] <= 3) {
    /* Nasir only. Do not change the shared ROM tier used by enemies/allies,
     * or the separately selected hurt/knockback velocity. Airborne horizontal
     * walking remains eligible so the peak hold can extend moving jumps. */
    if (value == 3) return 5;
    if (value == 0xFD) return 0xFB;
  }
  if (g_follower_speed && !LagoonFollower_OwnsVelocity(addr) && addr >= 0x546 && addr < 0xD40 &&
      ((addr - 0x500) % 0x40 == 6 || (addr - 0x500) % 0x40 == 8)) {
    uint32_t base = 0x500 + ((addr - 0x500) / 0x40) * 0x40;
    uint8_t mode = g_ram[base + 0x1B];
    if (g_ram[0x4E1] && (g_ram[base] & 0xC0) == 0xC0 &&
        g_ram[base + 0x18] == 1 && (mode == 0 || mode == 1 || mode == 3) &&
        g_ram[base + 0x1A] == 0 && !(g_ram[base + 0x1F] & 4)) {
      /* Authored target walking (+1F bit04), including Thor's Wish lead,
       * keeps its original 1px steps. At 3px it can miss the narrow approach
       * and oscillate outside the original <16px arrival test. Ordinary
       * following remains accelerated. High bytes stay FF/00. */
      if (value == 1) return 3;
      if (value == 0xFF) return 0xFD;
    }
  }
  if (g_recovery_time) {
    if (addr == 0x48E) g_recovery_armed = g_recovery_frames = 0;
    if (addr == 0x501 && (value & 0xC1) == 0x80 && !(g_ram[addr] & 0x80) &&
        (g_ram[0x500] & 0xC0) == 0xC0 && g_ram[0x518] == 0 &&
        qol_wram_read16(0x520)) {
      g_recovery_armed = 1;
      g_recovery_frames = 0;
      g_recovery_room = g_ram[0x48E];
    }
    if (addr == 0x501 && (g_ram[addr] & 0xC1) == 0x80 &&
        !(value & 0xC1) && g_ram[0x512] == 6 && g_recovery_armed &&
        qol_wram_read16(0x520) && g_ram[0x48E] == g_recovery_room) {
      g_recovery_armed = 0;
      g_recovery_frames = 8;
      g_recovery_start_frame = (uint32_t)snes_frame_counter;
    }
  }
  if (g_jump_hold && addr == 0x517 && (g_ram[0x501] & 0x20)) {
    if (value == 5) {
      g_jump_armed = 1;
      g_jump_holding = 0;
      g_jump_frames = 8;
    } else if (value == 0xFF && g_ram[0x517] == 0 &&
               g_ram[0x516] == 0x80 && g_jump_armed) {
      if (g_jump_frames) {
        g_jump_holding = 1;
        g_ram[0x516] = 0; /* low byte was stored before this high-byte hook */
        return 0;
      }
      g_jump_armed = 0;
    }
  }
  return value;
}

/* The extra window is a view of one ordinary-hit eligibility read, not an
 * entity flag: stock knockback, control, rendering and counter expiry remain
 * unchanged. The caller's live JSR return frame identifies the helper even in
 * generated code, whose architectural PC is not maintained. Interpreter reads
 * must additionally identify the exact pre-instruction cursor. */
static uint8_t qol_filter_read(uint32_t addr, uint8_t value, const CpuWramReadContext *c) {
  if(addr==0x23C)return LagoonFastMenus_FilterRead(addr,value,c,g_ram);
  value=LagoonFollower_FilterRead(addr,value,c,g_ram);
  if (!g_recovery_time || addr != 0x501 || !c) return value;
  qol_check_timeline();
  uint32_t elapsed = (uint32_t)snes_frame_counter - g_recovery_start_frame;
  if (!g_recovery_frames || elapsed >= 8 || g_ram[0x48E] != g_recovery_room ||
      !qol_wram_read16(0x520) || (g_ram[0x500] & 0xC0) != 0xC0 ||
      (value & 0xC1) || g_ram[0x518] != 0) return value;
  if ((c->pb & 0x7F) != 1 || c->y || !c->m || c->x || c->e || c->stack > 0x1FFD)
    return value;
  if (c->interpreter && (c->instruction_pc & 0x7FFFFF) != 0x01BA52) return value;
  uint16_t ret = (uint16_t)(g_ram[c->stack + 1] | ((uint16_t)g_ram[c->stack + 2] << 8));
  if (ret != 0xB9A8 && ret != 0xB9F6 && ret != 0xBA28 && ret != 0xBA31) return value;
  uint32_t off = (uint32_t)ret - 2;
  if (!g_recovery_rom || off + 2 >= g_recovery_rom_size ||
      g_recovery_rom[off] != 0x20 || g_recovery_rom[off + 1] != 0x52 ||
      g_recovery_rom[off + 2] != 0xBA ||
      memcmp(g_recovery_rom + 0xBA52, "\xB9\x01\x05\x30\x11", 5)) return value;
  return value | 0x80;
}

static void qol_mechanics_frame(void) {
  qol_check_timeline();
  if (g_recovery_frames) {
    uint32_t elapsed = (uint32_t)snes_frame_counter - g_recovery_start_frame;
    g_recovery_frames = elapsed < 8 ? 8 - (int)elapsed : 0;
  }
  if (g_jump_holding && g_jump_frames) --g_jump_frames;
  if (!(g_ram[0x501] & 0x20)) {
    g_jump_armed = g_jump_holding = g_jump_frames = 0;
  }
  if (!qol_wram_read16(0x520) || (g_ram[0x501] & 0x41) ||
      (g_ram[0x500] & 0xC0) != 0xC0 || g_ram[0x48E] != g_recovery_room)
    g_recovery_armed = g_recovery_frames = 0;
}

/* The original camera now owns its WRAM position, room limits, BG2 parallax
 * and eight-pixel tile-strip uploads. Never move its origin from a frame
 * callback: that bypasses the loader and exposes blank/stale ring-buffer tiles.
 * The guarded ROM operands above remove the dead zone and centre initial map
 * loading. Activation does not register a WRAM-writing camera callback. */
static uint16_t qol_wram_read16(uint32_t addr) {
  return (uint16_t)(g_ram[addr] | ((uint16_t)g_ram[addr + 1] << 8));
}
static void qol_wram_write16(uint32_t addr, uint16_t value) {
  g_ram[addr] = (uint8_t)(value & 0xFF);
  g_ram[addr + 1] = (uint8_t)(value >> 8);
}
static void qol_camera_activated(void) {}

/* Development hook: put Nasir on a chosen tile.
 *
 * This exists because route work is a positioning problem. Standing one tile off
 * from a door or an NPC changes the outcome completely -- a warp fires or it does
 * not, a dialogue opens or the button does nothing -- and walking him there with
 * scripted presses costs about 45 s a try and lands on whatever the collision
 * grid allows, not on the square being asked about. One write of $0502/$0504 at a
 * chosen frame makes the question "what happens on tile (352,60)?" a single run.
 *
 * It is a harness hook, not a feature: off by default, and the position comes
 * from the environment (LAGOON_DEBUG_AT="x,y[,frame]") rather than a player-facing
 * setting, so nothing about the shipped game depends on it. A player who has
 * never set that variable gets the faithful game even with the feature switched
 * on, which is the safe direction for a mistake.
 *
 * Velocity at $0506/$0508 is zeroed with the position, otherwise the momentum
 * from the walk that got him there carries on applying and the measured position
 * is a frame or two away from the one asked for.
 */
#define QOL_WRAM_ENTITY_X 0x0502u
#define QOL_WRAM_ENTITY_Y 0x0504u
#define QOL_WRAM_ENTITY_VX 0x0506u /* the velocity $02:80EA fills from the tier table */
#define QOL_WRAM_ENTITY_VY 0x0508u
#define QOL_DEBUG_ENV "LAGOON_DEBUG_AT"
#define QOL_DEBUG_MAX 8 /* a route is a handful of moves, not a stream */

typedef struct {
  uint16_t x, y;
  long frame; /* run frame to fire on, counted from plugin activation */
  int fired;
} qol_debug_move_t;

static qol_debug_move_t g_debug_moves[QOL_DEBUG_MAX];
static int g_debug_move_count;
static long g_debug_tick; /* frames since activation, so the hook is self-timed */

static void qol_debug_frame(void) {
  g_debug_tick++;
  for (int i = 0; i < g_debug_move_count; i++) {
    qol_debug_move_t *m = &g_debug_moves[i];
    if (m->fired || g_debug_tick < m->frame) {
      continue;
    }
    m->fired = 1;
    qol_wram_write16(QOL_WRAM_ENTITY_X, m->x);
    qol_wram_write16(QOL_WRAM_ENTITY_Y, m->y);
    qol_wram_write16(QOL_WRAM_ENTITY_VX, 0);
    qol_wram_write16(QOL_WRAM_ENTITY_VY, 0);
    fprintf(stderr, "lagoon.qol: debug_position frame %ld moved Nasir to (%u,%u)\n",
            g_debug_tick, (unsigned)m->x, (unsigned)m->y);
  }
}

/* Parse "x,y[,frame]" entries separated by ';'. More than one is needed because
 * a route is not one jump: getting to the fight means landing in the town band
 * first, crossing the gate, and only then standing next to the enemy. The frame
 * a move fires on defaults to 1 so a single-position spec still behaves. */
static void qol_debug_position_activated(void) {
  const char *spec = getenv(QOL_DEBUG_ENV);
  if (!spec || !*spec) {
    fprintf(stderr, "lagoon.qol: debug_position enabled but %s is unset, nothing moved\n",
            QOL_DEBUG_ENV);
    return;
  }
  g_debug_move_count = 0;
  g_debug_tick = 0;
  const char *p = spec;
  while (p && *p && g_debug_move_count < QOL_DEBUG_MAX) {
    long x = -1, y = -1, frame = 1;
    int used = 0;
    while (*p == ';') {
      p++; /* the separator is not part of the next entry, and %ld will not skip it */
    }
    if (sscanf(p, " %ld,%ld,%ld%n", &x, &y, &frame, &used) < 2) {
      used = 0;
      x = y = -1;
      frame = 1;
      if (sscanf(p, " %ld,%ld%n", &x, &y, &used) < 2) {
        fprintf(stderr,
                "lagoon.qol: debug_position refused, %s=\"%s\" is not "
                "\"x,y[,frame][;...]\"\n",
                QOL_DEBUG_ENV, spec);
        return;
      }
    }
    if (x < 0 || x > 65535 || y < 0 || y > 65535 || frame < 1) {
      fprintf(stderr, "lagoon.qol: debug_position refused, bad entry in \"%s\"\n", spec);
      return;
    }
    g_debug_moves[g_debug_move_count].x = (uint16_t)x;
    g_debug_moves[g_debug_move_count].y = (uint16_t)y;
    g_debug_moves[g_debug_move_count].frame = frame;
    g_debug_moves[g_debug_move_count].fired = 0;
    g_debug_move_count++;
    p = strchr(p, ';');
  }
  snes_mod_register_frame_callback(qol_debug_frame);
  fprintf(stderr, "lagoon.qol: debug_position armed with %d move(s) from %s\n",
          g_debug_move_count, QOL_DEBUG_ENV);
}

SNES_MOD_CONSTRUCTOR(lagoon_qol_register) {
  /* Only the plugin identities are declared at load time, since the activation
   * plugin table is not cleared. Anything per-frame belongs in the activation
   * callback above. */
  snes_mod_register_activation_plugin("lagoon.qol.walk_speed",
                                      qol_walk_speed_activated);
  snes_mod_register_activation_plugin("lagoon.qol.start_hp",
                                      qol_start_hp_activated);
  snes_mod_register_activation_plugin("lagoon.qol.camera_center",
                                      qol_camera_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_boost", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_2x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_3x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_4x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_5x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_6x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_7x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.exp_8x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_2x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_3x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_4x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_5x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_6x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_7x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.gold_8x", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.sword_reach", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.magic_power", qol_data_activated);
  snes_mod_register_activation_plugin("lagoon.qol.follower_speed", qol_follower_activated);
  snes_mod_register_activation_plugin("lagoon.qol.recovery_time", qol_recovery_activated);
  snes_mod_register_activation_plugin("lagoon.qol.jump_hold", qol_jump_activated);
  snes_mod_register_activation_plugin("lagoon.qol.debug_position",
                                      qol_debug_position_activated);
}
