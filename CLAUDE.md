# MyWorld — 2D Minecraft Sandbox (C + raylib 5.5, Windows)

## Build
```bash
make clean && make   # gcc/g++, raylib 5.5 static (include/ + lib/libraylib.a)
```

## Architecture
- **types.h** — ALL constants, enums, structs, extern globals, function declarations. Every `.c` includes only `types.h` (plus `net.h` for network structs).
- Entities are parallel arrays gated by an `active` flag (mobs, projectiles, particles, items, xp orbs); fixed caps, no dynamic allocation.
- `player` macro → `players[localPlayerId]`; same logic runs local or network-driven (`netControlled`/`moveInput`/`jumpHeld`).
- Win32 input override (`Win32IsKeyPressed()` etc. via `GetAsyncKeyState()`) bypasses broken GLFW input and isolates multi-instance focus.
- All textures/sounds procedurally generated at runtime (no asset files).

## File Map
| File | Role |
|------|------|
| types.h | Central header (constants, enums, structs, decls) |
| main.c | Entry, Win32 input, audio thread |
| game.c | State machine, update/draw dispatch, achievements, save slots |
| world.c | Generation (11-pass), blocks, redstone (BFS), water flow (BFS) |
| player.c | Physics, inventory, combat, armor, enchantments |
| rendering.c | All rendering (world, UI, menus) — largest file |
| crafting.c | Recipes, furnace/smelting UI |
| mob.c | 7 mob types (Pig/Zombie/Skeleton/Creeper/Spider/Slime/Enderman): spawn, AI, draw |
| light.c | BFS sunlight + point lights (levels 0-15) |
| sound.c | Procedural sounds, BGM, ambient |
| i18n.c | EN/ZH/JA string tables (~200+ strings) |
| save.c | Binary save (current SAVE_VERSION), RLE, version migration |
| net.c / net.h | UDP host/client (4 players), packet validation |
| daynight.c weather.c particles.c entities.c noise.c | Smaller subsystems |

## Conventions
- 4 spaces, K&R braces, camelCase, UPPER_SNAKE constants.
- All user-facing text via `S(STR_ID)` — never hardcode strings.
- New block: enum → blockInfo → DrawBlockPattern → StringId → i18n (3 langs) → recipe
- New mob: enum → spawn → AI → draw → sound → death message
- New sound: generator → globals → InitSounds → wrapper
- New string: StringId → 3 language tables → `S()`
- Bump SAVE_VERSION + add migration in save.c when changing save format.

## Key numbers
WORLD 2048×256 · BLOCK_SIZE 16 · CHUNK_SIZE 16 · MAX_MOBS 32 · MAX_ENTITIES 128 · MAX_LIGHT_LEVEL 15. Exact values + all others live in types.h.

## Backlog
See `TODO.md` for the prioritized backlog, known risks, and verified non-issues (don't re-investigate those).
