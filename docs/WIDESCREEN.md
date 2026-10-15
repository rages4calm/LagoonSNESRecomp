# Lagoon widescreen (experimental)

Widescreen is optional and defaults off. Enable `Widescreen = 1` in `[Graphics]`
and select `WideAspect = 16:9`, `16:10`, `21:9`, `32:9`, or `window` in
`[Lagoon]`. Window mode follows the drawable aspect. The existing DisplayAspect
setting still controls pixel shape, so a CRT 4:3 presentation uses fewer source
pixels than square-pixel mode for the same monitor. Width is bounded by the
framework's 800-pixel PPU capacity, with letterboxing beyond that capacity.

This renders additional world tiles at their original pixel scale. BG3
text/HUD remains centered at native size. Audited field resources currently
cover Atland (room 0), the guard exterior (room 2), and Gold Cave rooms 3–6.
Known dialogue and Start/status overlays retain that scenery when its original
map and graphics remain intact. Items/Magic panels have a separate guarded
original-art frame compositor; see [menu presentation](MENU-PRESENTATION.md).
Titles, unsupported intro layouts, other menus, parallax, BG1/BG2 mosaic and
unknown room resources keep a clean centered native fallback. Matching a PPU
mode alone is insufficient. Room loading (`$100E`) normally forces fallback;
the completed-upload authority described below permits the verified Atland
opening fade. Actual VRAM-write observation catches resource changes within a frame.
The official presentation compositor also blanks only the margins whenever the
completed frame selected native fallback, preserving all 256 native columns.
RAM can name the next room while the completed raster still shows the previous
room, so expansion requires three consecutive ready resource observations (two
prior completed boundaries). Loading, resource changes and timeline jumps reset
that settlement. A 391-present cave-entry audit includes the previously corrupt
single frame and checks clean fallback throughout the unsupported exterior,
then expansion once the cave raster is settled.

## Batch 7: continuous Atland opening fade

The initial Atland view previously remained narrow until `$100E` cleared,
although the game had already loaded its scenery and was fading that complete
world into view. The loader clears this flag at `018769`, after the final fade
call at `018762`. Widescreen now accepts this earlier world only after observing
the entire real BG upload: 14,272 sequential bytes from WRAM `7F:7F80` to VRAM
word zero, in the original four DMA segments. A DMA start alone gives no authority.

This witness binds to room, resource source and timeline generation. The actual
completed PPU characters must match those uploaded bytes, and initialized native
map cells must match the retained room layout. All other scene geometry, modal,
resource, settlement and final compositor guards still apply. Partial uploads,
changed resources, unsupported character ownership and timeline jumps refuse
expansion. No game memory or simulation timing is changed.

On the final candidate `lagoon-batch7-raster.exe`, SHA256
`03fcc1f6eb0fef35c9a3427bf36d1c6aa36adf8faed9103f465d0f670f35b2c2`,
73 opening checkpoints match native center, WRAM, registers, VRAM, OAM, CGRAM and
guest clocks with widescreen off/on. Across all 51 actual presentations covering
the fade, the native center remains exact. Both native world and expanded world
first become visible at frame 1603, removing the previous 44-field delay.
All 4,198,656 added pixel observations across those 44 newly widened fields
match independently decoded map/VRAM/CGRAM colors. Fade brightness is inferred
only from the native picture, never fitted to the new margins. The earlier
candidate also passed 1,191 consecutive actual presentation center comparisons.
The final executable additionally passes 35 outdoor checkpoints (both ordinary
town-door directions, Start/inventory/close and reset), 46 actual cave-door
checkpoints across rooms 4/5/6, and 11 dialogue-route checkpoints with
widescreen off/on. The all-nine-QoL real NPC dialogue also matches the retained
batch-6 control at 11 checkpoints. Complete expanded pictures at cave frame
4871 and dialogue frame 6320 are byte-identical to their previously source-audited
controls. These final bindings reuse the broader earlier presentation sequences;
they do not claim new campaign coverage or display performance.
The Atland title card itself retains native layout; this change expands its
subsequent real world, not the title lettering or unsupported intro artwork.

Gold Cave's fresh-load maps reference animated torch characters 944..949 beyond
the base BG pack. Their real animation upload is `01DF88` through `008F19`,
ROM `10:8900/89C0/8A80` to VRAM word `3B00`, length `C0`. Until these separate
completed animation uploads have independent ownership, early cave fades remain
conservative native fallback. Previously qualified settled cave scenery and
actor rules remain available. This is not universal first-frame expansion.

The per-word graphics observer also checks character address and ownership
before scanning modal BG3 text. Unrelated sprite/font words avoid that scan;
the full resource invalidation checks are retained. No frame-rate claim follows
from this ordering change.

Reproduce with private inputs and the matching executable manifest:

