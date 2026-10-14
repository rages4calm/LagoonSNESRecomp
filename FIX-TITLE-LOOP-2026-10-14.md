# Opening loop fix — October 14, 2026

## Result

The rebuilt `build/LagoonSNESRecomp.exe` passes the old man's dialogue,
enters Atland, moves the player down, opens the menu with Start, and closes
it with B. This is a bounded opening regression fix, not full-game certification.
The old release ZIP in `dist/` has not been replaced or published.

## Cause and correction

`recomp/bank00.cfg` authorized `$00:80E6` as a long-indirect pointer call with
only `$02:D646` and `$01:C643` as targets. The generated switch's default case
discarded the three-byte call frame and skipped the live target.

After the opening, `$00:9362` calls `$00:80B3` with A=0. This is the general
bank-$01 routine dispatcher: `$01:8000` contains `$807A`, the main gameplay
routine. `$01:807A` was absent from the authorized list. Skipping it allowed
`$00:9366` to jump to `$00:9304`, clearing scene state and restarting the
opening. The original instrumented run reached `$00:9304` at frame 4213.
This was a software opening-loop return, not a host process crash or a proved
hardware RESET-vector jump.

The correction replaces that incomplete authorization with `force_lle 0080e6`.
The actual ROM dispatcher now executes its runtime target through the existing
interpreter bridge. The patch does not force a game-state byte, alter the ROM,
invent a handler result, or hand-edit generated C. Other proven routines stay
recompiled. A clean default generation emits 143 AOT and 18 LLE variants.
The previous larger generated tree included additional discovery/profile roots;
the default rebuild does not preserve every historical promotion. Root counts
are not a runtime coverage percentage.

`tools/regen.sh` also ignored `--cfg-dir` and always passed `recomp` to the
generator. It now passes the selected directory. A real floor build was made
from `recomp-floor`, copied to `build/lagoon-floor.exe`, and the corrected AOT
build was regenerated/restored afterward. The former floor binary was backed up.

The comparison tools now reject unequal present-stream lengths, empty streams,
missing dump tags, and missing dump components instead of silently comparing
only their common subset. Dump comparisons use explicit Python and absolute
paths in `replay_parity.ps1`.

## Evidence

All evidence paths below are relative to this repository.

- Original trace: `build/investigation/trace.log`; temporary trace instrumentation
  was removed before the final rebuild. The framework submodule is clean.
- Fixed longer opening: `build/investigation/fixed.log`, 6500 frames.
- Deterministic input: `tools/script_gameplay_regression.txt`.
- Final fixed run: `build/investigation/gameplay-fixed.log`, 6239 frames.
- Actual floor run: `build/investigation/gameplay-floor.log`, 6239 frames.
- Captures: `build/investigation/gameplay-fixed/` and `gameplay-floor/`.
- Saved generated identity: `build/investigation/fixed-program_manifest.json`.
- Town appears at frame 4657; regression captures town at 6134, movement at
  6165, menu at 6202, and menu close at 6239.
- Those four gameplay snapshots have identical framebuffer, VRAM, OAM and
  CGRAM between corrected AOT and true floor.
- Full route has 193 differing presentations out of 6239. Raw WRAM/register
  snapshots are not all identical. At `town`, the sole WRAM difference is
  `$7E:01BD`, a stale stack return-address byte (`E3` versus `E6`). Do not use
  that observation to blanket-ignore other differences or claim full parity.
- Independent Mesen replay: `build/mesen-regression-moved/stdout.txt` reports
  `ok:true`, 6165 executed frames, reason `frame_limit`. Both implementations
  reach town and move. Its `last.png` remains visually different: vertical
  tile/HUD placement differs, and 123 of 131072 WRAM bytes differ. This is
  useful independent confirmation of the transition, not exact SNES parity.
- Five comparison-tool checks passed: both empty, one-sided tag, identical
  components, missing component, and differing component. PowerShell parser
  accepted the updated parity script. `git diff --check` passed.

## Save protection and build identity

Before reproduction, 36 existing save files were copied to the workspace's
`debug-backup-2026-10-14/`, with SHA-256 comparison recorded in `receipt.json`.
Every test used a separate save root. The original 36 file hashes remained
unchanged after testing. Owner configuration and key bindings were not replaced.

Final fixed executable SHA-256:
`3359E55275BE718A082405C92F2A0E394995CF82DC4DD46020FEC47C1797DE26`

Actual floor executable SHA-256:
`AA6C89AFBA6A070014A18BF969BEE67AC81B6DE3115F2D64946C08B7D91B2602`

Build hashes describe these exact artifacts, not any future rebuild. The
workspace-root `Start Lagoon.cmd` launches the corrected executable with the
owner's ROM path. The runtime does not depend on Mesen.
