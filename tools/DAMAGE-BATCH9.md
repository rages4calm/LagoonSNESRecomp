# Batch 9: bounded HP death and HUD tails

Selected optimized executable `81bf8a4bf685fca8994836e3701499677ad9afded0139673b5732e269d97b814` passes four primary routes and an additional all-nine-QoL contact check against retained batch 8 `25b3b33272d74beccc1213423d4379a9ebf1bbe550ed91e32e84f4b275001bda`. The baseline has 188 AOT / 18 LLE nodes and 59 roots; the selected candidate has 190 / 18 and 61 roots. Qualification used fresh private headless sessions; no visible window was used.

## Two exercised native tails

| Entry | Bounded responsibility | Decoded instruction variants |
| --- | --- | ---: |
| 019245 | HP-zero initialization, revival choice and death prelude through 9263 | 12 |
| 019294 | Post-damage player HUD selection and return | 8 |

The private analyzer and final manifest contain exactly these two new eligible nodes: 190 AOT / 18 LLE nodes and 61 roots. No other node or disposition changes. Manifest SHA-256 is `b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`. Production preserves the existing `Lagoon_Health_DeathTail` and `Lagoon_Health_HudTail` names already present in the accepted generated header, avoiding a preferred-alias rename across generation passes.

9245 uses `end:9266`. Its original JMP at 9257 tails into explicitly interpreted D481 for revival. The fallthrough after 9263 tails into explicitly interpreted 9266, starting at the original JSR 92A0. No 92A0, 92C3, CDB7 or C643 body is imported. The interpreted death-call tail still owns the original two-PLA nonlocal escape and boss-specific dispatcher. 0080E6 remains forced interpreted.

Both new roots are reached from the qualified native 9226 arithmetic prefix through original tail edges. The 9245 prelude performs no push, pop, mode change, subroutine call or MMIO. It inherits the original invocation's `_entry_s` and `_hrv`; selecting either interpreted sibling adds no new guest call frame. Thus 92A0's unusual return remains inside the same interpreted tail instead of crossing a newly compiled 92A0 call.

9294 enters at observed M1/X0, compares X with zero and skips the player HUD for nonplayer entities. The player path always selects original HUD case 17 through the existing interpreted 0080B9 dispatcher. Its only post-call instruction is RTS, safely decoded at every mode in the existing conservative four-mode return set. There is no new guessed width contract. Missing entry variants continue to use the interpreter.

## Actual baseline execution

| Routine | Contact | Revival | Boss entry | Boss defeat |
| --- | ---: | ---: | ---: | ---: |
| 9245 entry visits | 1 | 1 | 0 | 1 |
| 9245..9263 interpreted instructions | 9 | 8 | 0 | 8 |
| 9294 entry visits, M1/X0 | 5 | 7 | 1 | 2 |
| 9294..929F interpreted instructions | 25 | 35 | 5 | 8 |

These two baseline ranges account for only 98 interpreted instructions and 406 interpreted guest cycles across the four frozen routes. The final candidate removes 90 of those instructions and 373 interpreted guest cycles. This is a small amount of actual damage/death/HUD work, not a frame-rate claim or complete native death implementation.

| Routine range | Contact | Revival | Boss entry | Boss defeat |
| --- | ---: | ---: | ---: | ---: |
| 9245..9263 interpreted instructions | 9 -> 0 | 8 -> 0 | 0 -> 0 | 8 -> 0 |
| 9294..929F interpreted instructions | 25 -> 5 | 35 -> 0 | 5 -> 0 | 8 -> 3 |

The residual five-instruction player HUD body on contact and three-instruction nonplayer body on boss defeat are complete original interpreted arrivals after fallthrough from the preserved 9266 death-call tail. They are not merely native RTS cleanup and are retained in the accounting. Native entries from the 9226 prefix execute the newly compiled tails; an ordinary interpreter fallthrough does not automatically bounce into the native body. No direct native-entry counters are available, so evidence combines emitted bodies, real source/stack entry paths, interpreted-PC removal and complete state/timing parity.

## Independent exact comparison

| Frozen primary route | Checkpoints | Complete ordered presentations |
| --- | ---: | ---: |
| Ordinary contact and player death | 397 | 3,664 |
| Actual consumable revival | 397 | 3,666 |
| First-boss entry, quake and contact | 59 | 4,088 |
| First-boss defeat and original rewards | 100 | 4,521 |
| Total | 953 | 15,939 |

All framebuffer, WRAM, register, VRAM, OAM and CGRAM checkpoint bytes match the retained 25b3 control. Frame, master clock, vpos and hpos match. Complete ordered `(frame, crc32)` streams match in content and length. Frozen route hashes and feature settings match. Final candidate runs use script-only input, widescreen disabled and fresh private save roots. Coverage identities match the executable and manifest, with complete checkpoints and no dropped samples, tuple overflows or journal failures.

These targeted fixtures retain their earlier documented setup. They do not fabricate HP, damage outcomes, enemy health, rewards or victory. The four-route result covers ordinary player death, real consumable revival and the original first-boss death/reward sequence. The special type-2/mode-24 two-PLA escape remains structurally reasoned rather than encounter-qualified; its original interpreted tail remains intact.

A separate final candidate run with all nine QoL features matches the retained 25b3 all-nine capture at 397 checkpoints and all 3,664 ordered presentations, including all six components and clocks. Real hits reduce HP from 18 to 15, with original recovery/regeneration. Nasir survives that options-enabled route; it supplies contact/recovery evidence, while primary routes supply death/revival evidence. Combined checks cover 1,350 exact checkpoints and 19,603 presentations. The cost figures use only the four coverage-enabled primary routes. This provides no steady-60-fps or visible-display claim.

## Rejected and deferred alternatives

Bank-02 9029 is observed 336 times on first-boss entry and 183 on boss defeat, all at M1/X0. Every arrival is the interpreted 0080E6 -> 029029 tail dispatch. Although its 9031 JSR genuinely calls existing native 9036 (336 / 87 times), compiling 9029 alone would repeat the compiled-but-unentered problem seen earlier with ACE7. A separate no-new-contract analyzer probe also leaves 9029 interpreted because the return width of generic 9036 is unproven. It is not promoted.

A private 9346 prologue cut before C643 is structurally eligible, but contributes only twelve prologue instructions on one captured boss-death call and transfers with outstanding PHA/PHX locals. That would require separate stack qualification and is explicitly excluded from this batch. Full C643/CDB7 return contracts remain unproven. The fifteen other boss-type handlers are not extra phases of the first boss and have no measured entry visits on the current first-boss routes.

Private `hp-tails-probe/proposal.diff`, `analysis.json` and `opportunity.json` under `work-native-20261015-164402` retain the clean two-node proposal and measured baseline costs. The separate `motion-wrapper-probe` and `hp-death-prefix-probe` results preserve excluded alternatives without adding them to production. Private `probe-native-acceptance.json` / `verify_probe_native.py` bind the four-route comparison and per-PC costs; `probe-allnine-acceptance.json` / `verify_probe_allnine.py` retain the separate options-enabled comparison. Any later executable needs its own bound acceptance before using these results.

Earlier ef1a final-* and FD604419 renderer-* executables/captures remain retained privately as separate identity-bound evidence. The selected optimized 81bf8a4b executable uses fresh optimized-* runs and the same b872 manifest; those are the final comparison records referenced here. The checks remain headless at native width, and do not themselves establish visible performance or renderer correctness; those use the separate batch-9 renderer/performance records.
