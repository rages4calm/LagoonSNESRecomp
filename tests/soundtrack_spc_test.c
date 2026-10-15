/* ROM-required integration test of Lagoon's real SPC driver, not a replacement
 * implementation. Build separately with the framework SPC/APU/DSP sources.
 * This test never writes the supplied ROM or touches an installed game. */
#include "snes/apu.h"
#include "snes/spc.h"
#include <assert.h>

/* Single-threaded harness supplies the observability globals/locks normally
 * owned by common_rtl. Actual synthesis and timer execution stay unchanged. */
int snes_frame_counter;
uint64_t g_apu_timer0_total_ticks;
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}

static unsigned word(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
static unsigned file_offset(unsigned bus) {
  return ((bus >> 16) & 127) * 0x8000 + (bus & 0x7fff);
}
static void upload(Apu *apu, const uint8_t *rom, unsigned offset) {
  for (;;) {
    assert(offset + 4 <= 0x100000);
    unsigned length = word(rom + offset), dest = word(rom + offset + 2);
    offset += 4;
    if (!length) return;
    assert(offset + length <= 0x100000 && dest + length <= 0x10000);
    memcpy(apu->ram + dest, rom + offset, length);
    offset += length;
  }
}
static void bank(Apu *apu, const uint8_t *rom, unsigned table, unsigned id) {
  const uint8_t *entry = rom + table + id * 4;
  upload(apu, rom, file_offset(word(entry) | (unsigned)entry[2] << 16));
}
static unsigned active(const Apu *apu) {
  unsigned mask = 0;
  for (unsigned i = 0; i < 8; ++i)
    if (apu->dsp->channel[i].gain) mask |= 1u << i;
  return mask;
}
static unsigned run(Apu *apu, unsigned cycles) {
  unsigned mask = 0;
  for (unsigned i = 0; i < cycles; ++i) {
    apu_cycle(apu);
    mask |= active(apu);
    /* Drain the native sample ring; this test examines per-voice envelopes. */
    apu->dsp->sampleRead = apu->dsp->sampleWrite;
  }
  return mask;
}
static void check(const uint8_t *rom, unsigned event, int combat) {
  Apu *apu = apu_init();
  apu_reset(apu);
  apu->romReadable = false;
  upload(apu, rom, 0x68000);
  const uint8_t *record = rom + 0x679fc + event * 6;
  bank(apu, rom, 0x67fcc, record[0]);
  bank(apu, rom, 0x67fd8, record[1]);
  apu->spc->pc = 0x0800;
  run(apu, 1024000);
  for (unsigned i = 0; i < 3; ++i) apu_writePortNow(apu, i, record[i + 3]);
  assert(run(apu, 1024000) != 0); /* Actual music is playing. */
  for (unsigned i = 0; i < 3; ++i) apu_writePortNow(apu, i, 0);
  run(apu, 1024000); /* Let envelopes and echo decay. */
  assert(active(apu) == 0);
  assert(run(apu, 128000) == 0); /* Driver does not restart music. */
  if (combat) {
    /* Event $61's real cave hit sequence uses voices 6 and 7. */
    apu_writePortNow(apu, 1, 0x12);
    apu_writePortNow(apu, 2, 0x13);
    unsigned mask = run(apu, 128000);
    assert((mask & 0xc0) == 0xc0);
    assert((mask & 0x3f) == 0);
    assert(apu->ram[4] == 0); /* Music sequence remains stopped. */
  }
  printf("event %02x: music starts, zero releases all voices%s\n", event,
         combat ? ", real hit still activates voices 6/7" : "");
  apu_free(apu);
}
int main(int argc, char **argv) {
  if (argc != 2) { fprintf(stderr, "usage: soundtrack_spc_test ROM\n"); return 2; }
  FILE *input = fopen(argv[1], "rb");
  if (!input) return 2;
  uint8_t *rom = malloc(0x100000);
  size_t size = fread(rom, 1, 0x100000, input);
  int extra = fgetc(input);
  fclose(input);
  assert(size == 0x100000 && extra == EOF);
  check(rom, 0x10, 0);
  check(rom, 0x20, 0);
  check(rom, 0x30, 1);
  free(rom);
  return 0;
}
