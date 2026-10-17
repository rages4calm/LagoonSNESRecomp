# Accelerated campaign coverage — October 17, 2026

Using the supplied [walkthrough](https://mikesrpgcenter.com/lagoon/walkthrough.html)
as an encounter checklist, this batch trades travel and grinding for explicit
test setup, while preserving actual loading, combat, death and reward code.
It does not claim an uninterrupted natural playthrough or a fully native game.

## Actual results

| Coverage | Accepted result |
| --- | --- |
| Room entry | All 73 rooms with ordinary incoming doors load and settle |
| Main bosses | Seven real defeats, original rewards and completion flags |
| Player deaths | Real enemy damage kills Nasir in seven separate boss fixtures |
| Thimale guardians | Six actual kills unlock the boss; six nonlocal death escapes verified on stock ROM |
| Endgame | Shell, Green Spirit, Zerah, Thor, Bird and Red Spirit exercise original defeat/creation paths |
| Progression | Eight Thor orbs, earned Green/Bird flags, genuine Green-to-Zerah door, and ending-handler entry |

No accepted kill writes enemy HP, a death outcome, reward or victory flag.
Player teleports, high stats, HP restoration, facing and some prerequisite
flags are disclosed. Thor's defeat is witnessed through its death handler and
the original bird initializer; transient zero HP was not directly sampled.
Red's independent entry seeds prerequisite flags, rather than claiming a
continuous campaign. Credits and the script-only cloud entry remain open.

## Coverage ingestion and native candidate

The final canonical bundle contains 79 complete, identity-bound stock-ROM
captures: 73 room runs, six-guardian coverage and five endgame captures. It
records 824,526,719 interpreted instructions, 2,786,778,153 interpreted guest
cycles, 9,561 PC/mode cost variants and 641 transfer variants. SDK audit reports
no loss, overflow, duplicate records or warnings. Rooms 2A and 3F remain explicit
gaps. Feature-patched static-boss captures are valid separate evidence and are
not relabeled or merged into the canonical ROM identity.

An earlier room-only bundle was actually ingested, generated and compiled into
a private 348-AOT program. Camera and interpreter seams remain guarded. A small
framework patch adds exact `PC:M0X0`-style timing selectors alongside legacy
plain-PC selectors, preserving old timing modes and byte-identical no-profile
generation. The cumulative patch reconstructs all 38 paths from pinned upstream.

That expanded native program is **not selected**. Five bounded probes expose
strict state/register/clock differences, and Ella has one different ordered
presentation. The playable program retains **190 AOT / 18 declared LLE variants**
and its interpreter safety net. More generated variants are not accepted as
progress without runtime qualification.

## Selected runtime and validation

Executable SHA256:
`5b17f7704f6b8ea8eb2231f9addd68b5582ef917c37f7a6d51cc69540f1262bc`.
Program manifest SHA256:
`b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`.

With the campaign hook disabled, the original first-boss control matches at 100
checkpoints and all 4,521 ordered presentations, including memory and clocks.
Twenty-two campaign collector/harness tests, six SDK timing tests and the
production C door-guard test pass. Private generated programs, ROMs, states,
audio and raw captures remain outside the public tree. No visible accelerated
test window was opened, and owner saves/settings were not changed.

The custom renderer, Enhanced pixels, widescreen and state format are unchanged
from their existing coverage. The debug warp facility is inert in ordinary play.
An optional in-game map remains on TODO; this batch prioritized encounter
coverage and the new native candidate's concrete regressions.

Evidence details: [main bosses](tools/CAMPAIGN-BOSSES.md),
[endgame](tools/CAMPAIGN-ENDGAME.md), [warp contract](tools/CAMPAIGN-WARPS.md),
[coverage workflow](tools/CAMPAIGN-COVERAGE.md), and
[resume priorities](docs/DEVELOPMENT-STATUS.md).
