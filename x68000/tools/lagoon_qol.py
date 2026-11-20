"""Guest-code changes for the pinned English X68000 program."""

import struct


class Code:
    """Emit the small 68000 routines with their local branches and HU relocations."""

    def __init__(self, start):
        self.start = start
        self.data = bytearray()
        self.labels = {}
        self.branches = []
        self.addresses = []

    def emit(self, words):
        self.data.extend(bytes.fromhex(words))

    def mark(self, name):
        self.labels[name] = self.start + len(self.data)

    def branch(self, opcode, target):
        self.emit(opcode)
        self.branches.append((len(self.data), target))
        self.emit("0000")

    def address(self, opcode, target):
        self.emit(opcode)
        self.addresses.append((len(self.data), target))
        self.emit("00000000")

    def finish(self):
        for index, target in self.branches:
            struct.pack_into(">h", self.data, index, self.labels[target] - self.start - index)
        for index, target in self.addresses:
            value = self.labels[target] if isinstance(target, str) else target
            struct.pack_into(">I", self.data, index, value)
        return self.data, {self.start + index: 4 for index, _ in self.addresses}


def center_axis(code, axis, coordinate, lower, upper, origin, backward, forward):
    code.mark(f"camera_{axis}")
    code.emit("4a47")                      # skip stationary updates
    code.branch("6700", f"camera_{axis}_done")
    code.emit(coordinate)
    code.emit("0440 0080")                 # desired origin = player - 128
    code.emit(f"b06c {lower:04x}")
    code.branch("6c00", f"camera_{axis}_upper")
    code.emit(f"302c {lower:04x}")
    code.branch("6000", f"camera_{axis}_delta")
    code.mark(f"camera_{axis}_upper")
    code.emit(f"b06c {upper:04x}")
    code.branch("6f00", f"camera_{axis}_delta")
    code.emit(f"302c {upper:04x}")
    code.mark(f"camera_{axis}_delta")
    code.emit(origin)
    code.branch("6700", f"camera_{axis}_done")
    code.branch("6d00", f"camera_{axis}_backward")
    for direction, target in (("forward", forward), ("backward", backward)):
        if direction == "backward":
            code.mark(f"camera_{axis}_backward")
            code.emit("4440")
        code.emit("b047")                  # advance at most one original motion step
        code.branch("6400", f"camera_{axis}_{direction}_scroll")
        code.emit("3e00")                  # stop exactly at the target
        code.mark(f"camera_{axis}_{direction}_scroll")
        code.address("4ef9", target)       # original streaming and scroll update
    code.mark(f"camera_{axis}_done")
    code.emit("4e75")


