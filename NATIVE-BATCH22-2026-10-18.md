# Batch22: native boss refill continuations and room07 renderer

The exact qualified build is installed with **382 AOT /119 analyzed LLE /263 roots**. These are
routine counts, not completion percentages. The interpreter safety net remains.

Executable SHA256:
`dde68792632ccea8bb80436f630188cf40161ea551bd352349c23c88ed4d9138`.
Manifest SHA256:
`32e4c02646d6b8d25740860e432c921d08e37e904a31dab05ee5c72ead12fbf4`.

The tested executable/generated outputs were moved into the normal runtime
without rebuilding or making backups. A cold installed centered-camera partial
refill replay matches32 component hashes across four checkpoints. All seven
owner ROM/settings/save inputs remain unchanged; the SDK index is preserved.

## Boss refill ownership

Four M1X0 segments now run natively: B806 after rendering, B80F's original HP
refill call, B812's original MP refill call and B815's saved-X restoration.
Together these contain five guest instructions. The substantive change is
resuming the native orchestration across genuine waits; the HP/MP arithmetic
was already native. B7FE/C643 rendering, B80B's wait boundary, B816's boss-script
dispatcher, special death B43E and SPC/render recovery9346 remain interpreted.

An optional SDK continuation hook witnesses the real wait return and original
saved-X/outer caller bytes. It permits at most two ancestor stages, with exact
source, mode, stack, generation and ordered-phase checks. Unknown, stale,
nested, non-suspending and changed-state cases fall back. No guest frame or
healing result is fabricated, and no audio wait is skipped. The null hook keeps
the existing framework behavior.

Centered-camera mode changes the actor call into an existing balanced35-byte
dispatcher. The new guard accepts that exact approved code as well as the
stock call, retaining the real80CD outer return. An initial guard that accepted
only the stock call left the centered-camera boss route interpreted; the final
candidate exercises the native path in both modes.

## Verification

- Independent generated review: all378 prior native bodies, existing nodes
  and exit-width facts are unchanged. Exactly four new native bodies/roots are
  added; unsupported widths, original boundaries and opening0080E6 stay LLE.
- 23,040 comparisons of the actual four generated bodies with original ROM
  instructions pass complete RAM, architectural registers/flags, saved/dead
  stack and CPU/master/coprocessor clocks. HP/MP children stop at their actual
  callee entries in this oracle; full refill waits are checked separately live.
- 281 registration/source checks and1,510 production ancestor/source-policy
  cases pass, alongside the extracted SDK token matcher and framework test.
- Eleven fresh old/new strict pairs match44,579 ordered presentation
  observations and26 complete eight-component/clock checkpoints: seven boss
  wins, revival, final-boss player death and two depleted-HP/MP boss cases.
- All seven assisted wins witness the actual main-owner native refill sequence.
  The alternate render-owner frame is covered by source/policy tests; no live
  take is claimed for it. Report destinations are advisory metadata only;
  gameplay inputs, complete outputs and clocks remain strict.
- The two new refill cases set HP128/MP0 once, then use original recovery to
  reach255/255, with both stock and centered camera. DEF1000, STR8191, equipment
  and position assistance remain explicit. The older continuous-HP fixtures
  alone would not prove partial recovery. No healed/result bytes are written.

These are bounded, assisted checks. Owner and friends perform the uninterrupted
natural campaign. There is no full-native or campaign-completion claim.

## Room07 custom renderer

The live software renderer adds exact stable1024x1024 room07, including its
independent quarter-speed parallax BG2 and static VRAM pages. It retains exact
resource, map, palette, raster, generation and finite-world bounds. The existing
renderer still supplies a whole-field pixel reference before replacement;
unsupported/loading/fading fields retain their existing presentation.

Three final same-executable off/auto routes cover positions, edges and original
06↔07 doors. They match3,252 ordered observations and twelve seven-component
checkpoints, with2,082 replacements,998 independent draws and zero mismatches.
Routes overlap; these are not unique frames or complete-area coverage. Helper
checks separately compare1,863,680 parallax pixels and1,863,680 finite-edge
pixels, with source/raster/generation refusals. No new actor or fade authority,
new artwork, D3D12 backend or renderer speedup is claimed.

A separate hidden1080p OpenGL/EPX16:9 real-audio run enters07 through original
03→04→05→06→07 doors with disclosed positioning. Its stable07 sample spans550
intervals:60.0884 updates/sec, p9517.6789ms, max18.7177ms and zero above25ms.
The sole end dump is outside the sample. This is not a fullscreen/locked60 claim.

See [renderer details](docs/CUSTOM-RENDERER.md),
[frame pacing](docs/PERFORMANCE.md) and [current status](docs/DEVELOPMENT-STATUS.md).

The cumulative framework patch changes only the bridge C/header and its
existing policy-test section;66 other sections are byte-identical. Reverse
application checks pass. The intentionally modified SDK/index are preserved;
the SDK Gitlink is not staged. Latest checked upstream remains b894555,
already integrated additively in batch20. ROMs, generated code, fixture states,
audio and private captures remain local.

Next: broader original boss-script/death recovery ownership and the next exact
renderer family, then specific reported transitions. Optional door/exit mapping
and zoom-out remain considerations after native/custom renderer work.
