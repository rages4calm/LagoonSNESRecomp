# Product
<!-- impeccable:product-schema 1 -->

## Platform
Native desktop game, custom SDL framebuffer UI on Windows, Linux and macOS.

## Users and purpose
Players of Lagoon who want the original story and artwork with clearer navigation,
less backtracking and modern controls. The owner completed a full campaign and
approved the scenery-map and readable extra-menu direction before implementation.

## Capabilities and constraints
The main game CPU runs natively; retain the explicit reference interpreter and
hardware runtime. Extra menus pause simulation and must not change quests, combat,
inventory or saved progress by rendering. Use the player's own ROM for scenery;
never distribute extracted map images. Preserve existing achievement progress,
audio preferences, exploration persistence and keyboard/controller access.

## Current work
Full-zone scenery map with pan/zoom/recenter and useful markers; coordinated map,
achievement and audio menus. Approved subsequent QoL: journal, item explanations,
guarded dungeon return and optional sword-range cue. Test affected routes only.

## Brand commitments
Keep Lagoon's original scenery and recognizable black dialogue boxes with light
frames. The owner's Zelda reference informs map navigation, not replacement art.
No whole-game retexture. Readable lettering takes priority over copying old fonts.
