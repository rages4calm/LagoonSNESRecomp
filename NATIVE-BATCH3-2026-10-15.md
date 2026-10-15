# Lagoon combat, progression and optional audio batch

This batch continues the accepted `e62492c` native control. It retains the
65816 interpreter, including the opening dispatcher at `00:80E6`. It does not
qualify a full playthrough or resolve existing whole-port differences against
the pure interpreter.

## Native routines

Thirteen additional explicit M1/X0 roots use instruction-boundary timing:

| Address | Responsibility |
| --- | --- |
| 01:BA52 | Receive-hit eligibility |
| 01:B7AE | Hit rectangle containment |
| 01:B6BD | Sword rectangle construction |
| 01:B919 | Mark defeated actor |
| 01:947A | Gold addition and saturation |
| 01:94B2 | Experience award and level progression |
| 01:94F7 | Strength recalculation |
| 01:952F | Defense recalculation |
| 01:9170 | Regeneration timers |
| 01:91B4 | Queue HP regeneration |
| 01:91BB | Queue MP regeneration |
| 01:91C2 | Queue special regeneration |
| 01:91D5 | Apply regeneration/status updates |

Together with the previous eight promotions, generation produces 164 AOT
variants and 18 declared LLE variants. Counts describe generated coverage,
not the fraction of gameplay implemented natively.

`01:957F` remains interpreted because no qualified route invoked it. Higher
level sword-hit and defeat-reward wrappers `01:B5EE` and `01:B7D4` also remain
interpreted: their indirect continuations are unproven, and adding them as roots
discovered additional native descendants that failed the guest-clock gate.
Their failure is not evidence that the smaller accepted routines are incorrect.

## Bounded runtime evidence

The stock opening/movement/menu route retains 18 matching checkpoints. On that
route the native additions reduce interpreted instructions from 63,863,965 to
63,841,236. The small stock-route reduction is expected: it does not exercise
the newly qualified combat/progression branches extensively.

Six frozen fixtures cover a real spawned GoldCave bat taking six sword hits,
its actual death/reward, level 2 growth, gold saturation, both temporary stat
bonuses, and equipment changes through the real menu. Detailed fixture seeds,
route hashes and measured results are in
[the combat route record](tools/COMBAT-ROUTE-BATCH3.md).

The bat loses 12 HP through ordinary B-button attacks and awards 2 gold and 1
EXP. Starting EXP at 9 lets that actual award trigger level 2: strength 23,
defense 10. The gold fixture starts at 65534 and saturates at 65535. The two
bonus fixtures exercise strength 53 or defense 40 without changing MP directly.
The equipment fixture waits through the genuine loading/fade sequence and
changes strength 10 to 20 after equipping the owned short sword.

These are explicitly staged fixtures: they bypass the opening cave guard and
position the player, and some seed prior EXP, gold, ownership or a bonus flag.
They do not establish natural story-gate, spell-casting or campaign coverage.
The combat fixture leaves enemy HP, rewards and player HP untouched.

The native candidates match the accepted control at 85 checkpoints per combat,
level, gold and bonus route, and seven equipment checkpoints. Comparisons check
framebuffer, WRAM, registers, VRAM, OAM, CGRAM and master/beam clocks.
One older combat capture disagreed with a fresh repeat of the same control;
the cause is unresolved. Qualification uses the frozen fresh control, and the
harness now freezes route bytes before launching to prevent script-edit races.

The final integrated executable is SHA256
`c53cb2d4057cba0be13b198036bb5dbd898d4174b8b987ca5268eb0bc322c15f`.
The final relink repeats the 18-tag stock and eight-tag natural wide-town
comparisons without a component or clock difference; all 7,219 stock and 6,579
wide-town presentations match exactly. The preceding authoritative-map candidate
also repeats seven equipment checkpoints and all 3,687 equipment presentations.
The prior camera integration repeats the 85-tag bonus-2 comparison. Earlier
bound candidates retain the other branch proofs.

## Widescreen and camera

The new renderer reconstructs independently OAM-matched normal NPC and enemy
cells from the game's completed render queue, sampled immediately before OAM
DMA. It clips host additions out of the native 256-pixel center. Stale parked
sprites and unproven actor states are refused. See
[the renderer evidence and limits](docs/WIDESCREEN.md).

Camera centering now uses the original camera and scenery streamer. The old
end-of-frame WRAM writes bypassed incremental tile uploads and also incorrectly
wrapped world coordinates at 1024 pixels. Position-fixture screenshots initially
hid that defect because their normal-view controls contained the same blank
tiles. A natural walking route reproduced it independently.

The replacement changes five guarded immediate operands: four dead-zone
thresholds and the initial horizontal origin before the complete map load.
The original routines retain room limits, full world coordinates, parallax and
per-eight-pixel strip uploads. Those instruction paths remain interpreter-only.
Runtime entry checks and a conservative generated-body overlap check reject
AOT conflicts; the latter runs after regeneration and before each build.
Source regressions cover all-off identity, exact operands, unchanged vertical
initialization, atomic guard refusal and compiled-entry refusal.

A separate margin-only bug let captured blank circular-VRAM slots override
correct retained map descriptors. Lagoon now uses the framework's existing
authoritative `WsShadowForceTile` API in its supported ordinary-room map loop.
Other ports keep their existing policy; the native center still fetches actual
VRAM. A regression using real shadow storage and the actual PPU reproduces the
blank tile, proves the default policy unchanged, repairs the margin pixel and
preserves the native pixel. Remaining unmapped room void is not filled with
invented scenery.

## Optional X68000 music

The user-supplied VGZ ZIP was rendered privately to 40 loop-aware MSU PCM files.
No MP3 download or YouTube rip is required. The converter uses an independent
libvgm executable; no libvgm code is linked into the game. Copyrighted audio
and generated packs stay outside this repository.

Only three scene mappings are qualified for initial replacement: opening,
Atland and GoldCave. Other commands retain SNES music. Replacement is optional,
with SNES audio as the default. Missing or malformed tracks fall back before
stock music is suppressed, and recognized sound effects remain on the SPC.
The live guest JSL stack and ROM call instruction authorize commands; the stale
interpreter PC is diagnostic only.

Snapshots append a checked, pointer-free cue record. Loading restarts a mapped
track from its beginning; loading without the option/pack restores the original
stock commands. Legacy states stop stale host PCM. Run-ahead and netplay use
SNES audio. Exact host-audio position restoration is not implemented. See
[soundtrack setup, evidence and limits](docs/X68000-SOUNDTRACK.md).

## Reproduction and protection

Private backups/evidence are under the workspace's
`work-native-20261015-114641/`; `active-native-work.txt` identifies it.
The baseline receipt protects 139 saves/states. Every replay uses isolated
catalogs, configuration and save directories. No experiment enables options
in the owner's mod state.

The public framework base remains reachable upstream commit
`a6be9756285f26ccc74539c811d4d23d57d8c33d`. The compatibility patch now also
includes the read-only DMA observer and Windows-safe multi-package catalog
staging. Applying it to a clean private upstream worktree passes precise-root,
WRAM-filter and native PPU boundary regressions. Decoder, real PPU sprite and
DMA observer tests pass separately. Public source excludes ROMs, generated C,
audio, saves, states and captures.
