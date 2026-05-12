#include "types.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

//----------------------------------------------------------------------------------
// Save File Format:
//   Header:      "MWSV" + uint32 version + uint32 seed + uint32 worldW + uint32 worldH
//   DayNight:    float timeOfDay + float daySpeed + float lightLevel
//   Player:      float posX,Y + float velX,Y + bool onGround + int selectedSlot
//                + uint8 inventory[9] + int inventoryCount[9]
//   World:       RLE per column: (uint8 block, uint16 count) pairs
//----------------------------------------------------------------------------------

bool SaveExists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return true; }
    return false;
}

bool SaveWorld(const char *path)
{
    // Ensure saves directory exists
    mkdir("saves");

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    // Header
    fwrite(SAVE_MAGIC, 1, 4, f);
    uint32_t version = SAVE_VERSION;
    uint32_t seed = 0; // seed not stored globally, write 0 placeholder
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
    fwrite(player.inventory, sizeof(uint8_t), HOTBAR_SLOTS, f);
    fwrite(player.inventoryCount, sizeof(int), HOTBAR_SLOTS, f);

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
    return true;
}

bool LoadWorld(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    // Header
    char magic[4];
    fread(magic, 1, 4, f);
    if (memcmp(magic, SAVE_MAGIC, 4) != 0) { fclose(f); return false; }

    uint32_t version, seed, ww, wh;
    fread(&version, sizeof(uint32_t), 1, f);
    fread(&seed, sizeof(uint32_t), 1, f);
    fread(&ww, sizeof(uint32_t), 1, f);
    fread(&wh, sizeof(uint32_t), 1, f);

    if (version != SAVE_VERSION || ww != WORLD_WIDTH || wh != WORLD_HEIGHT) {
        fclose(f);
        return false;
    }

    // Day/Night
    fread(&dayNight.timeOfDay, sizeof(float), 1, f);
    fread(&dayNight.daySpeed, sizeof(float), 1, f);
    fread(&dayNight.lightLevel, sizeof(float), 1, f);

    // Player
    fread(&player.position.x, sizeof(float), 1, f);
    fread(&player.position.y, sizeof(float), 1, f);
    fread(&player.velocity.x, sizeof(float), 1, f);
    fread(&player.velocity.y, sizeof(float), 1, f);
    fread(&player.onGround, sizeof(bool), 1, f);
    fread(&player.selectedSlot, sizeof(int), 1, f);
    fread(player.inventory, sizeof(uint8_t), HOTBAR_SLOTS, f);
    fread(player.inventoryCount, sizeof(int), HOTBAR_SLOTS, f);

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
    for (int i = 0; i < loadedChunkCount; i++) {
        if (loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
        loadedChunks[i].textureValid = false;
    }

    return true;
}
