# Collecting accelerated campaign coverage

Coverage records real interpreter instructions and observed transfers. It does
not prove a whole boss was fought, a reward was granted, or a routine is safe to
run natively. Cheats and story/room seeds must remain explicit scenario context.
The interpreter safety net stays enabled throughout collection and qualification.

## Supported SDK flow

The current SDK checkout supports:

1. `tools/qualify_native.py --coverage`: enables runtime capture only in the new
   private run and redirects it under that run's `coverage/` directory. It retains
   the exact executable, program-manifest, script, input/settings and saved-state
   provenance in `receipt.json`. Use bounded headless scripts and private saves.
2. The runtime writes unique v2 JSON checkpoints and JSONL transfer journals.
   Each reset creates a separate capture session. Checkpoints contain instruction
   costs; journals preserve transfer discoveries and may survive a crash, but
   journal-only recovery is not complete cost accounting.
3. `snesrecomp/tools/tier2_ingest.py` audits one or more compatible captures. Its
   reader keeps the newest cumulative row per session before summing separate
   sessions, so checkpoint + journal + duplicate imports do not multiply costs.
4. `snesrecomp/snesrecomp_cli.py generate` accepts repeatable
   `--profile-manifest` inputs. Different executable/program identities must be
   kept separate; older-build bundles may be provided with repeatable
   `--historical-profile-manifest`. Historical inputs carry analysis seeds and
   conservative exclusions, never merged cross-build costs.

The SDK rejects mixed ROM/module/mapper/program/executable identities and
conflicting equal-sequence rows. Missing identity, bounded capture loss and
incomplete journals can otherwise be warnings. The project wrapper below adds
strict replay-receipt binding and refuses incomplete inputs by default.

## Project campaign registry

Create a **private** JSON registry. IDs and labels are chosen from the campaign
plan, rather than guessed by this tool. A run may have several scenario tags:

```json
{
  "schema": "lagoon campaign coverage plan v1",
  "rom_sha256": "<verified 64-character lowercase ROM digest>",
  "expected_scenarios": [
    {"id": "boss-first-entry", "label": "First boss: entry and combat"},
    {"id": "boss-next-entry", "label": "Next boss: entry and combat"}
  ],
  "runs": [
    {
      "id": "first-boss-attempt",
      "directory": "first-boss-private-run",
      "scenarios": ["boss-first-entry"],
      "context": {
        "setup": "Explicit room/story/progression seeds; list exact writes",
        "observed": "Describe actual combat, damage, death/reward evidence",
        "limits": "Declare anything skipped or not reached"
      }
    }
  ]
}
```

Directories resolve relative to the registry; absolute paths are also accepted
for reading. Collect each directory using the actual qualification harness:

```powershell
python -P tools/qualify_native.py --exe '<immutable-candidate.exe>' --manifest '<its-program_manifest.json>' --route '<bounded-script.txt>' --frames 8000 --coverage --out '<fresh-private-run>' --config '<private-config.ini>'
python -P tools/campaign_coverage.py --registry '<private-registry.json>' --out '<fresh-private-audit>'
```

The wrapper requires a successful, script-isolated run, route/manifest identity,
all seven dump components, and v2 capture identity matching the receipt's ROM,
program and actual executable digest. Captures must use Lagoon's main-module
LoROM identity. Paths must remain within each run; output must be fresh and must
not overlap inputs, and per-file reads are bounded to
128 MiB. It rejects missing modes, invalid PCs, negative/boolean/overflowing
uint64 counters, conflicting records, and runtime-injected qualification
allowlists. Same-build merged counters are checked again after SDK merging.

A zero process exit and complete capture checkpoint are insufficient when the
runtime failed. The collector independently scans saved stderr/stdout for
`[brk]`, `[interp_cap]`, `[interp_low_exec]`, `cpu_halt` and assertion-failure
markers, and rejects receipts with `runtime_failures` or a rejected
`qualification_passed` result. This also protects older receipts lacking those
fields. Fatal runs are rejected even with `--allow-incomplete`; their observations
must not seed campaign generation or count as a reached room/boss scenario.

