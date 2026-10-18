# Batch21: native defeat rewards and three more renderer areas

The exact qualified build is installed: **378 AOT /119 analyzed LLE /259 roots**.
These counts are not completion percentages. The interpreter safety net remains.

Executable SHA256:
`73334f40e4f995f0837178855a856d8d9cf0546f3955a77b9d4f01cdcbe288a4`.
Manifest SHA256:
`2f65717b526cf052b4fee89bada5d4267dc76dc8332c004fe3704288c748ce04`.
Installation moved the tested outputs without rebuilding or making backups.
Seven initial owner ROM/settings/save hashes and the SDK working/index hashes
remain unchanged. A cold installed reward fixture matches24 component hashes
from three qualified candidate checkpoints.

## Native reward path

Original01:B7D4 M1X0 executes16 instructions natively: marking defeat, reading
the original enemy gold/EXP rewards, calling the existing native gold and EXP
functions, and choosing the original ordinary return or boss-refill boundary.
The level-up children remain their original generated code. Exact per-instruction
bus and transfer timing preserves hardware phase and real paired call frames.

The boundary is BEFORE PHX at B7FE. B7FE boss rendering/refill orchestration,
B819 ordinary RTS, B43E special death and9346 SPC/render waits stay interpreted.
Other widths remain exact LLE variants. No new exit-width assertion, synthetic
guest frame, audio shortcut or interpreter ABI is added.

An oracle extracts the actual generated prefix and five actual generated child
bodies, then compares them with original ROM instructions. All14,400 cases pass
full128KiB RAM, registers/flags, paired/dead stack and CPU/master/coprocessor clocks.
This includes4,800 boss-boundary exits,9,600 ordinary exits and8,568 level-up cases.
Scheduling and dispatch are stubbed at the original boundaries in this oracle.

Fresh matched live comparisons cover all seven boss wins and consumable revival.
A separate staged ordinary-enemy reward fixture traverses the real dispatcher:
all16 prefix operations leave interpreter execution while complete state/clocks
match. Gold100→103, EXP0→64 and level1→3 result from original reward logic.
Enemy EXP64 and the actor's death-state/position/timer are explicit setup; no
player gold/EXP/level/result bytes are written. This is not a natural sword kill.

Together the nine strict pairs pass35,426 ordered presentations and19 complete
eight-component/clock checkpoints. They do not certify a full natural campaign.

Independent generated review confirms376 existing native bodies byte-identical.
The sole changed old body is B413's original B431 tail, now calling native B7D4
with inherited stack/return context. B7D4 is the sole new body/root. Existing
exits, HLE guards,119 analyzed LLE facts and opening0080E6 floor remain intact.
The latest checked upstream remains b894555, integrated in batch20's cumulative
patch; this batch makes no SDK changes or unrelated dependency update.

## Renderer coverage

Stable Gold Cave04/05/06 join Atland and Gold Cave03. Each requires its exact
original resource tuple and dimensions:04=768×256,05=1536×1280,06=512×256.
Full-word room checks, source pointers, finite bounds, palette/PPU/layout and
timeline guards remain. There is no generic room-range permission, new actor
producer, fade authority, new artwork or changed camera geometry.

Three overlapping same-executable off/auto routes enter the real doors using
explicit source-position assistance. Each added room ends with a positive
renderer replacement; unsupported/loading fields retain fallback. All2,741
ordered presentation observations and ten seven-component checkpoints match:
1,359 replacements,454 independent draws,34,780,032 independently drawn pixels
and zero mismatches. These overlapping samples are not counts of unique frames.

ROM-free helper checks add three room positives,69 refusal vectors, three stale
generation refusals and458,304 synthetic finite-extent pixel checks. Those helper
results are separate from real gameplay proof. Full reference drawing still runs
and must match before replacement, so no renderer speedup is claimed.

## Pacing and remaining work

A hidden1920×1080 OpenGL/EPX run with the real audio device traverses03→04→05→06.
Its1,328 measured intervals average60.025 presentation updates/sec, p9518.299ms,
maximum22.873ms and zero intervals over25ms. Eleven audio high-water waits remain.
Only an end dump is used: two longer intervals in the earlier diagnostic run
immediately followed mid-route checkpoint writes. No fullscreen scanout,
native60Hz physics or campaign-wide locked60 claim is made.

Continue native death/revival and boss orchestration at source-proven boundaries,
then other renderer scenes and specific unsupported widescreen routes. The audio
and frame-wait paths remain guarded interpreter work. Previously qualified menu
acceleration, Giles first aid and room07 fades were not reimplemented. The
optional door/exit map and modest zoom-out remain deferred until native work and
the custom renderer are finished. Owner/friends perform the natural campaign.
