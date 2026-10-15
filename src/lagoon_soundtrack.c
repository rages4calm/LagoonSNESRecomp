/* Default-off, partial X68000 replacement plus passive command research.
 * Only verified music-helper calls may be replaced; SFX and IPL pass through.
 * The independent probe never changes writes. Owner audio stays external. */
#include "lagoon_soundtrack.h"
#include "mod_runtime.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/msu1.h"
#include "snes/saveload.h"
#include "snes_runahead.h"
#include "netplay/snes_netplay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SoundEvent {
  uint64_t cycle;
  uint32_t pc;
  uint16_t event, reg;
  uint32_t return_pc;
  uint16_t stack;
  uint16_t helper;
  uint8_t value, bank0, bank1;
} SoundEvent;

/* Uploads can be large. Bounded memory and a dropped count keep an accidental
 * long probe from growing without limit. Frame flush keeps file I/O outside
 * the port write callback. The observer is on the CPU thread only. */
#define SOUND_EVENT_CAPACITY 65536u
static SoundEvent events[SOUND_EVENT_CAPACITY];
static unsigned count, dropped, frame;
static FILE *probe;
static int replacement, playing_event;
static uint64_t last_cycle;
static uint8_t pending_chunk[16];
static int pending_valid;
static char private_prefix[4096];

static uint32_t stacked_return(void) {
  if (g_cpu.S >= 0x1ffc) return 0;
  return g_ram[g_cpu.S + 1] | ((uint32_t)g_ram[g_cpu.S + 2] << 8) |
         ((uint32_t)g_ram[g_cpu.S + 3] << 16);
}

/* At each helper's STA, its temporary PHA has already been pulled. The
 * hardware JSL return frame is therefore on top for generated and interpreted
 * code alike. Validate the ROM instruction instead of trusting a stale PC.
 * Return addresses point to the final byte of JSL, not the following byte. */
static uint16_t command_helper(uint16_t reg, uint32_t ret) {
  unsigned bank = ret >> 16, address = ret & 0xffff;
  if (!g_rom || bank > 0x0f || address < 0x8003) return 0;
  unsigned offset = bank * 0x8000 + (address & 0x7fff) - 3;
  if (g_rom[offset] != 0x22 || g_rom[offset + 3] != 0x0c) return 0;
  unsigned helper = g_rom[offset + 1] | ((unsigned)g_rom[offset + 2] << 8);
  if ((reg == 0x2140 && (helper == 0xfe00 || helper == 0xfe0e)) ||
      (reg == 0x2141 && (helper == 0xfe2f || helper == 0xfe3d)) ||
      (reg == 0x2142 && (helper == 0xfe5e || helper == 0xfe6c)))
    return (uint16_t)helper;
  return 0;
}

/* Deliberately partial, scene-qualified mapping. Unknown cues keep SNES music.
 * The archive's sequence numbers are not guest command IDs. */
static int cue_info(unsigned event, uint8_t *bank0, uint8_t *bank1,
                    uint8_t payload[3]) {
  if (event == 0x10) { *bank0 = 1; *bank1 = 1; payload[0] = 1; payload[1] = 2; payload[2] = 3; return 1; }
  if (event == 0x20) { *bank0 = 2; *bank1 = 4; payload[0] = 1; payload[1] = 2; payload[2] = 0; return 2; }
  if (event == 0x30) { *bank0 = 2; *bank1 = 5; payload[0] = 1; payload[1] = 0; payload[2] = 0; return 4; }
  return 0;
}
static unsigned mapped_track(unsigned event) {
  uint8_t bank0, bank1, payload[3];
  unsigned track = cue_info(event, &bank0, &bank1, payload);
  return track && g_ram[0x434] == bank0 && g_ram[0x436] == bank1 ? track : 0;
}

static int valid_pcm(unsigned track) {
  /* Require an explicit importer prefix. No "auto" or directory guessing:
   * the validation must inspect exactly the file MSU will subsequently open. */
  const char *base = private_prefix;
  char path[4096];
  if (!base || !*base) return 0;
  int n = snprintf(path, sizeof(path), "%s-%u.pcm", base, track);
  if (n < 0 || (size_t)n >= sizeof(path)) return 0;
  FILE *f = fopen(path, "rb");
  if (!f) return 0;
  uint8_t header[8] = {0};
  int ok = fread(header, 1, sizeof(header), f) == sizeof(header) &&
           memcmp(header, "MSU1", 4) == 0 && fseek(f, 0, SEEK_END) == 0;
  long size = ok ? ftell(f) : -1;
  uint32_t loop = (uint32_t)header[4] | ((uint32_t)header[5] << 8) |
    ((uint32_t)header[6] << 16) | ((uint32_t)header[7] << 24);
  fclose(f);
  return ok && size > 8 && (size - 8) % 4 == 0 &&
         loop < (uint32_t)((size - 8) / 4);
}

static void stop_pcm(void) {
  if (playing_event && msu1_enabled()) msu1_write(0x2007, 0);
  playing_event = 0;
}

