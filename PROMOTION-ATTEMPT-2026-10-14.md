# Gameplay promotion attempt, the clock gate, and two corrections — 2026-10-14

Companion to `FIX-TITLE-LOOP-2026-10-14.md`. That file's fix is correct and is
not changed here. This records what a follow-up session measured, one promotion
attempt that failed the gate and why, and two claims in the earlier receipt that
needed correcting.

## The opening fix verifies independently

Fresh cards, headless (`SDL_AUDIODRIVER=dummy`), `tools/script_gameplay_regression.txt`,
6 240 frames, corrected candidate versus the genuine `recomp-floor` build:

- present streams: **0 differing frames of 6 240**, both 6 240 lines long;
- all eight dump tags identical on framebuffer, WRAM, VRAM, OAM, CGRAM and
  register snapshot — including `town`, which the earlier receipt recorded as
  differing at `$7E:01BD`;
- screenshots confirm real gameplay: town at 6134, moved down at 6165, Start
  menu at 6202 (`LV 1 GOLD 100 / HP 10/10 STR 10 / MP 3/3 DEF 5`), menu closed
  by B at 6245 (`build/dumps-qreg-aot/sheet.png`).

The route's last `dump` lands exactly on the frame limit, so a run needs
`-Frames 6245` to capture it. Comparisons must use equal frame counts: unequal
runs report a missing tag, not a divergence.

## Correction 1 — the 193 differing frames were audio pacing

`build/investigation/gameplay-fixed.log` and `gameplay-floor.log` each print
`audio device opened: freq=32000`: both sides were paced by a real audio device.
An audio callback paces the guest, and two untouched audio-paced runs of this
port diverge from each other (measured independently: first difference at frame
1128 with no input at all). Two dummy-audio runs do not diverge at all.

So "193 differing presentations out of 6 239" and the stale stack byte at
`$7E:01BD` were properties of audio pacing, not of ahead-of-time versus
interpreter. `tools/headless.ps1` now has `-AudioDriver` so this is testable
instead of argued about. Compare dummy-audio to dummy-audio, or audio-paced to
audio-paced, never across.

## Correction 2 — there is no vertical placement fault

Mesen's `--screenshot` is 256×239 (it includes 7 blanking lines); a framedump
framebuffer is the 256×224 active field. Laying them side by side therefore
*creates* a vertical shift. Cross-correlating every offset, measured twice, on
the retained artifact and on a fresh run of the same route:

    best offsets (mean abs difference, offset): (0.46, 7) (27.9, 8) (28.3, 6) (31.6, 9)

The port's field equals Mesen's rows 7..230 at 0.46 mean absolute difference.
What genuinely remains at frame 6165: 31 differing rows confined to 127..223,
which is the sprite and bottom-HUD band, and 123 differing WRAM bytes with 45 of
them off by one, concentrated in page-zero scratch, the stack page, and a table
at `$2442`. That is a small state/timing phase difference, not a PPU placement
difference. The real remaining work is timing alignment.

Also fixed in the harness: Mesen numbers controller ports from 0, so the first
`--input 1:A@...` harness was pressing SNES controller 2 — the same one-player
mistake the port itself had, now inside the comparison tool. `-Port` defaults to
0 and the reason is written into the script.

## The promotion attempt, and the gate it failed

`$01:C579` was the first candidate taken from gameplay rather than the sound
path: 12 775 interpreter entries from `JSR $C579` at `$01:C4DE`, arriving M0X0,
body a clean `PHY/PHX` … `PLX/PLY/RTS` leaf copying 16 words from `$118000,x`
into page-zero `$40,y`. Proved by disassembly (`tools/dis.py`), not guessed.

Compiling it worked as a mechanism: interpreted work on the opening route fell
from 61 241 252 to 58 456 377 instructions (−2 784 875 — 12 775 calls × ~218),
proving the interpreter hands control back to ahead-of-time code when it reaches
a compiled entry, which until now had only been assumed. The framebuffer stayed
bit-identical for 6 240 frames.

It still failed, on the guest clock. The dumps record `master_clock`, `vpos` and
`hpos`, and they disagree with the floor's from the first frames the routine
runs — `-8` dots at frame 3187, `+24` at 6134, `+8` at 9081, `+14` at 15227. On
the 21 121-frame route the port forked visibly at frame **4320**, 16 486 of
21 122 presents differing and WRAM differences in the tens of thousands.

The mechanism is in the generated code, and it is not a semantic bug: every
block carries one static cost (`cpu->cycles += 43` for the entry, 30 for the
loop block, 16 for the exit — 539 per call), while the interpreter charges
long-indexed address-transit penalties as they actually occur. The sign is not
monotonic, which is what a sporadic penalty looks like and not what a wrong
constant looks like. A routine can be instruction-exact and a few dots late on
every frame it runs, and a timing-sensitive scene will amplify that into a
different game.

So `F_01C579` is declared with `emit = false` and the whole argument in
`recomp/symbols.toml`. Rebuilt without it, the binary matches the accepted build
on every component of all eight tags including `clock=ok`; its size differs
(8 747 520 versus 8 745 984 bytes) because symbols.toml now declares the entry.
The shipped build is behaviourally the one the earlier receipt qualified.

**Rule this leaves behind: a gameplay routine may not be promoted until the
ahead-of-time cost model matches the interpreter's dynamic penalties.** The
cost is where to look: `LDA $118000,x` (long absolute indexed) inside a loop
whose index moves by 2, and `STA $40,y` over the same page.

## Gate tightening

`tools/dump_compare.py` gained a `clock` column and counts a mismatch. On the
rejected build it reports, at a frame where every other component said `ok`:

    c-3000  fb.bmp=ok  wram.bin=ok  regs.json=ok  vram.bin=ok  oam.bin=ok  cgram.bin=ok  clock=DIFF(mclk 1138881362/1138881354 delta=-8)

That is the class of fault that used to pass. Nothing was loosened.

## New tools

- `tools/compare_mesen.py` — port dump versus Mesen screenshot at matched frames,
  with a WRAM byte count per row. Run with `python -P`: `tools/dis.py` shadows
  the stdlib `dis` module, so plain `python tools/...` breaks any tool that
  imports PIL.
- `tools/tag_sheet.py` — a run's named dumps as one labelled sheet, each row
  stamped with the frame it came from, so a screenshot cannot be quoted without
  its moment.
- `tools/mesen_reference.ps1` — gained `-Port`, `-Timeout` (the agent's default
  30 s stops at ~1 800 frames and reports `ok:false, reason:timeout`, which
  silently compares two unrelated moments) and a per-tag Mesen home so runs can
  go at once.
- `tools/headless.ps1 -AudioDriver`.

## What is not claimed here

No campaign coverage, no widescreen, no audio qualification, no release. The
interpreter still carries gameplay: 61 241 252 instructions on the opening
route with `tier hits=0`, and `$80E6` was observed dispatching **12** distinct
targets on a gameplay route (`build/coverage-qreg-cov`) where the old cfg
authorized 2 — so `force_lle 0080e6` remains correct and its target set is still
unproven. `$02:81DE` (16 731 entries, `JSL` from `$01:80CD`, common exit `RTL`
at `$02:8255`) is the next candidate and additionally needs the three-byte
remote frame proved.
