# Lagoon X68000 Enhanced · Windows playtest preview

This is the English Sharp X68000 version of Lagoon with the Enhanced features built in. The Windows x64 ZIP includes Companion and the setup tools. Setup builds the playable game on your PC using your four Lagoon disk images and X68000 firmware.

## What’s new

- Widescreen with square pixels and live **1×**, **1.25×**, and **1.5×** view settings. The centered camera follows Nasir within each room’s scroll limits.
- A wider view of terrain, actors, and cloud paths with sprite clipping fixes and steadier room fades.
- A 49-map Atlas with terrain, Nasir’s position, current-room doorway markers, visited rooms, and recorded crossings.
- In-game Items guide, Journal, eight offline achievements with unlock chimes, Controls, and Settings for zoom, volume, mute, and quitting.
- Faster startup and shorter waits at ordinary doors and in Items and Equipment. Press **Start / Enter** to skip the opening.
- Longer sword reach with a small cleave, 50% stronger damaging magic, faster movement, more recovery time, a longer jump peak, a gentler opening, and double EXP and gold. These changes are always on.
- Three normal save slots on a writable User Disk, quick save and load, and controller shortcuts for menus and the Atlas.

## First setup

1. Extract **Lagoon-X68000-Enhanced-preview.1-win-x64.zip** to a folder you can write to. Open the extracted folder.
2. Install **Python 3.11 or newer** and **Visual Studio 2022 Build Tools** with the **Desktop development with C++** workload, including MSVC x64/x86 tools, Windows SDK, and CMake. [Python downloads](https://www.python.org/downloads/windows/) · [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
3. Put your **four Lagoon DIM disk images**, **`iplrom.dat`** (128 KiB), and **`cgrom.dat`** (768 KiB) in the **Inputs** folder. Extract the DIMs first if you have them in an archive. Setup recognizes the supported Japanese System disk and the patched English 0.9b System disk.
4. Run **Setup X68000.cmd**. It fetches the pinned RetroRecomp converter and, when using the Japanese System disk, applies the English 0.9b patch. Keep the window open while setup downloads compiler dependencies, builds, and verifies the game. The first run can take several minutes and caches its build tools in **`%LOCALAPPDATA%\L68`**. If GitHub downloads fail, put the official portable **`Retro-Recomp.exe`** and, if needed, the extracted English System **`.bps`** in **Inputs**, then run setup again. [RetroRecomp 1.8.0](https://github.com/ArthurReboulSalze/RetroRecomp/releases/tag/v1.8.0) · [English patch 0.9b](https://github.com/nleseul/lagoon_x68k_trans/releases/tag/0.9b)
5. Once setup says it’s complete, open **Play X68000.cmd** and click **Play** in Companion. The ZIP includes the launcher runtime. The game and saves use the extracted preview folder.

The download contains launcher code and build tools. Your disks, firmware, saves, and the locally generated game stay on your PC.

## Controller and recording

The preview starts **windowed** with **sharp square pixels**, **1.5×** gameplay view, and **80%** master volume. In OBS start with **Game Capture → Capture specific window → Lagoon**. Make a short test recording and switch focus away and back to check that video and audio keep moving on your setup. **Window Capture** is another capture option.

| Action | Controller | Keyboard |
| --- | --- | --- |
| Skip the opening | Start | Enter |
| Open Atlas | R3 | Ctrl+M |
| Switch Atlas tabs | LB / RB | Shift+Tab / Tab |
| Browse visited rooms on Map or Routes | LT / RT | Page Up / Page Down |
| Cycle gameplay zoom | LT | Ctrl+Z |
| Open Settings while playing | RT | Ctrl+, |
| Open Save / Load | Y | F1 with guest function keys enabled |
| Confirm a choice in the original Save / Load | B | D |
| Quick save / load | Keyboard only | F8 / F9, or Ctrl+F8 / Ctrl+F9 in guest function key mode |

Display mode and pixel filter are in Companion. In-game Settings applies zoom and volume live. Use **Quit to desktop** in Settings to exit. See [all controls](x68000/PLAYING.md).

## What to report

This preview is for a fresh run through the ending. If anything looks wrong, note the room or boss, the zoom and display mode, and what you did just before it happened. Clips are especially helpful for briefly disappearing sprites, cloud edges, flashes during room changes, audio issues, or OBS freezing when the game gets focus. Send setup error text or a clip through [GitHub Issues](https://github.com/rages4calm/LagoonSNESRecomp/issues). Leave disk images, firmware, and saves out of reports.

Thanks to **Zoom** for the original game, **nleseul** for the English translation, **Arthur Reboul-Salze** for RetroRecomp, and **DarkShadowRage** for requesting the X68000 version and helping test Lagoon.

## Build the Companion launcher from source

The [CompanionLauncher project](https://github.com/rages4calm/companion-launcher) supplies the MIT-licensed Core and Windows packages at version 0.2.0. Point the `COMPANION_LAUNCHER_FEED` environment variable at the directory containing those two NuGet packages, then use the .NET 8 SDK to publish the launcher.

```powershell
dotnet publish launcher/Game.Companion.csproj -c Release -r win-x64 --self-contained -o local-x68000/companion -p:RestoreLockedMode=true
```
