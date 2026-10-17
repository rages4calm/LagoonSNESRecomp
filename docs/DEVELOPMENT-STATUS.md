# Current development checkpoint — October 17, 2026

The accelerated campaign batch is complete. Resume from its receipts and the
remaining candidate regressions, rather than repeating the first-town research.

## Selected playable program

`build/LagoonSNESRecomp.exe`, frozen as `build/lagoon-campaign-door.exe`, SHA256
`5b17f7704f6b8ea8eb2231f9addd68b5582ef917c37f7a6d51cc69540f1262bc`.
Manifest SHA256:
`b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`.
It retains 190 AOT variants, 18 declared LLE variants and 47 promoted gameplay
roots. The new campaign warp facility is disabled during ordinary play.

The debug-disabled original first-boss fixture matches the prior build at 100
checkpoints and all 4,521 ordered presentations, including guest memory and
clocks. Saves and settings remain protected. The interpreter safety net remains.

## New coverage

- All 73 rooms with ordinary incoming doors load and settle in isolated tests.
  Cloud's script-only entry and record 3F remain outside that static entry sweep.
- Seven main bosses show real HP loss, death, original rewards and completion
  flags. Separate fixtures capture actual player deaths in all seven encounters.
- Six endgame phases exercise Shell, Green Spirit, Zerah, Thor, Bird and Red
  Spirit. Original transformations, eight orbs and earned completion bits are
  recorded. Red's independent entry uses disclosed prerequisite flags.
- All six Thimale guardians die and unlock the boss. Canonical stock-ROM
  coverage proves six executions of both PLA instructions and the counter
  increment in the exceptional nonlocal death escape.

These are explicit cheat-assisted encounter fixtures, not an uninterrupted
natural campaign. No enemy HP or victory result is written in accepted kills.
Credits beyond entry into the ending handler remain unqualified.

See [boss evidence](../tools/CAMPAIGN-BOSSES.md),
[endgame evidence](../tools/CAMPAIGN-ENDGAME.md),
[warp contract](../tools/CAMPAIGN-WARPS.md) and
[coverage ingestion](../tools/CAMPAIGN-COVERAGE.md).

## Native candidate: not selected

The clean room coverage was actually ingested and used to generate and compile
a private 348-AOT candidate, preserving the 190 existing variants. Structural
camera and interpreter gates pass. A small SDK patch adds mode-qualified timing
selection while retaining byte-identical legacy generation.

Runtime research fails strict CPU/memory/clock equality, and an Ella route has
one different presentation. No crash occurred in the five bounded probes, but
that does not qualify the candidate. Do not copy it over the playable build or
claim 348 variants are deployed. Raw coverage and generated C remain private.

## Next work

1. Isolate the candidate's first timing difference and Ella frame mismatch.
   Keep the known program as control and promote only reproducible changes.
2. Use the expanded canonical coverage to target native death/revival and boss
   work with actual entry modes and proven stack continuations. Keep 00:80E6
   interpreted; observed targets alone do not prove return contracts.
3. Continue source-proven renderer and widescreen transition coverage. The live
   custom renderer still covers stable Atland; reference rendering covers other
   scenes. Verify frame-time spikes without image logging or overlapping tests.
4. Qualify natural progression, the script-only cloud scene, credits and more
   hardware. An optional in-game map is on TODO after coverage priorities.

State format 3 and both raster journals remain mandatory. Smooth scaling stays
removed; Enhanced original pixels is optional. Full retexturing remains shelved.
No visible accelerated test windows were opened.

The public framework pin stays `a6be9756285f26ccc74539c811d4d23d57d8c33d`.
The cumulative patch reconstructs 38 paths from that clean pin. The local
framework has deliberate history and dirty source: do not reset/bootstrap it.
The project-parent resume pointer and campaign handoff contain private receipt
paths. Publish only substantial verified source batches; no ROM, generated C,
audio, saves or raw captures belong in the public tree.
