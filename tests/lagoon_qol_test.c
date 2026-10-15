#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lagoon_qol.c"
uint8 g_ram[0x20000];
static CpuWramWriteFilterHook filter;
static int enabled_all;
static int callbacks;
static uint64_t state_generation;
static int camera_aot_conflict;
const char *cpu_dispatch_entry_reason(uint32_t pc, uint8_t mx) {
  return camera_aot_conflict && pc==0x0183D8 && mx==2
    ? "compiled_entry_available_check_policy" : "missing_entry";
}
uint64_t RtlStateGeneration(void) { return state_generation; }
int snes_mod_runtime_feature_enabled_c(const char *package, const char *feature) {
  (void)package; (void)feature; return enabled_all;
}
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback cb) {
  (void)id; (void)cb; return 1;
}
int snes_mod_register_frame_callback(SNESModFrameCallback cb) {
  (void)cb; callbacks++; return 1;
}
void cpu_set_wram_write_filter_hook(CpuWramWriteFilterHook cb) { filter=cb; }
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
  assert(!memcmp(stock,rom,0x100000));
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
  /* Counter6 is withheld until eight frame ticks, then ordinary expiry runs. */
  g_ram[0x501]=0; put(0x501,0x80); put(0x512,0);
  for(int i=0;i<8;i++) { put(0x512,6); assert(g_ram[0x512]==5); qol_mechanics_frame(); }
  put(0x512,6); assert(g_ram[0x512]==6);
  /* Jump starts at 0580; only its original zero-velocity apex is held. */
  g_ram[0x501]=0x20; put(0x517,5); put(0x516,0x80); word(0x516,0);
  for(int i=0;i<8;i++) { word(0x516,0xFF80); assert(getword(0x516)==0); qol_mechanics_frame(); }
  word(0x516,0xFF80); assert(getword(0x516)==0xFF80);
  word(0x516,0); word(0x516,0xFF80); assert(getword(0x516)==0xFF80); /* no second hold */
  /* A reset or loaded guest snapshot invalidates stale host counters before
   * the next write. Do not apply an old hold to a restored velocity/counter. */
  g_ram[0x501]=0; put(0x501,0x80); put(0x512,6);
  assert(g_ram[0x512]==5); ++state_generation;
  put(0x512,6); assert(g_ram[0x512]==6 && !g_recovery_armed);
  g_ram[0x501]=0x20; put(0x517,5); put(0x516,0x80); word(0x516,0);
  word(0x516,0xFF80); assert(getword(0x516)==0);
  ++state_generation; word(0x516,0xFF80);
  assert(getword(0x516)==0xFF80 && !g_jump_armed);
  g_recovery_frames=7; g_recovery_holding=1; ++state_generation;
  qol_mechanics_frame(); assert(g_recovery_frames==0);
  LagoonQol_ResetTransient(); assert(!g_jump_frames && !g_recovery_frames);
  /* Camera changes only guarded centre operands. The guest owns movement,
   * streaming and large-room coordinates; activation never teleports WRAM. */
  assert(rom[0x8408]==128 && rom[0x8432]==128);
  assert(rom[0x845F]==112 && rom[0x8489]==112 && rom[0x87F6]==128);
  assert(rom[0x8815]==stock[0x8815]); /* vertical initial centre already112 */
  memset(g_ram,0xA5,sizeof(g_ram)); callbacks=0;
  qol_camera_activated();
  for (unsigned j=0;j<sizeof(g_ram);++j) assert(g_ram[j]==0xA5);
  assert(callbacks==0);
  /* A compiled camera entry refuses ALL camera operand patches. */
  memcpy(rom,stock,0x100000); camera_aot_conflict=1;
  LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!g_camera_center && rom[0x8408]==192 && rom[0x87F6]==112);
  camera_aot_conflict=0;
  /* A mismatching operand also refuses the full camera patch atomically. */
  memcpy(rom,stock,0x100000); rom[0x8432]^=1;
  LagoonQol_OnRomLoaded(rom,0x100000);
  assert(!g_camera_center && rom[0x8408]==192 && rom[0x87F6]==112);
  /* One failed guard refuses the whole associated ROM feature. */
  memcpy(rom,stock,0x100000); rom[0xFFBB]^=1;
  LagoonQol_OnRomLoaded(rom,0x100000); assert((rom[0x131DC]|rom[0x131DD]<<8)==500);
  free(rom); free(stock);
  puts("QoL checks passed: off identity, data patches, new-game guard, follower exclusion, recovery/jump timers, guard refusal");
  return 0;
}
