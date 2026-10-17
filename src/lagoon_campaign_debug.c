/* Explicit campaign CHEAT: substitute destination on an ALREADY MATCHED
 * ordinary guest door. No game assets, AI, control-flow or loader copy.
 * Intended only for bounded, isolated script qualification; absent env is inert.
 */
#include "lagoon_campaign_debug.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include "snes_runahead.h"
#include "netplay/snes_netplay.h"
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CampaignWarp { uint64_t tick; uint32_t descriptor; } CampaignWarp;
static CampaignWarp s_plan[256];
static unsigned s_count, s_next;
static uint64_t s_tick, s_guard_rejections;
static int s_initialized, s_enabled;
static const uint32_t s_trace_pc[]={0x01c8b0,0x01c8b5,0x01c8e2,
    0x01c8e5,0x01c8eb,0x018772,0x018703,0x0180a3,0x018b84};
static unsigned s_trace_count[sizeof(s_trace_pc)/sizeof(s_trace_pc[0])];

static unsigned word(const uint8_t *p) { return p[0] | (unsigned)p[1]<<8; }
static unsigned offset(uint32_t a) { return (a>>16)*0x8000u+(a&0x7fffu); }
static const uint8_t *rom_at(uint32_t a) { return g_rom+offset(a); }

/* Every accepted address is EXACTLY an existing door record's payload, not
 * merely any ROM bytes that happen to resemble a destination. */
