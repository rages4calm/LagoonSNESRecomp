---
name: Lagoon in-game menus
description: Readable native adventure menus in Lagoon's black-and-ivory dialogue-frame tradition.
colors:
  warm-gold: "#f2ce7c"
  panel-black: "#08090d"
  ivory-text: "#f6f1df"
  quiet-text: "#bdb9ad"
  divider: "#4e5056"
  selected-row: "#292a32"
  frame-outer: "#6e7076"
  frame-inner: "#77766e"
  map-black: "#080808"
  map-ivory: "#f5f1df"
  map-quiet: "#bdb8a7"
  map-gold: "#edc977"
typography:
  title:
    fontFamily: Atkinson Hyperlegible Next
    fontSize: "28px"
    fontWeight: 500
    lineHeight: "40px"
  body:
    fontFamily: Atkinson Hyperlegible Next
    fontSize: "14px"
    fontWeight: 500
    lineHeight: "20px"
  label:
    fontFamily: Atkinson Hyperlegible Next
    fontSize: "9px"
    fontWeight: 500
    lineHeight: "12px"
rounded:
  square: "0px"
spacing:
  map-inset: "12px"
  menu-content: "28px"
  body-line: "20px"
components:
  menu-canvas:
    backgroundColor: "{colors.panel-black}"
    textColor: "{colors.ivory-text}"
    rounded: "{rounded.square}"
    width: "640px"
    height: "360px"
  map-canvas:
    backgroundColor: "{colors.map-black}"
    textColor: "{colors.map-ivory}"
    rounded: "{rounded.square}"
    width: "960px"
    height: "540px"
  selected-action:
    backgroundColor: "{colors.selected-row}"
    textColor: "{colors.ivory-text}"
    typography: "{typography.body}"
    rounded: "{rounded.square}"
  tab-active:
    backgroundColor: "{colors.panel-black}"
    textColor: "{colors.warm-gold}"
    typography: "{typography.body}"
  tab-inactive:
    backgroundColor: "{colors.panel-black}"
    textColor: "{colors.quiet-text}"
    typography: "{typography.body}"
  achievement-toast:
    backgroundColor: "{colors.panel-black}"
    textColor: "{colors.ivory-text}"
    typography: "{typography.label}"
    width: "144px"
    height: "32px"
---

# Design System: Lagoon in-game menus

## Overview

**Creative North Star: "Lagoon's dialogue frame"**

The original game's black dialogue windows and light frames establish the visual
authority. The native map, Options and achievement browser extend that world with
readable sentence-case lettering, restrained gold selection and persistent control
hints. Original scenery supplies the imagery; the interface supplies orientation.

These are controller and keyboard surfaces drawn by C into authored pixel canvases.
Their text is independent of mutable game glyphs and the optional gameplay EPX
pass. The system favors a clear selected action over decorative motion or depth.

**Key Characteristics:**

- Black fields with layered ivory frames.
- Proportional lowercase lettering and a small, explicit type ramp.
- Gold cursors, active tabs and progress; quiet secondary information.
- Finite original terrain with fixed-size map labels and landmarks.
- Visible keyboard/controller commands at the bottom of each modal.

Evidence is `src/lagoon_ui.c/.h`, `src/lagoon_map.c`, `src/lagoon_settings.c`,
`src/lagoon_achievement_ui.c` and the actual 1920×1080 captures recorded in
`docs/screenshots/map-captures.json`. The `.impeccable/design.json` HTML/CSS
examples translate native patterns for documentation; they are not game code.
Sidecar tonal strips are generated color previews, not additional production
palette tokens. Preview text uses a browser fallback if the documented font is
not installed; the shipped game always uses its embedded atlas.

## Colors

The shared palette separates warm emphasis from a small set of dark surfaces,
ivory text and quiet structural grays. The map retains the close local variants
listed in frontmatter; do not silently merge them with shared UI constants.

### Primary

- **Warm Gold:** shared cursor, selected tab, completed milestone and progress fill.
- **Map Gold:** map heading, exit symbols, discovered-exit numbers and treasure markers.

### Neutral

- **Panel Black / Map Black:** opaque canvas grounds.
- **Ivory Text / Map Ivory:** primary copy, headings and bright frame strokes.
- **Quiet Text / Map Quiet:** secondary copy, explanations, status and control hints.
- **Divider:** thin content rules, progress tracks and subdued locked symbols.
- **Selected Row:** a tonal band behind the active menu or achievement row.
- **Frame Outer / Frame Inner:** narrow frame layers around the brighter double stroke.

**The Selection Rule.** Gold identifies the active choice, meaningful progress or
a map landmark. Ordinary paragraph text remains ivory or quiet gray.

## Typography

The shipped face is **Atkinson Hyperlegible Next**, rasterized at weight 500 into
immutable body and small ASCII atlases. The body atlas is generated at 14 pixels
and scaled by an integer for headings; the small atlas is generated at 9 pixels.
There is no runtime font download or system-font fallback. Unsupported glyphs use
the atlas question mark.

### Hierarchy

- **Title:** doubled body glyphs for the location, Options and Achievements headings.
- **Body:** item names, descriptions, tab names, map destinations and selected actions.
- **Label:** compact metadata, milestones, footer commands and achievement notifications.

