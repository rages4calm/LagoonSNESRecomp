# QoL implementation and qualification

October 15, 2026. All nine player features are optional and default off. The
package is `lagoon.quality`, version 1.0.0. Most features alter guarded data or
shared WRAM writes. Camera centring changes five guarded instruction operands;
the affected camera paths are explicitly interpreter-only and checked against
the generated AOT manifest before every build.

| Feature ID | Implementation |
| --- | --- |
| `walk_speed` | Guarded stock row at 02:8149 remains unchanged; shared WRAM filter turns only Nasir's ordinary tier-two velocity +/-3 into +/-5. Enemy/NPC velocities and hurt knockback stay stock. |
| `sword_reach` | Guarded four signed extent words in rectangles 01:B6F0..B70F become +/-38, exactly the supplied BPS changes. Orthogonal rectangle dimensions remain stock. |
| `follower_speed` | Shared WRAM filter on ally velocity stores: tier-zero +/-1 becomes +/-3 only with the BPS follower signature (04E1 active, entity >=40, state&C0=C0, type0518=1, mode051B=0/1/3, tier051A=0). |
| `recovery_time` | After stock ordinary-hit expiry, eight extra frames reject ordinary hits through a guarded read view at 01:BA52. Actual flags, counter, knockback and control expiry remain stock. |
| `start_hp` | Recognises the freshly cleared new-game party construction, changes current HP10->18 and subsequent DEF5->11. Curves: HP18/21/23 and DEF11/13/16 for levels1/2/3; level3 onward stock. |
| `exp_boost` | All39 enemy and16 boss template EXP words become floor(original*1.5). Zero rewards stay zero. Gold, stats and EXP thresholds remain stock. |
| `jump_hold` | At original apex, intercept velocity0000->FF80 for eight frame ticks, keeping velocity zero and horizontal movement running. One hold per jump. |
| `magic_power` | All nonzero player projectile powers (IDs1..8) and all16 selected-spell slots become floor(original*1.5); empty slots and enemy projectile tables stay stock. |
| `camera_center` | Guarded centre thresholds and initial X centre in interpreter-owned camera code; the original loader streams every exposed tile strip. |

The new level-two opening values are explicit project choices: 21HP and13DEF
sit between the requested level-one values and the stock level-three values.
The BPS does not specify this opening balance or exact eight-frame holds. Magic
uses an explicit uniform50% increase instead of its uneven supplied BPS values.

## Reverse-engineering evidence

Sword rectangles are four eight-byte signed xmin/xmax/ymin/ymax records. At
01:B6BC the direction050A becomes an eight-byte stride; coordinates0502/0504
are added to B6F0/B6F2/B6F4/B6F6 and stored in40/42/44/46. Collision comparisons
use these values at01:B7AE. The four patched words match the supplied BPS.
These are hit extents relative to the entity origin; sprite-art measurements
of exact visible blade length are not claimed.

New-game initialisation02:D9BB clears0480..0FFF, sets entityC0, level1,
directionFF, HP10, STR10, MP3, DEF5, gold100. The filter recognises the HP store
while STR/MP/DEF and priorHP are zero, then arms only the corresponding DEF
store. It never unconditionally rewrites a running game's defence. A normally
ordered loaded party record (HP preceding level) does not match this signature.

Enemy initialisation018CE8 selects bank05 pointer table0591AB (39 entries) for
type2 or059442 (16 entries) for type3. At018D25, record words+1,+3,+5,+7,+9,+11
become HP, MP, STR, DEF, gold and EXP. The final word lands in052A,x.
01:B7E9 loads this EXP, and01:B7F4 calls0194B2 to award it to Nasir.

The previously suspected EXP tables01:FFB7 and02:B1BE are magic DAMAGE.
01:BDF1 loads FFB7[projectileID*2] into0370;01:BA9E uses0370 in the damage
arithmetic. The player cast selector0195BF/C7 reads Nasir0537,0195CA dispatches
to019600..019690, and these choose projectile IDs1..8 with owner1014=0.
019A98 copies1012 to projectile0D40;019AA9 copies1014 to owner0D51.
Enemy spawn019814 sets owner1014=1 and selects IDs0B and above. Damage01BD87
uses owner0 to target enemy slots and nonzero owner to target Nasir.
02:B1AC reads selected spell0537 and indexes B1BE into1056/1057;02:B29E copies
that power to0370. These are two complementary player-magic damage paths.

