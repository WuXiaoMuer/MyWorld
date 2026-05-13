#include "types.h"
#include <math.h>
#include <stdlib.h>

// Held item state for inventory drag-and-drop
static uint8_t heldItem = BLOCK_AIR;
static int heldCount = 0;
static int heldDurability = 0;

// Death state
static float deathFadeTimer = 0.0f;

static void DrawSlot(int x, int y, int slotSize, int slotIndex, bool highlight)
{
    Color bg = highlight ? (Color){255, 255, 255, 200} : (Color){80, 80, 80, 200};
    DrawRectangle(x, y, slotSize, slotSize, bg);
    DrawRectangleLines(x, y, slotSize, slotSize, DARKGRAY);

    int item = player.inventory[slotIndex];
    if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
        Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

        if (player.inventoryCount[slotIndex] > 1) {
            DrawText(TextFormat("%d", player.inventoryCount[slotIndex]),
                     x + slotSize - 20, y + slotSize - 16, 12, WHITE);
        }

        // Tool durability bar
        if (IsTool((BlockType)item)) {
            int maxDur = GetToolMaxDurability((BlockType)player.inventory[slotIndex]);
            if (maxDur > 0) {
                float pct = (float)player.toolDurability[slotIndex] / maxDur;
                int barW = slotSize - 8;
                int barH = 3;
                int barX = x + 4;
                int barY = y + slotSize - 5;
                Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 180});
                DrawRectangle(barX, barY, (int)(barW * pct), barH, barColor);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Inventory Screen
//----------------------------------------------------------------------------------
void DrawInventoryScreen(void)
{
    if (!inventoryOpen) return;

    int slotSize = 44;
    int padding = 4;
    int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;
    int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
    int gridX = (SCREEN_WIDTH - gridW - 320) / 2;
    int gridY = (SCREEN_HEIGHT - gridH) / 2;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 150});
    DrawRectangle(gridX - 10, gridY - 30, gridW + 20, gridH + 50, (Color){50, 50, 50, 230});
    DrawRectangleLines(gridX - 10, gridY - 30, gridW + 20, gridH + 50, (Color){100, 100, 100, 255});
    DrawText("Inventory", gridX, gridY - 22, 18, WHITE);

    Vector2 mouse = GetMousePosition();

    for (int row = 0; row < INVENTORY_ROWS; row++) {
        for (int col = 0; col < INVENTORY_COLS; col++) {
            int idx = row * INVENTORY_COLS + col;
            int x = gridX + col * (slotSize + padding);
            int y = gridY + row * (slotSize + padding);

            Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);
            bool selected = (row == 0 && col == player.selectedSlot);

            DrawSlot(x, y, slotSize, idx, selected || hover);

            if (row == 0) {
                DrawText(TextFormat("%d", col + 1), x + 3, y + 2, 10, (Color){200, 200, 200, 150});
            }

            if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                    if (player.inventory[idx] != BLOCK_AIR) {
                        int startDest = (row == 0) ? HOTBAR_SLOTS : 0;
                        int endDest = (row == 0) ? INVENTORY_SLOTS : HOTBAR_SLOTS;
                        bool moved = false;
                        for (int d = startDest; d < endDest; d++) {
                            if (player.inventory[d] == player.inventory[idx] && player.inventoryCount[d] < 64) {
                                int space = 64 - player.inventoryCount[d];
                                int toAdd = player.inventoryCount[idx] > space ? space : player.inventoryCount[idx];
                                player.inventoryCount[d] += toAdd;
                                player.inventoryCount[idx] -= toAdd;
                                if (player.inventoryCount[idx] <= 0) {
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                }
                                moved = true;
                                break;
                            }
                        }
                        if (!moved) {
                            for (int d = startDest; d < endDest; d++) {
                                if (player.inventory[d] == BLOCK_AIR) {
                                    player.inventory[d] = player.inventory[idx];
                                    player.inventoryCount[d] = player.inventoryCount[idx];
                                    player.toolDurability[d] = player.toolDurability[idx];
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    break;
                                }
                            }
                        }
                    }
                } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    heldItem = player.inventory[idx];
                    heldCount = player.inventoryCount[idx];
                    heldDurability = player.toolDurability[idx];
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    player.toolDurability[idx] = heldDurability;
                    heldItem = BLOCK_AIR;
                    heldCount = 0;
                    heldDurability = 0;
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                    int space = 64 - player.inventoryCount[idx];
                    int toAdd = heldCount > space ? space : heldCount;
                    player.inventoryCount[idx] += toAdd;
                    heldCount -= toAdd;
                    if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; }
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    uint8_t tmpItem = player.inventory[idx];
                    int tmpCount = player.inventoryCount[idx];
                    int tmpDur = player.toolDurability[idx];
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    player.toolDurability[idx] = heldDurability;
                    heldItem = tmpItem;
                    heldCount = tmpCount;
                    heldDurability = tmpDur;
                }
            }

            if (hover && IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                PlaySoundDrop();
                player.inventoryCount[idx]--;
                if (player.inventoryCount[idx] <= 0) {
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                }
            }

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

    if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
        int mx = (int)mouse.x - slotSize / 2;
        int my = (int)mouse.y - slotSize / 2;
        Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(mx + 4), (float)(my + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (heldCount > 1) {
            DrawText(TextFormat("%d", heldCount), mx + slotSize - 20, my + slotSize - 16, 12, WHITE);
        }
    }

    int craftX = gridX + gridW + 20;
    int craftY = gridY - 10;
    DrawCraftingPanel(craftX, craftY);
}

