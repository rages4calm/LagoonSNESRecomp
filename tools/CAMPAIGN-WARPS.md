# Campaign qualification warps

This is an explicit, private test cheat for reaching later campaign scenes. It
does not earn progression, prove a playthrough, or change the normal game when
its environment variable is absent. The original interpreter safety net remains.

`campaign_rooms.py` reads an external original Lagoon USA ROM and emits metadata
for all75 room records, ordinary doors and static entity records. It verifies
the ROM SHA256. No ROM, graphics or audio assets belong in the source repository.

```powershell
python tools/campaign_rooms.py --rom '../Lagoon (USA).sfc' --out ../private-work/rooms.json
```

Room records are at `$05:B2DF`, stride `$38`, read by `$01:8B84`. The75 ordinary
door pointers start at `$05:8059`. Each11-byte door row contains source cell X/Y
(two words), destination X/Y (two words), destination room, facing and fade flags.
`FFFF` terminates a door list. Source cells are32 pixels wide; Y uses
`(playerY-1)>>5` (`$01:CA05`). Each catalogue room's `incoming` array provides
existing seven-byte ROM destination payload addresses: door row address plus4.
Some script-only scenes have no ordinary incoming door.

## Execution contract

The qualification harness accepts `--campaign-warps <plan>`, freezes the plan in
the private output directory and records its SHA256. Each noncomment plan line
contains an increasing positive host tick and a genuine ROM payload address:

```text
# Natela room41, canonical entrance128,220
50 0582ce
```

The harness sets `LAGOON_CAMPAIGN_WARPS` to the frozen file. The runtime additionally
requires bounded `SNESRECOMP_RUN_FRAMES`, dummy SDL audio, offline execution and
no run-ahead. The plan is host test orchestration and is not saved in savestates.
Do not enable it in ordinary play or use an owner save directory.

An armed entry waits for the game to match a real ordinary door. It does not
teleport immediately at the requested tick. The hook is precisely at `$01:C8B5`,
after original collision, fade setup, source-cell matching and room lookup have
already executed. It requires:

- Native CPU mode, M0/X0, direct page0, PB01/DB01, and the real main-loop JSR
  return `$80A2` on the guest stack.
- An active player, no conflicting player mode and no room load in progress.
- Original DP40..42 equal to the current room's actual door-list pointer,
  Y=`11*n+4`, and that row's coordinates equal to the game's `$102B/$102D` cells.
- The requested address be an existing door payload, with a valid room, facing
  and destination coordinates inside that room's source-defined dimensions.

Only DP40..42 and CPU Y are changed: the destination pointer is substituted and
Y becomes0. The original guest opcode continues. `$01:C8B5..C8E2` consumes the
payload and invokes `$01:8772`, including actor reset, room descriptor selection,
maps, graphics, palettes, entities and fades. Guest stack/control flow is retained.
The original RTS eventually returns through `$01:C8EB` to `$01:80A3`.

The initially tested Atland seed can trigger its real north exit with `press up
96` after arming the plan.24 fields were insufficient in the measured seed. The
first accepted Natela entry then needed roughly230 fields for its full loader.
Wait for actual completion; do not label a pending fade as a settled scene.

Stderr records every applied plan index, requested/actual tick, source/destination
room, payload and stack guard. A final summary reports applied/planned/pending
counts. Bounded development traces remain enabled only for an opted-in plan.
Require all intended entries to apply and inspect post-load room, dimensions,
loading flag, live actors and coherent CPU state before accepting coverage.

## Excluded preliminary experiment

The first implementation redirected directly from `$01:C7F6` to `$01:C8B0`.
Although it consumed the intended descriptor, it skipped ordinary door
preconditions, including `$01:CA05` and fade/collision setup. Its long Natela run
corrupted CPU/graphics state and reached BRK. Those runs are invalid fixture
experiments, excluded from campaign coverage and not evidence of a port defect.

The replacement matched-door implementation successfully entered Natela room41,
completed the original loader and returned through the expected guest stack
without those BRKs. That is bounded entry evidence, not boss or campaign success.

## Production-module guard test

The test includes the actual production source. It exercises genuine descriptor
membership, invalid addresses, CPU/loading/return guards, a real Atland door,
the exact three-byte WRAM write, Y substitution and one-shot consumption. Supply
the original ROM externally and put the executable in a private work directory.

```powershell
. ../env.ps1
& $env:CC -std=c11 -I snesrecomp/runner/src -I src tools/campaign_debug_test.c -o ../private-work/campaign-debug-test.exe
& ../private-work/campaign-debug-test.exe '../Lagoon (USA).sfc'
```

Boss completion bits are source-derived from `$01:B838` through `$01:CD68`:
index=`room-$40`, MSB first at WRAM `$04E0` onward. Other progression flags share
`$04E1..04E6`; inventory ownership uses `$04D0..04D7`. Changing these is additional
explicit cheat setup, which must be recorded independently. A cleared completion
bit alone does not initialize every scripted boss phase.

The harness records each explicit WRAM setup write. Campaign scripts that set a
nonzero player-HP high byte are rejected: the original maximum is 255, and the
discarded 32767-HP fixture broke the HUD's assumptions. DEF 32767 also overflows
the game's combat arithmetic. Accepted protection uses valid HP at the selected
level and a disclosed, bounded defense value. A blocked script or nonzero guest
execution failure cannot be counted as successful coverage even if the host
process exits with code zero.
