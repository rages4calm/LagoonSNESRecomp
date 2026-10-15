# Lagoon continuation

Keep the interpreter safety net and publish source in substantial, verified
batches. Generated routine counts are not campaign completion percentages.

- [x] First four native roots: movement, clamping, projection and velocity.
- [x] Second four: graphics staging, knockback, map sampling and jump checks.
      The 7,219-frame/18-tag stock route matches the previous native control.
- [x] Thirteen combat/progression roots, with staged real-bat defeat/reward,
      level 2, gold saturation, temporary bonus and equipment comparisons.
- [x] Six more roots: horizontal/vertical player hit direction and boss drawing
      helpers. Real player damage/death and first-boss damage retain exact parity.
- [ ] Promote HP subtraction, death/revival and boss phase/quake orchestration
      only after proving their indirect and nonlocal stack continuations.
- [ ] Continue native promotion from measured hot paths; resolve existing
      native-versus-pure-interpreter timing differences. Keep 00:80E6 interpreted.
- [x] Use CompanionLauncher 0.2.0 instead of the old pre-boot launcher.
- [x] Native display/audio/gamepad settings, Apply/Revert, nine QoL toggles,
      experimental widescreen aspects and preserved settings/bindings.
- [x] Sword reach: guarded directional extents match the supplied BPS's +/-38.
- [x] Followers: guarded ally tier-zero velocity +/-1 -> +/-3.
- [x] Walking: Nasir-only velocity +/-3 -> +/-5; shared tier-two data and NPC/
      enemy speeds stay stock. Measured in the actual town replay.
- [x] Recovery: eight additional immunity ticks without extending knockback;
      real damage routes and native/interpreted hit-eligibility paths agree.
- [x] New-game opening: 18 HP/11 DEF; level 2 is 21 HP/13 DEF, level 3+ stock.
      Current starting stats verified in the runtime opening route.
- [x] Enemy/boss EXP: floor(original * 1.5), all 39/16 guarded template records.
- [x] Jump peak hold: eight frame ticks; targeted fixture verifies the same peak
      and landing eight frames later. Town normally disables jumping.
- [x] Player magic: all nonzero player powers +50%, rounded down. Enemy powers
      unchanged. This is an explicit balance choice; the BPS values are uneven.
- [x] Immediate camera following, clamped to initialized room bounds.
- [x] Keep camera world coordinates intact across 1024-pixel PPU scroll seams.
- [x] Keep immediate following inside the original incremental scenery streamer;
      guarded camera operands stay interpreter-only with an AOT overlap check.
- [ ] Qualify ordinary combat, follower encounters, EXP awards, all spell casts,
      equipment/level progression and actual jump-enabled rooms in gameplay.
- [ ] Serialize optional hold timers for exact mid-hold save/load and rollback.
      Current reset/load generation guards cancel stale host timers safely.
- [x] Experimental additional world backgrounds at original pixel scale;
      16:9, 16:10, 21:9, 32:9 and window-driven aspects.
- [ ] Complete widescreen off-screen entity composition/graphics staging,
      parallax, map bounds, room transitions, HUD/dialogue and campaign coverage.
- [ ] Reconstruct the outdoor room-2 scenery resources before enabling its
      wider field; mask beyond its real 512×512 extent instead of exposing fill
      metatile 29. Dimensions are DP53/55 words multiplied by 8. Retain clean
      native fallback while its margin data is unproven.
- [ ] Adapt title/intro backdrops, the Start/status menu and inventory layout
      for wider displays. Their current native-width fallback is intentional.
- [x] Reconstruct OAM-matched normal NPC and enemy margin cells; reject stale
      parked sprites, dynamic player graphics and unproven states.
- [x] Import all 40 user-supplied X68000 VGZ tracks privately with loop points.
- [ ] Finish optional soundtrack scene mappings and campaign transition/audio
      qualification. Initial replacement covers only opening, Atland and cave.
- [ ] Broader save/load, hardware controller and campaign/ending qualification.

Every QoL feature and widescreen defaults off. Exact implementation/evidence:
[native batch](NATIVE-BATCH4-2026-10-15.md), [QoL](QOL-IMPLEMENTATION.md),
[widescreen](docs/WIDESCREEN.md).

The supplied BPS is a private reference. Keep the clean ROM identity for
generation; runtime-only instruction patches would disagree with AOT code.
Never publish ROMs, generated C, extracted graphics/audio, saves or captures.
