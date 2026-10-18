# Core menu transitions

Menu speed is part of the port. Items, Equipment and Magic no longer need an
optional faster-menu feature. The old `lagoon.quality:fast_menus` feature and
activation plugin remain as harmless compatibility aliases for existing catalogs;
the launcher does not expose or change their saved values.

The core implementation has two parts:

- Original brightness writes remain; guarded return frames reduce submenu fade
  waits. Opening uses one field per step; audited closing uses five real waits
  across all fifteen steps, as detailed below.
- A guarded native implementation expands the original compressed resources
  synchronously. The original VRAM upload/DMA code still consumes the result.

Unknown resources, changed instruction/payload bytes, unrecognized stack chains,
wrong CPU modes and ordinary room loads retain the original native/interpreter
path. The original decoder body remains available when the guarded entry refuses.
The synchronous computation intentionally removes guest work and its intervening
blank frames; clock parity with the historical slow menu is not claimed.

## Ownership and refusal

The dictionary implementation is a handwritten transcription of `$00:9143..9303`.
It runs on private WRAM and CPU copies. Bounds, dictionary traversal, termination,
original payload identity, consumed/output sizes and the complete real return
frame must validate before anything is committed. Refusal leaves WRAM, CPU and
the result parameter unchanged. The original final RTL consumes the validated
JSL frame; the original caller then queues and transfers the pixels normally.

Opening resources 6/7/8 are authorized through the original `0094A1` parent and
Items/Magic/Equipment helper frames. Closing resources require the actual world
loader `018874`/`018889` return, its two nested JSR frames, the bank dispatcher
frame and an exact submenu-close caller. Common close resources 9 and 0 have their
own original nested caller guards. Merely being in a menu or forced blank is
insufficient. Ordinary room transitions through the shared loader stay original.

The timer filter also checks original instruction bytes and the complete close
dispatcher chain. All optional gameplay changes can remain off while core menu
timing works. The SDK's `hle_guard_func` keeps the original decoder body/modes
and calls the title guard after its original entry deadline/refresh checks.

## Verification

The pure decoder was compared with the original instructions using the SDK's
65C816 interpreter, poisoned complete WRAM and varied CPU flags, including the
decimal flag. All 128 KiB of WRAM and A/X/Y/S/D/DB/P were compared immediately
before the original RTL, including dictionary scratch and dead hardware-stack
bytes. Guard tests additionally check resource output and genuine return-frame
consumption, plus transactional refusal for invalid contexts.

Private frozen core qualification measured opening from first dimmed field to
full brightness: Items 148 to 40, Magic 146 to 38 and Equipment 147 to 39 fields. All
three close paths improved 199 to 85 fields; forced-blank improved 112 to 56 fields.
These are real menu inputs and original resources in room 01. Remaining 56 blank
fields perform other original world-loader work and remain a profiling target.
Menus are faster, not instantaneous.

There are 75 distinct pinned streams: three opening streams, 70 BG/OBJ streams
observed in the 73-room campaign, and two common close/HUD resources. An independent
original-instruction oracle matched all 75 across four poisoned WRAM/CPU-flag
seeds (300 cases). Unknown streams retain the original native decoder. Guard
tests exercise 225 positive contexts and 1800 transactional refusals. The private
historical non-menu room03 comparison also remains exact across 861 ordered
pictures and four full state/clock samples.

Items' settled submenu matches complete historical WRAM and all graphics
resources; Magic's graphics resources also match, with timer/dead-stack phase
differences. Equipment animates its cursor, so historical samples after different
menu timing are not claimed byte-identical. Final matched-host native qualification
and narrow/wide presentation pairs belong to the batch handoff.

## Closing map and queue work (batch15)

The close loader now performs two bounded RAM computations in native C:

- `02:C5C6` fills the original 32 KiB map and expands both metatile planes, then
  resumes the original RTL at `02:C64D`.
