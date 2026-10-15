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
game scale. The HUD stays centered. Titles, intro scenes, Start/status and
inventory menus retain native-width layouts. Unsupported room resources use
native fallback while their wider maps are investigated. The cave capture uses
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
