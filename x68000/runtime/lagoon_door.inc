/* Advance normal door blackout waits without wall-clock pacing. The
 * original guest executes every VSync, disk operation and script. */
static uint32_t lagoon_stack_long(const uint8_t *ram, uint32_t address) {
    return (uint32_t)ram[address ^ 1u] << 24 |
        (uint32_t)ram[(address + 1) ^ 1u] << 16 |
        (uint32_t)ram[(address + 2) ^ 1u] << 8 | ram[(address + 3) ^ 1u];
}

static bool lagoon_door_waiting(void) {
    if (!core.get_cpu) return false;
    uint32_t regs[18];
    core.get_cpu(regs);
    if (regs[0] < LAGOON_BASE + 0x5f76 || regs[0] > LAGOON_BASE + 0x5f8a) return false;
    const uint32_t sp = regs[17];
    const uint8_t *ram = core.get_memory_data(RETRO_MEMORY_SYSTEM_RAM);
    const size_t size = core.get_memory_size(RETRO_MEMORY_SYSTEM_RAM);
    if (!ram || (sp & 1u) || sp >= size || size - sp < 80 ||
        size <= LAGOON_BASE + 0x956c || lagoon_byte(ram, 0x71d2) ||
        lagoon_word(ram, 0x955e) != 0x2f00 || lagoon_word(ram, 0x9560) != 0x7028 ||
        lagoon_stack_long(ram, sp) != LAGOON_BASE + 0x9566) return false;
    uint32_t caller = lagoon_stack_long(ram, sp + 8);
    return (caller == LAGOON_BASE + 0x6106 &&
            lagoon_stack_long(ram, sp + 76) == LAGOON_BASE + 0x1334) ||
           (caller == LAGOON_BASE + 0x6a38 &&
            lagoon_stack_long(ram, sp + 72) == LAGOON_BASE + 0x1342);
}
