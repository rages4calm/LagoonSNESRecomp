# Batch 6: native HP recovery and suspended-call bank correctness

This batch adds two exercised native roots: `01D487:M1X0` (18 decoded instructions) and `0191EA:M1X0` (27 decoded instruction variants). The manifest changes from 175 to 177 AOT-eligible nodes, with 18 LLE-only nodes. These are the only two new nodes. HP addition, carry propagation and player HP clamping now execute natively on the captured routes. The original HUD and frame wait retain their interpreter safety net.

## Why this boundary is sound

The existing lethal-hit fixture enters the original `01D481 JSR D487`. D487 saves X, starts the healing sound, sets X=0, obtains the original maximum HP, and repeatedly calls the original HP increment and DMA wait until HP reaches its original cap. The shared D4D3 tail restores sound, restores X and returns at D4DA. It does not consume the item or call the broad C643 actor dispatcher. Its interpreted caller later jumps D8CE for the original item consumption path.

The relevant normal-return width facts are closed over their actual callees, rather than guessed for a general dispatcher:

- `01957F` saves/restores Y and performs the level-table lookup without changing M/X.
- `008985` reads the transfer queue and either returns or polls completion, without changing M/X.
- `0191EA` performs the HP increment/clamp and, for X=0, invokes HUD case 17 through 0080B9. Bank-02 D459 reaches D4BD, whose REP20 path closes with SEP20 at D507. Its D53D path reaches 0089EC, whose REP blocks each close with SEP20 at 008A48, 008A69 or 008A8D. Its queue-full wait is the same width-preserving 008985. No branch changes index width.

The maximum-HP lookup and frame wait remain interpreted and have entry-specific M1/X0 normal-return contracts. The source-closed 0191EA HP helper is also compiled at M1/X0. Its X!=0 branch returns directly without changing modes; its X=0 branch always selects HUD case 17. The generic bank dispatcher 0080B9 remains interpreted with a conservative all-four possible exit-mode set. Its continuation in 0191EA is only RTS, which is valid at all four widths. This does not assert an exact mode for every generic dispatcher target. No additional HUD descendant is promoted. The existing sound helpers already have exact M1/X0 contracts. Native D487 uses the existing precise instruction timing and preserves the original stack frames. The broad HP/death wrapper 019226, actor dispatcher C643 and MP healing remain separate work; this change does not assert a new mode contract for them.

## Regression found by the revival fixture

The first native trial diverged at `damage-270`, frame 3457: the control had PB=00 while the candidate had PB=01. The other registers, guest state and clocks matched. D487's `01D49E JSL 008985` was suspended inside the bank-00 transfer wait. A cooperative yield unwinds host C calls while retaining the live guest call stack. It has not executed the original RTL.

The generic generated JSL envelope previously restored its saved caller PB during that host unwind, incorrectly changing the live suspended guest bank. It now restores PB only when `interp_bridge_lle_unwind_pending()` is false. An ordinary completed JSL still restores its caller bank; ordinary nonlocal SKIP propagation still performs its previous restore before propagating. The cooperative suspension path retains the callee's live bank for snapshots and resume. Short JSR transfers continue to preserve PB. This change does not redefine the existing nonlocal-return stack ABI.

The failed trial remains private. The following acceptance describes the corrected executable only.

## Independent bounded acceptance

Candidate executable SHA-256:
`14be32d86a553a27f47971bf5479f9900d225c832f6ee5dc016b870d1423a507`

Manifest SHA-256:
`c58f63a1b811411f399a6b13716d602865b002a12ac4ce06e8ef1f8921c23023`

Retained batch-5 control executable:
`13ce550c4ccdf0f5fc214abe9067e89e5dbbce31e70d1d8734922e063b35e439`

| Frozen route | Checkpoints | Complete ordered presentations |
| --- | ---: | ---: |
| Ordinary contact and player death | 397 | 3,664 |
| Actual consumable revival | 397 | 3,666 |
| First-boss entry, quake and contact | 59 | 4,088 |
| Actual first-boss defeat and rewards | 100 | 4,521 |
| Total | 953 | 15,939 |

