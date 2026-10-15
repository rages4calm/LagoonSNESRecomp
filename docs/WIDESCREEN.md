# Lagoon widescreen (experimental)

Widescreen is optional and defaults off. Enable `Widescreen = 1` in `[Graphics]`
and select `WideAspect = 16:9`, `16:10`, `21:9`, `32:9`, or `window` in
`[Lagoon]`. Window mode follows the drawable aspect. The existing DisplayAspect
setting still controls pixel shape, so a CRT 4:3 presentation uses fewer source
pixels than square-pixel mode for the same monitor. Width is bounded by the
framework's 800-pixel PPU capacity, with letterboxing beyond that capacity.

This renders additional world tiles at their original pixel scale. It does not
stretch the native image. BG3 text/HUD remains centered at native size. Titles,
menus with other PPU layouts, parallax rooms, BG1/BG2 mosaic, and unsupported map modes keep
their native field. Expansion is currently enabled only for the exact resource
combinations qualified in Atland (room 0), the guard exterior (room 2), and Gold
Cave (room 3). Matching PPU
layout alone is insufficient. Other rooms use a clean centered native fallback.
Room loading (`$100E`) and BG3 modal overlays also force that fallback; actual
VRAM-write observation catches loading/menu changes within a simulated frame.
The official presentation compositor also blanks only the margins whenever the
completed frame selected native fallback, preserving all 256 native columns.
RAM can name the next room while the completed raster still shows the previous
room, so expansion requires three consecutive ready resource observations (two
prior completed boundaries). Loading, resource changes and timeline jumps reset
that settlement. A 391-present cave-entry audit includes the previously corrupt
single frame and checks clean fallback throughout the unsupported exterior,
then expansion once the cave raster is settled.

## Read-only map reconstruction

The game retains 128-column byte metatile maps at the 24-bit pointers in direct
page `$88` (BG1) and `$8B` (BG2), and four-word metatile definitions at pointers
`$69` and `$75`. Routine `$01:8311` resolves a world pixel as:

```
metatile = map[(world_y >> 4) * 128 + (world_x >> 4)]
word_offset = metatile * 8 + ((world_x & 8) >> 2) + ((world_y & 8) >> 1)
tile = definition[word_offset]  // little-endian word
BG2: tile &= 0xDFFF
```

Out-of-map coordinates use the game's `$52` fill metatile. The resolver reads
WRAM/LoROM directly without ticking the CPU bus. It fills the framework's
host-only `WsShadow` store; guest WRAM, VRAM, OAM, DMA queues, camera and CPU
state are untouched. Actual PPU scroll supplies pixel phase; only the lost
high bits come from the game's camera mirrors. Renderer history resets on
scene transitions and timeline resets.

Native tiles continue to use guest VRAM. The map sampler was checked against
23,040 interior native tile entries across 16 ordinary gameplay checkpoints,
with zero mismatches. The wider renderer preserved the complete native center
frame and guest state at all 18 checkpoints of the existing 7,219-frame route
in each of 16:9, 16:10, 21:9 and 32:9 checks. With the default CRT pixel aspect,
the captured source fields were respectively 342, 308, 448 and 682 pixels wide,
all 224 pixels tall. These are bounded town/opening checks, not campaign
qualification or proof of every room type.

The normal 4bpp renderer opts into exact native boundaries: its tile spans split
at x=0 and x=256 before selecting native or shadow tiles. This matters when a
fine-scroll tile straddles the right edge and the native stream still holds a
different tile from the retained room map, especially with immediate camera
following. Only margin pixels use the reconstructed tile; the native fragment
keeps actual VRAM. Other games retain their existing behavior. The ROM-free
`tests/ppu/ppu_shadow_native_boundary_test.c` regression covers all eight scroll
phases on both main and subscreen, reproduces the former corruption, and checks
that the fix preserves every native pixel and existing margin pixels.
The same regression checks the actual PPU serializer: the host mask is excluded
and the three snapshot chunks retain their existing byte counts. Shadow history
also resets when `RtlStateGeneration()` changes, including external timeline
loads that bypass the desktop reset hook and unsupported rendering scenes.

