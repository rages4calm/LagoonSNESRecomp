# October 31 framework integration

Development source now pins SNESRecomp
`98f41838443a7cd7f9d56bec41387a6ebfc3bcbc`, 192 commits after the prior
`a6be9756` release-build pin. This includes the October 30 optimization campaign
and subsequent shared GPU Mode 7 work. Existing Windows 0.2.5 and Linux/macOS
preview downloads are separate qualified artifacts; changing source does not
update their binaries or establish a new campaign playtest.

## Preserved behavior

The residual framework patch carries title requirements still absent upstream.
Fresh application was checked against the pinned tree. Already-integrated
upstream changes are removed from the old patch rather than applied twice.

Preserved contracts include exact source/mode/stack/generation admission,
native/interpreter continuation ownership, instruction/audio timing, WRAM
filters, modal dialogue audio/input, host volume, wide OAM/scroll buffers,
native-center shadow boundaries, first-visible-line mosaic, save/load
reconciliation and profiling. New upstream guest-scope, producer-wait and HD
packet interfaces remain present. Lagoon's custom scene rendering stays
authoritative; GPU Mode 7 does not redraw ordinary tiled artwork.

The build adopts upstream native DSP mixing and fused immutable ROM reads.
References remain selectable. CPU interpretation, SPC execution and all
existing excluded game variants remain supported.

## Qualification boundaries

- Fresh generation retains guarded manifest
  `67a87e43c43317eaaf8bc7303bce70c8c6a98a37e9c7ea79aa9af16257fbae11`.
  Game bodies are identical; only generated build provenance changed.
- Framework generation/continuation tests: 616 passed, 2 skipped, 20 subtests.
- Wide native-center boundary: 8 scroll phases, main/subscreen and margins.
  WRAM alias, order, wrap, open-bus and unregister checks passed.
- Matched isolated routes cover Atland 32:9, Atland/cave/Elf/Voloh transitions,
  Elf enemy activation and Evil Spirit/Zerah health fixtures. Ordered pictures,
  checkpoint guest memory/registers/clocks, APU ports, DMA and PPU writes match
  the released Windows control. These are bounded fixtures, not a campaign.
- A separate released-control/new-build comparison retains complete ARAM and
  DSP-register write streams. No audio-trace exception was granted.
- Elf combat profiling uses serial ABBA control/candidate/candidate/control
  runs on one Windows machine. Gains are route-specific, not the upstream
  campaign's percentages and not a universal locked 60 fps claim.
- Linux/IntelMac/AppleSiliconMac compilation, GL and isolated persistence gates
  run in the private build repository. No ROM or owner progress is uploaded.
  Those gates are distinct from target gameplay/audio/controller playthroughs.

Compact local source bindings/receipts are in the owner's `validation/upgrade57`.
Raw outputs are private and disposable after summary/fixture retention. The
installed Windows release and original dirty SDK remain intact.

## RomAtlas research and rejected promotions

RomAtlas was built with Rust 1.90.0 and locked dependencies as an independent
research tool. A local Windows adapter explicitly refuses its Unix-only
external assembler execution; inspection/contextual decoding remain available.
No RomAtlas emulator/runtime code is linked into Lagoon. Its combined CLI uses
GPLv3; independent native-port helpers have separate MIT grants.

Two timing boundaries were investigated using its decoder and live coverage:

1. `$00:81F7:M1X0` is a queue service with a real `$00:81AB` child and a wait
   on `$023A`. The candidate passes 6,144 original-opcode comparisons of state,
   flags, real stack and constant-bus clocks. Live evidence records 170 compiled
   entries versus 0 in the same-EXE interpreter control. Pictures/checkpoint
   state match, but complete ARAM/DSP streams diverge. Completed-transfer timing
   alone does not fix it. **The force-LLE exclusion remains.** Next resolve
   nested bridge/APU batch boundaries; do not repeat the source matrix or use
   passing screenshots as sufficient proof.
2. `$01:814E` is the heavily executed `$0418` interrupt wait. Its counter update
   is in the original NMI route. Generic cycle replay has IRQ/HDMA/epoch guards.
   The bounded probe did not demonstrate replay activation; no declaration or
   guard relaxation ships from that experiment.

`tools/test_shared_services_oracle.py --probe-wait` supports the source matrix
against an explicitly selected SDK (`SNESRECOMP_ROOT`).
`tools/qualify_wait57.py` repeats the strict same-EXE 81F7 test for a deliberately
generated candidate, requiring positive entry evidence and exact device traces.
It correctly fails on the unqualified candidate and cannot pass merely because
the production build continues interpreting that routine.

Inventory remains 654 AOT/74 analyzed LLE variants, not a completion percentage.
This batch does not claim 100% translated CPU coverage or remove the safety net.
