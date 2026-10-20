# Fullscreen widescreen captures

The first three PNGs were captured from the actual Windows SDL fullscreen output at
**1920×1080**, with nearest-neighbor filtering and camera centering enabled.
They retain the monitor's letterboxing for 21:9 and 32:9 fields. Click an image
to view or download its full resolution.

## Atland — 21:9

![Atland at 21:9 in fullscreen](town-21x9-fullscreen.png)

## Atland — 32:9

![Atland at 32:9 in fullscreen](town-32x9-fullscreen.png)

## Gold Cave — 16:9

![Gold Cave at 16:9 in fullscreen](cave-16x9-fullscreen.png)

This is experimental additional scenery and actor rendering at the original
game scale. The HUD stays centered. Current audited dialogue and Start/status
screens keep their surrounding world scenery; the known Items/Magic frames
expand using their original artwork. Titles, unsupported intro layouts, other
menus and unknown room resources retain centered native fallback. The cave capture uses
the documented position-only guard bypass to reach its real entrance.

Capture frame numbers, feature selection and source executable identity are in
[capture metadata](captures.json). The PNGs preserve the fullscreen readback
pixels; raw 224-line diagnostic captures are kept privately.

## Atland - enhanced original pixels, 16:9

![Atland with optional EPX at 1920 by 1080](atland-epx-1920x1080.png)

Actual 1920 by 1080 OpenGL fullscreen readback with optional EPX and immediate
camera following. Original colors and sharp HUD; no redrawn artwork or artificial
screenshot upscale. The same-build stock/EPX pair preserves guest snapshots,
clock and all 1,762 pre-shader presentation entries. The full retexture idea is
shelved. See [enhanced pixels](../ENHANCED-PIXELS.md).

## Atland dialogue — 21:9, 1920×1080

![Atland NPC dialogue with the world remaining wide](atland-dialogue-21x9-1920x1080.png)

Actual Windows SDL fullscreen output from the batch 6 build, with nearest
filtering and camera centering. Ordinary walking leads into the real NPC
conversation; the text is still typing. The dialogue remains centered while
both sides retain the surrounding town. The PNG preserves the full-resolution
RGBA readback without any post-capture resizing. This visible run measured
46.2 host frames per second with normal frame delay; it is not a steady-60-FPS
performance claim.

## Items — 16:9, Enhanced original pixels

![Widened Items frame](items-16x9-epx-1920x1080.png)

## Magic — 32:9, Enhanced original pixels

![Widened Magic frame](magic-32x9-epx-1920x1080.png)

These are actual 1920×1080 fullscreen OpenGL readbacks from batch 6. Original
text/icons stay centered while the original frame expands. The early inventory
is mostly empty; populated late-game panels need further checks. The 32:9 field
is letterboxed on this 16:9 monitor. See [menu qualification](../MENU-PRESENTATION.md).

## Atland — current 32:9 Enhanced pixels, 1920×1080

![Current fullscreen Atland with Enhanced pixels](atland-32x9-epx-performance-1920x1080.png)

Actual OpenGL fullscreen output after the raster-state restoration fix. A fresh
version-3 state was loaded, followed by about 50 seconds of movement and idle
time. The original artwork and centered HUD remain; the 32:9 viewport is
letterboxed. This single visible run averaged 59.63 fps including state loading
and diagnostic overhead. A separate clean hidden-GPU interval measured about
60.12 fps with remaining frame-time spikes. See [performance evidence and
limits](../PERFORMANCE.md); the older 46.2 fps dialogue capture above is a
historical measurement, not the current controlled result.

## Batch 19 — Gold Cave, 16:9, Enhanced original pixels

![Gold Cave during the batch19 fullscreen check](gold-cave-batch19-1920x1080.png)

Actual 1920×1080 OpenGL desktop-fullscreen readback from the selected batch19
build, with Enhanced original pixels. PNG preserves the readback resolution
and colors. This is an assisted performance encounter; the upper black region
is authored space outside this cave corridor. See [capture metadata](batch19-capture.json).


## Batch26 - new NPC contexts, hidden GPU captures

![Room19 with an actual NPC beyond the original view](room19-batch26-32x9-1920x1080.png)

Room19 at32:9, with the real NPC70 patrol visible to Nasir's right. The wide
field is letterboxed on a16:9 output. Black space at a finite map edge is retained.

![Room1A interior with original NPC artwork](room1a-batch26-16x9-1920x1080.png)

Room1A at16:9; this narrow authored room has finite black margins. These are
actual1920x1080 OpenGL readbacks from the selected batch26 build, using Enhanced
original pixels. PNG conversion is lossless, with no resize/crop/repainting.
Both were captured in a **hidden nonfullscreen window**, with dummy audio to
preserve the bounded assisted-room-loader guard. They are distinct from the
historical visible fullscreen captures above and are not physical scanout proof.
See [exact capture metadata](batch26-captures.json) and [renderer scope](../CUSTOM-RENDERER.md).


## Batch27 - room18, 16:9 Enhanced pixels

![Room18 in the new custom renderer](room18-batch27-16x9-1920x1080.png)

Actual 1920x1080 OpenGL readback with Enhanced original pixels and centered camera.
The original assisted room loader and enemy AI run normally; player HP/defense
are protected for exploration. PNG conversion preserves every readback pixel,
with no crop, resize or repainting. This is a hidden nonfullscreen GPU window
with dummy audio, distinct from the historical visible fullscreen captures.
See [capture metadata](batch27-capture.json), [renderer scope](../CUSTOM-RENDERER.md)
and [bounded timing evidence](../PERFORMANCE.md).

## Batch28 - room24 NPC83, 32:9 Enhanced pixels

![Room24 with a genuine NPC83 beyond the original view](room24-batch28-32x9-1920x1080.png)

Actual1920x1080 OpenGL readback from the batch28 candidate, with
Enhanced original pixels and centered camera. The original room24 NPC83 is
visible in the right margin after an assisted player-position route. The left
black region lies beyond the finite map edge; top/bottom letterboxing is retained
for the32:9 field on a16:9 output.

The PNG preserves every GPU readback pixel without resize, crop or repainting.
This capture uses a **hidden nonfullscreen window and dummy audio**, distinct
from the older fullscreen images. The pre-shader field and two existing guest
checkpoints match the headless NPC route exactly. This image does not certify
physical scanout or full-game rendering. See [exact capture metadata](batch28-capture.json)
and [renderer scope](../CUSTOM-RENDERER.md).
