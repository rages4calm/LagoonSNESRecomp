/* Actual SDK rewrite function with bounded execution-context stand-ins.
 * This checks its ownership/propagation policy, not scheduler or APU behavior. */
#include "cpu_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int direct, crossed, nested_result, delta, skip, propagated;
static int nested_calls, yield_calls, s_lle_unwind_active;
static int s_interp_bounce_recomp_base = 1;
static uint16_t seen_s;
static uint32_t seen_target, seen_source;
enum { TIER2_KIND_DISPATCH = 1 };
int interp_bridge_has_direct_paired_bounce(void) { return direct; }
static int interp_owner_crossed(unsigned s) { (void)s; return crossed; }
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c, uint32_t pc) {
  ++yield_calls; seen_s = c->S; seen_target = pc;
  return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;
}
static void interp_tier_note(uint32_t pc) { seen_target = pc; }
static uint8_t tier2_entry_mx(CpuState *c) { (void)c; return 2; }
static void tier2_discover(uint32_t s, uint32_t t, uint8_t mx, int kind, int id) {
  assert(mx == 2 && kind == TIER2_KIND_DISPATCH && id == -1);
  seen_source = s; seen_target = t;
}
static int tier2_outcome(int ok) { return ok; }
static void tier2_record(uint32_t s, uint32_t t, uint8_t mx, int k, int ok) {
  assert(s == seen_source && t == seen_target && mx == 2 && k == 1);
  (void)ok;
}
static int interp_run_propagated_return(int ok, RecompReturn *out) {
  (void)ok; *out = RECOMP_RETURN_SKIP_2; return propagated;
}
int cpu_resolve_post_return_skip(uint16_t s) { (void)s; return skip; }
static int interp_bridge_run_ex2(CpuState *c, uint32_t pc, uint16_t s,
    uint32_t *landing, uint32_t *ret, uint32_t yield, uint16_t flag,
    uint8_t value, int reset, const uint32_t *stop, int count, int rti) {
  assert(pc == seen_target && s == c->S && landing && !ret && !yield &&
         !flag && !value && !reset && !stop && !count && !rti);
  seen_s = s; ++nested_calls; c->S += delta; return nested_result;
}
#include "inline_rewrite_under_test.h"

int main(void) {
  unsigned cases = 0;
  for (unsigned base = 0; base < 2; ++base)
    for (unsigned d = 0; d < 2; ++d)
      for (unsigned x = 0; x < 2; ++x)
        for (unsigned result = 0; result < 4; ++result) {
          CpuState c = {0}; c.S = base ? 0x1FD0 : 0x180;
          c.cycles = 197; c.master_cycles = 1594; c.coprocessor_master_cycles = 1542;
          CpuState before = c;
          direct = d; crossed = x; s_interp_bounce_recomp_base = 1;
          nested_result = result != 0; delta = 2; skip = result == 1 ? 3 : -1;
          propagated = result == 2; s_lle_unwind_active = result == 3;
          nested_calls = yield_calls = 0;
          RecompReturn out = interp_tier_dispatch_rewritten_return(&c,0x01D460,0x008106);
          if (d && !x) {
            assert(out == RECOMP_RETURN_LLE_UNWIND_BASE && yield_calls == 1 && !nested_calls);
            assert(!memcmp(&c,&before,sizeof c));
          } else {
            assert(nested_calls == 1 && !yield_calls && seen_s == before.S);
            assert(seen_source == 0x008106 && seen_target == 0x01D460);
            assert(out == (result == 0 ? RECOMP_RETURN_NORMAL :
                result == 1 ? RECOMP_RETURN_SKIP_3 :
                result == 2 ? RECOMP_RETURN_SKIP_2 : RECOMP_RETURN_LLE_UNWIND_BASE));
            before.S += result == 0 ? 0 : 2;
            assert(!memcmp(&c,&before,sizeof c));
          }
          ++cases;
        }
  printf("PASS actual SDK rewritten-return policy %u contexts: direct paired handoff, compiled-ancestor nested run, failure restores post-pop S, nonlocal/propagated/deadline unwind; clock fields untouched\n",cases);
  return 0;
}