- `00:89EC` builds only the world loader's original 1x1 tile rectangles. The
  original 480-entry queue capacity stays fixed. A full queue resumes at `00:8A97`,
  so the original PHX/PHY/JSL8985 wait, NMI upload and register restoration still
  execute. Other rectangles and callers use the original interpreter.

Both hooks require the original instruction bytes, native CPU widths, binary
arithmetic, direct page 0, the original submenu-close caller/dispatcher frames,
forced blank and valid source bounds. Unknown PCs/frames/modes/data refuse
without changing RAM, CPU or the pending redirect. Map output is transactional;
the fixed tile path validates every bound before its first write and has no
allocation or other fallible work afterward.

Private source qualification measured room 01 Items, Magic and Equipment closing
at 60 fields instead of batch14's 85; town 61 instead of 92; outdoor room 02 60 instead of 85.
Opening stays 40/38/39 fields. Forced blank is 31 fields (town 32). The remaining
loader still spends time on original DMA waits and the broader 0189F5 coordinate
and initialization loop. Reducing that needs a separate parent-loop model;
these hooks do not skip it or speed unrelated room loads.

The independent original-opcode oracle covers 73 room map configurations across
four poisoned RAM/CPU-flag seeds (292 cases), plus 384 tile cases including the
full queue, FFFF sentinel, destination wrapping, carry and overflow. All 128 KiB
of WRAM and architectural registers/flags match at the original RTL/wait seams.
Decimal mode is explicitly refused. The actual full-queue PHX/PHY/JSL wait and
next-field restoration were witnessed at runtime. Instrumentation separately
matched the same updated-core program exactly across 1,762 pictures and six full
state/clock checkpoints; comparing old/new menu timing is intentionally unequal.

Reproduce the ROM-free tests and original-opcode checks with your own clean ROM:

```powershell
./tools/test_core_menus.ps1 -Rom '<path to clean Lagoon USA ROM>' `
  -OutDir '<fresh private output directory>' -Compiler '<path to clang.exe>'
```

The tests contain source addresses, dimensions and expected counts, not ROM
assets or captured game-state payloads. Final integrated build qualification
belongs to the current batch handoff.

## Batch 16 parent computation

Two exact-source/caller guarded helpers now reproduce the RAM-only parent
prefix at 018A10 and suffix at 018A32. Original tile queue calls, full-queue
waits, DMA and final return remain. Audited closing improves from 60 to 52
fields in room 01/room 02 and from 61 to 53 in Atland; openings remain 40/38/39.
The prefix and suffix pass 5,936 original-opcode full-WRAM/CPU/stack cases;
production guards pass 1,622 checks. Actual trace confirms both hooks execute,
with 2,070 original queue calls preserved. The suffix has no separately measured
whole-field gain. See [batch 16](../NATIVE-BATCH16-2026-10-17.md).

## Batch 17 close-fade waits

Audited Items/Magic/Equipment closing now takes 32 fields in room 01 and outdoor
room 02,33 in Atland, instead of 52/53. The 20-field saving preserves the 24/25
forced-blank fields: original resource uploads and queue-full waits still run.
Openings remain 40/38/39 fields. Historical measurements above belong to their
own batches and are not current timing claims.

The exact owned close stack/dispatcher chain and original fade/NMI instruction
bytes authorize a read-only timer view at 0084E3. On intermediate brightness
steps its real 0/1 counter may read as0; the backing byte is never cleared early.
The real stored counter remains 1 until the original NMI decrements it. Values
above1, unknown callers/modes and unrelated NMI reads are never concealed.
All original brightness writes execute, with five real waits in each direction.

A rejected early-zero-store experiment could becomeFF when the original NMI
decrement ran between store and poll. The final read-view implementation avoids
that race. Its oracle executes the original timer and three NMI-counter opcodes,
covering1,920 boundary interleavings and 72 complete fades, including every
brightness write, registers, stack and untouched WRAM. It also witnesses the
zero-store FF regression. This is bounded timer/NMI proof, not a model of the
entire NMI/DMA system. Unknown contexts retain original timing.
