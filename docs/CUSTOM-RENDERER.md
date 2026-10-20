# Bounded custom scene renderer

The interpreter safety net remains in place. `src/lagoon_scene_renderer.c` now
recognizes all 75 original room descriptors (`00..4A`), including seven exact
layered scenes and the authored boss arenas. It captures its own scene packet,
draws it independently and compares every 32-bit pixel with the reference
presentation before replacement. Exact source, layout, generation, native OAM
and raster guards remain required. Unsupported fields retain the reference.
This uses the original pixel art and finite game geometry.

The integration defaults to **auto** for supported stable packets. **Verify**
computes and checks the candidate without replacing the picture. Implementation
and ROM-free unit tests alone do not prove live gameplay coverage; bounded runtime
acceptance is recorded separately. There is currently no launcher selector for
this diagnostic.

## Batch 29 full room catalog and remaining ordinary actors

The [room matrix](RENDERER-COVERAGE.csv) records all 75 exact BG/OBJ resources,
definitions, dimensions, animation owners and ordinary actor kinds. Existing
qualified contexts remain supported. Six further layered contexts
`0C/12/13/28/29/2A` own two independent original 2KiB BG2 pages with quarter-speed
camera anchors; none borrows 07's map. Actual room 12 cameraY1824 means wrapped
BG1 scroll 800 and quarter BG2 scroll 456. Both compressed sources/decoded pages,
full current/captured resource contexts and raster phases must match. Room 07's
original camera bounds remain unchanged.

All 17 original animation descriptors have exact sequence contracts and 32 room
owners. Length and source stride are independent: E010 copies 160 bytes at 128 byte
spacing, E030 has six ping-pong entries/four distinct sources, and E008 keeps its
original delay 7. Other owners retain delay 4 and their original phase limits.
The final correction adds only 32/33/34 ownership of E050/DD80, two 192 byte sources
10B980/10BA40 for characters 944..949. Fresh loaders show both resident phases;
the sampled WRAM maps do not use these high characters. This closes a source
contract omission, without claiming a visible animation defect in those samples.

NPC/enemy kinds are restricted to exact source rooms. Authored script NPC0/1 and
Lagon NPC8 additionally require the original entry-script and D20C source pins.
The global NPC0/1 follower producer has separate original loader/table, slot 800
and 04E1 flag guards. Original C1C6 metadata includes 07's NPC priority override.
Room 21 NPC0/33 and the selector-zero path have source qualification, without a
live-far claim for every story branch or escort.

Status 4 enemies (including 13/14/29) and type 5 props can render an **unchanged
individual guest pose** after genuine native OAM anchors that same entity. The
witness binds the full 64 byte record except checked camera-derived nativeXY and
the draw-queue bit, exact room/resources/generation, complete atlas/palette and
signedDP90. Props use original frame 3E and height correction. Changed frame,
timer, worldXY, subtype, status, source or graphics revokes that authority.
No B131 AI, reward/death code or special attack prediction runs for presentation.
Unseen/changed special far poses, dynamic Nasir art and unowned cells keep their
native fallback. Source proof does not mean every actor is visible in the margins.

Type 3 multipart bosses and type 4 projectiles retain their original native OAM
producer. The independent renderer composes that real OAM and verifies its whole
picture. Arenas 40..48 and story rooms 49/4A have an authored 256 pixel world width
(44 is 512 pixels high); columns beyond those finite worlds stay black. No larger
arena or invented world is implied.

Outgoing/fade packets now recognize all exact descriptors, while binding the
captured maps, full tilemaps, characters, actor sources and generation to the
actual raster. New/unknown character or tilemap writes revoke the outgoing
packet before an incomplete incoming upload can borrow it. The old 07->06
source-owned 136 byte queue exception remains confined to its original contract.
Incoming partial brightness uses the completed-source reference path; independent
C replacement still requires full brightness. Room 17's sampled Ella portrait/text
screen is original fixed Mode 0 UI and correctly retains native presentation.

