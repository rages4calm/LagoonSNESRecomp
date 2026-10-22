#include "lagoon_native_iterator52.h"
#include "snes/interp_bridge.h"
#include "cpu_trace.h"
#include <string.h>

static struct {
    const CpuState *cpu;
    const uint8_t *ram;
    uint64_t generation;
    uint16_t sp, x;
    uint8_t stack[9], phase;
} witness;
static struct { uint64_t armed, runs, instructions, inactive, active, complete,
                         deadlines, refused; } stats;
int LagoonNativeIterator52_Source(const uint8_t *rom, size_t size) {
    if (!rom || size != 0x100000) return 0;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned i = 0; i < 0x98; ++i)
        hash = (hash ^ rom[0xc643 + i]) * UINT64_C(1099511628211);
    return hash == UINT64_C(0x8fb341abfbe88a3a);
}
static int mode(const CpuState *c) {
    return c && c->ram && c->PB == 1 && c->DB == 1 && !c->D &&
        !c->emulation && !c->x_flag && !c->_flag_D && c->_flag_I &&
        (c->P & 0x3c) == ((c->m_flag ? 0x20 : 0) | 4) &&
        c->S >= 0x100 && c->S <= 0x1ff6;
}
static int live(const CpuState *c, uint64_t generation) {
    return witness.phase && witness.cpu == c && witness.ram == c->ram &&
        witness.generation == generation && witness.sp == c->S &&
        !memcmp(witness.stack, c->ram + c->S + 1, 9);
}
static void nz(CpuState *c, unsigned v, unsigned sign) {
    c->_flag_Z = !v; c->_flag_N = !!(v & sign); cpu_mirrors_to_p(c);
}
static void a(CpuState *c, unsigned value) {
    if (c->m_flag) { c->A = (c->A & 0xff00) | (value & 255); nz(c, value & 255, 128); }
    else { c->A = value & 65535; nz(c, c->A, 32768); }
}

int LagoonNativeIterator52_Process(const uint8_t *rom, size_t size, CpuState *c,
    const CpuState *current, uint64_t generation, uint64_t frame,
    uint32_t pc, uint32_t *landing) {
    if (pc == 0x01c68f) memset(&witness, 0, sizeof witness);
    if (!landing || c != current || generation != frame || !mode(c) ||
        !LagoonNativeIterator52_Source(rom, size)) {
        witness.phase = 0; ++stats.refused; return 0;
    }
    if (pc == 0x01c68f) {
        if (!c->m_flag || c->S < 0x102) return 0;
        witness.cpu = c; witness.ram = c->ram; witness.generation = generation;
        witness.sp = c->S - 2; witness.phase = 1;
        witness.stack[0] = 0x91; witness.stack[1] = 0xc6;
        memcpy(witness.stack + 2, c->ram + c->S + 1, 7);
        return 0; /* The real original JSR still pushes its own frame. */
    }
    if (!live(c, generation)) { witness.phase = 0; ++stats.refused; return 0; }
    if (pc == 0x01c695) {
        if (witness.phase != 1 || !c->m_flag) { witness.phase = 0; return 0; }
        witness.phase = 2; witness.x = 0; ++stats.armed;
        return 0; /* Original LDX0/STX048C remain original instructions. */
    }
    if ((pc != 0x01c69b && pc != 0x01c6af) || !c->m_flag ||
        c->X != witness.x || c->X > 0x800 || (c->X & 63) ||
        witness.phase != (pc == 0x01c69b ? 2 : 3)) {
        witness.phase = 0; ++stats.refused; return 0;
    }
    ++stats.runs;
    cpu_instruction_timing_commit(c);
    for (;;) {
        if (interp_bridge_lle_master_deadline_reached(c)) {
            witness.phase = 0; ++stats.deadlines; *landing = pc; return 1;
        }
        unsigned n = 1, cycles = 2, next = pc + 1;
        /* Fetch bytes are source-owned slow LoROM. Data reads still use the
         * real runtime bus, including its title filters and open-bus value. */
        switch (pc) {
        case 0x01c69b: n=3; cycles=5; next=0x01c69e; break;
        case 0x01c69e: case 0x01c6a0: case 0x01c6a2:
        case 0x01c6af: case 0x01c6ba: case 0x01c6bf: n=2; break;
        case 0x01c6b3: case 0x01c6b7: case 0x01c6bc: n=3; cycles=3; break;
        default: break;
        }
        cpu_trace_block(c, pc);
        c->open_bus = rom[(pc & 32767) + 0x8000 + n - 1];
        switch (pc) {
        case 0x01c69b:
            a(c, cpu_read8_timed(c, 1, 0x500 + c->X)); break;
        case 0x01c69e: a(c, c->A & 0xc0); next=0x01c6a0; break;
        case 0x01c6a0:
            c->_flag_C = (c->A & 255) >= 0xc0;
            nz(c, ((c->A & 255) - 0xc0) & 255, 128); next=0x01c6a2; break;
        case 0x01c6a2:
            cycles = c->_flag_Z ? 2 : 3;
            next = c->_flag_Z ? 0x01c6a4 : 0x01c6af;
            if (!c->_flag_Z) ++stats.inactive;
            break;
        case 0x01c6af: cycles=3; c->P &= ~0x20; cpu_p_to_mirrors(c); next=0x01c6b1; break;
        case 0x01c6b1: a(c, c->X); break;
        case 0x01c6b2: c->_flag_C=0; cpu_mirrors_to_p(c); break;
        case 0x01c6b3: {
            unsigned old=c->A, sum=old+64+c->_flag_C;
            c->_flag_C=sum>65535;
            c->_flag_V=!!((~(old ^ 64) & (old ^ sum)) & 32768);
            a(c,sum); next=0x01c6b6; break;
        }
        case 0x01c6b6: c->X=c->A; nz(c,c->X,32768); break;
        case 0x01c6b7: a(c,c->A & 255); next=0x01c6ba; break;
        case 0x01c6ba: cycles=3; c->P |= 0x20; cpu_p_to_mirrors(c); next=0x01c6bc; break;
        case 0x01c6bc:
            c->_flag_C=c->X>=0x840; nz(c,(c->X-0x840)&65535,32768); next=0x01c6bf; break;
        case 0x01c6bf:
            cycles=c->_flag_Z?2:3; next=c->_flag_Z?0x01c6c1:0x01c69b; break;
        default: /* Only the finite PCs above can be produced. */ return 0;
        }
        c->cycles += cycles; c->master_cycles += cycles*6 + n*2;
        ++stats.instructions; pc=next;
        cpu_instruction_timing_commit(c);
        if (pc == 0x01c6a4) {
            witness.phase=3; witness.x=c->X; ++stats.active; *landing=pc; return 1;
        }
        if (pc == 0x01c6c1) {
            witness.phase=0; ++stats.complete; *landing=pc; return 1;
        }
    }
}

