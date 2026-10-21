#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Device, mutex and lookup plumbing are mocked. Policy, commit, DRAM refresh
 * and paired-return functions are extracted verbatim from the live SDK. */
typedef struct TestApu { int portTimeValid; } TestApu;
typedef struct TestSnes { int *cart; TestApu *apu; double apuCatchupCycles; } TestSnes;
static TestSnes *g_snes;
CpuState g_cpu;
static int timeline, extended, shared, snes_frame_counter;
static uint64_t g_apu_last_sync_master, s_apu_pending_master;
static uint64_t g_refresh_charged_upto, s_refresh_phase;
static const double kInterpApuPerMaster = (32040.0 * 32.0)/(1364.0*262.0*60.0);
static uint64_t beam, sync_masters[16384], sync_beams[16384];
static unsigned syncs, locks, unlocks, lock_depth, relative_calls, beam_commits, cart_commits;
static bool rtl_apu_frame_timeline_active(void) { return timeline != 0; }
static bool rtl_apu_extended_frame_timing(void) { return extended != 0; }
static int cart_has_sa1(const int *c) { return c && *c; }
static int cosim_apu_shared_clock(void) { return shared; }
static void RtlApuLock(void) { assert(lock_depth == 0); lock_depth++; locks++; }
static void RtlApuUnlock(void) { assert(lock_depth == 1); lock_depth--; unlocks++; }
static void rtl_sync_apu_to_cpu_locked(void) {
    assert(lock_depth == 1 && syncs < 16384);
    sync_masters[syncs] = g_cpu.master_cycles;
    sync_beams[syncs++] = beam;
}
static void snes_catchupApu(TestSnes *snes) { assert(snes == g_snes && lock_depth == 1); relative_calls++; }
static void snes_sync_master_clock(TestSnes *snes, uint64_t master) {
    assert(snes == g_snes && lock_depth == 0); beam = master; beam_commits++;
}
static void cart_sync_coprocessors(int *cart, uint64_t master) {
    assert(cart == g_snes->cart && beam == master); cart_commits++;
}
static RecompReturn (*selected_fp)(CpuState *);
static unsigned lookup_calls, log_calls;
static int fake_pending, fake_mirror;
static RecompReturn (*_cpu_dispatch_lookup(CpuState *cpu, uint32 pc))(CpuState *) {
    (void)cpu; lookup_calls++;
    return fake_mirror && !(pc & 0x800000) ? NULL : selected_fp;
}
static int dispatch_has_rom_mirror(uint32 pc) { (void)pc; return fake_mirror; }
static void _dispatch_log_record(uint32 pc, uint32 from, unsigned mx, int found, int mirror) {
    (void)pc; (void)from; (void)mx; (void)found; (void)mirror; log_calls++;
}
int interp_bridge_lle_unwind_pending(void) { return fake_pending; }
#define SNES_COSIM 1
#include "apu_cadence_under_test.h"