Broad runtime evidence uses executable`4fbe90fc6ee8e8a0...` and manifest
`e375af907ee11273...`. Final executable`c0b78249eb5c3381...` differs only by the
three 32..34 animation-owner rows:117/118 objects, including every generated,
native and core object, are byte-identical. Independent source review transfers
unaffected evidence explicitly; the three affected rooms were rerun on final.
Earlier runs are not relabeled as final-binary runs.

| Bounded route group | Ordered presentations | Checkpoint locations | Custom replacements |
| --- | ---: | ---: | ---: |
|40 remaining loaders, broad 4FBE|34,440|40|23,500|
|19 actual-spawn near/far routes, broad 4FBE|35,227|38|27,564|
|Original script 2A / synthetic resource 3F, broad 4FBE|1,722|2|1,190|
|Real doors 09->08 /26->44 /28->29, broad 4FBE|4,428|6|2,954|
|Affected 32/33/34 loaders, finalC0B|2,583|3|1,818|

Every off/auto pair matches ordered expanded presentations and guest checkpoint
components/clocks, with zero accepted reference mismatches. Fresh installed FEEF
controls also match all eight checked guest components and clocks for the 40
loaders,19 actor routes and final three affected loaders. Isolated 2A/3F are
same-candidate controls;3F is explicitly a synthetic resource selector, not
natural campaign reachability. Intentional old/new margin changes and diagnostic
mode/report-path differences remain visible in receipts.

Actual far observations include NPC16/47/61, enemies 5/6/7/8/11/12/14/23/30/35 and
prop 3. Kind 14 includes 263 status 4 far-cell observations; prop 3 has 366. These are
observed cells, not unique actors. Assistance changes only Nasir position in 8 px
steps with existing camera-centering QoL and protects HP/DEF; no actor records
or quest outcomes are fabricated. Targets 9/10/13/26/24/34 did not establish
target-positive far evidence;28's assisted path entered boss 48. Those roles are
source-qualified, with no complete live-far claim. Native-frame witness refusals
are conservative gates, not accepted render mismatches. The failed 0C door
approach is excluded from transition coverage.

Public tests use the verified owner ROM without embedding its bytes:
`tools/test_wide_remaining.ps1`, `tools/test_wide_parallax_sources.ps1` and
`tools/test_wide_follower_source.ps1`. Their bounded source/policy evidence is:

- 28,512 actual selectors/poststates;23,556 complete original OAM calls/221,779
  exact cells, including 3,600 prop calls and bounded status 4 current poses.
- 649 original DF88/008F19 DMA jobs;96,242 descriptor/table/source/resident-byte
  mutants and 5,025 all 75-room/17-descriptor ownership checks.
- 14 actual decompression/queue/NMI DMA jobs for seven layered scenes; independent
  source/page mutants and 68,705 full-camera/wrapped-raster checks.
- 2,400 original follower initializations,14,400 selector/draw chains/72,900 cells;
  17 actual script spawns and full original draw chains.
- 75 exact descriptors,41 fresh stable source snapshots and one native-UI refusal;
  19,102,720 scalar source pixels/25,053,952 finite-edge pixels.
- 21 synthetic unchanged-pose positives/20 source-family fixtures and 5,878 pose
  refusals;39 actual-source outgoing packets/8,775 incoming-fade protocol checks
  and 200,802 map/character/source/generation/layout/order refusals.

Source-oracle ABI fixtures, synthetic policy and real assisted routes are separate
evidence. They do not certify a full native CPU port, an uninterrupted campaign,
hardware timing, fullscreen scanout or sustained 60 fps across all rooms. The
owner/friends' natural playtest remains the campaign check.

## Batch28 rooms22..25 and original NPCs

Four further original contexts use the independent scene backend. Exact current
and captured room/resource pointers, dimensions, both definitions, maps7F0000 /
7F4000, ordinary WRAM50=0, generation, full brightness and no loading remain
required. Native raster, OAM/atlas/priority ownership and full-field reference
comparison remain authoritative. Finite map edges are clipped.

