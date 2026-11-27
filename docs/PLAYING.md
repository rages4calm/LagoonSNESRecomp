# Playing Lagoon: both editions

Windows 0.6.1 opens **SNES Enhanced** or **X68000 Enhanced** in one Companion.
Use **Choose edition** in its sidebar to switch. Linux/macOS 0.5.3 downloads
contain SNES Enhanced only. The editions have separate stories, controls,
settings and saves. [X68000 controls and saves](#x68000-enhanced).

Controller labels below are **Xbox-style physical labels** used by PC input
APIs. Nintendo and PlayStation pads may print different symbols. The original
virtual SNES button labels are identified separately in the gameplay table.
**L2/R2** means **LT/RT** on an Xbox-style pad; **R3** means pressing the right stick.

## SNES Enhanced

Use the Windows 0.6.1 or portable SNES 0.5.3 package from the [main page](../README.md), or build from
[these instructions](BUILDING.md). On first launch, choose your clean USA ROM with the file picker.
Verification and setup run automatically; then press Play. Use Choose ROM.cmd
or Choose ROM.command to reopen the picker later. Apply launcher settings before
starting, or restart to use those changes. In-game zoom and audio adjustments
apply live. Normal release execution is 100% native for the main game CPU;
the interpreter is an explicit developer reference option. Graphics and audio
hardware continue through the runtime.

The scenery map and five-tab Options menu are included.

### SNES keyboard and controller defaults

| Action / virtual SNES input | Keyboard | Physical controller |
| --- | --- | --- |
| Move | Arrows | D-pad |
| Sword / SNES B | Z | A |
| Jump / SNES A | X | B |
| Talk / SNES X | S | Y |
| SNES Y | A | X |
| SNES L | C | LB |
| Magic / SNES R | V | RB |
| Main game menu / Start | Enter | Start |
| Select | Right Shift | View / Back |

The original game calls its sword button **B**, but the default PC controller
binding is physical **A**. Keyboard **Z** also swings the sword. Companion's
Controller page edits these assignments; Apply and restart after rebinding.

### SNES display

**Square pixels**, **widescreen** and **1.5x world zoom out** can all be enabled
together. Square pixels keep equal horizontal/vertical pixel widths; Original
CRT proportions recreate the horizontal television correction (about17% wider).
Choose either in **Display → Pixel proportions**. New packages use square pixels;
existing native settings are preserved. Apply and restart after changing it.

World zoom out is a separate **Display** choice: **1x**, **1.25x** or **1.5x**.
It expands the playable world horizontally and vertically while keeping the HUD
readable. You can also change it live in **Options → Display → World zoom out**:
use Up/Down to select the row, then Left/Right or Enter/controller A to cycle.
Close Options to resume with the new view. During ordinary gameplay, **Ctrl+Z**
or clicking and releasing the right stick (**R3**) cycles the same three choices.
**Select+R3** keeps its existing rewind gesture and does not change zoom.
The original titles, inventory and authored UI retain their intended composition.


For a 1920×1080 monitor, choose **Borderless fullscreen**, widescreen **16:9** and VSync.
Enhanced original pixels (EPX or MMPX) is optional: it treats diagonal pixel edges using
original colors while keeping the bottom HUD sharp. It uses D3D11 on Windows
or OpenGL with the bundled shaders elsewhere. Turning it off restores the previous graphics selection.
Linear filtering and frame blending should remain off for a sharp picture.
The retired Smooth scaling option is no longer offered.

Window scale changes the size of a window, not the fullscreen field of view.
Leaving it at 3 is fine in fullscreen. 21:9/32:9 reveal more horizontal scenery
but produce a shorter, letterboxed picture on a 16:9 monitor. Widescreen adds
world scenery at the existing game scale; it is not a zoom-out feature.

Ordinary supported dialogue retains the surrounding world, and qualified menu
frames expand using original artwork. Titles, portrait layouts, finite map edges,
unsupported effects and unknown resources can retain the original geometry.
Additional drawn sprites retain their original collision rules. Ordinary enemies
in the expanded horizontal view can also run their original AI; rendering and
simulation coverage are separate checks. Recent fixes preserve side margins
while moving vertically and prevent the old doorway from flashing back during
room loading.

For OBS, use **Game Capture → Capture specific window → LagoonSNESRecomp.exe**.
[Recording and streaming help](RECORDING.md).

### SNES quality-of-life options

Gameplay toggles default off in Companion's Quality of life page. EXP and gold
have separate selectors defaulting to original rewards. Opening balance affects
new games; reward choices affect future enemy and boss rewards, not saved totals.

| Option | Effect |
| --- | --- |
| Sword reach | Four forward extents become 38 pixels; perpendicular edges gain 3 pixels of cleave. Qualified sword poses gain a matching 12-pixel visual blade extension. |
| Follower speed | Fallback escort movement increases from 1 to 3 pixels per frame; the built-in obstacle-aware follower already uses Nasir's walking pace. Authored destination walks retain their precise original steps. |
| Walking speed | Nasir's ordinary 3-pixel movement becomes 5; enemies and hurt knockback remain stock. |
| Hit recovery | Eight additional frames reject ordinary consecutive hits. |
| Opening balance | Start at 18 HP and 11 defense; level 2 uses 21 HP and 13 defense. Level 3 onward follows stock growth. |
| EXP rewards | Original, legacy 1.5x, or 2x–8x enemy/boss EXP. Multipliers replace one another and never stack. |
| Gold rewards | Original or 2x–8x enemy/boss gold. Shop prices, purchases, quest rewards and existing saved gold stay unchanged. |
| Jump hold | Hold the original jump peak for eight frames while horizontal movement continues. |
| Magic power | Nonzero player magic powers increase 50%, rounded down. |
| Centered camera | Follow Nasir using the game's original scenery streaming. |

Faster Items/Magic/Equipment transitions and Giles's first aid are built into the
port. Speak to Giles, choose Yes and follow the escort quest; no Healing Pot is
required or consumed. The ROM remains clean. These are native port features
rather than applying the supplied BPS at launch. [Credits](../CREDITS.md).

### SNES escorts and the map

Giles and Thor find a walkable route toward Nasir's recent position, keeping a
short gap. They account for their own collision footprint around walls, corners
and stairs. Movement stays capped at Nasir's walking pace, including when they
fall behind; old movement is never replayed as a fast-forward catch-up. Loading
a save clears the disposable route and lets them find their way back to Nasir.
During ordinary following, you can walk through your escort when reversing
direction. Walls, enemies, ordinary NPCs, doors and quest scripts retain their
original rules. Scripted movement and
contexts that cannot be qualified retain the original behavior. This is built in
and does not require enabling a mod. During an active escort, run-ahead yields to
ordinary simulation so speculative frames cannot discard navigation state.
Thor's scripted walk to the second tablet keeps its original step size, fixing
the back-and-forth stall caused by applying the follower speed boost to that route.

Press **M** or controller **L2** to open the map. Gameplay pauses while it is open.
The current area's complete scenery is shown when available. This is terrain
drawn from your own game, including its background layers, rather than a camera
screenshot. It excludes moving characters, enemies and the HUD. L/R browsing
keeps each area's last mapped scenery, including after a restart. Areas from
older exploration-only files gain scenery when you visit them once with 0.4.0;
areas without captured scenery retain their labeled explored-layout view.

| Map action | Keyboard | Controller |
| --- | --- | --- |
| Pan | Arrow keys | D-pad |
| Zoom in / out | + / - or A / X | A / X |
| Recenter on Nasir | Home or Y | Y |
| Fit the whole area | 0 or Enter | Start |
| Browse visited areas | [ / ] or PageUp / PageDown | L / R |
| Next discovered-exit page | Tab | Select |
| Close | M or Escape | L2 or B |

Doors are landmarks. Discovered exits receive numbers that match the paged
destination list; destinations stay unnamed until you visit them. An exit
marker does not mean its story gate is unlocked. Treasure markers appear after
discovery, and opened chests are dimmed. Complete scenery visibility does not
add exploration credit, grant treasure or guarantee every tile is walkable.

Exploration saves separately in `saves/lagoon-exploration.map`; local terrain is
stored in `saves/lagoon-scenery.maps` (at most about 5.3 MiB). Both belong with your
saves when upgrading. Loading an earlier state retains mapped places without
changing that state's story or inventory. No map artwork is downloaded or included
with the port. Scenery covers supported areas, including the layered
backgrounds in Gold Cave, Philips Castle, Dwarf Cave, Phantom Hill and the Cloud;
loading or unsupported views can use the explored layout instead.
During the early tablet quest, the map also shows the next step for the Tablets
of Faith, Wish and Hope, then the return to Voloh's elder. Purple markers identify
applicable objective locations. Thor provides the first two tablets through his
original dialogue; the third requires Nasir to reach the chest near Philips
Castle. Hints do not grant items or change quest flags. Options also has a
chapter journal; the map's pan/zoom changes only the map, not gameplay zoom.

### SNES saves and states

Normal in-game saves remain available. **F1 loads slot 1; Shift+F1 saves it.**
F2–F10 select the other slots, with Shift to save; F11 opens the state browser.
In a source checkout, settings live in `build/config.ini`, bindings in
`build/keybinds.ini`, and saves in `build/saves`. The downloadable release uses
`local/config.ini` and `local/saves` in its extracted folder; keyboard controls
remain beside its executable in `game/keybinds.ini`.
The launcher does not migrate an unrelated older save folder automatically.

Development state formats can change. Incompatible or corrupt states are refused
before changing the running guest. Use normal game saves for campaign progress.
Version 0.5.3 updates SNESRecomp's audio timing state and requires freshly created
quick save states. Load your normal in-game save to continue from an earlier
release, then create new quick states. Existing incompatible state files are
preserved; changing their version number cannot safely convert them.
Transient extra recovery/jump timers reset on a state load, so a state saved during
one of those effects does not reproduce its remaining optional hold exactly.

### SNES hotkeys and quick settings

Companion has separate **Keyboard**, **Controller** and **Hotkeys** pages.
Keyboard controls use native SDL key names; system shortcuts also accept modifier
combinations such as `Ctrl+p`. The Controller page assigns buttons to the twelve
SNES inputs. L2 and R2 are reserved for the map and quick settings. Apply and
restart after changing bindings. Existing controller selection and player-two
bindings are retained.

| Default shortcut | Action |
| --- | --- |
| M / controller L2 | Open or close the scenery/exploration map. |
| Ctrl+comma / controller R2 | Open or close quick settings. |
| Ctrl+J | Open or close achievements. |
| Ctrl+Z / controller R3 (release) | Cycle gameplay zoom: 1x → 1.25x → 1.5x. |
| Controller Select+R3 | Open rewind; does not cycle zoom. |
| Arrow keys / D-pad | Pan the map or navigate the selected Options tab. |
| PageUp / PageDown, Tab / controller L or R in Options | Switch Sound, Journal, Items, Display and Travel tabs. |
| Left/Right in Sound | Adjust the selected volume by 5%. |
| Enter / controller A or Y on Achievements | Open the achievement browser from Sound. |
| Escape / controller B | Close a panel; cancel Travel or Quit confirmation first. |
| Keypad + / Keypad - | Raise or lower master volume during gameplay. |
| Alt+Enter | Toggle fullscreen. |
| F | Toggle the performance display. |
| F11 | Open the save-state browser. |
| F12 | Open rewind. |
| F1 through F10 | Load quick-state slots 1 through 10. |
| Shift+F1 through Shift+F10 | Save quick-state slots 1 through 10. |
| P / Shift+P | Pause with dimmed picture / pause. |
| Tab, held | Fast forward. |
| Ctrl+R | Reset game; save normal progress first. |
| Ctrl+Up / Ctrl+Down | Increase / decrease window scale. |
| Alt+W | Toggle widescreen. |
| R | Toggle renderer. |

The screenshot shortcut is unassigned by default and can be set in Companion.

Quick settings pause guest execution, so changing a volume does not advance the
battle or escort. Master volume affects all audio. X68000 music volume trims only
the optional replacement soundtrack; its sound effects retain the master level.
These settings persist without returning to Companion. Changing the soundtrack
itself still requires Companion and a restart.

The **Journal** follows current story, boss and item progress. It gives a chapter
recap, the next suggested step and relevant milestones; it is not a log of every
conversation or a promise that every optional quest is tracked.

The **Items** reference has 25 consumable, equipment and quest-item entries.
Up/Down browses their effects or purpose, whether Nasir carries them, and their
Atland/Voloh shop prices where sold. It does not select, use, buy or grant an item.

Under **Display**, Up/Down selects **Sword range cue** or **World zoom out**.
The sword cue defaults **Off**; Left/Right or Enter/A toggles the visual guide
following the active reach and cleave. It changes neither damage nor hit detection.
World zoom out cycles **1x / 1.25x / 1.5x** using the same controls and applies
without restarting the game. Both choices persist with settings; if zoom cannot
be written to the INI, it still applies for this session and Display shows a warning.

To quit without Alt+F4, open **Options → Sound → Quit to desktop**. The
confirmation defaults to **Cancel**. Select **Quit** with Left/Right, then press
Enter/controller A. Escape/B cancels and leaves Options open. Save your progress
first; confirming uses the normal application shutdown path.

Under **Travel**, **Return to dungeon entrance** is available after the dungeon's
boss is defeated in Gold Cave, Philips Castle, Dwarf Cave, Siegfried Castle,
Ice Cave and Lagoon Castle. Stop on safe ground, away from nearby enemies or
effects, and finish any escort first. Boss arenas, transitions and incompatible
play settings refuse the action. It returns just inside the main entrance using
the game's room-loading sequence, rather than unlocking a route ahead.

Press Enter/A once to open the confirmation and again to return. Escape/B
cancels the confirmation and keeps Options open. Eligibility is checked again
when the request reaches gameplay, so an unavailable return will show a reason.

### SNES achievements and hidden visitor

The hidden visitor accepts controller Y or keyboard **E** when nearby and
facing him. Enter/A/Y reveals the rest of a typing page, then advances on the
next press; Escape/B cancels. The original-style frame and Crono portrait
retain their proportions while the music continues and the world pauses.
Completing his conversation plays the selected laugh before the achievement chime.

**Ctrl+J** opens the in-game browser. With a controller, press **R2**, select
**Achievements** and press **A** or **Y**. Up/Down selects entries; Left/Right
changes page. **Escape**, controller **B** or **R2** closes it. Gameplay pauses
while browsing. The Companion **Achievements** page shows the same earned,
locked and partial progress; the launcher only reads progress and never awards it.

There are 26 achievements covering bosses, levels, treasure, early quests, gold,
the completed ending and a hidden encounter. Locked secret text stays masked in
both browsers. Each new unlock has a short notification and a synthesized chime.
The chime follows **master volume**, including mute, rather than the X68000 music
trim. Notifications wait while panels/dialogue are open.

Progress works offline; there is no account or server. It belongs to the selected
game save folder and is stored separately at
`Achievements/lagoon/native-qol/unseeded/progress.json` beneath it. In the release,
that is inside `local/saves`; in the normal source checkout, inside `build/saves`.
Loading an older game save or state does not erase earned achievements. Extracting
a separate package gives it independent local progress unless you transfer your
save folder yourself.

Loading a save establishes a baseline and does not retroactively award its
existing accomplishments or replay unlock sounds. Continue playing to earn
future qualifying milestones. Scripted/assisted diagnostic sessions disable
tracking. A corrupt/unreadable progress file or another running copy leaves
gameplay available and shows a tracking status; the game does not reset the file
or change guest HP, items, rewards, flags or clocks to award achievements.

### SNES audio

On Companion's **Audio** page, choose **Original SNES** or **X68000**, Apply and
restart the game. The release includes the converted X68000 pack; no folder
selection or conversion is needed. Missing/unmapped cues retain SNES music and
sound effects remain on the original driver. Save-state loads restart the
replacement track. Run-ahead uses SNES audio. The supported X68000 cues use
attenuation based on measured perceived loudness to reduce jumps between areas,
including Voloh; PCM data and loop points are unchanged. Master and X68000 music
levels can be changed in the in-game quick settings panel. See [soundtrack details and MP3
listening downloads](SOUNDTRACK.md).

### Reporting a bug

The owner and friends perform the uninterrupted campaign playtest. Include:

- Edition, location, nearest save/slot and exact steps/buttons to reproduce.
- Expected behavior and what happened; a short image/video for visual faults.
- Executable version/hash, aspect, Enhanced setting, QoL choices and controller.
- Relevant crash/log text, and whether the problem repeats after reloading.

For slowdown, mention the room and enemies present, and whether movement,
animation, audio or only presentation slows down. A steady presentation counter
alone does not prove equal gameplay cadence everywhere.
### SNES world view and recording

Companion's Display page offers **World zoom out: 1x, 1.25x, 1.5x**. Enable
widescreen first. During play, **Options → Display**, **Ctrl+Z**, or a click and
release of controller **R3** changes zoom live and saves the preference. The
**Select+R3** rewind gesture remains separate.

Zoom reveals more scenery vertically and horizontally without changing Nasir's
movement or sword collision. Distant ordinary enemies wander without increasing
attack or awareness ranges; their original behavior resumes in the original
viewport. Original cutscenes/unsupported visual states retain their authored
composition. HUD and host menus keep their readable pixel size.

On Windows, **Auto (D3D11)** is the default graphics backend. Enhanced original
pixels runs on the GPU with the HUD and menus kept sharp. OpenGL is available
as an alternative. Linux/macOS use OpenGL and share the world-view options.
See [recording instructions](RECORDING.md) for OBS and borderless fullscreen.

Enhanced pixel style selects **EPX — crisp** or **MMPX — refined** when Enhanced
pixels is enabled. Both use sharp fractional scaling and preserve readable HUD
text. MMPX reconstructs curves/diagonals; it does not replace the original art.

## X68000 Enhanced

Choose **X68000 Enhanced** in Companion and finish its one-time local setup.
Select your four-disk ZIP/folder, then **Set up game**. Setup obtains its portable
tools and compatible firmware automatically; custom firmware is optional under
the advanced setting. [Setup details](BUILDING.md#x68000-local-build-and-setup).
Its gameplay improvements are always enabled; display/audio preferences remain adjustable.

### X68000 controls

| Action | Keyboard | Physical controller |
| --- | --- | --- |
| Move | Arrows | D-pad |
| Keep sword ready | Hold S | Hold A |
| Jump / talk / original dialogue confirm | D | B |
| Skip opening | Enter | Start |
| Items | I | LB |
| Equipment | E | RB |
| Original Save / Load | F1 with guest function keys enabled | Y |
| Atlas | Ctrl+M | R3 |
| Cycle world zoom | Ctrl+Z | L2 |
| In-game settings | Ctrl+comma | R2 |
| Save quick state | F8 | Use keyboard shortcut |
| Load quick state | F9 | Use keyboard shortcut |
| Control guide | Ctrl+Alt+H | Use keyboard shortcut |
| Player Options / remapping | Ctrl+Alt+O | Open the player's Options menu |
| Quit prompt | Escape, or Settings -> Quit | Settings -> Quit |

**Hold Sword, then move into the enemy.** This edition uses thrust/contact
combat. Stationary or wall-blocked movement does not deal a hit. Sword artwork,
readiness, physical damage rolls and enemy death handling follow the original.
Leave autofire off for ordinary play.

**Original dialogue and Save/Load confirm with B / D.** Sword cancels a yes/no
prompt to No. Choose Yes with Down, then confirm with B/D. The added Atlas and
Settings use A/Enter to confirm and B/Escape to close.

**Ctrl+Alt+K** toggles full guest keyboard mode. In that mode F1 reaches the
original Save/Load menu, and F8/F9 go to the X68000 instead of host quick states.
Use **Ctrl+F8 / Ctrl+F9** for host states while guest keyboard is enabled.
Toggle back to game-control mode to restore the normal S/D shortcuts; a physical
controller still works in guest keyboard mode. Controller **Y** opens Save/Load
directly, without changing keyboard modes.

Additional player shortcuts in normal game-control keyboard mode:

| Key | Action |
| --- | --- |
| F1 | Player control guide; guest F1 is separate as described above |
| F2 / Ctrl+Alt+O | Player Options and bindings |
| F3 | Cycle picture filter |
| F4 / Ctrl+Alt+Enter | Cycle display mode |
| F6 | Toggle autofire; leave Off for ordinary sword combat |
| F7 | Cycle player UI language; the story translation stays English |
| F11 | Pause / resume |
| Ctrl+Alt+H | Open control guide in either keyboard mode |

The player's control guide lists the remaining machine/debug shortcuts. Disk
changes are automatic; normal play does not require manual disk swapping.

### X68000 Atlas and settings

The Atlas pauses gameplay and fits one of 49 area maps to the room. It shows
original terrain/foreground details, Nasir's marker, ordinary doorways and
recorded crossings. Door markers may still require story progress. Items,
Journal, Awards, Controls and Settings provide references, regional reminders,
eight offline awards and active bindings. A visit does not mark every quest complete.

| Atlas action | Keyboard | Controller |
| --- | --- | --- |
| Pan | Arrows | D-pad |
| Cycle fit / 2x / 4x map zoom | Z | Y |
| Browse visited rooms | PageUp / PageDown | L2 / R2 |
| Switch tabs | Tab / Shift+Tab | LB / RB; View also works |
| Open selected recorded destination | Enter | A |
| Return to current room | Home | X |
| Close | Escape | B |

In Settings, Up/Down selects rows, Left/Right adjusts volume or zoom, and A/Enter
mutes or opens the quit confirmation. B/Escape cancels. Audio and world zoom
persist; Quit defaults to Keep playing.

Widescreen uses an expanded 16:9 canvas with square pixels, fitted to the window.
World zoom offers **1x, 1.25x and 1.5x**; new
installs default to **1.25x**. It reveals more scenery without changing the camera
target. Zoomed views move the original HP/MP display to the bottom. The outer
view draws enemies already active in the original simulation. Map visibility
does not make every region walkable or extend combat through walls.

### X68000 title and opening — Windows 0.6.1

The title and prologue keep their original artwork and
normal timing. Press **Start / Enter** to skip deliberately. They no longer use
the gameplay widescreen canvas or automatic opening fast-forward.

**Windowed** and **Borderless · fit** enlarge the original picture proportionally.
**Borderless · pixel perfect** retains integer scaling and may leave larger
margins. The opening also keeps its proportions in Stretch mode. Square original
artwork naturally leaves sidebars on a wide monitor; this does not crop or redraw
it. The gameplay **1x / 1.25x / 1.5x** zoom setting does not zoom the opening.
New windows start at up to 1280×720, limited to the available desktop area;
existing saved window sizes and display preferences are preserved.

### X68000 gameplay and built-in changes

This edition has **38-pixel sword reach**, **4 extra pixels of cleave on each side**,
three-pixel walking steps, eight extra jump-peak updates and sixteen extra
protection updates. Damaging magic is 50% stronger after defense/resistance;
enemy/boss EXP and gold are **2x**. New games start with **18 HP / 8 base defense**;
level 2 maximum HP is 21. Items, Equipment, dialogue and ordinary door waits are quicker.

Giles accepts help without spending a Potion and returns to town through the
original event. There is no Giles escort. Thor has the X68000 story/cutscene/boss
role, without the SNES tablet hunt. Follow this edition's Journal; the two games
have different routes. A [dedicated X68000 walkthrough](https://gamefaqs.gamespot.com/x68000/936018-lagoon/faqs/72538)
provides the full route.

In the icy region, **select the Fur Cloak in Items**. Carrying it alone does not
protect against cold. Boosted HP cannot prevent death from falling off a cloud
or into a pit.

### X68000 saves

The correct disks, including a writable User Disk, mount automatically. Open
original Save/Load with **Y**, choose the action and slot with the D-pad, then
confirm with **B**. Keyboard users can use guest F1 and confirm with D after
returning to game-control mode.

**F8 saves a quick state; F9 loads it.** These keys differ from SNES. Fresh states
include dialogue scenery and saved zoom. Older source identities/presentation
formats are refused; ordinary in-game saves carry progress across those updates.

Data lives under **local-x68000/retrorecomp-expedition/datas/**. Updates that
change disk identity use a separate data folder. Use **Copy X68000 saves** in
Companion to copy an ordinary save into an empty destination slot. Start the new
edition once and close it before importing; close any other running copy too.
Import preserves current game files and copies only the selected save sector.
Do not replace a new User Disk with an entire older disk: it also contains game
files and can undo updated scripts. Quick states cannot cross source identities.

## Upgrading and your progress

### Integrated Windows updates — 0.6.1

For your first update from **0.6.0**:

1. Close both games and Companion.
2. Extract the **whole 0.6.1 ZIP over your current release folder**, accepting
   replacement of packaged files. Keep **local/** and **local-x68000/**; neither
   is included in the ZIP.
3. Open Companion. Choose **Update X68000 game**, then **Update game**, to apply
   the opening fixes. Re-select your disk ZIP if asked, let it finish, then play.

Version 0.6.0 has no integrated updater. After this first upgrade, choose
**Check for updates**, then **Download and update** when a newer stable package
is available. Companion restarts after application. Package updates replace
the released application files while preserving **local/** and
**local-x68000/**, including the locally compiled X68000 executable. Close both
games before applying an update; the updater does not forcibly terminate them.
An unavailable update server does not require repeating game setup.

**Update X68000 game** is a separate operation. A launcher-only update does not
require compiling the game again. Changes to the X68000 runtime do require a
local rebuild; cached game inputs and tools are reused when available. If those
inputs have been cleared, select your original game ZIP again. Your existing
game remains available while deciding whether to rebuild. Quick states still
need to match the runtime source/format; keep ordinary in-game saves for upgrades.
The update window reports **Already up to date** without rebuilding when the
installed runtime matches. An interrupted package update offers **Resume update**;
**Official releases** opens the download page.

Automatic package replacement is supported in extracted release folders. A
source checkout can check the latest release and open its download page, but
the updater does not replace your Git working tree. Linux/macOS retain their
existing manual SNES-only update procedure.

### Manual updates

If you modified packaged application/source files, extract the full new package
into a fresh writable folder instead of overwriting your changes. The integrated
updater refuses to overwrite locally edited managed files. Transfer progress as
follows; the same procedure works for a normal manual move to another folder.

- **SNES:** transfer **local/** to retain normal saves, settings, map/scenery and
  achievements. The prebuilt game stays in **game/**. Source checkouts use
  **build/config.ini**, **build/keybinds.ini** and **build/saves/** instead.
- **X68000:** retain **local-x68000/**, and use Companion's ordinary-slot copying
  tool after the new edition creates its User Disk. Do not transplant its old
  bootable disk over a new template. Its locally compiled executable contains
  private inputs and should stay private.
- **Quick states:** recreate them when format/source identity changes. Renaming
  a file or editing its version bytes cannot safely convert it. Windows 0.6.1
  retains the SNES 0.5.3 engine and its existing state-format requirements.

SNES and X68000 saves and achievements are independent. Include the edition in
bug reports. Do not upload ROMs, disk images, firmware or generated payloads.
