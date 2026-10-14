# Campaign route: what the guide says, what the port has measured

Five of the owner's QoL features (sword reach, +8 recovery frames, EXP +50%, follower
speed, magic power) are blocked on reaching ordinary combat, and ordinary combat is
blocked on leaving the starting town, and leaving the starting town is **not** a matter of
walking to a map edge. This file records what is known about that route, and keeps the
three kinds of knowledge apart, because they fail differently:

- **GUIDE** — from an external walkthrough, reliable as intent, not verified here.
- **MEASURED** — read out of this port's own runtime dumps or the ROM bytes.
- **INFERRED** — arithmetic or reasoning on top of the two, with the arithmetic shown.

## GUIDE: the chain that opens the north exit

Source: <https://mikesrpgcenter.com/lagoon/walkthrough.html> (fetched 2026-10-14), with
the town layout from <https://mikesrpgcenter.com/lagoon/maps/atland.html>.

Atland, in order:

1. Go to the **mayor's house** in the southwest corner; learn the mayor has gone to the
   church.
2. Enter the **church** near the centre of town and speak to the mayor. A man rushes in
   announcing a problem at the cave.
3. Speak to the **hurt man**: demons in the cave, Giles still inside. The mayor asks for
   news to be brought to the cleric.
4. Re-enter the town and speak to the **cleric** in the church, then speak to the **mayor
   in his house**: he asks you to enter the cave and gives **300 gold**.
5. Buy gear in the **armour shop** to the north (Short Sword 150, Bandit Armor 100, Iron
   Shield 150).
6. Leave through the **north exit** into **Gold Cave** — the first ordinary enemies, and
   therefore the first place the combat-dependent QoL features can be measured at all.

Later milestones the same page gives, kept here so route work has a target: Giles rescued
and escorted back, Samson's Key from the woman in the southeast ("Place of Faith"), Samson
behind the locked door, then Elf Field, Voloh, and Thor joining as a **follower** (the
follower-speed feature needs exactly that), Philips Castle, Dwarf Desert, and on to Lagoon
Castle.

The guide's ordering matters for the QoL work more than for play: the Short Sword is
bought *before* the first fight, so the sword-reach numbers the owner gave
(`28,17,28,27` → `38,38,38,38`) are most likely a weapon table entry rather than a
constant — a reason to look for a per-weapon table instead of four immediates.

## MEASURED: the entity record layout, from a town dump

`build/dumps-qreg-aot/town.wram.bin`, window `$7E:0500..$0600`, read with
`tools/wram_peek.py`. Records are **0x40 bytes apart** — the three after the player's all
begin with the same `C1 00` state pair at `$0540`, `$0580`, `$05C0`:

| offset in record | field | how it is known |
|---|---|---|
| `+00` | state/flags (`0xD8` player, `0xC1` the three others) | dump; `AND #$E7` at `$01:A4DE` |
| `+02` | world X, 16-bit px | `$01:A5AA` (`LDA $0502,x; SEC; SBC $0480`) |
| `+04` | world Y, 16-bit px | same routine |
| `+0C` | screen X = world X − `$0480` | same routine; dump agrees |
| `+0E` | screen Y = world Y − `$0482` | same |
| `+1A` | speed-tier selector | read at `$02:80EF`, `<<4` as a table stride |
| `+20`…`+28` | **party stats live inside entity 0's record** | `$0520=HP $0522=MP $0524=STR $0526=DEF $0528=GOLD`, matching the menu's `LV 1 HP 10/10 MP 3/3 STR 10 DEF 5 GOLD 100` |

That last row settles an open question that had been blocking "start at 18 HP / 11
defence": the party block is not a separate structure, it is entity 0 at `+0x20`, so the
same init that spawns Nasir writes it.

Contents of the town dump at the frame the route reaches Atland:

| record | state | world X,Y | on screen? |
|---|---|---|---|
| `$0500` Nasir | `D8` | 640, 112 | yes |
| `$0540` | `C1` | 320, 428 | left of and below the view |
| `$0580` | `C1` | 1136, 1116 | off-screen |
| `$05C0` | `C1` | 240, 1084 | off-screen |