| Room | BG source | OBJ source | Both definitions | Dimensions in pixels |
| --- | --- | --- | --- | --- |
|22|14D33D|1ABF2D|06E2F8|1024x1024|
|23|12B297|1AAED9|06999C|512x512|
|24|12EE12|1DC859|06E5AC|768x768|
|25|149197|18AFB0|06B3C4|512x512|

Ordinary enemy kinds19/20 are admitted only in22. NPC kinds80..90 are admitted
only in24 with that room's exact original descriptor, including native seams.
Kind numbers are decimal; room numbers and resource addresses are hexadecimal.
All other actors in these new contexts retain native OAM. This keeps older
high-NPC restrictions and field/town whitelists intact. NPC/actor status, original
frame selection, bounded metasprite decoding, matching native coordinates/OAM,
complete atlas, palette and generation must qualify before extra columns appear.
Their native-center pixels remain transparent.

The original24 loader genuinely spawns all eleven NPCs80..90. A focused route
moves the player with eight-pixel position assistance and follows original
camera/map streaming, NPC movement and animation; NPC83 appears in the right
margin. Its50 recorded far-cell rows are live observations, not unique actors.
The other ten NPCs have original-opcode and synthetic ownership qualification,
with no live-far claim. The22 route similarly records genuine original enemies19
and20 in5,816 and1,021 far-cell rows. Both routes report zero native-frame
mismatches and zero unsupported deep-object gates. No actor records, quest flags
or outcomes are injected. The selected25 loader has no actors; scenery proof
is not a claim that all possible script-spawned actors are supported.

Only22 adds animation ownership: descriptor01:E028 /table01:DCFE selects three
128-byte sources10:A880 /10:A900 /10:A980, uploaded to VRAM word3B00. Four animated
characters944..947 require exact descriptor/table/source bytes, transfer bank,
length/destination/order, current room/source, generation and completed resident
phase. All three phases are positively observed live. Rooms23/24/25 have no new
animation owner:23 cannot borrow room08's animation despite sharing its BG
source. Existing animation lengths and phase counts remain unchanged. No fade,
loading or retained outgoing-scene permission is added.

Final coherent candidate`feef9b2199a3f93c...`, manifest`8071eb6c28ab9380...`, passes
fresh installed0994 controls and same-executable renderer-off/auto pairs:

| Route group | Ordered presentations | Checkpoints | Replacements | Independent draws |
| --- | ---: | ---: | ---: | ---: |
| Four original loaders22 /23 /24 /25 |3,444|4|2,468|612|
| Actual22 enemies and24 NPC83 near/far |2,788|4|2,299|958|
| Total overlapping observations |6,232|8|4,767|1,570|

These independently draw239,845,760 source pixels with zero mismatches. Every
same-executable presentation CSV column matches. All eight checkpoint components,
guest registers/clocks and optional APU traces match both off/auto and old/new.
Old/new strict reports retain4,383 intentional expanded-picture observations and
report-path differences; off/auto strict reports retain diagnostic mode/path
differences. No picture CRC is masked. Bounded internal captures are separate
from physical display or performance qualification.

Focused source tests execute4,864 actual original NPC/enemy selectors plus4,864
resulting-state checks,5,378 refusals and267 code-byte mutations. Independent
complete C1C6 orC214 ->8B71 ->8211 draw calls match1,810 original OAM cells across
317 admitted fixtures after exercising four enemy variant values;243 invalid/oversized
decoder fixtures refuse authority. The actual DF88 ->008F19 DMA oracle performs
15 complete three-phase jobs,1,203 descriptor/table/source/transfer/VRAM byte
mutations and39 protocol/context/animation-borrow refusals.

Separate production tests use actual ROM/loader resources with explicitly
synthetic placement for all13 NPC/enemy kinds:114 far cells,31 native seams,
2,400 transparent native-center pixels,5,107 kind/NPC/prop refusals and4,687
ownership/source/OAM/atlas/center refusals. Four real loader snapshots pass
1,863,680 independent background pixels,2,444,288 finite-edge pixels and132
resource/layout refusals, plus four generation refusals. These fixtures remain
distinct from the genuinely spawned live routes.

