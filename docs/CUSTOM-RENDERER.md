# Bounded custom scene renderer

The interpreter safety net remains in place. `src/lagoon_scene_renderer.c` adds
a bounded live software scene backend for stable Atland, exact Gold Cave03/04/05/06
and room07 with independently owned parallax, with Widescreen
enabled. It captures its
own scene packet, independently draws a field, and compares every 32-bit output
pixel with the existing presentation before allowing replacement. Unsupported
fields and any mismatch retain the existing picture. This is original pixel art
and scene geometry, not new art or a D3D12 uploader.

The integration defaults to **auto** for supported stable packets. **Verify**
computes and checks the candidate without replacing the picture. Implementation
and ROM-free unit tests alone do not prove live gameplay coverage; bounded runtime
acceptance is recorded separately. There is currently no launcher selector for
this diagnostic.

Batch22 adds stable room07 with exact1024x1024 dimensions, BG129E89,
OBJ1A906F, definitions069720 and WRAM50=2. BG1 keeps the existing circular
native tilemap and independently owned world map. BG2 samples its own copied
VRAM32x64 tilemap at7800/7C00 using its independent quarter-speed scroll anchors;
it never borrows BG1 coordinates or the ordinary BG2 world-map path. Existing
compressed11FB7D source and both decoded-page hashes are mandatory. Exact PPU
layout71/7A/3C, full-word room, source pointers, stable field, generation,
character/palette/OAM data and whole-reference equality remain required.
Margins beyond the finite BG1 world are explicitly black after compositing.
No new actor producer or loading/fading authority was added.

Fresh same-executable off/auto comparisons on candidate`dde68792632ccea8...`
with manifest`32e4c02646d6b8d2...` cover positions, finite edges and
original06<->07 doors, with disclosed near-door/position fixture assistance:

| Route | Ordered presentations | Replacements | Independent draws | Source pixels |
| --- | ---: | ---: | ---: | ---: |
| Room07 positions |1,305|1,055|515|78,675,520|
| Room07 edges |783|533|253|38,650,304|
| Original06<->07 doors |1,164|494|230|35,136,640|

All twelve checkpoints match seven components, all3,252 ordered presentations
agree and there are zero renderer mismatches. The2,082 replacements and998 draws
sum overlapping route observations, not unique room frames. Unsupported/loading/
fading fields use the original raster. These checks establish neither complete
room coverage nor a performance improvement; the reference raster still runs.

The helper adds1,863,680 synthetic independent BG2 scalar pixel checks across
four anchor phases and256/342/682/800 widths, plus1,863,680 full-composite finite
edge assertions. Optional external owned-ROM and actual room07 VRAM arguments
exercise a real source/page-positive packet,32 entry refusals, four mid-raster
refusals and a generation refusal; no game bytes are embedded. Run the existing
ROM-free test executable with `<private.sfc> <room07.vram.bin>` for that additional
source-guard check. These vectors complement rather than replace live off/auto
qualification.

Batch21's exact cave04/05/06 extension is qualified, selected and installed.
Batch20 previously added stable cave03 to Atland.
The new descriptors require the same exact BG128B77, OBJ1D8000, both069298
definition pointers and7F0000/7F4000 maps, with independently enumerated finite
dimensions: room04=768×256, room05=1536×1280, room06=512×256. No generic room range
or shared-art assumption grants authority. Existing PPU, immutable packet,
generation, actor and full-word reference equality checks remain mandatory.

Three same-executable off/auto routes enter through original real door loaders,
with disclosed near-door position assistance. The combined chain matches1,478
ordered presentations and five checkpoints with795 replacements. Separate routes
ending in room04 and room05 match449/814 presentations and two/three checkpoints,
with207/357 replacements. Each added room has a final actual replacement. Total:
2,741 ordered presentations across three overlapping routes, ten checkpoints
with all seven components exact,1,359 replacements,454 independent draws,
34,780,032 drawn pixels and zero mismatches. These counts are route observations,
not unique frames. The narrow finite rooms also exercise
outside-world clipping. These are bounded route results, not complete room or
campaign coverage. No fading authority, actor producer, new art or speedup was
added. The reference render still runs for every checked present. A separate
hidden1080p EPX real-audio traversal without interior diagnostic dumps measures
60.02539Hz,p9518.2993ms,max22.8731ms and zero intervals above25ms across1,328
intervals. Eleven audio high-water waits remain. No fullscreen, locked60 or
renderer speedup claim follows from this bounded measurement.

The ROM-free helper additionally passes three exact descriptor positives,
69 altered-resource/dimension/layout refusals, three generation refusals and
458,304 synthetic finite-bound pixel checks, alongside all prior vectors. These
synthetic tests do not substitute for the real-door route comparisons above.