## INFERRED: where the guide's buildings are in world coordinates

The published Atland map (`maps/atland.png`) is 640×768 for a world the engine clamps to
`$0492 = 1520` by `$0494 = 1522`, so the scale is 2.375× horizontally and 1.981×
vertically. Multiplying the pixel positions of the numbered doors:

| guide label | image px | world px (×2.375, ×1.981) | cross-check |
|---|---|---|---|
| 1 north exit to Gold Cave | ~325, 25 | ~772, 50 | gap in the top wall, right of the water channel |
| 2 armour shop | ~118, 205 | ~280, 406 | entity `$0540` at 320,428 stands beside this door |
| 4 church door | ~287, 340 | ~681, 673 | the central building |
| 6 mayor's house door | ~118, 545 | ~280, 1080 | entity `$05C0` at 240,1084 stands beside this door |
| 5 weapon shop | ~540, 355 | ~1282, 703 | |
| 7 Place of Faith | ~580, 520 | ~1378, 1030 | |

Two of them land within a few tens of pixels of an entity the dump shows standing there,
which is the cross-check that the scale is right. These are still **estimates read off an
image**: the route is built by walking to them and confirming the position in a dump, not
by trusting the multiplication.

## MEASURED: leaving the town, and why my first conclusion was wrong

`tools/script_exithunt.txt` (fast walk on) walked from spawn in all four directions and
every dump showed `$0492/$0494` fixed at **1520 / 1522**, which I read as "walking does not
leave the town". **That reading was wrong**, and the correction is the single most useful
thing on this page:

`$0492/$0494` are not the bounds of the world, they are the bounds of the **current band** --
the region the engine has loaded. Atland's town band is 1520x1522; the area north of the wall
is **496x498**; the area beyond that is **1776x1618**. Crossing a band edge changes those two
words, and that is the transition detector.

The crossing itself is a scripted sequence, which is what made it look like a wall:

- input is ignored for roughly **200 frames** (measured: locked from f1789 to f1985 in
  `dumps-barrier`; A does not dismiss it);
- position reads as frozen and `$0500` sits on one value the whole time;
- then control returns by itself and walking is normal again.

A route therefore has to `wait 260` after any band crossing and cannot treat a frozen
position as a blocked tile. Two earlier notes of mine -- "pinned between rooftops" and
"walking alone does not leave the town" -- were both artifacts of not waiting.

**The way out is up the corridor at x=352.** Walking north from spawn leads into the 496x498
band (the northern wall of Atland, with a waterfall and a painted figure at 352,44), and its
wall stops him at y=131 across most of its width -- trees, as the published map draws them.
At x=352 exactly, walking up fires the crossing into the 1776x1618 area, which is **Gold
Cave**: stone-walled rooms and ordinary enemies. `tools/script_combat3.txt` plus
`LAGOON_DEBUG_AT="352,120,2100;1456,1440,2670"` reaches a fight in about 45 seconds.

Two consequences for the QoL backlog, both now measurable instead of blocked: combat
features (sword reach, recovery frames, EXP, magic power) have a reachable target, and the
route to it is short enough to iterate on.

## MEASURED: what a fight looks like in WRAM

From `dumps-combat2` / `dumps-combat5` (Gold Cave, one blue flying enemy):

| address | meaning | how it is known |
|---|---|---|
| `$0520` | **Nasir's current HP** | 10 at town start (matches the menu's `HP 10/10`); 10 to 6 to 2 to 0 across three enemy touches, ending in death; once rose 5 to 7 |
| `$0501` bit `$10` | a status flag, set with `$0510/$0511` zeroed | `$02:81AA`: if `$053F,x` is negative and the bit is clear, set it and clear that timer |
| `$0501` bit `$20` | a second status flag, set with `$0516=$80, $0517=$05` | `$02:81CB`: needs `$053E,x` negative **and** `($0501 & $30) == 0`, i.e. nothing else of this kind is already active |
| entity `+0x20..+0x2A` | an enemy's stat block, e.g. `0C 00 0F 00 0A 00 02 00` | constant across the whole fight, so these are the stats and not the current HP |