Jump0281CB sets airborne20 and velocity0580.01:A516 adds velocity to height0514;
01:A528 subtracts0080 from velocity. Initialisation writes high05 first at
0281D5, then low80 at0281DA; the arming hook accepts that actual byte order. Therefore the first0000->FF80 transition
is the original peak. Ordinary hit01:BA93 sets80 and01:BA98 resets counter0512;
01:C5F4 increments the counter and01:C5FA tests6 before clearing protection.

## Tests and practical limits

`tests/lagoon_qol_test.c` compiles the actual feature source with small runtime
registration stubs. It tests every off-state WRAM offset and ROM identity,
exact data values, preserved enemy projectile words, enemy/boss EXP examples,
new-game HP/STR/MP/DEF sequence, loaded-record exclusion, follower/enemy
separation, eight timer ticks for recovery and apex, no second apex hold,
callback deduplication in either activation order, and mismatching ROM refusal.

From the repository root:

```powershell
clang -Wall -Wextra -Isrc -Isnesrecomp/runner/src tests/lagoon_qol_test.c -o <private-output>/qol-test.exe
& <private-output>/qol-test.exe '../Lagoon (USA).sfc'
```

These tests certify the transformations, not an entire campaign. Recovery uses
an eight-frame guest-counter deadline; the apex hold uses frame callbacks,
which intentionally skip speculative frames. Actual combat/jump replay, equipment progression, follower encounters,
all spell casts, and in-session save/load need gameplay qualification. These
per-session host timer counters are not serialized in existing SNES states.
`RtlStateGeneration()` is checked before active-mechanics CPU reads/writes and timer
frame callbacks. A reset or successful file/memory snapshot load clears pending
new-game, immunity and hold counters before they can affect the restored timeline. Loading
a mid-hold state therefore resumes stock expiry/descent without reconstructing
the extra immunity or remaining hold; exact mid-hold snapshot parity is not claimed. Rollback
restores intentionally do not change this generation, so netplay/rollback parity
for these optional mechanics remains unqualified. A public
`LagoonQol_ResetTransient()` hook is also available to title integration.
The new-game guard is specific to the observed initializer sequence. Feature
states should be selected before starting the game; hot toggling is not provided.

Both interpreter and generated AOT CPU writes use the same generic canonical
WRAM filter, installed at ROM load through `cpu_set_wram_write_filter_hook`.
16-bit stores apply the low-byte filter/store before the high-byte filter/store.
This ordering permits the apex hook to keep both velocity bytes zero. Camera centring never writes its origin from a host frame callback. Runtime snapshot parity
for the default-off build and enabled feature routes is recorded separately by
the integrating agent; this document does not turn synthetic tests into gameplay
proof.

## Jump replay input

The script button is `a`. The host uses a canonical pad mask: script `a` is0100,
then host_main.c's conversion maps it to physical SNES0080. Script `b` is0001
and maps to physical8000, so the previous B-named route did not exercise jumping.

02805A copies new-press low controller byte022E into053E;0281C6 requires its
bit80. This is A. The grounded gate checks0501&30==0 and0501&03==0;DP51 bit0 enables jumping for the current map. Use a fresh3-frame A pulse after reaching controllable gameplay,
then release for several frames before trying again. Add a direction such as down
for an airborne-distance test. Record0501 bit20, height0514, velocity0516, and
position0502/0504 each frame; a snapshot merely named jump is insufficient.



## Measured targeted airborne fixture

`tools/script_jump_fixture.txt` deliberately changes town map flagDP51 from82
to83, enabling the ROM's existing jump action. The unmodified town's bit0 is
clear, so A correctly produces no jump there. This is a controlled physics
fixture, not evidence of reaching a jump-enabled campaign room.

Final fixture SHA256:
`2a73a4ee2308b885777313709706f43411d2a5e32eedfc890f27e74d3d4ea826`.
Same executable, fresh private save roots, only `jump_hold` differs:

| Measurement | Off | On |
| --- | --- | --- |
| Stationary peak height0514 | 8448 | 8448 |
| First negative velocity frame | 6570 | 6578 |
| Stationary landing frame | 6592 | 6600 |
| Moving landing frame (two-frame samples) | 6677 | 6685 |
| Moving landing world Y (starts202) | 271 | 283 |

The original height is preserved, descent and landing are delayed exactly eight
frames, and ongoing directional input covers twelve additional pixels before
landing. The fixed total-input route's final grounded position is equal; the
relevant distance comparison is position at landing. Private receipts and all
per-frame state rows are in `work-native-20261015-111814/jump-fixture-final-off`,
`jump-fixture-final-on` and `jump-fixture-verification.json` outside this source
repository. Native-versus-prior-control qualification is recorded separately.

