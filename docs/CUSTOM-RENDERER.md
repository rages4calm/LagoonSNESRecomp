# Custom scene renderer experiment

The game still uses its qualified SNES renderer and interpreter safety net.
`tools/scene_renderer_prototype.py` is an **offline experiment**, with no launcher
option and no live renderer replacement. It is a concrete first step toward the
custom renderer discussed for this port.

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

## Path to an actual custom runtime renderer

1. Extract a reusable read-only scene packet at the real completed field/DMA
   boundary: room resources, world camera, finite extent, background cells,
   palette, original UI, and objects with explicit graphics ownership and order.
2. Preserve the existing retained-map, completed-upload, torch-animation and
   timeline checks. Transfer their proven ownership into the packet; do not
   create a second AI or guess stale object frames.
3. Add a live software scene backend first, with optional GPU texture upload.
   Compare every native pixel and both added margins through fades, dialogue,
   inventory, doors, damage, cold loads and reset. Keep unknown scenes on the
   original renderer.
4. Add raster changes and unsupported room/object families incrementally, then
   measure sustained gameplay performance using logging-free hidden tests.
   A D3D12 uploader, shader or larger framebuffer alone does not establish a
   correct game-state renderer.

Higher-resolution art, perspective changes and a full 3D conversion are separate
projects. This experiment retains the original pixel assets and scene geometry.
