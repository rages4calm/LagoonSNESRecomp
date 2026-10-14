# QoL feature addresses: what the ROM actually does

Derived 2026-10-14. Every address here was produced by the game's own behaviour,
not by a guess: the port's framedump series identifies variables by how they move,
and a Mesen instruction trace names the code that moves them. Claims say what they
are proven by, and things that are not found yet are listed as not found.

Owner-specified features are the target list; this document is the evidence layer
under them, so the next attempt starts from proof rather than from a disassembler.

## Method, in the order it paid off

1. `tools/dump_series.py --constant` derives the per-interval step of every WRAM
   address from a route that holds one direction, instead of asking whether an
   address matches a number somebody measured. Requiring *every* interval to agree
   is what makes it trustworthy: the right-walk series reported nothing at first
   because the player walked into a wall mid-series, and a looser test would have
   called that a hit.
2. `tools/ram_track.py` then reads the candidate addresses directly across dumps,
   so a candidate becomes a known variable only when it moves when it should and
   holds when it should.
3. `tools/mesen_reference.ps1 -SaveState` then `-LoadState -Trace` produces a
   25-frame instruction trace (5.2 MB) from a state loaded at frame 6 141. That is
   how the ROM's own routine was named without tracing 180 million instructions.
4. `tools/dis.py` reads the routine the trace pointed at.

**Two limitations of `mesen-agent` v0.1.0, measured not assumed** (`tools/mesen_cbtest.lua`,
`tools/mesen_probe.lua`): `emu.addMemoryCallback` registers successfully and then
never fires (`write=0 read=0` over 600 frames against an address the dumps prove is
rewritten every other frame), and `--breakpoint` never interrupts (`codeBreak` fires
only at the end of the run). The API namespace is `emu`, there is no `memory` table,
and `emu.getState()` returns flat dotted keys (`cpu.pc`), not nested tables. Event
callbacks (`endFrame`) do work. So this reference core can *trace* and *read*, but
cannot *watch*.

## Movement: the owner's numbers are the ROM's numbers

Entity records start at `$7E:0500` and are indexed by an entity number in X:

| offset | meaning | proven by |
|---|---|---|
| `$0500,x` | entity state/flags (`AND #$E7` at `$01:A4DE`) | trace + disassembly |
| `$0502,x` | X position, integer pixels | stepped +2304 per 7-frame press interval when pressing right, held constant when pressing down |
| `$0504,x` | Y position, integer pixels | +2304 going down, −2304 going up |
| `$0506,x` / `$0508,x` | velocity X / Y, signed 16-bit | written by `$02:8106` / `$02:8112`, zeroed by `$02:811C` / `$02:8122` |
| `$050B,x` | direction index | read by `$02:80EA`, `BMI $811C` sends it to "no velocity" |
| `$051A,x` | speed-tier selector | read at `$02:80EF`, shifted `<<4`, used as a table stride |
| `$050C,x`, `$050E,x` | camera X / Y (candidates) | stored by the same routine at `$01:A5B3`, `$01:A5BE`; see the caveat below |

Velocity is not an immediate. `$02:80EA` computes `direction*4 + tier*16` and reads
a four-byte `(vx_lo, vx_hi, vy_lo, vy_hi)` record from the table at `$02:8129`
(ROM file offset `0x10129`):

| tier | right | down | left | up |
|---|---|---|---|---|
| 0 | +1 | +1 | −1 | −1 |
| 1 | +2 | +2 | −2 | −2 |
| 2 | **+3** | **+3** | **−3** | **−3** |
| 3 | +4 | +4 | −4 | −4 |
| 4 | **+5** | **+5** | **−5** | **−5** |
| 5 | +6 | +6 | −6 | −6 |

Nasir walks on tier 2 and the position advanced 9 px per 7-frame interval in which
the button was held 6 frames: 1.5 px/frame average, which is 3 px per *applied*
frame, because movement is applied every other frame. That is the owner's
"3 px/frame" confirmed by the ROM's own arithmetic. Tier 0 is 1 px, which is the
owner's follower number, so follower and player differ only by the selector byte.

**Walking speed 3 → 5 px/frame.** Two ways, both AOT-safe: write `$051A` for the
player entity (tier 4 already contains 5) from a frame callback, or patch the tier-2
row of the table. Neither touches an instruction, so neither collides with a future
promotion of `$02:80EA`.
**Follower 1 → 3 px/frame.** Same mechanism, one tier step. Still to do: the
follower entity's index — there is no follower entity moving at 1 px/frame in the
town dumps, because the party does not walk through town, so this needs an
overworld route.

**Camera centring.** `$050C/050E` are written by the routine around `$01:A5B3`, and
the dumps show the behaviour the owner described: while X went 137→149 px the camera
only crept 122→130 and then dithered ±1 px around 133.5, i.e. it scrolls on a border
trigger rather than following. The border comparison itself has not been read yet
(`$01:A56A`–`$01:A624` is the range), so the mod is specified but not yet implemented,
and `$050C/050E` are labelled candidates until that range is read.

## Shipped: `lagoon.quality` / `walk_speed`

The first feature through the whole pipeline, because a mechanism proven once
turns the other eight into repetitions.

