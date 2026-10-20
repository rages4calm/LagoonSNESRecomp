# Batch28: remaining boss motion table

The remaining 15 targets of the original `02:9036` boss-motion dispatcher now
have native M1/X0 bodies. Eight contain motion/state work; seven are genuine
single-RTS slots. The previously qualified first-landing target `02:906A` remains
unchanged. This completes native target selection for this authored 16-way motion
table, while original RNG, unknown pointers, unsupported modes and other service
boundaries retain the interpreter safety net.

| New substantive body | Original work |
| --- | --- |
| `0290A1` | Eardon HP-dependent motion flags |
| `0290BE`, `02911C` | Duma hand velocity/bounds, including shared backward flow |
| `02914C` | Thimale motion/guardian-unlock state |
| `0291A3` | Battler component movement sequence |
| `029245` | Bird position sequence |
| `029312` | Thor-orb position sequence |
| `0294AF` | Ella teleport positions and delay, with original RNG calls |

The original RTS entries are `0290A0`, `0290BD`, `0291A2`, `029242`, `029243`,
`029244` and `029525`. Legacy `LleBoundary` names remain aliases; manifest
dispositions identify actual native eligibility.

Source traversal preserves `911C`'s backward branches into `90EE/90F4` while
already in M0/X0, followed by the original `AND 00FF / SEP20 / RTS`. No new manual
exit-width assertion replaces those instructions. ROM tables are excluded from
code. `009800/009848` RNG remains original interpreted code using its existing
exit fact and real three-byte JSL/RTL frames.

The manifest is **425 AOT /112 analyzed LLE /304 roots**, not completion
percentages. All 410 old native dispositions remain; 409 old bodies are exact and
only the motion dispatcher's target calls change. All 393 prior mode facts,
23 exit-mode sets, four HLE guards and 22 continuation descriptors remain intact.
No SDK, camera or recovery policy change was needed.

## Qualification

Actual generated bodies and the actual dispatcher pass **466,944 original-opcode
comparisons**:

- 222,944 complete returns.
- 10,528 genuine child-entry boundaries, including 10,240 unknown pointers.
- 233,472 deadline-before-entry yields, executing no native instructions.

The oracle executes **8,704 complete original RNG service calls** rather than
mocking random results. It compares all 128KiB RAM, modeled device registers,
CPU registers/flags, hidden accumulator high byte, saved/dead stack and
CPU/master/coprocessor clocks. Cases include decimal arithmetic, coordinate
wrap/bounds, signed distance, table indices, phase limits and real RTS slots.
The unchanged first-landing target stops at its genuine entry in this oracle.

The deterministic unit bus uses eight-master-cycle accesses and immediate divider
quotient availability. It proves the promoted source operations and modeled
effects; it does not certify physical divider latency, interrupts or APU timing.
Focused runtime comparisons separately check actual guest clocks and state.

Fresh prior-build controls and the coherent candidate pass:

| Assisted route | Ordered observations | Eight-component checkpoints | Original interpreted entries → candidate |
| --- | ---: | ---: | --- |
| Eardon | 5,621 | 2 | `90A1`: 280 → 0 |
| Duma | 4,306 | 2 | `90BE`, `911C`: each 121 → 0 |
| Stock Thimale guardians | 4,681 | 2 | `914C`: 2,162 → 0 |
| Battler | 5,941 | 2 | `91A3`: 633 → 0 |
| Ella | 2,911 | 2 | `94AF`: 428 → 0 |
| Original Zerah defeat / Thor-orb idle | 6,962 | 2 | `9312`: 5,319 → 0 |
| **Total** | **30,422** | **12** | **All pictures, checkpoint state and clocks exact** |

Release AOT trace hooks are disabled. The reviewed compiled dispatch and removal
of original interpreted entries provide native-use inference; these are not
direct native routine counters or a live RNG-call count.

The accepted endgame route uses the original loader and original Zerah defeat.
Both subsequent checkpoints show Thor kind11 at 256 HP and eight original kind13
orbs. It stops with Thor alive. Separate bounded attempts using Thor HP1 and
player positioning/attack-state cheats did not reach Bird; those attempts are
excluded from acceptance. **Bird `029245` has complete opcode proof but no live
positive qualification in this batch.** The original Thor death handler `019382`
and Bird initializer call at `019392` are not covered by the accepted route.

The qualified executable SHA256 is
`feef9b2199a3f93ca9dabda87eaf924ce9fd0fa71fd91bcedd22abbc2cb38600`;
manifest SHA256 is
`8071eb6c28ab93805384253e7941df5e5c20626bbc5421b9acb77a8e64f72030`.
Controls used prior executable `0994da88…ee4b32` and manifest `2e09a000…be1bfb`.
Compact local receipts bind the opcode result, source hashes and six strict
comparisons. Native portable inputs pass six dependency-only dry runs and three
Python AST checks; no new retained fixture state is required.

## Reproduction and remaining boundaries

From the repository, with Clang, Python, the cumulative SDK changes, matching
locally generated code and your own pinned clean ROM:

```powershell
python -P tools/test_boss_motion_oracle.py --rom "<owned ROM path>" --gen src/gen --out "<fresh oracle output>"
```

`-P` avoids local tool names shadowing Python standard-library modules. Assisted
runtime pairs additionally require private fixture states and matching route,
configuration and diagnostic inputs; those assets and raw captures stay local.

This batch qualifies the motion table, not every boss child or endgame service.
The original inline dispatcher, special waits, nonlocal escapes, Thor
transformation and B90B ending/credits loop retain their existing interpreter
boundaries. It does not establish an uninterrupted campaign or locked frame rate.
The owner and friends retain the full playtest.
