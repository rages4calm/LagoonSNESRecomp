# Playing and settings

Build from [these instructions](BUILDING.md), open `Start Lagoon.cmd`, select your
clean USA ROM in Companion Launcher and choose Play. Apply settings before starting
or restart to use changes. This is an experimental build with interpreter fallback.

During gameplay, the SNES **B** button swings the sword and **A** jumps. These
are controller button names, rather than literal keyboard keys; use your input
bindings for the corresponding actions.

## Display

For a 1920×1080 monitor, choose desktop fullscreen, widescreen **16:9** and VSync.
Enhanced original pixels (EPX) is optional: it treats diagonal pixel edges using
original colors while keeping the bottom HUD sharp. It requires OpenGL and the
bundled shader files. Turning it off restores the previous graphics selection.
Linear filtering and frame blending should remain off for a sharp picture.
The retired Smooth scaling option is no longer offered.

Window scale changes the size of a window, not the fullscreen field of view.
Leaving it at 3 is fine in fullscreen. 21:9/32:9 reveal more horizontal scenery
but produce a shorter, letterboxed picture on a 16:9 monitor. Widescreen adds
world scenery at the existing game scale; it is not a zoom-out feature.

Ordinary supported dialogue retains the surrounding world, and qualified menu
frames expand using original artwork. Titles, portrait layouts, finite map edges,
unsupported effects and unknown resources can retain the original geometry.
Additional sprites are presentation only: they do not extend enemy AI or collision.

## Quality of life

The nine gameplay options default off and can be selected individually in
Companion's Quality of life page. Opening balance affects new games.

| Option | Effect |
| --- | --- |
| Sword reach | Four forward extents become 38 pixels; perpendicular edges gain 3 pixels of cleave. |
| Follower speed | Qualified escort movement increases from 1 to 3 pixels per frame. |
| Walking speed | Nasir's ordinary 3-pixel movement becomes 5; enemies and hurt knockback remain stock. |
| Hit recovery | Eight additional frames reject ordinary consecutive hits. |
| Opening balance | Start at 18 HP and 11 defense; level 2 uses 21 HP and 13 defense. Level 3 onward follows stock growth. |
| EXP rewards | Enemy/boss EXP increases 50%, rounded down. |
| Jump hold | Hold the original jump peak for eight frames while horizontal movement continues. |
| Magic power | Nonzero player magic powers increase 50%, rounded down. |
| Centered camera | Follow Nasir using the game's original scenery streaming. |

Faster Items/Magic/Equipment transitions and Giles's first aid are built into the
port. Speak to Giles, choose Yes and follow the escort quest; no Healing Pot is
required or consumed. The ROM remains clean. These are shared native/interpreter
hooks rather than applying the supplied BPS at launch. [Credits](../CREDITS.md).

## Saves and states

Normal in-game saves remain available. **F1 loads slot 1; Shift+F1 saves it.**
F2–F10 select the other slots, with Shift to save; F11 opens the state browser.
Settings live in `build/config.ini`, bindings in `build/keybinds.ini`, and saves
in `build/saves`. The launcher does not migrate them.

Development state formats can change. Incompatible or corrupt states are refused
before changing the running guest. Use normal game saves for campaign progress.
Transient extra recovery/jump timers reset on a state load, so a state saved during
one of those effects does not reproduce its remaining optional hold exactly.

## Audio

Original SNES music is the default. The optional imported X68000 pack currently
maps opening, Atland and opening Gold Cave music. Missing/unmapped cues retain
SNES music and sound effects remain on the original driver. See
[soundtrack setup](SOUNDTRACK.md). No music is bundled.

## Reporting a bug

The owner and friends perform the uninterrupted campaign playtest. Include:

- Location, nearest save/slot and exact steps/buttons to reproduce.
- Expected behavior and what happened; a short image/video for visual faults.
- Executable version/hash, aspect, Enhanced setting, QoL choices and controller.
- Relevant crash/log text, and whether the problem repeats after reloading.

For slowdown, mention the room and enemies present, and whether movement,
animation, audio or only presentation slows down. A steady presentation counter
alone does not prove equal gameplay cadence everywhere.
