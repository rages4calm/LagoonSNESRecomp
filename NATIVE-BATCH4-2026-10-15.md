# Player damage, boss contact and wider actor buffers

This batch continues source `31a8fab` and the accepted hybrid executable
`c53cb2d4057cba0be13b198036bb5dbd898d4174b8b987ca5268eb0bc322c15f`.
The interpreter safety net, including `00:80E6`, remains enabled. This is bounded
qualification against the previous hybrid build, not a complete playthrough or
independent proof of console accuracy.

## Native execution

Six explicit M1/X0 roots use instruction-boundary timing:

| Entry | Responsibility |
|---|---|
|01:B9B0|Select horizontal/vertical hit direction|
|01:B9C4|Horizontal hit direction|
|01:B9DC|Vertical hit direction|
|01:C3E8|Boss frame offsets|
|01:C402|Boss render palette selection|
|01:C486|Boss projected jump anchor, including hardware division|

The four words at `01:BAFD` are `B9C4,B9DC,B9C4,B9DC`. The dispatcher preserves
the original return frame and uses a pointer-tail declaration for these two
distinct handlers. Both horizontal and vertical actual-hit routes exercised it.
Declaring the handlers alone was insufficient: interpreter JMP arrivals do not
enter native bodies. The qualified dispatcher supplies the genuine call path.

`01:92F1` and `01:DDAD` remain interpreted. Their compiled trial bodies were
never entered because callers arrive through jumps without a native call-frame
contract. The HP wrapper `01:9226`, damage calculation `01:BA6B`, death dispatcher
`01:92C3`, first-boss phase routine and nonlocal death/revival exits also remain
interpreted. No guessed exit modes or broad indirect target sets were added.

Generation produces **170 AOT / 18 declared LLE variants**, from 41 roots.
Twenty-seven explicitly promoted gameplay roots span batches 1–4. These counts
do not measure campaign completion.

## Runtime evidence

| Frozen route | Checkpoints | Ordered presentations |
|---|---:|---:|
|Real horizontal damage, five hits and death|397|3,664|
|Real vertical damage and death|397|3,664|
|Actual first-boss door, initialization, movement and player hit|59|4,088|
|Actual first-boss sword hit, with sword-reach option|71|3,707|

Every framebuffer, WRAM, register, VRAM, OAM, CGRAM and guest-clock checkpoint
matches the control. Every ordered `(frame, crc32)` presentation entry matches,
including stream lengths. No route writes HP, boss damage, enemy flags, rewards
or victory state. Position/guard bypass and equipment-selector setup are explicit
in [damage fixtures](tools/DAMAGE-BATCH4.md) and [boss fixtures](docs/BOSS-BATCH4.md).
Nasir really dies in the damage routes; the boss really loses 96 → 94 HP from a
sword hit. Boss victory and rewards remain unverified.

The six accepted routine ranges remove 93,226 interpreted instructions across
these four routes. `C3E8` and `C486` still have interpreted internal arrival paths;
compiling a root is not evidence that every possible entry uses native code.

## Recovery correction and interpreter support

The previous extra-recovery implementation held a shared counter and prolonged
knockback. Recovery now changes only the validated ordinary-hit eligibility read.
Guest flags, counter, movement and damage arithmetic retain their stock behavior.
The horizontal route preserves all 397 stock movement/HP/counter snapshots. In
the vertical route, protection lasts exactly eight additional samples, followed
by a real hit as soon as the window expires.

The shared WRAM read hook supplies live CPU registers for generated code and
scoped pre-instruction registers for the interpreter. It validates the original
ROM helper and caller return frame; interpreter reads also require the exact
instruction cursor. Hook-off behavior remains the original bus path. Production
bus tests cover aliases, ordered word reads, wrapping, open bus, unchanged RAM,
IO bypass and nested context restoration. A separate build forcing `01:BA52`
through the interpreter matches all 397 vertical checkpoints with recovery both
off and on. Reset/load generations cancel stale protection safely; exact timer
serialization remains unfinished.

## Renderer and public screenshots

The wider actor-buffer qualification and remaining gates are described in
[the buffer receipt](tools/WIDE-BUFFER-BATCH4.md). It is read-only presentation:
guest AI, movement, combat, room data and saves remain authoritative.

Inventory and Start/status overlays now trigger clean native-width composition,
including mid-frame BG3 changes. Unsupported resource sets and active room loads
also use native fallback. Atland and Gold Cave have exact audited resource
fingerprints; the outdoor room before the cave remains native-width. Its opaque
fill metatile was exposed beyond the actual 512×512 room and produced the reported
edge garbage. This gate bounds support; it does not claim to repair every outdoor
map. Wider title/intro and menu layouts are explicit follow-up work.
Resource changes must settle across two prior completed field boundaries before
expansion resumes. The official title compositor enforces fallback on the actual
presentation destination. A frame sequence covering the cave transition checks
all 391 presentations, including the formerly corrupt single frame; unsupported
margins are clean and the native center is unchanged.

Screenshots requested by the project owner are published under
`docs/screenshots/`. They illustrate experimental supported scenes; they do not
certify every room, actor form, aspect ratio or campaign event.
All three are actual 1920×1080 Windows SDL fullscreen readbacks with camera
centering enabled. Their source presentation pixels also match the final build.
Visible harness runs now keep normal pacing; headless checks remain unthrottled.

## Reproduction and evidence identity

Accepted executable SHA256:
`904d218f875e4ebd877e9f5df4b85aecf901efbdc70fd7b04c26d9822fcf752b`.
Private evidence is under workspace `work-native-20261015-130023`, using fresh
save roots and frozen route copies. Final damage/boss outputs are `published-contact`,
`published-vertical`, `published-boss`, `published-boss-damage`; corrected recovery is
`published-recovery`. The earlier `native-seven-execution.json` records the
trial and explicitly rejects its quake-native claim. The final receipt binds the
accepted six-root executable separately in `published-native-verification.json`.

SNESRecomp upstream HEAD was rechecked as
`a6be9756285f26ccc74539c811d4d23d57d8c33d`. The public gitlink stays at that
reachable revision. The cumulative framework patch applies to a fresh checkout;
its read/write-bus and precise-timing regressions pass there. Generated code,
ROMs, audio, save/state files and private probe output remain excluded from Git.
