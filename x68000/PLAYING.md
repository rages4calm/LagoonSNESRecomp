# X68000 Enhanced

See [the controls guide](../docs/PLAYING.md#x68000-enhanced) and
[automatic setup instructions](../docs/BUILDING.md#x68000-local-build-and-setup).

Choose **X68000 Enhanced** in Companion, select your supported four-disk Lagoon
**ZIP or folder**, and press **Set up game**. Compatible firmware and verified
portable Python/build tools download automatically. No manual Python, compiler,
administrator installation or firmware-file hunt is required. The setup window
shows progress; leave it open until setup finishes, then press **Play**.

Setup never downloads Lagoon disks. Your local executable embeds the supplied
game and stays private. Custom IPL/CGROM files remain optional under **Use
custom firmware (advanced)**. **Setup X68000.cmd** also works with your game
ZIP or extracted disks under **Inputs/X68000/**.

## Controls at a glance

These are Xbox-style physical controller labels; other pads may print different
symbols. The full guide explains guest keyboard mode and all Atlas controls.

| Action | Keyboard | Controller |
| --- | --- | --- |
| Move | Arrows | D-pad |
| Sword | Hold S, then move into an enemy | Hold A, then move into an enemy |
| Jump / talk / original dialogue confirm | D | B |
| Skip opening | Enter | Start |
| Items / Equipment | I / E | LB / RB |
| Original Save / Load | Guest F1 | Y |
| Atlas | Ctrl+M | R3 |
| World zoom: 1x / 1.25x / 1.5x | Ctrl+Z | L2 |
| Settings / Quit | Ctrl+comma | R2 |
| Quick save / quick load | F8 / F9 | Use keyboard shortcuts |

Original conversations and Save/Load confirm with **B / D**. Sword cancels a
yes/no question to No. Atlas/Settings confirm with **A / Enter**. **LB/RB**
switches Atlas tabs; **L2/R2** browses visited rooms. In full guest keyboard mode,
use **Ctrl+F8 / Ctrl+F9** for host quick states. **Ctrl+Alt+K** switches keyboard
modes. [Complete controls and saves](../docs/PLAYING.md#x68000-enhanced).

## Included enhancements

- Expanded 16:9 scenery with square pixels, centered camera and live zoom;
  new installations use 1.25x. The zoomed HUD stays at the bottom.
- 38-pixel sword reach and four extra lateral pixels of cleave, using original
  sword art and damage/death behavior. Hold the weapon and move to attack.
- Three-pixel walking steps, eight extra jump-peak updates and sixteen extra
  ordinary-hit protection updates; stronger damaging magic and 2x EXP/gold.
- 18 starting HP and eight base defense; faster menus, dialogue and door waits.
- Potion-free Giles rescue, automatic disk changes and a ready-to-save User Disk.
- A 49-map scenery Atlas, regional journal, item help, eight offline awards,
  controller navigation, live audio/zoom settings and a confirmed Quit action.

Gameplay changes are built in. Giles returns to town through the original event;
there is no escort or SNES-style Thor tablet hunt here. Select the Fur Cloak in
Items to wear it in cold regions. Carrying it alone does not protect Nasir.

The X68000 edition retains its compatibility/runtime support. The SNES edition's
native main-CPU claim does not describe this version. Saves and achievements
are separate; use Companion's ordinary-slot copy tool for older X68000 saves.
Never replace the new bootable User Disk with a whole old disk, since it contains
game files as well as progress. Recreate incompatible quick states.

## Opening and updates — Windows 0.6.1

The title/prologue play at normal speed;
**Start / Enter** skips them. Windowed and Borderless fit enlarge the original
picture proportionally, while Borderless pixel perfect keeps integer scaling.
Opening artwork retains square pixels and natural sidebars, including in Stretch
mode. Gameplay widescreen and world zoom do not squeeze or zoom the opening.
New windows use up to 1280×720; existing window preferences are kept.

**Check for updates** updates the released Companion package without discarding
your local X68000 game, saves or settings. **Update X68000 game** handles runtime
changes separately: it rebuilds locally using cached inputs/tools, asking for
the game ZIP again if needed. A package update alone does not require rebuilding.
Close the game before applying updates. Ordinary saves remain the preferred way
to carry progress between incompatible runtime/quick-state versions.

**Upgrading from 0.6.0:** close both games and Companion, extract the entire
0.6.1 ZIP over the existing release folder and replace packaged files. Keep
**local/** and **local-x68000/**; the ZIP contains neither. Reopen Companion,
choose **Update X68000 game → Update game** once, and select your disk ZIP again
if asked. Future package updates use **Check for updates → Download and update**.
For locally modified application files, use a fresh folder and the
[manual save-transfer instructions](../docs/PLAYING.md#manual-updates).
