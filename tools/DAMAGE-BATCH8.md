# Batch 8: native HP arithmetic, bounds and first-boss landing

Final candidate `25b3b33272d74beccc1213423d4379a9ebf1bbe550ed91e32e84f4b275001bda` passes the four primary gameplay routes and an additional all-nine-QoL contact route against batch 7 `03fcc1f6eb0fef35c9a3427bf36d1c6aa36adf8faed9103f465d0f670f35b2c2`. Seven canonical symbol roots, explicit interpreted boundaries and precise-timing entries are integrated. Regeneration without broad cfg-root promotion produces exactly 188 AOT / 18 LLE nodes and 59 roots. Camera patch safety passes. All tests use fresh private headless sessions; no visible window was used.

## Actual next paths

The fifteen other handlers in the ACC7 boss table have zero entry-PC visits in both existing first-boss routes. That table indexes boss type 051B, rather than progressing the first boss through fifteen additional phases. Promoting those unentered routines would not provide measured first-boss native work.

Seven new native nodes follow actual captured damage and first-boss execution:

| Entry | Responsibility | Decoded instructions |
| --- | --- | ---: |
| 019BCE | Actor bounds dispatcher | 10 |
| 01A01A | Boss-type bounds dispatcher | 10 |
| 01A04E | First-boss X/Y bounds and velocity reset | 23 |
| 01A053 | First-boss shared X-bound helper | 22 |
| 029036 | Boss-type motion dispatcher | 10 |
| 02906A | First-boss airborne countdown and landing state | 22 |
| 019226 | Bounded HP subtraction prefix; death/HUD tails interpreted | 12 |

The final generated manifest has exactly these seven new nodes and no other disposition changes relative to batch 7. Its SHA-256 is `ca8099f0601967b4b13614f90fc26a192a2637be15a2fe0bc1d20bed11eac3b7`. Canonical generation preserves the bounded HP-prefix attributes.

## Return ownership and width proof

019BCE is genuinely called by the original main actor loop at 0180C1. It balances PHY / PLY before its tail JMP at 9BDF. The six original type slots at 9BE2..9BED have four distinct targets: 9BEF, 9C1B, A01A and 9BEE. The player, NPC/enemy and empty handlers retain explicit interpreted boundaries.

The A01A boss bounds table has sixteen original words at A02E..A04D and three distinct targets: A04E, 9C1B and A0A4. Only the first-boss path A04E is promoted. Its genuine JSR to A053 checks X against 30..E0 and clamps when outside; the Y continuation checks 50..D8. The original shared tail clears velocity and sets the original bound-state bit. It performs byte arithmetic as the original code does; this proposal does not replace that behavior with a different coordinate model. A04E's and A053's overlapping shared tail is counted only once in interpreted opportunity ranges.

Bank-02 9029 has a genuine JSR at 9031 to 9036, followed by its own PLY / RTL. The called dispatcher balances its own PHY / PLY before JMP 9047. Its sixteen original type targets at 904A..9069 retain explicit interpreted boundaries except 906A. Thus 906A's RTS consumes the original 9031 JSR return; no synthetic native frame is invented.

906A checks airborne flag 20, decrements original jump counter 0390, clears velocity when the countdown reaches zero, and clears airborne/animation counters on landing. It leaves the original 03AE/03AF signalling intact. All six proposed bodies preserve entry M1/X0 directly: no REP, SEP, PLP, nonlocal stack escape, MMIO or unknown child return contract occurs in them. A053 is the only ordinary child call in the proposed bounds bodies; it has the same closed width and stack behavior. The computed dispatchers preserve their original inherited return context, using the same model qualified in batch 7.

## Bounded native HP arithmetic prefix

Existing SDK region controls isolate the twelve arithmetic instructions at 9226..9243 with `end:9245` and named interpreted boundaries at 9245 and 9294. Both analysis and final generation report exactly one additional eligible node, `019226:M1X0`, for 188 AOT / 18 LLE nodes and 59 roots. Its three outgoing edges are interpreted: BMI at 923B to death tail 9245, zero-HP fallthrough at 9243 to 9245, and nonzero-HP BNE at 9243 to HUD tail 9294. No 92A0, 92C3 or CDB7 body is imported, and no new exact return-width contract is asserted.

The prefix changes no stack register or return frame and preserves M1/X0. Its genuine caller is the qualified native BA6B JSR at BAF8. Each interpreted tail inherits that original invocation and stack watermark; the special 92A0 two-PLA escape remains inside one interpreted death tail. CDB7 is not a shortcut to a general closed width contract: its equipped-item branch sets 0417=1 and calls broad C643 before PLX/PLY. That still requires renderer proof if compiled in full.

