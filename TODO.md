# MyWorld — Steam Release TODO

## Fatal Bugs Fixed

- [x] CRITICAL: OOB `world[blockX][blockY-1]` when planting at y=0 (player.c:1569)
- [x] HIGH: `sprintf` on 16-byte stack buffer × 4 (rendering.c)
- [x] HIGH: Unchecked `strcat` in `/list` command (game.c)
- [x] MEDIUM: Unchecked `fwrite` on save trailer (save.c)
- [x] MEDIUM: `strcpy` player name init → `snprintf` (player.c)
- [x] MEDIUM: `selectedSlot` bounds guard (player.c)
- [x] Save atomicity (Windows `MoveFileExA`)
- [x] Save integrity trailer
- [x] Fire Aspect per-mob accumulator
- [x] Reliable packet buffer overflow guard
- [x] Death item drops
- [x] Host-only auto-save
- [x] Trade UI close with E/ESC
- [x] Bed spawn obstruction check
- [x] Passive mobs on snowy grass

## Verified Safe

- `IsBlockSolid()` bounds-checks
- All `fread()` return values checked
- RLE load bounded correctly
- No infinite loops
- TextFormat/Sf() single-threaded OK
- Player/mob physics bounds OK

## Polish TODO

- [ ] Resolution option in settings
- [ ] Creative mode
- [ ] Key rebinding
- [ ] More biomes/mobs