- Package `mods/preloaded/packages/lagoon.quality/1.0.0/manifest.toml`, feature
  `walk_speed`, `default_enabled = false`, plugin id `lagoon.qol.walk_speed`
  registered from `src/lagoon_qol.c`. Only implemented features are declared,
  because the runtime refuses an enabled feature whose plugin id the executable
  never registered.
- Implementation is `LagoonQol_OnRomLoaded`, wired through the desktop host's
  game descriptor: it replaces the sixteen bytes of the tier-2 speed row (bus
  `$02:8149`, file `0x10149`) with the ±5 row, but only after comparing the row
  against the stock bytes proven above. A row that does not match is refused with
  a message rather than patched over. ROM *data*, not instructions, so the
  interpreter and any future ahead-of-time code both honour it.
- **Feature off**, regression route against the accepted control: `fb`, `wram`,
  `regs`, `vram`, `oam`, `cgram` and `clock` all `ok` on every one of the eight
  shared tags. Adding the code changed nothing, which is what a default-off
  feature has to mean.
- **Feature on**, same walk route: Y advances **+15 px per 7-frame interval**
  instead of +9, on every one of the eight intervals — 5 px per applied frame
  against the stock 3. That is the owner's requested number, measured in the
  guest's own state rather than asserted.
- Correction found on the way: entity positions are 16-bit pixel coordinates at
  `$0502`/`$0504`, not 8.8 fixed-point. The fixed-point reading was this project's
  own off-by-one (`$0501` for `$0502`) looking at the same variable; `$01:A4EB`
  adds a 16-bit velocity with `REP #$20`, which is only consistent with plain
  pixels. `tools/ram_track.py` now prints raw unless `--fixed` is asked for.
- Not yet verified through a launcher screen: the bundled launcher's Mods page
  should list the package, and that page is being replaced by the Companion
  adapter anyway.

## Starting stats: block found, source not yet

The party screen at game start reads `LV 1 · GOLD 100 · HP 10/10 · MP 3/3 ·
STR 10 · DEF 5`, and those exact values sit in WRAM as 16-bit little-endian words:

```
7E:0520 0A 00   7E:0522 03 00   7E:0524 0A 00   7E:0526 05 00
7E:0528 64 00   7E:052C 01 00
```

`0528`=100 is gold and `052C`=1 is level, so the block is the party record. Which of
`0520`/`0522`/`0524` is HP-current versus HP-max versus STR is *not* resolved: HP and
STR are both 10, so the values alone cannot separate them, and it needs a
differential (take damage, cast magic) rather than another look.

The owner's "18 HP and 11 base defense" therefore has stock counterparts 10 and 5,
which matches the screenshot. But the level/stat source is not a contiguous ROM
table: searching the ROM for `10,3,10,5` as bytes, as 16-bit words, and for
`10,10,3,3,10,5` as bytes all returned zero matches, so the starting stats are
assembled by code. Because the reference core cannot watch writes, the next move is
a state-plus-trace window at the moment the save file is created, or a watchpoint in
this project's own interpreter (which is where gameplay runs today, so it would see
the store and name the PC — a framework addition, not a patch).

## Not located yet

- **Sword reach 28,17,28,27.** The sequence is not contiguous in the ROM as bytes or
  as 16-bit words, so the extents are computed or split across sources; the owner's
  own asymmetry (down = 17 against ~28 for the others) suggests two sources and must
  be confirmed rather than patched as one four-byte table. Needs a route that swings
  the sword, then a dump series.
- **Recovery frames (+8 after an ordinary hit)** — needs a damage differential, so a
  combat route comes first.
- **Enemy and boss EXP (+50%, rounded down)** — needs the per-enemy EXP table; a
  battle is required to see it read.
- **Jump peak hold (8 frames)** — code change, so it wants the vertical-velocity
  writer identified the same way movement was.
- **All magic power raised** — needs the magic power table.
- **Follower speed** — mechanism known, entity index unknown (overworld route needed).

## Tools added for this

| tool | purpose |
|---|---|
| `tools/rom_scan.py` | search ROM or any dump for byte/word sequences, reported at LoROM bus addresses |
| `tools/dump_series.py` | find addresses by fixed rate (`--delta`) or derive the rate the game actually uses (`--constant`), with `--only`/`--held`/`--skip` |
| `tools/ram_track.py` | print chosen addresses across a dump series, raw and as 8.8 fixed, with per-interval deltas |
| `tools/wram_peek.py` | hexdump a window of any dump, flagging bytes equal to values you already know |
| `tools/mesen_watch.lua` | RAM write watch through the agent's Lua (honest about an unsupported API) |
| `tools/mesen_probe.lua` | list what the agent's Lua environment really exposes |
| `tools/mesen_cbtest.lua` | decide whether callbacks fire at all, and report `getState`'s shape |
| `mesen_reference.ps1` | now passes `-Breakpoint`, `-Lua`, `-SaveState`, `-LoadState`, `-Trace`; `-MaxInputs -1` means no input |
| `real_input_probe.ps1` | audio now defaults to silent: a window is unavoidable for real key delivery, a noise is not |

Route files: `tools/script_walkhunt.txt` (down), `_R` (right), `_L` (left then up),
`_D` and `_E` (clear the corridor, then right) — the spawn sits in a vertical
corridor, which is why the first right-walk attempt showed no movement at all.
