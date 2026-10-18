# Lagoon playtest guide

Launch `Start Lagoon.cmd`, open Companion Launcher and start the game with your
clean USA ROM. Keep the existing saves and settings in the normal build folder.
This is a beta/WIP with an interpreter safety net; the owner/friends' uninterrupted
start-to-finish run is the campaign test.

For a1920×1080 monitor, choose desktop fullscreen and widescreen16:9. Enhanced
original pixels is optional. Keep linear filtering/frame blending off for sharp
pixels and VSync on. Wider21:9/32:9 modes show more scenery but make the game
shorter on a16:9 display. Unsupported scenes may use native-width presentation.

F1 loads slot1; Shift+F1 saves slot1. F2–F10 select the other slots with Shift
to save. F11 opens the state browser. Normal in-game saves remain available.
See [save-state details](SAVE-STATES.md).

Giles now accepts dressing his wounds without a Healing Pot. Speak to him,
choose Yes and follow the original escort quest. Improved sword reach, when
enabled in Companion, also includes a small three-pixel lateral cleave.

For a bug report, include:

- The executable version/hash, room/location and nearest save/slot.
- Exact steps and buttons from that save, expected result and actual result.
- Enabled QoL options, display aspect, Enhanced setting and controller type.
- Whether the issue repeats after loading; a short screenshot/video for visual
  faults and relevant log lines for crashes or stalled input.

For slowdown, identify the room, enemies on screen and whether player movement,
animation, audio or only display updates slow down. A steady presentation rate
alone does not prove that guest gameplay updates at the same cadence everywhere.
