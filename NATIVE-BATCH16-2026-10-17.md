# Faster menu closing, native child states and Elf Field Start

Batch 16 keeps the interpreter safety net and all 361 previously selected native
variants. The selected program has **369 AOT variants, 123 declared LLE variants
and 252 analysis roots**. These counts are not a completion percentage.

## Menu closing

Items, Magic and Equipment close in **52 fields**, down from 60 in the audited
room-01 routes. Atland improves from 61 to **53**; outdoor room 02 improves from
60 to **52**. Openings remain 40/38/39 fields. Transitions still include fades
and original upload waits; they are not instant.

Two guarded native helpers reproduce original parent-loop calculations before
and after the tile queue call. The prefix stops at original `018A2F`; the suffix
returns to the next `018A10` iteration or original final RTS `018A5D`.
They preserve actual caller frames, CPU flags and even unused hardware-stack
bytes. Unknown sources, modes, return frames and resource bounds refuse cleanly.
The prefix is transactional on failed reads; the suffix validates all bounds
before writing. Neither helper skips DMA, the queue-full wait or room loading.

Original-opcode comparison covers 1,584 prefix and 4,352 suffix cases, checking
all 128 KiB WRAM, CPU state and stack bytes. Production callbacks pass 1,622
checks, including 27 positives and 525 read-only refusals. Prior decoder,
map and queue oracles remain covered. An actual interpreter trace confirms both
hooks execute: 1,023 direct queue-call-to-next-queue-call transitions replace
parent arithmetic, while all 2,070 original queue calls remain. The suffix has
no separately measured whole-field benefit; the combined improvement is eight.

Five old/new timing routes preserve aligned native-center fade pixels. Those
measurements use the unchanged batch-16 host with the intermediate 368 program;
three clean final-369 comparisons against the same-host 361 control match all
5,367 pictures and 18 complete state/clock checkpoints. Historical slow-menu
clocks intentionally differ and are not presented as strict parity.

## Native actor and boss child-state dispatch

New M1/X0 entries are bank 01 `AB31`, `B131`, `B15B`, `B1E2`, `B2CC`, `B2E8`,
`B385` and `B3E6`. The two dispatchers use exact original ROM tables and balanced
PHY/PLY frames. Original state calculations and existing native children remain.
Other AB31 widths, both unqualified parent-table targets and five unobserved
B131 child states stay interpreted. Broader death/wait orchestration, the old
timing/audio deferrals and the opening interpreter floor remain unchanged.

Simply compiling the seven child entries initially removed no interpreted work:
the interpreted AB31 dispatcher still reached them by JMP. That intermediate
368 result is excluded from native gains. The source-proven native AB31 prefix
connects the original dispatch chain. Only the existing A7B5 router's AB31 arm
changes; the other 360 old generated bodies remain byte-identical.

Four fresh matched runs cover rooms 03 and 14, the Thimale guardians and endgame
combat. All eight new entries actually execute. Exclusive source-PC/mode
accounting removes **328,948 interpreted instruction executions**, including
**303,173 nonreturn executions**, across these fixtures. Of 173 statically
reachable source keys, 123 execute in this corpus. All 1,627 raw coverage records
parse completely with zero loss counters. These are assisted test totals, not
an FPS measurement, natural campaign or full-native claim. The old canonical
79-capture corpus was not recaptured or relabeled for this batch.

## Elf Field widescreen

Elf Field's animation changed six referenced scenery characters. The retained
world witness correctly rejected the new phase, causing Start and part of the
destination fade to fall back. The existing DMA-witness helper now recognizes
that room's exact two-frame descriptor, original source table, transfer length,
generation and complete graphics bytes. Generic modal ownership is unchanged.

The fix restores **35 Start fields and 45 destination-fade fields**. All 35
original centers match. Independent decoding verifies every one of 95,424 status
margin pixels: 95,050 original background pixels and 374 actual OAM sprite
pixels. Partial transfers, wrong phases/resources/tables and timeline resets
refuse. The final 369 build matches its same-host 361 control across 1,406
ordered pictures and eight complete guest-state/clock checkpoints.

Room 07 parallax and other unqualified layouts remain explicit gaps. The live
custom renderer retains its stable Atland scope; this batch adds no graphics API
or retexture and makes no locked-60-FPS claim.

## Next work and local hygiene

Final qualification passes **32 fresh comparisons**, covering 97,059 ordered
presentations and 142 complete state/clock checkpoints: four native-work pairs,
24 combat/death/revival pairs, three menu pairs and one Elf Field pair. Inputs
match and no output masks are used. Most replay routes retain at most four/six
original diagnostic checkpoints to limit disk use; guest actions and cheats
remain explicit. The exact tested binary is installed without rebuilding it.

Continue the remaining menu close setup/waits, source-proven native death/boss
orchestration and unqualified widescreen layouts. The owner and friends will
perform the uninterrupted start-to-finish playtest; development uses focused
encounter and regression checks instead of repeating that campaign.

Private inputs and small evidence summaries stay outside Git. No project,
SDK, executable or owner-save backup trees are created. Successful raw scratch
cleanup was blocked by automatic approval review; the owner-run cleanup command
removes the listed completed test batch. Raw historical audit files do not
remain independently rerunnable after cleanup. ROMs, states, generated C, audio
and private captures are never included in the source push.