Batch20's previously selected cave extension remains supported. Its exact room03
descriptor requires BG128B77, OBJ1D8000, both definition pointers069298,
maps7F0000/7F4000 and finite1792×1632 dimensions. Other caves are not admitted by
similarity. The same-executable off/auto fixture matches646 ordered presentations
and both checkpoints' seven state components, with626 replacement presentations,
311 independent renders,23,825,088 independently drawn pixels and zero mismatches.
Loading and fading remain fallback; no extra actor producer or authority was added.
These measurements qualify this route, not all cave scenes or improved frame rate.

## Live packet and safety boundary

The actual raster observer captures the first visible line and requires all 224
visible lines in order, one timeline generation, stable registers, palette/OAM,
and unchanged VRAM/OAM write counters. A final VRAM and extra-object check rejects
changed sources. The caller also supplies its existing audited world authority
and rejects HDMA. Loading, color math, windows, mosaic, rotated OBJ order, other
rooms and unknown resource layouts fall back. A cold load/reset invalidates the
packet before another complete field can be accepted.
Atland's original fill beyond its physical room extent is not generalized by
this backend; a difference there keeps the original presentation through the
same pixel gate.

The packet owns copies of retained world maps, ROM metatile definitions, resident
VRAM, CGRAM, OAM, and validated extra-object pixel cells. It contains no borrowed
guest pointers and no framebuffer. BG1/BG2 in the added columns are resolved from
world coordinates. Within the original 256 columns, BG tiles are decoded from the
copied native circular tilemap to preserve guest upload history, including its
last-row streamer boundary. This is a source-data compatibility rule, not a copied
framebuffer row. BG3 remains native size and centered. The packet also captures
actual OAM placement/hint/grace policy after raster evaluation and requires it to
remain stable throughout the field. This preserves authorized native edge
partials and moving left-margin cells without inventing another motion heuristic
or unparking unhinted objects. Native OBJ ordering, tile flips, priority,
and copied presentation objects are composed independently. The result is cached
per completed packet and output width for repeated pause/interpolated presents.
Decoded 2bpp/4bpp source rows and RGB15 conversions are cached, and map/tile
lookups are hoisted across eight-pixel spans. Consecutive fields can reuse the
independent candidate only when every captured rendering input is byte-identical
and the previous candidate belongs to the immediately preceding packet at the
same output width. New clocks and fields are still simulated normally.
Every present still checks its entire reference field before any output write.
The comparison includes the high byte and both widened margins, not only RGB or
the native center. There is no gameplay, asset, PPU-register or guest-RAM write.
The reference renderer still runs in this qualification stage. Independent
replacement establishes correctness and live coverage; it does not yet remove
the reference rendering cost or establish a performance improvement.

Private controls:

```text
LAGOON_SCENE_RENDERER=off|verify|auto
LAGOON_SCENE_RENDERER_REPORT=<private report.json>
```

`off` skips the backend, `verify` checks without replacing, and `auto` replaces
only exact candidates. Diagnostics accumulate in memory and write once at normal
process exit, avoiding per-field file flushing. `packets` counts stable captured
fields, `renders` and `independent_pixels` count actual independent drawing,
`verified_presents` counts exact full-field checks, and `replaced_presents` counts
actual output replacements. `refused_packets`, `mismatches`, and `fallbacks` are
separate. Reports bind to executable/routes through the qualification receipt;
they do not by themselves establish frame-rate or campaign coverage.

ROM-free focused test:

```powershell
clang -std=c11 -Wall -Wextra -Werror -I snesrecomp/runner/src tests/lagoon_scene_renderer_test.c -o <private-test.exe>
& <private-test.exe>
```

The vectors exercise planar source decoding, a stale native bottom-row upload
with independently retained margins, full-word comparison, immutable
captured data, render caching, padded rows, mode/generation/reset refusal,
incomplete raster lines, register/palette/write-counter changes, and malformed
objects. The synthetic vectors do not qualify real room resources.

## What the prototype actually renders

The tool consumes private ROM and framebuffer/WRAM/VRAM/OAM/CGRAM/register dumps
from a known stable Atland field. It reconstructs BG1/BG2 from world coordinates,
retained room maps and metatile definitions, rather than the SNES circular
screen tilemap ring. Four-plane character data and original palette colors come
from the actual resident graphics. The original BG3 text/HUD is drawn at native
size from its UI data. The prototype implements the audited mode-1 priorities,
transparency, tile/object flips and native object order.

Native objects use the actual uploaded OAM, including Nasir's resident dynamic
staging graphics. Ordinary NPC/enemy cells can enter added columns only when the
current guest draw queue, ROM frame decoder, complete OAM cell and resident
object character range agree. This is existing uploaded presentation evidence;
it does not infer AI, recover far animation history or claim every object form.
Unmatched parked OAM cannot appear in the margins.

