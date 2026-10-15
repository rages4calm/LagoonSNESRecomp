# Batch 4 wider actor buffer: implementation and bounded qualification

Implemented after the native combat gate. The interpreter remains enabled.
This is bounded presentation work, not full campaign or full native coverage.
Current player-facing behavior and acceptance are in `docs/WIDESCREEN.md`.
Private evidence is under `work-native-20261015-130023`; no ROM/assets or raw
runtime dumps are included in public sources.

## Preserve the simulation and native field

Keep real OAM-DMA capture timing, room/timeline fingerprints, actual-VRAM native
fragments, zero host opacity across the native256 columns and parked-OAM hints.
Preserve `WsShadowForceTile` for the independently audited retained maps. No
raster-scroll API was added: the former apparent phase error was an audit that
omitted SNES BG scanline1 for output row0.

The guest's normal active interval is approximately x[-96,352). Increasing its
culling constants would change simulation and expose stale frame/graphics data.
The host producer therefore reads retained entity state without executing AI,
collision or guest stores. Native entity simulation remains authoritative.

## Read-only frame sources

`$01:A7B5` dispatches animation through the table at `$01:A7C9`:

| Type | Routine | Frame source |
|---|---|---|
|1 NPC|`$01:AC4F`|pointer `$01:F885 + kind*2`, then delay and16 directional frame bytes|
|2 normal enemy|`$01:AB31` / `$01:AB5E`|17-byte record `$01:F41A + kind*17`|
|0 player|`$01:A7D6`|separate native path|
|3 special/boss|`$01:ACAA`, dispatch `$01:ACC7`|unsupported far scope|

Entity records begin at WRAM0500, stride40hex,33 records. Common selection uses
record direction+0A, motion+0B, timer+10, phase+11, type+18, kind+1B, frame+1C,
and enemy variant+2C. Idle conditions reset phase/timer. Supported moving poses
use the original delay/phase table and enemy variant*16. No offscreen current
pose is inferred from stale guest+1C alone.

Current far whitelist is NPC kinds41/59 and normal enemy kinds0/1. Nonzero death
status, unknown kinds/types, malformed tables and player graphics are rejected.
Queued cells must first decode to actual uploaded OAM. The post-state common
frame selector must also match the actual native frame before that kind gains
a current-room witness. One initial cave kind1 discrepancy revokes the witness;
it is not counted as a valid far pose.

The separate host presentation clock follows actual WRAM0408 main-loop clock
increments, bounded to8 ticks per captured interval. It synchronizes to current
native phase when queued and freezes while the menu suspends the guest clock.
Room/source/timeline changes reset history. This does not claim guest offscreen
animation equivalence: presentation phase may reconcile on native reentry.

## Resident atlas ownership

Ordinary room loading uses staging WRAM7F:7F80. BG source DP57 installs its room
graphics, while OBJ source DP5D installs2A00bytes at VRAMword4400 in three actual
DMA segments:

| Source | Destination word | Bytes |
|---|---|---|
|7F:7F80|4400|1000hex|
|7F:8F80|4C00|1000hex|
|7F:9F80|5400|0A00hex|

`$01:88CD` stages destination/length; `$00:908A` splits uploads. Existing generic
DMA-start observation captures channel direction/mode/source/destination before
transfer; actual PPU byte-write observation marks ownership only for the
recognized sequential transfer. Unknown later writes invalidate touched bytes.
Additional `$00:94A1` packs can overlap the atlas tail at5800; these bytes cannot
be guessed or reused as if still owned. Dynamic player pack4000..423F is refused.

The atlas is bound to room048E, graphics source and explicit state generation.
Far cells require every plane byte owned, plus the current-room native kind
witness. Loading an old state without a new complete witnessed upload hides far
actors until a valid new room load. No host atlas exports or fake textures exist.

## Composition and order

Native sorting at `$01:C026` is unsigned ground-Y descending, stable for equal Y
under ascending entity update order from `$01:80AF..80E1`. Deep-margin queued
and far cells share this order, with cell order preserved inside an entity.
The near native seams retain original OAM slots. Far bounds must be wholly in
deep margins x<-16 or x>=272, and added pixels are always transparent in the
native256 columns. Unsupported live native objects in deep margins and
capacity overflow disable the far producer for that frame.

## Background and transient scope

Exact resource fingerprints, not PPU layout alone, authorize expansion:

| Room | BG source | OBJ source | Both definition pointers | Maps |
|---|---|---|---|---|
|Atland0|138000|198000|068000|7F0000 /7F4000|
|Gold Cave3|128B77|1D8000|069298|7F0000 /7F4000|

Other scenes, modal BG3 overlays and loader100E use clean native fallback. The
original loader sets100E at01878C and clears it after map/graphics/palette/entity
installation at018769. Actual VRAM writes detect mid-frame changes. The official
DrawFrame compositor blanks the mutable presentation destination margins after
copying the const native field; native columns and guest state are untouched.

Room RAM can switch before the completed raster changes rooms. Expansion thus
requires three consecutive ready resource observations (two prior completed
boundaries), reset by loading, room/resource or timeline changes. A first-write
guard alone did not eliminate one stale completed-raster frame; the settlement
check is required by the actual391-present sequence.

Room2's out-of-room fill29 resolves opaque grey/white patterned graphics, so
merely returning the same fill at tighter bounds cannot remove its exposed
edges. That outdoor room is deliberately outside the qualified resource set.
Future support needs actual room bounds/masking and independent map/graphics
proof; the current fallback is not an outdoor decoder repair.

## Bounded final acceptance

Candidate `lagoon-batch4-settled.exe`, SHA256
`904d218f875e4ebd877e9f5df4b85aecf901efbdc70fd7b04c26d9822fcf752b`.
Walking captures cover NPC kind59 at screen origin367 and normal enemy kind0 at
origin-131, beyond native activation, with216/216 right and476/476 left actual
VRAM/CGRAM color witnesses. The cave fixture declares a pre-entry guard bypass;
inside-room movement uses ordinary input.63 paired menu/town/cave tags preserve
native framebuffer, WRAM, registers, VRAM, OAM, CGRAM, SRAM and guest clock.
A391-present transition audit has unchanged native centers and no unsupported
margin leaks, including the formerly corrupt frame2864. Cave expansion resumes
at3105 after settlement. Stable inventory and return checks verify clean native
menu margins and resumed scenery after closing.

ROM-free regressions exercise the actual decoder, PPU and DMA, plus pure common
frame selection, ownership invalidation, room/source/timeline reset, modal and
resource gates, pitch padding/native-center preservation and scene settlement.
They complement runtime evidence; they do not certify all entities/campaign.
Remaining scope includes special/death/boss forms, additional kinds/rooms,
full campaign save/load visits, parallax, room edge policy and wider UI layout.
