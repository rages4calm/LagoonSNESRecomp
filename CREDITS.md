# Credits

- **ZOOM and KEMCO** — the original Lagoon games. This project does not include
  their ROMs or extracted game graphics.
- **oipo, the Lagoon QoL hack author identified by the project owner** — the changes requested for this port
  were inspired by [Lagoon QoL, Romhacking.net hack #9856](https://www.romhacking.net/hacks/9856/).
  The supplied patch was a reference for reach, follower and movement behavior.
  This port implements its own optional native/interpreter-compatible hooks;
  its opening balance, uniform magic boost, additional lateral cleave and
  Giles first-aid rewrite are project choices.
- **Hideki Suzuki** — X68000 music, as credited in the supplied VGM package.
  **GTheGuardian** created that package; **Valley Bell** optimized it.
  The owner-supplied archive was converted to loop-aware PCM for the optional
  release soundtrack and MP3 for listening. Original music rights remain with
  their respective owners; the port's source licenses do not claim the music.
- **RetroPortingToolKit and SNESRecomp contributors** — recompilation, runtime,
  rendering and the interpreter safety net. See the framework's license.
- **Companion Launcher contributors** — the shared launcher foundation.
- **Carl Prewitt Jr. / CompanionAchievements contributors** — the portable local
  achievement system, distributed under its unchanged MIT notice in
  `third_party/companion_achievements/LICENSE`. Exact imported core and dependency
  hashes are recorded in that directory's `provenance.json`.
- **Dave Gamble and cJSON contributors** — JSON parsing, under the unchanged MIT
  notice in `third_party/cjson/LICENSE`.
- **Square and the respective Chrono Trigger rights holders** — the small Crono
  cameo artwork. Three front-arm poses and the status portrait were sourced from
  [VideoGameSprites.net's Crono page](https://www.videogamesprites.net/ChronoTrigger/Party/Crono/)
  and decoded without repainting their original opaque colors. Source hashes,
  pixel normalization and attribution are in `assets/easter_egg/provenance.json`.
  No Chrono Trigger ROM or whole sprite sheet is included. This original artwork
  has separate copyright; the MIT licenses for Companion and achievements do not
  grant a license to it. Cameo dialogue is original text written for this port. The owner-selected
  completion laugh is the supplied `CTSoundFX_wav/B1.wav`; its original game
  sound retains the same separate Square/rightsholder copyright.
- **Valley Bell and libvgm contributors** — the separate offline VGM renderer
  used to import the privately supplied soundtrack. Its code is not linked into
  the game; retain the renderer's own license when obtaining or distributing it.
- **joshleaves / ghidra-snes and Ghidra contributors** — reverse-engineering tools.

Original work and dependency licenses remain with their respective authors.
The playable package retains the exact achievement toolkit and cJSON MIT notices
under `licenses/`, alongside the framework and launcher license notices. The
root PolyForm license is unchanged and does not replace these separate notices
or the original music/artwork copyrights.

The sword/lagoon and compass application icons are original AI-generated artwork
created for this project. They do not replace the game's original artwork.