def build_code(start, adventure=False, expedition=False):
    code = Code(start)

    code.mark("sword")
    code.emit("4a40")                      # tst.w d0, result of the original contact
    code.branch("6600", "hit")
    code.emit("0c29 0004 0007")            # cmpi.b #4,7(a1), enemy only
    code.branch("6600", "miss")
    code.emit("0c28 0002 000e")            # cmpi.b #2,14(a0), attack state
    code.branch("6600", "miss")
    code.address("4a39", 0x4D3C)          # tst.b equipped sword
    code.branch("6700", "miss")
    code.address("4a39", 0x11D2B)         # tst.b attack disabled
    code.branch("6600", "miss")
    code.emit("0c45 00f9")                 # cmpi.w #249,d5, retain byte-sized step
    code.branch("6200", "miss")
    if adventure:
        code.emit("48e7 7c30 3f07 7e06")   # save d1-d5/a2-a3,d7; seven samples
        code.emit("47e8 0008 0828 0001 000c")  # lea x extent,a3; horizontal facing?
        code.branch("6600", "horizontal")
        code.emit("3011 9050 3229 0008")    # vertical attack: compare transverse x
        code.branch("6000", "side_overlap")
        code.mark("horizontal")
        code.emit("548b 3029 0002 9068 0002 3229 000a")  # compare transverse y
        code.mark("side_overlap")
        code.emit("3f13 d253 4a40")        # save original extent and find total width
        code.branch("6a00", "side_gap")
        code.emit("4440")                 # absolute transverse gap
        code.mark("side_gap")
        code.emit("b041")
        code.branch("6200", "widen")       # target is outside the original blade width
        code.emit("5245 7e05")             # preserve straight reach in narrow passages
        code.branch("6000", "project")
        code.mark("widen")
        code.emit("5453")                 # add two pixels on both sides
        code.mark("project")
    else:
        code.emit("48e7 7c20 3f07 7e05")   # save d1-d5/a2,d7; six samples

    # Skip distant actors before doing the per-pixel terrain checks.
    for axis, gap, extent in (
        ("x", "3011 9050", "3229 0008 d268 0008"),
        ("y", "3029 0002 9068 0002", "3229 000a d268 000a"),
    ):
        code.emit(gap)                    # move target coordinate to d0; subtract player
        code.branch("6a00", f"{axis}_gap")
        code.emit("4440")                 # neg.w d0
        code.mark(f"{axis}_gap")
        code.emit(extent)                 # move target extent to d1; add player extent
        code.emit("d245 5c41 b041")       # add step and six pixels; cmp.w d1,d0
        code.branch("6200", "blocked")

    code.mark("sample")
    if not adventure:
        code.emit("5245")                 # earlier profile starts one pixel out
    code.emit("48e7 0440")                 # save d5/a1
    code.emit("1f28 000d 1145 000d")       # save motion step; use projected step
    code.address("4eb9", 0xF47E)          # original terrain check
    code.emit("115f 000d 4cdf 0220")       # restore motion step and d5/a1
    code.emit("0c40 0001")                 # reject blocking terrain
    code.branch("6700", "blocked")
    code.emit("0c40 000d")
    code.branch("6400", "blocked")
    code.address("45f9", 0xF338)          # lea original facing displacement,a2
    code.emit("3012 4eb2 0000 4a40")       # call the original direction check
    code.branch("6600", "restore")
    if adventure:
        code.emit("5245")                 # include lateral cleave on the original plane
    code.branch("51cf", "sample")          # dbra d7,sample

    code.mark("blocked")
    code.emit("7000")                      # moveq #0,d0
    code.mark("restore")
    if adventure:
        code.emit("369f 3e1f 4cdf 0c3e")   # restore transverse extent, d7 and d1-d5/a2-a3
    else:
        code.emit("3e1f 4cdf 043e")
    code.emit("4a40")                     # tst.w d0
    code.branch("6600", "hit")
    code.mark("miss")
    code.address("4ef9", 0xF340)
    code.mark("hit")
    code.address("4ef9", 0xEFE0)

    code.mark("hit_player")
    code.address("b3fc", 0x402E)          # cmpa.l #player,a1
    code.branch("6600", "take_damage")
    code.address("4a39", "recovery")
    code.branch("6600", "skip_damage")
    code.mark("take_damage")
    code.address("4eb9", 0x1200)          # original HP and hit flash
    code.address("4eb9", 0xF1AE)          # original knockback and hurt animation
    code.address("b3fc", 0x402E)
    code.branch("6600", "hit_sound")
    code.address("13fc 0010", "recovery") # move.b #16,recovery
    code.mark("hit_sound")
    code.address("4ef9", 0xF1A6)
    code.mark("skip_damage")
    code.emit("4e75")

    code.mark("world_tick")
    code.address("4a39", "recovery")
    code.branch("6700", "update_actors")
    code.address("5339", "recovery")      # subq.b #1,recovery
    code.mark("update_actors")
    code.address("4eb9", 0xEED8)
    code.address("4ef9", 0xFB0A)          # original second update, tail call

    code.mark("init_player")
    code.address("4239", "recovery")
    if adventure:
        code.address("4239", "jump_hold")
    code.emit("117c 0003 000d 4e75")       # move.b #3,13(a0); rts

    if adventure:
        code.mark("magic_damage")
        code.emit("e441 9041")             # original quarter-defense subtraction
        code.branch("6f00", "magic_zero")
        code.emit("3f02 3400 5242 e24a d042")  # save d2; add half, rounded up
        code.branch("6800", "magic_return")
        code.emit("303c 7fff")             # saturate positive signed damage
        code.mark("magic_return")
        code.emit("341f 4e75")
        code.mark("magic_zero")
        code.emit("7000 4e75")

        code.mark("jump_start")
        code.emit("4212 4a28 000e")        # original clear and action-state test
        code.branch("6600", "jump_start_return")
        code.address("4239", "jump_hold")
        code.mark("jump_start_return")
        code.emit("4e75")

        code.mark("jump_tick")
        code.emit("5243 b63c 000c")        # advance jump age to the middle pose
        code.branch("6600", "jump_compare")
        code.address("0c39 0008", "jump_hold")
        code.branch("6400", "jump_compare")
        code.address("5239", "jump_hold") # count up to eight additional updates
        code.emit("5543")                 # undo both age increments for this update
        code.mark("jump_compare")
        code.emit("b63c 0017 4e75")        # original comparison with the landing age

    if expedition:
        center_axis(code, "y", "3028 0002", 6, 10, "906c 0002", 0xDF1C, 0xDF44)
        center_axis(code, "x", "3010", 4, 8, "9054", 0xDF6A, 0xDF90)

    code.mark("recovery")
    code.emit("00")
    code.mark("jump_hold")
    code.emit("00")                        # end on a word boundary
    return code


