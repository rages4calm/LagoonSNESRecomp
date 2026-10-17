# Current development checkpoint — October 17, 2026

Native batch 11 is selected. Resume from its timing policy and receipts rather
than repeating the initial campaign research. See [batch 11](../NATIVE-BATCH11-2026-10-17.md).

## Selected playable program

`build/LagoonSNESRecomp.exe`, frozen as `build/lagoon-native-batch11.exe`, SHA256
`864bcaf8dbb4b4c9f2d0ab8f54cb00e5d4c4bfa54162eba6419f3c391e8ab495`.
Manifest SHA256:
`8b783324ff54638f2fe92dd97d0a1474db0bd267dae5c2c64d9a024495bf72ac`.
It has **326 AOT variants, 121 declared LLE variants and 215 analysis roots**.
All 190 previously selected native bodies remain, with 136 additional variants.
The interpreter safety net and original opening-dispatch repair remain enabled.
The campaign warp facility is disabled during ordinary play.

## Fixes and evidence

Precise native code previously double-charged interpreter-owned indirect
instructions and entered callees before synchronizing completed transfer timing.
The staged timing policy fixes both native and paired-interpreter call boundaries.
Ella's transient rendering difference is resolved in the selected program.
The centered-camera guard also now permits an unrelated safe actor helper.
See [timing contracts](../tools/NATIVE-TIMING.md).

Twenty newly proposed bank-00 modes remain interpreted, together with
`01C2CD:M1X0` (actor events/sound) and `02D1C5:M1X0` (graphics). Those exact
modes still alter interrupt/beam timing when promoted. Qualified native siblings
are preserved. Selection uses real generated exclusions, with no runtime deny
override or comparison masks.

**95/95 strict replay comparisons** match the prior selected program across
**135,478 ordered presentations and 1,704 complete checkpoints**, including
registers, WRAM, VRAM, OAM, palette memory, guest clocks and beam position. This includes
73 room entries, seven main-boss wins and seven player deaths, the six guardians,
endgame fixtures and the original fresh-start first-boss route. Fixtures reuse
explicit teleports, bounded player-stat cheats and prerequisite flags; these
checks are not an uninterrupted natural playthrough or independent hardware parity.

The 79 fresh canonical stock-ROM coverage captures were actually collected and
ingested by the SDK with zero journal loss or audit warnings. They contain
779,504,296 interpreted instructions and 2,617,808,165 interpreted guest cycles.
Compared with the same prior fixtures, interpreted instructions fall by
45,022,423. This is fixture-specific work, not a completion percentage.
Cloud's script-only room 2A entry, record 3F classification and the full credits
sequence remain unqualified. See [coverage workflow](../tools/CAMPAIGN-COVERAGE.md),
[boss checks](../tools/CAMPAIGN-BOSSES.md) and [endgame checks](../tools/CAMPAIGN-ENDGAME.md).

Public generation policy includes reviewed analysis roots and two timing lists;
private coverage files are not needed to reproduce the program. The cumulative
framework patch reconstructs all 47 affected source paths from upstream pin
`a6be9756285f26ccc74539c811d4d23d57d8c33d`. All 44 project Python tests, the
production QoL C test and 23 focused Python tests plus the C mode parser on a
fresh reconstruction pass. Seven protected owner save/settings files are unchanged.

## Next work

1. Investigate the 22 deferred modes with instruction-level timing traces;
   retain fallback until their interrupt/display contracts are proven.
2. Continue measured native death/revival and remaining boss orchestration.
   Keep `00:80E6` interpreted; observed targets alone do not prove return contracts.
3. Expand source-proven custom rendering and widescreen transition coverage.
   Stable Atland uses the live renderer; other scenes still use reference output.
   Measure frame-time spikes without capture logging or concurrent tests.
4. Qualify natural progression, script-only cloud entry, credits and more hardware.
   The optional source-backed in-game map remains on TODO after coverage priorities.

State format 3 and both raster journals remain mandatory. Smooth scaling stays
removed; Enhanced original pixels is optional. Full retexturing remains shelved.
No new locked-60-FPS or campaign-wide renderer claim is made by this batch.

The local framework has deliberate history and dirty source: do not reset or
bootstrap it. The public submodule pin remains unchanged. Private receipts and
the parent handoff preserve the old selected build and exact tested binary.
Publish substantial verified source batches only; no ROM, generated C, audio,
saves, raw captures or test binaries belong in the public tree.