```powershell
.\tools\test_wide_additional_actors.ps1 -Rooms22To25 -OutDir <private-test-directory> -LoaderSnapshots <fresh-private-loader-root>
```

The focused wrapper verifies the owner ROM against`rom_identity.txt`. Private
assets, states and raw proofs remain local. This work does not certify a complete
campaign, fully native game, fullscreen scanout or renderer speedup. The
interpreter safety net remains enabled.

## Batch27 field scenes and ordinary enemies

Six further contexts use exact original resource ownership. Both current and
captured full-word room IDs, dimensions, BG/OBJ sources, both definition pointers,
maps7F0000 /7F4000, ordinary WRAM50=0, generation, full brightness and absence of
loading remain mandatory. Original raster and whole-picture verification remain
active; finite boundaries mask only added pixels.

| Room | BG source | OBJ source | Both definitions | Dimensions in pixels |
| --- | --- | --- | --- | --- |
|18|14ABBE|1A94FE|06CB0C|1536x1792|
|1D|14ABBE|1A94FE|06CB0C|1024x1024|
|1E|14C061|1AAED9|06DC14|1024x1024|
|1F|14C061|1AAED9|06DC14|512x512|
|20|14C061|19AF50|06DC14|1024x1024|
|21|14C061|1AED4A|06DC14|512x512|

Actor permission is narrower than scene permission: ordinary enemy kinds15/16
in18,17/18 in1E and21/22 in20, all decimal. Each requires the original selector,
metasprite decoder, native OAM/frame/position witness, complete resident atlas,
palette, priority and exact source context. Added cells keep native-center
pixels transparent. Room21's original script actually spawns NPC0/33 despite an
empty initial authored actor list. These NPCs, type5 props and all other actors
in these six contexts retain native OAM; no new seam/far permission is inferred.
The previously completed towns and late areas retain their existing scope.

BG14C061 in1E..21 has its own original01:E020 descriptor /01:DCE1 table.
Eight416-byte sources run from10:9B80 through10:A6E0 with416-byte stride, uploaded
to VRAM word3B00. Every descriptor/table/source byte, source bank/address, transfer
length/destination/order, generation and complete resident phase must match.
Storage accommodates eight bounded phases of at most512bytes; all earlier
room-specific two/three/four-phase limits remain unchanged. Actual live loader
observations include all eight phases in1E/1F/20; the selected21 script holds
phase0. Independent original DF88 ->008F19 tests exercise all eight phases in
all four contexts,160 complete configured DMA jobs,10,240 byte mutations and110
context/order/generation/old-phase-boundary refusals. Rooms18/1D gain no animation
owner and older rooms cannot borrow E020.

Final coherent candidate`0994da88d8c51dcb...`, manifest`2e09a00070df702f...`, passes
fresh matched controls against installed91D8 and same-executable off/auto pairs:

| Route group | Ordered presentations | Checkpoints | Replacements | Independent draws |
| --- | ---: | ---: | ---: | ---: |
| Six original loaders |5,166|6|3,662|1,002|
| Three actual-enemy near/far routes |3,761|6|3,003|1,154|
| Total overlapping observations |8,927|12|6,665|2,156|

These independently draw329,367,808 source pixels with zero mismatches. Every
same-executable presentation CSV column matches; all eight checkpoint components,
guest registers/clocks and optional APU trace bytes match both off/auto and old/new.
Old/new strict comparisons remain false for6,089 intentional expanded-picture
observations and diagnostic report paths; off/auto strict reports retain the
mode/report-path differences. No picture CRCs are masked.