//----------------------------------------------------------------------------------
// World Rendering (hash table iteration)
//----------------------------------------------------------------------------------
void DrawWorld(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (loadedChunks[i].chunkX < minCX || loadedChunks[i].chunkX > maxCX) continue;
        if (!loadedChunks[i].textureValid) continue;
        float x = (float)(loadedChunks[i].chunkX * CHUNK_SIZE * BLOCK_SIZE);
        DrawTexture(loadedChunks[i].texture, (int)x, 0, WHITE);
    }
}

//----------------------------------------------------------------------------------
// Water Rendering (uses cached waterTopY per chunk)
//----------------------------------------------------------------------------------
void DrawWater(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    int maxBY = (int)(viewBottom / BLOCK_SIZE) + 1;
    if (maxBY >= WORLD_HEIGHT) maxBY = WORLD_HEIGHT - 1;

    float time = (float)GetTime();

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (loadedChunks[i].chunkX < minCX || loadedChunks[i].chunkX > maxCX) continue;

        Chunk *c = &loadedChunks[i];
        int startX = c->chunkX * CHUNK_SIZE;

        for (int bx = 0; bx < CHUNK_SIZE; bx++) {
            int wx = startX + bx;
            if (wx < 0 || wx >= WORLD_WIDTH) continue;
            int waterTop = c->waterTopY[bx];
            if (waterTop < 0) continue;

            for (int by = waterTop; by <= maxBY; by++) {
                if (world[wx][by] != BLOCK_WATER) break;
                float wave = sinf(wx * 0.5f + time * 2.0f) * 1.5f;
                Color wc = blockInfo[BLOCK_WATER].baseColor;
                DrawRectangle(wx * BLOCK_SIZE, (int)(by * BLOCK_SIZE + wave), BLOCK_SIZE, BLOCK_SIZE, wc);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Player Sprite
//----------------------------------------------------------------------------------
void DrawPlayerSprite(void)
{
    float px = player.position.x;
    float py = player.position.y;
    float time = (float)GetTime();
    bool moving = fabsf(player.velocity.x) > 10.0f;
    float armSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;
    float legSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;

    DrawRectangle((int)(px + 2), (int)py, 8, 8, (Color){220, 180, 140, 255});
    DrawRectangle((int)(px + 2), (int)py, 8, 3, (Color){80, 50, 30, 255});
    DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
    DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
    DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, (Color){0, 100, 200, 255});
    DrawRectangle((int)(px - 2), (int)(py + 8 + armSwing), 3, 10, (Color){220, 180, 140, 255});
    DrawRectangle((int)(px + 11), (int)(py + 8 - armSwing), 3, 10, (Color){220, 180, 140, 255});
    DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, (Color){60, 40, 20, 255});
    DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, (Color){60, 40, 20, 255});
}

