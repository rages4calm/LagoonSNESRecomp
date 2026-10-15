# Native and renderer batch 9 — October 15, 2026

The live custom scene backend now independently composes supported stable Atland
fields from copied map, tile, palette and actor data. It replaces a presentation
only after every 32-bit pixel matches the still-running reference renderer.
Other scenes, loading/raster effects and failed source checks retain reference
output. This is bounded original-art rendering, not a full renderer replacement.

The selected executable SHA256 is
`81bf8a4bf685fca8994836e3701499677ad9afded0139673b5732e269d97b814`.
The generated manifest SHA256 is
`b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`:
190 AOT variants, 18 declared LLE variants and 47 promoted gameplay roots.
The opening indirect-dispatch safety floor at `00:80E6` remains interpreted.

Four renderer-on/off replay pairs on this exact executable match at 146
checkpoints and all 7,185 ordered presentations. They include cold loads at
16:9/32:9, Atland movement/dialogue, turns, menu and reset. The custom backend
actually replaces 2,166 fields with zero mismatches; source-identical adjacent
packets reuse prior independent output, so actual drawing occurs 637 times,
covering 95,637,696 pixel observations. Every replacement still compares the
entire reference field. Counts describe repeated bounded observations, not
unique assets or campaign coverage.

Native promotion adds only two bounded roots: `01:9245` death initialization
through the explicit `9266` boundary, and `01:9294` HUD selection/return. The
shared death/revival bodies and remaining boss handlers keep their interpreter
exits. See [damage evidence](tools/DAMAGE-BATCH9.md). The native changes remove
90 interpreted instructions and 373 interpreted guest cycles across the four
primary fixtures; this is no FPS claim. Existing native-versus-pure-interpreter
timing differences remain unresolved.

Outgoing widescreen fades retain a completed old room only while actual native
map/character data and actor ownership still prove it. The direct transition
stage qualifies 46 formerly narrow fields, all 4,389,504 added source-pixel
observations and 37,659,025 required-black observations without leaks. That
source-pixel analysis belongs to its recorded earlier executable; a separate
final-build binding preserves the distinction. See [widescreen evidence](docs/WIDESCREEN.md).

Rendering cost and cadence are measured separately from correctness. Source
decoding caches, eight-pixel background spans and exact packet reuse reduce
duplicate work. The reference PPU remains active, so this stage adds validation
work rather than demonstrating a renderer speedup. See the controlled timing
results and limitations in [performance evidence](docs/PERFORMANCE.md), and the
[renderer contract](docs/CUSTOM-RENDERER.md).

The final hidden, normally paced 32:9/Enhanced run averages 60.10 FPS over the
profiled interval; mean composition is 0.356 ms, versus 1.473 ms in the initial
uncached live-renderer run. It still has three intervals over 20 ms and two over
25 ms, reaching 30.09 ms. Settled audio high-water waits are zero on this run.
The same-executable off control has a 135.61 ms outlier, so this pair cannot
establish a frame-rate speedup or physical-display cadence. All seven endpoint
components/clocks match between modes.

States retain version-3 raster-journal validation. Owner saves/settings are
protected; test saves, ROM-derived generated C, graphics, audio and raw captures
stay private. The public framework pin remains upstream
`a6be9756285f26ccc74539c811d4d23d57d8c33d`; the unchanged cumulative patch
reconstructs 38 source paths exactly. No binary release is part of this batch.
