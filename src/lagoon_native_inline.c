/* Source-owned bank1 inline tables. Unknown callers retain the generated
 * prefix and original indirect jump; no return frame is manufactured. */
#include "common_rtl.h"
#include "lagoon_native_inline.h"
#include "lagoon_native_endgame.h"
#include "cpu_trace.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint16_t call, count;
  uint64_t hash;
  uint16_t owner[4];
  uint64_t parent_hash;
  uint64_t selectors;
} InlineFamily;

static const InlineFamily families[] = {
  {0x92CD,16,UINT64_C(0x4CE96AE2294F0E62),{0x9294},UINT64_C(0xBBEA194FD569EA94),0},
  {0x95CA,16,UINT64_C(0xD09E1DB81D85520C),{0x81A7,0x820B,0xB0F3,0xB343},UINT64_C(0xBC90844FD5F7378C),0},
  {0xB848,16,UINT64_C(0x552F104AF798C38E),{0xB819},UINT64_C(0xBE28294FD75169B3),0},
  {0xB939,16,UINT64_C(0x30553B57596F8E6E),{0xB5C2},UINT64_C(0xBDE4304FD717A8CC),0},
  {0xD409,25,UINT64_C(0x33C94789007A6341),{0x81AA},UINT64_C(0xBB5C824FD4F297B9),0},
  {0xCCDB,40,UINT64_C(0x388D2B99932B1042),{0xCCD8},UINT64_C(0xBB983F4FD52420FE),
   UINT64_C(0xF2FFEE0F78)}
};

/* Exact CCDB targets proved against original handlers; remaining selectors stay original. */
static const uint16_t script_targets[40]={
  [3]=0xCD65,
  [4]=0xCD79,
  [5]=0xCD8D,
  [6]=0xCDA0,
  [8]=0xCDE3,
  [9]=0xCDF6,
  [10]=0xCE0A,
  [11]=0xCE1E,
  [17]=0xD17A,
  [18]=0xD193,
  [19]=0xD199,
  [21]=0xD20C,
  [22]=0xD27D,
  [23]=0xD2BB,
  [24]=0xD2C5,
  [25]=0xD2CB,
  [26]=0xD2D1,
  [27]=0xD2E1,
  [28]=0xD2E9,
  [29]=0xD2F0,
  [30]=0xD31D,
  [31]=0xD341,
  [33]=0xD352,
  [36]=0xD3B4,
  [37]=0xD3BB,
  [38]=0xD3C1,
  [39]=0xD3C8,
};

static uint64_t source_hash(unsigned pc, unsigned n) {
  uint64_t h = UINT64_C(1469598103934665603);
  if (!g_rom) return 0;
  for (unsigned i = 0; i < n; ++i)
    h = (h ^ g_rom[pc + i]) * UINT64_C(1099511628211);
  return h;
}

static unsigned word(const uint8_t *p) { return p[0] | p[1] << 8; }
static void nz(CpuState *c, unsigned v, unsigned sign) {
  c->_flag_Z = !v;
  c->_flag_N = !!(v & sign);
  c->P = (c->P & ~0x82) | (c->_flag_Z ? 2 : 0) | (c->_flag_N ? 0x80 : 0);
}
static void a8(CpuState *c, unsigned v) {
  c->A = (c->A & 0xFF00) | (v & 255);
  nz(c, v & 255, 0x80);
}

