/* Synthetic host integration test. Uses the actual MSU stream implementation;
 * fixtures contain generated tones only. No ROM or soundtrack is required. */
#include "cpu_state.h"
#include "common_rtl.h"
#include "mod_runtime.h"
#include "snes/msu1.h"
#include "lagoon_soundtrack.h"
#include "snes/saveload.h"
#include <assert.h>

uint8 g_ram[0x20000];
const uint8 *g_rom;
CpuState g_cpu;
uint32_t g_interp816_cur_pc;
static SNESModActivationCallback activate;
static SNESModApuWriteCallback filter;
static SNESModActivationCallback reset_session_cb;
static SNESModFrameCallback frame_cb;
static unsigned writes;
static uint16 last_reg;
static uint8 last_value;
static int runahead, netplay;
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}
double RtlAudioOutputRate(void) { return 44100; }
void RtlApuWrite(uint16 reg, uint8 value) {
  ++writes; last_reg = reg; last_value = value;
}
int snes_mod_register_activation_plugin(const char *id, SNESModActivationCallback cb) {
  if (!strcmp(id, "lagoon.soundtrack.x68000")) activate = cb;
  return 1;
}
int snes_mod_register_reset_callback(SNESModActivationCallback cb) { reset_session_cb = cb; return 1; }
int snes_mod_register_frame_callback(SNESModFrameCallback cb) { frame_cb = cb; return 1; }
int snes_mod_register_apu_write_callback(SNESModApuWriteCallback cb) { filter = cb; return 1; }
int snes_mod_runtime_resource_path_c(const char *package, const char *feature,
                                    const char *id, char *out, uint32_t cap) {
  (void)package; (void)feature; (void)id; (void)cap; out[0] = 0; return 0;
}
int snes_runahead_active(void) { return runahead; }
int snes_netplay_active(void) { return netplay; }
typedef struct TestStream { SaveLoadInfo base; uint8_t bytes[16]; size_t size, pos; int load, error; } TestStream;
static void stream_func(SaveLoadInfo *sli, void *data, size_t size) {
  TestStream *s = (TestStream *)sli;
  if (size > sizeof(s->bytes) - s->pos || (s->load && size > s->size - s->pos)) { s->error = 1; return; }
  if (s->load) memcpy(data, s->bytes + s->pos, size);
  else { memcpy(s->bytes + s->pos, data, size); s->size = s->pos + size; }
  s->pos += size;
}
size_t RtlStateBytesRemaining(SaveLoadInfo *sli) { TestStream *s = (TestStream *)sli; return s->size - s->pos; }
void RtlStateStreamFail(SaveLoadInfo *sli) { ((TestStream *)sli)->error = 1; }
size_t RtlSnapshotGuestSize(uint32 *version) { *version = 10; return 32; }
static void load_chunk(TestStream *s) { s->load = 1; s->pos = 0; s->error = 0; LagoonSoundtrack_LoadExtra(&s->base, 10); }