static int valid_descriptor(uint32_t address) {
  if (!g_rom || address<0x058004 || address>0x05fff8) return 0;
  int found=0;
  for (unsigned room=0;room<75 && !found;room++) {
    uint32_t p=0x050000u|word(rom_at(0x058059u+room*2));
    if ((p&0xffffu)<0x8000u) return 0;
    for (unsigned n=0;n<256;n++,p+=11) {
      if (p>0x05fff4u) return 0;
      if (word(rom_at(p))==0xffffu) break;
      if (p+4==address) { found=1; break; }
      if (n==255) return 0;
    }
  }
  if (!found) return 0;
  const uint8_t *d=rom_at(address);
  if (d[4]>=75 || d[5]>3) return 0;
  const uint8_t *r=rom_at(0x05b2dfu+d[4]*0x38u);
  unsigned width=word(r+3)*8u,height=word(r+5)*8u;
  return width>=256 && width<=2048 && height>=224 && height<=2048 &&
      word(d)<width-16 && word(d+2)<height-14;
}
static void report(void) {
  if (s_initialized && s_count)
    fprintf(stderr,"[campaign-warp] summary applied=%u planned=%u pending=%u guard_rejections=%" PRIu64 " ticks=%" PRIu64 "\n",
        s_next,s_count,s_count-s_next,s_guard_rejections,s_tick);
}
static int initialize(void) {
  const char *path=getenv("LAGOON_CAMPAIGN_WARPS");
  if (!path || !*path) return 0;
  const char *frames=getenv("SNESRECOMP_RUN_FRAMES");
  const char *audio=getenv("SDL_AUDIODRIVER");
  char *end=NULL;
  uint64_t limit=frames?strtoull(frames,&end,10):0;
  if (!limit || !end || *end || limit>10000000 || !audio || strcmp(audio,"dummy") ||
      snes_runahead_frames() || snes_netplay_active()) {
    fprintf(stderr,"[campaign-warp] refused: bounded dummy-audio offline harness required\n");
    return 0;
  }
  /* Main JSR and complete warp prefix: refuse a different title/code revision.
   * Main executable separately verifies the original Lagoon USA SHA256. */
  if (!g_rom || memcmp(rom_at(0x0180a0),"\x20\xf6\xc7",3) ||
      memcmp(rom_at(0x01c8b0),"\xc2\x20\xa0\x00\x00\xb7\x40\x8d\x02\x05",10)) {
    fprintf(stderr,"[campaign-warp] refused: source instruction identity\n"); return 0;
  }
  FILE *f=fopen(path,"rb");
  if (!f) { fprintf(stderr,"[campaign-warp] refused: unreadable plan\n"); return 0; }
  char line[256]; unsigned line_number=0; int valid=1;
  while (fgets(line,sizeof(line),f)) {
    line_number++;
    char *p=line; while (isspace((unsigned char)*p)) p++;
    if (!*p || *p=='#') continue;
    uint64_t tick; unsigned descriptor; int used=0;
    if (s_count==256 || sscanf(p,"%" SCNu64 " %x %n",&tick,&descriptor,&used)!=2 || !used) { valid=0; break; }
    while (isspace((unsigned char)p[used])) used++;
    if ((p[used] && p[used]!='#') || tick==0 || tick>limit ||
        (s_count && tick<=s_plan[s_count-1].tick) || !valid_descriptor(descriptor)) { valid=0; break; }
    s_plan[s_count++]=(CampaignWarp){tick,descriptor};
  }
  if (ferror(f)) valid=0;
  fclose(f);
  if (!valid || !s_count) {
    fprintf(stderr,"[campaign-warp] refused: invalid plan at line%u\n",line_number);
    s_count=0; return 0;
  }
  atexit(report);
  fprintf(stderr,"[campaign-warp] enabled explicit CHEAT plan entries=%u\n",s_count);
  return 1;
}
static void at_door_check(CpuState *cpu,uint32_t pc24) {
  if (!s_enabled || s_next==s_count || s_tick<s_plan[s_next].tick) return;
  /* Bank dispatcher0080B5/C9..CD loads DB=01 for bank01 routines.
   * Absolute0500 writes mirror WRAM; B9 table reads require bank01 ROM. */
  if (!cpu || pc24!=0x01c8b5 || cpu->PB!=1 || cpu->DB!=1 || cpu->D ||
      cpu->m_flag || cpu->x_flag || cpu->emulation || cpu->S>0x1ffdu ||
      word(g_ram+cpu->S+1)!=0x80a2 || g_ram[0x100e] ||
      (g_ram[0x500]&0xc0)!=0xc0 || (g_ram[0x501]&0xf3) ||
      snes_runahead_frames() || snes_netplay_active()) {
    if (s_guard_rejections<8 && cpu) {
      unsigned ret=cpu->S<=0x1ffdu?word(g_ram+cpu->S+1):0xffffu;
      fprintf(stderr,"[campaign-warp] guard tick=%" PRIu64 " pc=%06x PB=%02x DB=%02x D=%04x M=%u X=%u E=%u P=%02x S=%04x ret=%04x player=%02x/%02x loading=%02x runahead=%d netplay=%d\n",
          s_tick,pc24,cpu->PB,cpu->DB,cpu->D,cpu->m_flag,cpu->x_flag,
          cpu->emulation,cpu->P,cpu->S,ret,g_ram[0x500],g_ram[0x501],
          g_ram[0x100e],snes_runahead_frames(),snes_netplay_active());
    }
    s_guard_rejections++; return;
  }
  /* Prove that original C87B..C8A2 has matched the CURRENT room's real
   * door row, after collision/fade normalization and room-index setup. */
  unsigned room=g_ram[0x48e];
  uint32_t original_ptr=g_ram[0x40]|(uint32_t)g_ram[0x41]<<8|(uint32_t)g_ram[0x42]<<16;
  if (room>=75 || original_ptr!=(0x050000u|word(rom_at(0x058059u+room*2))) ||
      cpu->Y<4 || (cpu->Y-4)%11 || !valid_descriptor(original_ptr+cpu->Y) ||
      word(rom_at(original_ptr+cpu->Y-4))!=word(g_ram+0x102b) ||
      word(rom_at(original_ptr+cpu->Y-2))!=word(g_ram+0x102d)) {
    s_guard_rejections++;return;
  }
  uint32_t p=s_plan[s_next].descriptor;
  if (!valid_descriptor(p)) { s_enabled=0; fprintf(stderr,"[campaign-warp] refused changed descriptor\n"); return; }
  unsigned old_room=g_ram[0x48e],new_room=rom_at(p)[4];
  g_ram[0x40]=(uint8_t)p;g_ram[0x41]=(uint8_t)(p>>8);g_ram[0x42]=(uint8_t)(p>>16);
  cpu->Y=0;
  /* Original opcode/control flow continues unchanged, reading the selected
   * real ROM payload and invoking018772 after original door prerequisites. */
  fprintf(stderr,"[campaign-warp] applied index=%u requested=%" PRIu64 " tick=%" PRIu64
      " old=%02x new=%02x descriptor=%06x sp=%04x return=0180a2 M0X0E0D0DB01 matched-door\n",
      s_next,s_plan[s_next].tick,s_tick,old_room,new_room,p,cpu->S);
  s_next++;
}
static void trace_warp(CpuState *cpu,uint32_t pc24) {
  if (!s_enabled || !s_next || !cpu) return;
  unsigned n;
  for (n=0;n<sizeof(s_trace_pc)/sizeof(s_trace_pc[0]);n++)
    if (pc24==s_trace_pc[n]) break;
  if (n==sizeof(s_trace_pc)/sizeof(s_trace_pc[0]) || s_trace_count[n]++>=4) return;
  unsigned ret=cpu->S<=0x1ffdu?word(g_ram+cpu->S+1):0xffffu;
  uint32_t ptr=g_ram[0x40]|(uint32_t)g_ram[0x41]<<8|(uint32_t)g_ram[0x42]<<16;
  fprintf(stderr,"[campaign-trace] tick=%" PRIu64 " pc=%06x A=%04x X=%04x Y=%04x S=%04x ret=%04x PB=%02x DB=%02x D=%04x P=%02x M=%u Xf=%u E=%u ptr=%06x room=%02x xy=%u,%u loading=%02x\n",
      s_tick,pc24,cpu->A,cpu->X,cpu->Y,cpu->S,ret,cpu->PB,cpu->DB,cpu->D,
      cpu->P,cpu->m_flag,cpu->x_flag,cpu->emulation,ptr,g_ram[0x48e],
      word(g_ram+0x502),word(g_ram+0x504),g_ram[0x100e]);
}
void LagoonCampaignDebug_BeforeFrame(void) {
  if (!s_initialized) { s_initialized=1;s_enabled=initialize(); }
  if (!s_enabled) return;
  ++s_tick;
  /* Idempotent and also restores the hook if a runtime reset cleared hooks. */
  if (!interp_bridge_add_pre_opcode_hook(0x01c8b5,at_door_check)) {
    s_enabled=0;fprintf(stderr,"[campaign-warp] refused: hook capacity\n");
  }
  for (unsigned n=0;n<sizeof(s_trace_pc)/sizeof(s_trace_pc[0]);n++)
    if (!interp_bridge_add_pre_opcode_hook(s_trace_pc[n],trace_warp)) {
      s_enabled=0;fprintf(stderr,"[campaign-warp] refused: trace hook capacity\n");break;
    }
}
