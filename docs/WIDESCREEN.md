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
their native field. The first implementation supports ordinary mode-1 rooms
with the two retained map layers and the verified `$7000/$7800` tilemap bases.

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
when the viewport widens. Actual off-screen entity generation is still pending:
enemies/NPCs can appear at the native culling boundary. Changing guest culling
without reproducing their graphics staging would expose stale tiles, so this
first stage deliberately expands world backgrounds while keeping the existing
entity simulation and OAM producers.

## Reproduce the map check

Use private ROM and state-dump paths; these inputs and screenshots must stay
out of the source repository:

```
python -P tools/verify_wide_map.py --rom <private.sfc> --dumps <private-dumps> --out <private-report.json>
python -P tools/qualify_native.py --exe <candidate.exe> --route tools/script_native_gameplay.txt --frames 8000 --config <private-config.ini> --screenshot-frame 6513 --out <new-private-output>
```

`-P` avoids the existing `tools/dis.py` shadowing Python's standard `dis` module.
Diagnostic state dumps intentionally contain only the native 256-pixel center;
`presentation.ppm` captures the entire expanded field. `LAGOON_WIDE_ASPECT` may
override the aspect for diagnostics without writing player settings.

Future work: title-specific off-screen object composition and verified sprite
ordering, parallax BG2 reconstruction, room-bound policy, HUD anchoring, first
visit/room transition and save-load checks across more of the campaign. Follow
the framework's `docs/WIDESCREEN_PATTERNS.md` invariants during that work.
