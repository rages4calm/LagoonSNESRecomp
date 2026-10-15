# Enhanced original pixels

The optional Enhanced original pixels setting smooths diagonal edges using EPX/Scale2x decisions on the
existing OpenGL presenter. It uses existing source colors, retains nearest
sampling for the bottom 32 logical rows containing the HUD, and does not change
frame geometry, SNES pixels, simulation, saves, movement or timing. It is not new
artwork or motion interpolation. Stock remains the default. The owner selected
this treatment, and the bounded actual GPU checks below pass.

Secret of the Stars' inspected presentation source uses a color-preserving
Scale2x-style CPU pass, with an eight-level color tolerance. Its separate authored
HD menu frames do not establish replacement terrain, characters or fonts. Lagoon
uses exact color comparisons to keep the GPU and deterministic oracle identical;
no source code or files in Secret of the Stars were changed.

## Integration contract

The `assets/shaders/lagoon-epx.glslp` preset refers to `lagoon-epx.glsl` in the
same directory. The native build stages both beside the executable under
`assets/shaders/`. The existing OpenGL Shader setting accepts the preset path;
the shared loader handles compilation and uses the ordinary renderer if shader
compilation fails. An SDL renderer does not execute this preset.

The Companion adapter selects the existing Shader/OpenGL configuration directly,
so no extra native option or runtime module hook is required. Its boolean
`video.enhanced-pixels` defaults off. Enable saves previous Shader, Renderer,
OutputMethod and LinearFiltering values in `[Lagoon] EpxPreviousGraphicsV1`
(base64-encoded JSON), then selects the preset and nearest sampling. Disable
restores those values, including previously absent keys. Unknown fields survive.
INI and shader assets participate in stale-write detection; per-file atomic
commit and rollback are shared with the existing adapter. Missing assets and
incompatible Smooth scaling are rejected before writes. The shared Companion
foundation is unchanged.
Do not modify `prepare_frame` dimensions or the PPU render buffer to enable this.
Only explicitly choosing the treatment should select OpenGL/the preset.

The fragment shader evaluates the published [Scale2x algorithm](https://www.scale2x.it/algorithm)
directly at the viewport's subpixel location, with clamped image boundaries and
no temporal history. The independent CPU oracle in
`tests/support/lagoon_epx_reference.c` writes a 2x surface for tests and offline
comparisons only; it is not linked into the native game. There are no imported
shader implementations, asset packs or GPL
library dependencies.

## Bounded qualification

Standalone tests cover corner decisions, unchanged input, output palette,
padded pitches, HUD replication and rejected dimensions. Companion tests also
verify that the setting defaults off.
Shader vertex/fragment stages pass GLSL 330 validation. Synthetic Companion tests
pass actual Apply/Revert, missing-asset refusal, previous custom/absent graphics
restoration, shader-file revision conflicts and unchanged mod state, alongside
the existing atomic rollback and unknown-field checks. The Display page was
rendered and inspected with the optional control visible.

Actual 1920x1080 OpenGL readbacks at town frame 1750 were compared with the same
executable, private settings, immediate camera following and 16:9 world view.
EPX changed 75,017 display pixels, introduced no new colors, and left every pixel
in the protected HUD rows identical. The town and end-of-route guest framebuffer,
WRAM, registers, VRAM, OAM, CGRAM and clock metadata match exactly. These are
actual post-shader GPU captures; original-resolution diagnostic frames remain
unchanged. Private receipts bind executable, configuration and shader hashes.
The final candidate repeats that result. An additional actual 1920x1080 menu
capture at frame 1786 was inspected: ITEM/MAGIC/EQUIP/SAVE labels, numeric stats
and borders remain readable. The menu retains the guarded native-width view.
That menu image is a readability sample, not an independent stock-menu parity run.

This proves the bounded town comparison, not all scenes or hardware. Separate
cave, boss, shader failure fallback and other GPU/monitor combinations
remain broader checks. No motion smoothness or frame-rate improvement is claimed
by this spatial filter.
