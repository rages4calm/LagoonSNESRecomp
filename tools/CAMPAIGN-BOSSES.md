# Accelerated boss coverage, October 17, 2026

Seven static boss encounters now have captured real sword damage, death, original rewards and completion flags. Separate fixtures capture Nasir dying to each encounter. The six Thimale guardians also exercise the previously untested nonlocal HP/death escape.

These are explicitly assisted fixtures, not a naturally completed campaign. They run the game's actual room loader, AI, collision, HP subtraction and death/reward code while retaining the interpreter safety net. They do not establish independent-console parity, complete native compilation, visible rendering quality or frame-rate performance.

## Frozen build and setup

Executable: `5b17f7704f6b8ea8eb2231f9addd68b5582ef917c37f7a6d51cc69540f1262bc` (`lagoon-campaign-door.exe`). Program manifest: `b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`, containing 190 AOT nodes and 18 declared LLE nodes. This is the campaign exploration build, not a new native promotion receipt.

All runs use copied private save roots and script-only headless input. The fixture substitutes a genuine existing ROM door descriptor only after the original Atland door check succeeds; the ordinary loader creates the destination actors. The earlier attempt to redirect directly at the door-check entry was rejected after corrupt execution and is excluded.

Kill fixtures explicitly select level 35, sword/armor/shield 5, player STR 8191 and DEF 1000. They refresh player HP to 255, the original maximum at level 35. Teleports, facing writes and short position holds align real sword attacks. These are cheats. No enemy HP, enemy phase, kill counter, reward or victory flag is written. Death fixtures instead set Nasir to 1 HP and 0 DEF after entry and allow real enemy damage to reduce HP to zero.

The accepted kills enable `sword_reach`; the Thimale guardian/finish fixtures also enable `camera_center`. Their coverage headers correctly identify the modified runtime ROM, which differs from the stock-ROM hash. Do not normalize those identities or ingest them as stock captures. Source ROM and program manifest identities remain recorded separately.

## Actual boss results

Frames below are the first captured observation, not a claim of the exact intervening CPU cycle.

| Boss / room | Observed HP | First zero frame | Original gold / EXP | Original completion |
| --- | --- | ---: | ---: | --- |
| Samson / 40 | 96 to 0 | 6975 | 100 / 64 | 04E0 mask 80 |
| Natela / 41 | 100 to 0 | 6536 | 500 / 250 | 04E0 mask 40 |
| Eardon / 42 | 150 to 0 | 8529 | 2800 / 500 | 04E0 mask 20 |
| Duma / 43 | 240 to 0 | 7400 | 300 / 800 | 04E0 mask 10 |
| Thimale / 44 | 256 to 0 | 12597 | 1000 / 1500 | 04E0 mask 08 |
| Battler / 45 | 256 to 0 | 7976 | 1000 / 3000 | 04E0 mask 04 |
| Ella / 48 | 256 to 0 | 7415 | 1000 / 2000 | 04E1 mask 80 |

Room and flag values are hexadecimal. Original gold starts at 100; the table lists the actual increase. Eardon's reward precedes its completion bit: the full MP refill and original post-boss dialogue must finish. Merely observing the actor disappear was not accepted as victory.

Accepted private capture names are `boss-40-kill-r2`, `boss-41-kill-r2`, `boss-42-kill-r4`, `boss-43-kill-r4`, `boss-44-finish-r1`, `boss-45-kill-r3` and `boss-48-kill-r2`. Thimale's finish resumes a copied state made after the real six-guardian sequence.

## Thimale's six guardians

`boss-44-guardians-r1` follows all six original type-2/ID-24 guardians at their original positions. Each loses its original 40 HP through sword collision. The original 03AF counter advances through 1, 2, 3, 4, 5 and 6. Only then does original motion code at 02918A change Thimale's 056C phase from 0 to 1. Neither counter nor phase is seeded.

Coverage records exactly six executions each of 0192AE, 0192AF and 0192BF: both original PLA instructions and the counter increment inside the exceptional 0192A0 escape. This is actual encounter coverage of the previously source-reasoned nonlocal return. Its interpreted tail remains intact; these runs do not promote it or remove the interpreter floor at 0080E6.

## Actual player deaths and limits

The seven `boss-XX-playerdeath-r1` captures begin the vulnerable portion at 1 HP and then observe actual zero HP and the original death state. First zero observations are Samson 6879, Natela 6847, Eardon 6847, Duma 7359, Thimale 6847, Battler 7039 and Ella 6911. No script writes zero HP or a death flag.

Every accepted capture has a complete coverage checkpoint, no tuple overflow, no dropped cost samples and no journal write failure. Private `static-boss-acceptance.json` and `campaign_boss_acceptance.py` bind route hashes, setup writes, build identities, HP observations, rewards and flags under `work-campaign-20261017-155739`.

Rejected 32767-HP setup runs overflowed the game's HUD assumptions and are excluded. DEF 32767 also overflows the original damage arithmetic; DEF 1000 avoids that. One-field direction presses can miss the game's alternating update, and airborne facing is locked; accepted targeted attacks verify or explicitly select facing. Duma is only vulnerable during its original animation phase. Components such as Duma's hands and Battler's eggs start at zero HP by design; their zero values are not independent kills.

The endgame shell, Evil Spirit, Zerah, Thor, orbs, bird transformation and ending have separate receipts. Nothing here claims their completion or a complete campaign. Boss names follow the [TASVideos Lagoon research](https://tasvideos.org/GameResources/SNES/Lagoon); local ROM tables independently supply the stats, actor IDs and initialization paths.

## Canonical stock-ROM escape capture

A separate `boss-44-guardians-stock-r3` run disables every QoL feature and retains the exact stock ROM identity `46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb`. It independently kills all six original guardians through sword hits, advances 03AF through 1 to 6, and lets the original code unlock Thimale phase 1 while its HP remains 256. Nasir remains alive.

This fixture uses a 20-pixel attack offset, explicit facing, the same declared player stat cheats, and player Y holds with real Up input. Stock camera code at 0183D8 requires an actual movement direction in 0519; merely teleporting and waiting does not move it. The Up input runs the original camera and strip loader before each attack. No camera register, ROM operand, enemy health or counter is written.

Its stock-ROM coverage independently records exactly six M1X0 executions of each 0192AE, 0192AF and 0192BF, with a complete checkpoint and zero overflow, drop or journal-failure counters. `canonical-guardian-acceptance.json` retains the exact identity, setup writes and observations. Earlier stock trials only reached four visible guardians and are excluded from the six-guardian result.
