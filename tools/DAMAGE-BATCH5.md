# Damage, revival and first-boss death: batch 5

These are targeted fixtures, not naturally earned progression or campaign
completion. They retain the interpreter safety net. They never write player
HP, enemy state, damage results, rewards, death or victory. Position and legal
equipment/stat setup are explicit below.

Baseline source is `b6da71f700f393b97543e0ed7c21f689472c7d37`; executable SHA256
`904d218f875e4ebd877e9f5df4b85aecf901efbdc70fd7b04c26d9822fcf752b`.
The immutable executable and manifest are retained in workspace
`work-native-20261015-190426/baseline`. Runs use fresh private saves/settings.
This document records baseline observations, native boundary reasoning and the
qualified candidate below. The detailed private acceptance receipt retains
component comparisons and per-PC interpreted costs.

## Frozen first-boss defeat fixture

`script_boss_batch5_defeat.txt`, SHA256
`92c4bc04bbb43c6dc9c20cf727b5964b6df3d19017c5df177df064dfc1fae76f`,
extends the previous real-door/jump/sword fixture. It requires only
`--features sword_reach` and a frame cap of 6000. It captures 100 checkpoints
through frame 4521. Accepted baseline: `boss-defeat-baseline-03`.

Guard and flank position staging are inherited and declared inline. The new
setup uses legal level 35 and sword/armor/shield selectors 5/5/5. ROM stat
tables give level-35 base STR 255 and DEF 255; sword 5 adds 120 STR, armor 5
and shield 5 each add 100 DEF. The fixture writes the corresponding STR 375
and DEF 455 along with the selectors. It does not claim those items or levels
were naturally earned. HP remains the initial 10, well below that level's max.

Actual B-sword contact reduces the initialized type-3/mode-0 boss from 96 to
0 HP at frame 3592. Nasir takes real contact damage, 10 to 8 HP. After the
game's boss death animation, gold increases 100 to 200 and EXP 0 to 64 at
frame 3891. Subsequent HP restoration is performed by game logic. These are
observed death/reward operations, not edits or an inference from a removed
sprite. No story flag change in `04C0..04CF` was observed; the fixture does not
prove post-boss campaign progression, exit access or ending behavior.

Newly useful real entries, all M1X0/E0:

| Site to target | Calls | Evidence |
| --- | ---: | --- |
| `01BA1B -> 01BA6B` | 2 | Player contact plus lethal boss hit |
| `01BAF8 -> 019226` | 2 | Actual HP subtraction |
| `019266 -> 0192A0` | 1 | Boss type 3 takes ordinary return, not special type 2 |
| `019291 -> 0192C3` | 1 | Actual boss death dispatch |
| `008106 -> 019306` | 1 | Inline dispatcher selects boss mode-zero death handler |
| `019306 -> 0192F1` | 1 | Genuine JSR entry to stop-velocity leaf |
| `01931E -> 019346` | 1 | Actual death animation and sound synchronization |

The `0192F1` call is materially different from batch 4's failed claim: this
route really executes a JSR targeting that leaf. Its earlier compiled trial
could not be entered by the ordinary player's intra-body branch arrival.

## Frozen revival fixture

`script_damage_batch5_revival.txt`, SHA256
`71284ef5735ddaaf88e439d2a7a89c634f682cd7b89768fbc59939143e39259e`,
extends the frozen horizontal contact route by selecting valid revival item
`0536 = 25`. All QoL options remain off. Use a frame cap of 6000. Accepted
baseline `revival-baseline-01` contains 397 checkpoints through frame 3666.

Real enemy hits trigger the lethal branch at guest frame 3456. Instead of
game over, the game enters `01D481 -> 01D487` once in M1X0/E0. Post-frame
snapshots show HP 4 to 1 at 3457, then healing one HP per frame to 10 at 3466.
The item selector becomes FF after consumption. No fixture writes HP or
restores the item. Later enemy contact continues normally.

`damage_batch5_report.py` reads immutable route receipts, setup writes,
snapshots and actual entry coverage. Reports are retained as `batch5-report.json`
under both accepted runs. It distinguishes declared setup from observed
transitions; observed entry modes alone do not certify all return contracts.

## Promotion blockers and bounded path forward

`01BA6B` performs hit-state setup, two hardware divisions, random damage and a
JSR to `019226`. Its prior analysis stops at that unproven callee exit; this
is not evidence that the arithmetic itself cannot run natively. The qualified candidate uses a precise
native damage calculation with an interpreted HP/death wrapper. Actual contact, vertical, boss-hit, boss-death and
revival fixtures now cover complementary paths. The integration evidence below checks
its divider timing and actual route behavior against the retained hybrid control.