Every framebuffer, WRAM, register, VRAM, OAM and CGRAM checkpoint is byte-identical to the retained control. Frame, master clock, vpos and hpos match. Complete ordered `(frame, crc32)` presentation streams match, including length. All four are fresh runs of the final integrated executable, with widescreen disabled and the original per-route feature settings. The route hashes match their original frozen scripts, the candidate uses script-only input, and the coverage build/program identities match its retained receipts. Coverage reports no dropped cost samples, overflowed tuples or journal failures, and complete checkpoints.

The revival fixture explicitly equips the existing revival consumable; real enemy damage triggers death and the original healing/item consumption. The boss fixture explicitly sets legal level/equipment/derived combat stats and a position for the real door entry. It does not seed HP, boss health, rewards, death or victory. These are targeted fixtures, not natural campaign completion. Their exact scripts and setup limits are recorded in `DAMAGE-BATCH5.md`.

## Actual native work and remaining interpreter cost

Counts use the exact 18 instruction PCs of D487 plus its D4D3 shared tail, excluding the intervening MP-heal routine, and the HP helper's original 0191EA..019225 instruction PCs:

| Routine and route | Control interpreted instructions | Candidate interpreted instructions | Control/candidate interpreted guest cycles |
| --- | ---: | ---: | ---: |
| D487 revival | 72 | 62 | 313 / 267 |
| D487 boss defeat | 1,498 | 1,488 | 6,496 / 6,450 |
| 91EA ordinary contact | 4,161 | 0 | 16,618 / 0 |
| 91EA revival | 4,572 | 0 | 18,232 / 0 |
| 91EA first-boss entry | 3,864 | 0 | 15,456 / 0 |
| 91EA boss defeat | 9,054 | 0 | 35,724 / 0 |

The D487 entry instructions disappear from interpreted coverage. Its wait yields and the original loop continuation resumes interpreted, so D487 itself removes only 10 interpreted instructions per captured invocation. Its repeated genuine JSR calls now enter native 91EA for HP arithmetic and clamping. The captured 175 HP-helper calls on ordinary contact, 194 on revival, 161 on first-boss entry and 408 on boss defeat all complete their original interpreted HUD calls from native callsite 019221 without yielding. The original HUD updates remain intact and all 91EA interpreted instruction costs disappear.

Across the four exact routes this removes 21,671 interpreted instructions and 86,122 interpreted guest cycles. These include ordinary HP regeneration as well as revival healing. Guest instruction counts are not a wall-clock speedup claim. There are no direct native-entry counters in this capture format; evidence combines emitted precise bodies, their executed callsite records and interpreted-PC removal with complete state/timing parity.

Private `native-integrated-acceptance.json` and `verify_integrated_acceptance.py` retain the independent comparison, exact PC costs and callsite evidence under `work-native-20261015-145037`. Earlier one-root and two-route acceptances remain private as distinct intermediate results. The interpreter safety net remains necessary for unqualified death/boss paths, other bosses and later campaign progression. The four matching routes exercise real ordinary completion and nested cooperative suspension but do not exhaust every possible nonlocal SKIP path in the generic emitter.


## Additional recovery-time regression check

A fifth frozen route enables the existing eight-frame recovery option. Its 397 checkpoints and 3,664 ordered presentations match all framebuffers, WRAM, VRAM, OAM, CGRAM, clocks and presentation records. Three register captures differ only in PB: control 02 versus candidate 00 at `damage-232` through `damage-234` (frames 3417..3419). This is recorded separately, not counted as exact register parity or included in the exact-route totals above.

The live guest stack identifies the original `0289B6 JSL 0089A7` return as bytes `B9 89 02` at S+1. Bank 00 at 0089AC polls the transfer flag until its RTL at 0089B3; the bank-02 caller resumes at 0289BA. During that suspension the correct live PB is 00. The old envelope prematurely restored caller bank 02 when unwinding the host stack; retaining 00 is the same source-backed cooperative-yield correction described above. Every other register field is identical. This result is a bounded expected correction, not permission to disregard arbitrary register differences. The retained fifth-route control lacks a cost capture, so no interpreted instruction saving is inferred from it.
