# Endgame encounter qualification

Six endgame forms were exercised with explicit cheats through original combat,
death and actor-creation routines. This is encounter coverage, not a naturally
completed campaign or proof that every room/path is correct. The interpreter
safety net remains active; these runs do not claim additional native promotions.

The selected immutable executable for this evidence is
`lagoon-campaign-door.exe`, SHA256
`5b17f7704f6b8ea8eb2231f9addd68b5582ef917c37f7a6d51cc69540f1262bc`.
Its190-AOT manifest SHA256 is
`b8720505d6b6b1f57c04b9dbac7abeba56c3e02f3f99485801dc416be78f934e`.
No optional QoL features were enabled. The original, unheadered Lagoon USA ROM
SHA256 was `46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb`;
this evidence does not qualify a BPS-patched ROM or different revision.

## Observed outcomes

Frame numbers below are the game's recorded frame counter after loading copied
private states. Independent fixtures have independent timelines.

| Form | Actual observations |
|---|---|
| Beast Zerah, type2/ID26hex | HP500 decreased through real sword hits to0 at frame10522. Original death logic then created Thor. |
| Thor, type3/ID0B | HP256→72 sampled; original death handler `$01:9382` entered at12995 and `$01:9392` called actor initializer `$01:8DC6` at13001. Bird creation and removal of the orbs followed. A transient Thor HP0 was **not directly sampled**. |
| Eight Thor orbs, type3/ID0D | All eight original actors simultaneously present by11623, following Zerah's actual death. |
| Thor Bird, type3/ID0C | First sampled HP145 after creation, then28→21→0 at16065. Original completion bit `$04E0 & 01` appeared16359; NPC6 replaced the boss. |
| Spirit Shell, type2/ID25hex | HP1280 decreased through repeated hits to0 at7307. Original shell removal followed; Green Spirit became vulnerable. |
| Green Evil Spirit, type3/ID0A | HP256→198→148→42→8→0 at7937. Original completion bit `$04E0 & 02` appeared8441 and the boss became NPC5. |
| Red Evil Spirit, type3/ID0F | HP256→193→70→0 at7016. Original ending handler `$01:B90B` entered7570. This proves reaching that handler, not a separately audited full credits sequence. |

The earned Green completion state also traversed the real room46 north door
into room47, preserving completion bit02 and initializing an actual500HP Zerah.
The entire Green→Zerah→Thor→Bird→Red sequence was not rerun as one uninterrupted
fixture. Red's independent entry explicitly seeded prerequisite bits03 before
the original room loader. That seed is setup, not evidence of either earlier
victory; the earlier victories have their separate actual combat evidence above.

## Cheats and evidence boundaries

Test setup included player level35, STR4095, DEF1000, sword selector5, player HP
restoration to the level's255HP maximum, and player position/facing staging.
The Shell/Green fixture fixed player XY and facing to permit a sword hit before
contact knockback moved Nasir away. Enemy HP, IDs, death states and victory
outcomes were not written by the accepted kill routes.

Entry used the validated real-door destination substitution described in
[CAMPAIGN-WARPS.md](CAMPAIGN-WARPS.md). Initial states were copied into new private
save roots. All runs used script-only input and headless execution. No owner save
was an output destination, and no visible test window was opened.

The private `endgame-acceptance.json` binds five accepted runs, their frozen
routes, save-input hashes, executable/manifest identities and actual actor
checkpoints. It explicitly distinguishes sampled HP0 from Thor's observed death
handler and replacement. All five raw coverage files reported zero tuple
overflow, journal write failures, RAM-routine overflow and dropped cost samples;
each marked its checkpoint complete. These counters describe retained diagnostic
evidence, not independent-emulator parity or full-game certification.

Missed positioning attempts and the earlier direct-PC-redirect experiments are
excluded. The final source keeps the original loader, gameplay and interpreter
fallback. Private ROMs, states, dumps, replay plans and raw captures are not part
of this document or the public source tree.
