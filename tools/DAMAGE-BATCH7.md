# Batch 7: native actor dispatch, player hits and first-boss phase

Candidate `03fcc1f6eb0fef35c9a3427bf36d1c6aa36adf8faed9103f465d0f670f35b2c2` passes four frozen gameplay routes against the retained batch-6 executable `14be32d86a553a27f47971bf5479f9900d225c832f6ee5dc016b870d1423a507`. It adds four exercised native nodes: A7B5 actor dispatch, ACAA boss dispatch, ACE7 first-boss phase and B9F4 hit handling. Manifest `3d7a1863a4509a5a8bed9e5523e1cd4567705231c01de751e8d1c04f2cd780e2` contains 181 AOT-eligible and 18 LLE-only nodes, with no other node or disposition changes. The interpreter remains required.

## Independent runtime qualification

| Frozen route | Checkpoints | Complete ordered presentations |
| --- | ---: | ---: |
| Ordinary contact and player death | 397 | 3,664 |
| Actual consumable revival | 397 | 3,666 |
| First-boss entry, quake and contact | 59 | 4,088 |
| First-boss defeat and original rewards | 100 | 4,521 |
| Total | 953 | 15,939 |

All framebuffer, WRAM, register, VRAM, OAM and CGRAM checkpoint bytes match. Frame, master clock, vpos and hpos match. Complete ordered `(frame, crc32)` presentation streams match in content and length. These are fresh headless runs with script-only input, private saves/configuration, widescreen disabled and the original per-route feature settings. Route hashes, coverage build/program identities and retained manifest hashes match. Coverage has complete checkpoints and no dropped cost samples, tuple overflows or journal failures.

These targeted fixtures preserve their earlier explicit setup limits: revival equips the existing consumable; boss defeat uses legal level/equipment/derived combat stats and a position for real door entry. They do not seed HP, enemy health, damage outcomes, rewards or victory. This is bounded hybrid qualification, not complete campaign or independent-console parity. It provides no visible-display or 60-fps performance proof.

## Why the dispatch seam is necessary

The first candidate compiled ACE7 but left it interpreted at all 337 / 94 observed boss-route entries. ACC4 was still inside interpreted ACAA, and ordinary interpreter tail jumps do not become paired native calls. That intermediate candidate's exact output was useful evidence but did not establish native boss-phase execution.

The accepted candidate promotes the actual paired actor dispatcher A7B5 (10 instructions) and boss dispatcher ACAA (14), allowing the ACE7 child to run natively. A7B5 is genuinely called by the main actor loop at 0180CA and renderer at C6AC. Both dispatchers balance PHY with PLY before their computed JMP, passing their original return-frame ownership to the selected handler. Generated tail dispatch explicitly inherits that context.

A7C6 has exactly six original type targets from A7C9..A7D3: A7D6, AC4F, AB31, ACAA, A7B5 and B12B. The original type-4 self-tail remains present. ACC4 selects sixteen handlers from ACC7..ACE5. Every handler except ACE7, along with the direct B573 death tail and other actor-family targets, has an explicit interpreted boundary. No guessed exact return-width assertion is added for these tail handlers. This preserves the same guest-frame model already used by the native D8DA room dispatcher.

ACE7 has one callee, existing native B730. Its post-call continuation contains absolute loads, shifts, CLC, absolute ADC, stores and RTS, and safely decodes the existing conservative four-mode callee return set. The independent local flag-04 branch also decodes structurally but is not exercised in these captures.

## Player-hit and nonlocal stack boundary

B9F4 calls native hit eligibility BA52. If eligible, it saves Y and restores it before its BA6B damage call. The only continuation after that call is RTS at BA1E, which safely decodes at all four possible return modes. Its own local stack is balanced before nested damage begins.

The exceptional 92A0 escape remains inside interpreted 9226. Its two PLA instructions discard the internal return to 9269, and its RTS consumes BA6B's return to reach BAFB. BA6B restores X and returns; B9F4 then performs its ordinary RTS. Promoting B9F4 does not introduce a new native host boundary at 92A0. Revival through D481/D487/D8CE and boss death through 9346 preserve their original guest frames and cooperative yields. The frozen routes exercise those ordinary nested yields. The type-2/mode-24 special escape still has structural reasoning only, without an actual encounter fixture.

## Measured native work

Interpreted instruction counts use the exact four routine ranges, excluding neighboring handler bodies:

| Routine | Contact | Revival | Boss entry | Boss defeat |
| --- | ---: | ---: | ---: | ---: |
| A7B5 actor dispatch | 99,730 -> 9 | 117,710 -> 29 | 58,380 -> 0 | 53,980 -> 0 |
| ACAA boss dispatch | 0 -> 0 | 0 -> 0 | 4,381 -> 0 | 1,606 -> 0 |
| ACE7 first-boss phase | 0 -> 0 | 0 -> 0 | 5,864 -> 0 | 1,588 -> 0 |
| B9F4 hit wrapper | 64 -> 64 | 89 -> 89 | 18 -> 0 | 35 -> 11 |

Across all four ranges and routes, 343,243 interpreted instructions and 1,330,591 interpreted guest cycles disappear. These are guest instruction costs, not measured wall-clock speedups. The small actor-dispatch residuals are actual interpreted continuation PCs after yields; ordinary B9F4 shared-suffix costs also remain. They are retained in the accounting rather than rounded away.

No direct native-entry counters are available in this capture format. Evidence combines the emitted precise native bodies and inherited tail-return contexts, actual entry/range removal from interpreted coverage, and complete state/timing/presentation parity. ACE7's entry instruction and all captured phase-body instructions now disappear, unlike the rejected compiled-but-unentered intermediate claim.

The baseline first-boss routes exercise these phase branches:

| Branch PC | Boss entry | Boss defeat |
| --- | ---: | ---: |
| ACF6 ordinary pose | 271 | 84 |
| ACFA attack timer | 48 | 0 |
| AD11 timer completion | 3 | 0 |
| AD1A contact call | 45 | 0 |
| AD22 airborne pose | 18 | 10 |
| AD34 flag-04 branch | 0 | 0 |

## Remaining boundaries

9226 owns HP subtraction, revival tails, the exceptional two-PLA return and boss-specific death dispatch. A private attempt to retain 9294 as an interpreted continuation and give 92C3 all four exit modes still decodes the X1 CPX variant as BRK at 9296; its revival tail also imports the unproven CDB7 call. That attempt is structurally ineligible and rejected. It does not justify a native HP subtraction root.

9346 calls broad C643 before a width-dependent CPX. Keep it interpreted until its width contract is source-closed or a suitable region boundary exists. Its 1,187,008 interpreted instructions on boss defeat are mostly acknowledgement polling: 593,475 LDA 2141 instructions at 937C and 593,475 branches at 937F. A compiled but unentered poll helper would not establish native work, and no such helper is promoted here.

Private `native-raster-acceptance.json`, `verify_raster_native.py` and the four `raster-native-*` directories under `work-native-20261015-154136` retain exact comparisons and per-PC costs. The clean `boss-dispatch-seam` proposal and separate rejected `hp-subtract-seam` analysis preserve the source investigation. State/raster repair and visible performance evidence are owned by their separate batch-7 checks, not this gameplay receipt.
