# Lagoon Recomp

A work-in-progress Windows port of **Lagoon (USA)** built with
[SNESRecomp](https://github.com/RetroPortingToolKit/snesrecomp).
Native gameplay runs alongside an interpreter safety net, with experimental
widescreen, a custom scene renderer and optional quality-of-life improvements.
**Bring your own clean USA ROM.** No ROM or extracted game graphics are bundled.

![Atland with Enhanced original pixels at 1920 by 1080](docs/screenshots/atland-epx-1920x1080.png)

[More screenshots](docs/screenshots/README.md) ·
[Windows preview](https://github.com/rages4calm/LagoonSNESRecomp/releases) ·
[Build](docs/BUILDING.md) · [Playing and settings](docs/PLAYING.md) ·
[Development](docs/DEVELOPMENT.md) · [Credits](CREDITS.md)

## What you can expect

- **Wider views:** 16:9, 16:10, 21:9, 32:9 and window-driven aspects, with
  additional scenery, supported NPC/enemy sprites, selected multipart boss poses
  and supported projectile and fullscreen magic effects beyond the original view.
  Loading a compatible Elf Field or Voloh state also recovers supported actors in the
  added view without requiring a door reload, including more supported Voloh NPCs.
  Supported enemy defeat effects also recover after compatible state loads.
  Equipped spell graphics also recover after compatible loads, including
  supported homing spells in the added view.
  A verified cave actor pack also restores supported enemies after loading,
  while unknown graphics remain excluded.
  Moving NPCs now use the completed original draw pose, with additional
  source-checked Thor, Bird and orb rendering.
- **Original artwork, enhanced:** optional EPX treatment preserves the palette
  and keeps the HUD sharp. No replacement artwork is required.
- **Gameplay options:** faster walking, longer sword reach with a modest
  cleave, extra hit recovery, gentler opening balance, more EXP, longer jumps,
  stronger magic and a centered camera. EXP and gold rewards have independent
  Original–8x selectors; the previous 1.5x EXP setting remains available. The
  longer-reach option also extends supported sword artwork to match its reach.
- **Built-in escort improvements:** Giles can receive first aid without spending
  a Healing Pot. Giles and Thor route around walls and stairs at Nasir's
  walking pace, keeping a short gap and the original wall collision. You can walk through
  your active escort when reversing direction. Menus also open faster.
- **Explored map:** press **M** or controller **L2** to pause and see your
  position, discovered exits, links to visited areas and discovered treasure.
  The early tablet quest includes next-step hints and relevant objective markers.
- **Quick settings:** controller **R2** or **Ctrl+comma** opens an in-game
  master-volume and X68000 music-volume panel. The game pauses while it is open.
- **Companion Launcher:** display, audio, input and QoL settings in one place.
  Choose **Original SNES** or the bundled **X68000** music on Audio; no manual
  conversion or folder selection. Unmapped cues retain SNES music and sound
  effects remain original. X68000 cues now have balanced playback levels, with
  a separate music trim. Keyboard, controller and system-hotkey editors write
  the native settings directly. [Music details and MP3s](docs/SOUNDTRACK.md).

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

The source and installed local development build are now **0.1.10**. The latest
packaged Windows preview remains **0.1.3**; its release notes describe that package.

This is experimental. Native combat, damage, death/revival, boss services and
endgame routes, display fades and guarded HUD dispatch have focused checks;
the interpreter remains enabled for paths
that have not been qualified. Scenery contracts cover all 75 authored room
descriptors, but that does **not** mean every actor, effect, menu or transition
supports widescreen. Unsupported content falls back to the original view.

The owner and friends will perform the uninterrupted start-to-finish playtest.
Special actor/effect coverage and occasional frame-time spikes remain work in
progress. Native enemy calculations now cover ordinary areas, including sprite
packing and direction/velocity commands. A focused busy Elf Field route holds
the intended 30 game updates per second with approximately 60 displayed frames
per second; this is not a full-game performance guarantee. A guarded transition
bridge also covers the brief gap before an area's loading flag appears.
Incoming mosaic fades now retain the current room's original center and fill
qualified widescreen margins; completed menus take priority over cached scenery.
Native table dispatch now covers qualified item, spell, death and boss-script
contexts, with the full ordinary actor sorting loop also compiled. Unknown
script contexts continue through the interpreter. All 40 commands in the NPC
script family now have qualified native entry contexts, with additional dialogue
continuations and separate assisted state/audio checks. Native music uploads
preserve actual device timing; initial unmapped boot uploads remain interpreted.
A read-only map-resource scan also reduces loading work without changing game
speed. Focused fullscreen measurements distinguish original loading waits from
host rendering cost; they do not establish locked 60 FPS throughout the game.
Tablet hints are available;
a full objective journal, broader route guidance and zoom-out remain later work. See [development scope](docs/DEVELOPMENT.md)
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