Lagoon parks unused sprites at x=320 without clearing their tile/y fields.
Strict margin hints prevent those stale menu/health sprites from appearing
when the viewport widens. Ordinary room NPCs (draw type 1) and nondeath enemies
(type 2) now have a bounded host presentation path. It captures entity records
and source OAM at the real `$7E:2000` OAM DMA boundary, checks the completed PPU
OAM fingerprint, then independently decodes the native metasprite cells. Every
eligible cell must match an original OAM slot before that frame can reveal any
extra pixels. Its host copy reuses that exact slot's priority/order and clears
all pixels inside the native 256-pixel viewport.

The copy reads actual PPU tile/palette data, including cell size, flips and
normal enemy damage flashing. It refuses the player's dynamic graphics pack,
unsupported/common forms, death animations, special/boss forms, rotated OAM,
and stale timelines. No guest entity, camera, graphics staging, OAM, DMA or CPU
state is changed. The generic DMA callback is read-only and occurs before the
transfer changes its source/count fields; HDMA does not trigger it.

The final isolated 21:9 checks use ordinary walking after real room
initialization; the cave prefix retains an explicitly declared guard bypass.
Eight town and thirty-nine cave checkpoints match the normal-view control in
the native center, WRAM, registers, VRAM, OAM, CGRAM, SRAM and guest clock.
The town retained/native map audit checks 8,640 entries with zero mismatches;
all five actual Gold Cave checkpoints match both maps. At exact captures,
independently selected OAM slots and actual VRAM/CGRAM reproduce 345 left /
231 right NPC pixels and 38 left / 346 right enemy pixels. The left cave
enemy is mostly hidden by the foreground wall; those 38 visible pixels are
counted separately from its 401 opaque source pixels. Other actor witnesses
match every opaque source pixel in the selected margins.

Earlier abrupt player/camera fixtures are actor-only evidence. Their jumps
bypassed native strip streaming, leaving guest blank `$01FF` where the room
map contained scenery. Matching that normal-view control did not certify
scenery. A separate camera-centering fault during natural walking was fixed
by changing the original thresholds and initialization offset, preserving the
game's own one-pixel movement and eight-pixel strip loaders.

A distinct host history issue also produced missing scenery entirely in the
margin: automatic circular-VRAM captures could mark uninitialized slots as
authoritative and prevent ordinary prefill from replacing them. Lagoon now
uses the existing explicit `WsShadowForceTile` API for its proven retained
maps. Native fragments still use actual VRAM, and other ports' default prefill
policy is unchanged. The former right black rectangle contains 5,632/5,632
exact retained-map/graphics/palette pixels and zero black pixels in the final
capture. The color audit uses the actual PPU scroll and SNES BG scanline offset;
an initially reported phase discrepancy was an audit error, not a runtime bug.
No extra raster-scroll policy was retained. The actual-shadow/actual-PPU
regression reproduces captured blank history and checks exact margin color,
native color preservation and reset.

Generate the bounded routes with
`tools/make_wide_entity_routes.py --natural --out <new-private-directory>`;
enable `camera_center` in both comparison runs. The default position-fixture
mode is only for actor diagnostics and is unsuitable for scenery screenshots.
`tools/verify_wide_entity_pixels.py` records exact-frame color witnesses for
independently validated actor OAM slots.

The town opening also exercised fail-closed native-cell mismatches: those frames
show no added actor pixels until validation succeeds. Reset cleared the host
object list. These are bounded presentation fixtures, not natural campaign
progression or post-reset gameplay qualification. Generate them with
`tools/make_wide_entity_routes.py --out <new-private-route-directory>` and use
`camera_center` for both comparison runs. `tools/verify_wide_entity_pixels.py`
records the exact-frame color witness for independently validated OAM slots.

## Wider actor buffer (batch 4)

Ordinary queued actors retain the original decoded-cell/OAM witness and exact
native-edge clipping. Farther out, a bounded host producer now supports NPC
kinds41/59 and normal enemy kinds0/1, after that kind has matched native OAM
in the current room. This is presentation only: it never extends guest AI,
collision, damage or entity simulation.