static int start_pcm(unsigned event) {
  unsigned track = mapped_track(event);
  if (!track || !msu1_enabled() || !valid_pcm(track)) {
    stop_pcm();
    return 0;
  }
  if (playing_event == (int)event && (msu1_read(0x2000) & 0x10)) return 1;
  stop_pcm();
  msu1_write(0x2004, (uint8_t)track);
  msu1_write(0x2005, 0);
  if (msu1_read(0x2000) & 0x08) return 0;
  msu1_write(0x2006, 255);
  msu1_write(0x2007, 3); /* These three archive tracks all loop. */
  if (!(msu1_read(0x2000) & 0x10)) return 0;
  playing_event = (int)event;
  fprintf(stderr, "lagoon.soundtrack: event %02x -> X68000 track %u\n", event, track);
  return 1;
}

static void restore_stock(unsigned event) {
  uint8_t bank0, bank1, payload[3];
  if (!cue_info(event, &bank0, &bank1, payload) ||
      g_ram[0x434] != bank0 || g_ram[0x436] != bank1) return;
  for (unsigned i = 0; i < 3; ++i) RtlApuWrite((uint16_t)(0x2140 + i), payload[i]);
}

static int unsupported_mode(void) {
  return snes_runahead_active() || snes_netplay_active();
}

static int replace_write(uint16_t reg, uint8_t value) {
  if (replacement && unsupported_mode()) {
    unsigned cue = (unsigned)playing_event;
    stop_pcm();
    if (cue) restore_stock(cue);
    return 0;
  }
  if (!replacement || !command_helper(reg, stacked_return())) return 0;
  unsigned event = g_ram[0x438] | ((unsigned)g_ram[0x439] << 8);
  if (reg == 0x2140) {
    /* Port zero owns six BGM voices. Other cues, silence, missing resources,
     * and unsupported scenes stop host PCM before passing stock commands. */
    if (!value || !start_pcm(event)) { stop_pcm(); return 0; }
  }
  if (playing_event == (int)event && mapped_track(event)) {
    /* Suppress only this qualified music event's helper writes, including
     * secondary BGM voices. Real SFX events on ports 1/2 pass through. Calling
     * RtlApuWrite directly bypasses mod filtering, retaining guest timestamps. */
    RtlApuWrite(reg, 0);
    return 1;
  }
  return 0;
}

void LagoonSoundtrack_Reset(void) {
  stop_pcm(); pending_valid = 0; last_cycle = g_cpu.master_cycles;
}

static void reconcile_frame(void) {
  if (playing_event && (g_cpu.master_cycles < last_cycle || unsupported_mode())) {
    unsigned cue = (unsigned)playing_event;
    stop_pcm(); restore_stock(cue);
  }
  last_cycle = g_cpu.master_cycles;
}

static uint16_t chunk_check(const uint8_t *chunk) {
  uint16_t sum = 0x4c47;
  for (unsigned i = 0; i < 14; ++i) sum = (uint16_t)((sum * 31u) ^ chunk[i]);
  return sum;
}
static int chunk_valid(const uint8_t *c) {
  if (memcmp(c, "LGA1", 4) || c[4] != 1 || c[5] > 1 || c[7] || c[13] ||
      chunk_check(c) != (uint16_t)(c[14] | (unsigned)c[15] << 8)) return 0;
  if (!c[5]) {
    for (unsigned i = 6; i < 14; ++i) if (c[i]) return 0;
    return 1;
  }
  uint8_t b0, b1, payload[3];
  return cue_info(c[6], &b0, &b1, payload) && b0 == c[8] && b1 == c[9] &&
         !memcmp(payload, c + 10, 3);
}

bool LagoonSoundtrack_Preflight(const void *data, size_t size) {
  pending_valid = 0;
  if (size < 8) return false;
  uint32_t header[2], current_version;
  memcpy(header, data, sizeof(header));
  size_t guest = RtlSnapshotGuestSize(&current_version);
  if (header[0] != 0x52544c53u) return false;
  if (header[1] != current_version) return true; /* Generic legacy validation. */
  if (!guest) return false;
  if (size == guest) return true; /* Current-format state before audio support. */
  return size == guest + sizeof(pending_chunk) &&
         chunk_valid((const uint8_t *)data + guest);
}

void LagoonSoundtrack_SaveExtra(SaveLoadInfo *sli) {
  uint8_t c[16] = {'L','G','A','1',1};
  if (playing_event && mapped_track((unsigned)playing_event)) {
    c[5] = 1; c[6] = (uint8_t)playing_event;
    cue_info(c[6], c + 8, c + 9, c + 10);
  }
  uint16_t check = chunk_check(c);
  c[14] = (uint8_t)check; c[15] = (uint8_t)(check >> 8);
  sli->func(sli, c, sizeof(c));
}

void LagoonSoundtrack_LoadExtra(SaveLoadInfo *sli, uint32_t version) {
  (void)version;
  pending_valid = 0;
  size_t remaining = RtlStateBytesRemaining(sli);
  if (!remaining) return;
    if (remaining < sizeof(pending_chunk)) { RtlStateStreamFail(sli); return; }
  sli->func(sli, pending_chunk, sizeof(pending_chunk));
  if (!chunk_valid(pending_chunk)) { RtlStateStreamFail(sli); return; }
  pending_valid = 1;
}