The frontmatter values are logical pixels, not monitor pixels. Body line cells
are 20 pixels high; small cells are 12. Component rows can have a different stride
to fit their information. Use measured glyph advances, wrapping and the existing
fit/elision helpers rather than character-count assumptions.

**The Fixed Label Rule.** Terrain zoom changes terrain and landmark positions;
map headings, labels and command hints keep their authored size.

Font source, reproducible atlas generator, provenance and SIL Open Font License
are in `assets/fonts/atkinson-hyperlegible/`. The original Lagoon dialogue font
remains appropriate for original NPC-style dialogue; this record concerns the
native host menus rather than replacing the game's art or lettering globally.

## Layout

The map canvas is 960×540 logical pixels. Its repeated outer inset is 12 pixels;
a heading frame leads a large terrain viewport, a narrower legend/destination
column and a two-line command footer. Pan and zoom remain bounded to the area's
finite dimensions. The location heading has precedence over the map-view label.

Options and achievements use 640×360 canvases. Their title/header precedes the
content and a bottom rule separates controls and status. The recurring menu
content inset is 28 pixels. Options has five fixed tabs: Sound, Journal, Items,
Display and Travel. Items and achievements expose five rows per page.

These layouts scale as complete canvases into the available display viewport,
using square pixels for opaque host panels. They do not have CSS breakpoints or
reflow into mobile layouts. The shared frame, glyphs and primitives clip to their
destination. The game's browser/rewind and transparent dialogue retain their own
presentation paths.

## Elevation & Depth

Depth is expressed through tonal selection bands and nested border strokes,
without drop shadows. `LagoonUI_Frame` places the outer gray stroke, a bright
double ivory stroke and an inner muted stroke over the dark panel. It is a
border construction, not a floating card or offset-shadow style.

**The Frame Rule.** Use the shared layered frame to establish a modal or bounded
map region; use a thin divider to separate content within it.

## Shapes

The system uses square rectangles, straight rules, small map symbols and an
authored triangular selection cursor. The cursor is drawn geometry, not a font
glyph. The trophy and location symbols are likewise small authored primitives.
Do not introduce rounded cards merely because a new feature is being added.

## Components

### Framed modal

An opaque dark page with the shared nested frame, a clear title and persistent
commands. Body and label text retain their separate roles. Rendering a panel
does not advance the guest game or change inventory, quests or achievements.

### Selected action and progress

A tonal band and gold triangular cursor identify selection. Actions use normal
body lettering; volume and achievement progress use a thin gray track with a
gold fill. The native UI has no pointer-hover treatment. Selection is driven by
keyboard/controller edges, with explicit held-input repeat where implemented.

### Tab navigation

Sound, Journal, Items, Display and Travel share one header. An active tab uses
gold text and a short underline; inactive tabs use quiet text. Shoulder buttons
or page keys move between tabs, as shown by the footer. A navigation change does
not itself edit a setting or request travel.

### Scenery map and destinations

The image contains original terrain and background layers, excluding moving
actors and the HUD. Current scenery and retained visits share the same viewport;
retained terrain is explicitly labeled **Last mapped visit**. Missing scenery
uses the labeled explored layout. Labels do not claim that a story gate is open.

Numbers associate discovered exit markers with a paged destination list. The
list exposes overflow counts and its paging command. Chest markers require
discovery; terrain visibility alone does not grant exploration, treasure or
quest progress. L/R or bracket keys browse mapped areas; the footer retains
pan, zoom, recenter, fit, page and close commands.

### Journal, item guide and confirmation

The journal uses a chapter, next-step heading, short explanation and compact
milestones. The item guide pairs its list with a detail column for purpose,
carried state and known prices. Both are reference views.

Travel first displays availability and its reason, then a separately framed
confirmation with explicit confirm/cancel commands. The game revalidates a
confirmed action; copy and selection must not imply an unavailable action is ready.

### Achievement browser and toast

The browser separates selection rows from the selected objective and tracking
status. Secret objectives keep their existing mask until earned. A compact
144×32 notification carries an authored trophy, an Unlocked label and a bounded
title. Its existing 5,500 ms lifetime pauses with the other host modals.

## Do's and Don'ts

### Do:

- **Do** use the shared frame and immutable menu font for new host panels.
- **Do** keep map labels fixed while terrain pans or zooms.
- **Do** expose selection and keyboard/controller commands without relying on color alone.
- **Do** distinguish current scenery, a last mapped visit and unavailable scenery honestly.
- **Do** use actual progress and read-only reference data for journal and item copy.

### Don't:

- **Don't** apply gameplay EPX or mutable game-glyph uploads to opaque host-menu text.
- **Don't** use viewing or navigation as an implicit confirmation of a gameplay action.
- **Don't** replace original scenery with decorative illustrations or a whole-game retexture.
- **Don't** treat a discovered exit landmark as proof that its story gate is unlocked.

Not canonized: fixed canvas coordinates and the map's local palette differences
are recorded as current implementation constraints, not a universal spacing grid
or evidence of a separately approved visual world. No hover animation, web input
field or additional decorative shadow is invented for the native UI.