The second row matters directly: the owner's *"eight extra frames of protection after an
ordinary hit"* is a timer of exactly this shape, and `$02:81CB` is the engine deciding to
start one. The first row is the number the "start with 18 HP" feature changes.

What is still **not** proved, and is deliberately not claimed anywhere: which byte is the
enemy's current HP (nothing in its record decreased monotonically), and whether the
`+0x15/+0x17` pair that read `1E FE` during a swing is a hitbox offset or the low half of the
`$0516/$0517` timer above -- `$02:81D5` writes `$0517` as a timer, so the hitbox reading is
now in doubt and the sword-reach address is still open.

Entity records in the cave run: 31 live records, stride `0x40` again, with the enemies being
the records that carry a velocity and an actor state (`$0BC0`, state `C8`/`D8`, `vx=-1`,
closing on Nasir) while state `C0` records sit on 32-pixel grid positions and are map
furniture. `tools/entities.py` prints all of this per dump.

## MEASURED: the intro is press-gated, not a fixed movie

The patient route waited 240 frames between presses and reached Atland at f6134. Repeating
the same presses with `wait 20` reaches it at about **f1500**, at the identical spawn
(640,112) with the same party block. Route experiments are therefore ~4x cheaper, which is
what made the cave reachable in an afternoon. The gated regression route keeps its patient
head -- the accepted dumps were made with it, and changing them would invalidate the
comparisons already on file.

## HOW: the development position hook

`debug_position` (in `lagoon.quality`, default off) moves Nasir to a chosen world position at
a chosen frame: `LAGOON_DEBUG_AT="x,y[,frame];x,y[,frame]"`, frames counted from plugin
activation. It exists because route work is a positioning problem -- one tile off from a door
or an NPC changes the outcome entirely, and walking him there lands on whatever the collision
grid allows rather than the square being asked about. It is a harness hook, not a player
feature: with the variable unset nothing is written at all, even when the feature is on.

Measured caveats worth knowing before using it:

- Landing **on** a warp tile starts the warp and leaves it hanging (`$0500` = `C0`, the value
  the engine uses while entering a room). Write him *below* the edge and walk up onto it.
- More than one move per run is necessary in practice, because a route is a sequence: band
  first, then the far side of the gate. The hook takes a list for that reason.
- It does not change the room. Position is within the band the engine has loaded, so the
  band crossings still have to be walked.

## Route scripts

| script | what it does | status |
|---|---|---|
| `tools/script_gameplay_regression.txt` | patient intro → Atland → move → Start menu → close | the gated route, AOT ≡ floor, do not re-time its head |
| `tools/script_walkhunt.txt` | town, eight measured walk steps | proves walk speed +15 vs +9 |
| `tools/script_exithunt.txt` | four long directional walks | its conclusion was wrong (no wait after a band crossing); kept as the trail |
| `tools/script_intro_fast2.txt` | the same presses with `wait 20` | proves Atland at f1500, so route heads are 4x cheaper |
| `tools/script_barrier.txt` | walks into the crossing and holds A through it | dates the scripted lock at ~200 frames |
| `tools/script_gapetop.txt`, `script_edgesweep.txt` | push the far wall and sweep it | locates the crossing at x=352 |
| `tools/script_combat3.txt` | fast head → gate → Gold Cave → swing at the enemy | **the combat route**, used with `LAGOON_DEBUG_AT="352,120,2100;1456,1440,2670"` |
| `tools/script_talk.txt` | first attempt to reach an NPC | broken: a PowerShell function ate the presses (`+=` inside a function binds locally) |

## Tools built for this

| tool | what it answers |
|---|---|
| `tools/entities.py` | who is in a scene dump: state, world and screen position, velocity, per record |
| `tools/wram_diff.py` | what a button press changed; now accepts `dump <tag>` names, not just framedump frame numbers |
| `tools/ram_track.py` | one address across a run's tags, with deltas -- the position/HP/bounds trace |
| `tools/tag_sheet.py` | the pictures, so a claim about a doorway can be looked at instead of argued about |
| `tools/wram_peek.py` | a hex window of any dump, for reading a record directly |


