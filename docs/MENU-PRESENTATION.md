# Wide Items and Magic presentation

When experimental widescreen is enabled, verified Items and Magic screens reuse the original ornate frame across the available width. The title, text, cursor and item/magic body retain their original source pixels and stay centered. This is a host compositor change: it does not change guest RAM, PPU memory, gameplay, fonts or artwork.

![Items, 16:9, Enhanced pixels](screenshots/items-16x9-epx-1920x1080.png)

![Magic, 32:9, Enhanced pixels](screenshots/magic-32x9-epx-1920x1080.png)

Both PNGs are lossless conversions of actual 1920x1080 fullscreen OpenGL readbacks with Enhanced pixels enabled. The 32:9 viewport is letterboxed on this 16:9 monitor. These are running-game captures, separate from the standalone compositor's small source-pixel test previews. The early-game inventory is mostly empty; populated late-game panels remain to be checked.

## Original-art layout

The compositor moves the original side and corner pieces to the new edges, repeats the existing 24-pixel chain period, and keeps the 80-pixel title segment centered. Expansion uses symmetric 48-pixel steps. Its virtual width is `256 + 48 * floor((actualWidth - 240) / 48)`; slight excess clips only the original outer black margin, never text or ornament. Narrower residual gaps remain black. Magic's existing divider extends in its original two colors. Nothing is stretched or newly drawn.

The centered content body at native x=40..215, y=32..199 is copied exactly. Top/bottom center art is preserved. Output pitch padding and source/destination aliasing are supported.

## Conservative identification

`src/lagoon_menu_presentation.c` requires the matching scene flag, loaded resource pointer, visible PPU mode, screen maps, zero scroll, and two serialized VRAM fingerprints: shared border/font graphics plus the immutable Items or Magic map perimeter/title. A black screen or BG3 presence alone is insufficient. It also checks chain periodicity before reflow. Unknown templates and modes decline without touching the destination. Equipment, Save, cutscenes and other unaudited panels keep the existing fallback.

## Integration verification

Combined executable SHA-256: `14be32d86a553a27f47971bf5479f9900d225c832f6ee5dc016b870d1423a507`.

Same-route wide/narrow checks matched the complete guest framebuffer, WRAM, CPU/PPU register dump, VRAM, OAM, CGRAM and clock information at all four snapshots for both Items and Magic. All 2,007 Items and 2,013 Magic presentation frame/alpha entries matched. Host-composed CRCs differ as expected because the frame is wider; they are not a guest-state parity claim.

Actual pre-GPU 342x224 Items and 682x224 Magic composites retained every original body and title pixel exactly and introduced zero new colors. The GPU screenshots above include optional EPX filtering after that composition. Visual inspection found clean repeated seams and readable original text.

The C11 unit test also passed synthetic widths 256, 258, 280, 308, 342, 448, 684 and 800; native/minimum reconstruction, alpha preservation, padded rows, immutable input, alias safety, disabled refusal and mutated identity/resource/mode refusal were checked. Real dumps from both integrated GPU runs passed the same production compositor checks.

```powershell
gcc -std=c11 -O2 -Wall -Wextra -Werror -I snesrecomp/runner/src tests/lagoon_menu_presentation_test.c -o <private-test.exe>
& <private-test.exe> <private-dump-prefix> <existing-private-output-directory>
```

The private evidence receipt is `work-native-20261015-145037/menu-integrated-acceptance.json` beside this checkout. No owner configuration, save or mod state was changed for qualification.
