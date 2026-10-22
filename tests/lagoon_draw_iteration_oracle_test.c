/* Native actor depth-sort and iteration versus original opcodes. The dynamic
 * child executes actual ROM instructions, including every real RTS/JSL/RTL. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(v) do { if (!(v)) { fprintf(stderr, "line %d case %u: %s\n", __LINE__, cases, #v); exit(2); } } while (0)
static uint8 rom[0x100000], ram[0x20000], before[0x20000], expected[0x20000];
static uint8 io[65536], expected_io[65536];
static unsigned cases, completed, interrupted, children, pending, boundary, handler_mask;
static uint64 deadline, bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char *name) { (void)name; CHECK(++g_recomp_stack_top < 16); }
void RecompStackPop(void) { CHECK(g_recomp_stack_top > 0); --g_recomp_stack_top; }
void RecompStackPopYield(void) { RecompStackPop(); }
void WatchdogCheck(void) {}
void cpu_dbg_funcname(const char *name) { (void)name; }
int cpu_take_tailcall_return_context(uint16 *s, uint8 *h) { (void)s; (void)h; return 0; }
void cpu_tailcall_inherit_return_context(uint16 s, uint8 h) { (void)s; (void)h; CHECK(0); }
int interp_bridge_lle_master_deadline_reached(const CpuState *cpu) { return deadline && cpu->master_cycles >= deadline; }
int interp_bridge_lle_unwind_pending(void) { return pending; }
RecompReturn interp_bridge_lle_yield_unwind(CpuState *cpu, uint32 pc) { (void)cpu; pending = 1; boundary = pc; return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE; }
void cpu_instruction_timing_commit(CpuState *cpu) { (void)cpu; }
int cpu_dispatch_has_entry(CpuState *cpu, uint32 pc) { (void)cpu; (void)pc; return 0; }
int interp_bridge_has_direct_paired_bounce(void) { return 0; }
int interp_bridge_in_lle_scheduler(void) { return 1; }
int cpu_resolve_ancestor_skip(uint16 s) { (void)s; CHECK(0); return -1; }
int interp_bridge_return_targets_owner(uint16 s, uint16 post) { (void)s; (void)post; CHECK(0); return 0; }
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *cpu, uint32 pc, uint32 site) { (void)cpu; (void)pc; (void)site; CHECK(0); return RECOMP_RETURN_NORMAL; }
RecompReturn interp_tier_dispatch_popped_return(CpuState *cpu, uint32 pc, uint32 site, uint16 s) { (void)cpu; (void)pc; (void)site; (void)s; CHECK(0); return RECOMP_RETURN_NORMAL; }
RecompReturn cpu_dispatch_pc_from(CpuState *cpu, uint32 pc, uint16 s, uint32 site) { (void)cpu; (void)pc; (void)s; (void)site; CHECK(0); return RECOMP_RETURN_NORMAL; }
RecompReturn cpu_unresolved_abandon_balanced(CpuState *cpu, uint32 pc, uint16 s, uint8 h) { (void)cpu; (void)pc; (void)s; (void)h; CHECK(0); return RECOMP_RETURN_NORMAL; }
uint32_t cpu_region_speed(uint32_t address) { (void)address; return 8; }
static uint8 peek(uint32 address) {
    int32_t offset = cpu_wram_offset(address >> 16, address);
    if (offset >= 0) return ram[offset];
    if ((address >> 16) < 64 && (address & 65535) >= 0x2100 && (address & 65535) < 0x4400) return io[address & 65535];
    if ((address & 65535) >= 32768) return rom[((address >> 16) & 31) * 32768 + (address & 32767)];
    return 0;
}
static void write(uint32 address, uint8 value) {
    int32_t offset = cpu_wram_offset(address >> 16, address);
    if (offset >= 0) { ram[offset] = value; return; }
    if ((address >> 16) >= 64 || (address & 65535) < 0x2100 || (address & 65535) >= 0x4400) return;
    address &= 65535; io[address] = value;
    if (address == 0x4203) { unsigned product = io[0x4202] * value; io[0x4216] = product; io[0x4217] = product >> 8; }
    if (address == 0x4206) { unsigned dividend = io[0x4204] | io[0x4205] << 8; unsigned q = value ? dividend / value : 65535; unsigned r = value ? dividend % value : dividend; io[0x4214] = q; io[0x4215] = q >> 8; io[0x4216] = r; io[0x4217] = r >> 8; }
}
uint8 cpu_read8(CpuState *cpu, uint8 bank, uint16 address) { return cpu->open_bus = peek((uint32)bank << 16 | address); }
void cpu_write8(CpuState *cpu, uint8 bank, uint16 address, uint8 value) { cpu->open_bus = value; write((uint32)bank << 16 | address, value); }
uint16 cpu_read16(CpuState *cpu, uint8 bank, uint16 address) { unsigned lo = cpu_read8(cpu, bank, address); return lo | cpu_read8(cpu, bank, address + 1) << 8; }
void cpu_write16(CpuState *cpu, uint8 bank, uint16 address, uint16 value) { cpu_write8(cpu, bank, address, value); cpu_write8(cpu, bank, address + 1, value >> 8); }
static uint8 read_bus(void *ctx, uint32 address) { (void)ctx; bus_extra += 2; return peek(address); }
static void write_bus(void *ctx, uint32 address, uint8 value) { (void)ctx; bus_extra += 2; write(address, value); }
static Interp816 *make_interp(CpuState *cpu, unsigned pc) {
    Interp816 *interpreter = interp816_init(NULL, read_bus, write_bus); CHECK(interpreter);
    cpu_mirrors_to_p(cpu);
    interpreter->a = cpu->A; interpreter->x = cpu->X; interpreter->y = cpu->Y;
    interpreter->sp = cpu->S; interpreter->dp = cpu->D; interpreter->db = cpu->DB;
    interpreter->k = cpu->PB; interpreter->pc = pc; interpreter->e = cpu->emulation;
    interp816_setFlags(interpreter, cpu->P); return interpreter;
}
static RecompReturn original_child(CpuState *cpu) {
    unsigned sp = cpu->S, operations = 0;
    Interp816 *interpreter = make_interp(cpu, 0xc0aa);
    children++;
    while (interpreter->sp <= sp && !interp_bridge_lle_master_deadline_reached(cpu)) {
        const unsigned handlers[] = {0xc0ca, 0xc1c6, 0xc214, 0xc284, 0xc41b, 0xc44e};
        for (unsigned index = 0; index < 6; index++)
            if (interpreter->k == 1 && interpreter->pc == handlers[index]) handler_mask |= 1u << index;
        if (++operations >= 200000) { fprintf(stderr, "child cap PC %02x%04x S %04x entry %04x\n", interpreter->k, interpreter->pc, interpreter->sp, sp); CHECK(0); }
        bus_extra = 0;
        cpu->coprocessor_master_cycles = cpu->master_cycles;
        unsigned cycles = interp816_runOpcode(interpreter);
        cpu->cycles += cycles; cpu->master_cycles += cycles * 6 + bus_extra;
    }
    cpu->A = interpreter->a; cpu->X = interpreter->x; cpu->Y = interpreter->y;
    cpu->S = interpreter->sp; cpu->D = interpreter->dp; cpu->DB = interpreter->db;
    cpu->PB = interpreter->k; cpu->P = interp816_getFlags(interpreter); cpu_p_to_mirrors(cpu);
    unsigned pc = interpreter->k << 16 | interpreter->pc;
    interp816_free(interpreter);
    if (cpu->S <= sp) return interp_bridge_lle_yield_unwind(cpu, pc);
    CHECK(cpu->S == sp + 2 && pc == 0x01c0a0);
    return RECOMP_RETURN_NORMAL;
}
#include "draw_iteration_under_test.h"
static void put(unsigned address, unsigned value) { ram[address] = value; ram[address + 1] = value >> 8; }
static void compare(unsigned count, unsigned order, unsigned flags, unsigned phase) {
    memset(ram, 0, sizeof ram); memset(io, 0, sizeof io);
    for (unsigned index = 0; index < count; index++) {
        unsigned address = 0xec0 + index * 6;
        put(address, index * 64); put(address + 2, order ? (count - index) * 11 : index * 11);
        /* Inactive types 2/3 return without needing a loaded sprite pack.
         * Other types execute original ROM code under a finite deadline;
         * there is no fabricated pack, NMI or graphics-child completion. */
        ram[address + 4] = phase ? (index + flags) % 6 : 2 + (index & 1);
        ram[0x500 + index * 64] = 0x80; /* Inactive actor still exercises sort and selected return. */
    }
    ram[0x48c] = count;
    CpuState initial = {0}; initial.A = (flags & 1) ? 0x81 : 0x01;
    initial.X = 0x480; initial.Y = 0x7ffe; initial.S = (flags & 2) ? 0x1fd0 : 0x180;
    initial.DB = initial.PB = 1; initial.P = CPU_P_M | CPU_P_I | ((flags & 4) ? CPU_P_C | CPU_P_V : 0);
    initial.ram = ram; initial.host_return_valid = 2; cpu_p_to_mirrors(&initial);
    /* The original main-loop JSR creates the real physical owner frame. */
    Interp816 *caller = make_interp(&initial, 0x80f1);
    CHECK(interp816_runOpcode(caller) == 6 && caller->pc == 0xc026);
    initial.S = caller->sp; interp816_free(caller);
    memcpy(before, ram, sizeof ram);
    deadline = phase == 1 ? 1 : phase == 2 ? 100 : phase == 3 ? 800 : phase == 4 ? 4000 : 0;
    pending = boundary = 0; CpuState actual = initial;
    RecompReturn result = DRAW_ITERATION(&actual);
    cpu_mirrors_to_p(&actual); CHECK(!g_recomp_stack_top);
    CHECK(result == RECOMP_RETURN_NORMAL || (pending && boundary));
    if (pending) interrupted++; else { completed++; CHECK(actual.S == initial.S + 2); }
    memcpy(expected, ram, sizeof ram); memcpy(expected_io, io, sizeof io);
    memcpy(ram, before, sizeof ram); memset(io, 0, sizeof io);
    Interp816 *reference = make_interp(&initial, 0xc026);
    uint64 cycles = 0, last = 0; unsigned operations = 0; bus_extra = 0;
    while (cycles < actual.cycles) { CHECK(++operations < 2000000); last = cycles * 6 + bus_extra; cycles += interp816_runOpcode(reference); }
    unsigned stop = pending ? boundary : 0x0180f4;
    if (memcmp(ram, expected, sizeof ram) || memcmp(io, expected_io, sizeof io) ||
        reference->a != actual.A || reference->x != actual.X || reference->y != actual.Y ||
        reference->sp != actual.S || reference->dp != actual.D || reference->db != actual.DB ||
        reference->k != actual.PB || interp816_getFlags(reference) != actual.P ||
        ((unsigned)reference->k << 16 | reference->pc) != stop ||
        cycles != actual.cycles || cycles * 6 + bus_extra != actual.master_cycles || last != actual.coprocessor_master_cycles) {
        fprintf(stderr, "count %u order %u flags %u phase %u RAM %d IO %d PC %02x%04x/%06x A %04x/%04x X %04x/%04x Y %04x/%04x S %04x/%04x P %02x/%02x cycles %llu/%llu master %llu/%llu copro %llu/%llu\n", count, order, flags, phase, memcmp(ram, expected, sizeof ram), memcmp(io, expected_io, sizeof io), reference->k, reference->pc, stop, reference->a, actual.A, reference->x, actual.X, reference->y, actual.Y, reference->sp, actual.S, interp816_getFlags(reference), actual.P, (unsigned long long)cycles, (unsigned long long)actual.cycles, (unsigned long long)(cycles * 6 + bus_extra), (unsigned long long)actual.master_cycles, (unsigned long long)last, (unsigned long long)actual.coprocessor_master_cycles);
        CHECK(0);
    }
    interp816_free(reference); cases++;
}
int main(int argc, char **argv) {
    CHECK(argc == 2); FILE *file = fopen(argv[1], "rb"); CHECK(file && fread(rom, 1, sizeof rom, file) == sizeof rom); fclose(file);
    const unsigned counts[] = {0, 1, 2, 3, 6, 12, 24, 34};
    for (unsigned index = 0; index < sizeof counts / sizeof *counts; index++)
        for (unsigned order = 0; order < 2; order++)
            for (unsigned flags = 0; flags < 8; flags++)
                for (unsigned phase = 0; phase < 5; phase++) compare(counts[index], order, flags, phase);
    CHECK(completed && interrupted && children && handler_mask == 63);
    printf("PASS draw iteration: %u cases, %u complete / %u deadline, %u actual dynamic child executions, all six authored handler landings; physical original main JSR, full WRAM/IO/registers/flags and CPU/master/copro clocks.\n", cases, completed, interrupted, children);
    return 0;
}