def decode_relocations(data):
    result = {}
    position = offset = 0
    while position < len(data):
        distance, = struct.unpack_from(">H", data, position)
        position += 2
        if distance == 0:
            break
        if distance == 1:
            distance, = struct.unpack_from(">I", data, position)
            position += 4
            size = 4
        else:
            size = 2 if distance & 1 else 4
            distance &= ~1
        offset += distance
        result[offset] = size
    return result


def encode_relocations(relocations):
    result = bytearray()
    previous = 0
    for offset, size in sorted(relocations.items()):
        distance = offset - previous
        if distance > 0xFFFE and size == 4:
            result.extend(struct.pack(">HI", 1, distance))
        else:
            result.extend(struct.pack(">H", distance | (size == 2)))
        previous = offset
    return result


def enhance_program(program, adventure=False, expedition=False):
    # The English file has no BSS or debug trailer. Keep every original text
    # and data address by appending the guest routines after existing data.
    header = bytearray(program[:64])
    text, data, bss, reloc = struct.unpack_from(">4I", header, 12)
    end = text + data
    if bss or len(program) != 64 + end + reloc:
        raise ValueError("Unexpected Human68k layout for the enhanced edition.")
    image = bytearray(program[64:64 + end])
    relocations = decode_relocations(program[64 + end:])
    code = build_code(end, adventure, expedition)
    extension, extra_relocations = code.finish()

    def replace(offset, original, replacement):
        original = bytes.fromhex(original)
        if image[offset:offset + len(original)] != original or len(original) != len(replacement):
            raise ValueError(f"Unexpected English program bytes at offset {offset:05X}.")
        image[offset:offset + len(original)] = replacement

    hooks = (
        (0xF33A, "4a40 6600 fca2", "sword", "4ef9", ""),
        (0xF19E, "4eb9 00001200 6108", "hit_player", "4ef9", "4e71"),
        (0xDE24, "6100 10b2 6100 1ce0", "world_tick", "4eb9", "4e71"),
        (0xFDE4, "117c 0002 000d", "init_player", "4eb9", ""),
    )
    if adventure:
        hooks += (
            (0xF10E, "e441 9041 6c02", "magic_damage", "4ef9", ""),
            (0x11A3C, "4212 4a28 000e", "jump_start", "4eb9", ""),
            (0x11B3A, "5243 b63c 0017", "jump_tick", "4eb9", ""),
        )
    if expedition:
        hooks += (
            (0xDEF4, "3028 0002 906c 0002", "camera_y", "4ef9", "4e71"),
            (0xDF0A, "3010 9054 b07c 0040", "camera_x", "4ef9", "4e71"),
        )
    for site, old, routine, opcode, suffix in hooks:
        replace(site, old, bytes.fromhex(opcode) + code.labels[routine].to_bytes(4, "big") + bytes.fromhex(suffix))
        relocations[site + 2] = 4

    if expedition:
        # Inventory and Equipment hide their plane before these waits.
        # Leave disk, dialogue and boot transitions on the original timer.
        for site, old in ((0x9606, "6100 ff56"), (0x9734, "6100 fe28"),
                          (0x9822, "6100 fd3a"), (0x9A82, "6100 fada")):
            replace(site, old, b"\x61\x00" + struct.pack(">h", 0x5F76 - site - 2))

    replace(0x4DA0, "0a0a", b"\x12\x12")        # starting maximum and current HP
    replace(0x4DAC, "0005", b"\x00\x08")        # starting defense
    replace(0x4E96, "11", b"\x15")              # level two maximum HP
    relocations.update(extra_relocations)
    table = encode_relocations(relocations)
    struct.pack_into(">I", header, 16, data + len(extension))
    struct.pack_into(">I", header, 24, len(table))
    return bytes(header + image + extension + table)
