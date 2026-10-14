# Lagoon Recomp

A standalone Windows executable built on
[snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp), using both
recompiled game routines and the framework's 65816 interpreter fallback.
It is a work in progress, not a completed or 100% ahead-of-time native port.
Mesen is used only as an independent development test reference; the Lagoon
executable does not launch, link, or require Mesen.

No ROM data is distributed. Generate the game code locally from your own ROM.

## Status

The October 14, 2026 build fixes the return to the opening screens after the
old man's dialogue. The regression route now enters Atland, moves the player,
opens the status/item menu with Start, and closes it with B.
Only this opening route has been verified for this fix. The full campaign,
combat, progression, saving/loading, and ending are not qualified.
The existing `dist/lagoon-0.1.0-windows-x86_64.zip` predates this fix; use the
rebuilt `build/LagoonSNESRecomp.exe`.

The reset was caused by an incomplete authorization at the long-indirect
routine dispatcher `$00:80E6`. Its two known targets omitted `$01:807A`, the
main gameplay routine. The generated default case popped the guest call frame
and skipped the routine, allowing `$00:9366` to restart the opening loop.
`recomp/bank00.cfg` now keeps this dispatcher on the interpreter tier, which
executes the live ROM target instead of discarding gameplay calls. Other proven
routines remain recompiled. A clean default generation currently emits 143
AOT variants and 18 LLE variants; these counts are not a gameplay-coverage
percentage, and additional runtime fallback can still occur.

The previously advertised interpreter-floor comparison was not independent:
`tools/regen.sh --cfg-dir recomp-floor` ignored its argument and generated from
`recomp/`. That option is now honored, and `build/lagoon-floor.exe` was rebuilt
from the actual floor configuration.

## Qualification

Evidence for this bounded fix is recorded in `FIX-TITLE-LOOP-2026-10-14.md`.

| Check | Result |
|---|---|
| Original failure | Reproduced; trace reached `$00:9366` and `$00:9304` at frame 4213 |
| Corrected opening route | Atland visible by frame 4657; player movement and menu open/close verified |
| Regression script | `tools/script_gameplay_regression.txt`, 6239 frames, isolated fresh save roots |
| Genuine interpreter floor | Rebuilt from `recomp-floor`; also reaches town and responds to input |
| Gameplay snapshots versus floor | Town/moved/menu/back framebuffer, VRAM, OAM and CGRAM match byte for byte |
| Full route parity | Not exact: 193 differing presentations across 6239 frames; WRAM/register differences remain, including stale stack bytes |
| Save protection | Existing 36 save files copied and SHA-256 verified before diagnosis; tests used separate save roots |

The former claim of zero differing frames and identical state is withdrawn.
Agreement between two builds of this framework is useful for finding
recompilation errors, but does not establish correct SNES behavior. Independent
emulator comparisons are development evidence, not a dependency of the port.
## ROM identity

| | |
|---|---|
| File | `Lagoon (USA).sfc` |
| Publisher | — |
| Developer | — |
| Year | — |
| Mapping | lorom |
| Region | USA (North America) |
| Coprocessor | none |
| CRC32 | `d2554270` |
| SHA-256 | `46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb` |

`tools/regen.sh` refuses to run against anything else, so a mismatched
revision fails immediately instead of producing subtly wrong output.

## Build

