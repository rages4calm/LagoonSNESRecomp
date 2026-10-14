# Lagoon Recomp

A native recompilation of **Lagoon** (USA), built on
[snesrecomp](https://github.com/RetroPortingToolKit/snesrecomp).

Boots, plays, and takes input. The KEMCO logo, the title screen, and the story
scenes are all rendered by recompiled code, and the whole program replays
byte-for-byte the way the framework's interpreter tier does — see
[Qualification](#qualification) for how that is measured.



> **You must legally own a copy of the game.** No ROM data is distributed with
> this project, in the repository or in any release. The recompiled C is
> generated locally from your own copy and is never committed.

## Status

**A working port.** Lagoon boots from a wiped save card to the KEMCO logo, the
title screen, and the story scenes, and the pad reaches the guest: from an
identical starting state, pressing the d-pad changes the framebuffer, WRAM and
VRAM where the same route with no input leaves them byte-identical.

Two framework bugs stood between the scaffold and that state. Both were fixed
in the framework rather than papered over in this port:

- **`$4210` bit 7 is the vblank window, not a first-reader token.** The
  read-and-clear model starved Lagoon's vblank wait, so the game never left a
  black screen. Framework commit `d56921e`.
- **Bounded interpreter runs ignored the host's master deadline.** Lagoon's SFX
  handler ends in `LDA $023C / BNE` and `$023C` is written by the vblank NMI;
  inside a bounded run the deadline was invisible, so the wait burned the whole
  step cap and re-entered, and the frame driver — the only thing that can
  deliver that NMI — never got control back. The logo froze at the first frame
  that entered the handler. Framework commit `6c9e91a`.

Analysis input lives in `recomp/`: four banks, four hand-written
`indirect_dispatch` authorizations (each commented with the ROM evidence that
decided its shape), and the `[[func]]` boundaries that promotion evidence
supported. The current build emits 213 roots as 344 exact ahead-of-time variants
across banks `00`/`01`/`02`/`0C`, with 51 variants kept as low-level emulation —
mostly the game's own hardware-wait loops, which cost cycles on real hardware
too.

## Qualification

Nothing here is judged by looking at a screenshot; every claim above is a
comparison a tool makes.

| Check | Tool | Result |
|---|---|---|
| Ahead-of-time build replays a route exactly like the interpreter floor | `tools/replay_parity.ps1` | 2 757-frame route and 12 001-frame route, **0 differing presents**, every framedump component identical |
| The pad reaches the guest | `tools/script_play.txt` vs `tools/script_play_ctl.txt` + `tools/dump_compare.py` | pre-input dump identical; after each of the four directions, framebuffer / WRAM / regs / VRAM all differ |
| The frame driver holds 60 fps | host log | `fps=60.01`, `cpu_seconds` 7.31 vs 8.25 on the floor for the same 2 757 frames |
| No indirect transfer is being abandoned | `tools/coverage_report.py` | `unresolved_abandons.total_hits = 0` |
| It runs as a program, not just a harness target | windowed run, real drivers | `video=windows audio=directsound`, device opened, `fps=59.46`, clean exit; the launcher entry point opens and waits |
| Long soak on a wiped card | `tools/script_walk.txt` | 22 195 frames of prologue to the title logo, dumping along the way; the A-mashing variant (`tools/script_deep.txt`, 24 466 frames) never gets the pad, which is a fact about Lagoon's opening, not about the port |

Two habits make those numbers mean anything:

- **A card per run.** Lagoon reads its battery-backed save at boot, so every run
  inherits the last run's saved game. Measured against a baseline taken with
  different SRAM in the slot, an unchanged binary looks like a regression — that
  false alarm cost a full debugging detour. `tools/headless.ps1` now gives each
  run its own card through the host's `--save-root`, and `-CleanSaves` wipes the
  tag's card so a fresh-card route actually starts fresh. Screenshot legs get a
  wiped card of their own too: a photograph of frame N has to begin where the
  long run began, or it is a different game at frame N.
- **A floor reference.** `recomp-floor/bank00.cfg` + `tools/regen.sh --cfg-dir
  recomp-floor` builds `build/lagoon-floor.exe`, the interpreter-only build that
  the ahead-of-time build has to match. Without it, "looks right" is the only
  oracle available. Rebuild it whenever the framework moves, or the comparison
  is against an older engine.

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
