# Lagoon Recomp

A work-in-progress Windows port of **Lagoon (USA)** built with
[SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp).
Native gameplay runs alongside an interpreter safety net, with experimental
widescreen, a custom scene renderer and optional quality-of-life improvements.
**Bring your own clean USA ROM.** No ROM or extracted game graphics are bundled.

![Atland with Enhanced original pixels at 1920 by 1080](docs/screenshots/atland-epx-1920x1080.png)

[More screenshots](docs/screenshots/README.md) ·
[Windows preview](https://github.com/rages4calm/LagoonSNESRecomp/releases/tag/v0.1.0-preview.1) ·
[Build](docs/BUILDING.md) · [Playing and settings](docs/PLAYING.md) ·
[Development](docs/DEVELOPMENT.md) · [Credits](CREDITS.md)

## What you can expect

- **Wider views:** 16:9, 16:10, 21:9, 32:9 and window-driven aspects, with
  additional scenery, supported NPC/enemy sprites, selected multipart boss poses
  and supported projectile and fullscreen magic effects beyond the original view.
- **Original artwork, enhanced:** optional EPX treatment preserves the palette
  and keeps the HUD sharp. No replacement artwork is required.
- **Gameplay options:** faster walking, longer sword reach with a modest
  cleave, extra hit recovery, gentler opening balance, more EXP, longer jumps,
  stronger magic and a centered camera.
- **Built-in escort improvements:** Giles can receive first aid without spending
  a Healing Pot. Qualified escorts follow Nasir's actual route at his pace,
  keeping a short gap and the original wall collision. You can walk through
  your active escort when reversing direction. Menus also open faster.
- **Explored map:** press **M** or **Select+L** to pause and see your position,
  discovered exits, links to visited areas and discovered treasure.
- **Companion Launcher:** display, audio, input and QoL settings in one place.
  Choose **Original SNES** or the bundled **X68000** music on Audio; no manual
  conversion or folder selection. Unmapped cues retain SNES music and sound
  effects remain original. [Music details and MP3s](docs/SOUNDTRACK.md).

## Run it

Download the **Windows x64 preview ZIP** from
[Releases](https://github.com/rages4calm/LagoonSNESRecomp/releases).
Extract the whole ZIP and open **Setup Lagoon.cmd** or **lagoon.Companion.exe**.
Select your clean USA ROM on Game and Apply, then open Game setup and Run setup.
Press Play when setup finishes. The playable game executable and self-contained
Companion are included, each with a custom icon; no compiler, SDK or download is
needed for setup. Your ROM is verified and is never bundled or copied by setup.
Preview settings and saves live in the extracted folder's `local` directory.

For a source checkout, [build the game and launcher](docs/BUILDING.md), then open
**Start Lagoon.cmd**. Development settings and saves remain in `build`.

On a **1920×1080 monitor**, start with desktop fullscreen, **16:9 widescreen**,
VSync on and optional Enhanced original pixels. Leave linear filtering and frame
blending off for a sharp picture. 21:9 and 32:9 show more horizontal scenery, with
letterboxing on a 16:9 display. Window scale affects windowed size only.

## Project status

This is experimental. Native combat, damage, death/revival, boss services and
endgame routes, display fades and guarded HUD dispatch have focused checks;
the interpreter remains enabled for paths
that have not been qualified. Scenery contracts cover all 75 authored room
descriptors, but that does **not** mean every actor, effect, menu or transition
supports widescreen. Unsupported content falls back to the original view.

The owner and friends will perform the uninterrupted start-to-finish playtest.
Special actor/effect coverage and frame-time spikes remain work in progress;
there is no locked-60-FPS or full-game completion claim. An objective journal,
route guidance and further zoom-out are later considerations. See [development scope](docs/DEVELOPMENT.md)
and [how to report a playtest issue](docs/PLAYING.md#reporting-a-bug).

## Credits and licensing

QoL ideas were inspired by **oipo's**
[Lagoon QoL hack](https://www.romhacking.net/hacks/9856/).
See [full credits](CREDITS.md) for the original developers and project dependencies.

Retain the included licenses: the SNESRecomp foundation and derivatives use
**PolyForm Noncommercial**; Companion Launcher and the Lagoon launcher adapter
use **MIT**. Public source availability does not remove the noncommercial terms.
ROMs, generated ROM-derived code, extracted assets, saves and private validation
captures stay local. The gallery contains selected development screenshots.
