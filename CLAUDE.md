# MyWorld — Project Guide

## Overview

MyWorld is a 2D sandbox game inspired by Minecraft, written in pure C with raylib 5.5. **All assets (textures, sounds, music) are procedurally generated at runtime** — no external image/audio files are used.

- **Platform**: Windows (Win32 APIs for input isolation)
- **Language**: C (compiled as C, linked with g++ for raylib)
- **Engine**: raylib 5.5 (headers in `include/`, static lib in `lib/`)
- **World**: 2048x256 blocks, 16px per block, chunk-based rendering (16 blocks/chunk)
- **Multiplayer**: UDP host/client, up to 4 players, authoritative host model

## Build System

### Build Commands
```bash
# VS Code / GNU Make
make

# Red Panda C++ IDE
make -f makefile.win

# Clean
make clean
```

### Build Toolchain
- **Compiler**: gcc (C files), g++ (linking)
- **Resource compiler**: windres (for .rc → .res)
- **Link flags**: `-lraylib -lopengl32 -lgdi32 -lwinmm -lws2_32 -static`
- **Stack size**: 12MB (`-Wl,--stack,12582912`)
- **Charset**: UTF-8 input/exec (`-finput-charset=UTF-8 -fexec-charset=UTF-8`)

### raylib Setup
Place raylib 5.5 files in:
```
include/raylib.h, raymath.h, rlgl.h
lib/libraylib.a
```

## Architecture

### File Map

| File | Lines | Responsibility |
|------|-------|----------------|
| `types.h` | ~1333 | **Central header** — ALL constants, enums, structs, extern globals, function declarations |
| `main.c` | ~429 | Entry point, Win32 input override, audio loading thread, game loop |
| `game.c` | ~1664 | Game state machine, main update/draw dispatch, settings, multiplayer sync |
| `world.c` | ~2752 | World generation (11 passes), block atlas, chunk system, redstone BFS, gravity |
| `player.c` | ~1203 | Player physics, inventory, mining, combat, armor, status effects |
| `rendering.c` | ~3919 | All rendering: world, player, UI, menus, inventory, minimap, map |
| `crafting.c` | ~629 | Crafting/smelting recipes, furnace UI, crafting panel with search |
| `mob.c` | ~1322 | 7 mob types with AI state machines, spawning, combat |
| `light.c` | ~163 | BFS light propagation, sunlight, torch/lantern/redstone lamp light |
| `sound.c` | ~751 | Procedural sound synthesis via wave generators, BGM generation |
| `i18n.c` | ~1322 | 3 languages (EN/ZH/JA), ~180+ string IDs, CJK font support |
| `save.c` | ~350 | Binary save format v8 with RLE compression, version migration |
| `net.c` + `net.h` | ~500 | UDP multiplayer, host/client, block sync, player state sync |
| `noise.c` | ~50 | hash2D, valueNoise, fbm — world generation noise functions |
| `daynight.c` | ~67 | Day/night cycle, sky color |
| `weather.c` | ~150 | Rain/thunder weather system |
| `particles.c` | ~150 | Particle effects (block break, damage, sprint dust, bubbles) |
| `entities.c` | ~150 | Item entity physics, pickup |

### Key Design Patterns

**Single central header**: `types.h` contains ALL type definitions, constants, extern declarations, and function prototypes. Every `.c` file includes only `types.h`. There are no per-module headers except `net.h` (for network structs).

**Parallel arrays for entities**: Mobs, projectiles, particles, and item entities use fixed-size parallel arrays with an `active` flag, NOT linked lists or dynamic allocation.

**Global state**: Game state is stored in global variables declared `extern` in `types.h` and defined in the appropriate `.c` file. The `player` macro expands to `players[localPlayerId]`.

**Win32 input override**: All input goes through `Win32IsKeyPressed()`, `Win32IsKeyDown()`, etc. which use `GetAsyncKeyState()` directly. This bypasses GLFW's broken input on some systems and isolates input between multiple game instances.

**Procedural generation**: All textures are drawn pixel-by-pixel in `DrawBlockPattern()` into a shared atlas Image. All sounds use callback-based wave generators registered in `InitSounds()`.

### Key Systems

**World Generation** (world.c `GenerateWorld()`):
11 passes: bedrock → stone → ores → caves → dirt → surface → sand → water → trees → biomes → structures (dungeons).

**Light System** (light.c):
BFS propagation from sunlight (top-down) and point sources (torches, lanterns, redstone lamps). Light levels 0-15.

**Redstone System** (world.c):
BFS signal propagation from power sources (levers, pressure plates). Power decays by 1 per wire block, max 15. Lamps emit light level 12 when powered.

**Mob AI** (mob.c):
7 types with state machines: Pig (wander), Zombie (chase), Skeleton (ranged), Creeper (fuse/explode), Spider (wall-climb), Slime (hop/split), Enderman (teleport). Despawn after 5 minutes if player is >200px away.