The assisted routes change only player position/stat protection, use original
loaders/camera/map streaming/AI and genuinely spawn all six enemy kinds. Recorded
far-cell rows are594/392 for15/16,1,508/86 for17/18 and60/811 for21/22. These are
cell observations, not unique actors. Actual enemy variant bytes are0 at the
captured checkpoints. One transient kind15 native-frame disagreement and27 deep
unsupported-object gates in the18 route refuse presentation authority; these
refusals remain recorded rather than being suppressed.

Focused source tests execute6,144 actual original selectors plus6,144 resulting
state checks,7,945 refusals and267 code-byte mutations. For four animation variant
values, an independent interpreter executes399 complete original C214 ->8B71
->8211 calls. All3,093 bounded admitted cells match position, tile, palette,
priority, flips and size;753 invalid/oversized decoder combinations refuse before
authority. Separate production observer tests use actual loader resources with
explicitly synthetic placements:117 far cells,14 seams,1,392 transparent native
pixels,7,674 kind/NPC/prop refusals and2,562 ownership/source/OAM/atlas refusals.
Six actual loader snapshots pass2,795,520 independently sampled background pixels,
3,666,432 finite-edge pixels and198 resource/layout refusals. These unit fixtures
are distinct from the genuinely spawned live routes.

Reproduce only this focused source scope with the verified owner ROM and fresh
matching private loader snapshots:

```powershell
.\tools\test_wide_additional_actors.ps1 -FieldOnly -OutDir <private-test-directory> -LoaderSnapshots <fresh-private-loader-root>
```

The wrapper pins`rom_identity.txt` SHA256 before compilation. No private assets,
raw captures or saves are published. These bounded internal-field checks do not
establish a complete campaign, fully native gameplay, fullscreen scanout or a
renderer performance improvement. The interpreter safety net remains enabled.

## Batch26 NPC areas and original animation

Four connected NPC contexts now use the independent scene renderer. Each requires
its exact original descriptor, dimensions, both definition pointers, maps7F0000 /
7F4000, ordinary WRAM50=0, matching captured/current room and generation, full
brightness15 and no loading. Existing native-center, raster, atlas, palette,
priority and whole-picture comparison guards remain mandatory.

| Room | BG source | OBJ source | Both definitions | Dimensions in pixels |
| --- | --- | --- | --- | --- |
|19|13D745|1DB67E|06D090|1024x1024|
|1A|13D745|1DB67E|06D090|256x256|
|1B|13D745|1B8000|06D090|256x512|
|1C|12E794|1DB67E|06D984|768x1024|

Finite edges remain clipped. The 256-pixel-wide interiors do not gain invented
scenery outside their authored width. This batch adds no loading, partial-fade
or retained outgoing-scene authority.

Ordinary NPC animation and metasprites are admitted through exact per-room kind
lists: decimal69..78 in19;30/31/79/91 in1A;18 in1B;19..26 in1C. The bounded
128-entry kind witness accommodates these IDs. IDs above63 still require a new
room's exact descriptor and authored whitelist even for native-anchored seams;
older areas do not acquire broader actor permission. Far presentation still
requires a real native OAM/frame/atlas witness and matching projected native
coordinates. Presentation extrapolation does not run NPC AI or alter guest state.
The six enemy kinds qualified in batch25 retain their previous scope.

The actual assisted room19 route renders original NPC70 beyond native activation
in the right margin. It changes player positions in eight-pixel steps so the
original camera, map streaming, NPC movement and animation execute normally.
No NPC records, quest flags or results are manufactured. NPC91 has original-opcode
and synthetic ownership qualification; its conditional live appearance is not
claimed. The selected original room1C script removes its NPCs at this test's
story state, so its loader comparison qualifies scenery rather than live NPCs.

Town19 additionally owns fifteen animated characters through original descriptor
01:E018 and table01:DCB1. Two480-byte sources10:97C0 /10:99A0 upload to VRAM
word3B00. Only the exact room/BG source, descriptor, table, source bank/address,
length, destination, ordered complete bytes, generation and matching resident
VRAM authorize those phases. The bounded animation buffer grows to512bytes;
all earlier room-specific lengths, tables and phase counts remain unchanged.
Both original resident phases are positively observed in the live NPC route.

