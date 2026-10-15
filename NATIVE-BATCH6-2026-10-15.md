# Native HP, camera, menus and reliable states — batch 6

This is a bounded continuation with the 65816 interpreter safety net intact.
It does not certify the full campaign or a fully native port.

## Native gameplay

Two new roots cover HP addition/clamping (`0191EA`) and the revival healing
entry (`01D487`). There are 34 explicit promoted gameplay roots, 177 eligible
AOT variants and 18 declared LLE variants. Four combat routes preserve six
snapshot components and guest clock at 953 checkpoints, plus all 15,939 ordered
presentations, against batch 5. Those captures remove 21,671 interpreted
instructions representing 86,122 guest cycles; this is interpreted-work cost,
not a measured wall-clock speedup or completion percentage.

The generated JSL envelope now keeps the live callee bank when an interpreted
child suspends before RTL. Executed-code unit tests cover ordinary returns,
nonlocal returns and suspension. An additional recovery-option route preserves
all presentations, timing and bytes except three PB fields: source/stack evidence
proves the corrected bank-00 wait, replacing prematurely restored caller PB=02.
That route is explicitly excluded from exact-parity totals.
See [the detailed native evidence](tools/DAMAGE-BATCH6.md).

## Camera and widescreen

Immediate camera following now runs after movement and collision resolution.
Bounded stop, turn and wall checks keep Nasir at the intended centered anchor;
already-queued projectile/actor projections follow the same final camera.
Audited dialogue and Start retain the wider world after valid scene settlement.
Items and Magic widen their original decorative frames while keeping live text,
icons and the cursor at original pixel scale. Gold Cave rooms 4, 5 and 6 add
source-audited extents and real door-loader checks. Unknown resources, title/
intro, equipment/save and unqualified scripted layouts retain fallback.
See [widescreen details](docs/WIDESCREEN.md) and the fullscreen captures.

## Presentation and menus

Smooth/bilinear scaling has been removed from Companion. Enhanced original
pixels remains optional, using EPX with sharp HUD sampling. Actual fullscreen
OpenGL readbacks verify the widened Items and Magic frames at 1920×1080.

The new optional Faster menus setting reduces only the proven Items/Magic entry
fade waits. It saves 60 fields, about one second, with the original asset decode/
upload interval intact. It also works when camera/recovery options are off.
This opt-in timing change intentionally differs from stock transition timing.
See [menu timing](docs/FAST-MENUS.md).

## Save states

New saves include live runtime CPU/bridge execution, beam resume cursor and PPU
ports/latches. Both same-process replay and cold-process replay match all seven
state/metadata files. Five malformed/incomplete-state cases reject before guest
mutation and match an equal-frame no-load control. Failed F1 loads no longer
reset the running session. Earlier incomplete test states are refused; start a
fresh game and save with this build. No checkpoint-specific recovery is carried.
Optional mid-hit/jump-hold timer serialization remains open.
See [state format and reproduction](docs/SAVE-STATES.md).

## Build and publication boundary

Qualified executable SHA256:
`14be32d86a553a27f47971bf5479f9900d225c832f6ee5dc016b870d1423a507`

Manifest SHA256:
`c58f63a1b811411f399a6b13716d602865b002a12ac4ce06e8ef1f8921c23023`

The cumulative framework patch reconstructs 37 files against upstream
`a6be9756285f26ccc74539c811d4d23d57d8c33d`, retaining the pinned public gitlink.
Reconstruction matches local source modulo checkout line endings. WRAM read/
write, precise timing, JSL suspension, QoL, menu layout and widescreen actor/
PPU/DMA tests pass. ROM, generated C, audio, saves and raw proof captures remain
private. Curated fullscreen screenshots are the explicitly requested exception.