//----------------------------------------------------------------------------------
// Hotbar
//----------------------------------------------------------------------------------
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

        int item = player.inventory[i];
        if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            if (player.inventoryCount[i] > 1) {
                DrawText(TextFormat("%d", player.inventoryCount[i]), x + slotSize - 20, y + slotSize - 16, 12, WHITE);
            }

            // Tool durability bar
            if (IsTool((BlockType)item)) {
                int maxDur = GetToolMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    float pct = (float)player.toolDurability[i] / maxDur;
                    int barW = slotSize - 8;
                    int barH = 3;
                    int barX = x + 4;
                    int barY = y + slotSize - 5;
                    Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                    DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 180});
                    DrawRectangle(barX, barY, (int)(barW * pct), barH, barColor);
                }
            }
        }

        DrawText(TextFormat("%d", i + 1), x + 3, y + 2, 10, (Color){200, 200, 200, 150});
    }
}

//----------------------------------------------------------------------------------
// Player Status Bars (health, hunger, oxygen, XP)
//----------------------------------------------------------------------------------
void DrawPlayerStatus(void)
{
    int slotSize = 44;
    int padding = 4;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int barY = SCREEN_HEIGHT - slotSize - 28;

    int iconSize = 10;
    int iconPad = 2;
    int barX = startX;

    // Health hearts
    for (int i = 0; i < MAX_HEALTH / 2; i++) {
        int x = barX + i * (iconSize + iconPad);
        bool filled = player.health >= (i + 1) * 2;
        bool half = !filled && player.health >= i * 2 + 1;
        Color c = filled ? RED : (half ? (Color){200, 50, 50, 255} : (Color){60, 20, 20, 200});
        DrawRectangle(x, barY, iconSize, iconSize, c);
        DrawRectangleLines(x, barY, iconSize, iconSize, (Color){100, 30, 30, 200});
    }

    // Hunger
    int hungerX = barX + (MAX_HEALTH / 2) * (iconSize + iconPad) + 12;
    for (int i = 0; i < MAX_HUNGER / 2; i++) {
        int x = hungerX + i * (iconSize + iconPad);
        bool filled = player.hunger >= (i + 1) * 2;
        bool half = !filled && player.hunger >= i * 2 + 1;
        Color c = filled ? (Color){180, 120, 40, 255} : (half ? (Color){120, 80, 30, 255} : (Color){50, 30, 15, 200});
        DrawRectangle(x, barY, iconSize, iconSize, c);
        DrawRectangleLines(x, barY, iconSize, iconSize, (Color){80, 50, 20, 200});
    }

    // Oxygen (only show when underwater or not full)
    if (player.oxygen < MAX_OXYGEN) {
        int oxyX = hungerX + (MAX_HUNGER / 2) * (iconSize + iconPad) + 12;
        for (int i = 0; i < MAX_OXYGEN / 2; i++) {
            int x = oxyX + i * (iconSize + iconPad);
            bool filled = player.oxygen >= (i + 1) * 2;
            bool half = !filled && player.oxygen >= i * 2 + 1;
            Color c = filled ? (Color){80, 180, 255, 255} : (half ? (Color){60, 130, 200, 255} : (Color){30, 60, 100, 200});
            DrawRectangle(x, barY, iconSize, iconSize, c);
            DrawRectangleLines(x, barY, iconSize, iconSize, (Color){40, 80, 120, 200});
        }
    }

    // XP bar (thin bar below hunger)
    if (player.xp > 0) {
        int xpBarX = hungerX;
        int xpBarY = barY + iconSize + 3;
        int xpBarW = (MAX_HUNGER / 2) * (iconSize + iconPad) - iconPad;
        int xpBarH = 4;
        float xpPct = (float)player.xp / MAX_XP;
        DrawRectangle(xpBarX, xpBarY, xpBarW, xpBarH, (Color){30, 30, 30, 200});
        DrawRectangle(xpBarX, xpBarY, (int)(xpBarW * xpPct), xpBarH, (Color){80, 220, 50, 220});
    }
}