## Walking scope correction

The supplied BPS preserves both original and relocated tier-two rows at+/-3.
It adds a tier-seven row at+/-5 and changes Nasir's initializer to select7.
The earlier implementation patched shared tier two, which could also accelerate
NPCs/enemies. The corrected implementation leaves that ROM row unchanged and
filters only player velocity0506/0508 when the player is active, direction0..3,
tier2 and neither hurt/knockback flagC0 is set. Horizontal walking while airborne
is eligible; the separately guarded follower feature still produces+/-3 even
with player faster walking enabled. Tests explicitly cover enemy/NPC exclusion,
both signs, hurt states, nonwalking tiers and invalid/no-direction state.

## Camera streaming correction

The earlier host callback moved0480/0482 directly. A natural town route (down60,
left50) exposed missing/stale tiles because the native camera's eight-pixel strip
upload checks had not run. Short centered snapshots had not qualified streaming.
The callback and its direct WRAM writes have been removed completely.

The corrected optional feature changes only five verified instruction operands:

| CPU instruction | Stock operand | Centred operand |
| --- | --- | --- |
| 01:8407, right threshold | 192 | 128 |
| 01:8431, left threshold | 64 | 128 |
| 01:845E, down threshold | 176 | 112 |
| 01:8488, up threshold | 80 | 112 |
| 01:87F5, initial X offset | 112 | 128 |

Initial vertical offset at01:8814 already uses112 and stays unchanged. The new
initial X centre is applied before01:8869/01:88E1 load the full native view, so
there is no initial origin teleport across an unprimed tile ring. The four
thresholds then let the original direction-specific camera loops move one pixel
at a time, invoke strip loaders at every eight-pixel boundary, and maintain the
original BG2/parallax and room-edge logic. Native camera coordinates retain their
full world range; no host10-bit mask or guessed room margin is applied.

Both the normal and the safety-floor execution paths interpret these camera
instructions. Explicit `force_lle` entries cover0183D8,0183FB,018426,018452,
01847D,0187D0 and0187E9. `tools/check_camera_patch_safety.py` rejects generated AOT
bodies whose covered range overlaps any modified operand and runs after generation
and during the build. The runtime also checks affected entry ranges for AOT
variants and refuses the camera patch if it detects a conflict. All five stock
instructions are checked before any camera byte changes; one mismatch refuses
the entire camera change. The original ROM on disk is never modified.

Actual-source tests verify unchanged stock ROM/RAM with features off, all five
expected operands, the untouched initial vertical centre, zero camera callback
registrations/WRAM writes, rejection of a compiled camera entry, and atomic
refusal on one mismatching operand. The integrated natural town route checks
8,640 native tile descriptors without a mismatch; each of five actual GoldCave
checkpoints also matches both retained map layers. Normal/wide paired routes
preserve native pixels, guest state and clock. Transition frames are evaluated
separately from initialized ordinary rooms. These bounded checks do not qualify
every room, parallax scene, transition or the full campaign.

## Recovery correction: batch 4

The actual contact fixture in `tools/DAMAGE-BATCH4.md` found that holding the
shared counter at 5 added 24 pixels to first-hit knockback. The correction never
changes counter0512 or flag0501. A fresh alive player ordinary-hit flag arms
the detector; its stock counter6/flag-clear transition starts an eight-frame
window measured with `snes_frame_counter`. Room writes, death, reset and state
generation changes invalidate pending state.

Only the first `LDA0501,Y` of helper01:BA52 may see a synthetic80. The read
requires playerY0, M1X0/E0, bank01/81, a live JSR return from one of four
observed eligibility call sites, guarded JSR operands and the guarded helper
bytes. Interpreter reads additionally require the exact pre-instruction PC;
native reads use live registers/stack without trusting a stale PC. Both paths
use the shared canonical CPU read API. Other guest readers and WRAM itself see
stock flags; existing knockback/control/animation routines run unchanged.
The read hook exists only while this feature is active and clears at ROM load.
Unit checks prove those guard exclusions and the exact eight-counter window.
The c7c2d806 integration candidate passes the frozen ordinary-enemy damage
routes: all 397 horizontal snapshots retain stock position/velocity/HP/flags/
counter, while the vertical route blocks an immediate rehit for exactly eight
additional frames (3283..3290) and permits the next actual hit at 3291.
See `tools/DAMAGE-BATCH4.md` for hashes, setup and private receipts. Special
hazards bypassing this helper, bosses and rollback remain unqualified here.