**Save Format** (save.c):
Binary: `"MWSV"` magic + version(u32) + seed(u32) + worldW(u32) + worldH(u32) + DayNight + Player + Chest data + RLE-compressed world columns. Current version: 8.

**Multiplayer** (net.c):
UDP sockets, host is authoritative. Clients send input, host broadcasts state. Block changes synced via `ModifiedBlock` array (max 16384).

## Coding Conventions

### Style
- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**: camelCase for functions and variables, UPPER_SNAKE for constants/macros
- **Types**: Use `stdint.h` types (`uint8_t`, `int32_t`, etc.) for data that crosses file boundaries
- **Booleans**: `stdbool.h` — use `bool`, `true`, `false`
- **Strings**: All user-facing text goes through `S(STR_ID)` for i18n support

### Adding a New Block
1. Add entry to `BlockType` enum in `types.h` (before `BLOCK_COUNT`)
2. Add `blockInfo[]` entry in `world.c` (name, colors, solid/transparent/breakable)
3. Add pixel art in `DrawBlockPattern()` in `world.c`
4. Add `StringId` entry in `types.h` (after existing block names)
5. Add translations in `i18n.c` (EN, ZH, JA tables)
6. Add crafting recipe in `crafting.c` if craftable
7. If light source: update `light.c` (IsTransparent, RemoveLight, RecalculateAllLight)
8. If redstone component: update redstone functions in `world.c`

### Adding a New Mob
1. Add entry to `MobType` enum in `types.h`
2. Add spawn logic in `SpawnMob()` in `mob.c`
3. Add AI behavior in `UpdateMobs()` in `mob.c`
4. Add drawing in `DrawMobs()` in `mob.c`
5. Add sound in `sound.c` (new generator + register in `InitSounds()`)
6. Add death message `StringId` in `types.h` and translations in `i18n.c`

### Adding a New Sound
1. Create a `static float xxxGen(float t, float freq, unsigned int *rng)` callback in `sound.c`
2. Add `Sound sndXxx;` global in `main.c` and `extern` in `types.h`
3. Register in `InitSounds()`: `sndXxx = GenerateWave("xxx", duration, volume, xxxGen);`
4. Free in `UnloadSounds()`: `UnloadSound(sndXxx);`
5. Add `PlaySoundXxx()` wrapper function

### Adding i18n Strings
1. Add `STR_XXX` to `StringId` enum in `types.h`
2. Add string to all 3 language tables in `i18n.c` (EN, ZH, JA)
3. Use `S(STR_XXX)` in code

## Known Technical Debt

### Performance
- `UpdateRedstoneTick()` scans all 524K blocks per frame — needs pressure plate list optimization
- `RemoveLight()` recalculates entire affected area — could be incremental
- `RecordBlockChange()` in net.c uses linear search — needs hash table

### Code Quality
- `DrawBlockPattern()` in world.c is ~1544 lines — should split by material category
- `UpdateGame()` in game.c is ~716 lines — should split by subsystem
- Ore drop logic duplicated 3x in player.c
- Sword damage logic duplicated 2x in player.c
- UI cleanup code duplicated 10+ times in rendering.c
- Missing `fwrite`/`malloc` return value checks in save.c and world.c
- `wheelAccum` in main.c accessed from both main thread and Win32 hook thread without synchronization

### Architecture
- `types.h` is 1333 lines — could split into per-module headers
- `rendering.c` is 3919 lines — could split into rendering_world.c, rendering_ui.c, rendering_menu.c

## Development Workflow

### Before Committing
1. Build with `make clean && make` to verify no compile errors
2. Launch game and test the changed feature
3. Test save/load if world format changed
4. Test multiplayer if net code changed

### Save File Compatibility
When modifying save format:
1. Increment `SAVE_VERSION` in `types.h`
2. Add migration code in `LoadWorld()` to handle old versions
3. Old saves must continue to load correctly

### Multiplayer Protocol
When modifying net code:
1. Block changes use `NetSyncBlockChange()` / `NetBroadcastBlockChange()`
2. Player state is sent every frame as raw struct
3. Host is authoritative — client inputs are applied on host side

## Constants Reference

| Constant | Value | Description |
|----------|-------|-------------|
| `WORLD_WIDTH` | 2048 | World width in blocks |
| `WORLD_HEIGHT` | 256 | World height in blocks |
| `BLOCK_SIZE` | 16 | Pixels per block |
| `CHUNK_SIZE` | 16 | Blocks per chunk |
| `MAX_MOBS` | 32 | Max simultaneous mobs |
| `MAX_ENTITIES` | 128 | Max item entities |
| `MAX_PARTICLES` | 256 | Max particles |
| `MAX_PROJECTILES` | 32 | Max projectiles |
| `MAX_NET_PLAYERS` | 4 | Max multiplayer players |
| `MAX_LIGHT_LEVEL` | 15 | Max light level |
| `SAVE_VERSION` | 8 | Current save format version |
| `MAX_MODIFIED_BLOCKS` | 16384 | Max tracked block changes for multiplayer sync |
