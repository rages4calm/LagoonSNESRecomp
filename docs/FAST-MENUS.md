# Faster Items and Magic fades

`lagoon.quality:fast_menus` is optional and defaults off. It changes only the
three-frame brightness steps used by the proven Items/Magic submenu entry
fade-out and fade-in to one frame each. A full pair uses 30 fade frames instead
of 90. It does not remove resource decoding, VRAM queues, world or introduction
fades, or accelerate unrelated menus. Actual runtime qualification belongs to the
combined native candidate; standalone tests do not establish a measured release.

The original Items route spends about 61 additional blank fields doing active
resource work: resources 7, 8 and 6 pass through `0094A1`, the `009143` decoder,
and real VRAM queues. These operations remain intact. This option can remove
roughly one second of the proven fade pair; it does not make loading instant.

## Exact authorization

`LagoonFastMenus_Reset(rom,size)` disables the feature and validates the timer
helper and six fade/caller instruction sites. Plugin activation enables it only
after those checks pass. The parent calls `LagoonFastMenus_Filter` from the WRAM
write hook with the scoped interpreter context and live RAM.

Only a value-three write to canonical WRAM `023C` at instruction `0084E0`, in
native CPU mode M1/X0/E0 and an actual interpreter scope, qualifies. Two checked
guest JSL return frames must identify the matching fade and menu caller:

| Direction | Inner return | Items outer return | Magic outer return |
|---|---|---|---|
| Fade out | `0083D8` | `02B6B9` | `02B7E3` |
| Fade in | `008471` | `02B6E6` | `02B817` |

Wrong pairs, unknown modes/stacks, native cursors, mismatched ROM bytes, other
addresses/values and disabled activation pass through. The module never writes
RAM directly. Its constructor registers only the activation plugin. It does not
replace the existing write hook, decoder, interrupt handling or interpreter.

ROM-free unit tests use synthetic instruction-site guards, exercise all four
contexts and refusals, confirm immutable RAM and 45-to-15 timer ticks per fade,
and verify reset disables activation. Private combined-run fixtures separately
sample brightness and end states with fast menus off/on.

A further integrated check enabled **only** `fast_menus`, with camera and recovery
features disabled. Items reached full brightness at field 1919 instead of 1979:
60 fields earlier. Forced-blank loading remained 61 fields in both runs. The
complete framebuffer, WRAM, registers, VRAM, OAM and CGRAM matched at town,
settled Items and return checkpoints. Guest clock phase differed by 6 and 12
master ticks at the latter checkpoints, so exact clock parity is not claimed
between feature modes. This confirms the scoped context is available when fast
menus is the only optional feature. The private integrated receipt records all
469 snapshots in each run and the empty/off versus single-feature/on catalogs.
