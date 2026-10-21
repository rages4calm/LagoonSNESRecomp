# Optional X68000 soundtrack

The stock SNES soundtrack remains the default. A separately obtained X68000 VGM
archive can now be converted privately into loop-aware PCM files. Music playback
inside Lagoon is **experimental and partial**, checked on the opening combat
route. A converted pack alone does not enable it.

## Use the optional soundtrack

1. Open Companion's **Audio** page. **Original SNES** is the default.
2. Set **Imported X68000 music folder** to the converted pack folder. Use your own private output folder.
3. Choose **X68000 (partial, experimental)**, Apply, then start the game.

VGZ archives can be converted using the importer below; MP3 files are not needed. The option replaces title music, Atland and the opening Gold Cave.
Other music and missing tracks retain SNES audio. SNES sound effects remain.
Save-state loads restart the selected track rather than restoring its exact
sample position. Run-ahead and netplay use SNES audio. No owner setting was
enabled automatically, and no soundtrack files belong in a public package.

## Import a VGM archive

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