static void context(uint8 *rom, unsigned event, unsigned bank0, unsigned bank1,
                    unsigned return_pc, unsigned helper) {
  g_cpu.S = 0x1e0;
  for (unsigned i = 0; i < 3; ++i) g_ram[g_cpu.S + 1 + i] = return_pc >> (8 * i);
  unsigned p = (return_pc >> 16) * 0x8000 + (return_pc & 0x7fff) - 3;
  rom[p] = 0x22; rom[p + 1] = helper; rom[p + 2] = helper >> 8; rom[p + 3] = 0x0c;
  g_ram[0x438] = event; g_ram[0x439] = 0;
  g_ram[0x434] = bank0; g_ram[0x436] = bank1;
  static const struct { unsigned event; uint8_t p[3]; } records[] = {
    {0x10,{1,2,3}},{0x20,{1,2,0}},{0x21,{3,4,0}},
    {0x22,{5,6,0}},{0x23,{7,8,0}},{0x24,{9,10,0}},
    {0x25,{11,12,0}},{0x30,{1,0,0}},{0x31,{2,0,0}},
    {0x32,{3,0,0}},{0x35,{1,0,0}},{0x37,{3,0,0}},
    {0x38,{4,0,0}},{0x39,{5,0,0}},{0x3a,{1,0,0}},
    {0x3b,{2,0,0}},{0x3d,{4,0,0}},
  };
  for(unsigned i=0;i<sizeof(records)/sizeof(records[0]);i++)if(event==records[i].event){
    uint8_t *row=rom+0x679fc+event*6;row[0]=bank0;row[1]=bank1;row[2]=7;
    memcpy(row+3,records[i].p,3);
  }

}
int main(int argc, char **argv) {
  assert(argc == 2); /* Prefix containing generated track 1; track 2 absent, 4 bad. */
  _putenv_s("SNESRECOMP_MSU1", argv[1]);
  msu1_init();
  uint8 *rom = calloc(1, 0x100000);
  g_rom = rom;
  assert(!filter && activate); /* Constructor alone cannot activate playback. */
  activate();
  context(rom, 0x10, 1, 1, 0x0cf8e0, 0xfe00);
  assert(filter(0x2140, 1) == 1 && writes == 1 && last_value == 0);
  assert(msu1_read(0x2000) & 0x10);
  int16_t out[32] = {0};
  msu1_mix(out, 16);
  assert(LagoonSoundtrack_GetVolume()==100);
  assert(out[0] == 1000*145/255 && out[1] == -1000*145/255); /* Balanced title PCM. */
  /* A generated ramp proves live trim/mute never restarts or pauses the cursor. */
  LagoonSoundtrack_SetVolume(50);assert(LagoonSoundtrack_GetVolume()==50);
  memset(out,0,sizeof(out));msu1_mix(out,16);
  assert(out[0]==1016*73/255&&out[1]==-1016*73/255);
  LagoonSoundtrack_SetVolume(0);assert(LagoonSoundtrack_GetVolume()==0);
  memset(out,0,sizeof(out));msu1_mix(out,16);
  for(unsigned i=0;i<32;i++)assert(out[i]==0);
  LagoonSoundtrack_SetVolume(1000);assert(LagoonSoundtrack_GetVolume()==100);
  memset(out,0,sizeof(out));msu1_mix(out,16);
  assert(out[0]==1048*145/255&&out[1]==-1048*145/255);
  assert(msu1_read(0x2000)&0x10);
  context(rom, 0x10, 1, 1, 0x0cf8e9, 0xfe2f);
  assert(filter(0x2141, 2) == 1 && writes == 2 && last_reg == 0x2141 && last_value == 0);
  context(rom, 0x61, 1, 1, 0x0cf9c5, 0xfe3d);
  assert(filter(0x2141, 0x12) == 0 && writes == 2); /* SFX preserved. */
  assert(msu1_read(0x2000) & 0x10);
  context(rom, 0x26, 2, 4, 0x0cf8e0, 0xfe00);
  assert(filter(0x2140, 3) == 0 && !(msu1_read(0x2000) & 0x10)); /* Unknown cue. */
  context(rom, 0x20, 2, 4, 0x0cf8e0, 0xfe00);
  assert(filter(0x2140, 1) == 0); /* Missing PCM. */
  context(rom, 0x30, 2, 5, 0x0cf8e0, 0xfe00);
  assert(filter(0x2140, 1) == 0); /* Malformed PCM. */
  context(rom, 0x10, 1, 1, 0x0cff9f, 0xff69);
  assert(filter(0x2140, 1) == 0 && writes == 2); /* IPL payload cannot qualify. */
  context(rom, 0x10, 1, 1, 0x0cf8e0, 0xfe00);
  assert(filter(0x2140, 1) == 1);
  TestStream saved = {{stream_func, NULL}, {0}, 0, 0, 0, 0};
  LagoonSoundtrack_SaveExtra(&saved.base);
  assert(saved.size == 16 && !saved.error && saved.bytes[5] == 1);
  uint8_t snapshot[48] = {0};
  uint32_t header[2] = {0x52544c53u, 10};
  memcpy(snapshot, header, sizeof(header)); memcpy(snapshot + 32, saved.bytes, 16);
  assert(LagoonSoundtrack_Preflight(snapshot, sizeof(snapshot)));
  load_chunk(&saved); assert(!saved.error);
  g_cpu.master_cycles = 100;
  LagoonSoundtrack_SetVolume(25);
  LagoonSoundtrack_OnStateLoaded(10);
  assert(msu1_read(0x2000) & 0x10); /* State restores cue, not host preference. */
  assert(LagoonSoundtrack_GetVolume()==25);
  memset(out,0,sizeof(out));msu1_mix(out,16);
  assert(out[0]==1000*36/255&&out[1]==-1000*36/255);
  LagoonSoundtrack_SetVolume(100);
  frame_cb(); assert(msu1_read(0x2000) & 0x10); /* Reconcile does not undo load. */
  /* A failed preflight must erase an earlier staged cue. */
  load_chunk(&saved);
  snapshot[32] ^= 1;
  assert(!LagoonSoundtrack_Preflight(snapshot, sizeof(snapshot)));
  LagoonSoundtrack_OnStateLoaded(10);
  assert(!(msu1_read(0x2000) & 0x10));
  snapshot[32] ^= 1;
  assert(LagoonSoundtrack_Preflight(snapshot, 32)); /* Legacy/no title chunk. */
  TestStream legacy = {{stream_func, NULL}, {0}, 0, 0, 1, 0};
  load_chunk(&legacy); LagoonSoundtrack_OnStateLoaded(10);
  assert(!(msu1_read(0x2000) & 0x10));
  TestStream bad = saved; bad.bytes[6] ^= 1;
  load_chunk(&bad); assert(bad.error);
  bad = saved; bad.size = 15; load_chunk(&bad); assert(bad.error);
  _putenv_s("SNESRECOMP_MSU1", "missing-soundtrack-fixture");
  activate();
  unsigned missing_before = writes;
  load_chunk(&saved); LagoonSoundtrack_OnStateLoaded(10);
  assert(writes == missing_before + 3 && !(msu1_read(0x2000) & 0x10));
  _putenv_s("SNESRECOMP_MSU1", argv[1]);
  activate();
  /* Loading an audio-muted state after disabling the option restores the
   * original three guest commands rather than leaving permanent silence. */
  reset_session_cb();
  unsigned before = writes;
  load_chunk(&saved); LagoonSoundtrack_OnStateLoaded(10);
  assert(writes == before + 3 && last_reg == 0x2142 && last_value == 3);
  assert(!(msu1_read(0x2000) & 0x10));
  activate();
  assert(filter(0x2140, 1) == 1);
  runahead = 1; before = writes;
  assert(filter(0x2140, 1) == 0 && writes == before + 3);
  assert(!(msu1_read(0x2000) & 0x10));
  runahead = 0;
  assert(filter(0x2140, 1) == 1);
  netplay = 1; before = writes;
  assert(filter(0x2140, 1) == 0 && writes == before + 3);
  netplay = 0;
  assert(filter(0x2140, 1) == 1);
  LagoonSoundtrack_Reset();
  assert(!(msu1_read(0x2000) & 0x10));
  /* Newly supported named cues use generated-tone fixtures; no copyrighted
   * audio is required by this integration test. Exact source rows are checked. */
  static const unsigned expanded[][3] = {
    {0x21,2,4},{0x22,2,4},{0x23,2,4},{0x24,2,4},{0x25,2,4},
    {0x31,2,5},{0x32,2,5},{0x35,2,6},{0x37,2,6},{0x38,2,6},
    {0x39,2,6},{0x3a,2,7},{0x3b,2,7},{0x3d,2,8},
  };
  const unsigned gains[]={160,153,160,197,207,129,127,141,120,145,126,115,116,130};
  for(unsigned i=0;i<sizeof(expanded)/sizeof(expanded[0]);i++) {
    context(rom,expanded[i][0],expanded[i][1],expanded[i][2],0x0cf8e0,0xfe00);
    unsigned prior=writes;
    assert(filter(0x2140,1)==1&&writes==prior+1&&last_value==0);
    assert(msu1_read(0x2000)&0x10);
    int16_t pcm[32]={0};msu1_mix(pcm,16);
    assert(pcm[0]==(int)(1000*gains[i]/255)&&pcm[1]==-(int)(1000*gains[i]/255));
    /* The music trim cannot scale the pre-existing S-DSP/SFX mix. */
    LagoonSoundtrack_SetVolume(0);for(unsigned n=0;n<32;n++)pcm[n]=1234;
    msu1_mix(pcm,16);for(unsigned n=0;n<32;n++)assert(pcm[n]==1234);
    assert(msu1_read(0x2000)&0x10);LagoonSoundtrack_SetVolume(100);
    /* A mismatched source row cannot authorize replacing guest audio. */
    rom[0x679fc+expanded[i][0]*6+3]^=0x80;
    assert(filter(0x2140,1)==0&&!(msu1_read(0x2000)&0x10));
  }
  context(rom,0x23,2,4,0x0cf8e0,0xfe00);
  assert(filter(0x2140,1)==1);
  context(rom,0x34,2,5,0x0cf8e0,0xfe00);
  assert(filter(0x2140,5)==0&&!(msu1_read(0x2000)&0x10)); /* Unmapped boss. */
  free(rom);
  puts("PASS: PCM mixer, secondary BGM mute, SFX/IPL pass, cue fallback, legacy/new/malformed states, mode change, reset, all14newcue tones and source-row refusal");
  return 0;
}