## MEASURED: the first fight cannot be won, because Nasir has no sword

`tools/script_combat7.txt` stands Nasir on top of the Gold Cave enemy (teleported onto its
position every 30 frames so he cannot walk out of range) and mashes A through eight swings.
The enemy's block never moved:

    p00  player HP 10   enemy ($0BC2,$0BC4) = (1472,1440)   $0BE0=12 $0BE2=0 $0BE4=15 $0BE6=10
    p02  player HP  5   enemy = (1480,1456)                 12      0      15      10
    p04  player HP  1   enemy = (1488,1444)                 12      0      15      10
    p06  player HP  0   enemy = (1488,1424)                 12      0      15      10
    p08  player HP  0   enemy = (1468,1424)                 12      0      15      10

Nasir took 10 to 5 to 1 to 0 while the enemy's four stat words did not change at all, so the
swings were not grazing -- nothing connected, at point-blank range, eight times.

The published walkthrough explains why: the Short Sword is **bought** in Atland for 150 gold
and Nasir starts with 100 (`$0528 = 64 00`, measured). The event chain in Atland pays 300
gold, which is what makes the shop reachable, and the guide's ordering -- do the chain, buy
the sword, then go north -- is not a suggestion but a requirement of the economy. So
"sword reach" and "enemy EXP" are gated behind *dialogue and a purchase*, not behind finding
an address: the reach number probably belongs to a per-weapon table, and punching is not a
substitute for measuring it.

