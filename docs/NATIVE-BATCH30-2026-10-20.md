# Native services and current actor poses â€” batch30

The exact tested executable is installed. Launch `Start Lagoon.cmd`.
Executable SHA256: `ebc62242e068c530892fada95e15cb0aed3c8773051408ff7a83991613b09cb7`.
Manifest SHA256: `45228f8b9120cdd01101feffbb08cca6362137716a31f6471b6d28db85131e14`.
480 AOT /102 analyzed LLE /353 roots are analysis counts, not completion percentages.
The interpreter and original raster remain active.

## Native scope

| Original entry | Complete source service |
| --- | --- |
| 008211 | Sprite staging, including early/overflow paths and real RTS |
| 008C96 | Rectangular metasprite drawing and nested sprite children |
| 0089EC | Tilemap queue builder, including genuine queue wait |
| 008AA4 | Map-position conversion |
| 008DFE | Resource index calculation |
| 008D82 /008DCE | Resource loader/chunk loader with original polling and shared cleanup |
| 01C136 /01C156 /01C17B /02B2C1 | Four complete callers unlocked by the drawing service |

All eleven entries qualify only at M1X0. Naturally derived normal return facts
preserve widths; no unknown service completion or return frame is fabricated.
D82 and DCE both include their shared tail through8DFD, with their own real saved
X/Y and long return frames. The original8985 wait and unsupported modes stay on
the interpreter. Variable dialogue/input/audio and special display paths remain
outside this group.

Existing8291/8B68/8D0B/8E2C and the full8C24 sprite caller now use precise
instruction/transfer timing. The old palette helper charged112 master clocks
under the unit8-clock bus; the actual three instructions take106. The old full
sprite caller also retained a measured coarse timing discrepancy. Both causes
are reproduced as explicit failed diagnostics before the corrected strict tests.

All469 prior bodies remain.459 are byte-exact; five old callers change only seven
interpreter-call lines to native calls, four helpers gain precise timing, and
8C24 retains its original head/tail logic while delegating its actual8211 tail
with the inherited real owner. All436 old mode facts,23 exit sets and4 HLE guards
remain exact; eight new normal-return facts are derived from complete source.

## Actor scope

A separate source-qualified path reads the actual current64-byte actor record
and guest frame1C for eligible NPCs, nondeath enemies and fixed-frame props.
It requires the full current/captured record, both world/camera/native coordinate
relations, original producers, scene/layout/timeline/generation, finite sprite
source, owned atlas cells, live palette and zero opacity in the native center.
No B131 AI, animation dispatch or future frame is simulated.

Only2,658 finite frame entries across76 selectors qualify. Every admitted generic
frame is compared with the original sprite writer. Kind-specific metadata tests
use bounded representative entries rather than claiming every possible
kind/frame combination. Empty/dynamic/frameFF/death forms refuse. Type3 multipart
and type4 projectile paths retain native OAM.

Four genuine assisted actor recipes cover7,216 ordered CRC/luma observations,
eight checkpoints,5,559 custom replacements and zero reference mismatches.
All72 complete dump component files and clocks match both candidate off/auto
and the original-opcode control. New-path numeric far cells are: kind12=228,
13=214,14=249,23=115. Enemy26 and room21 NPC0/33 have no target-positive far claim
in these routes. Source coverage does not imply every quest state is explored.

## Verification and honest differences

- Strict shared-service oracle:162,816 cases, including71,424 complete returns,
  20,736 child/poll/internal-deadline boundaries and70,656 entry deadlines.
  It compares full RAM/IO/registers/flags/hiddenA/real nested and dead stack,
  CPU/master/coprocessor clocks against the original opcode interpreter.
  Poll acknowledgement is an explicit unit model; real devices are checked below.
- Credits with the corrected palette child:68,544 original-opcode cases.
  Its25 selected bodies remain exact after the last sprite timing correction.
- Independent original palette timing:368,640 cases, actual original JSL/RTL,
  two stack owners, hidden accumulator bits, flags and instruction deadlines.
- Current-actor source tests:43,414 original opcode calls,165,897 exact OAM cells,
  4,208 metadata fixtures,3,655 projection/culling positions and31,143 refusals.
  Host producer tests separately exercise unseen/changed poses, source mutation,
  generation/layout/atlas/record refusal and2,880 independently decoded pixels.

A separate validation build forces exactly these16 services through their real
original opcodes, preserving unrelated native callers. It uses the same host,
renderer and dirty SDK. This is a genuine floor control, not an interpreter-bounce
environment setting that direct native calls could bypass. It is not installed.

The dungeon and menu pair matches2,267 ordered observations and12 complete
state/clock checkpoints. Menu strict status stays false for its absolute diagnostic
report path. Four cumulative APU trace rows differ only in host beam-probe
attribution; event/frame/port/value/sample and order are unchanged. This does not
claim raw trace or PCM equality.

The old timed ending fixture missed Talk after the corrected timing, identically
in native and original-opcode controls. The new fixture waits67 fields for Thor's
actual ready flag, talks normally, observes the real pendant, traverses the real
door and reaches THE END. The matched pair has24,147 exact ordered pictures.
Strict status remains false:57/64 standard component pairs are exact; the
remaining seven contain dead-stack bytes, one paused X register and bounded
master-phase differences. DMA and PPU-write logs match; APU ring phase/sample
positions and one adjacent CPU/SPC read ordering differ. These traces stay
explicit; no PCM or audio-event-order parity is claimed. The early
`red-defeated` dump name is a retained label: Red still has256HP there and reaches
zero later, before the credits. No boss result, inventory or quest completion is
written by the fixture.

The exact selected files were moved into the working install without an
executable backup or rebuild. Installed cold ending plus actor14 match26,028
ordered pictures,10 checkpoints/80 standard component pairs and all120 dump files
against the selected candidate. Ending strict-passes; actor strictfalse is only
its absolute diagnostic report path.110 protected ROM/settings/save inputs are preserved.
The owner/friends perform the uninterrupted campaign and hardware playtest.

## Framework and performance

The cumulative framework patch includes official upstream6af98166f81f33ce4b10ae67240c2d999fcb984f
(October20): manual mod documentation and a legacy patched-ROM compatibility
warning. Its predecessor-to-latest six-file increment changes no CPU/graphics
logic or ABI. The intentionally dirty local SDK/index and public Gitlink are
preserved;112 unrelated cumulative patch sections remain byte-exact.

A hidden1920x1080 Enhanced32:9 room14 check exercises the new far-pose path and
verifies2,122 replacements with zero mismatch. Across1,681 measured intervals it
averages60.01 presentation updates/s, with15 intervals above20ms. During the
source-observed current-pose interval,639 intervals average59.87, p9518.36ms,
maximum35.86ms, with12 above20ms. Dummy audio, diagnostics and one synchronous
readback are explicit. This is not locked60FPS or physical scanout evidence.

## Continuing

Keep source/mode/real-stack/generation guards and the interpreter. Prior damage,
revival, boss, menu/camera and scenery work is already completed; do not redo old
campaign audits. Continue from concrete playtest defects or independently owned
multipart/projectile sources. A discovery map/doors/exits and15-20% zoom-out are
still deferred. Private compact reports and portable input add-ons stay local;
ROMs, audio, generated code and states are excluded from GitHub.