Final batch26 candidate`91d8c0119e304e2c...`, manifest`63abf343c3e27f2a...`, passes
fresh same-executable renderer-off/auto comparisons:

| Route group | Ordered presentations | Checkpoints | Replacements | Independent draws |
| --- | ---: | ---: | ---: | ---: |
| Four original loaders19 /1A /1B /1C |3,444|4|2,478|408|
| Actual NPC70 near/far route |1,155|2|905|364|
| Total overlapping observations |4,599|6|3,383|772|

These routes independently draw117,936,896 source pixels with zero mismatches.
Each loader has one final checkpoint and a complete ordered presentation stream;
the NPC route has two full eight-component checkpoints. All six old/new native
and guest checkpoints, registers and clocks remain exact. Old/new strict reports
remain false for1,938 intentional expanded-picture observations and diagnostic
report-path differences. No picture CRCs are masked. The NPC off/auto pair also
matches every presentation CSV column and all eight checkpoint components.

Source checks execute5,888 original NPC selector calls and5,888 resulting-frame
checks, plus1,272 refusal cases and267 code-byte mutations. An independent
interpreter executes368 complete original C1C6 ->8B71 ->8211 draw calls;880
actual OAM cells match the host decoder's position, tile, palette, priority,
flips and size, including NPC91. Original DF88 ->008F19 code configures ten
complete animation DMA jobs;2,908 byte mutations and14 protocol/context refusals
exercise the animation owner. Separate synthetic production-policy tests cover
23 actual-ROM NPC kinds,4,790 high-kind/old-scene refusals and329 context/source/
OAM/atlas/alignment refusals. Synthetic placements are not live spawning evidence.
Four actual loader snapshots additionally pass1,863,680 independent background
pixel checks,2,444,288 finite-edge checks and132 resource/layout refusals.

An internal expanded NPC capture has238/238 right-margin colors matching the
actual emitted cells and held resident atlas/palette. This is a bounded source
color witness, not same-field full guest-memory logging. These headless/internal
field checks do not establish fullscreen or new-area GPU performance, a complete
playthrough, or a fully native game. The reference raster and interpreter remain
active; no renderer speedup is claimed.

Reproduce the focused source tests with the owner ROM and fresh matching private
loader snapshots:

```powershell
.\tools\test_wide_additional_actors.ps1 -TownOnly -OutDir <private-test-directory> -LoaderSnapshots <fresh-private-loader-root>
```

The wrapper verifies the ROM SHA256 from`rom_identity.txt` before compiling or
running tests. Snapshots and other game assets are not included in public source.

## Batch25 actor extension

The exact twelve descriptors now permit ordinary native-anchored seams and
offscreen enemy types25/27/28/31/32/33 where their original source/frame rules
are proven. These are decimal actor kinds. Actual spawned enemies in rooms2C,
2F and35 exercise all six; the authored room lists contain no ordinary NPCs.
No synthetic NPC spawn is claimed as gameplay coverage. Specialkind29/status4
and all unsupported streams retain the original picture.

The captured and current scene contexts independently require original resources,
full brightness, no loading and matching generation. The real OAM-kind/frame
witness must agree with source-projected native coordinates, resident atlas,
palette, priority and cell bytes before offscreen pose extrapolation. Losing
that witness revokes authority. Full-word queue bounds include the high byte.
New extras keep native-center pixels transparent and finite map edges clipped.

The selector oracle executes22,016 actual original NPC/enemy selector calls and
checks22,016 actual resulting frame states, plus271 negative cases and267
source-byte mutations. A separate production-observer test uses actual ROM and
loader atlas data with explicitly synthetic OAM placement:62 farcells,20seamcells,
1,584 transparent center pixels and465 refused source/context/OAM/atlas cases.
Those fixture cells are distinct from the three actual-spawn live route pairs.

