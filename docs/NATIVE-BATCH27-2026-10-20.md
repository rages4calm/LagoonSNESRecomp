# Batch27: alternate player animation and boss death timers

Two complete original M1/X0 bodies now run natively: `01:AAF6` alternate/fall
player animation and `01:B573` boss death progression. The latter retains the
legacy generated name `Lagoon_ActorLleBoundary_b573`; that name no longer means
its M1/X0 body is interpreted. Other entry widths retain their interpreter floor.

AAF6 updates the original eight-frame timer, eight-stage animation and shared
`AB1D..AB28` sprite tail, including the final `TRB $0500` that clears bit40.
B573 updates three staggered timers and saturation counters, calls the original
cleanup service at its actual `B5BF → B936` JSR, and tails the existing native
`B7D4` rewards body when all three progress counters reach bit10. The source
tables, thresholds, arithmetic and original frames are preserved.

The `B936 → 0080E9` inline dispatch service remains interpreted. Its existing
M1/X0 exit fact and six already native table targets remain intact; inline data
at `B93D..B95C` is excluded from code. An initial generation with unnecessary
B936 boundary/force overrides suppressed that analysis and produced 403 native
bodies. It was rejected before compilation. Removing those overrides preserved
the original policy; no manual replacement exit assertion was added.

The selected manifest has **410 AOT /112 analyzed LLE /289 roots**. All 408 prior
native dispositions, all 392 prior exit facts, 23 exit-mode sets and four HLE
guards remain. The two expected old-body changes are native target selection
inside A7D6 and the B573 tail inside the boss dispatcher ACAA. Existing camera,
recovery, source/mode/stack/generation guards and all 22 wait-continuation
descriptors remain unchanged. These counts are not completion percentages.

## Qualification

The actual generated AAF6/B573 bodies and their actual selectors pass
**179,712 original-opcode comparisons**:

- 54,592 complete returns.
- 35,264 genuine child-entry boundaries.
- 89,856 deadline-before-entry yields, with no native instructions executed.

The oracle executes **3,456 complete original B936/0080E9 service calls**, covering
all 16 source-owned table selectors through the real JSR/JSL/PLA/JML/RTS frames.
Unknown selector/hidden accumulator-high contexts stop at the original B936
entry. Rewards and other unsupported children likewise stop at their real entry;
no result or return is fabricated. Comparisons cover all 128KiB RAM, registers,
flags, hidden accumulator high byte, saved/dead stack and CPU/master/coprocessor
clocks. Timer/stage thresholds, saturation, byte wrapping and decimal arithmetic
are included. The unit bus has a constant eight-master-cycle access cost; live
scheduler/APU behavior is checked separately.

Fresh installed-prior-build controls and the coherent candidate pass three strict
matched pairs, including every ordered picture and all checkpoint guest state
and clocks:

| Assisted route | Ordered observations | Eight-component checkpoints | Scope |
| --- | ---: | ---: | --- |
| Ordinary boss40 defeat | 2,911 | 2 | B573 interpreter entries 96 → 0; B936 stays 6 → 6 |
| Seeded alternate player animation | 218 | 2 | AAF6 interpreter entries 65 → 0; timer0/stage8 and bit40 clear match |
| Existing stock guardian route | 4,681 | 2 | Regression; no B573/B936 entries on either build |
| **Total** | **7,810** | **6** | **All exact** |

Release AOT trace hooks are disabled. Entry removal together with the reviewed
native dispatch establishes source-closed native-use inference, not a direct
native-routine counter. The alternate-animation route explicitly sets player
state3 and timers on an existing fixture; it is not a natural fall/death proof.
The stock guardian pair is not positive coverage for the new bodies.

The qualified executable SHA256 is
`0994da88d8c51dcb0647048d03cdc849133869703b6866e7eedce5e4efee4b32`;
its manifest SHA256 is
`2e09a00070df702f3123b2d0a75711d28d606def1b34ef7a0a15b3be72be1bfb`.
Controls used the prior `91d8c011…effb1` executable and `63abf343…4d598` manifest.
The local compact native receipt binds source hashes, the opcode result and the
three strict comparisons. Portable local inputs pass three dependency-only dry
runs with zero launches/new states; three Python AST checks pass.

## Reproduction and limits

From the repository with Clang, Python, the project's cumulative SDK changes,
matching locally generated code and your own pinned clean ROM:

```powershell
python -P tools/test_fall_boss_death_oracle.py --rom "<owned ROM path>" --gen src/gen --out "<fresh oracle output>"
python -P tools/test_player_death_oracle.py --rom "<owned ROM path>" --gen src/gen --out "<fresh compatibility output>" --compile-only
```

The older player-death oracle retains an explicit AAF6 entry-stop sentinel;
complete AAF6 execution belongs to the new oracle. The compatibility invocation
extracts and compiles current bodies and executes zero opcode cases. Runtime
comparisons require local assisted routes and fixture states, which stay private.

The interpreter safety net remains enabled for original inline services,
unsupported modes, special waits and nonlocal/endgame paths. This batch does not
qualify every boss child, the B90B ending/credits loop or an uninterrupted
campaign. The owner and friends retain the full playtest.
