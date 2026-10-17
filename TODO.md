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
- [x] Native damage calculation, boss player-contact, death-stop velocity and
      room-hazard/first-boss quake dispatch; exact damage/revival/defeat parity.
- [x] Native HP addition/clamping and revival healing entry, with suspended-call
      bank preservation and four bounded combat-route checks.
- [x] Native paired actor/boss dispatch, player-hit wrapper and first-boss phase;
      four complete replay comparisons confirm actual native execution.
- [x] Native HP-subtraction prefix and bounded death initialization/HUD tails,
      with exact damage, consumable revival and boss-defeat replay comparisons.
- [x] Batch 12: native HP/MP caps and actor-hit setup plus 13 further variants;
      all 326 prior modes retained, SRAM-aware replay checks and room-01 scenery.
- [x] Batch 13: original six-guardian death escapes, revival/recovery entries,
      status/destination-fade torch ownership, strict optional audio diagnostics
      and linked area-map references. Cooperative recovery loops still use LLE.
- [ ] Promote full death/revival and remaining boss-phase orchestration
      only after proving their indirect and nonlocal stack continuations.
- [ ] Continue native promotion from measured hot paths; resolve existing
      native-versus-pure-interpreter timing differences. Keep 00:80E6 interpreted.
- [x] Use CompanionLauncher 0.2.0 instead of the old pre-boot launcher.
- [x] Native display/audio/gamepad settings, Apply/Revert, ten QoL toggles,
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
- [x] Expand audited outdoor room 2 using actual raster scroll anchors and
      mask beyond its real 512 by 512 extent. Natural walking, menu, doors and
      reset checks preserve native center/state/timing; unknown resources fallback.
- [x] Keep verified world scenery behind Start and town dialogue; reflow Items
      and Magic frames from original pixels without stretching text/icons.
- [x] Audit Gold Cave rooms 4, 5 and 6, real door transitions and world extents.
- [ ] Adapt title/intro, equipment/save layouts and unaudited scripted scenes.
- [x] Optional Faster menus: one second less fade delay entering Items/Magic.
- [x] Remove Smooth scaling; retain optional Enhanced original pixels.
- [x] Preserve both raster register journals in development states; fix alternating
      black frames after cold loading, with consecutive-field and hidden GPU checks.
- [x] General live-execution/PPU-port state serialization, exact same-process
      and cold-process restores; incomplete/corrupt files refuse cleanly.
- [x] Reconstruct OAM-matched normal NPC and enemy margin cells; reject stale
      parked sprites, dynamic player graphics and unproven states.
- [x] Import all 40 user-supplied X68000 VGZ tracks privately with loop points.
- [ ] Finish optional soundtrack scene mappings and campaign transition/audio
      qualification. Initial replacement covers only opening, Atland and cave.
- [x] Optional original-art EPX smoothing with sharp HUD, reversible Companion
      settings and actual full-HD GPU capture checks. Full retexture shelved.
- [ ] Broader EPX menu, GPU and monitor qualification.
- [x] Profile sustained widescreen world performance in isolated fullscreen runs;
      the earlier 46 FPS average was not reproduced, with baseline runs near 60.
- [x] Use a precise final deadline wait on SDL3 to reduce scheduler jitter.
- [x] Keep the initial Atland world wide during its verified loaded fade.
- [x] Retain source-proven outgoing scenery/actors through audited room fades;
      expire ownership on actual map/character uploads and timeline changes.
- [ ] Broaden sustained frame-time checks across more areas/devices and investigate
      remaining cave fade, state-load settlement and unaudited transition gaps.
- [x] Connect an independent live Atland scene renderer with immutable packets,
      exact whole-field reference checks and fail-closed presentation.
- [ ] Broaden custom renderer scene/HUD coverage and reduce its measured CPU cost.
      Choose graphics API from measured needs; keep reference output.
- [ ] Broader save/load, hardware controller and campaign/ending qualification.
- [x] Accelerated entry into all 73 rooms with ordinary incoming doors, seven
      main-boss wins and player deaths, six endgame phases and six real Thimale
      guardian escapes. Original outcomes and explicit cheats are recorded.
- [x] Audit and ingest canonical campaign coverage with build/ROM identity,
      journal-loss, duplicate-session, bounds and guest-failure checks.
- [x] Resolve the expanded candidate's Ella and handoff timing regressions;
      select 326 native variants (+136), qualified through 95 strict replays.
- [ ] Investigate 22 deferred exact modes: 20 bank-00 interrupt/display paths,
      actor-event sound timing at 01C2CD:M1X0 and graphics timing at 02D1C5:M1X0.
      Preserve qualified sibling modes and the interpreter until proven safe.
- [ ] Natural campaign progression, source/runtime classification of records2A/3F,
      unobserved entry routes and complete credits qualification.
- [ ] Optional in-game map showing Nasir, visited rooms, doors and key locations;
      build from source-proven room data after broad campaign coverage.

Every QoL feature and widescreen defaults off. Exact implementation/evidence:
[native batch](NATIVE-BATCH9-2026-10-15.md), [QoL](QOL-IMPLEMENTATION.md),
[widescreen](docs/WIDESCREEN.md).

The supplied BPS is a private reference. Keep the clean ROM identity for
generation; runtime-only instruction patches would disagree with AOT code.
Never publish ROMs, generated C, extracted graphics/audio, saves or raw test
captures. Only explicitly approved, curated gameplay screenshots are public.
