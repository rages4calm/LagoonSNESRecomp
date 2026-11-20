# Lagoon · X68000 Enhanced

Open **Start Lagoon.cmd**, choose **X68000 Enhanced**, and press **Play**.
There is one X68000 game to play. Its gameplay improvements are built in.
The separate **SNES Enhanced** choice opens the installed SNES game.

## Start playing

- **Start / Enter** skips the opening. You can press it as soon as the game
  window opens, even before the logo appears.
- The correct floppies, including the writable User Disk, mount automatically.
- Use the **D-pad / arrows** to move. **S / D** correspond to the controller's
  **A / B** buttons in the original game.
- **Esc** opens the exit prompt in normal host keyboard mode.
  **Ctrl+Alt+H** opens the player's control guide.
- The launcher's **Display** page offers window, borderless and pixel-filter
  options. It does not change gameplay rules.

## Built-in changes

- Widescreen draws the live terrain and actors beyond the original view.
  **1×**, **1.25×** and **1.5×** keep square pixels and reveal more of the room
  without changing the camera target. The expanded modes move the original
  HP and MP display to the bottom. The outer view draws enemies already active
  in the game's simulation.
- The camera follows Nasir from the center as he walks, within each room's
  scroll limits. Ordinary doors pass through their fixed blackout waits faster.
- An equipped sword reaches up to six pixels farther, with two pixels of
  cleave on either side. Attacks still respect walls. Damaging magic deals
  50% more after defense and resistance.
- Nasir walks three pixels per movement step. Jumps hold the middle pose eight
  updates longer. Ordinary hits give sixteen updates of extra protection.
- A new game starts with 18 HP and 8 base defense. Maximum HP reaches 21 at
  level 2. Combat rewards give **2× EXP and gold**.
- Items and Equipment open and close faster. The original Save / Load menu
  writes to a real writable User Disk.

## Controls and Atlas

| Action | Keyboard | Controller |
| --- | --- | --- |
| Open Items | I | LB |
| Open Equipment | E | RB |
| Open Lagoon's Save / Load | F1 with guest function keys enabled | Y |
| Confirm an original save menu selection | D | B |
| Open Atlas | Ctrl+M | R3 |
| Cycle view zoom | Ctrl+Z | LT |
| Open Settings | Ctrl+, | RT |

When the Atlas is open, the **D-pad / arrows** pan the map, **Y / Z** cycles
fit, 2× and 4×, and **LT / RT** or **Page Up / Page Down** browse visited rooms.
**LB / RB** switches tabs. **Tab / Shift+Tab** and **View** also work.
**A / Enter** opens a recorded destination in Routes,
**X / Home** returns to the current room and **B / Esc** closes the Atlas.

The Atlas pauses the game and fits each of its 49 maps to the room. The map
shows the original terrain and foreground details, with a bright marker for
Nasir's current or last position. Cyan outlines mark ordinary doorways in the
current room before you use them. Some still require story progress. Numbered
pins mark crossings you've already made.
**Items**, **Journal**, **Awards** and **Controls** show item effects, player
stats, eight achievements, and the active bindings. Set custom bindings in the
player's **Options**.

Open **Settings** with **Ctrl+, / RT** during gameplay, or select its tab in the Atlas.
Use the **D-pad / arrows** to select a row and left/right to change volume or
zoom. Use **A / Enter** to mute or open the quit prompt. Audio and view zoom
are saved to the player's preferences. The quit prompt defaults to
**Keep playing**.

## Saves

In the original Save / Load menu, choose an action with the **D-pad / arrows**
and confirm with **B / D**. Wait for the three slots to appear, then confirm a
slot. **F8 / F9** save and load a quick state. If the X68000 receives function
keys directly, use **Ctrl+F8 / Ctrl+F9**.

The unified game uses the existing Expedition data folder under
`local-x68000/retrorecomp-expedition/datas/`. The previous Expedition build's
quick states and ordinary saves use the same game identity.

To bring over an **ordinary in-game save** from an older User Disk profile,
start X68000 Enhanced once and close it. In the edition picker choose
**Import older X68000 saves…** to copy a selected save into an empty slot,
then open Lagoon's own Load menu. Quick states from different media builds
remain tied to their original build.

## Rebuild on Windows

Use the official portable [RetroRecomp v1.8.0](https://github.com/ArthurReboulSalze/RetroRecomp/releases/tag/v1.8.0)
and Microsoft's Visual Studio C++ Build Tools. Place the four game DIM images
in `Lagoon_Sharp-X68000_EN/`, the verified English System disk at
`x68000/patched/System-EN.dim`, and the 128 KiB IPL and 768 KiB font firmware
at `local-x68000/firmware/iplrom.dat` and `local-x68000/firmware/cgrom.dat`.
The alternate System disk (CRC32 `684A43EF`) uses the English 0.9b patch from
[nleseul](https://github.com/nleseul/lagoon_x68k_trans). Add
`--patch <path-to-bps>` if building from the alternate disk and patch.

Run from the LagoonExtra directory:

```powershell
python -B -P x68000/tools/build_retrorecomp.py --converter "local-x68000/converter/Retro-Recomp.exe" --ipl "local-x68000/firmware/iplrom.dat" --cgrom "local-x68000/firmware/cgrom.dat" --user-disk --expedition --exp-rate 2 --gold-rate 2 --keep-build
python -B -P x68000/tools/build_atlas_player.py "<conversion report from the first command>" "<scratch folder>" --expedition --install
powershell -File scripts/build_launcher.ps1
```

The launcher requires the verified enhanced player and its assembled Atlas
runtime. The build embeds the local disks and firmware in the playable export.
These inputs, generated code, executables and saves stay local.
