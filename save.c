#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
__declspec(dllimport) int __stdcall MoveFileExA(const char*, const char*, unsigned long);
#define MOVEFILE_REPLACE_EXISTING 1
#endif

#define SAVE_TRAILER_MAGIC 0x4D59574C  // "MYWL" in little-endian

//----------------------------------------------------------------------------------
// Save File Format v16:
//   Header:      "MWSV" + uint32 version + uint32 seed + uint32 worldW + uint32 worldH
//   DayNight:    float timeOfDay + float daySpeed + float lightLevel
//   Player:      float posX,Y + float velX,Y + bool onGround + int selectedSlot
//                + uint8 inventory[36] + int inventoryCount[36] + int toolDurability[36]
//                + int health + int hunger + int oxygen + int xp
//   Furnace:     multi-furnace data (v5+, v9+ multi)
//   Chests:      count + per-chest data (v6+)
//   Weather:     type + duration (v7+)
//   Achievements: bool[ACH_COUNT] + totalMobsKilled + totalBlocksPlaced (v9+; v9-v13 stored 6)
//   World:       RLE per column: (uint8 block, uint16 count) pairs
//   Modified:    count + per-block x,y,type (v8+)
//   Mobs:        count + per-mob data (v10+)
//   Cauldrons:   count + per-cauldron x,y,fillLevel (v11+)
//   Crops:       uint32 count + per-crop (uint16 x, uint16 y, uint8 growth) (v12+)
//   Fluids:      uint32 count + (uint16 x,y, uint8 block, kind, level, source) (v16+)
//   Dimensions:  uint8 dimensionCount                                        (v17+)
//                Currently always 1. Reserved so adding a second dimension later
//                does not require another save-version bump; readers that predate
//                it simply stop before this field.
//   Trailer:     uint32 SAVE_TRAILER_MAGIC
// NOTE: Cauldrons/Crops/Fluids are written AFTER World+Modified; LoadWorld reads them before the trailer.
//----------------------------------------------------------------------------------

bool SaveExists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

void GetSavePath(int slot, char *buf, int bufSize)
{
    snprintf(buf, bufSize, "%s/world%d.mwsav", SAVE_DIR, slot);
}

