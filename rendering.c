#include "types.h"
#include <math.h>
#include <stdio.h>

// Held item state for inventory drag-and-drop
static uint8_t heldItem = BLOCK_AIR;
static int heldCount = 0;

static void DrawSlot(int x, int y, int slotSize, int slotIndex, bool highlight)
{
    Color bg = highlight ? (Color){255, 255, 255, 200} : (Color){80, 80, 80, 200};
    DrawRectangle(x, y, slotSize, slotSize, bg);
    DrawRectangleLines(x, y, slotSize, slotSize, DARKGRAY);

    if (player.inventory[slotIndex] != BLOCK_AIR) {
        Rectangle src = { (float)(player.inventory[slotIndex] * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

        if (player.inventoryCount[slotIndex] > 1) {
            DrawText(TextFormat("%d", player.inventoryCount[slotIndex]),
                     x + slotSize - 20, y + slotSize - 16, 12, WHITE);
        }
    }
}

void DrawInventoryScreen(void)
{
    if (!inventoryOpen) return;

    int slotSize = 44;
    int padding = 4;
    int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;
    int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
    int gridX = (SCREEN_WIDTH - gridW - 320) / 2;  // Leave room for crafting panel
    int gridY = (SCREEN_HEIGHT - gridH) / 2;

    // Dim overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 150});

    // Inventory panel background
    DrawRectangle(gridX - 10, gridY - 30, gridW + 20, gridH + 50, (Color){50, 50, 50, 230});
    DrawRectangleLines(gridX - 10, gridY - 30, gridW + 20, gridH + 50, (Color){100, 100, 100, 255});
    DrawText("Inventory", gridX, gridY - 22, 18, WHITE);

    Vector2 mouse = GetMousePosition();

    // Draw inventory grid
    for (int row = 0; row < INVENTORY_ROWS; row++) {
        for (int col = 0; col < INVENTORY_COLS; col++) {
            int idx = row * INVENTORY_COLS + col;
            int x = gridX + col * (slotSize + padding);
            int y = gridY + row * (slotSize + padding);

            Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);
            bool selected = (row == 0 && col == player.selectedSlot);

            DrawSlot(x, y, slotSize, idx, selected || hover);

            // Row labels for hotbar
            if (row == 0) {
                DrawText(TextFormat("%d", col + 1), x + 3, y + 2, 10, (Color){200, 200, 200, 150});
            }

            // Handle click
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    // Pick up item
                    heldItem = player.inventory[idx];
                    heldCount = player.inventoryCount[idx];
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    // Place held item
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    heldItem = BLOCK_AIR;
                    heldCount = 0;
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                    // Stack
                    int space = 64 - player.inventoryCount[idx];
                    int toAdd = heldCount > space ? space : heldCount;
                    player.inventoryCount[idx] += toAdd;
                    heldCount -= toAdd;
                    if (heldCount <= 0) {
                        heldItem = BLOCK_AIR;
                        heldCount = 0;
                    }
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    // Swap held and slot
                    uint8_t tmpItem = player.inventory[idx];
                    int tmpCount = player.inventoryCount[idx];
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    heldItem = tmpItem;
                    heldCount = tmpCount;
                }
            }

            // Right click to pick up half
            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && heldItem == BLOCK_AIR
                && player.inventory[idx] != BLOCK_AIR) {
                heldItem = player.inventory[idx];
                heldCount = (player.inventoryCount[idx] + 1) / 2;
                player.inventoryCount[idx] -= heldCount;
                if (player.inventoryCount[idx] <= 0) {
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                }
            }
        }
    }

    // Draw held item following mouse
    if (heldItem != BLOCK_AIR) {
        int mx = (int)mouse.x - slotSize / 2;
        int my = (int)mouse.y - slotSize / 2;
        Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(mx + 4), (float)(my + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (heldCount > 1) {
            DrawText(TextFormat("%d", heldCount), mx + slotSize - 20, my + slotSize - 16, 12, WHITE);
        }
    }

    // Crafting panel
    int craftX = gridX + gridW + 20;
    int craftY = gridY - 10;
    DrawCraftingPanel(craftX, craftY);

    // Drop held item if inventory closes with held item
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) {
        if (heldItem != BLOCK_AIR) {
            AddToInventory(heldItem);
            heldItem = BLOCK_AIR;
            heldCount = 0;
        }
    }
}

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