void LagoonSoundtrack_OnStateLoaded(uint32_t version) {
  (void)version;
  stop_pcm();
  if (pending_valid && pending_chunk[5]) {
    unsigned cue = pending_chunk[6];
    if (!replacement || unsupported_mode() || !start_pcm(cue)) restore_stock(cue);
  }
  pending_valid = 0;
  last_cycle = g_cpu.master_cycles;
}

static bool resolve_track(void *context, const char *base, uint16_t track,
                          char *path, size_t capacity) {
  (void)context; (void)base;
  int n = snprintf(path, capacity, "%s-%u.pcm", private_prefix, (unsigned)track);
  return private_prefix[0] && n >= 0 && (size_t)n < capacity;
}

static void activate_replacement(void) {
  char directory[4000];
  private_prefix[0] = 0;
  if (snes_mod_runtime_resource_path_c("lagoon.soundtrack", "x68000", "pack",
                                       directory, sizeof(directory)) && directory[0]) {
    snprintf(private_prefix, sizeof(private_prefix), "%s/lagoon-x68000", directory);
  } else {
    /* Standalone embedding/tests may arm an explicit prefix themselves. The
     * normal mod runtime clears this environment when no resource is selected. */
    const char *base = getenv("SNESRECOMP_MSU1");
    if (base) snprintf(private_prefix, sizeof(private_prefix), "%s", base);
  }
  replacement = 1;
  msu1_set_track_resolver(resolve_track, NULL);
  playing_event = 0;
  last_cycle = g_cpu.master_cycles;
  snes_mod_register_apu_write_callback(replace_write);
  snes_mod_register_frame_callback(reconcile_frame);
}

static void reset_session(void) {
  stop_pcm();
  replacement = 0;
  private_prefix[0] = 0;
  msu1_set_track_resolver(NULL, NULL);
}

static int observe_write(uint16_t reg, uint8_t value) {
  if (reg < 0x2140 || reg > 0x2143 || !probe) return 0;
  if (count < SOUND_EVENT_CAPACITY) {
    SoundEvent *e = &events[count++];
    e->cycle = g_cpu.master_cycles;
    /* This is the last interpreter cursor, NOT the bus instruction PC:
     * generated callees can leave their interpreter caller's value behind.
     * It helps correlate traces but must never authorize interception. */
    e->pc = g_interp816_cur_pc;
    e->stack = g_cpu.S;
    e->return_pc = 0;
    if (g_cpu.S < 0x1ffc) {
      e->return_pc = g_ram[g_cpu.S + 1] |
        ((uint32_t)g_ram[g_cpu.S + 2] << 8) |
        ((uint32_t)g_ram[g_cpu.S + 3] << 16);
    }
    e->helper = command_helper(reg, e->return_pc);
    e->event = g_ram[0x438] | ((uint16_t)g_ram[0x439] << 8);
    e->reg = reg; e->value = value;
    e->bank0 = g_ram[0x434]; e->bank1 = g_ram[0x436];
  } else ++dropped;
  return 0;
}

static void flush_frame(void) {
  if (!probe) return;
  ++frame;
  for (unsigned i = 0; i < count; ++i) {
    const SoundEvent *e = &events[i];
    fprintf(probe, "%u,%llu,%06x,%04x,%04x,%02x,%02x,%02x,%04x,%06x,%04x\n", frame,
      (unsigned long long)e->cycle, (unsigned)e->pc, (unsigned)e->event,
      (unsigned)e->reg, (unsigned)e->value, (unsigned)e->bank0,
      (unsigned)e->bank1, (unsigned)e->stack, (unsigned)e->return_pc,
      (unsigned)e->helper);
  }
  count = 0;
  if (dropped) {
    fprintf(probe, "# dropped=%u frame=%u\n", dropped, frame);
    dropped = 0;
  }
  fflush(probe);
}

static void activate_probe(void) {
  const char *path = getenv("LAGOON_SOUNDTRACK_PROBE");
  if (probe) { flush_frame(); fclose(probe); probe = NULL; }
  count = dropped = frame = 0;
  if (!path || !*path) return;
  /* Exclusive creation protects an earlier receipt or an owner file from
   * truncation. Parent supplies a fresh private path per route. */
  probe = fopen(path, "wx");
  if (!probe) {
    fprintf(stderr, "lagoon.soundtrack: cannot create fresh probe %s\n", path);
    return;
  }
  fprintf(probe, "frame,master_cycle,interpreter_cursor,event,port,value,bank0,bank1,stack,stack_top24,command_helper\n");
  snes_mod_register_apu_write_callback(observe_write);
  snes_mod_register_frame_callback(flush_frame);
}

SNES_MOD_CONSTRUCTOR(lagoon_soundtrack_register) {
  snes_mod_register_reset_callback(reset_session);
  snes_mod_register_activation_plugin("lagoon.soundtrack.probe", activate_probe);
  snes_mod_register_activation_plugin("lagoon.soundtrack.x68000", activate_replacement);
}
