# recomp/

Analysis input for Lagoon. `tools/regen.sh` reads this directory and
writes `src/gen/`.

| File | Owner | Notes |
|---|---|---|
| `bank*.cfg` | you, plus generated blocks | Per-bank analysis directives |
| `symbols.toml` | you | Progressive name map; the source of truth |
| `campaign-roots.txt` | reviewed coverage policy | Exact additional analysis seeds, without introducing function boundaries |
| `precise-timing.txt` | reviewed timing policy | Instruction timing selectors; modes are explicit where needed |
| `transfer-timing.txt` | reviewed timing policy | Completed-call synchronization for newly staged native variants |
| `funcs.h` | generated | Re-synced by `tools/regen.sh` — do not hand-edit |

The block between `>>> BEGIN symbols.toml` and `<<< END symbols.toml` in
each bank cfg is rewritten from `symbols.toml` on every regen. Edit
`symbols.toml`, not the block.

Python 3.11+ includes the TOML reader. On older Python, install it with
`python -m pip install tomli`. Projects without `symbols.toml` do not need it.

Each `[[func]]` has a C identifier `name`, a 16-bit hexadecimal `addr`
(for example, `"8000"`), and an integer `bank` (for example, `0xc0`).
Missing bank configs are created automatically; hand-written directives outside
the generated blocks are preserved. Removing an entry also removes its generated
directives on the next regen.

- `emit = false` (the default) writes a `symbol` label and `force_lle` boundary:
  it stays interpreted, with no generated `func` declaration in `funcs.h`.
- `emit = true` writes a `func` declaration and requests analysis from that
  entry, even without `--cfg-roots`. Only variants the analyzer proves safe
  become AOT code; others still use the interpreter. Optional `entry_m` and
  `entry_x` select the initial register-width flags (0 or 1, both default to 1).

An unchanged cfg file is not by itself evidence of failed code generation:
`src/gen/bank*_v2.c` contains code discovered from the ROM, while `bank*.cfg`
contains analysis inputs. Discovery does not copy every reached function back
into the configs.

## Campaign generation policy

Use `tools/regen.sh` for the selected policy. It supplies the additional root
list and both timing selectors automatically; private coverage captures are not
needed to reproduce the public program. Generated C, ROMs and runtime captures
remain private. A seed is a request for ROM analysis, not permission to bypass
eligibility checks or the interpreter.

`force_lle_variant 0084E9:M1X1` retains one architectural mode on the interpreter
while preserving native sibling modes at the same address. Ordinary
`force_lle 0080E6` still excludes every mode at that address. These exclusions
apply to the generated program, rather than depending on a diagnostic deny file.
See [timing contracts](../tools/NATIVE-TIMING.md) and
[current qualification](../docs/DEVELOPMENT-STATUS.md) before changing them.
