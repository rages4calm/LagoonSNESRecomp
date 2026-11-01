# Architectural native backend inputs

These files contain a finite whitelist of CPU addresses and M/X/E modes, the
original ROM identity, reviewed SDK source hashes and the centered-camera patch
contract. They contain no ROM, generated C, assets or saved games. Some keys are
conditional static continuations; inclusion does not prove runtime reachability
or whole-game completion. Unsupported instructions fail generation.

The default build is unchanged. After setting up the ordinary generated project
and the pinned SDK with this project's cumulative framework patch, a **fresh**
build directory can opt in:

```text
cmake -S . -B build-native -DLAGOON_GENERATE_NATIVE69_FROM_ROM=ON -DLAGOON_NATIVE69_ROM="/path/to/Lagoon (USA).sfc"
cmake --build build-native --config Release
```

Use `SNESRECOMP_ROOT` to select the reviewed SDK when necessary. The generator
does not download, bootstrap or modify it. A changed compiler or ABI requires an
explicit review of the metadata pins. Both C profiles are written only beneath
the selected build directory. Do not distribute those generated sources.
SDK and metadata text pins validate UTF-8 and normalize only CRLF to LF before
hashing, so Windows and Unix Git checkouts agree. All other bytes remain
significant. The ROM identity always hashes the original binary bytes.

The same generator also runs independently:

```text
python -P tools/generate_native_backend69.py --rom "/path/to/Lagoon (USA).sfc" --sdk "/path/to/snesrecomp" --out-dir "/private/build/native69"
```

Generation checks the camera manifest against the actual QoL patch writer and
follower guards. Changing either contract requires updating and verifying the
profile together. Original-ROM attestation, runtime patch admission and per-step
source/mapping checks remain distinct. Generating these files does not disable
the interpreter safety net or enable strict testing by itself.

## Launch the opt-in native build

The ordinary launcher does not enable this development backend automatically.
Launch the generated build explicitly in strict native mode:

```text
python tools/run_native_backend69.py --exe build-native/LagoonSNESRecomp.exe -- --no-launcher
```

Use the actual executable location for your platform/build configuration. All
arguments after `--` pass unchanged to the game. The child runs in the executable
directory with `LAGOON_NATIVE_BACKEND69=1`, `SNESRECOMP_NATIVE_ONLY=1` and
`LAGOON_NATIVE_BOOT62=off`. The wrapper does not edit settings, saves or the parent
environment; the game still uses its normal save/settings behavior. Missing
native coverage or an incompatible build fails closed. The game's exit status,
including strict failures, is returned unchanged.
