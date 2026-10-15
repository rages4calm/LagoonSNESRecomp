# Lagoon continuation

Owner request, October 15, 2026. Keep the interpreter safety net while moving
measured gameplay work into generated native code. Prioritize correct execution
and record route coverage separately from generated function counts.

- [x] First promotion batch: movement integration, clamping, projection and
      velocity selection; 7,219-frame/18-tag replay matches prior native control.
- [ ] Continue promoting gameplay routines using the Ghidra entry/call/dispatch reference;
      measure actual interpreter work before/after and qualify matching routes.
- [x] Replace the default SNESRecomp pre-boot UI with a basic adapter for
      `C:\Users\crono\Desktop\Projects\CompanionLauncher`.
- [ ] Extend Companion's adapter with native display/audio/input and QoL
      controls. Initial adapter supports ROM/executable/config paths and Play.
- [ ] Sword reach: right/down/left/up 28/17/28/27 -> 38/38/38/38 pixels.
- [ ] Follower movement: 1 -> 3 pixels per applied movement frame.
- [ ] Walking: 3 -> 5 pixels per applied movement frame. Existing optional
      implementation needs verification against the supplied BPS.
- [ ] Ordinary-hit protection: eight additional frames.
- [ ] Starting stats: 18 HP, 11 base defense; recover the patch's level-2
      growth changes rather than assuming a uniform bonus at every level.
- [ ] Enemy and boss EXP: floor(original * 1.5).
- [ ] Jump: eight-frame hold at the original peak, retaining movement.
- [ ] Increase every magic power using the supplied hack's values as reference.
- [ ] Immediate camera follow centered on Nasir; inspect existing optional
      implementation and patch behavior, including room boundaries.
- [ ] Optional widescreen for modern monitors: 16:9, 16:10, 21:9, 32:9 and
      window-driven aspect within renderer limits. Study Star Fox Enhanced or
      Mega Man X's framework integration; qualify Lagoon camera, map prefill,
      sprite culling, HUD/dialogue, transitions and reset. Expanded view, not
      simply stretching the original image.

Patch reference: `C:\Users\crono\Downloads\Lagoon QoL.bps`. Analyze a private
patched copy; preserve the clean ROM identity used by generation. Instruction
changes must reach generated AOT code too, not only the interpreter's ROM view.