#ifndef LAGOON_NATIVE_ITERATOR52_TEST
#include "common_rtl.h"
#include <stdio.h>
#include <stdlib.h>
static uint64_t frame_generation;
static int enabled=-1, report_enabled;
static void report(void) {
    if (report_enabled) fprintf(stderr,
        "[native-iterator52] armed=%llu runs=%llu instructions=%llu inactive=%llu active=%llu complete=%llu deadlines=%llu refused=%llu enabled=%d\n",
        (unsigned long long)stats.armed,(unsigned long long)stats.runs,
        (unsigned long long)stats.instructions,(unsigned long long)stats.inactive,
        (unsigned long long)stats.active,(unsigned long long)stats.complete,
        (unsigned long long)stats.deadlines,(unsigned long long)stats.refused,enabled);
}
static void hook(CpuState *c, uint32_t pc) {
    if (!enabled || c != &g_cpu || !rtl_apu_absolute_periodic_enabled() ||
        c->master_cycles < g_apu_last_sync_master) return;
    uint32_t landing;
    if (LagoonNativeIterator52_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),
        frame_generation,pc,&landing)) interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeIterator52_BeforeFrame(void) {
    if (enabled < 0) {
        const char *v=getenv("LAGOON_NATIVE_ITERATOR52");
        enabled=!v || !strcmp(v,"auto") || !strcmp(v,"on");
        v=getenv("LAGOON_NATIVE_ITERATOR52_REPORT");report_enabled=v&&!strcmp(v,"1");
        if (report_enabled) atexit(report);
    }
    frame_generation=RtlStateGeneration();
    if (witness.phase && witness.generation != frame_generation) witness.phase=0;
    if (enabled) {
        interp_bridge_add_pre_opcode_hook(0x01c68f,hook);
        interp_bridge_add_pre_opcode_hook(0x01c695,hook);
        interp_bridge_add_pre_opcode_hook(0x01c69b,hook);
        interp_bridge_add_pre_opcode_hook(0x01c6af,hook);
    }
}
#endif
