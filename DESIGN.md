# In-game menus

This records the implemented 0.4.0 design.
The original Lagoon dialogue frame is the visual authority: players should find
their location, read an item's purpose or change an option using a controller.

## Shared appearance

Near-black panels use ivory double/beveled frames, warm gold cursors and selected
labels, and quiet gray secondary text. Use clear sentence case and player-facing
objectives. Keep technical addresses and qualification language out of the UI.
Map, Options and achievements share the frame and type renderer.

Atkinson Hyperlegible Next provides proportional lowercase lettering. The game
embeds an immutable glyph atlas independently of Lagoon's dialogue font, so
guest font uploads cannot change these menus. The body font is rasterized at
14 pixels; compact labels and achievement toasts use the separate small atlas.
The original font and reproducible generator are in
`assets/fonts/atkinson-hyperlegible/`, with pinned provenance and the SIL Open
Font License 1.1. Runtime packages retain the OFL and provenance, not the TTF.

The map uses a 960×540 logical canvas; Options and achievements use 640×360.
The final overlay is composed independently of the game's optional EPX treatment.
Terrain zoom never changes label size. Achievement notifications remain compact
at 144×32 logical pixels with the existing lifetime and pause behavior.

## Scenery map

Use a clear location heading, a large terrain viewport, a narrow marker and
destination column, and a two-line control footer. Render finite terrain from
the game's own map, tile, palette and layer data. Exclude actors, the HUD and
transient effects. Parallax backgrounds retain their actual layer layout and
an anchored view; they are not stretched to fill the area.

Whole-zone scenery is visible in the supported current area. Discovery remains
separate: only discovered exits get matching map/list numbers, destinations are
unnamed until visited, and chests appear after discovery. Busy destination lists
have pages. Browsed areas or unavailable scenery use a labeled explored layout.
No map view grants items, exploration credit, access through a lock or story progress.

D-pad/arrows pan, A/X or +/- zoom, Y/Home recenters on Nasir, Start/0 fits the
area, L/R or brackets browse visited areas, Select/Tab pages discovered exits,
and M/L2/B/Esc closes. The full terrain is a navigation aid, not a walkability
guarantee or a zoom-out of active gameplay.

## Options and achievements

Five tabs share a fixed header and two-line navigation footer:

- **Sound:** the existing master volume, X68000 music volume and achievements
  actions, retaining their input behavior and targeted preference persistence.
- **Journal:** current chapter, next suggested step and relevant milestones from
  actual adventure flags; no invented conversation history.
- **Items:** 25 item explanations, carried state and known Atland/Voloh prices,
  with separate list and detail columns. Browsing cannot use or purchase an item.
- **Display:** the optional sword-range cue, off by default and shown only during
  attacks. It follows active reach/cleave without changing damage or collision.
- **Travel:** a guarded return inside a cleared dungeon's main entrance. The
  first activation opens confirmation; a separate activation requests return.
  Cancel remains in Options. Changed availability cancels confirmation, and the
  live runtime revalidates the request before normal room loading.

L/R or PageUp/PageDown switches tabs. Up/Down selects Sound actions or Items;
Left/Right adjusts Sound or the Display toggle. Enter/A activates a selected
action; B/Esc closes or cancels Travel confirmation first. Viewing and navigation
do not change guest progress. Travel alone can request the separately guarded
game action after confirmation.

## Verification boundary

C raster fixtures verify layout, glyph clipping, input, persistence and request
behavior. Actual GPU readback verifies the presentation pipeline; affected game
routes verify scenery identity, return behavior and unchanged ordinary play.
These evidence types remain distinct. The earlier campaign does not by itself qualify newly added interactions.
Focused model, actual GPU and original-loader route checks cover the new features.