bool GetSlotInfo(int slot, SaveSlotInfo *info)
{
    info->exists = false;
    info->seed = 0;
    info->worldW = 0;
    info->worldH = 0;

    char path[256];
    GetSavePath(slot, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4) { fclose(f); return false; }
    if (memcmp(magic, SAVE_MAGIC, 4) != 0) { fclose(f); return false; }

    uint32_t version, seed, ww, wh;
    if (fread(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&seed, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&ww, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&wh, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }

    fclose(f);
    info->exists = true;
    info->seed = seed;
    info->worldW = (int)ww;
    info->worldH = (int)wh;
    return true;
}

void DeleteSaveSlot(int slot)
{
    char path[256];
    GetSavePath(slot, path, sizeof(path));
    remove(path);
}

bool SaveWorld(const char *path)
{
    if (player.playerDead) return false;
    mkdir("saves");

    // Write to tmp first for crash safety
    char tmpPath[256];
    snprintf(tmpPath, sizeof(tmpPath), "%s.tmp", path);
    FILE *f = fopen(tmpPath, "wb");
    if (!f) return false;

    bool ok = true;

    // Header
    ok = ok && fwrite(SAVE_MAGIC, 1, 4, f) == 4;
    uint32_t version = SAVE_VERSION;
    uint32_t seed = worldSeed;
    uint32_t ww = WORLD_WIDTH;
    uint32_t wh = WORLD_HEIGHT;
    ok = ok && fwrite(&version, sizeof(uint32_t), 1, f) == 1;
    ok = ok && fwrite(&seed, sizeof(uint32_t), 1, f) == 1;
    ok = ok && fwrite(&ww, sizeof(uint32_t), 1, f) == 1;
    ok = ok && fwrite(&wh, sizeof(uint32_t), 1, f) == 1;

    // Day/Night
    ok = ok && fwrite(&dayNight.timeOfDay, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&dayNight.daySpeed, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&dayNight.lightLevel, sizeof(float), 1, f) == 1;

    // Player
    ok = ok && fwrite(&player.position.x, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&player.position.y, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&player.velocity.x, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&player.velocity.y, sizeof(float), 1, f) == 1;
    ok = ok && fwrite(&player.onGround, sizeof(bool), 1, f) == 1;
    ok = ok && fwrite(&player.selectedSlot, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(player.inventory, sizeof(uint8_t), INVENTORY_SLOTS, f) == INVENTORY_SLOTS;
    ok = ok && fwrite(player.inventoryCount, sizeof(int), INVENTORY_SLOTS, f) == INVENTORY_SLOTS;
    ok = ok && fwrite(player.toolDurability, sizeof(int), INVENTORY_SLOTS, f) == INVENTORY_SLOTS;
    ok = ok && fwrite(player.itemEnchantments, sizeof(uint16_t), INVENTORY_SLOTS, f) == INVENTORY_SLOTS;
    ok = ok && fwrite(&player.health, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&player.hunger, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&player.oxygen, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&player.xp, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&player.spawnX, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&player.spawnY, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(player.armor, sizeof(uint8_t), 4, f) == 4;
    ok = ok && fwrite(player.armorDurability, sizeof(int), 4, f) == 4;
    ok = ok && fwrite(player.armorEnchantments, sizeof(uint16_t), 4, f) == 4;

    // Furnace state (v9+: multi-furnace array, v5-v8: single furnace)
    // Sync active furnace before saving
    if (activeFurnace >= 0 && activeFurnace < furnaceCount) SyncActiveToFurnace(activeFurnace);
    // Write legacy single-furnace fields for v5-v8 compat (first furnace or active)
    if (furnaceCount > 0) {
        FurnaceData *fd = &furnaces[0];
        ok = ok && fwrite(&fd->x, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&fd->y, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&fd->fuel, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&fd->fuelCount, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&fd->input, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&fd->inputCount, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&fd->output, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&fd->outputCount, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&fd->progress, sizeof(float), 1, f) == 1;
        ok = ok && fwrite(&fd->fuelBurn, sizeof(float), 1, f) == 1;
        ok = ok && fwrite(&fd->fuelBurnMax, sizeof(float), 1, f) == 1;
    } else {
        int zero = 0; float zerof = 0.0f; uint8_t zerob = 0;
        ok = ok && fwrite(&zero, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&zero, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&zerob, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&zero, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&zerob, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&zero, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&zerob, sizeof(uint8_t), 1, f) == 1;
        ok = ok && fwrite(&zero, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&zerof, sizeof(float), 1, f) == 1;
        ok = ok && fwrite(&zerof, sizeof(float), 1, f) == 1;
        ok = ok && fwrite(&zerof, sizeof(float), 1, f) == 1;
    }
    // v9+: additional furnaces
    {
        int extraFurnaces = furnaceCount > 1 ? furnaceCount - 1 : 0;
        ok = ok && fwrite(&extraFurnaces, sizeof(int), 1, f) == 1;
        for (int i = 1; i < furnaceCount && ok; i++) {
            FurnaceData *fd = &furnaces[i];
            ok = ok && fwrite(&fd->x, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&fd->y, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&fd->fuel, sizeof(uint8_t), 1, f) == 1;
            ok = ok && fwrite(&fd->fuelCount, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&fd->input, sizeof(uint8_t), 1, f) == 1;
            ok = ok && fwrite(&fd->inputCount, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&fd->output, sizeof(uint8_t), 1, f) == 1;
            ok = ok && fwrite(&fd->outputCount, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&fd->progress, sizeof(float), 1, f) == 1;
            ok = ok && fwrite(&fd->fuelBurn, sizeof(float), 1, f) == 1;
            ok = ok && fwrite(&fd->fuelBurnMax, sizeof(float), 1, f) == 1;
        }
    }

    // Chest data (v6+)
    ok = ok && fwrite(&chestCount, sizeof(int), 1, f) == 1;
    for (int i = 0; i < chestCount && ok; i++) {
        ok = ok && fwrite(&chestData[i].x, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(&chestData[i].y, sizeof(int), 1, f) == 1;
        ok = ok && fwrite(chestData[i].items, sizeof(uint8_t), CHEST_SLOTS, f) == CHEST_SLOTS;
        ok = ok && fwrite(chestData[i].counts, sizeof(int), CHEST_SLOTS, f) == CHEST_SLOTS;
        // v13+: tool durability + enchantments for items stored in the chest
        if (version >= 13) {
            ok = ok && fwrite(chestData[i].durability, sizeof(int), CHEST_SLOTS, f) == CHEST_SLOTS;
            ok = ok && fwrite(chestData[i].enchantments, sizeof(uint16_t), CHEST_SLOTS, f) == CHEST_SLOTS;
        }
    }

    // Weather state (v7+)
    ok = ok && fwrite(&weather.type, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&weather.duration, sizeof(float), 1, f) == 1;

    // Game mode (v15+)
    {
        uint8_t gm = (uint8_t)gameMode;
        ok = ok && fwrite(&gm, sizeof(uint8_t), 1, f) == 1;
    }

    // Achievements (v9+)
    ok = ok && fwrite(achievements, sizeof(bool), ACH_COUNT, f) == ACH_COUNT;
    ok = ok && fwrite(&totalMobsKilled, sizeof(int), 1, f) == 1;
    ok = ok && fwrite(&totalBlocksPlaced, sizeof(int), 1, f) == 1;

    // Mob data (v10+)
    {
        int activeMobs = 0;
        for (int i = 0; i < MAX_MOBS; i++) {
            if (mobs[i].active) activeMobs++;
        }
        ok = ok && fwrite(&activeMobs, sizeof(int), 1, f) == 1;
        for (int i = 0; i < MAX_MOBS && ok; i++) {
            if (!mobs[i].active) continue;
            ok = ok && fwrite(&mobs[i].type, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].position.x, sizeof(float), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].position.y, sizeof(float), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].health, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].maxHealth, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].facingRight, sizeof(bool), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].isBaby, sizeof(bool), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].growTimer, sizeof(float), 1, f) == 1;
            ok = ok && fwrite(&mobs[i].slimeType, sizeof(int), 1, f) == 1;
        }
    }

    // World data - RLE per column
    for (int x = 0; x < WORLD_WIDTH && ok; x++) {
        int y = 0;
        while (y < WORLD_HEIGHT && ok) {
            uint8_t block = world[x][y];
            uint16_t count = 1;
            while (y + count < WORLD_HEIGHT && world[x][y + count] == block && count < 65535) {
                count++;
            }
            ok = ok && fwrite(&block, sizeof(uint8_t), 1, f) == 1;
            ok = ok && fwrite(&count, sizeof(uint16_t), 1, f) == 1;
            y += count;
        }
    }

    // Modified blocks for multiplayer world sync (v8+)
    uint32_t modCount = (uint32_t)modifiedBlockCount;
    ok = ok && fwrite(&modCount, sizeof(uint32_t), 1, f) == 1;
    for (uint32_t i = 0; i < modCount && ok; i++) {
        ok = ok && fwrite(&modifiedBlocks[i].x, sizeof(uint16_t), 1, f) == 1;
        ok = ok && fwrite(&modifiedBlocks[i].y, sizeof(uint16_t), 1, f) == 1;
        ok = ok && fwrite(&modifiedBlocks[i].blockType, sizeof(uint8_t), 1, f) == 1;
    }

    // v11+: cauldron data
    if (version >= 11) {
        ok = ok && fwrite(&cauldronCount, sizeof(int), 1, f) == 1;
        for (int i = 0; i < cauldronCount && ok; i++) {
            ok = ok && fwrite(&cauldrons[i].x, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&cauldrons[i].y, sizeof(int), 1, f) == 1;
            ok = ok && fwrite(&cauldrons[i].fillLevel, sizeof(uint8_t), 1, f) == 1;
        }
    }

    // v12+: crop growth — sparse, only BLOCK_CROPS cells carry a stage.
    // Two passes: count first (the header needs the total), then write each cell.
    if (version >= 12) {
        uint32_t cropN = 0;
        for (int x = 0; x < WORLD_WIDTH; x++)
            for (int y = 0; y < WORLD_HEIGHT; y++)
                if (world[x][y] == BLOCK_CROPS) cropN++;
        ok = ok && fwrite(&cropN, sizeof(uint32_t), 1, f) == 1;
        for (int x = 0; x < WORLD_WIDTH && ok; x++) {
            for (int y = 0; y < WORLD_HEIGHT && ok; y++) {
                if (world[x][y] != BLOCK_CROPS) continue;
                uint16_t cx = (uint16_t)x, cy = (uint16_t)y;
                uint8_t g = (uint8_t)GetCropGrowth(x, y);
                ok = ok && fwrite(&cx, sizeof(uint16_t), 1, f) == 1;
                ok = ok && fwrite(&cy, sizeof(uint16_t), 1, f) == 1;
                ok = ok && fwrite(&g, sizeof(uint8_t), 1, f) == 1;
            }
        }
    }

    // v16+: sparse dynamic fluid state.
    if (version >= 16) {
        uint32_t fluidN = 0;
        for (int x = 0; x < WORLD_WIDTH; x++) for (int y = 0; y < WORLD_HEIGHT; y++) {
            uint8_t bt, kind, level; bool source;
            GetFluidState(x, y, &bt, &kind, &level, &source);
            if (kind && (level || source)) fluidN++;
        }
        ok = ok && fwrite(&fluidN, sizeof(fluidN), 1, f) == 1;
        for (int x = 0; x < WORLD_WIDTH && ok; x++) for (int y = 0; y < WORLD_HEIGHT && ok; y++) {
            uint8_t bt, kind, level; bool source;
            GetFluidState(x, y, &bt, &kind, &level, &source);
            if (!kind || (!level && !source)) continue;
            uint16_t sx = (uint16_t)x, sy = (uint16_t)y;
            uint8_t src = source ? 1 : 0;
            ok = ok && fwrite(&sx, sizeof(sx), 1, f) == 1;
            ok = ok && fwrite(&sy, sizeof(sy), 1, f) == 1;
            ok = ok && fwrite(&bt, sizeof(bt), 1, f) == 1;
            ok = ok && fwrite(&kind, sizeof(kind), 1, f) == 1;
            ok = ok && fwrite(&level, sizeof(level), 1, f) == 1;
            ok = ok && fwrite(&src, sizeof(src), 1, f) == 1;
        }
    }

    // Dimensions (v17+) - reserved. Always 1 today; a second dimension would be
    // written here without another version bump.
    if (ok) {
        uint8_t dimensionCount = CURRENT_DIMENSION_COUNT;
        ok = ok && fwrite(&dimensionCount, sizeof(dimensionCount), 1, f) == 1;
    }


    fclose(f);

    // Append integrity trailer
    if (ok) {
        f = fopen(tmpPath, "ab");
        if (f) {
            uint32_t trailer = SAVE_TRAILER_MAGIC;
            if (fwrite(&trailer, sizeof(uint32_t), 1, f) != 1) ok = false;
            fclose(f);
        }
    }

    if (!ok) {
        remove(tmpPath);
        return false;
    }

    // Atomic replace: use MoveFileEx on Windows for true atomicity