Keep `0192A0` interpreted until its exceptional stack path is handled. For
type 2/mode 24 it executes two 8-bit PLA operations, discarding its caller's
JSR return `9268`, then RTS uses `019226`'s original caller return `BAFA`.
Thus it skips the rest of the HP wrapper and lands at `BAFB`. The enclosing
interpreted `019226` call still returns normally from `01BA6B`'s perspective;
compiling `019226` and treating `0192A0` as an ordinary child is a different,
unsafe assumption. The analyzer's AOT eligibility for `0192A0` does not prove
that nonlocal contract.

The special enemies are actually initialized in room 44, from spawn list
`05:AD03`: six type-2/mode-24 entities at X 72/184, Y 160/288/416. The shortest
door-table graph from Gold Cave passes 23 transitions including later regions
and bosses. No such enemy was fabricated or claimed covered by these fixtures.

`019226` also reaches revival `D481 -> D487 -> D8CE`, player HUD dispatch
through `0080B9` argument 17 (bank-2 target `02:D459`), and three boss death
handlers `92FE/9306/9382`. Exact mode evidence must be local to those paths;
a blanket exit-mode declaration for the common dispatcher is not justified.
The new boss and revival fixtures provide real runtime coverage without
seeding their outcomes.

`01ACE7` has a genuine phase/animation body ending at `AD68`, but its contact
child `01B730` reaches the same unresolved damage/HP chain. It is entered by
the `ACC4` pointer jump, so compiling only the target is insufficient. Its
sixteen-word boss-mode table at `ACC7` must have a genuine native dispatcher
entry and an explicit complete target/stack contract.

The candidate also promotes the narrower `01B730` body: `01AD1A` genuinely JSRs
to it. It sets Y to zero at `B760` before rectangle testing and the `B9F4`
damage call, so this path targets the player, not a boss or the special mode-24
enemy. Its REP block restores M1 before both child calls. It keeps `B9F4`
interpreted and accepts every possible normal-return width, providing a smaller
seam than compiling the full phase or HP wrapper.

`01DDAD` is a bounded quake helper through `DE10`. Every REP20 branch restores
M1 before RTS; its only direct child is sound helper `0C:F8FE`. However, its
real arrival is `01D8F6`'s JMP. The caller `01D8DA` has no calls or width changes
before that jump, and its real room table has 75 words at `D8F9..D98D` with
28 unique targets. The qualified precise dispatcher and exact pointer-tail contract
give the quake helper a genuine native arrival while other room handlers stay
interpreted. Compiling the helper alone repeats the earlier false-bounce case.

Unique room targets, taken from the actual complete table:

```text
D98F D990 D9DE D9E9 DA66 DA74 DA7F DB18 DB29 DB37 DBA4 DC2D DCA9 DCB7
DCDC DCF9 DD07 DD39 DD6A DD7B DD86 DD97 DDA2 DDAD DE65 DE66 DED0 DF7D
```

Fixture and analysis work made no core, CFG or shared-build edits. The
integration owner applied and built the qualified candidate. Independent
verification against the retained baselines is recorded below.

## Width-proof follow-up: unresolved shared render dependency

The four C654 targets are C683, C688, C6DB and C726. C683/C688 save X/Y,
call C695, restore X/Y and RTL. C6DB saves X, draws through 008B71, calls C669,
restores X and RTL. C726 saves X, draws through 008B71, calls 0081B5, restores
X and RTL. The existing analyzer already proves 008B71 and 0081B5 return
M1X0. C726 therefore has a closed normal-return width argument from M1X0.

The other three tails reach C69B, which calls A4D6 and conditionally A7B5,
then C4AF/C026 and the bank-2 display handlers. A7B5 contains actor type,
flag and state dispatches, including the living boss phase/contact path.
Balanced local saves and RTL do not prove those child modes. A global C643
exit-mode declaration remains unsupported by this bounded analysis.

The revival path also has this dependency: D8CE calls CDB7, which clears the
selected item, sets 0417 to 1 and calls C643 at CDD8. Thus proving only the
player HP subtraction and healing loop does not close B9F4's exceptional
revival return. Keep that unresolved dependency explicit rather than claiming
an independent player-only proof.

Several smaller facts are closed directly from source: 01957F is only PHY,
table reads, PLY and RTS; 008985 has no width-changing instruction. HUD
02D459 calls D4BD, whose REP block restores M1 at D507 before D53D calls
0089EC. That transfer helper restores M1 at 008A48, 008A69 and 008A8D; its
queue-full wait uses 008985. None changes index width. The prior analyzer
also proves sound 0CF8FE and random helper 009800 return M1X0. These local
facts are useful proof inputs, not a substitute for closing C643's actor graph.

## Conservative width sets and stack boundary

The candidate does not assert that the complex HP or player-hit wrappers
always return M1X0. Their entry-specific contracts include every possible
normal-return width:

```text
exit_mx_set 019226 M1X0 M0X0,M0X1,M1X0,M1X1
exit_mx_set 01B9F4 M1X0 M0X0,M0X1,M1X0,M1X1
```

