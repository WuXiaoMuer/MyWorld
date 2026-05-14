#include "types.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

//----------------------------------------------------------------------------------
// Save File Format v3:
//   Header:      "MWSV" + uint32 version + uint32 seed + uint32 worldW + uint32 worldH
//   DayNight:    float timeOfDay + float daySpeed + float lightLevel
//   Player:      float posX,Y + float velX,Y + bool onGround + int selectedSlot
//                + uint8 inventory[36] + int inventoryCount[36] + int toolDurability[36]
//                + int health + int hunger + int oxygen + int xp
//   World:       RLE per column: (uint8 block, uint16 count) pairs
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

    // Header
    fwrite(SAVE_MAGIC, 1, 4, f);
    uint32_t version = SAVE_VERSION;
    uint32_t seed = worldSeed;
    uint32_t ww = WORLD_WIDTH;
    uint32_t wh = WORLD_HEIGHT;
    fwrite(&version, sizeof(uint32_t), 1, f);
    fwrite(&seed, sizeof(uint32_t), 1, f);
    fwrite(&ww, sizeof(uint32_t), 1, f);
    fwrite(&wh, sizeof(uint32_t), 1, f);

    // Day/Night
    fwrite(&dayNight.timeOfDay, sizeof(float), 1, f);
    fwrite(&dayNight.daySpeed, sizeof(float), 1, f);
    fwrite(&dayNight.lightLevel, sizeof(float), 1, f);

    // Player
    fwrite(&player.position.x, sizeof(float), 1, f);
    fwrite(&player.position.y, sizeof(float), 1, f);
    fwrite(&player.velocity.x, sizeof(float), 1, f);
    fwrite(&player.velocity.y, sizeof(float), 1, f);
    fwrite(&player.onGround, sizeof(bool), 1, f);
    fwrite(&player.selectedSlot, sizeof(int), 1, f);
    fwrite(player.inventory, sizeof(uint8_t), INVENTORY_SLOTS, f);
    fwrite(player.inventoryCount, sizeof(int), INVENTORY_SLOTS, f);
    fwrite(player.toolDurability, sizeof(int), INVENTORY_SLOTS, f);
    fwrite(&player.health, sizeof(int), 1, f);
    fwrite(&player.hunger, sizeof(int), 1, f);
    fwrite(&player.oxygen, sizeof(int), 1, f);
    fwrite(&player.xp, sizeof(int), 1, f);
    fwrite(&player.spawnX, sizeof(int), 1, f);
    fwrite(&player.spawnY, sizeof(int), 1, f);
    fwrite(player.armor, sizeof(uint8_t), 4, f);
    fwrite(player.armorDurability, sizeof(int), 4, f);

    // World data - RLE per column
    for (int x = 0; x < WORLD_WIDTH; x++) {
        int y = 0;
        while (y < WORLD_HEIGHT) {
            uint8_t block = world[x][y];
            uint16_t count = 1;
            while (y + count < WORLD_HEIGHT && world[x][y + count] == block && count < 65535) {
                count++;
            }
            fwrite(&block, sizeof(uint8_t), 1, f);
            fwrite(&count, sizeof(uint16_t), 1, f);
            y += count;
        }
    }

    fclose(f);

    // Atomic replace: remove old file, rename tmp
    remove(path);
    if (rename(tmpPath, path) != 0) {
        remove(tmpPath);
        return false;
    }
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
        if (fread(&player.health, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.hunger, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.oxygen, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        if (fread(&player.xp, sizeof(int), 1, f) != 1) { fclose(f); return false; }
        // v3+: bed spawn point (optional - may not exist in older v2 saves)
        if (fread(&player.spawnX, sizeof(int), 1, f) != 1) { player.spawnX = -1; player.spawnY = -1; }
        else if (fread(&player.spawnY, sizeof(int), 1, f) != 1) { player.spawnY = -1; }
        // v4+: armor slots (optional - may not exist in older saves)
        if (version >= 4) {
            if (fread(player.armor, sizeof(uint8_t), 4, f) != 4) { for (int i = 0; i < 4; i++) player.armor[i] = BLOCK_AIR; }
            if (fread(player.armorDurability, sizeof(int), 4, f) != 4) { for (int i = 0; i < 4; i++) player.armorDurability[i] = 0; }
        } else {
            for (int i = 0; i < 4; i++) { player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; }
        }
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
            y += count;
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
