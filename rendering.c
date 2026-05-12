#include "types.h"
#include <math.h>
#include <stdio.h>

void DrawWorld(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    for (int i = 0; i < loadedChunkCount; i++) {
        Chunk *c = &loadedChunks[i];
        if (c->chunkX < minCX || c->chunkX > maxCX) continue;
        if (!c->textureValid) continue;
        float x = (float)(c->chunkX * CHUNK_SIZE * BLOCK_SIZE);
        DrawTexture(c->texture, (int)x, 0, WHITE);
    }
}

void DrawWater(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewTop = camera.target.y - (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;

    int minBX = (int)(viewLeft / BLOCK_SIZE) - 1;
    int maxBX = (int)(viewRight / BLOCK_SIZE) + 1;
    int minBY = (int)(viewTop / BLOCK_SIZE) - 1;
    int maxBY = (int)(viewBottom / BLOCK_SIZE) + 1;

    if (minBX < 0) minBX = 0;
    if (maxBX >= WORLD_WIDTH) maxBX = WORLD_WIDTH - 1;
    if (minBY < 0) minBY = 0;
    if (maxBY >= WORLD_HEIGHT) maxBY = WORLD_HEIGHT - 1;

    float time = (float)GetTime();

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (world[bx][by] == BLOCK_WATER) {
                float wave = sinf(bx * 0.5f + time * 2.0f) * 1.5f;
                Color wc = blockInfo[BLOCK_WATER].baseColor;
                DrawRectangle(bx * BLOCK_SIZE, (int)(by * BLOCK_SIZE + wave), BLOCK_SIZE, BLOCK_SIZE, wc);
            }
        }
    }
}

void DrawPlayerSprite(void)
{
    float px = player.position.x;
    float py = player.position.y;
    float time = (float)GetTime();
    bool moving = fabsf(player.velocity.x) > 10.0f;
    float armSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;
    float legSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;

    // Head
    DrawRectangle((int)(px + 2), (int)py, 8, 8, (Color){220, 180, 140, 255});
    // Hair
    DrawRectangle((int)(px + 2), (int)py, 8, 3, (Color){80, 50, 30, 255});
    // Eyes
    DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
    DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});

    // Body
    DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, (Color){0, 100, 200, 255});

    // Arms
    DrawRectangle((int)(px - 2), (int)(py + 8 + armSwing), 3, 10, (Color){220, 180, 140, 255});
    DrawRectangle((int)(px + 11), (int)(py + 8 - armSwing), 3, 10, (Color){220, 180, 140, 255});

    // Legs
    DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, (Color){60, 40, 20, 255});
    DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, (Color){60, 40, 20, 255});
}

void DrawHotbar(void)
{
    int slotSize = 44;
    int padding = 4;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int startY = SCREEN_HEIGHT - slotSize - 10;

    DrawRectangle(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){0, 0, 0, 150});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;

        Color slotColor = (i == player.selectedSlot) ? (Color){255, 255, 255, 200} : (Color){80, 80, 80, 200};
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, DARKGRAY);

        if (player.inventory[i] != BLOCK_AIR) {
            Rectangle src = { (float)(player.inventory[i] * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            if (player.inventoryCount[i] > 1) {
                const char *countText = TextFormat("%d", player.inventoryCount[i]);
                DrawText(countText, x + slotSize - 20, y + slotSize - 16, 12, WHITE);
            }
        }

        DrawText(TextFormat("%d", i + 1), x + 3, y + 2, 10, (Color){200, 200, 200, 150});
    }
}

void DrawCrosshair(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX >= 0 && blockX < WORLD_WIDTH && blockY >= 0 && blockY < WORLD_HEIGHT) {
        float px = (float)(blockX * BLOCK_SIZE);
        float py = (float)(blockY * BLOCK_SIZE);
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 180});
    }
}

void DrawDebugInfo(void)
{
    if (!showDebug) return;
    int y = 10;
    int lineH = 16;
    Color c = (Color){255, 255, 0, 200};

    DrawText(TextFormat("FPS: %d", GetFPS()), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Pos: %.1f, %.1f", player.position.x, player.position.y), 10, y, 14, c); y += lineH;
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    DrawText(TextFormat("Block: %d, %d", bx, by), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Chunks: %d", loadedChunkCount), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Time: %.2f", dayNight.timeOfDay), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Light: %.2f", dayNight.lightLevel), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("OnGround: %s", player.onGround ? "yes" : "no"), 10, y, 14, c);
}