```
python -P tools/qualify_native.py --exe <candidate.exe> --manifest <matching-manifest.json> --route tools/script_wide_initial_world.txt --frames 5000 --features walk_speed,start_hp,camera_center,sword_reach,follower_speed,recovery_time,jump_hold,magic_power,exp_boost --config <private-wide.ini> --env LAGOON_WIDE_ASPECT=32:9 --env LAGOON_WIDE_ENTITY_REPORT=entities.txt --screenshot-range 1600 1650 --out <new-private-output>
python -P tools/verify_wide_opening.py --run <wide-output> --control <normal-output> --baseline <batch6-wide-output> --rom <private.sfc> --source-tag opening-064 --out <private-report.json>
```

`tools/test_wide_entities.ps1` exercises the production completed-upload tracker:
full byte ownership, partial/reordered writes, room/generation changes and
unexpected overwrites. Private raw captures and ROM inputs remain outside source.

## Batch 6: camera, dialogue, and three more cave areas

Qualification uses `lagoon-batch6-integrated.exe`, SHA256
`14be32d86a553a27f47971bf5479f9900d225c832f6ee5dc016b870d1423a507`.
These are bounded checks with private, isolated input and save directories.

The optional camera previously ran before player movement/collision and checked
only the current direction. A blocked player could remain three pixels off
center; moving frames could sit five pixels away and snap back when stopping.
The guarded interpreter dispatcher now runs after the player's actual entity
collision and checks both axes. It calls the original one-pixel scroll loops,
strip loaders, parallax/lock checks and room clamps. It never teleports camera
WRAM from a host frame callback. The collision routine's return stack is
unchanged, including its nonlocal damage paths.

Projectiles already move and queue before the player. A precisely guarded
same-value `$0519` store marker therefore refreshes every already-queued screen
projection and ground-Y sort key from its actual world coordinates and the
new camera. It does not repeat projectile movement, AI, damage or queue insertion.
The marker requires its exact interpreted instruction and original caller stack.
The whole queue's types, indices, alignment and capacity are validated first.

The natural stop/turn/wall route has 61 checkpoints: every held/idle interior
sample stays at (128,112), including eight stopped wall-contact frames. All
player world coordinates match the previous camera build. A separate disclosed
fixture seeds one ordinary projectile record, then moves normally: 33 actual
projectile screen/sort-key witnesses match world minus camera. Its 40 checkpoints
and the 61 natural checkpoints preserve guest components, native framebuffer
and clock exactly between widescreen off/on. Reproduce with
`tools/script_camera_batch6_stop_turn.txt`,
`tools/script_camera_batch6_projectile_fixture.txt` and
`tools/camera_batch6_report.py`.

Dialogue authorization uses actual background ownership. A first known field
captures only character IDs referenced by the complete retained room map,
including its fill. Later BG3 overlays may retain scenery only while those
character words are unchanged and initialized native BG1/BG2 map interiors
still match the retained descriptors. This excludes BG3's `$3C00` tilemap,
which aliases unused background character slots. Inventory replacements do
not inherit permission from stale room pointers. Resource changes and timeline
loads clear this witness. Without a prior valid world witness, text falls back
conservatively. The natural NPC conversation has 11 matching state/clock
checkpoints and 164 actual paired presentations: the native center is exact
and both margins retain scenery, including 68 frames with the live text box.
Start/status over town scenery was also captured with intact margins.

Rooms 4–6 use the same audited graphics/definition resources as room 3. Their
actual extents are 768×256, 1536×1280 and 512×256 pixels. They now use the same
completed-raster boundary mask as the guard exterior: added pixels outside the
real room are black; native columns remain untouched. The door fixture places
Nasir near source-derived door coordinates, then uses real movement, collision,
warp and the full room loader. It does not write room IDs, camera, maps, story
flags or HP. This is staged presentation testing, not earned campaign progress.
It enters 4, 5 and 6, then returns from 6 to 5.

Across 46 checkpoints, widescreen off/on preserves all guest components and
clock. Across 1,501 actual paired presentations, all native centers match;
89,535,898 added pixel observations required to be black by loading or room
bounds contain zero leaks. Three complete margin samples account for 218,999
in-map pixels: 218,523 match independently decoded background colors and the
remaining 476 exactly match the ordinary enemy's actual OAM/VRAM/CGRAM colors.
Its coordinate hash equals the entire set of background occlusions. Two samples
have no actor occlusion and match all 95,424 / 57,344 background pixels.

Black beyond finite room boundaries and genuine map void remains intentional.
Unknown rooms, special/death/boss forms, unwitnessed graphics and unsupported
parallax remain gated; this does not certify the whole campaign or all actors.
ROM-free routes `tools/script_wide_dialogue_batch6.txt` and
`tools/script_wide_cave456_fixture.txt` reproduce the interactions and doors.
Raw captures, ROM input and snapshots remain private.

## Earlier map and actor qualification

### Read-only map reconstruction

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
retains each row's coordinate separately. In batch 5 inventory remained
native-width; batch 6 adds the separate guarded Items/Magic compositor described above.

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