bool LagoonInlineTable_Run(CpuState *c, uint16_t entry_s, uint8_t hrv,
                          uint32_t host_return, RecompReturn *out) {
  if (!c || !c->ram || !out || c->PB || c->DB != 1 || c->D ||
      !c->m_flag || c->x_flag || c->emulation || c->_flag_D ||
      (c->P & (CPU_P_M | CPU_P_X | CPU_P_D)) != CPU_P_M ||
      hrv != 3 || c->host_return_valid != 3 || entry_s != c->S ||
      !interp_bridge_has_direct_paired_bounce() ||
      c->S < 0x100 || c->S > 0x1FFA || c->ram[c->S + 3] != 1 ||
      source_hash(0xE9, 32) != UINT64_C(0xD98520CA8A4BEFBC)) return false;
  unsigned call = (word(c->ram + c->S + 1) - 3) & 65535;
  const InlineFamily *f = NULL;
  for (unsigned i = 0; i < sizeof families / sizeof *families; ++i)
    if (families[i].call == call) f = families + i;
  if (!f || c->A >= f->count || host_return != (0x010000u | (call + 4)) ||
      source_hash(0x8000 + (call & 32767), 4 + f->count * 2) != f->hash)
    return false;
  if (f->selectors && !(f->selectors & (UINT64_C(1) << c->A))) return false;
  unsigned owner = (word(c->ram + c->S + 4) + 1) & 65535, known = 0;
  for (unsigned i = 0; i < 4; ++i) known |= f->owner[i] && owner == f->owner[i];
  if (!known || source_hash(0x8000 + ((owner - 3) & 32767), 3) != f->parent_hash)
    return false;
  unsigned target = 0x010000 | word(g_rom + 0x8000 + ((call + 4) & 32767) + c->A * 2);
  if (f->call == 0xCCDB && target != (0x010000u | script_targets[c->A])) return false;
  if (!cpu_dispatch_has_entry(c, target) || cpu_dispatch_inline_arg_bytes(target))
    return false;
  LagoonNativeEndgameConsumeInline((unsigned)(f - families));

  /* Each original instruction commits separately. The guarded inputs have
   * D=0 and PB=0; data/stack/pointer bus costs still use the real runtime. */
#define STEP(pc, cyc, bytes) do { \
    cpu_instruction_timing_commit(c); \
    if (interp_bridge_lle_master_deadline_reached(c)) { \
      *out = interp_bridge_lle_yield_unwind(c, pc); return true; \
    } \
    cpu_trace_block(c, pc); \
    c->coprocessor_master_cycles = c->master_cycles; \
    c->cycles += cyc; c->master_cycles += (cyc) * 6 + (bytes) * 2; \
  } while (0)
  STEP(0x80E9,3,2); cpu_write8_timed(c,0,0,c->A);
  STEP(0x80EB,4,1); a8(c,cpu_read8_timed(c,0,++c->S));
  STEP(0x80EC,3,2); cpu_write8_timed(c,0,1,c->A);
  STEP(0x80EE,4,1); a8(c,cpu_read8_timed(c,0,++c->S));
  STEP(0x80EF,3,2); cpu_write8_timed(c,0,2,c->A);
  STEP(0x80F1,4,1); a8(c,cpu_read8_timed(c,0,++c->S));
  STEP(0x80F2,3,2); cpu_write8_timed(c,0,3,c->A);
  STEP(0x80F4,3,2); cpu_write8_timed(c,0,6,c->A);
  STEP(0x80F6,4,1); --c->S; cpu_write16_timed(c,0,c->S,c->Y); --c->S;
  STEP(0x80F7,3,2); a8(c,cpu_read8_timed(c,0,0));
  STEP(0x80F9,2,1);
  c->_flag_C = !!(c->A & 0x80); c->P = (c->P & ~1) | c->_flag_C;
  a8(c,(c->A & 255) << 1);
  STEP(0x80FA,2,1); c->Y = c->A; nz(c,c->Y,0x8000);
  STEP(0x80FB,2,1); ++c->Y; nz(c,c->Y,0x8000);
  STEP(0x80FC,6,2);
  unsigned p = cpu_read16_timed(c,0,1) | cpu_read8_timed(c,0,3) << 16;
  p = (p + c->Y) & 0xFFFFFF; a8(c,cpu_read8_timed(c,p >> 16,p));
  STEP(0x80FE,3,2); cpu_write8_timed(c,0,4,c->A);
  STEP(0x8100,2,1); ++c->Y; nz(c,c->Y,0x8000);
  STEP(0x8101,6,2);
  p = cpu_read16_timed(c,0,1) | cpu_read8_timed(c,0,3) << 16;
  p = (p + c->Y) & 0xFFFFFF; a8(c,cpu_read8_timed(c,p >> 16,p));
  STEP(0x8103,3,2); cpu_write8_timed(c,0,5,c->A);
  STEP(0x8105,5,1);
  ++c->S; c->Y = cpu_read16_timed(c,0,c->S); ++c->S; nz(c,c->Y,0x8000);
  STEP(0x8106,6,3);
  p = cpu_read16_timed(c,0,4) | cpu_read8_timed(c,0,6) << 16;
  c->PB = p >> 16;
  cpu_instruction_timing_commit(c);
  /* The real DC has selected p and left the original two-byte owner. Pair
   * that existing frame with the exact available native handler. Its clean
   * RTS consumes the owner, so the helper must unwind to that owner's actual
   * continuation rather than return into the caller's inline table bytes. */
  *out = cpu_dispatch_pc_paired(c,p,2);
  if (*out == RECOMP_RETURN_NORMAL && !interp_bridge_lle_unwind_pending()) {
    if (c->S != (uint16_t)(entry_s + 5) || c->PB != 1) {
      /* The handler has already mutated guest state. There is no safe PC to
       * infer here: never resume the consumed inline table or invent a frame. */
      fprintf(stderr, "[native_contract] inline target=$%06X returned NORMAL "
          "S=$%04X PB=$%02X; expected consumed owner=$01%04X S=$%04X PB=$01\n",
          p, c->S, c->PB, owner, (uint16_t)(entry_s + 5));
      abort();
    }
    *out = interp_tier_dispatch_rewritten_return(c,0x010000 | owner,0x008106);
  }
  return true;
#undef STEP
}
