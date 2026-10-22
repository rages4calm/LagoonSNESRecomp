# Optional X68000 soundtrack

Choose **Original SNES** or **X68000** on Companion's **Audio** page, then Apply
and launch. Original SNES remains the default. The prepared soundtrack is included
in `soundtrack/x68000`; no ripping, converter installation or folder selection is
needed. The game's lossless 44.1 kHz stereo PCM preserves the VGZ loop points.
The accompanying MP3 exports are for ordinary music players, while gameplay uses
PCM so looping does not depend on MP3 encoder delay.

[Browse the 40 MP3s and playlist](../soundtrack/listening) or
[download the MP3 album ZIP](https://github.com/rages4calm/LagoonSNESRecomp/releases/download/v0.1.0-preview.1/Lagoon-X68000-MP3.zip).

Seventeen source-qualified cues use the matching X68000 themes. Other cues,
missing or invalid files, run-ahead and netplay keep the SNES soundtrack. SNES
sound effects remain original. Save-state loads restart the selected track;
they do not restore its exact sample position.

The 0.2.4 audit checked all 75 original room descriptors and the 17 PCM file
identities, headers and loops. Fifty-three descriptors select a mapped theme;
22 retain SNES music, including Elf Field's cue 3C and ambiguous boss scenes.
The prisoner mantle award intentionally plays the SNES item jingle, then
resumes Philips Castle's theme. No new theme substitutions were guessed.

| SNES cue | Location/theme | X68000 archive track |
|---|---|---|
| 10 | Title | 01 Premonition |
| 20 | Atland | 02 Atland |
| 21 | Worship Site | 03 Keep the Faith |
| 22 | Shops | 28 Shop |
| 23 | Denegul | 10 Senegul |
| 24 | Poper | 13 Town of Disease (Poper) |
| 25 | Lilaty / Gnome Tree | 17 Lilaty |
| 30 | Gold Cave | 04 Walking in the Shadows |
| 31 | Philips Castle | 08 Castle Mark Philips |
| 32 | Dwarf Desert | 09 Desert Plains |
| 35 | Siegfried Castle | 15 Castle Siegfried |
| 37 | Hobbit Valley | 12 Valley |
| 38 | Cloud | 21 Reach for the Sky |
| 39 | Ice Cave / Gnome Plain | 18 Ice Cave |
| 3A | Phantom Hill | 19 Run to the Hill of Phantom |
| 3B | Lagoon Castle | 22 Master of Water (Castle Lagoon) |
| 3D | Voloh | 07 Voloh |

Cue numbers above are hexadecimal; archive track numbers are decimal. The
original room descriptors at `$05:B2DF` supply the cue through `$01:8B84` and
`$00:90DB`. Shop scripts use the four `$01:D052` entries. The runtime checks the
original six-byte command row, loaded sound banks and actual JSL helper frame
before replacing music. Ambiguous boss, special and cutscene cues remain SNES;
the included 40-track archive is not a claim that every track has a game mapping.
Original shared music cues stay shared between areas. Names pair location themes
between the two versions; this does not assert identical arrangements.

Music is credited to **Hideki Suzuki / ZOOM**. The VGMRips package was created by
**GTheGuardian**, with optimization by **Valley Bell**. These third-party music
assets retain their original rights and are separate from the project's source
code licenses.

## Reproduce the conversion

The supplied `Lagoon_(Sharp_X68000).zip` contains VGZ files, which are compressed
sound-chip command logs. Use the external [libvgm](https://github.com/ValleyBell/libvgm)
`vgm2wav` converter with **both YM2151 and MSM6258** enabled:

```powershell
python tools/import_x68000_soundtrack.py 'C:/music/Lagoon_(Sharp_X68000).zip' `
  --vgm2wav 'C:/tools/vgm2wav.exe' `
  --output 'C:/music/Lagoon-X68000-Pack'
```

The output must be a fresh directory. The importer reads ZIP members directly,
checks source hashes before and after conversion, and renders one pass with no
fade. It validates both sound devices, exact sample counts and non-silent audio.
The tested converter revision is `a6eed945ecf4e5a25cf82004779c6f7e4fd5c319`.
FFmpeg/libgme is not the converter here; omitting the OKIM6258 core loses drums.

`lagoon-x68000-N.pcm` contains an `MSU1` header, original loop-start frame and
signed 16-bit little-endian stereo samples. `pack.json` records titles,
source/converter/output hashes, duration, loop flags and sample measurements.
`lagoon-x68000.msu` is the empty audio-only data file. All 40 tracks are prepared;
29 Item Get and 31 Say ZOOM! are one-shots, the other 38 have original loops.
A zero PCM header loop offset alone does not distinguish a one-shot from a loop
at sample zero. The runtime retains explicit per-track repeat policy.

The converter runs separately; no libvgm emulator core is linked into the game.
A separately redistributed converter must retain its own licenses: the selected
YM2151 core is GPL-2.0-or-later and OKIM6258 core is BSD-3-Clause. The release
includes prepared audio rather than the converter executable.