Consequence for the route: the campaign path for the combat features now runs through the
Atland event chain (mayor's house, church, the hurt man, the cleric, mayor pays 300) and the
shop, which is a dialogue problem rather than a walking problem. The position hook makes the
dialogue squares reachable; what is still needed is reliably *initiating* them.

## Gotcha: a teleport run needs debug_position switched on

The position hook is a feature like any other, and the parity gate flips **every** feature
off, including it. Two combat experiments were wasted reading a silent teleport before this
was noticed: `LAGOON_DEBUG_AT` being set does nothing while `debug_position` is
`enabled = false` in `build/mods/preloaded/state.toml`. Check the state file before any run
that depends on placement.

## Pick-up point for the next session (written when the working context ran out)

State of the gates, all reproducible: `ninja -C build`; `tools/regen.sh --cfg-dir recomp-floor`
and `tools/regen.sh` must be run through `C:\Program Files\Git\bin\bash.exe` (the `bash.exe` on
PATH is the WSL stub, which silently does nothing). Wrap native commands as
`cmd /c "tool ... > log 2>&1"` because `env.ps1` sets `$ErrorActionPreference = Stop`, which
turns a native stderr line into a terminating error.

Shipped and measured: walk speed, camera centring, 18 HP + 11 defence (`lagoon.quality`,
default off, faithful default re-gated after each: 9 tags, 0 mismatches, `clock=0k`).

Five things worth doing, in the order that buys the most:

1. **The Atland event chain, then the shop.** Three features (sword reach, EXP +50%, recovery
   frames) wait behind buying the Short Sword at 150 gold with 100 in pocket. The route is
   `tools/script_combat3.txt` plus `LAGOON_DEBUG_AT` for standing on a dialogue square; what is
   missing is only reliably *initiating* dialogue. Useful fields: `$0528` gold, `$0536` compared
   with `#$25` in `$01:9250` (some special id), `$053E/$053F` status-inflict flags.
2. **AOT cost model.** Lagoon never writes `$2181`, so `g_memsel` stays 0 and every AOT block
   charges 8 cycles per internal cycle while the interpreter charges `internal * 6` plus
   transit -- a systematic +2 per internal cycle. Find that expression in the runtime, mirror
   the interpreter, and re-gate; the gate is what proves the fix rather than a benchmark.
3. **`$80E6` target set.** 12 targets observed on the regression route; `tools/dispatch_hints.py`
   emits the `indirect_dispatch` lines the evidence supports. Until the set is proved complete,
   `force_lle 0080e6` stays.
4. **CompanionLauncher adapter** at `..\CompanionLauncher`, untouched so far.
5. **`-AudioDriver directsound` pacing**, the only check that needs a real window and sound.

Known stale-binary trap: a build that fails to compile leaves the previous executable in place,
and runs then look green. Check the ninja exit code before believing any measurement.

## Dialogue probe: what A does NOT do (two runs, dumps on disk)

The northern band (496x498) holds three immobile records: `$0540` at (352,44) -- the painted
figure at the gate -- and `$0640`/`$0680` at (368,144)/(336,144), which render as two standing
figures in blue and grey. Their state is `C1` in an early pass and `C9` once Nasir is close, so
they do respond to proximity; they are not scenery.

`tools/script_dialog2.txt` puts Nasir at (336,154), the same column as `$0680` at (336,144) --
ten pixels directly below, facing up -- and presses A four times with 40 frames between.
Nothing happened that any instrument can see: position frozen at (336,154), HP still 10, player
state still `C8`, the NPC state still `C9`, and `build/sheet-dialog2.png` shows no text window.
The earlier `tools/script_dialog1.txt` walked him north *past* both figures with A held between
steps and also produced no window.

So the open question is narrowed to two candidates, both cheap to test and neither of them an
address: the interact key is not A (X, Y or Start are untested), or these two figures are
event-gated and answer only after something in Atland has happened. The published walkthrough
has Atland as the place where the story starts, so the second candidate is at least as likely
as the first -- these may simply be the travellers you cannot recruit yet.

Harness note that cost two dead runs today: `cmd /c` cannot execute a `.ps1`, so a probe
launched that way produces no dumps at all and no obvious error. `headless.ps1` must be called
directly; only the `bash tools/regen.sh` step needs `cmd /c`.

## Dialogue probe 3: it is not the button

Same tile as probe 2 (Nasir at (336,171), ten pixels below the figure at (336,144), facing it),
one button per 45-frame interval, measured instead of looked at. Counting bytes that differ from
the pre-press dump:

| press | vram | cgram | framebuffer | position |
|---|---|---|---|---|
| X | 0 | 5 | 3 165 | unchanged |
| Y | 338 | 0 | 1 550 | unchanged |
| B | 314 | 5 | 4 657 | unchanged |
| **Start** | **782** | **350** | **163 929 of 172 032** | unchanged |
| up (menu open) | 782 | 350 | 163 883 | unchanged |

X does literally nothing. Y and B each redraw a few hundred VRAM bytes, which is the shape of
the magic/equipment quick-windows rather than speech. Start replaces almost the entire frame --
that is the field menu, and the next line shows the cursor moving inside it while the position
stays frozen, so this is a usable checkpoint for "inventory/menu opened from the field".

No button produced a text window with these two figures, so the remaining explanation is the
one the walkthrough supports: they are event-gated. Atland's chain -- mayor's house, church, the
hurt man, the cleric, mayor pays 300 gold -- is the campaign entry point, and buying the Short
Sword behind it is what unblocks sword reach, EXP and recovery frames.

## AOT cost model: the mismatch is NOT cpu_region_speed

`snesrecomp/runner/src/cpu_state.c:354 cpu_region_speed()` prices **one bus transfer** by
region, and its comment states the property that matters: the LLE bridge and the AOT paced
accessors call this same function, so both tiers price transfers identically. That is why the
gate passes today, and it rules out the theory that the AOT blocks are wrong about region
speed. The remaining difference is the price of *internal* cycles (no transfer): the
interpreter charges `bus_master + internal_cycles * 6` while a translated block charges
`instructions * (g_memsel && (cpu->PB & 0x80) ? 6 : 8)`, and Lagoon never writes `$2181`, so
`g_memsel` stays 0 and the AOT side charges 8 where the interpreter charges 6.

Measured on the gated route this is invisible, because that route barely executes any promoted
code (10 AOT declarations). So the cost-model item and the promotion item are the same item:
changing the internal rate only shows up once gameplay routines are promoted, and any change
must be re-gated on a route that does execute them -- otherwise a green gate proves nothing
about a code path that never ran.