The producer recognizes the actual three-segment ordinary-room OBJ upload,
tracks ownership of every atlas byte, and rejects unknown later overwrites.
It reads the common animation table from the private ROM and advances a separate
presentation phase using the game's own main-loop clock. Returning to the
native queue synchronizes to the guest phase. A guest offscreen phase is not
claimed to run while culled. Room/source/timeline changes clear phase history,
kind witnesses and atlas ownership; loading a state without a fresh witnessed
atlas load safely hides far actors.

Deep-margin objects share the game's unsigned ground-Y order and entity-index
tie order. The near seam preserves exact original OAM slots, and added objects
have zero opacity throughout the native 256 pixels. Unsupported native objects
in the deep margin or an over-capacity buffer disable the far producer for that
frame. Death, bosses, special/player packs, unrecognized kinds, more rooms and
complete campaign coverage remain unfinished.

Actual32:9 walking captures include a far-right NPC with 216/216 opaque source
pixels matching current VRAM/CGRAM colors, and a far-left cave enemy with 476/476
matching pixels. Their origins are beyond the native[-96,352) horizontal active
buffer. Town/cave/menu paired routes cover63 tags with exact native framebuffer,
WRAM, registers, VRAM, OAM, CGRAM, SRAM and guest clock. The cave route declares
a guard-position fixture before room entry; movement inside the cave is normal
input. One initial cave kind1 frame-selector mismatch is rejected and revokes
its witness until a later matching native frame; it is not treated as a valid
far pose. These are bounded checks, not full campaign certification.

Inventory and Start overlays now keep clean native margins, including when the
menu writes its BG3 map after the frame starts. A stable inventory/open/return
route additionally checks that expanded scenery resumes after closing.

The guard exterior (room 2) exposed patterned white out-of-room fill and colored
edge corruption during batch 4. Its original native fallback is now superseded
by the bounded exterior support below. Other room expansion still needs its
own map/graphics/bounds and transition proof.

## Reproduce the map check

Use private ROM and state-dump paths. ROMs, states and raw probe captures stay
out of the source repository. The three explicitly requested curated fullscreen
PNGs in `docs/screenshots` are the screenshot exception:

```
python -P tools/verify_wide_map.py --rom <private.sfc> --dumps <private-dumps> --out <private-report.json>
python -P tools/qualify_native.py --exe <candidate.exe> --route tools/script_native_gameplay.txt --frames 8000 --config <private-config.ini> --screenshot-frame 6513 --out <new-private-output>
```

`-P` avoids the existing `tools/dis.py` shadowing Python's standard `dis` module.
Diagnostic state dumps intentionally contain only the native 256-pixel center;
`presentation.ppm` captures the entire expanded field. `LAGOON_WIDE_ASPECT` may
override the aspect for diagnostics without writing player settings.

The ROM-free entity regressions compile the actual DMA and PPU implementations:

```
powershell -File tools/test_wide_entities.ps1 -OutDir <private-test-output>
```

They cover stream grids/escapes/refusal, actual native-versus-host PPU colors
for all four flips, native-center clipping, dynamic-player-pack refusal, and
DMA callback timing/state preservation. Additional tests cover pure animation selection, segmented atlas ownership,
room/source/timeline invalidation, modal fallback and exact audited-resource
acceptance/refusal. These synthetic checks complement
runtime parity; they do not certify all campaign entities.

Future work: additional far actor kinds and rooms, special/death/boss
composition, parallax BG2 reconstruction, room-bound policy, HUD anchoring, first
visit/room transition and save-load checks across more of the campaign. Follow
the framework's `docs/WIDESCREEN_PATTERNS.md` invariants during that work.

## Batch 4 acceptance binding

The bounded menu/far-actor/transition checks bind to
`lagoon-batch4-settled.exe`, SHA256
`904d218f875e4ebd877e9f5df4b85aecf901efbdc70fd7b04c26d9822fcf752b`.
The 63 tag comparison includes seven binary components plus guest clock; the
391-present audit separately preserves the native center and checks transient
margins. The ROM-free regressions cover native-center/pitch padding preservation
and scene settlement across room/load/timeline changes. Public fullscreen
captures have their separate source/display provenance in `docs/screenshots`.

