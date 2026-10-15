# Native gameplay continuation — October 15, 2026

This records the first accepted batch. [The second batch](NATIVE-BATCH2-2026-10-15.md)
supersedes its pending launcher/QoL/widescreen items and the `01:C579` rejection.
The original timing failures below remain useful evidence, not current status.

Public source: https://github.com/rages4calm/LagoonSNESRecomp

## Accepted changes

The latest upstream SNESRecomp main was fetched at
`a6be9756285f26ccc74539c811d4d23d57d8c33d`. Existing Lagoon compatibility fixes
were reconciled on top, including the isolated save-root option and bounded
interpreter deadline behavior. `patches/lagoon-framework.patch` preserves that
stack and the new timing work against the public upstream pin. A fresh worktree
at that pin accepted the patch and passed the new emitter regression checks.

Four Ghidra-derived entries are now explicitly promoted at M1/X0:

| CPU entry | Native routine |
|---|---|
| `01:A4EB` | Entity movement/airborne integration |
| `01:A56A` | Clamp world position to room boundaries |
| `01:A5AA` | Project entity position to screen and build visibility mask |
| `02:80EA` | Select velocity from direction and speed tier |

These entries use the opt-in list in `recomp/precise-timing.txt`. The emitter
uses an instruction per block for these roots, making deadline suspension
possible at the same boundaries as the interpreter. Static CPU cycles use a
six-master-clock baseline; fetches and actual data/stack transfers add the
addressed bus region's excess over six. Dynamic charges use internal-cycle
cost. Host ABI inspection is not counted as a guest bus transfer.

This is staged adoption for qualified roots, not a claim that this timing
model has been qualified for every opcode, mapper or game. Other roots retain
the prior emitter behavior. Generated C was not hand-edited.

The default game binary no longer links the old pre-boot launcher. `Start
Lagoon.cmd` uses the pinned Companion Launcher 0.2.0 adapter. It verifies the
original ROM and passes existing native settings to the game without moving
saves. The adapter currently exposes paths and Play; broader settings/mod UI
integration remains pending.

## Measured qualification

All gameplay/QoL features were off. Tests used dummy video/audio and private,
fresh save roots. Unthrottled tests use copied configs, with rendering enabled.

| Comparison | Frames | Snapshot tags | Result |
|---|---:|---:|---|
| Four roots vs prior native control, opening/town/menu | 6,239 | 9 | Every component and guest clock identical; zero differing presentations |
| Four roots vs control, existing `script_combat7.txt` | 2,832 | 5 | Every component and guest clock identical; zero differing presentations |
| Four roots vs control, extended movement/menu route | 7,219 | 18 | Every component and guest clock identical; zero differing presentations |
| Default binary without old launcher vs accepted candidate | 7,219 | 18 | Every component and guest clock identical; zero differing presentations |

Compared components: framebuffer, complete WRAM, registers, VRAM, OAM, CGRAM,
vpos/hpos and master clock. Full presentation-stream lengths also match.
The existing combat-named script is a bounded replay, not proof of combat
victory or campaign progression. The extended route genuinely changes the
player's world coordinates in four directions; its jump-named snapshots did
not show airborne state, so they do not qualify airborne branches or jumps.

Interpreter instruction counts, same route and coverage mode:

- Opening: 61,241,252 -> 60,337,436 (903,816 fewer).
- Extended movement: 72,427,712 -> 70,655,953 (1,771,759 fewer, about 2.45%).

These are reductions in interpreted work, not native-execution percentages
or a demonstrated wall-clock speedup. The tree generates 147 AOT-eligible
variants and 18 declared LLE variants. Uncovered variants retain fallback.

Additional checks: five BPS format/corruption checks, two precise-root emitter
checks, fifteen upstream cycle-model checks, launcher two-page smoke rendering,
clean/wrong ROM readiness, structured launch-plan checks and an isolated
120-frame launch through the adapter's native contract all passed.

Before runtime work, 139 pre-existing save/state files were backed up and
SHA256 checked. All 139 remained unchanged after these checks. These include
previous test saves as well as owner data; the count is not 139 player slots.

## Rejected work and remaining limits

The first four-root candidate reduced interpretation but failed guest-clock
and CPU-state parity; its static block timing was replaced before acceptance.
The tile-row staging routine `01:C579` was then retried with instruction-boundary
timing and still failed CPU/clock parity, including on the second replay.
It stays `emit=false`. Do not enable it because its images happen to match.
Its remaining timing issue is not fully localized.

A newly built pure interpreter floor still differs in CPU/WRAM/clock state
from the pre-existing native control. Images match at the compared opening
snapshots, but this batch does not establish whole-port interpreter parity.
The four accepted routines preserve the existing native control exactly on
the checked routes; they do not resolve older roots' timing differences.

`00:80E6` remains forced to the interpreter. Its live ROM target is still
authoritative, preserving the repaired opening-to-gameplay transition. No
incomplete target authorization or guest-stack shortcut was reintroduced.

No campaign, combat victory, save/load, ending, hardware controller or
widescreen qualification is claimed. Optional QoL changes are tracked in
`TODO.md`; they were not enabled or newly delivered in this batch.

## Private evidence and next work

Full receipts, manifests, logs, protected-file hashes, dumps and the patched
QoL ROM are in the workspace's `work-native-20261015-105606/` directory,
outside the public source repository. `qualification-summary.json` records
presentation comparisons and protected-file verification.

Use the original Ghidra handoff for further entry/dispatch discovery. Next,
qualify actual jumping/damage states for movement integration, identify
`01:C579`'s remaining timing mismatch, and expand movement/combat callees in
small measured batches. Keep the interpreter fallback. Use the corrected BPS
diff for the requested QoL changes; preserve AOT/interpreter agreement when
porting instruction changes. Widescreen requires Lagoon-specific camera,
prefill, HUD and sprite handling rather than just an aspect toggle.

GitHub publication is one source batch. No automatic pushes, recurring sync
or playable release is configured by this work.