Checkpoint/journal bytes are frozen before merging, so later source-file changes
cannot silently alter the published bundle. Original capture IDs, sequences,
outcomes, widths, emulation modes, processors and raw bank addresses survive.
The wrapper never turns a journal sighting into a completed call.

Output contains:

- `summary.json`: input/route/receipt/dump hashes, settings and setup context,
  per-build costs, candidate-status counts and every expected scenario. Missing
  scenarios are explicitly `absent`; a tagged run is **not** a verified victory.
- `groups/<identity-digest>/coverage.bundle.json`: original v2 records in the
  SDK's supported `snesrecomp tier2 bundle v2` format, separated by immutable
  ROM/module/mapper/program/executable identity.
- The original `program_manifest.json` and SDK `audit.json` alongside each bundle.
- `INGESTION.md`: PowerShell-quoted audit and **private candidate-generation**
  commands. The tool does not execute them or edit existing generation settings.

`--allow-incomplete` permits a salvage audit while preserving loss/torn-tail/
checkpoint warnings. It sets `ingestion_ready=false` and omits generation recipes.
This option does not reconstruct missing instruction costs. Captures with
contradictory identities or invalid values remain errors even in salvage mode.

## From observations to native candidates

Audit each bundle against its saved original program manifest:

```powershell
python -P snesrecomp/tools/tier2_ingest.py '<bundle.json>' --program-manifest '<original-program_manifest.json>' --rom-sha256 '<verified-ROM-digest>' --cfg-dir recomp --json
```

Only after reviewing the actual caller/mode/exit evidence should the SDK be run
against a **private reviewed cfg copy** and **fresh private generated directory**.
Preserve the title's existing root flags, exclusions and precise-timing setup.
Do not use the normal `tools/regen.sh` for an exploratory import: its output is
the shared `src/gen`, and it can rewrite the shared function header.

```powershell
python -P snesrecomp/snesrecomp_cli.py generate --rom '<private-ROM>' --cfg-dir '<private-reviewed-cfg>' --out-dir '<fresh-private-gen>' --expected-sha256 '<verified-ROM-digest>' --profile-manifest '<same-build-bundle.json>' --historical-profile-manifest '<older-build-bundle.json>'
```

Profile seeds can automatically materialize newly eligible native bodies in this
**candidate** output. This is not an analysis-only or acceptance switch. The SDK
filters bailed/unsafe targets, unknown entry modes, emulation-mode entries, and
goto landings without independent function boundaries; it also analyzes every
exact variant. Those filters do not establish semantic correctness, exact cycle
charging, caller stack contracts, indirect dispatch completeness or useful native
execution. RAM snapshots require decoded, stable extents and byte guards.

Inspect the entire generated closure, retain real interpreter defaults, and
promote only bounded reviewed roots. Confirm they actually execute natively, then
compare the same campaign routes across guest state, complete ordered pictures
and clocks before adopting the change. Preserve old failure exclusions until the
specific bug is understood. Keep costs from different executables separate.

`tools/coverage_report.py` remains a legacy per-file convenience display and is
not the campaign merge/identity gate. Use the SDK reader through this wrapper.

Focused ROM-free tests:

```powershell
python -P -m unittest discover -s tests -p test_campaign_coverage.py -v
```

No campaign coverage percentage, all-boss completion, or full native-port claim
is inferred from a list of observed PCs.

## Canonical campaign observation batch — 2026-10-17

The final stock-ROM bundle contains **79 accepted captures** from one executable
and program manifest, with all QoL options off: 73 room-entry/activity probes,
one original six-guardian path, and five accepted endgame runs covering Zerah,
Thor, Thor Bird, Spirit Shell, Green Evil Spirit and Red Evil Spirit. The
75-room registry explicitly leaves rooms `2A` and `3F` absent. Room entry alone
does not prove boss behavior or victory; the separately bound acceptance
receipts contain the actual damage, death, creation and earned-flag evidence.

The collector and real SDK `tier2_ingest.py` audit accepted:

| Observation | Count |
| --- | ---: |
| Interpreted instructions | 824,526,719 |
| Interpreted guest cycles | 2,786,778,153 |
| Instruction PC/mode variants | 9,561 |
| Distinct processor/instruction addresses | 9,551 |
| Transfer variants | 641 |
| Complete capture sessions | 79 |

All reported loss/overflow counters and SDK warnings are zero. There are
15,540 raw records, zero exact duplicate records, and no capture IDs reused
across runs. The SDK still joins cumulative checkpoint/journal observations by
session and sequence before adding independent captures; raw record totals are
not instruction counts.

The identity is stock-ROM SHA256
`46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb`,
program manifest `b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`,
and executable `5b17f7704f6b8ea8eb2231f9addd68b5582ef917c37f7a6d51cc69540f1262bc`.
The merged bundle SHA256 is
`279bc9371fe0c517202aba367f84690b65c465b0c8fa65b0a8eeb6b9dc513090`.
Raw captures, ROMs, private states and the machine-local registry stay private.

Setup cheats include explicit room-door substitutions, player stats, equipment,
HP restoration, positions/facing, and Red's entry prerequisite flags. Thor's
zero HP was not directly sampled; its original death handler and subsequent
Bird creation were observed. This is accelerated, bounded evidence rather
than an uninterrupted natural campaign or cross-emulator comparison. Seven
QoL-patched boss captures remain outside this canonical bundle; their ROM
identity was not rewritten to stock.

**No new native code was promoted.** An earlier private 348-AOT candidate failed
strict WRAM/register/clock comparisons and one ordered Ella picture at field
631, so it remains rejected. The final merged observations were audited only;
they were not used for another generation or build. Next native work should
select bounded targets from this evidence and qualify their real executions.

## Private builds and exact timing modes

CMake accepts `-DLAGOON_GENERATED_DIR=<private-generated-directory>` so an
exploratory program can be compiled without replacing `src/gen` or the selected
playable executable. Keep its build and save roots private. Generation or a
clean structural check does not qualify that executable.

The project framework patch accepts exact timing selectors such as
`0084E9:M1X1` in `SNESRECOMP_PRECISE_AOT_ROOTS`. Legacy plain PC tokens continue
to select every mode at that entry exactly as before. Exact selectors allow a
newly observed mode to use precise instruction timing without changing a
historical sibling's coarse timing. Invalid mode selectors fail generation.
The legacy no-profile output is byte-identical in the recorded test. The timing
selector alone did not qualify the earlier larger candidate. Batch 11 below
records acceptance of the narrower program after its timing fixes and deferrals.

## Strict replay comparison

Use `python -P tools/compare_native_runs.py BASELINE_RUN CANDIDATE_RUN --out REPORT.json`
for two isolated `qualify_native.py` runs. Exit 0 means exact, 1 means a measured
difference, and 2 means invalid evidence. Inputs and frozen files must match;
all ordered presentations, full checkpoint RAM/VRAM/OAM/palettes/registers and
clock metadata must match. Stack bytes and beam positions are included.
Requested features that refuse activation invalidate qualification, even when
the host exits successfully. Executable/program identities are recorded
separately because a candidate intentionally differs from its control.

Campaign roots now also have a reviewed public source policy in
`recomp/campaign-roots.txt`; `tools/regen.sh` applies it without needing private
captures. This does not grant automatic native eligibility to later profiles.


## Batch 11 selected-program refresh

All 79 canonical stock-ROM captures were rerun on the selected 326-variant
program and strictly compared with their prior controls before collection.
Actual SDK ingestion accepted 779,504,296 interpreted instructions,
2,617,808,165 interpreted guest cycles, 8,463 PC/mode variants (8,454 addresses)
and 535 transfers. All four loss counters were zero, with no warnings or reused
capture IDs. The same fixture set executes 45,022,423 fewer interpreted
instructions than the prior 190-variant build. This is measured work within
these assisted fixtures, not a campaign completion percentage. Raw profiles and
their receipt bindings remain private. Public roots and timing selectors reproduce
the selected generated program without distributing those captures.