These are conservative TOP sets, not claims that all four modes occur. The
callees remain interpreted. There is no conflicting exact-width fact. Both
parsers preserve these entry-specific sets; the decoder creates four
continuations and the emitter selects the live CPU M/X after normal return.
The actual BA6B cleanup has four versions of BAFB PLX and BAFC RTS; B730 has
four versions of B76D RTS. No hypothetical width is forced onto guest state.

BA6B's PHX at BA9B saves its entry X; its real JSR at BAF8 pushes BAFA.
The interpreted HP/death/revival wrapper returns to BAFB, followed by the
original PLX/RTS. The special 92A0 path discards its own 9268 return frame and
RTS consumes BAFA, reaching the same external boundary. B730 has no local
pushes; its JSR to B9F4 pushes B76C, and that helper balances PHY/PLY before
calling BA6B. It returns to B76D. The generated short-call bridge handles
nonnormal SKIP results before selecting any lexical width continuation.
Width TOP does not itself prove arbitrary emulation, PB or stack changes;
keeping the entire exceptional wrapper interpreted is part of this boundary.

The wider TOP trial for C643 is unused in the accepted candidate. It exposed
an impossible X1 continuation in 9346 whose immediate operand decoded as
BRK019370. That branch remains rejected and 9346 stays interpreted. This is
an overapproximation limit, not evidence of a real boss-animation crash.

Unqualified random-number/divider routines 009800/009848 also stay interpreted.
Their parent call's exact M1X0 return fact is independently source-backed:
009800's only REP20 is at 981A, and every normal return passes SEP20 at 983A
then RTL9847. Its only child 9848..9878 changes neither width and has no child
calls. Neither changes index width. The zero-input self-loop does not return.
This narrow fact avoids silently admitting two additional RNG AOT bodies.

## Independent native acceptance

Qualified candidate SHA256:
`13ce550c4ccdf0f5fc214abe9067e89e5dbbce31e70d1d8734922e063b35e439`.
Program manifest SHA256:
`4cbec4d1cf816a9bbcdfb0a427f844ed171d25b2ad4f4f010e88bb7123372d72`.
It contains 175 AOT and 18 declared LLE variants, from 46 roots. The exact
five-root AOT increase over batch 4 is:

| Entry | Qualified responsibility |
| --- | --- |
| 0192F1 | Stop velocity through a real boss-death JSR |
| 01B730 | First-boss contact rectangle and player-hit bridge |
| 01BA6B | Damage arithmetic and hardware division, with interpreted HP wrapper |
| 01D8DA | Room hazard dispatcher with original tail-call ownership |
| 01DDAD | First-boss phase/quake helper through that real dispatcher |

Fresh script-only candidate runs independently match retained controls:

| Route | Checkpoints | Complete ordered presentations |
| --- | ---: | ---: |
| Horizontal contact and player death | 397 | 3,664 |
| Actual consumable revival | 397 | 3,666 |
| Real first-boss entry, quake and contact | 59 | 4,088 |
| Real first-boss defeat and rewards | 100 | 4,521 |
| Total | 953 | 15,939 |

Every framebuffer, WRAM, register, VRAM, OAM and CGRAM checkpoint is byte
identical. Frame, vpos, hpos and master clock also match. Complete ordered
`(frame,crc32)` presentation streams match in content and length, with no
missing checkpoint tags. Original controls did not guarantee physical-input
isolation; exact equality with these fresh script-only runs establishes
uncontaminated equivalence for these specific routes, not a blanket guarantee
about older captures. The unarmed boss candidate enabled sword_reach; the
retained control did not, and their complete outputs still match.

Coverage has no dropped samples or overflowing tuples. Across the five exact
routine ranges, observed interpreted instructions change as follows:

| Range | Control interpreted | Candidate interpreted |
| --- | ---: | ---: |
| 0192F1..0192FD | 10 | 5 |
| 01B730..01B76D | 641 | 0 |
| 01BA6B..01BAFC | 902 | 4 |
| 01D8DA..01D8F6 | 44,086 | 0 |
| 01DDAD..01DE10 | 6,861 | 0 |

That is 52,491 fewer interpreted instructions on these four routes, not a
whole-game native percentage or wall-clock speedup. The five remaining
stop-velocity instructions are the player's intra-body interpreted arrival;
the boss-death genuine JSR removes its five instructions entirely. BA6B's
four residual instructions are one PLX/RTS cleanup pair after the revival
wrapper yields and one after the boss-death wrapper yields. The arithmetic
itself no longer interprets on these captures.

The capture format does not provide direct native-function entry counters.
Evidence combines emitted precise bodies, actual entry/range interpretation
removal, complete gameplay/state/timing equivalence, and native-body BAF8
callsite records showing completed/yielded LLE calls to 019226. This avoids
equating a compiled but unentered root with gameplay running natively.
Detailed evidence is private in `native-top-acceptance.json` and the four
`top-*` run directories. Revival items, first-boss rewards and these routes
are bounded checks; special mode-24 enemies, other bosses, natural campaign
progression, rollback and endings remain separate qualification work.
