# Combat and progression fixtures: batch 3

October 15, 2026. These are bounded replay tests, not campaign completion.
The accepted fixtures exercise real sword hits, an enemy death, rewards,
level-up calculations and the equipment menu. Deliberate setup edits are listed
below. No fixture writes an enemy's HP or replaces a kill or reward operation.

Baseline: `build/lagoon-batch2-ready.exe`, SHA256
`5e0d05fbf7f017c201482af48d3454372910f16f46490c6b3cad66e2c63c1911`.
Its retained program manifest is in the workspace's
`work-native-20261015-114641/baseline/src/gen/program_manifest.json`.
Private captures, reports and fresh save roots are under that work directory.
Candidate qualification is recorded separately in the native batch receipt.

## Natural route and correct controls

`script_progression_batch3_natural.txt` reaches town at frame 1750, then walks
north into room 2 at frame 1789. After the transition settles, it walks to
(256,224), right to (352,224), then north to (352,154). Guards at (336,144) and
(368,144) block further movement. The opening story chain and a legitimate
sword purchase are not completed by this route.

`script_progression_batch3_guard.txt` uses X while facing north at that position.
The actual guard dialogue opens. Interaction tests `(053E | 053F) & 40` at
`019B34`, with map flag `DP51 & 02` enabled. Both X and Y satisfy this test.
Earlier A-button probes did not; some also stood beyond the 15-pixel interaction
depth. Private evidence: `progression-guard-01/`.

B starts sword attacks through `053F` bit 80. A starts jumps through `053E`
bit 80. Both actions require map flag `DP51` bit 0, which town's value 82 lacks
and Gold Cave's value 03 includes. Sword collision at `01B5EE` also requires a
nonzero sword selector at `0530` and swing phase `0511 == 2`. The older
`script_combat7.txt` mashed A and did not demonstrate sword combat.

## Frozen combat fixture

`script_progression_batch3_combat_fixture.txt` uses three explicit setup edits:

1. Place Nasir at (352,120), beyond the guards. Walk north 72 frames and wait
   300. The real map transition enters Gold Cave, room 3, at (1408,1564).
2. Set `0530 = 1`, the Short Sword selector. This permits sword collision but
   does not simulate a purchase or recalculate STR: this fixture retains 10 STR.
3. Place Nasir near the first bat, then take one ordinary right step to
   (1448,1440). The bat at entity slot `0BC0` approaches normally.

The script then pulses B for three frames and waits twelve, 48 times. It never
writes HP, EXP, gold, level, enemy state, damage results or reward values.

Baseline `progression-combat-fixture-03/` has 85 snapshots:

| Frame | Bat HP at `0BE0` | Nasir EXP | Nasir gold |
| --- | ---: | ---: | ---: |
| 3186 | 12 | 0 | 100 |
| 3298 | 10 | 0 | 100 |
| 3378 | 8 | 0 | 100 |
| 3442 | 6 | 0 | 100 |
| 3522 | 4 | 0 | 100 |
| 3586 | 2 | 0 | 100 |
| 3650 | 0 | 0 | 100 |
| 3682 | Reward completed | 1 | 102 |

Nasir stays at 10 HP during this accepted fight. This proves a real ordinary
enemy kill and gold/EXP award within the declared fixture. It does not prove a
natural purchase, boss victory or campaign progression. This bat's 1 EXP remains
1 under floor(1.5x), so it cannot measure the EXP QoL increase directly.

## Progression branches

Each variant preserves the actual fight and declares its additional setup:

| Fixture suffix | Additional setup | Actual result after the kill |
| --- | --- | --- |
| `level_fixture.txt` | Seed prior EXP to 9 before combat | EXP 9→10, level 1→2, gold 100→102, STR 10→23, DEF 5→10 |
| `gold_fixture.txt` | Seed gold to FFFE before combat | The ordinary 2-gold award saturates 65534→65535; EXP 0→1 |
| `level_buff1_fixture.txt` | Level setup; maintain status `0533 = 1` just before the reward | Level 2, STR 23, DEF 40 |
| `level_buff2_fixture.txt` | Level setup; maintain status `0533 = 2` just before the reward | Level 2, STR 53, DEF 10 |

All filenames begin `script_progression_batch3_`. Each has 85 snapshots.
The level variant's reward is visible at frame 3684; HP and MP subsequently
regenerate naturally to 17 and 8. The seeded 9 EXP is prior synthetic progress,
not earned EXP. The next actual 1-EXP reward crosses the level threshold.

Buff variants apply `forcepoke 0533` after `combat-29`, during the defeated bat's
death animation. Existing 3 MP lasts through the real award. MP reaches 1 by
frame 3686, then 0; the deliberately forced status remains selected. There are
no MP or HP edits. These fixtures cover status branches, not natural spell casts
or sustained buffs. The extra 30 STR/DEF demonstrates the REP/SEP branch itself.

