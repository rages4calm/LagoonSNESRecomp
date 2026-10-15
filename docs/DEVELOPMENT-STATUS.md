# Current development checkpoint — October 15, 2026

Development is paused at the user's request. Resume from the verified batch 9
implementation; do not restart completed research or discard local framework
changes. No further gameplay changes were made after batch 9 qualification.

## Selected build and evidence

- Native executable: `build/LagoonSNESRecomp.exe`; immutable copy:
  `build/lagoon-batch9-optimized.exe`.
- Executable SHA256:
  `81bf8a4bf685fca8994836e3701499677ad9afded0139673b5732e269d97b814`.
- Manifest SHA256:
  `b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`.
- 190 AOT variants, 18 declared LLE variants, 47 promoted gameplay roots.
  Counts are not a native-completion percentage.
- Live stable-Atland renderer: 2,166 replacements, zero mismatches, 146 exact
  checkpoints and 7,185 exact ordered presentations against renderer-off.
- Damage, death/revival and first-boss fixtures: 1,350 exact checkpoints and
  19,603 exact ordered presentations against retained batch 8.
- Audited outgoing fades: 46 formerly narrow fields expanded; final-build
  checkpoint/presentation binding preserves earlier direct pixel provenance.
- Hidden, normally paced 32:9/Enhanced run: 60.10 FPS average, composition
  0.356 ms, p99 18.23 ms, two intervals over 25 ms. No locked-60 claim.

Details: [batch receipt](../NATIVE-BATCH9-2026-10-15.md),
[renderer](CUSTOM-RENDERER.md), [performance](PERFORMANCE.md),
[widescreen](WIDESCREEN.md), [native damage](../tools/DAMAGE-BATCH9.md).

## Resume priorities

1. Profile remaining capture/reference cost and intermittent presentation spikes.
   Preserve audio safeguards and normal guest timing. Measure without per-field
   image logging, readbacks or concurrent heavy tests.
2. Extend live custom rendering to source-proven partial-brightness, cave and
   outdoor scenes. Preserve complete raster, resource and actor ownership gates.
   Continue finding unsupported widescreen transitions and menu/script gaps.
3. Continue native death/revival and remaining boss logic only after proving
   actual entry paths, CPU widths and nonlocal stack continuations. Keep the
   opening `00:80E6` interpreter floor.

## Continuation constraints

The reference PPU still runs and every custom replacement must match all pixels;
visual similarity between town and cave is expected. This is not a D3D12 backend.
Current native coverage and renderer coverage remain partial.

Use private test save roots and preserve owner saves/settings. State format 3
requires both raster journals; old/incomplete states refuse. Smooth scaling was
removed at the user's request; Enhanced original pixels remains optional. Do not
resume the shelved full-retexture experiment.

Keep local framework changes. Public upstream pin remains
`a6be9756285f26ccc74539c811d4d23d57d8c33d`; do not reset/bootstrap the deliberately
dirty local submodule. Push substantial verified source batches. Never publish
ROMs, generated C, extracted graphics/audio, saves or raw test captures.

The private project-parent `HANDOFF-NATIVE-BATCH9-2026-10-15.md` contains precise
paths, rejected alternatives and test receipts. `active-native-work.txt` selects
the evidence directory. No binary release or install is pending.
