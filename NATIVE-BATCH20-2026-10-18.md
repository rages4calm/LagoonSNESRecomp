# Batch20: native death animation and Gold Cave scene rendering

Selected and installed without rebuilding the qualified executable. The program
has **377 AOT /119 analyzed LLE /258 roots**; these are not completion percentages.
The interpreter safety net, opening0080E6 floor and exact mode/stack guards remain.

Executable SHA256:
`15cca706f309e8cc29b3219ac132e6ca31f8525e609687654aea1174ab1de6c8`.
Manifest SHA256:
`508aa194317244bf39009da15b398fe8d684e46812f7032402231e9ef4a3c5ce`.
All seven hashed owner ROM/settings/save inputs remain unchanged. A fresh cold
installed check matches all24 component hashes from three candidate checkpoints.

## Ordinary death-animation prefix

Exact01:B413 M1X0 now runs18 original instructions natively through the existing
AB31 table dispatcher. It updates the original timer/frame RAM and uses the real
RTS frame, or tails to the original interpreted B7D4 reward or B43E special-death
path. Other widths remain interpreted. No new return-width assertion or SDK ABI
was introduced. Player death, audio waits and whole boss orchestration are not
declared native by this change.

The oracle extracts the actual generated body and compares original ROM opcodes
over11,520 cases, including decimal/NZVC flags, timers, animation thresholds and
kind24/25 boundary. Full128KiB RAM, CPU/flags, dead stack, CPU/master clocks and
last-instruction coprocessor clock match:5,760 special exits,192 reward tails,
5,568 real RTS returns. Scheduling/dispatch are stubbed in this oracle.

A separate live fixture explicitly stages one ordinary enemy's death-animation
state and position, then allows the original dispatcher/timer/rewards to run.
All18 source PCs execute under the original interpreter control (240 instructions)
and disappear from its opcode watcher in the candidate (zero). Ordered pictures,
three complete eight-component checkpoints and clocks remain exact. This staged
fixture is not represented as a natural sword kill or complete campaign coverage.

Fresh strict comparisons also pass boss40 and boss48 kills, ordinary contact,
consumable revival and the existing room03 door route. No old mismatch reports
were masked or relabeled. Menu acceleration and room07 fade work are already
complete for their audited routes and were not reimplemented.

## Live custom scene renderer

Stable Gold Cave03 now uses its exact resource tuple, definitions069298, world
1792x1632 and original map pages. Atland remains supported. Original PPU layout,
palette, timeline and sealed object checks remain; no new virtual actor producer
or fading packet authority is admitted. Independent drawing must match every
reference pixel before replacing the frame, otherwise normal rendering is used.

Same-executable off/auto qualification passes646 ordered presentations and two
seven-component checkpoints:626 replacement fields,311 independent renders,
23,825,088 independently drawn pixels, zero mismatches. Twenty unsupported fields
use fallback. The helper adds a distinct cave-definition positive and20 resource,
modal, fade, bounds, room-alias and PPU refusal cases.

Reference rendering still runs for verification. This is a renderer coverage
expansion, not a GPU speedup, new DirectX backend, retexture or zoom-out.

A bounded hidden-window1920x1080 OpenGL/EPX check with the real audio device
measures60.135 presentation updates/sec, p9517.646ms and maximum21.218ms over550
intervals. No interval exceeds25ms;274 gameplay tick gaps are two fields with
zero slower gaps. Eleven audio high-water waits remain. This is not fullscreen
scanout proof or a campaign-wide locked60 guarantee.

## Current upstream tooling

The cumulative framework patch integrates the latest checked upstream through
b894555 additively, excluding the unrelated recomp-net dependency pointer update.
Local SDK HEAD/index and all local timing, provenance, save-root and input fixes
are preserved. Upstream computed near/far synthetic-return handling is included.
Forty-four focused tests and20 subtests pass, with no skips.

Generated review finds34 old bodies byte-identical and342 bodies differing only
by462 upstream return-guard replacements, plus AB31's one native-call redirection.
B413 is the sole additional body. Roots/exits/mode sets/HLE guards are unchanged;
all28 pre-existing exact-mode LLE deferrals remain. The public base pin plus the
updated cumulative patch reproduce this combined toolchain; do not reset the
working SDK to upstream HEAD.

Next: broader native death/boss continuations and additional custom-renderer
scenes, followed by specific remaining widescreen/performance reports. The
optional door/exit map and modest zoom-out remain deferred considerations after
native work and the custom renderer. Owner/friends perform the natural campaign.