Private baseline directories are `progression-level-fixture-01/`,
`progression-gold-fixture-01/`, `progression-level-buff1-fixture-01/` and
`progression-level-buff2-fixture-01/`.

## Equipment menu: transient setup, not a permanent black screen

`script_progression_batch3_equipment_verified_fixture.txt` grants only Short
Sword ownership (`04D0 = 40`, item 1), then uses the actual Start menu and Equip
screen. It does not set the sword selector or STR directly.

Baseline `equipment-black-baseline-01/` has seven snapshots:

| Frame | Observed state |
| --- | --- |
| 1849 | Normal Start field menu |
| 1962 | Equip setup, forced blank, INIDISP 128 |
| 2262 | Visible equipment screen, INIDISP 15, STR 10, DEF 5 |
| 2862 | Still visible and responsive after another 600 frames |
| 2957 | Right selected Short Sword: selector 1 and computed STR 20 |
| 3082 | Confirm/exit transition, forced blank |
| 3687 | Normal town view restored; Short Sword and STR 20 retained |

`02BC41` enters forced blank through `0083C4`, loads/draws menu assets, then
calls fade-in `008458` at `02BC62`. Controller polling at `02BC66` follows this
setup. `008458` raises brightness from 0 to 15 using three-frame waits. Earlier
short probes sent inputs while setup was incomplete. No game-code repair is
proposed for those transient captures.

The visible UI and actual selector/STR change qualify equipment recomputation
inside this declared ownership fixture, not a shop purchase. Coverage records
471 M1X0/E0 dispatcher entries each for `0194F7` and `01952F`, from frames 1970
to 2955. The private `equipment-fade-verification.json` records brightness,
pixels and state. Earlier unqualified/duplicate probes were moved with verified
hashes to `work-native-20261015-114641/rejected-probes/`; they are not published.

## Native entry evidence and remaining limits

These call entries were measured M1X0, E0 in the combat fixture:

| Site → target | Hits |
| --- | ---: |
| `019138 → 019170` | 978 |
| `01913B → 0191C2` | 978 |
| `01913E → 0191D5` | 978 |
| `01918B → 0191B4` | 156 |
| `019195 → 0191BB` | 86 |
| `0191A0 → 0191B4` | 46 |
| `01B607 → 01B6BD` | 96 |
| `01B622 → 01B7AE` | 53 |
| `01B9A6 → 01BA52` | 6 |
| `01BAF8 → 019226` | 6 |
| `01B7D4 → 01B919` | 1 |
| `01B7E4 → 01947A` | 1 |
| `01B7F4 → 0194B2` | 1 |

The level and buff fixtures additionally measure `0194E9 → 0194F7` and
`0194ED → 01952F`, M1X0/E0, once each at frame 3672. `01957F` has a recovered
M1X0 entry in Ghidra but no runtime coverage is claimed here.

`019226` includes an exceptional death branch through `0192A0` that uses
PLA/PLA to escape its caller for enemy type 2/mode 24. The bat fixture does not
cover that branch. `01B5EE` and `01B7D4` are not qualified native wrappers merely
because their children were exercised; the integrating agent records their
fallback/timing decisions separately. Preserve interpreter fallback.

`progression_batch3_report.py` summarizes actual snapshots, EXP changes, entity
HP and removed slots. It avoids HP comparisons across room/type/mode changes
and does not equate every removed sprite with a kill. Use `--fixture` to declare
setup and `--measure-after combat-before` to exclude seeded progress from the
earned-EXP test. Full events retain the setup changes.

Native acceptance needs the same route hash and full WRAM/register/clock/graphics
comparison against its retained control. These word summaries and routine counts
alone do not establish parity, save compatibility or campaign completion.

## Frozen qualifier SHA256 hashes

| Route | SHA256 |
| --- | --- |
| Combat | `6276541c021c909cb7c87872e14314f7fef428c57ac7c550d3fa053a6635b1a0` |
| Level | `06ec6fc899e725da7a0a61fc6ce91c90b0b5af8a7e01a0af6099c87d6cc9cd56` |
| Gold | `6830610805d04b0745546d9d0eb9588b5f7d6680158d2c38a531013123f5a875` |
| Buff 1 | `34f105b95efc11db18a12d647cfda49afd1f9e753b0d75f000df778181cca6c8` |
| Buff 2 | `c117961c12dfb5b0f4d06c2181270fb21ca781ad0aef192b517a2953d8d48bed` |
| Equipment | `26425c91cc1852e6249ba4511438c0a904e7d394bf275546d2ab1a7a1ecd8607` |
