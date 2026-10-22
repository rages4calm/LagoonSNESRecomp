# Playing and settings

The hidden visitor accepts controller Y or keyboard **E** when you are nearby
and facing him. Enter/controller A or Y reveals the rest of a typing page,
then advances it on the next press; Escape/controller B cancels. His conversation
uses the original Lagoon lettering and frame, with a Crono portrait. Music
continues while the world pauses. Completing it plays a owner-selected Chrono Trigger laugh
before the achievement chime.

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
required or consumed. The ROM remains clean. These are shared native/interpreter
hooks rather than applying the supplied BPS at launch. [Credits](../CREDITS.md).

## Escorts and the map

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

Press **M** or controller **L2** to open the explored map. Gameplay pauses while it is
open. Left/Right or the shoulder buttons browse visited areas; **Y** returns to
Nasir's area. **M**, **L2**, **Escape** or controller **B** closes it.

The map reveals nearby observed terrain, Nasir's trail, exits and discovered
chests. Hollow chest markers mean opened. Exit links name destinations you have
visited; unexplored destinations stay hidden. An exit marker does not mean its
story gate is unlocked. Area IDs distinguish rooms with the same location name.
The map is a record of exploration, not a guarantee that every ground tile is
walkable. Exploration saves separately in `saves/lagoon-exploration.map`; loading
an earlier state retains places already explored. No map artwork is downloaded.
During the early tablet quest, the map also shows the next step for the Tablets
of Faith, Wish and Hope, then the return to Voloh's elder. Purple markers identify
applicable objective locations. Thor provides the first two tablets through his
original dialogue; the third requires Nasir to reach the chest near Philips
Castle. Hints do not grant items, reveal terrain or change quest flags. A full
objective journal, guidance for other quests and zoom-out remain future work.

## Saves and states

Normal in-game saves remain available. **F1 loads slot 1; Shift+F1 saves it.**
F2–F10 select the other slots, with Shift to save; F11 opens the state browser.
In a source checkout, settings live in `build/config.ini`, bindings in
`build/keybinds.ini`, and saves in `build/saves`. The downloadable preview uses
`local/config.ini` and `local/saves` in its extracted folder; keyboard controls
remain beside its executable in `game/keybinds.ini`.
The launcher does not migrate existing saves.

Development state formats can change. Incompatible or corrupt states are refused
before changing the running guest. Use normal game saves for campaign progress.
Transient extra recovery/jump timers reset on a state load, so a state saved during
one of those effects does not reproduce its remaining optional hold exactly.

## Controls and quick settings

Companion has separate **Keyboard**, **Controller** and **Hotkeys** pages.
Keyboard controls use native SDL key names; system shortcuts also accept modifier
combinations such as `Ctrl+p`. The Controller page assigns buttons to the twelve
SNES inputs. L2 and R2 are reserved for the map and quick settings. Apply and
restart after changing bindings. Existing controller selection and player-two
bindings are retained.

| Default shortcut | Action |
| --- | --- |
| M / controller L2 | Open or close the explored map. |
| Ctrl+comma / controller R2 | Open or close quick settings. |
| Ctrl+J | Open or close achievements. |
| Arrow keys / D-pad | Navigate a map or settings panel. |
| Left/Right in quick settings | Adjust the selected volume by 5%. |
| Enter / controller A or Y on Achievements | Open the achievement browser from quick settings. |
| Escape / controller B | Close a panel. |
| Keypad + / Keypad - | Raise or lower master volume during gameplay. |
| Alt+Enter | Toggle fullscreen. |
| F | Toggle the performance display. |
| F11 | Open the save-state browser. |

Quick settings pause guest execution, so changing a volume does not advance the
battle or escort. Master volume affects all audio. X68000 music volume trims only
the optional replacement soundtrack; its sound effects retain the master level.
These settings persist without returning to Companion. Changing the soundtrack
itself still requires Companion and a restart.

## Achievements

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
`Achievements/lagoon/native-qol/unseeded/progress.json` beneath it. In the preview,
that is inside `local/saves`; in the normal source checkout, inside `build/saves`.
Loading an older game save or state does not erase earned achievements. Extracting
a separate preview gives it independent local progress unless you transfer your
save folder yourself.

Loading a save establishes a baseline and does not retroactively award its
existing accomplishments or replay unlock sounds. Continue playing to earn
future qualifying milestones. Scripted/assisted diagnostic sessions disable
tracking. A corrupt/unreadable progress file or another running copy leaves
gameplay available and shows a tracking status; the game does not reset the file
or change guest HP, items, rewards, flags or clocks to award achievements.

## Audio

On Companion's **Audio** page, choose **Original SNES** or **X68000**, Apply and
restart the game. The preview includes the converted X68000 pack; no folder
selection or conversion is needed. Missing/unmapped cues retain SNES music and
sound effects remain on the original driver. Save-state loads restart the
replacement track. Run-ahead uses SNES audio. The supported X68000 cues use
attenuation based on measured perceived loudness to reduce jumps between areas,
including Voloh; PCM data and loop points are unchanged. Master and X68000 music
levels can be changed in the in-game quick settings panel. See [soundtrack details and MP3
listening downloads](SOUNDTRACK.md).

## Reporting a bug

The owner and friends perform the uninterrupted campaign playtest. Include:

- Location, nearest save/slot and exact steps/buttons to reproduce.
- Expected behavior and what happened; a short image/video for visual faults.
- Executable version/hash, aspect, Enhanced setting, QoL choices and controller.
- Relevant crash/log text, and whether the problem repeats after reloading.

For slowdown, mention the room and enemies present, and whether movement,
animation, audio or only presentation slows down. A steady presentation counter
alone does not prove equal gameplay cadence everywhere.
