# Player damage fixtures: batch 4

October 15, 2026. These bounded fixtures exercise real enemy contact, player
damage, knockback, recovery and death. They do not complete the opening story
or qualify boss damage, revival items, continue/load or the whole campaign.

The retained baseline is `build/lagoon-batch3-ready.exe`, SHA256
`c53cb2d4057cba0be13b198036bb5dbd898d4174b8b987ca5268eb0bc322c15f`.
Its manifest and private captures are retained under the workspace directory
`work-native-20261015-130023`. Runs use fresh private save/config roots.
Candidate native parity is recorded separately by the integration owner.

## Frozen routes and explicit setup

Both routes retain the batch 3 opening and guarded corridor approach. They
place Nasir beyond the guards at (352,120), then walk north 72 frames and wait
300. The game performs its normal transition into Gold Cave, room 3.

`script_damage_batch4_contact.txt` then places Nasir at (1488,1440), takes one
right step to (1491,1440), releases input and waits for the approaching bat.
`script_damage_batch4_vertical.txt` instead uses (1504,1424) and one down step
to (1504,1427). Each captures 360 consecutive frames and a later final state:
397 snapshot tags, ending at frame 3664. Neither route attacks, equips an item
or writes HP, DEF, EXP, gold, enemy state, damage results or recovery counters.

SHA256:

| Route | Hash |
| --- | --- |
| contact | `42407144e6e22cb0b3ffe7e1819ee36cdd57ee1ea64cf4505cdb8b9609b8c147` |
| vertical | `685112fc9ada70bdf73057a198291e2af6a6ad002c752815205f6d412ca73012` |

Run from the source directory, substituting either route and a fresh output:

```powershell
python -P tools/qualify_native.py --exe build/lagoon-batch3-ready.exe --route tools/script_damage_batch4_contact.txt --frames 5000 --out ../work-native-20261015-130023/FRESH-OUTPUT --manifest ../work-native-20261015-130023/baseline/src/gen/program_manifest.json --coverage
python -P tools/damage_batch4_report.py ../work-native-20261015-130023/FRESH-OUTPUT --setup "Guard and player position seeds only; actual enemy contact" --out ../work-native-20261015-130023/FRESH-OUTPUT/damage-report.json
```

## Observed baseline behavior

Accepted contact run: `damage-contact-baseline-02`, all QoL off. Actual HP
decreases are 10 to 5 near frame 3182, 10 to 5 at frame 3277, 8 to 3 at 3337,
6 to 1 at 3397, and 4 to 0 at 3457. Natural regeneration between hits explains
the changing starting HP; the fixture does not restore it. Coverage timestamps
are one frame before the corresponding post-frame dump.

After the first hit, counter `0512` advances from 0 to 6, horizontal velocity
becomes -6, and X moves from 1491 to 1461. At frame 3195 the ordinary hurt flag
clears and velocity is zero. The final capture at frame 3664 has zero HP and
the GAME OVER banner; the screenshot was inspected. This proves player death
and game over, not just a zero-HP value.

Accepted vertical run: `damage-vertical-baseline-01`, all QoL off. HP falls
10 to 5 at frame 3269 and 5 to 0 at 3283. VY reaches -6 at 3273; the map
collision subsequently clips the movement. This route exercises the distinct
vertical direction calculation without changing collision or enemy logic.

## Measured entry modes and promotion limits

All entries below were observed in 65C816 native CPU mode, M1X0/E0. Counts are contact /
vertical. Missing-entry coverage identifies actual interpreter calls, not
proof that compiling each whole routine is safe.

| Site to target | Hits | Meaning |
| --- | ---: | --- |
| `01A378 -> 01B9A6` | 14 / 49 | Contact and eligibility wrapper |
| `01B9AB -> 01B9B0` | 5 / 2 | Hit direction wrapper |
| `01B9C1 -> 01B9C4` | 5 / 0 | Horizontal comparison leaf |
| `01B9C1 -> 01B9DC` | 0 / 2 | Vertical comparison leaf, goto entry |
| `01BA1B -> 01BA6B` | 5 / 2 | Actual damage calculation |
| `01BAF8 -> 019226` | 5 / 2 | HP subtraction and death handling |
| `019266 -> 0192A0` | 1 / 1 | Ordinary player death return only |
| `019291 -> 0192C3` | 1 / 1 | Ordinary player case only |

`0192F1` is a simple stop-velocity leaf reached within the player death path:
four 8-bit stores clear `0506..0509`, then RTS. It has no separate call-gap
count in these captures. `01BA6B` uses the hardware divider twice; native
promotion needs exact MMIO timing. `0192A0` also contains a type-2/mode-24
nonlocal PLA/PLA escape, which these player fixtures do not exercise.
`0192C3` includes a boss inline dispatch table, also unqualified here.

## Recovery option regression found by the real route

The same contact route with only `recovery_time` enabled was captured in
`damage-contact-recovery-01`. The first hurt flag clears at frame 3203,
exactly eight frames after stock. However, the old implementation holds the
shared `0512` counter at 5, which also extends knockback: first-hit final X is
1437 instead of 1461, an unwanted extra 24 pixels. Velocity magnitude remains
-6, but its duration and displacement change. A comparison with every captured
frame is retained in `damage-recovery-comparison.json`.

The source correction leaves WRAM flags, counter and knockback stock and
adds an eight-frame immunity window only to the ordinary hit eligibility
read in `01BA52`. It requires precise call-context qualification shared by
both execution paths, state-generation/reset invalidation, and new runtime
evidence before being described as runtime-qualified. Unit tests cover the
shared read guards and exact eight-frame window.

## Corrected runtime evidence

`build/lagoon-batch4-seven-candidate.exe`, SHA256
`c7c2d8060ed78ed287f6faadc73d2cc21ed0c7ec1a45ee793589ba684a4fc5dd`,
includes the correction. Fresh runs enabled only `recovery_time`:
`damage-contact-corrected-01` and `damage-vertical-corrected-01`, each 397 tags.
Their frozen route hashes match the baseline routes above.

The contact route matches stock at all 397 snapshots for X, Y, VX, VY, HP,
actual flags and hit counter. First-hit X ends at 1461, not the old feature's
1437. Knockback, ordinary hurt flags and counter expiry now follow stock.

The vertical route matches stock through frame 3282, including its original
knockback and map collision. At frame 3283 stock takes another lethal hit;
the corrected run instead has actual flags `00CA`, counter 6 and HP 5. For
eight consecutive samples, frames 3283 through 3290, it avoids another hit
without extending the real hurt flag or counter. Natural regeneration raises
HP to 6. The next real hit occurs at frame 3291, reducing HP to 1; later contact
causes death at 3311. Thus the observed hit eligibility window lasts exactly
eight extra frames while the first-hit movement remains stock.

Per-frame comparisons are retained privately in
`damage-contact-corrected-comparison.json` and
`damage-vertical-corrected-comparison.json`. This qualifies ordinary bat
contact on these two routes. Bosses, revival items, bypassing hazards,
rollback and campaign-wide behavior remain separate coverage requirements.