static int sa1;
static TestApu apu;
static TestSnes snes;
static uint64_t fake_cost;
static uint8 fake_pop, fake_hrv;
static RecompReturn fake_result;
static unsigned function_calls;
static RecompReturn fake_body(CpuState *cpu) {
    assert(cpu->host_return_valid == fake_hrv);
    function_calls++;
    cpu->master_cycles += fake_cost;
    cpu->S = (uint16)(cpu->S + fake_pop);
    return fake_result;
}
static void reset(void) {
    memset(&g_cpu, 0, sizeof(g_cpu));
    sa1 = 0; apu.portTimeValid = 1; snes.cart = &sa1; snes.apu = &apu;
    snes.apuCatchupCycles = 0; g_snes = &snes;
    timeline = extended = snes_frame_counter = 1; shared = 0;
    g_apu_last_sync_master = s_apu_pending_master = beam = 0;
    g_refresh_charged_upto = s_refresh_phase = 0;
    syncs = locks = unlocks = lock_depth = relative_calls = beam_commits = cart_commits = 0;
    lookup_calls = log_calls = function_calls = 0;
    fake_pending = fake_mirror = 0; selected_fp = fake_body;
    fake_cost = 48; fake_pop = 2; fake_hrv = 2; fake_result = RECOMP_RETURN_NORMAL;
}
static void interpreted_complete(uint64_t cost) {
    g_cpu.master_cycles += cost;
    snes_refresh_charge();
    g_cpu.coprocessor_master_cycles = g_cpu.master_cycles;
    snes_sync_master_clock(g_snes, g_cpu.master_cycles);
    cart_sync_coprocessors(g_snes->cart, g_cpu.master_cycles);
    if (!cosim_apu_shared_clock()) {
        s_apu_pending_master += cost;
        if (bridge_periodic_apu_flush_due(&g_cpu)) bridge_apu_flush(&g_cpu);
    }
}
typedef struct Captured {
    uint64_t master, anchor, refresh_phase, charged, clock[16384], beam[16384];
    unsigned count;
} Captured;
static Captured captured;
static void capture(void) {
    captured.master = g_cpu.master_cycles; captured.anchor = g_apu_last_sync_master;
    captured.refresh_phase = s_refresh_phase; captured.charged = g_refresh_charged_upto;
    captured.count = syncs;
    memcpy(captured.clock, sync_masters, syncs * sizeof(uint64_t));
    memcpy(captured.beam, sync_beams, syncs * sizeof(uint64_t));
}
static void compare_capture(void) {
    assert(captured.master == g_cpu.master_cycles && captured.anchor == g_apu_last_sync_master);
    assert(captured.refresh_phase == s_refresh_phase && captured.charged == g_refresh_charged_upto);
    assert(captured.count == syncs);
    assert(!memcmp(captured.clock, sync_masters, syncs * sizeof(uint64_t)));
    assert(!memcmp(captured.beam, sync_beams, syncs * sizeof(uint64_t)));
    assert(locks == unlocks && locks == syncs && lock_depth == 0);
}
int main(int argc, char **argv) {
    assert(argc == 3);
    uint64_t threshold = strtoull(argv[1], NULL, 0);
    int no_apu = atoi(argv[2]); unsigned tests = 0;
    assert(rtl_apu_periodic_flush_threshold() == threshold);
    assert(bridge_bounce_flush_thresh() == threshold);
    const uint64_t times[] = {0,1,1023,1024,1025,2048,UINT64_MAX-1,UINT64_MAX};
    const uint64_t limits[] = {0,1,1024,UINT64_MAX};
    for (unsigned absolute=0; absolute<2; absolute++) for (unsigned m=0; m<8; m++)
    for (unsigned s=0; s<8; s++) for (unsigned p=0; p<8; p++) for (unsigned t=0; t<4; t++) {
        uint64_t master=times[m], sync=times[s], pending=times[p], limit=limits[t];
        int expected=absolute ? (master>=sync && master-sync>=limit) : pending>=limit;
        assert(interp_bridge_apu_periodic_flush_due(absolute,master,sync,pending,limit)==expected); tests++;
    }
    for (unsigned bits=0; bits<256; bits++) for (unsigned m=0; m<8; m++)
    for (unsigned s=0; s<8; s++) for (unsigned p=0; p<8; p++) {
        reset(); timeline=!!(bits&1); extended=!!(bits&2); sa1=!!(bits&4);
        apu.portTimeValid=!!(bits&8); shared=!!(bits&16); snes_frame_counter=!(bits&128);
        snes.apu=bits&32 ? NULL : &apu; g_snes=bits&64 ? NULL : &snes;
        g_cpu.master_cycles=times[m]; g_apu_last_sync_master=times[s]; s_apu_pending_master=times[p];
        int absolute=!no_apu && snes_frame_counter && timeline && !shared && g_snes &&
                     snes.apu && (sa1 || (extended && apu.portTimeValid));
        assert(rtl_apu_absolute_periodic_enabled()==absolute);
        int expected=absolute ? (g_cpu.master_cycles>=g_apu_last_sync_master &&
             g_cpu.master_cycles-g_apu_last_sync_master>=threshold) : s_apu_pending_master>=threshold;
        assert(bridge_periodic_apu_flush_due(&g_cpu)==expected);
        rtl_apu_pace_check();
        assert(syncs==(unsigned)(absolute && expected));
        assert(locks==unlocks && lock_depth==0); tests++;
    }
    reset(); g_cpu.master_cycles=8192;
    cpu_instruction_timing_commit(&g_cpu);
    assert(beam==g_cpu.master_cycles && g_cpu.coprocessor_master_cycles==g_cpu.master_cycles);
    assert(beam_commits==1 && cart_commits==1 && syncs==(unsigned)!no_apu);
    if (syncs) assert(sync_masters[0]==sync_beams[0]); tests++;
    reset(); CpuState other={0}; other.master_cycles=8192;
    cpu_instruction_timing_commit(&other); assert(syncs==0); tests++;
    /* Unchanged flush body retains no-APU, relative, mandatory/empty behavior. */
    reset(); timeline=0; s_apu_pending_master=500; g_cpu.master_cycles=beam=500;
    bridge_apu_flush(&g_cpu);
    assert(s_apu_pending_master==0 && relative_calls==(unsigned)!no_apu);
    assert(syncs==(unsigned)!no_apu && locks==unlocks);
    if (!no_apu) assert(snes.apuCatchupCycles==500*kInterpApuPerMaster);
    bridge_apu_flush(&g_cpu); assert(syncs==(unsigned)!no_apu); tests++;
    reset(); g_apu_last_sync_master=UINT64_MAX; g_cpu.master_cycles=9;
    rtl_apu_pace_check(); assert(syncs==0); tests++;
    /* Normal paired RTS/RTL commits only after a real consumed frame. */
    const RecompReturn returns[]={RECOMP_RETURN_NORMAL,RECOMP_RETURN_SKIP_1,
        RECOMP_RETURN_SKIP_2,RECOMP_RETURN_SKIP_3,(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE};
    const uint16 stacks[]={0x1E0,0xFFFE};
    for (unsigned r=0;r<5;r++) for (unsigned pending=0;pending<2;pending++)
    for (unsigned frame=0;frame<5;frame++) for (unsigned pop=0;pop<5;pop++)
    for (unsigned st=0;st<2;st++) {
        reset(); g_cpu.S=stacks[st]; g_cpu.master_cycles=9000; beam=123;
        g_cpu.coprocessor_master_cycles=321; s_refresh_phase=17; g_refresh_charged_upto=9000;
        fake_result=returns[r]; fake_pending=pending; fake_hrv=frame; fake_pop=pop;
        RecompReturn result=cpu_dispatch_pc_paired(&g_cpu,0x871C,(uint8)frame);
        int completed=r==0 && !pending && (frame==2 || frame==3) && pop==frame;
        assert(result==returns[r] && function_calls==1 && log_calls==1);
        assert(g_cpu.S==(uint16)(stacks[st]+pop));
        assert(g_cpu.master_cycles==9048); /* No invented terminal opcode cost. */
        assert(beam_commits==(unsigned)completed && cart_commits==(unsigned)completed);
        assert(syncs==(unsigned)(completed && !no_apu));
        if (completed) {
            assert(beam==9048 && g_cpu.coprocessor_master_cycles==9048);
            assert(s_refresh_phase==65 && g_refresh_charged_upto==9048);
        } else {
            assert(beam==123 && g_cpu.coprocessor_master_cycles==321);
            assert(s_refresh_phase==17 && g_refresh_charged_upto==9000);
        }
        tests++;
    }
    reset(); g_cpu.S=0x1E0; selected_fp=NULL; g_cpu.master_cycles=4000;
    assert(cpu_dispatch_pc_paired(&g_cpu,0x871C,2)==RECOMP_RETURN_NORMAL);
    assert(g_cpu.S==0x1E0 && g_cpu.master_cycles==4000 && beam_commits==0 && syncs==0); tests++;
    reset(); fake_mirror=1; g_cpu.S=0x1E0; g_cpu.master_cycles=9000;
    assert(cpu_dispatch_pc_paired(&g_cpu,0x871C,2)==RECOMP_RETURN_NORMAL);
    assert(lookup_calls==2 && g_cpu.S==0x1E2 && beam_commits==1); tests++;
    /* Authored completed-opcode costs, partitioned into interpreted and precise
     * native segments. Actual next-entry commits plus actual paired final return
     * must yield identical sync master+beam targets and real refresh state.
     * Zero-threshold diagnostic explicitly permits redundant same-clock calls;
     * its per-boundary behavior is covered above, not collapsed for this check. */
    if (!no_apu && threshold>0) for (unsigned split=0;split<31;split++) {
        reset();
        for (unsigned i=0;i<2048;i++) interpreted_complete(6+((i*37)%89)*2);
        capture(); reset();
        for (unsigned i=0;i<2048;) {
            if ((i+split)%7<3) { interpreted_complete(6+((i*37)%89)*2); i++; continue; }
            unsigned count=1+(i+split)%13;
            if (count>2048-i) count=2048-i;
            g_cpu.S=0x1E0;
            for (unsigned j=0;j<count;j++,i++) {
                cpu_instruction_timing_commit(&g_cpu);
                uint64_t cost=6+((i*37)%89)*2;
                if (j+1==count) {
                    fake_cost=cost; fake_hrv=fake_pop=2;
                    assert(cpu_dispatch_pc_paired(&g_cpu,0x871C,2)==RECOMP_RETURN_NORMAL);
                } else g_cpu.master_cycles+=cost;
            }
        }
        compare_capture(); tests++;
    }
    printf("PASS %u actual policy/commit/refresh/return/partition cases; threshold=%" PRIu64 "; noAPU=%d\n",tests,threshold,no_apu);
    return 0;
}