#ifdef _WIN32
    if (!MoveFileExA(tmpPath, path, MOVEFILE_REPLACE_EXISTING)) {
        remove(tmpPath);
        return false;
    }
#else
    remove(path);
    if (rename(tmpPath, path) != 0) {
        remove(tmpPath);
        return false;
    }
#endif
    return true;
}

bool LoadWorld(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    // Header
    char magic[4];
    if (fread(magic, 1, 4, f) != 4) { fclose(f); return false; }
    if (memcmp(magic, SAVE_MAGIC, 4) != 0) { fclose(f); return false; }

    uint32_t version, seed, ww, wh;
    if (fread(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&seed, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&ww, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
    if (fread(&wh, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }

    if (ww != WORLD_WIDTH || wh != WORLD_HEIGHT) {
        fclose(f);
        return false;
    }
    if (version > SAVE_VERSION) {
        fclose(f);
        return false;
    }

    worldSeed = seed;

    // Day/Night
    if (fread(&dayNight.timeOfDay, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&dayNight.daySpeed, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&dayNight.lightLevel, sizeof(float), 1, f) != 1) { fclose(f); return false; }

    // Player
    if (fread(&player.position.x, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&player.position.y, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&player.velocity.x, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&player.velocity.y, sizeof(float), 1, f) != 1) { fclose(f); return false; }
    if (fread(&player.onGround, sizeof(bool), 1, f) != 1) { fclose(f); return false; }
    if (fread(&player.selectedSlot, sizeof(int), 1, f) != 1) { fclose(f); return false; }

    if (version >= 2) {
        // v2+: full 36-slot inventory
        if (fread(player.inventory, sizeof(uint8_t), INVENTORY_SLOTS, f) != INVENTORY_SLOTS) { fclose(f); return false; }
        if (fread(player.inventoryCount, sizeof(int), INVENTORY_SLOTS, f) != INVENTORY_SLOTS) { fclose(f); return false; }
    } else {
        // v1 compat: only 9 hotbar slots, clear the rest
        if (fread(player.inventory, sizeof(uint8_t), HOTBAR_SLOTS, f) != HOTBAR_SLOTS) { fclose(f); return false; }
        if (fread(player.inventoryCount, sizeof(int), HOTBAR_SLOTS, f) != HOTBAR_SLOTS) { fclose(f); return false; }
        for (int i = HOTBAR_SLOTS; i < INVENTORY_SLOTS; i++) {
            player.inventory[i] = BLOCK_AIR;
            player.inventoryCount[i] = 0;
        }
    }

    if (version >= 3) {
        // v3: tool durability + player status
        if (fread(player.toolDurability, sizeof(int), INVENTORY_SLOTS, f) != INVENTORY_SLOTS) { fclose(f); return false; }
        // v9+: item enchantments
        if (version >= 9) {
            if (fread(player.itemEnchantments, sizeof(uint16_t), INVENTORY_SLOTS, f) != INVENTORY_SLOTS) { fclose(f); return false; }
        } else {
            for (int i = 0; i < INVENTORY_SLOTS; i++) player.itemEnchantments[i] = 0;
        }
        if (fread(&player.health, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.hunger, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.oxygen, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.xp, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        // v3+: bed spawn point (optional - may not exist in older v2 saves)
        if (fread(&player.spawnX, sizeof(int), 1, f) != 1 || fread(&player.spawnY, sizeof(int), 1, f) != 1) {
            player.spawnX = -1; player.spawnY = -1;
        }
        // v4+: armor slots (optional - may not exist in older saves)
        if (version >= 4) {
            if (fread(player.armor, sizeof(uint8_t), 4, f) != 4 || fread(player.armorDurability, sizeof(int), 4, f) != 4) {
                for (int i = 0; i < 4; i++) { player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; }
            }
            // v9+: armor enchantments
            if (version >= 9) {
                if (fread(player.armorEnchantments, sizeof(uint16_t), 4, f) != 4) {
                    for (int i = 0; i < 4; i++) player.armorEnchantments[i] = 0;
                }
            } else {
                for (int i = 0; i < 4; i++) player.armorEnchantments[i] = 0;
            }
        } else {
            for (int i = 0; i < 4; i++) { player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; player.armorEnchantments[i] = 0; }
        }
        // v5+: furnace state (legacy single furnace)
        furnaceCount = 0;
        activeFurnace = -1;
        if (version >= 5) {
            FurnaceData fd;
            memset(&fd, 0, sizeof(FurnaceData));
            if (fread(&fd.x, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.y, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.fuel, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.fuelCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.input, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.inputCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.output, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.outputCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.progress, sizeof(float), 1, f) != 1) { fclose(f); return false; }
            if (fread(&fd.fuelBurn, sizeof(float), 1, f) != 1) { fclose(f); return false; }
            if (version >= 9) {
                if (fread(&fd.fuelBurnMax, sizeof(float), 1, f) != 1) { fclose(f); return false; }
            }
            // Add to furnace array if valid
            if (fd.x >= 0) {
                furnaces[furnaceCount++] = fd;
            }
            // v9+: additional furnaces
            if (version >= 9) {
                int extraFurnaces = 0;
                if (fread(&extraFurnaces, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                for (int i = 0; i < extraFurnaces && furnaceCount < MAX_FURNACES; i++) {
                    FurnaceData efd;
                    memset(&efd, 0, sizeof(FurnaceData));
                    if (fread(&efd.x, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.y, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.fuel, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.fuelCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.input, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.inputCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.output, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.outputCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.progress, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.fuelBurn, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&efd.fuelBurnMax, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    furnaces[furnaceCount++] = efd;
                }
            }
            // Load first furnace into globals for backward compat
            if (furnaceCount > 0) {
                SyncFurnaceToActive(0);
            } else {
                furnaceFuel = BLOCK_AIR; furnaceFuelCount = 0;
                furnaceInput = BLOCK_AIR; furnaceInputCount = 0;
                furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0;
                furnaceProgress = 0.0f; furnaceFuelBurn = 0.0f;
                furnaceFuelBurnMax = 0.0f;
                furnaceBlockX = -1; furnaceBlockY = -1;
            }
        } else {
            furnaceFuel = BLOCK_AIR; furnaceFuelCount = 0;
            furnaceInput = BLOCK_AIR; furnaceInputCount = 0;
            furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0;
            furnaceProgress = 0.0f; furnaceFuelBurn = 0.0f;
            furnaceBlockX = -1; furnaceBlockY = -1;
        }
        // v6+: chest data
        if (version >= 6) {
            if (fread(&chestCount, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            for (int i = 0; i < chestCount; i++) {
                if (fread(&chestData[i].x, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                if (fread(&chestData[i].y, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                if (fread(chestData[i].items, sizeof(uint8_t), CHEST_SLOTS, f) != CHEST_SLOTS) { fclose(f); return false; }
                if (fread(chestData[i].counts, sizeof(int), CHEST_SLOTS, f) != CHEST_SLOTS) { fclose(f); return false; }
                // v13+: per-slot durability + enchantments (older saves default to 0)
                if (version >= 13) {
                    if (fread(chestData[i].durability, sizeof(int), CHEST_SLOTS, f) != CHEST_SLOTS) { fclose(f); return false; }
                    if (fread(chestData[i].enchantments, sizeof(uint16_t), CHEST_SLOTS, f) != CHEST_SLOTS) { fclose(f); return false; }
                } else {
                    for (int s = 0; s < CHEST_SLOTS; s++) { chestData[i].durability[s] = 0; chestData[i].enchantments[s] = 0; }
                }
            }
        } else {
            chestCount = 0;
        }
        // v7+: weather state
        if (version >= 7) {
            int weatherType;
            if (fread(&weatherType, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            weather.type = (WeatherType)weatherType;
            if (fread(&weather.duration, sizeof(float), 1, f) != 1) { fclose(f); return false; }
            weather.transitionTimer = 0;
            weather.rainAlpha = (weather.type == WEATHER_CLEAR) ? 0 : 1;
        } else {
            InitWeather();
        }
        // v15+: game mode
        if (version >= 15) {
            uint8_t gm;
            if (fread(&gm, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
            gameMode = (gm <= GAME_CREATIVE) ? (GameMode)gm : GAME_SURVIVAL;
        } else {
            gameMode = GAME_SURVIVAL;
        }
        // v9+: achievements. v9-v13 stored 6 achievements; v14+ stores ACH_COUNT.
        if (version >= 9) {
            int achStored = (version >= 14) ? ACH_COUNT : 6;
            for (int i = 0; i < ACH_COUNT; i++) achievements[i] = false;
            for (int i = 0; i < achStored; i++) {
                bool b;
                if (fread(&b, sizeof(bool), 1, f) != 1) { fclose(f); return false; }
                if (i < ACH_COUNT) achievements[i] = b;
            }
            if (fread(&totalMobsKilled, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            if (fread(&totalBlocksPlaced, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        } else {
            for (int i = 0; i < ACH_COUNT; i++) achievements[i] = false;
            totalMobsKilled = 0;
            totalBlocksPlaced = 0;
        }
        // v10+: mob data
        if (version >= 10) {
            int activeMobs = 0;
            if (fread(&activeMobs, sizeof(int), 1, f) != 1) { fclose(f); return false; }
            InitMobs();
            for (int i = 0; i < activeMobs && i < MAX_MOBS; i++) {
                int type;
                if (fread(&type, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                Mob *mob = SpawnMob((MobType)type, 0, 0);
                if (mob) {
                    if (fread(&mob->position.x, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->position.y, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->health, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->maxHealth, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->facingRight, sizeof(bool), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->isBaby, sizeof(bool), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->growTimer, sizeof(float), 1, f) != 1) { fclose(f); return false; }
                    if (fread(&mob->slimeType, sizeof(int), 1, f) != 1) { fclose(f); return false; }
                }
            }
        } else {
            InitMobs();
        }
        // Cauldron (v11+) and crop (v12+) data are written by SaveWorld AFTER the
        // world + modified-block sections, so they are read below those sections.
        // Default here; the previous version read them here, which desynced the
        // stream for v11 saves (cauldron count was read from world-RLE bytes).
        cauldronCount = 0;
    } else {
        // v2 compat: default values
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (IsTool((BlockType)player.inventory[i])) {
                player.toolDurability[i] = GetToolMaxDurability((BlockType)player.inventory[i]);
            } else {
                player.toolDurability[i] = 0;
            }
        }
        player.health = MAX_HEALTH;
        player.hunger = MAX_HUNGER;
        player.oxygen = MAX_OXYGEN;
        player.xp = 0;
        for (int i = 0; i < 4; i++) { player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; }
    }
    player.oxygenTimer = 0.0f;
    player.hungerTimer = 0.0f;
    player.regenTimer = 0.0f;
    player.drownTimer = 0.0f;
    player.hungerDamageTimer = 0.0f;
    player.damageFlashTimer = 0.0f;
    player.sprinting = false;
    player.playerDead = false;
    player.fallPeakVel = 0.0f;
    player.fallDistance = 0.0f;
    player.knockbackTimer = 0.0f;
    player.cameraShakeIntensity = 0.0f;
    player.cameraShakeTimer = 0.0f;
    player.attackCooldown = 0.0f;
    player.coyoteTimer = 0.0f;
    player.jumpBufferTimer = 0.0f;

    // World data - RLE per column
    for (int x = 0; x < WORLD_WIDTH; x++) {
        int y = 0;
        while (y < WORLD_HEIGHT) {
            uint8_t block;
            uint16_t count;
            if (fread(&block, sizeof(uint8_t), 1, f) != 1) { fclose(f); return false; }
            if (fread(&count, sizeof(uint16_t), 1, f) != 1) { fclose(f); return false; }
            for (int i = 0; i < count && y + i < WORLD_HEIGHT; i++) {
                world[x][y + i] = block;
            }
            y += count > 0 ? count : 1;
        }
    }

    // Modified blocks for multiplayer world sync (v8+)
    modifiedBlockCount = 0;
    if (version >= 8) {
        uint32_t modCount = 0;
        if (fread(&modCount, sizeof(uint32_t), 1, f) == 1) {
            for (uint32_t i = 0; i < modCount && i < MAX_MODIFIED_BLOCKS; i++) {
                if (fread(&modifiedBlocks[i].x, sizeof(uint16_t), 1, f) != 1) break;
                if (fread(&modifiedBlocks[i].y, sizeof(uint16_t), 1, f) != 1) break;
                if (fread(&modifiedBlocks[i].blockType, sizeof(uint8_t), 1, f) != 1) break;
                modifiedBlockCount++;
            }
        }
    } else {
        // Pre-v8 save: compute modified blocks by comparing against freshly generated world
        // Save current world, regenerate from seed, compare, restore
        uint8_t (*savedWorld)[WORLD_HEIGHT] = malloc(WORLD_WIDTH * WORLD_HEIGHT);
        if (savedWorld) {
            memcpy(savedWorld, world, WORLD_WIDTH * WORLD_HEIGHT);
            GenerateWorld(worldSeed);
            for (int x = 0; x < WORLD_WIDTH && modifiedBlockCount < MAX_MODIFIED_BLOCKS; x++) {
                for (int y = 0; y < WORLD_HEIGHT && modifiedBlockCount < MAX_MODIFIED_BLOCKS; y++) {
                    if (savedWorld[x][y] != world[x][y]) {
                        modifiedBlocks[modifiedBlockCount].x = (uint16_t)x;
                        modifiedBlocks[modifiedBlockCount].y = (uint16_t)y;
                        modifiedBlocks[modifiedBlockCount].blockType = savedWorld[x][y];
                        modifiedBlockCount++;
                    }
                }
            }
            memcpy(world, savedWorld, WORLD_WIDTH * WORLD_HEIGHT);
            free(savedWorld);
        }
    }

    // v11+: cauldron data (SaveWorld writes this AFTER world + modified blocks).
    // Read non-fatally: a truncated tail leaves earlier sections intact.
    if (version >= 11) {
        int cc = 0;
        if (fread(&cc, sizeof(int), 1, f) == 1 && cc >= 0 && cc <= MAX_CAULDRONS) {
            cauldronCount = cc;
            for (int i = 0; i < cauldronCount; i++) {
                if (fread(&cauldrons[i].x, sizeof(int), 1, f) != 1) { cauldronCount = i; break; }
                if (fread(&cauldrons[i].y, sizeof(int), 1, f) != 1) { cauldronCount = i; break; }
                if (fread(&cauldrons[i].fillLevel, sizeof(uint8_t), 1, f) != 1) { cauldronCount = i; break; }
            }
        }
    }

    // v12+: crop growth (sparse — only BLOCK_CROPS cells). cropGrowth was cleared
    // by InitRedstone() before LoadWorld, so unlisted cells stay at stage 0.
    if (version >= 12) {
        uint32_t cropN = 0;
        if (fread(&cropN, sizeof(uint32_t), 1, f) == 1) {
            for (uint32_t i = 0; i < cropN; i++) {
                uint16_t cx, cy; uint8_t g;
                if (fread(&cx, sizeof(uint16_t), 1, f) != 1) break;
                if (fread(&cy, sizeof(uint16_t), 1, f) != 1) break;
                if (fread(&g, sizeof(uint8_t), 1, f) != 1) break;
                SetCropGrowth((int)cx, (int)cy, (int)g);
            }
        }
    }

    // Legacy fluid migration: preserve old visible fluids as static full cells.
    // Historical formats had no source/level metadata, so do not guess dynamic sources.
    if (version < 16) {
        for (int x = 0; x < WORLD_WIDTH; x++) for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_WATER) RestoreFluidState(x, y, BLOCK_WATER, BLOCK_WATER, 7, false);
            else if (world[x][y] == BLOCK_LAVA) RestoreFluidState(x, y, BLOCK_LAVA, BLOCK_LAVA, 5, false);
        }
    }

    // v16+: restore sparse dynamic fluid metadata directly (never propagate per record).
    if (version >= 16) {
        uint32_t fluidN = 0;
        if (fread(&fluidN, sizeof(fluidN), 1, f) != 1 || fluidN > WORLD_WIDTH * WORLD_HEIGHT) {
            fclose(f); return false;
        }
        for (uint32_t i = 0; i < fluidN; i++) {
            uint16_t x, y; uint8_t bt, kind, level, src;
            if (fread(&x, sizeof(x), 1, f) != 1 || fread(&y, sizeof(y), 1, f) != 1 ||
                fread(&bt, sizeof(bt), 1, f) != 1 || fread(&kind, sizeof(kind), 1, f) != 1 ||
                fread(&level, sizeof(level), 1, f) != 1 || fread(&src, sizeof(src), 1, f) != 1) {
                fclose(f); return false;
            }
            if (x >= WORLD_WIDTH || y >= WORLD_HEIGHT ||
                (kind != BLOCK_WATER && kind != BLOCK_LAVA) ||
                (kind == BLOCK_WATER && level > 7) || (kind == BLOCK_LAVA && level > 5) ||
                (bt != BLOCK_WATER && bt != BLOCK_LAVA)) {
                fclose(f); return false;
            }
            RestoreFluidState((int)x, (int)y, bt, kind, level, src != 0);
        }
        QueueFluidSources();
    }

    // Dimensions (v17+). Older saves have no such field and imply 1.
    if (version >= 17) {
        uint8_t dimensionCount = 0;
        if (fread(&dimensionCount, sizeof(dimensionCount), 1, f) != 1) { fclose(f); return false; }
        if (dimensionCount != CURRENT_DIMENSION_COUNT) {
            // A save from a build that had a different number of dimensions.
            TraceLog(LOG_WARNING, "SAVE: file has %u dimensions, this build supports %u",
                     (unsigned)dimensionCount, (unsigned)CURRENT_DIMENSION_COUNT);
        }
    }

    {
        uint32_t trailer = 0;
        if (fread(&trailer, sizeof(uint32_t), 1, f) != 1) {
            // The trailer was introduced after early v14 saves. EOF is valid
            // for pre-v16 historical files whose preceding sections passed.
            if (version >= 16 || !feof(f)) { fclose(f); return false; }
        } else if (trailer != SAVE_TRAILER_MAGIC) {
            fclose(f);
            return false;
        }
    }

    fclose(f);

    // Invalidate all loaded chunks so they regenerate textures
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
        loadedChunks[i].textureValid = false;
    }

    return true;
}