Final910F actual-enemy pairs pass4,821 ordered observations/18 checkpoints;
the original four-family door chain adds1,909/7, all with zero mismatches.
Twelve1CA5 loader pairs pass10,332/48 and preserve all native/guest checkpoint
components against old controls.214 expanded-image differences are intentional;
old/new strict comparisons remain false. Independent exact source/object transfer
binds the later queue-only refusal to final910F without claiming twelve reruns.
See [batch25](../NATIVE-BATCH25-2026-10-20.md) for build identities and limits.

## Batch24 background qualification

Batch24 adds twelve exact stable ordinary-room descriptors. Shared artwork alone
does not grant authority: each row requires its original dimensions, full-word
room ID, both definition pointers, maps7F0000/7F4000 and WRAM50=0.

| Rooms | BG source | OBJ source | Both definitions | Dimensions in pixels |
| --- | --- | --- | --- | --- |
|2C/2D/2E|15A43D|1B84D8|078C00|2048x1536 /1024x512 /1024x512|
|2F/30/31|15A43D|1BA17D|0794DC|2048x1536 /512x512 /512x512|
|32/33/34|15B8AC|1BA17D|079C14|2048x1536 /768x512 /1024x512|
|35/36/37|15C815|1CC387|07A0D8|2048x1536 /768x512 /768x512|

All require the observed PPU layout71/79/3C, full brightness15, stable original
loading state and the existing source, raster, generation and whole-picture
guards. Finite-world pixels outside the physical map are black after composition.
The proven negative-Y cache-key translation also covers these exact descriptors;
physical worlds never wrap. Loading/fading fields retain the reference renderer.
Batch24 initially used native OAM alone here. Batch25 extends guarded ordinary
actor seams and far enemies as described below; no retained-fade authority is
added. Existing0..08 behavior is preserved.

Rooms35..37 additionally require the original animation descriptor01:E058 and
table01:DD8B: four64-byte sources10:BB00/BB40/BB80/BBC0 uploaded to VRAM word3B00.
Complete transfer, source/table/clock, order, size, generation and resident-VRAM
checks remain mandatory. Guest animation clocks are not simulated.

Component qualification build`bcd237eee3f95b55...`, manifest`b7fc10141e0608d6...`, passes thirteen
fresh same-executable renderer-off/auto comparisons: twelve original assisted
room loaders and one original-door chain2C->2F->32->35. Each room has a positive
final replacement, including all three animated rooms. The chain retains seven
checkpoints across actual loading transitions, with explicit position assistance
at original door cells and no room/layout/quest/result flag writes.

| Route group | Ordered presentations | Checkpoints | Replacements | Independent draws |
| --- | ---: | ---: | ---: | ---: |
| Twelve original room loaders |10,332|48|7,263|1,572|
| Original cross-family door chain |1,909|7|976|309|
| Total overlapping observations |12,241|55|8,239|1,881|

All ordered full-width pictures and all seven checkpoint components match between
off/auto, with287,356,608 independently drawn source pixels and zero mismatches.
The reference raster still runs. Comparisons against the old installed controls
retain strict`passed:false` for all thirteen routes:7,407 expanded-picture CRC
observations change as the newly admitted backgrounds replace previously absent
margins. No CRC mask is used. All55 old/new native/guest checkpoints and clocks
remain exact, as do presentation count, frame and alpha. This separately qualifies
the source-backed visual extension; it does not claim per-frame guest/native-center
logging, complete rooms/campaign coverage or a performance gain.

The installed final build is`14bbf8c764b02389...` /manifest`6dc5d93d2bb996c6...`.
A later boss-only correction hands the original B8DD JSL back to the interpreter
so all five actual wait frames are witnessed. Its affected staged route passes
state/picture/clock comparison. Renderer source and reused compiled renderer
objects are unchanged; independent source review records this component transfer.
The final installed four-family door chain and revival replay match72 component
hashes at nine checkpoints, with positive custom replacement and zero mismatches.
The earlier thirteen renderer pairs are not relabeled as runs of this later binary.