Canonical symbol synchronization preserves the manual `end:9245` declaration instead of replacing it with an unrestricted wrapper. Explicit `tail_call:` is unnecessary and its AOT-only sibling validation would reject this interpreted target. The emitter's normal named-boundary routing generates all three edges as `interp_tier_dispatch_tail` with the original `_entry_s` and `_hrv`; this was checked in the generated C before building. The prefix, original death/revival tails and all four routes now pass exact runtime comparison. The type-2/mode-24 escape remains structurally reasoned rather than encounter-qualified.

## Actual runtime qualification of the seven roots

Baseline captures record 336 / 183 first-boss A04E entries on boss entry / boss defeat. The bank-02 9036 and 906A entries occur 336 / 87 times. These are real executed paths, without adding boss flags, HP, rewards or victory seeds.

| Unique PC range | Contact | Revival | Boss entry | Boss defeat |
| --- | ---: | ---: | ---: | ---: |
| 019BCE..019BDF | 99,160 -> 4 | 117,140 -> 6 | 57,780 -> 0 | 53,180 -> 0 |
| 01A01A..01A02B | 0 -> 0 | 0 -> 0 | 3,360 -> 0 | 1,830 -> 0 |
| 01A04E..01A0A3, including shared A053 | 0 -> 0 | 0 -> 0 | 6,720 -> 0 | 3,660 -> 0 |
| 029036..029047 | 0 -> 0 | 0 -> 0 | 3,360 -> 0 | 870 -> 0 |
| 02906A..02909F | 0 -> 0 | 0 -> 0 | 1,458 -> 0 | 402 -> 0 |
| 019226..019243 | 57 -> 0 | 93 -> 0 | 12 -> 0 | 21 -> 0 |

These are baseline -> candidate interpreted instruction counts at unique original instruction PCs. The shared A04E/A053 tail is counted once. Across the four captures, 349,093 interpreted instructions and 1,357,600 interpreted guest cycles disappear. The ten remaining 9BCE-range instructions are actual continuation PCs 9BDC/9BDE/9BDF after yields, retained in the accounting. This is actual native work, not merely a larger compiled node count. No direct native-entry counters are available; evidence combines emitted precise bodies and original paired-call/return ownership with actual interpreted-entry/range removal and complete output/timing parity. Guest instruction costs are not a wall-clock speedup or steady-60-fps claim.

| Frozen route | Checkpoints | Complete ordered presentations |
| --- | ---: | ---: |
| Ordinary contact and player death | 397 | 3,664 |
| Actual consumable revival | 397 | 3,666 |
| First-boss entry, quake and contact | 59 | 4,088 |
| First-boss defeat and original rewards | 100 | 4,521 |
| Total | 953 | 15,939 |

Every framebuffer, WRAM, register, VRAM, OAM and CGRAM checkpoint is byte-identical to the retained batch-7 control. Frame, master clock, vpos and hpos match. Complete ordered `(frame, crc32)` presentation streams match in content and length. Original route hashes and feature settings match; new runs use script-only input, widescreen disabled and fresh private save roots. Coverage identities match each executable and retained manifest, with complete checkpoints and no dropped cost samples, tuple overflows or journal failures. The common 9BCE dispatch therefore has direct evidence on all four routes, rather than only the boss encounter.

These remain targeted fixtures with previously documented legal setup. Neither this batch nor its tests fabricate HP, enemy health, rewards or victory. Other bosses, full natural campaign progression and visible frame pacing are separate work. The interpreter safety net remains active.

Private `final-native-acceptance.json`, `verify_final_native.py`, `final-manifest.json` and the four `final-*` capture directories under `work-native-20261015-161718` retain independent exact comparisons, identities and per-PC costs. The private `boss-update-seams` directory under `work-native-20261015-161718` retains `proposal.diff`, its independent analyzer result and per-range instruction/cycle opportunity. The death/revival regions beyond the native prefix, exceptional 92A0 branch, 9346 and other bosses retain their interpreter safety net. No guessed generic-dispatcher width contract bypasses their earlier blockers.


## All nine QoL options together

A separate fresh baseline/final pair enables walk speed, follower speed, sword reach, recovery time, starting HP/defense, EXP boost, jump hold, magic power and centered camera on the original contact route. All six state components and clocks match at 397 checkpoints, and all 3,664 ordered presentations match. Real hits reduce HP from 18 to 15 and original regeneration restores it. With these stronger settings Nasir survives this bounded route, so this extra check is contact/recovery evidence; death and revival remain covered by the primary routes. It does not qualify every optional feature throughout the campaign.

The combined output checks cover 1,350 exact checkpoints and 19,603 presentations. Cost claims above use only the four coverage-enabled primary routes. Private `allnine-contact-acceptance.json` and `verify_allnine_contact.py` retain the separate options-enabled comparison.