//----------------------------------------------------------------------------------
// Crosshair
//----------------------------------------------------------------------------------
void DrawCrosshair(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX >= 0 && blockX < WORLD_WIDTH && blockY >= 0 && blockY < WORLD_HEIGHT) {
        float px = (float)(blockX * BLOCK_SIZE);
        float py = (float)(blockY * BLOCK_SIZE);
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 180});

        // Mining progress bar
        float progress = GetMiningProgress();
        if (progress > 0.0f) {
            int barW = BLOCK_SIZE;
            int barH = 3;
            int barX = (int)px;
            int barY = (int)py - 6;
            DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 180});
            DrawRectangle(barX, barY, (int)(barW * progress), barH, (Color){50, 200, 50, 220});
        }
    }
}

//----------------------------------------------------------------------------------
// Debug Info
//----------------------------------------------------------------------------------
void DrawDebugInfo(void)
{
    if (!showDebug) return;
    int y = 10;
    int lineH = 16;
    Color c = (Color){255, 255, 0, 200};

    int chunkCount = 0;
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX != CHUNK_EMPTY) chunkCount++;
    }

    DrawText(TextFormat("FPS: %d", GetFPS()), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Pos: %.1f, %.1f", player.position.x, player.position.y), 10, y, 14, c); y += lineH;
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    DrawText(TextFormat("Block: %d, %d", bx, by), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Chunks: %d", chunkCount), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Time: %.2f", dayNight.timeOfDay), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Light: %.2f", dayNight.lightLevel), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("OnGround: %s", player.onGround ? "yes" : "no"), 10, y, 14, c); y += lineH;
    // Player status
    DrawText(TextFormat("HP: %d/%d  Hunger: %d/%d", player.health, MAX_HEALTH, player.hunger, MAX_HUNGER), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Oxygen: %d/%d  XP: %d/%d", player.oxygen, MAX_OXYGEN, player.xp, MAX_XP), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Underwater: %s", IsPlayerUnderwater() ? "yes" : "no"), 10, y, 14, c);
}

//----------------------------------------------------------------------------------
// Message (inventory full, etc.)
//----------------------------------------------------------------------------------
void DrawMessage(void)
{
    if (messageTimer <= 0.0f) return;
    int fontSize = 20;
    int textW = MeasureText(messageText, fontSize);
    int x = (SCREEN_WIDTH - textW) / 2;
    int y = SCREEN_HEIGHT / 2 + 80;

    float alpha = messageTimer > 0.5f ? 1.0f : messageTimer * 2.0f;
    unsigned char a = (unsigned char)(alpha * 255);

    DrawRectangle(x - 12, y - 6, textW + 24, fontSize + 12, (Color){0, 0, 0, (unsigned char)(a * 0.7f)});
    DrawText(messageText, x, y, fontSize, (Color){255, 100, 100, a});
}

//----------------------------------------------------------------------------------
// Pause Menu
//----------------------------------------------------------------------------------
void DrawPauseMenu(void)
{
    if (!gamePaused) return;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});

    int boxW = 300;
    int boxH = 200;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    DrawRectangle(boxX, boxY, boxW, boxH, (Color){40, 40, 40, 240});
    DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){120, 120, 120, 255});

    DrawText("PAUSED", boxX + boxW / 2 - MeasureText("PAUSED", 28) / 2, boxY + 20, 28, WHITE);

    Vector2 mouse = GetMousePosition();

    // Continue button
    Rectangle btnContinue = { (float)(boxX + 50), (float)(boxY + 70), 200.0f, 40.0f };
    bool hoverC = CheckCollisionPointRec(mouse, btnContinue);
    DrawRectangleRec(btnContinue, hoverC ? (Color){80, 140, 80, 255} : (Color){60, 100, 60, 255});
    DrawRectangleLinesEx(btnContinue, 1, DARKGRAY);
    DrawText("Continue", (int)(btnContinue.x + 50), (int)(btnContinue.y + 10), 20, WHITE);

    if (hoverC && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        gamePaused = false;
    }

    // Save & Quit button
    Rectangle btnQuit = { (float)(boxX + 50), (float)(boxY + 130), 200.0f, 40.0f };
    bool hoverQ = CheckCollisionPointRec(mouse, btnQuit);
    DrawRectangleRec(btnQuit, hoverQ ? (Color){140, 80, 80, 255} : (Color){100, 60, 60, 255});
    DrawRectangleLinesEx(btnQuit, 1, DARKGRAY);
    DrawText("Save & Quit", (int)(btnQuit.x + 40), (int)(btnQuit.y + 10), 20, WHITE);

    if (hoverQ && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        SaveWorld(SAVE_PATH);
        CloseWindow();
        exit(0);
    }
}