## Guard exterior boundaries (batch 5)

The 512 by 512 guard exterior now shows additional scenery inside its real room
extent. Pixels beyond that extent are black in the added margins; the native
256 columns remain unchanged. This prevents the opaque patterned fill 29 from
appearing as white/yellow garbage beyond the map. Black outside the small room
on an ultrawide display represents its actual boundary.

The exact room 2 sources are BG128000, OBJ1A8000, definitions068E90 for both
layers, and maps7F0000/7F4000. Direct-page words53/55 specify dimensions in 8-pixel
units; this fingerprint requires both 64. The existing beam-driver read-only
observer records actual horizontal/vertical scroll on every rendered line.
The final compositor masks added pixels using these raster coordinates and the
real extent, including the SNES scrollY+1 first output row. Missing raster rows
or an unsupported layer/resource combination retain clean native fallback.
The three-field settlement, loader/menu gates, atlas ownership and actor rules
from batch 4 remain in place.

`tools/script_wide_outdoor.txt` uses ordinary input for both horizontal bounds,
Start/inventory, closing, the town door in both directions and reset. It has no
RAM writes or debug positions. The separate cave-entry transient fixture still
declares its original guard bypass before entering the cave.

Three scenery samples account for 161,952 added pixels inside the map:
161,920 match independently decoded map/VRAM/CGRAM background colors; the other
32 are the exact pixels and actual colors of uploaded guard OAM slots 0/1,
whose Y242 wraps across the output top. Their coordinate-set hashes match all
32 background occlusions. This is bounded source/pixel evidence, not full
campaign coverage or new art.

The qualification harness now disables physical keyboard/gamepad seats only
in each private run config. Script input remains active. This prevents live
controller presses from contaminating replays. `--live-input` explicitly opts
into merged input, and `--input-trace` records actual guest input words; the
receipt records the isolation setting and effective config hash.

Reproduce the outdoor check with private inputs:

```
python -P tools/qualify_native.py --exe <candidate.exe> --manifest <matching-manifest.json> --route tools/script_wide_outdoor.txt --frames 10000 --features camera_center --config <private-wide.ini> --env LAGOON_WIDE_ASPECT=32:9 --env LAGOON_WIDE_ENTITY_REPORT=entities.txt --input-trace --screenshot-range 2650 3100 --out <new-private-output>
python -P tools/verify_wide_outdoor.py --run <wide-output> --bounds <wide-output>/entities.txt.bounds.tsv --control <normal-output> --rom <private.sfc> --texture-tag outdoor-left --texture-tag outdoor-right --texture-tag outdoor-middle --out <private-report.json>
```

The trace contains captured row anchors and the checks fail rather than guess
if these fixtures gain nonuniform per-line scrolling. The renderer itself
retains each row's coordinate separately. Inventory remains native-width;
redesigning title/intro/menu layout is separate unfinished work.

Batch 5 presentation acceptance binds to `lagoon-batch5-top.exe`, SHA256
`13ce550c4ccdf0f5fc214abe9067e89e5dbbce31e70d1d8734922e063b35e439`.
The isolated ordinary-input route has 35 checkpoints matching all seven binary
components and guest clock with widescreen off/on and the retained 904d baseline.
The cave transient has 40 matching checkpoints against its retained isolated
normal control. All 2,505 actual presentation captures preserve the native center:
451 walking, 922 menu, 741 doorway and 391 cave-entry captures. Across those runs,
102,875,872 margin pixel observations required to be black by room bounds or
native fallback are black. These are repeated observations, not unique assets.
The previously corrupt frame 2864 is included. Start/inventory are visibly open
at full brightness with black margins; closing restores 57,344 mapped margin
pixels. Both ordinary town-door directions and reset are exercised.

`tools/test_wide_entities.ps1` additionally applies the production bounds mask
to actual PPU-rendered pixels, preserving in-map colors and every native column
while removing out-of-map colors. The source captures above precede optional GPU
visual filters; fullscreen/display provenance is recorded separately. Failed
probes that merged physical input with scripts are excluded from acceptance.
