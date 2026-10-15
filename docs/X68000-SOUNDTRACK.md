# Optional X68000 soundtrack

The stock SNES soundtrack remains the default. The owner-provided X68000 VGM
archive can now be converted privately into loop-aware PCM files. Music playback
inside Lagoon is **experimental and partial**, checked on the opening combat
route. A converted pack alone does not enable it.

## Use the optional soundtrack

1. Open Companion's **Audio** page. **Original SNES** is the default.
2. Set **Imported X68000 music folder** to the converted pack folder. For this
   local setup it is `C:\Users\crono\Desktop\Projects\Lagoon\Soundtracks\X68000`.
3. Choose **X68000 (partial, experimental)**, Apply, then start the game.

The supplied VGZ archive has already been privately converted; MP3 files are
not needed. The option replaces title music, Atland and the opening Gold Cave.
Other music and missing tracks retain SNES audio. SNES sound effects remain.
Save-state loads restart the selected track rather than restoring its exact
sample position. Run-ahead and netplay use SNES audio. No owner setting was
enabled automatically, and no soundtrack files belong in a public package.

## Import the owner's archive

Use a separately built [libvgm](https://github.com/ValleyBell/libvgm) `vgm2wav`
executable. The tested reference revision is
`a6eed945ecf4e5a25cf82004779c6f7e4fd5c319`. Enable **both YM2151 and MSM6258**.
FFmpeg/libgme is not the converter used here: the OKIM6258 drum/sample device
must not disappear from the conversion.

```powershell
python tools/import_x68000_soundtrack.py 'C:/private/Lagoon_(Sharp_X68000).zip' `
  --vgm2wav 'C:/private/tools/vgm2wav.exe' `
  --output 'C:/private/Lagoon-X68000-Pack'
```

The output must be a fresh directory outside this public source checkout.
`--track 2` performs a bounded Atland conversion before importing everything.
The input ZIP is read directly; paths inside it are not extracted. Input hashes
are checked before and after conversion. Failed conversions remain in a private
`.partial-<id>` directory for inspection and never replace an existing pack.

Outputs are 44.1 kHz, signed 16-bit little-endian stereo:

- `lagoon-x68000-N.pcm`: `MSU1` magic, a little-endian 32-bit loop-start frame,
  then interleaved samples. N is the **archive track number**, not a guest command.
- `lagoon-x68000.msu`: empty audio-only data file.
- `pack.json`: titles, source/converter/output hashes, original VGM total and
  loop sample counts, loop flags, sound-chip clocks and render measurements.
- `render-NN.log`: converter identity/device information and rendering diagnostics.

The importer renders exactly one original pass with no fade. The VGM loop-start
sample is `total_samples - loop_samples`; it is not assumed to be zero. Non-looping
tracks are marked `loop: false` in the manifest and must be played once. The PCM
header's zero field alone cannot distinguish a non-looping track from a loop at
sample zero. The importer requires the rendered sample count to match the source
and confirms both sound devices initialized. Full-scale sample counts are reported;
this is not an independent mastering or listening-quality certification.

The supplied archive credits Hideki Suzuki / ZOOM for the music, GTheGuardian for
the VGMRips pack and Valley Bell for optimization. Generated music, the original
archive, conversion logs and the converter executable stay private. The converter
is an external process; its emulator cores are not linked into this MIT game code.

### Building the external converter

In a separate private tool directory, check out the reference libvgm revision and
[zlib](https://github.com/madler/zlib) tag `v1.3.1`
(`51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf`). Build/install zlib with CMake, then
configure libvgm with its install root:

```powershell
cmake -S libvgm -B libvgm-build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ `
  -DBUILD_LIBAUDIO=OFF -DBUILD_PLAYER=OFF -DBUILD_TESTS=OFF -DBUILD_VGM2WAV=ON `
  -DUSE_SANITIZERS=OFF -DLINK_STATIC_LIBS=OFF -DSNDEMU__ALL=OFF `
  -DSNDEMU_YM2151_MAME=ON -DSNDEMU_MSM6258_ALL=ON -DZLIB_ROOT=C:/private/zlib-install
cmake --build libvgm-build --target vgm2wav -j 4
```

Keep its runtime dependencies beside the external executable or on its tool PATH.
For the tested dynamic zlib build this includes `libzlib.dll`. Do not copy these
libraries into the Lagoon game executable. Observe the separate tool/core licenses
if redistributing a converter; this project does not redistribute that binary.
The selected YM2151 core is GPL-2.0-or-later; the OKIM6258 core is BSD-3-Clause.

## Sound-command research

The shared framework already has an MSU PCM streaming mixer, loop support and an
APU-write interception API. It also has a small copied-PCM overlay mixer, but its
16 MiB cap makes it unsuitable for a complete soundtrack. Streaming is the intended
path. Overlaying PCM over existing SPC music, or muting the entire SPC, is not an
acceptable implementation: SNES sound effects must remain audible.

Lagoon's `0C:F800` and `0C:F8FE` helpers read six-byte event records at `0C:F9FC`.
The first two record bytes can initiate SPC bank uploads; the remaining bytes select
port commands. Helpers at `0C:FE00`, `FE2F`, `FE5E` write ports `$2140/41/42`.
The repeated-event helpers at `FE0E`, `FE3D`, `FE6C` toggle command bit `$40`.
The IPL transfer at `0C:FF69` also uses these ports and must remain intact.
The passive probe now correlates high-level commands with all four CPU APU
ports. It is a default-off development feature (`lagoon.soundtrack:probe`),
requiring `LAGOON_SOUNDTRACK_PROBE` to name a fresh private CSV. It never consumes
a write. The interpreter cursor in this receipt is only the last interpreter
cursor: a generated callee can leave its caller's value behind. It must not be
used to authorize interception. The newest probe also records the live guest
stack and validates the stacked JSL instruction against the six port helpers;
the classifier recognized 31 normal commands in an actual combat-route capture
while rejecting every upload write (162,399 total port writes).

The stock combat route identified these commands:

| Event | Observed purpose | CPU port values |
|---|---|---|
| `10` | Title music | `01 / 02 / 03` |
| `20` | Atland music | `01 / 02 / 00` |
| `30` | Opening Gold Cave music | `01 / 00 / 00` |
| `58` | Scene sound before cave entry | Port 2: `38` |
| `61` | Actual combat hit | Ports 1/2: `12 / 53`, then `52 / 13` |

Bit 6 alternates on retrigger commands; the sequence IDs are the low six bits.
Music uses port 1 (voice 6) as well as port 0 (voices 0–5), and some cues use
all three ports. Muting port 0 alone therefore leaves music audible.

A private utility linked to the unchanged framework SPC/APU/DSP executed the
clean ROM's driver and sample/music banks. With the cave banks loaded, starting
`01 / 00 / 00` activated music voices, sending `00 / 00 / 00` released them,
and sending the actual hit's `12 / 13` activated voices 6 and 7 while music
remained stopped. This establishes the driver's music/SFX separation; it does
not by itself qualify interception inside the game or replacement playback.

The passive probe was also compared with the same executable with it disabled:
85 combat checkpoints (1,020 dump files), full SPC RAM traces, and DSP register
traces were byte-identical. These receipts are private development evidence,
not assets for distribution.

## Experimental implementation and tests

The feature `lagoon.soundtrack:x68000` is off by default. Its optional `pack`
resource selects a private directory under the `snes.msu1.pack` resource identity.
The framework arms MSU from that committed resource; a raw environment override
alone is cleared by normal mod initialization. A track resolver uses the exact
`lagoon-x68000` importer prefix rather than guessing another pack in the folder.
It maps title
(`10`) to archive track 1, Atland (`20`) to track 2, and the opening Gold Cave
(`30`) to track 4. Other cues use SNES audio. The feature validates the PCM
signature, stereo sample alignment and loop point before suppressing a cue;
missing or malformed files retain stock music. Only helper-qualified writes
belonging to the mapped music event are zeroed; effects and IPL remain stock.

`tests/soundtrack_runtime_test.c`, linked separately with
`src/lagoon_soundtrack.c` and the framework `snes/msu1.c`, passed with generated
tone fixtures. It checked actual mixer output, suppression of secondary BGM,
SFX/IPL passthrough, unknown/missing/malformed fallback, and reset. It does not
replace full-game qualification. `tests/soundtrack_spc_test.c`, linked with the
framework SPC/APU/DSP, DSP shadow and audio trace sources, independently checks
the supplied clean ROM's driver. Run it with the ROM path as its only argument;
it does not write or include the ROM.

The host calls `LagoonSoundtrack_Reset()` on hardware/session reset to stop stale
PCM. The title snapshot hooks append a checked 16-byte `LGA1` cue chunk: loading
a state saved during replacement restarts that cue from its beginning. Loading
it with the option off or its PCM missing restores the three original stock
commands instead. A legacy state with no chunk stops host PCM and keeps its
stock SPC state. Current-format preflight rejects malformed/truncated extras
before the guest is restored, and clears staging before every attempt.

Run-ahead and netplay use SNES audio. A frame-clock discontinuity outside the
normal load hooks falls back to the stock cue. These behaviors and missing-pack
state recovery pass synthetic tests; full-game state/load and transition checks
are still required. Exact playback-position restoration is not offered: tracks
restart on load. Remaining track mappings are unfinished.

## Bounded in-game acceptance

The final integrated executable (`b25171ee...`) runs the 85-checkpoint staged
combat route with camera centering and the relocated private pack. Logs confirm
all three mapped starts. Actual hit commands still reach both SFX ports, and
DSP voices 6/7 change after real hits. HP, gold, EXP and level match stock. With
replacement enabled, eight checkpoints contain differences only in unused guest
stack residue; music/SPC handshake timing changes, so full guest-state/audio
identity is not claimed for this optional mode. Missing-pack enabled mode matches
the disabled control's WRAM and complete DSP trace at all 85 checkpoints.

Actual save/load checks restart GoldCave from a new muted state, restore stock
music when that state loads with the option off or pack missing, and load an
older stock state without the cue suffix. A malformed suffix is rejected before
guest writes; the following WRAM matches a no-load control. The private harness
uses a short relative save root to avoid the framework's current 96-byte root
buffer truncating long Windows test paths. Owner states remain untouched.

These checks complement the real MSU mixer's synthetic sample-output test. An
actual post-mix waveform was not captured; full soundtrack, hardware audio and
campaign transition coverage remain unfinished. Companion Apply/Revert, resource
roundtrip, stale-manifest rejection and private native startup tests also pass.

When extending the remaining soundtrack coverage:

1. Identify music start/stop/fade commands in the SPC driver and prove sound effects
   use a preserved path. Never suppress uploads or infer that all port-zero writes
   are music.
2. Map guest scene/music commands to archive tracks. Names differ between versions:
   community documentation suggests SNES Tavern uses X68000 Unusable 03, for example.
   Verify every mapping against actual scenes before treating it as authoritative.
3. Validate the requested PCM before suppressing stock BGM. Missing/bad tracks must
   retain stock music, without silence or two simultaneous BGMs.
4. Handle silence, transitions, fades, one-shot jingles and repeated commands.
5. Reset/rewind/load must discard stale host playback and restore a coherent cue;
   speculative frames must not trigger duplicate starts.
6. Prove disabled-mode guest/audio parity, enabled music replacement, preserved SFX,
   missing-track fallback and representative save/load transitions using synthetic
   audio fixtures before testing the owner's private music.
7. Keep the default-SNES selector and supported-cue description in Companion
   accurate. Pack files and copyrighted music must never enter source commits
   or public releases.

Reference pages: [VGMRips pack metadata](https://vgmrips.net/packs/pack/lagoon-sharp-x68000),
[community track-name comparison](https://w.atwiki.jp/gamemusicbest100/pages/414.html),
and [the unfulfilled MSU-1 request](https://www.zeldix.net/t2387-lagoon).