//----------------------------------------------------------------------------------
// Death Screen
//----------------------------------------------------------------------------------
void DrawDeathScreen(float dt)
{
    if (player.position.y <= DEATH_Y) {
        deathFadeTimer = 0.0f;
        return;
    }

    deathFadeTimer += dt;
    float alpha = deathFadeTimer < 1.0f ? deathFadeTimer : 1.0f;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){120, 0, 0, (unsigned char)(alpha * 180)});

    if (alpha > 0.5f) {
        const char *text = "You Died!";
        int fontSize = 48;
        int textW = MeasureText(text, fontSize);
        unsigned char textA = (unsigned char)((alpha - 0.5f) * 2.0f * 255);
        DrawText(text, (SCREEN_WIDTH - textW) / 2, SCREEN_HEIGHT / 2 - 40, fontSize, (Color){255, 50, 50, textA});
    }

    if (alpha > 0.8f) {
        const char *sub = "Press any key to respawn";
        int subW = MeasureText(sub, 18);
        unsigned char subA = (unsigned char)((alpha - 0.8f) * 5.0f * 255);
        DrawText(sub, (SCREEN_WIDTH - subW) / 2, SCREEN_HEIGHT / 2 + 30, 18, (Color){200, 200, 200, subA});
    }
}

//----------------------------------------------------------------------------------
// Background (parallax clouds + mountains)
//----------------------------------------------------------------------------------
void DrawBackground(void)
{
    float camX = camera.target.x;
    float lerp = dayNight.lightLevel;

    // Distant mountain silhouettes (parallax 0.1) — subtle, clipped to sky area
    {
        float parallax = 0.1f;
        float offsetX = camX * parallax;
        int baseY = 180;
        unsigned char mr = (unsigned char)(100 + lerp * 50);
        unsigned char mg = (unsigned char)(110 + lerp * 40);
        unsigned char mb = (unsigned char)(130 + lerp * 40);
        unsigned char ma = (unsigned char)(60 + lerp * 40);  // semi-transparent
        Color mountainColor = { mr, mg, mb, ma };

        for (int x = -100; x < SCREEN_WIDTH + 100; x += 4) {
            float wx = (float)x + offsetX;
            float h = 30.0f + 20.0f * sinf(wx * 0.003f) + 15.0f * sinf(wx * 0.007f + 1.5f);
            DrawRectangle(x, baseY - (int)h, 4, (int)h, mountainColor);
        }
    }

    // Clouds (parallax 0.05, slowly drifting)
    {
        float parallax = 0.05f;
        float offsetX = camX * parallax + (float)GetTime() * 8.0f;
        Color cloudColor = { 255, 255, 255, (unsigned char)(30 + lerp * 50) };

        for (int i = 0; i < 8; i++) {
            float cx = fmodf(i * 200.0f + offsetX, (float)(SCREEN_WIDTH + 400)) - 200;
            float cy = 40 + (i * 37) % 80;
            float w = 80 + (i * 53) % 60;
            float h = 12 + (i * 23) % 12;
            DrawRectangle((int)cx, (int)cy, (int)w, (int)h, cloudColor);
            DrawRectangle((int)(cx + w * 0.2f), (int)(cy - h * 0.3f), (int)(w * 0.6f), (int)(h * 0.7f), cloudColor);
        }
    }
}
