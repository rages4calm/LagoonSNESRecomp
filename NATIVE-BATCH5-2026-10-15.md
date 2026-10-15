# Native combat, outdoor widescreen and enhanced pixels - batch 5

This is a bounded continuation with the 65816 interpreter safety net intact.
It does not establish a complete native port or a completed campaign.

## Native gameplay

Five additional exercised roots: damage calculation `01BA6B`, boss
player-contact `01B730`, death-stop velocity `0192F1`, room-hazard dispatcher
`01D8DA` and first-boss quake `01DDAD`. There are 32 explicit promoted gameplay
roots, 175 eligible AOT variants and 18 declared LLE variants, with runtime
fallback available. An eligible count is not a completion percentage.

Four targeted routes preserve all six binary snapshot components and guest
clock at 953 checkpoints, plus every ordered frame/CRC in 15,939 presentations:

| Route | Checkpoints | Presentations |
| --- | ---: | ---: |
| Ordinary contact/damage/death | 397 | 3,664 |
| Consumable revival | 397 | 3,666 |
| First-boss combat | 59 | 4,088 |
| First-boss defeat/rewards | 100 | 4,521 |

The recovery-time option separately retains parity at 397 checkpoints and all
3,664 presentations. Revival consumes a valid selected item after real lethal
damage, then the game heals Nasir. Actual sword contact defeats the boss
(96 to 0 HP); its death animation awards gold (100 to 200) and EXP (0 to 64).
These fixtures explicitly set legal player stats/equipment and position; they
never write HP, death, rewards or victory. They do not establish naturally earned
equipment or post-boss story progression. See [setup, return-width and stack
proofs](tools/DAMAGE-BATCH5.md).

HP/death `019226`, special escape `0192A0`, player-hit/revival `01B9F4` and
boss death-animation `019346` stay interpreted. New native callers accept all
four possible returned M/X widths and dispatch using live CPU flags; they do
not assert a guessed shared-renderer exit mode. The nonlocal 92A0 return stays
inside the interpreted HP wrapper. RNG/hardware-divider helpers remain
interpreted. The complete 75-word room table exposes 28 distinct handlers;
only the audited quake handler is native, with explicit LLE boundaries for
the others. Keep opening dispatcher `0080E6` interpreted.

The five added routine ranges remove 52,491 interpreted instructions across those
four fixtures. Some player branch arrivals and post-LLE cleanup still run through
the interpreter; this is measured workload reduction, not a full-native claim.

## Verification input isolation

The old harness ORed live physical input into scripted buttons. This produced
unplanned B presses and inconsistent menus during concurrent tests. New runs
disable physical controller sources only in their private copied INI;
`--live-input` opts back into the old behavior, and `--input-trace` records actual
inputs. No owner settings change. Isolated reruns restored exact parity, including
revival and the outdoor route; earlier contaminated runs are rejected evidence.

## Graphics

Outdoor room 2 uses exact audited resource identity and per-scanline PPU scroll
anchors. Added margins are clipped to its real 512 by 512 world. Missing,
changed or unready resources use clean native fallback. The original 256-pixel
center remains intact. Existing three-field settlement, modal/loader gates and
conservative NPC/enemy atlas ownership remain in force. Final-candidate proofs
include 35 outdoor checkpoints, menus/doors and 2,505 presentation captures.
See [widescreen implementation and checks](docs/WIDESCREEN.md).

The owner chose optional EPX/Scale2x treatment of original art instead of a full
retexture. The shader uses original colors, no temporal history, and sharp
sampling in the bottom 32 logical HUD rows. Companion selects the preset with
OpenGL and restores prior graphics choices when disabled. It defaults off.
See [enhanced pixels](docs/ENHANCED-PIXELS.md) and actual fullscreen
captures under [screenshots](docs/screenshots/README.md).

## Reproduction and limits

Accepted candidate: `build/lagoon-batch5-top.exe`, SHA256
`13ce550c4ccdf0f5fc214abe9067e89e5dbbce31e70d1d8734922e063b35e439`.
Private evidence is under workspace `work-native-20261015-190426`; the matching
manifest is `top-manifest.json`. All 139 protected original/backup files retain
their hashes. Owner settings and private music remain outside publication.

SNESRecomp upstream HEAD was rechecked as
`a6be9756285f26ccc74539c811d4d23d57d8c33d`. The public gitlink remains there;
`patches/lagoon-framework.patch` reproduces local changes on a fresh upstream
checkout (31 files; checkout line endings normalized). Shared WRAM read/write
filter tests and three precise-timing tests pass there. ROM-free actor/DMA/PPU
and EPX oracle tests pass. OpenGL diagnostic readback is opt-in; visible runs
keep normal pacing.

Remaining work includes HP/death and remaining boss phases, special actor forms,
more room/parallax resources, menu expansion, exact optional hold-timer
serialization, music cue coverage, hardware/controller/save-load and campaign
qualification. Existing native-versus-pure-interpreter differences remain open;
the accepted hybrid is the comparison control. No binary release or live
installation is part of this batch.
