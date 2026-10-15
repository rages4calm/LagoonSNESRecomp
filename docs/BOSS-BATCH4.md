# Batch 4 first-boss fixtures

These are targeted development fixtures, not a natural campaign completion.
They retain the interpreter safety net. No fixture writes player/enemy HP,
damage, rewards, death flags or victory. Each deliberate position or equipment
selector write is listed inline and in `tools/boss_batch4_report.py` output.

## Real entry and initialization

The real door resolver `$01:C87B` reads the room-indexed pointer table
`$05:8059`; room 3 (Gold Cave) points to `$05:8158`. Its fourth 11-byte door
record has source cell `(40,1)`, destination position `(128,236)`, room `$40`,
direction 3 and script byte zero. The fixture places Nasir at `(1296,90)` in
the already initialized Gold Cave and presses Up through that actual door.
The game's `$01:C8B5` transition and ordinary room initializer create the boss.
The earlier room-2 guard bypass is also an explicit position-only setup.

Room `$40` initializes slot `$0540` as type 3, mode 0, with 96 HP, 24 strength,
65 defence, 100 gold and 64 EXP reward fields. A type-5/mode-9 entity also
appears at `$0580`. These are observed initial values, not awarded rewards.
`$01:8CE8` obtains type-3 templates from `$05:9442`; mode zero points to the
record at `$05:9462`. Boss defeat uses the separate type-3 death dispatch and
the room-minus-`$40` progression table. Those paths remain interpreted unless
separately qualified.

## Observed gameplay

`script_boss_batch4_entry.txt` enters the boss room unarmed and observes actual
boss movement, a hit reducing Nasir from 10 HP to 2, and knockback. The boss
remains at 96 HP. `script_boss_batch4_sword.txt` explicitly selects Short Sword
index 1, as in the earlier combat fixture. It does not recompute strength:
the observed value remains 10. Grounded sword attempts did not damage the boss.
`script_boss_batch4_jump.txt` adds one explicit right-flank position setup and
real A-jump/B-sword input. The bounded attempt ended in actual player death and
the game-over transition. The boss remained at 96 HP while room `$40` was live;
its slot becoming zero after the room changed is not boss damage or victory.
No boss reward, defeat or post-boss progression has been qualified by these
fixtures. Intermediate failed flank experiments remain private receipts.

## Qualified damage fixture

`script_boss_batch4_damage.txt` requires **`--features sword_reach`**, the existing
opt-in 38-pixel reach feature. It waits for the real quake to finish and the
boss's Y anchor to equal 158, faces left **before** jumping (airborne facing is
locked), then places the jumping player at `(164,158)` and sends actual B input.
Short Sword selector 1 is explicit; strength remains 10. No player-level/stat
or enemy-state staging was needed.

This baseline produced a real sword collision and **boss HP 96 → 94** at frame
3587; the boss remained at 94 afterwards. Nasir also took a real hit, 10 → 2 HP.
The path observed `$01:B62A -> $01:B9A6` once and
`$01:BA1B -> $01:BA6B -> $01:9226` twice, all M1/X0. It does not prove stock
28-pixel flank positioning, boss death, rewards or victory. The frozen route
SHA-256 is `fb747289749813f0d59099557d6575f2af9a59ec32f6f5148c3d8d6866a06f27`.

The first boss's quake code explains an important input condition: at quake
start, a grounded Nasir can be locked by incrementing `$0500`; the sword
updater rejects that low bit. A Nasir already jumping (`$0501 & $20`) skips
that lock. Jump probes use real A then B input, not writes to quake, attack,
invulnerability or enemy state.

## Bounded native candidates

The unarmed entry baseline observed these entries in M1/X0:

| Entry | Evidence and boundary |
|---|---|
| `$01:ACE7` | First-boss phase/animation driver; 337 dispatches from `$01:ACC4`. Code ends at `$01:AD68` RTS. Calls contact-damage wrapper `$01:B730`; not a leaf. |
| `$01:C3E8` | 26-byte coordinate-offset leaf; 355 calls from `$01:C345/$01:C35E`. Writes `$027F/$0281`, returns M1. |
| `$01:C402` | 25-byte room/palette selection leaf; 387 calls including pre-boss scenes. |
| `$01:C486` | 41-byte projected jump-anchor leaf; 1,460 calls including pre-boss scenes. Uses hardware division, requiring exact timing qualification. |
| `$01:DDAD` | 100-byte enrage/quake helper through `$01:DE10`; 336 dispatches from `$01:D8F6`. Sole direct child is the sound-event helper `$0C:F8FE`; no indirect transfer. `$01:DE11` onward is data. |

`$01:B730` builds a contact rectangle `[bossX-32,bossX+8)` by
`[bossY,bossY+32)`, checks the player through `$01:B7AE`, and can enter
`$01:B9F4 -> $01:BA6B -> $01:9226`. The exceptional death/return behavior in
that chain is a reason to preserve its interpreter boundary until qualified.
`$01:C5DA`'s room-$46 branch was not covered by this first-boss fixture and is
not justified as a new native mechanic by the observed early-return calls.

All measured modes describe observed calls, not every possible branch or boss.
Coverage and snapshots live in private run directories; no ROM, images, saves
or extracted data belong in this public source tree.