The current strict entry contract accepts only Atland room 0 with its audited
resource pointers, full brightness, stable mode 1, native BG3 coordinates and
known OBJ layout. Loading, other rooms, color math, windows, HDMA, mosaic and
unsupported layouts are refused. Running Python with `-O` is also refused because
the snapshot contract uses assertions. Source/reference mismatches write
`accepted: false` and exit unsuccessfully.

## Bounded reference evidence

The live C backend on the optimized batch9 candidate `81bf8a4bf685fca8…`
independently rendered and actually replaced these fields with **zero mismatches**:

| Actual route | Internal field | Replaced fields | Independent redraws | Independently drawn pixels | Reference fallbacks |
| --- | --- | ---: | ---: | ---: | ---: |
| Fresh-state restore, all nine gameplay QoL options | 342x224 | 66 | 22 | 1,685,376 | 20 |
| Fresh-state restore, all nine gameplay QoL options | 682x224 | 66 | 22 | 3,360,896 | 20 |
| Opening, Atland movement and NPC dialogue, camera centering only | 682x224 | 1,486 | 391 | 59,732,288 | 4,937 |
| Turns, wall movement, menu and reset, all nine original QoL options | 682x224 | 548 | 202 | 30,859,136 | 42 |

The fallbacks include startup/loading/unsupported fields. The figures count
actual replacements, not merely eligible packets: **2,166 replaced fields**,
**637 independent redraws**, and **95,637,696 independently drawn pixel
observations** across the four controls. Exact-source cache reuse explains why
redraws are fewer than replacements; every replacement still compares its whole
reference field.
All seven guest snapshot components/clocks matched at **146 checkpoints**, and
all **7,185 ordered presentations** matched renderer-off controls exactly.
These are bounded routes, not campaign coverage. The private
`work-native-20261015-164402/optimized-scene-*-acceptance.json` reports bind executable,
manifest, scripts and both replay receipts. The accepted executable SHA256 is
`81bf8a4bf685fca8994836e3701499677ad9afded0139673b5732e269d97b814`;
manifest SHA256 is `b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`.

A separate hidden, normally paced 32:9 OpenGL/Enhanced Pixels off/default-auto
pair used the selected optimized executable. Default auto actually replaced
another **1,772 fields**, from **844 independent redraws and 128,936,192
independently drawn pixel observations**, with zero mismatches and 20 startup
fallbacks. Both runs ended with identical guest components/clocks. The shader
runs after the internal 682x224 pixels are composed. Neither run used a per-field
image log/readback, and renderer reports were written only at exit. Auto measured
60.10 fps with 17.83/18.23 ms p95/p99 and three intervals above 20 ms, two above
25 ms. See [PERFORMANCE.md](PERFORMANCE.md) for the off-control outlier, measured
costs and limits; correctness does not imply a performance improvement over the
reference renderer alone. The earlier uncached candidate `fd6044196a395c18…`
and its higher-cost performance result remain labeled historical evidence.

The actual NPC dialogue snapshot at guest frame 6318 reproduces all **57,344
native pixels** and all **152,768 pixels of the 682x224 expanded presentation**.
The source-backed ordinary NPC has two cells in OAM slots 27/28. Original HUD,
text, player sprites and scenery are compared, not merely the ground layer.

A separate fresh v3-state restore snapshot at guest frame 6137 also reproduces
all **57,344 native pixels** and **152,768 expanded pixels** of the first
post-load field. This includes one queued ordinary NPC cell in OAM slot 0.
These are two stable snapshots, not motion/campaign or performance qualification.
Private reports bind all RAM/graphics/register inputs, ROM, native framebuffer
and optional expanded reference with SHA256. PNGs and input assets remain private.

Reproduce with private inputs:

```powershell
python -P tools/scene_renderer_prototype.py --snapshot <dump-prefix> --rom <private.sfc> --width 682 --reference <actual-present.ppm> --out <fresh-private-directory>
```

The output contains `scene.png`, `scenery.png` and a UTF-8 `report.json`. The
prototype neither edits nor distributes the supplied ROM, graphics or state.

## Remaining renderer work

1. Expand supported room/object families and partial-brightness transitions while
   preserving retained-resource ownership and complete reference comparison.
2. Add raster changes incrementally, then
   measure sustained gameplay performance using logging-free hidden tests.
   A D3D12 uploader, shader or larger framebuffer alone does not establish a
   correct game-state renderer.

Higher-resolution art, perspective changes and a full 3D conversion are separate
projects. This experiment retains the original pixel assets and scene geometry.
