#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lagoon_qol.c"
uint8 g_ram[0x20000];
static CpuWramWriteFilterHook filter;
static CpuWramReadFilterHook read_filter;
int snes_frame_counter;
static int enabled_all;
static int callbacks;
static uint64_t state_generation;
static int camera_aot_conflict;
static const CpuWramReadContext *current_context;
const CpuWramReadContext *cpu_current_wram_context(void) { return current_context; }
void LagoonFastMenus_Reset(const uint8_t *rom,size_t size) { (void)rom;(void)size; }
uint8_t LagoonFastMenus_Filter(uint32_t a,uint8_t v,const CpuWramReadContext *c,const uint8_t *r) {
  (void)a;(void)c;(void)r; return v;
}
const char *cpu_dispatch_entry_reason(uint32_t pc, uint8_t mx) {
  return camera_aot_conflict && pc==(camera_aot_conflict==2 ? 0x01807A : 0x0183D8) && mx==2
    ? "compiled_entry_available_check_policy" : "missing_entry";
}
uint64_t RtlStateGeneration(void) { return state_generation; }
int snes_mod_runtime_feature_enabled_c(const char *package, const char *feature) {
  (void)package; return enabled_all==2 ? !strcmp(feature,"fast_menus") : enabled_all;
}
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback cb) {
  (void)id; (void)cb; return 1;
}
int snes_mod_register_frame_callback(SNESModFrameCallback cb) {
  (void)cb; callbacks++; return 1;
}
void cpu_set_wram_write_filter_hook(CpuWramWriteFilterHook cb) { filter=cb; }
void cpu_set_wram_read_filter_hook(CpuWramReadFilterHook cb) { read_filter=cb; }
static void put(unsigned a, uint8_t v) { g_ram[a]=filter(a,v); }
static void word(unsigned a, unsigned v) { put(a,(uint8_t)v); put(a+1,(uint8_t)(v>>8)); }
static unsigned getword(unsigned a) { return g_ram[a]|g_ram[a+1]<<8; }
int main(int argc, char **argv) {
  assert(argc==2);
  FILE *f=fopen(argv[1],"rb"); assert(f);
  uint8_t *rom=malloc(0x100000), *stock=malloc(0x100000);
  assert(fread(rom,1,0x100000,f)==0x100000); fclose(f);
  memcpy(stock,rom,0x100000);
  enabled_all=0; LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!memcmp(stock,rom,0x100000)); assert(!read_filter);
  for (unsigned i=0;i<0x20000;i++) assert(filter(i,(uint8_t)i)==(uint8_t)i);
  enabled_all=1; LagoonQol_OnRomLoaded(rom,0x100000);
  assert(rom[0xE141]==18 && rom[0xE142]==21 && rom[0xE143]==23);
  assert(rom[0xE0FA]==11 && rom[0xE0FC]==13 && rom[0xE0FE]==16);
  assert(rom[0xB6F2]==38 && rom[0xB6FE]==38 && rom[0xB700]==0xDA && rom[0xB70C]==0xDA);
  assert(!memcmp(rom+0x10149,stock+0x10149,16));
  assert((rom[0xFFBB]|rom[0xFFBC]<<8)==270);
  assert((rom[0x131DC]|rom[0x131DD]<<8)==750);
  /* Enemy projectile range is not a player magic power table. */
  assert(!memcmp(rom+0xFFCD,stock+0xFFCD,46));
  /* First enemy EXP=2 ->3; first boss64 ->96. */
  assert((rom[0x291F9+11]|rom[0x291F9+12]<<8)==3);
  assert((rom[0x29462+11]|rom[0x29462+12]<<8)==96);
  /* New-game construction; STR stays10, HP18, DEF11. */
  memset(g_ram,0,sizeof(g_ram));
  put(0x500,0xC0); put(0x52C,1); put(0x50B,0xFF);
  word(0x520,10); word(0x524,10); word(0x522,3); word(0x526,5);
  assert(getword(0x520)==18 && getword(0x524)==10 && getword(0x526)==11);
  /* A loaded level-one record's HP arrives before level; no new-game rewrite. */
  memset(g_ram,0,sizeof(g_ram)); word(0x520,10); assert(getword(0x520)==10);
  /* Faster walking affects Nasir's ordinary tier-two velocity only. */
  g_ram[0x500]=0xC0; g_ram[0x501]=0; g_ram[0x51A]=2; g_ram[0x50B]=0;
  word(0x506,3); word(0x508,0xFFFD);
  assert(getword(0x506)==5 && getword(0x508)==0xFFFB);
  word(0x546,3); assert(getword(0x546)==3); /* same-tier NPC/enemy */
  g_ram[0x501]=0x80; word(0x506,3); assert(getword(0x506)==3); /* hit knockback */
  g_ram[0x501]=0x40; word(0x506,0xFFFD); assert(getword(0x506)==0xFFFD);
  g_ram[0x501]=0x20; word(0x506,3); assert(getword(0x506)==5); /* moving jump */
  g_ram[0x501]=0; g_ram[0x51A]=1; word(0x506,3); assert(getword(0x506)==3);
  g_ram[0x51A]=2; g_ram[0x50B]=0xFF; word(0x506,3); assert(getword(0x506)==3);
  /* Only the exact following-ally signature is accelerated. */
  qol_follower_activated();
  g_ram[0x4E1]=1; g_ram[0x540]=0xC0; g_ram[0x558]=1; g_ram[0x55B]=0;
  word(0x546,1); word(0x548,0xFFFF);
  assert(getword(0x546)==3 && getword(0x548)==0xFFFD);
  g_ram[0x558]=2; word(0x546,1); assert(getword(0x546)==1);
  /* Both activation orders register exactly one timer callback. */
  callbacks=0; g_mechanics_registered=0; qol_jump_activated(); qol_recovery_activated(); assert(callbacks==1);
  /* Stock counter and entity flags expire unchanged. Only exact ordinary-hit
   * reads receive eight extra frames of immunity; velocity is never touched. */
  memset(g_ram,0,sizeof(g_ram)); g_ram[0x500]=0xC0; g_ram[0x48E]=3;
  word(0x520,10); g_ram[0x501]=0; put(0x501,0x80); put(0x512,0);
  word(0x506,0xFFFA); put(0x512,6); assert(g_ram[0x512]==6);
  snes_frame_counter=100; put(0x501,0);
  assert(g_ram[0x501]==0 && getword(0x506)==0xFFFA && g_recovery_frames==8);
  CpuWramReadContext c={0x01BA52,0x1F0,0,1,1,0,0,1};
  g_ram[0x1F1]=0xA8; g_ram[0x1F2]=0xB9;
  assert(read_filter && read_filter(0x501,0,&c)==0x80);
  for (int i=0;i<8;i++) {
    snes_frame_counter=100+i; qol_mechanics_frame();
    assert(read_filter(0x501,0,&c)==0x80 && g_ram[0x501]==0 && g_ram[0x512]==6);
  }
  snes_frame_counter=108; assert(read_filter(0x501,0,&c)==0);
  snes_frame_counter=100;
  c.interpreter=0; c.instruction_pc=0xFFFFFFFF;
  assert(read_filter(0x501,0,&c)==0x80); /* native registers + real stack */
  c.interpreter=1; c.instruction_pc=0x01BA55;
  assert(read_filter(0x501,0,&c)==0); /* stale/other interpreter cursor */
  c.instruction_pc=0x81BA52; c.pb=0x81; assert(read_filter(0x501,0,&c)==0x80);
  c.pb=1; c.instruction_pc=0x01BA52;
  const unsigned returns[]={0xB9A8,0xB9F6,0xBA28,0xBA31};
  for (unsigned i=0;i<4;i++) {
    g_ram[0x1F1]=(uint8_t)returns[i]; g_ram[0x1F2]=(uint8_t)(returns[i]>>8);
    assert(read_filter(0x501,0,&c)==0x80);
  }
  g_ram[0x1F1]=0xA8; g_ram[0x1F2]=0xB9;
  c.y=0x40; assert(read_filter(0x501,0,&c)==0); c.y=0;
  c.m=0; assert(read_filter(0x501,0,&c)==0); c.m=1;
  c.x=1; assert(read_filter(0x501,0,&c)==0); c.x=0;
  c.e=1; assert(read_filter(0x501,0,&c)==0); c.e=0;
  c.pb=0; assert(read_filter(0x501,0,&c)==0); c.pb=1;
  c.stack=0xFFFF; assert(read_filter(0x501,0,&c)==0); c.stack=0x1F0;
  g_ram[0x1F1]^=1; assert(read_filter(0x501,0,&c)==0); g_ram[0x1F1]^=1;
  assert(read_filter(0x541,0,&c)==0 && read_filter(0x501,1,&c)==1);
  rom[0xB9A6]^=1; assert(read_filter(0x501,0,&c)==0); rom[0xB9A6]^=1;
  rom[0xBA52]^=1; assert(read_filter(0x501,0,&c)==0); rom[0xBA52]^=1;
  word(0x520,0); assert(read_filter(0x501,0,&c)==0); word(0x520,10);
  snes_frame_counter=99; assert(read_filter(0x501,0,&c)==0); snes_frame_counter=100;
  put(0x48E,4); assert(read_filter(0x501,0,&c)==0);
  /* A dead player cannot arm an extra window. */
  LagoonQol_ResetTransient(); word(0x520,0); put(0x501,0x80); put(0x512,6); put(0x501,0);
  assert(!g_recovery_frames);
  /* Jump starts at 0580; only its original zero-velocity apex is held. */
  g_ram[0x501]=0x20; put(0x517,5); put(0x516,0x80); word(0x516,0);
  for(int i=0;i<8;i++) { word(0x516,0xFF80); assert(getword(0x516)==0); qol_mechanics_frame(); }
  word(0x516,0xFF80); assert(getword(0x516)==0xFF80);
  word(0x516,0); word(0x516,0xFF80); assert(getword(0x516)==0xFF80); /* no second hold */
  /* A reset or loaded guest snapshot invalidates stale host counters before
   * the next write. Do not apply an old hold to a restored velocity/counter. */
  word(0x520,10); g_ram[0x500]=0xC0; g_ram[0x501]=0;
  put(0x501,0x80); put(0x512,6); put(0x501,0); assert(g_recovery_frames==8);
  ++state_generation;
  assert(read_filter(0x501,0,&c)==0 && !g_recovery_frames);
  g_ram[0x501]=0x20; put(0x517,5); put(0x516,0x80); word(0x516,0);
  word(0x516,0xFF80); assert(getword(0x516)==0);
  ++state_generation; word(0x516,0xFF80);
  assert(getword(0x516)==0xFF80 && !g_jump_armed);
  g_recovery_frames=7; ++state_generation;
  qol_mechanics_frame(); assert(g_recovery_frames==0);
  LagoonQol_ResetTransient(); assert(!g_jump_frames && !g_recovery_frames);
  /* Camera changes only guarded centre operands. The guest owns movement,
   * streaming and large-room coordinates; activation never teleports WRAM. */
  assert(rom[0x8408]==128 && rom[0x8432]==128);
  assert(rom[0x845F]==112 && rom[0x8489]==112 && rom[0x87F6]==128);
  assert(!memcmp(rom+0x809D,"\xEA\xEA\xEA",3));
  assert(!memcmp(rom+0x80C7,stock+0x80C7,3)); /* collision stack is untouched */
  assert(!memcmp(rom+0x80CA,"\x20\xD8\x83",3));
  assert(rom[0x83D8]==0x5A && rom[0x83DB]==0x16);
  assert(!memcmp(rom+0x83F2,"\x7A\x4C\xB5\xA7",4)); /* tail animation */
  assert(rom[0x8815]==stock[0x8815]); /* vertical initial centre already112 */
  memset(g_ram,0xA5,sizeof(g_ram)); callbacks=0;
  qol_camera_activated();
  for (unsigned j=0;j<sizeof(g_ram);++j) assert(g_ram[j]==0xA5);
  assert(callbacks==0);
  /* Real write-filter marker reprojects projectile-first queue correctly,
   * including negative screen coordinates, without touching world or velocity. */
  memset(g_ram,0,sizeof(g_ram)); g_ram[0x48C]=2;
  word(0x480,512); word(0x482,200);
  word(0xD41,490); word(0xD43,220); word(0xD45,3);
  word(0x502,640); word(0x504,312); word(0x506,5);
  word(0xEC0,0); g_ram[0xEC4]=4; word(0xEC6,0); g_ram[0xECA]=0;
  CpuWramReadContext marker={0x0183EF,0x1E0,7,1,1,0,0,1};
  word(marker.stack+3,0x80CC); current_context=&marker;
  put(0x519,0); assert(getword(0xD49)==(uint16_t)-22 && getword(0xD4B)==20);
  assert(getword(0x50C)==128 && getword(0x50E)==112);
  assert(getword(0xEC2)==20 && getword(0xEC8)==112);
  assert(getword(0xD41)==490 && getword(0x502)==640 && getword(0xD45)==3 && getword(0x506)==5);
  word(0x50C,77); marker.instruction_pc++; put(0x519,0); assert(getword(0x50C)==77);
  marker.instruction_pc--; word(marker.stack+3,0x80CD); put(0x519,0); assert(getword(0x50C)==77);
  word(marker.stack+3,0x80CC); g_ram[0xEC4]=6; put(0x519,0); assert(getword(0x50C)==77);
  g_ram[0xEC4]=4; word(0xEC0,1); put(0x519,0); assert(getword(0x50C)==77);
  current_context=NULL;
  /* A compiled camera entry refuses ALL camera operand patches. */
  memcpy(rom,stock,0x100000); camera_aot_conflict=1;
  LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!g_camera_center && rom[0x8408]==192 && rom[0x87F6]==112);
  camera_aot_conflict=2; LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!g_camera_center && !memcmp(rom+0x809D,stock+0x809D,3));
  camera_aot_conflict=0;
  /* Each altered wrapper/callsite guard refuses every camera byte. */
  const unsigned camera_guards[]={0x809D,0x80C7,0x80CA,0x83D8,0x83FA};
  for(unsigned i=0;i<sizeof(camera_guards)/sizeof(camera_guards[0]);++i) {
    memcpy(rom,stock,0x100000); rom[camera_guards[i]]^=1;
    LagoonQol_OnRomLoaded(rom,0x100000);
    assert(!g_camera_center && rom[0x8408]==192 && rom[0x87F6]==112);
  }
  /* A mismatching operand also refuses the full camera patch atomically. */
  memcpy(rom,stock,0x100000); rom[0x8432]^=1;
  LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!g_camera_center && rom[0x8408]==192 && rom[0x87F6]==112);
  /* One failed guard refuses the whole associated ROM feature. */
  memcpy(rom,stock,0x100000); rom[0xFFBB]^=1;
  LagoonQol_OnRomLoaded(rom,0x100000); assert((rom[0x131DC]|rom[0x131DD]<<8)==500);
  enabled_all=0; LagoonQol_OnRomLoaded(stock,0x100000); assert(!read_filter);
  enabled_all=2; LagoonQol_OnRomLoaded(stock,0x100000);
  assert(read_filter && !g_camera_center && !g_recovery_time);
  assert(read_filter(0x501,0x17,NULL)==0x17); /* fast-menu-only context observer */
  free(rom); free(stock);
  puts("QoL checks passed: off identity, data patches, new-game guard, follower exclusion, recovery/jump timers, guard refusal");
  return 0;
}
