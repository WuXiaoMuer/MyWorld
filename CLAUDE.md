# MyWorld — 2D Minecraft Sandbox (C + raylib 5.5)

## Build
```bash
make clean && make   # gcc/g++, raylib static, Windows only
```
raylib 5.5 → `include/` + `lib/libraylib.a`

## Architecture
- **types.h** — ALL constants, enums, structs, extern globals, function declarations
- Every `.c` includes only `types.h` (except `net.h` for network structs)
- Parallel arrays for entities (mobs, projectiles, particles) with `active` flag
- `player` macro → `players[localPlayerId]`
- Win32 input override: `Win32IsKeyPressed()` etc. via `GetAsyncKeyState()`
- All textures/sounds procedurally generated at runtime

## File Map
| File | ~Lines | Role |
|------|--------|------|
| types.h | 1378 | Central header |
| main.c | 432 | Entry, Win32 input, audio thread |
| game.c | 1668 | State machine, update/draw dispatch |
| world.c | 2949 | Generation, blocks, redstone, water flow |
| player.c | 1272 | Physics, inventory, combat, armor |
| rendering.c | 4014 | All rendering (world, UI, menus) |
| crafting.c | 646 | Recipes, furnace UI |
| mob.c | 1296 | 7 mob types, AI, spawning |
| light.c | 168 | BFS light propagation |
| sound.c | 899 | Procedural sounds, BGM, ambient |
| i18n.c | 1352 | EN/ZH/JA, ~200+ strings |
| save.c | 386 | Binary v8, RLE, version migration |
| net.c+net.h | 631 | UDP multiplayer, packet validation |

## Key Systems
- **World**: 2048×256 blocks, 11-pass generation, chunk rendering
- **Redstone**: BFS from levers/pressure plates, power decay, lamps
- **Water**: BFS flow from sources, level 1-7, water bucket
- **Light**: BFS sunlight + point sources, levels 0-15
- **Mobs**: 7 types (Pig/Zombie/Skeleton/Creeper/Spider/Slime/Enderman)
- **Difficulty**: Peaceful/Easy/Normal/Hard (affects spawns, damage, hunger)
- **Ambient**: Cave drips/rumble underground, wind on surface
- **Multiplayer**: UDP host/client, 4 players, packet validation
- **Save**: Binary v8, RLE compression, version migration

## Conventions
- 4 spaces, K&R braces, camelCase, UPPER_SNAKE constants
- User text via `S(STR_ID)` for i18n
- New block: enum → blockInfo → DrawBlockPattern → StringId → i18n → recipe
- New mob: enum → spawn → AI → draw → sound → death message
- New sound: generator → globals → InitSounds → wrapper
- New string: StringId → 3 language tables → `S()`

## Constants
| Name | Value | Name | Value |
|------|-------|------|-------|
| WORLD_WIDTH | 2048 | MAX_MOBS | 32 |
| WORLD_HEIGHT | 256 | MAX_ENTITIES | 128 |
| BLOCK_SIZE | 16 | MAX_LIGHT_LEVEL | 15 |
| CHUNK_SIZE | 16 | SAVE_VERSION | 8 |