The external-owned-ROM helper verifies48 actual loader snapshots,22,364,160
independent source pixels,7,332,864 finite-edge pixels,396 resource/layout/authority
refusals and12 generation refusals. It checks all four actual animation phases
in each35..37 room and1,824 animation-protocol refusals. Portable inputs are in
`validation/inputs/batch24/renderer`: explicit executable/manifest/output required,
zero new seed states. After old executable retirement, historical controls are
receipts; future changes require fresh matched controls.

Batch23 adds exact stable room08 Elf Field. Its descriptor requires 1536x1024,
BG12B297, OBJ1891E4, both06999C definition pointers, maps 7F0000/7F4000,
ordinary WRAM50=0 and PPU layout 71/79/3C. The six animated world characters
continue through their existing source-owned witness; no animation, actor or
loading/fading authority is broadened. Finite clipping blacks physical pixels
outside the exact world after composition. Earlier scene behavior is unchanged.

Runtime qualification also found a real reference-widescreen bug during the
opening Elf Field camera quake: WRAM camera Y0 and PPU Y1023 place the native
first visible row at source-worldY0, while the old margin path substituted raw
Y1023 and read blank map padding. Actual source snapshots prove all 32 native
BG1 and 32 BG2 first-row tiles equal the original ROM/WRAM world row0.
Only audited room08 negative-Y presentation keys now receive a 2048 bias, and
source lookup subtracts that bias. This is a cache-key translation; the physical
1536x1024 world does not wrap. Signed finite masks, native rendering and other
rooms remain unchanged. The custom renderer keeps its independent signed camera.

Final executable`27758caa6deb6bc2...`, manifest`9b718937025e37de...`, passes three
fresh same-executable renderer-off/auto scopes:

| Route | Ordered presentations | Replacements | Independent draws | Source pixels |
| --- | ---: | ---: | ---: | ---: |
| Original room08 loader/dialogue |861|604|177|27,039,936|
| Dialogue, east, status/save and reset |962|705|178|27,192,704|
| Complete dialogue, menus and reset |1,406|1,149|242|36,969,856|

All 3,229 ordered presentation observations and nineteen seven-component
checkpoints match, with 2,458 replacements, 597 draws, 91,202,496 source pixels
and zero mismatches. These are overlapping assisted routes, not complete room
or campaign coverage. The first scope replaces all 99 previously mismatching
quake fields, rather than accepting those fields through fallback.

Comparisons against the installed batch22 controls intentionally retain strict
`passed:false`: each route has the same 99 changed expanded-picture CRCs during
presentation 326..522. There are no other input or capture differences; all
nineteen complete native/guest checkpoints and clocks remain exact, as do
presentation counts, frame numbers and alpha. The repeated intervals are 99
unique quake fields observed in three overlapping routes. No CRC mask is used.
This is separately source-qualified visual correction, not strict old-picture
parity or full per-frame guest/native-center logging.

The helper additionally passes 465,920 scalar pixels from an external owned ROM
and real loader RAM/VRAM, 1,863,680 synthetic finite-edge pixels, 32 entry refusals,
five raster refusals and generation refusal. Tests directly exercise the actual
SDK shadow API across negative phases -3/-2/-1, positive phases, resets and bias
changes:1,713,372 margin checks and 64 real native-row source checks pass; the old
raw-scroll policy differs at 513,870 source samples. The SDK implementation is
included read-only by the test, with no SDK source change. Full-reference equality
and interpreter fallback remain mandatory; the reference raster still runs.

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
# Optional exact room08 source/shadow test, using matching private snapshots:
& <private-test.exe> room08 <private.sfc> <room08.wram.bin> <room08.vram.bin>
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

The original scene catalog is covered. Further actor work concerns changed or
never-seen special far poses, independently owned multipart/projectile producers,
and story branches without live-far evidence. New raster/UI behavior needs its
own source/ownership contract. Natural campaign playtesting and broader
logging-free performance samples remain separate work.

Higher-resolution art, perspective changes and a full3D conversion are separate
projects. This renderer retains original pixel assets and finite geometry.
