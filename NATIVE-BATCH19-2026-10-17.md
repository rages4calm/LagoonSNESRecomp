# Batch19: menus, cave cadence, first aid and guarded scenery

Batch19 is selected and installed. This remains a beta/WIP with the interpreter
safety net: **376 AOT /119 analyzed LLE /258 roots**, not a completion percentage.

Executable SHA256:
`432b5a5808eebcf8395d14c2399b43e1402a1149965c16a55a1bf875c8bb0629`.
Manifest SHA256:
`f991f7c7bf62e881360c1ee5150af5bff76d90bf3d2c67bdb7e4e853f41bd2a5`.
The exact tested executable was moved into the normal runtime without rebuilding.
Cold installed Items and portable no-potion Giles checks pass. All seven protected
owner ROM/settings/save files and the cumulative dirty SDK remain unchanged.
Source commit: recorded by the final Git publication step.

## Menus and first aid

Five audited checks open Items/Magic/Equipment in20/18/19 fields and close in22.
Twelve original graphics jobs use real charged DMA with exact source, destination,
size and ownership guards. Unknown contexts retain original execution. Shortened
transitions change NMI/animation phases; old pictures at another phase are not
claimed byteexact. Original graphics/resource checks pass for the tested cases.

Giles now accepts wound dressing without requiring or consuming a Healing Pot.
The original Yes/No, following/completed guards and escort actions remain in his
script. Five actual guest cases cover no-potion Yes, owned-potion Yes, decline,
already-following and completed. Acceptance sets the original follow flag and
walking moves GilesY288→365 while Nasir movesY300→375. Potion inventory and the
equipped selector stay unchanged. The installed portable no-potion check also
passes. This proves rescue/following, not the entire escort-home quest.

Optional sword_reach keeps38px forward range and adds3px per lateral edge.
The independent NPC interaction table is unchanged. Exact original script/text/
table guards protect shared data read by both native and interpreted consumers.
See [QoL details](QOL-IMPLEMENTATION.md) and the private portable Giles fixtures.

## Original enemy math on the host

Two guarded entries compute original RAM-only clamp01:A56A and projection01:A5AA
for live type2 enemies in normal rooms03..06. Real caller framesA4D8/A4E7 and
outer80C6, exact ROM bytes, CPU widths/bank/stack, alive loaded field state and
physical nonblank are required. The full16-bit room word is checked. Boss rooms,
Nasir, NPCs, loading, menus and unknown contexts retain original generated code.

The oracle passes7,392 full128KiB RAM/CPU/flag/dead-stack cases,66 read-only
refusals and64 real JSR/RTS entries. Actual two-byte RTS reads remain charged42
master clocks. Original AI, collision, DMA, NMI, audio and tick work are not
skipped. Moving RAM math to host computation removes its guest instruction time
and intentionally changes hardware phase; strict old-clock parity is not claimed.

All376 original bodies remain:374 complete bodies are byteexact; two gain only
an entry guard, with their entire fallbacks byteexact after removing the guard.
Roots, modes, exits and interpreter boundaries are unchanged. Manifest changes
only annotate the two guarded entries. No native promotion or SDK ABI is added.
LAGOON_ACTOR_COMPUTE=off retains original execution; defaultauto is narrowly guarded.

Final same-executable1920×1080 measurements use the real audio device, privately
muted volume, original EPX, VSync and16:9. The sampled window is fields150..1200
(1,051 field positions); fullscreen excludes readback fields247..253.

| Mode | Presentation FPS | Host interval p95 |
|---|---:|---:|
| Hidden, compute off | 60.124 | 17.981ms |
| Hidden, auto | 60.120 | 17.496ms |
| Visible fullscreen, auto | 60.116 | 17.459ms |

Off has69 three-field and421 two-field gameplay updates; auto has525 two-field
updates and zero slower updates. No measured interval exceeds25ms. Eleven audio
high-water waits remain in auto; audio synchronization is not disabled. These
bounded measurements do not guarantee locked60FPS across every room or computer.

## Damage, deaths and boss regressions

The earlier19 old/new pairs retain14 strict passes, three separately source-
qualified cleave-scratch differences and two broader failures. Their original
strict results remain unchanged; no mismatch mask is used. All seven boss kill/
reward/completion and player-death endpoints remain observed in those fixtures.

Final same-cleave off/auto comparisons for Eardon42, Thimale44 and boss48 player
death have byteexact ordered pictures, all checkpoint components and guest clocks.
Their strict comparators remain false solely because off/auto is a declared input
difference. This is qualified mode A/B equivalence, not a strict replay pass.

Room03 revival intentionally changes cadence with compute active. A separate
final event proof observes actual lethal damageHP4→1, then the original healing
loop1→10 over fields3458..3466. Item25 is consumed toFF after healing; checkpoints
through twelve subsequent fields remain alive atHP10. No HP/result writes were
used; selecting the existing item and assisting position are explicit setup.
This event proof is not off/auto timing parity or full death orchestration proof.

## Source-owned incoming and outgoing fades

Room02's twelve original water phases use four384-byte sources. Full ordered
source/descriptor/table/timeline witnesses keep44 incoming fade/settling fields
wide on the final executable. An independent source-matched sample checks all
95,424 margin pixels and the native center.

All46 outgoing03 fade fields459..504 pass independent original-source decoding:
4,389,504 margin observations comprise4,365,584 background,4,324 actual object
and19,596 required-black pixels, with46 exact native centers. Original ROM frame10
pins the actual OAM slots25..32; current OAM reconstruction follows real two-pixel
motion after CPU actor records are cleared. Gold Cave03 also requires its exact
authored1792x1632 world dimensions.

Only nonempty fully native-anchored packets get this exemption. Positions must
unwrap uniquely within eight pixels of their original captured anchors; actual
current tiles/priority/math are rebuilt. Changed unowned margin cells, stale
generations or changed atlas/palette/layout reject. Mixed/virtual packets retain
full CPU-record guards. Old-world native map/used-character authority still binds
the background. Unknown packets retain native fallback; other fades are not
implicitly authorized by these results.

## Validation and remaining work

All50 Python tests, the actor oracle/refusals/returns, wide helpers, water-source
checks, full audited menu-resource checks and launcher Apply/Revert plus120-frame
startup pass. Private compact receipts include final-gpu-summary.json,
FINAL-WIDE-BATCH19.json, final mode A/B comparisons and final-revival-event.
They bind actual identities; their different acceptance criteria are not combined
into a fabricated strict-pass total. Historical evidence is preserved separately.

Private validation retains small recipes and deduplicated states, not executable/
SDK backups or raw campaign archives. It is excluded from public packages/source.
Read [PLAYTEST.md](docs/PLAYTEST.md) for launch/display/state controls. The owner
and friends perform the uninterrupted campaign. Remaining native death/table/boss
orchestration, unsupported scenes, soundtrack mapping and hardware/input coverage
remain unfinished. The custom renderer stays Atland-only; no full-native,
new-backend, retexture or complete-game certification is claimed.