```sh
git submodule update --init --recursive
bash tools/regen.sh --rom /path/to/Lagoon (USA).sfc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The ROM does not have to live in the repository — keeping it on your own
drive is the better habit, and `SNESRECOMP_ROM` sets the path once for a
shell. Without either, `tools/regen.sh` finds `Lagoon (USA).sfc` at the repo root,
then the path the setup wizard recorded in `rom.cfg`; `.gitignore` blocks
both from ever being committed.

`tools/regen.sh` verifies the ROM, generates `src/gen/*.c`, and re-syncs
`recomp/funcs.h`. Re-run it whenever you change anything under `recomp/`.

## Run

```sh
./build/LagoonSNESRecomp                     # launcher picks the ROM
./build/LagoonSNESRecomp /path/to/Lagoon (USA).sfc # or name it and skip the launcher
```

With no ROM on the command line the build opens the recomp-ui launcher: a ROM
picker with a verification badge, plus display / audio / input settings. Your
choice is cached in `rom.cfg` beside the executable, so the next launch opens
on it and "Skip launcher on boot" makes it immediate. If the launcher is not
built in (`--no-recomp-ui`), the host falls back to a native file picker.

Either way the ROM is checked against the digests above before anything boots
— a dump that could not have produced this build is refused at the door rather
than mis-executing ten frames in.

## Quality-of-life options

The nine changes the owner asked for are tracked in
[QOL-ADDRESSES.md](QOL-ADDRESSES.md), which records for each one the address it lives at
and how that address was proved. They ship through the framework's mod catalog as the
built-in package `lagoon.quality`
(`mods/preloaded/packages/lagoon.quality/1.0.0/manifest.toml`), every one of them
**default off**, so the faithful game remains the default. A feature is a static plugin
compiled into the executable plus data in the manifest — behaviour cannot live in the
`.snesmod` archive — and it must be a RAM write or a ROM *data* patch, because an
*instruction* patch would be silently undone by a later ahead-of-time promotion of the
routine that holds it.

Three of the nine are shipped and measured; six are not shipped yet.

| feature | state |
|---|---|
| Walk speed 3 → 5 px/frame | **shipped** — replaces the 16-byte tier-2 movement row at `$02:8149`. Off: +9 px per 7-frame interval. On: +15, on all eight intervals of the walk route. |
| Immediate camera following | **shipped** — writes the engine's own camera `$0480/$0482` at the frame boundary, clamped to the room. Off: screen x 112 while the player drifts to screen y 179. On: screen x 128, the field centre, y held at 105–115. |
| Start at 18 HP | **shipped** — adds eight to the per-level maximum-HP curve at `$01:E140`, which `$01:957F` loads into `$0370` using Nasir's level `$052C`. Off: `$7E:0520 = 10`. On: `18`, MP/STR/DEF/GOLD untouched. Level 2 becomes 25 where it was 17, so every delta is preserved. |
| 11 base defence | not shipped — the curve covers HP and MP only, and no `00 05 …` defence curve exists near it, so the source of the starting 5 is still untraced |
| Sword reach 28,17,28,27 → 38,38,38,38 | not shipped — those four values are not present in the ROM as bytes or words; combat is now reachable, but a landed hit is still needed to identify the reach |
| Follower speed 1 → 3 px/frame | not shipped — needs a route that puts a follower on the overworld |
| +8 recovery frames after an ordinary hit | not shipped — `$02:81CB` is the engine's own "start a protection period" decision (`$0501` bit `$20`, timer `$0516/$0517`); which field carries the length is not yet proved |
| Enemy and boss EXP +50%, rounded down | not shipped — needs a victory route |
| 8-frame hold at the original jump peak | not shipped |
| Raise all magic power for the end game | not shipped |

With every feature off, a 6 245-frame route replays bit-identically to the
interpreter-floor build on every captured component *including* `master_clock` — the gate
under Qualification — so "the unmodified game is unchanged" is a measurement, not an
assumption.

To turn a feature on in a built tree: set `enabled = true` for it in
`build/mods/preloaded/state.toml` and start the game. The launcher's mod list is meant to
be the player-facing route for this, and has not been qualified yet, so the file is the
documented method for now.

**Development hook, not a player feature.** `debug_position` (off by default) moves Nasir to a
world position at a chosen frame when `LAGOON_DEBUG_AT="x,y[,frame];..."` is set, and writes
nothing at all when it is not. It exists because route work is a positioning problem: one
tile off from a door or an NPC changes the outcome, and scripted presses land on whatever the
collision grid allows. With the rebuilt port it took the route from Atland to a fight in Gold
Cave down to about 45 seconds per experiment. `CAMPAIGN-ROUTE.md` documents how to use it and
what it cannot do.

## Layout

| Path | What lives there |
|---|---|
| `recomp/` | Analysis input: `bank*.cfg`, `symbols.toml`, generated `funcs.h` |
| `rom_identity.txt` | ROM digests — read by the build, `tools/regen.sh` and CI |
| `src/` | Host code you own: `main.c`, `game_rtl.c` |
| `src/gen/` | Generated C. Never committed — regenerate locally |
| `snesrecomp/` | Framework submodule (owns `lib/recomp-net`, `lib/retcomm-rbengine`) |
| `tools/` | `regen.sh` (ROM → C), the headless rig, and the qualification tools below |
| `recomp-floor/` | Analysis config for the interpreter-floor reference build |
| `scripts/` | `package_release.sh` — player-facing zip |
| `framework_pins.txt` | Exact framework commits this project was scaffolded against |

### The day-to-day loop

```sh
# 1. Drive a route and record what the interpreter is still doing.
./tools/headless.ps1 -Frames 2900 -Script tools/script_move.txt -Tag cov -Coverage -CleanSaves

# 2. Read the capture: what falls back, with the targets actually observed.
python tools/coverage_report.py build/coverage-cov
python tools/dispatch_hints.py  build/coverage-cov     # the cfg lines the evidence supports

# 3. Write it down in recomp/, regenerate.
bash tools/regen.sh --cfg-roots --profile <capture.json> --profile <capture.jsonl> \
                    --historical-profile <older.json> --historical-profile <older.jsonl>
cmake --build build -j

# 4. Prove the promotion changed nothing.
./tools/replay_parity.ps1 -Script tools/script_move.txt -Frames 2760 -Dumps -Tag mv
```

`tools/find_refs.py` answers "who transfers here, and with what calling
convention", which is how a dispatch's `return:`/`frame:` semantics get decided
instead of guessed. `tools/bin_diff.py` names the addresses where two snapshots
disagree, when a framedump comparison says "different" but not "where". A capture
from an older build never merges with a newer one — costs and build identity do
not cross a build boundary — so it goes in as `--historical-profile`, which
contributes evidence without letting a stale build shrink the program.

## Porting from here

The scaffold stops where the game-specific work starts. In rough order:

1. **Make it boot.** `src/game_rtl.c` holds the frame driver. It starts on
   the framework's beam-aligned driver (`snesrecomp/runner/src/beam_frame_driver.h`):
   one frame per PPU field, interrupts taken where the beam latches them,
   the field rasterized from the raster journal. A title whose main loop it
   does not fit replaces those calls with its own driver.
2. **Name things.** Add entries to `recomp/symbols.toml` as you identify
   routines, then re-run `tools/regen.sh`. Set `emit = true` to promote one
   into ahead-of-time analysis; leave it false to keep it interpreted.
   Regen synchronizes the marked blocks in each bank cfg, creating missing
   configs. Only proven variants become AOT code. See `recomp/README.md`.
3. **Resolve dispatch misses.** After every run, deal with unresolved
   indirect targets before anything else — they are the reason a port
   diverges, and they are cheap to fix early.
4. **Never synthesise a result** to get past uncovered code, and never edit
   `src/gen/` by hand. Fix the config or the framework and regenerate.

## Multiplayer

Single player.

## License

This project's own source is under the license in `LICENSE`. The framework
carries its own terms — see `snesrecomp/LICENSE` and
`snesrecomp/THIRD_PARTY_ATTRIBUTION.md`. Neither covers the game data, which
is not distributed here.
