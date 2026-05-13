#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Held item state for inventory drag-and-drop
static uint8_t heldItem = BLOCK_AIR;
static int heldCount = 0;
static int heldDurability = 0;

void ReturnHeldItem(void)
{
    if (heldItem == BLOCK_AIR) return;
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            player.inventory[i] = heldItem;
            player.inventoryCount[i] = heldCount;
            player.toolDurability[i] = heldDurability;
            heldItem = BLOCK_AIR;
            heldCount = 0;
            heldDurability = 0;
            return;
        }
    }
    // No empty slot — item is lost (shouldn't happen normally)
    heldItem = BLOCK_AIR;
    heldCount = 0;
    heldDurability = 0;
}

// Crafting scroll state
int craftScrollOffset = 0;

// Death state
static float deathFadeTimer = 0.0f;

//----------------------------------------------------------------------------------
// Inventory Screen
//----------------------------------------------------------------------------------
void DrawInventoryScreen(void)
{
    if (!inventoryOpen) return;

    // Reset scroll when inventory first opens
    static bool prevInvOpen = false;
    if (inventoryOpen && !prevInvOpen) craftScrollOffset = 0;
    prevInvOpen = inventoryOpen;

    int slotSize = 40;
    int padding = 3;
    int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;
    int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;

    // Crafting panel dimensions
    int craftSlotH = 38;
    int craftPad = 2;
    int craftPanelW = 280;
    int visibleRecipes = 10;
    int craftVisibleH = visibleRecipes * (craftSlotH + craftPad);
    int craftPanelH = craftVisibleH + 32; // title + padding

    // Unified container
    int panelPad = 14;
    int dividerW = 2;
    int totalW = panelPad + gridW + panelPad + dividerW + panelPad + craftPanelW + panelPad;
    int totalH = panelPad + 24 + gridH + panelPad; // 24 for title
    if (craftPanelH + panelPad > totalH - panelPad) totalH = craftPanelH + panelPad * 2;

    int containerX = (SCREEN_WIDTH - totalW) / 2;
    int containerY = (SCREEN_HEIGHT - totalH) / 2;

    // Background overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});

    // Container background with subtle gradient feel
    DrawRectangle(containerX, containerY, totalW, totalH, (Color){45, 42, 50, 240});
    DrawRectangleLines(containerX, containerY, totalW, totalH, (Color){90, 85, 100, 255});
    // Inner highlight line
    DrawRectangleLines(containerX + 1, containerY + 1, totalW - 2, totalH - 2, (Color){65, 60, 75, 200});

    // Inventory title
    int invX = containerX + panelPad;
    int invY = containerY + panelPad;
    DrawText("Inventory", invX, invY, 16, (Color){220, 210, 230, 255});
    invY += 24;

    // Divider line
    int divX = containerX + panelPad + gridW + panelPad;
    DrawRectangle(divX, containerY + 8, dividerW, totalH - 16, (Color){80, 75, 90, 200});

    Vector2 mouse = GetMousePosition();

    // Inventory grid
    for (int row = 0; row < INVENTORY_ROWS; row++) {
        for (int col = 0; col < INVENTORY_COLS; col++) {
            int idx = row * INVENTORY_COLS + col;
            int x = invX + col * (slotSize + padding);
            int y = invY + row * (slotSize + padding);

            Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);
            bool selected = (row == 0 && col == player.selectedSlot);

            // Slot background
            Color bg = selected ? (Color){100, 95, 110, 220} : (hover ? (Color){75, 70, 85, 210} : (Color){55, 52, 62, 200});
            Color border = selected ? (Color){160, 150, 180, 255} : (Color){80, 75, 90, 200};
            DrawRectangle(x, y, slotSize, slotSize, bg);
            DrawRectangleLines(x, y, slotSize, slotSize, border);

            int item = player.inventory[idx];
            if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                if (player.inventoryCount[idx] > 1) {
                    DrawText(TextFormat("%d", player.inventoryCount[idx]),
                             x + slotSize - 20, y + slotSize - 14, 11, WHITE);
                }

                if (IsTool((BlockType)item)) {
                    int maxDur = GetToolMaxDurability((BlockType)player.inventory[idx]);
                    if (maxDur > 0) {
                        float pct = (float)player.toolDurability[idx] / maxDur;
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

            if (row == 0) {
                DrawText(TextFormat("%d", col + 1), x + 2, y + 1, 9, (Color){180, 170, 190, 120});
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

    // Crafting panel
    int craftX = divX + dividerW + panelPad;
    int craftY = containerY + panelPad;
    DrawCraftingPanel(craftX, craftY, craftPanelW, visibleRecipes, craftSlotH, craftPad);

    // Drop held item by clicking outside the container
    if (heldItem != BLOCK_AIR && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Rectangle container = { (float)containerX, (float)containerY, (float)totalW, (float)totalH };
        if (!CheckCollisionPointRec(mouse, container)) {
            PlaySoundDrop();
            SpawnItemEntity(heldItem, heldCount,
                            player.position.x + PLAYER_WIDTH / 2,
                            player.position.y);
            heldItem = BLOCK_AIR;
            heldCount = 0;
            heldDurability = 0;
        }
    }

    // Tooltip for hovered slot
    if (heldItem == BLOCK_AIR) {
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int idx = row * INVENTORY_COLS + col;
                int x = invX + col * (slotSize + padding);
                int y = invY + row * (slotSize + padding);
                Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                    BlockType bt = (BlockType)player.inventory[idx];
                    const char *name = blockInfo[bt].name;
                    int tw = MeasureText(name, 12);
                    int tx = (int)mouse.x + 14;
                    int ty = (int)mouse.y - 18;
                    if (tx + tw + 8 > SCREEN_WIDTH) tx = (int)mouse.x - tw - 14;
                    if (ty < 4) ty = (int)mouse.y + 14;
                    DrawRectangle(tx - 4, ty - 2, tw + 8, 16, (Color){20, 18, 25, 230});
                    DrawRectangleLines(tx - 4, ty - 2, tw + 8, 16, (Color){80, 75, 90, 200});
                    DrawText(name, tx, ty, 12, (Color){230, 225, 240, 255});

                    // Extra info line for food/tools
                    char info[64] = { 0 };
                    if (IsFood(bt)) {
                        snprintf(info, sizeof(info), "+%d hunger", GetFoodValue(bt));
                    } else if (IsTool(bt)) {
                        int maxDur = GetToolMaxDurability(bt);
                        if (maxDur > 0) {
                            int pct = player.toolDurability[idx] * 100 / maxDur;
                            snprintf(info, sizeof(info), "Durability: %d%%", pct);
                        }
                    }
                    if (info[0]) {
                        int iw = MeasureText(info, 11);
                        if (iw > tw) tw = iw;
                        ty += 16;
                        DrawRectangle(tx - 4, ty - 2, tw + 8, 15, (Color){20, 18, 25, 230});
                        DrawRectangleLines(tx - 4, ty - 2, tw + 8, 15, (Color){80, 75, 90, 200});
                        DrawText(info, tx, ty, 11, (Color){180, 200, 180, 255});
                    }
                }
            }
        }
    }

    // Held item follows mouse
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

    // World edge indicators
    int worldPixelW = WORLD_WIDTH * BLOCK_SIZE;
    int edgeW = 4;
    float viewTop = camera.target.y - (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    if (viewLeft < edgeW) {
        DrawRectangle(0, (int)viewTop, edgeW, (int)(viewBottom - viewTop), (Color){180, 40, 40, 150});
    }
    if (viewRight > worldPixelW - edgeW) {
        DrawRectangle(worldPixelW - edgeW, (int)viewTop, edgeW, (int)(viewBottom - viewTop), (Color){180, 40, 40, 150});
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
                Color wc = ApplyLighting(blockInfo[BLOCK_WATER].baseColor, wx, by);
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
    bool facing = player.facingRight;
    float armSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;
    float legSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;

    // Sprint dust particles
    if (player.sprinting && player.onGround && moving) {
        if ((int)(time * 15) % 2 == 0) {
            float dustX = facing ? px : px + PLAYER_WIDTH;
            SpawnDamageParticles(dustX, py + PLAYER_HEIGHT,
                                 (Color){180, 170, 150, 120});
        }
    }

    // Damage flash tint
    Color skin = (Color){220, 180, 140, 255};
    Color hair = (Color){80, 50, 30, 255};
    Color shirt = (Color){0, 100, 200, 255};
    Color pants = (Color){60, 40, 20, 255};
    if (player.damageFlashTimer > 0.0f) {
        skin = (Color){255, 150, 150, 255};
        shirt = (Color){100, 50, 50, 255};
    }

    if (facing) {
        // Facing right
        DrawRectangle((int)(px + 2), (int)py, 8, 8, skin);
        DrawRectangle((int)(px + 2), (int)py, 8, 3, hair);
        DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, shirt);
        // Back arm (no item)
        DrawRectangle((int)(px - 2), (int)(py + 8 + armSwing), 3, 10, skin);
        // Front arm (holds item)
        DrawRectangle((int)(px + 11), (int)(py + 8 - armSwing), 3, 10, skin);
        // Legs
        DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, pants);
        DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, pants);
    } else {
        // Facing left (mirrored)
        DrawRectangle((int)(px + 2), (int)py, 8, 8, skin);
        DrawRectangle((int)(px + 2), (int)py, 8, 3, hair);
        DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, shirt);
        // Front arm (holds item) - left side when facing left
        DrawRectangle((int)(px - 2), (int)(py + 8 - armSwing), 3, 10, skin);
        // Back arm
        DrawRectangle((int)(px + 11), (int)(py + 8 + armSwing), 3, 10, skin);
        // Legs
        DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, pants);
        DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, pants);
    }

    // Draw held item
    int heldItem = player.inventory[player.selectedSlot];
    if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
        int itemX, itemY;
        if (facing) {
            itemX = (int)(px + 12);
            itemY = (int)(py + 6 - armSwing);
        } else {
            itemX = (int)(px - 6);
            itemY = (int)(py + 6 - armSwing);
        }
        Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)itemX, (float)itemY, 8, 8 };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
    }
}

//----------------------------------------------------------------------------------
// Hotbar
//----------------------------------------------------------------------------------
void DrawHotbar(void)
{
    int slotSize = 42;
    int padding = 3;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int startY = SCREEN_HEIGHT - slotSize - 10;

    // Background
    DrawRectangle(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){20, 18, 25, 180});
    DrawRectangleLines(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){60, 55, 70, 180});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;

        bool selected = (i == player.selectedSlot);
        if (selected) {
            // Glow behind selected slot
            DrawRectangle(x - 2, y - 2, slotSize + 4, slotSize + 4, (Color){140, 130, 170, 80});
        }
        Color slotColor = selected ? (Color){100, 95, 110, 230} : (Color){50, 47, 58, 200};
        Color borderColor = selected ? (Color){180, 170, 200, 255} : (Color){70, 65, 80, 200};
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, borderColor);

        int item = player.inventory[i];
        if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            if (player.inventoryCount[i] > 1) {
                DrawText(TextFormat("%d", player.inventoryCount[i]),
                         x + slotSize - 20, y + slotSize - 14, 11, WHITE);
            }

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

        DrawText(TextFormat("%d", i + 1), x + 2, y + 1, 9, (Color){180, 170, 190, 120});
    }
}

//----------------------------------------------------------------------------------
// Player Status Bars (health, hunger, oxygen, XP)
//----------------------------------------------------------------------------------
void DrawPlayerStatus(void)
{
    int slotSize = 42;
    int padding = 3;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int barY = SCREEN_HEIGHT - slotSize - 24;

    int iconSize = 12;
    int iconPad = 2;
    int barX = startX;

    // Health hearts
    for (int i = 0; i < MAX_HEALTH / 2; i++) {
        int x = barX + i * (iconSize + iconPad);
        bool filled = player.health >= (i + 1) * 2;
        bool half = !filled && player.health >= i * 2 + 1;
        Color c = filled ? (Color){200, 45, 45, 255} : (half ? (Color){150, 40, 40, 230} : (Color){50, 18, 18, 180});
        DrawRectangle(x, barY, iconSize, iconSize, c);
        DrawRectangleLines(x, barY, iconSize, iconSize, (Color){90, 30, 30, 180});
    }

    // Hunger
    int hungerX = barX + (MAX_HEALTH / 2) * (iconSize + iconPad) + 10;
    for (int i = 0; i < MAX_HUNGER / 2; i++) {
        int x = hungerX + i * (iconSize + iconPad);
        bool filled = player.hunger >= (i + 1) * 2;
        bool half = !filled && player.hunger >= i * 2 + 1;
        Color c = filled ? (Color){170, 110, 35, 255} : (half ? (Color){110, 75, 28, 230} : (Color){45, 28, 12, 180});
        DrawRectangle(x, barY, iconSize, iconSize, c);
        DrawRectangleLines(x, barY, iconSize, iconSize, (Color){75, 48, 18, 180});
    }

    // Oxygen (only show when underwater or not full)
    if (player.oxygen < MAX_OXYGEN) {
        int oxyX = hungerX + (MAX_HUNGER / 2) * (iconSize + iconPad) + 10;
        for (int i = 0; i < MAX_OXYGEN / 2; i++) {
            int x = oxyX + i * (iconSize + iconPad);
            bool filled = player.oxygen >= (i + 1) * 2;
            bool half = !filled && player.oxygen >= i * 2 + 1;
            Color c = filled ? (Color){70, 170, 240, 255} : (half ? (Color){50, 120, 190, 230} : (Color){25, 50, 90, 180});
            DrawRectangle(x, barY, iconSize, iconSize, c);
            DrawRectangleLines(x, barY, iconSize, iconSize, (Color){35, 70, 110, 180});
        }
    }

    // XP bar
    if (player.xp > 0) {
        int xpBarX = hungerX;
        int xpBarY = barY + iconSize + 3;
        int xpBarW = (MAX_HUNGER / 2) * (iconSize + iconPad) - iconPad;
        int xpBarH = 3;
        float xpPct = (float)player.xp / MAX_XP;
        DrawRectangle(xpBarX, xpBarY, xpBarW, xpBarH, (Color){25, 25, 25, 200});
        DrawRectangle(xpBarX, xpBarY, (int)(xpBarW * xpPct), xpBarH, (Color){70, 200, 45, 220});
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

        // Mining crack overlay on the block being mined
        float progress = GetMiningProgress();
        int mBlockX = GetMiningBlockX();
        int mBlockY = GetMiningBlockY();
        if (progress > 0.0f && mBlockX >= 0 && mBlockY >= 0) {
            float mpx = (float)(mBlockX * BLOCK_SIZE);
            float mpy = (float)(mBlockY * BLOCK_SIZE);
            unsigned char crackA = (unsigned char)(120 + progress * 100);
            int cracks = (int)(progress * 5) + 1;
            // Draw crack lines radiating from center
            int cx = (int)mpx + BLOCK_SIZE / 2;
            int cy = (int)mpy + BLOCK_SIZE / 2;
            for (int i = 0; i < cracks && i < 5; i++) {
                int x1 = cx + (i * 3 - 6);
                int y1 = cy + (i * 2 - 4);
                int x2 = cx + ((i % 2) ? 6 : -6) + (i * 2 - 4);
                int y2 = cy + ((i % 2) ? -6 : 6) + (i * 3 - 6);
                // Clamp to block bounds
                if (x1 < (int)mpx) x1 = (int)mpx;
                if (x2 > (int)mpx + BLOCK_SIZE) x2 = (int)mpx + BLOCK_SIZE;
                if (y1 < (int)mpy) y1 = (int)mpy;
                if (y2 > (int)mpy + BLOCK_SIZE) y2 = (int)mpy + BLOCK_SIZE;
                DrawLine(x1, y1, x2, y2, (Color){30, 30, 30, crackA});
            }
            // Darken the block slightly
            DrawRectangle((int)mpx, (int)mpy, BLOCK_SIZE, BLOCK_SIZE, (Color){0, 0, 0, (unsigned char)(progress * 60)});
        }

        // Crosshair outline
        DrawRectangleLines((int)px - 1, (int)py - 1, BLOCK_SIZE + 2, BLOCK_SIZE + 2, (Color){0, 0, 0, 120});
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 200});

        // Mining progress bar
        if (progress > 0.0f) {
            int barW = BLOCK_SIZE + 4;
            int barH = 4;
            int barX = (int)px - 2;
            int barY = (int)py - 8;
            DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 200});
            DrawRectangle(barX + 1, barY + 1, (int)((barW - 2) * progress), barH - 2, (Color){60, 220, 60, 240});
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
void ShowMessage(const char *msg, Color color)
{
    snprintf(messageText, sizeof(messageText), "%s", msg);
    messageTimer = MESSAGE_DURATION;
    messageColor = color;
}

void DrawMessage(void)
{
    if (messageTimer <= 0.0f) return;
    int fontSize = 18;
    int textW = MeasureText(messageText, fontSize);
    int x = (SCREEN_WIDTH - textW) / 2;
    int y = SCREEN_HEIGHT / 2 + 80;

    float alpha = messageTimer > 0.5f ? 1.0f : messageTimer * 2.0f;
    unsigned char a = (unsigned char)(alpha * 255);

    DrawRectangle(x - 14, y - 6, textW + 28, fontSize + 12, (Color){20, 18, 25, (unsigned char)(a * 0.8f)});
    DrawRectangleLines(x - 14, y - 6, textW + 28, fontSize + 12, (Color){80, 75, 90, (unsigned char)(a * 0.5f)});
    DrawText(messageText, x, y, fontSize, (Color){messageColor.r, messageColor.g, messageColor.b, a});
}

//----------------------------------------------------------------------------------
// Pause Menu
//----------------------------------------------------------------------------------
static float bgmVolumeSlider = 0.3f;

void DrawPauseMenu(void)
{
    if (!gamePaused) return;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});

    int boxW = 380;
    int boxH = 470;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;

    // Container
    DrawRectangle(boxX, boxY, boxW, boxH, (Color){45, 42, 50, 240});
    DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){90, 85, 100, 255});
    DrawRectangleLines(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (Color){65, 60, 75, 200});

    // Title
    const char *title = "PAUSED";
    int titleW = MeasureText(title, 26);
    DrawText(title, boxX + (boxW - titleW) / 2, boxY + 16, 26, (Color){220, 210, 230, 255});

    Vector2 mouse = GetMousePosition();

    // --- Volume Slider ---
    int sliderX = boxX + 28;
    int sliderY = boxY + 56;
    int sliderW = boxW - 56;

    DrawText("Music Volume", sliderX, sliderY, 14, (Color){180, 175, 190, 255});
    sliderY += 22;

    Rectangle track = { (float)sliderX, (float)sliderY, (float)sliderW, 6.0f };
    DrawRectangleRec(track, (Color){35, 33, 42, 255});

    float handleX = sliderX + bgmVolumeSlider * sliderW;
    Rectangle handle = { handleX - 5.0f, (float)(sliderY - 3), 10.0f, 12.0f };

    Rectangle sliderArea = { (float)(sliderX - 10), (float)(sliderY - 8), (float)(sliderW + 20), 24.0f };
    static bool sliderDragging = false;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, sliderArea)) {
        sliderDragging = true;
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        sliderDragging = false;
    }
    if (sliderDragging) {
        bgmVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
        if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
        SetBGMVolume(bgmVolumeSlider);
    }

    DrawRectangle(sliderX, sliderY, (int)(bgmVolumeSlider * sliderW), 6, (Color){80, 180, 80, 255});
    DrawRectangleRec(handle, (Color){220, 220, 220, 255});

    char volText[16];
    sprintf(volText, "%d%%", (int)(bgmVolumeSlider * 100));
    DrawText(volText, sliderX + sliderW + 8, sliderY - 3, 13, (Color){180, 175, 190, 200});

    // --- Controls ---
    int ctrlY = sliderY + 34;
    const char *ctrlTitle = "--- Controls ---";
    DrawText(ctrlTitle, boxX + (boxW - MeasureText(ctrlTitle, 14)) / 2, ctrlY, 14, (Color){200, 190, 100, 220});
    ctrlY += 22;

    int keyX = boxX + 36;
    int actX = boxX + 160;
    const char *keys[] = { "WASD", "Space", "Shift", "Left Click", "Right Click", "E", "F3", "ESC", "1-9 / Scroll" };
    const char *acts[] = { "Move", "Jump / Swim", "Sprint", "Break / Attack", "Place / Eat", "Inventory", "Debug info", "Pause", "Select hotbar" };
    int numControls = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < numControls; i++) {
        DrawText(keys[i], keyX, ctrlY, 12, (Color){220, 215, 230, 220});
        DrawText(acts[i], actX, ctrlY, 12, (Color){170, 165, 180, 220});
        ctrlY += 17;
    }

    // --- Buttons ---
    int btnW = 150;
    int btnH = 36;
    int btnY = boxY + boxH - 56;
    int btnGap = 16;
    int totalBtnW = btnW * 2 + btnGap;
    int btnStartX = boxX + (boxW - totalBtnW) / 2;

    // Continue button
    Rectangle btnContinue = { (float)btnStartX, (float)btnY, (float)btnW, (float)btnH };
    bool hoverC = CheckCollisionPointRec(mouse, btnContinue);
    DrawRectangleRec(btnContinue, hoverC ? (Color){70, 130, 70, 255} : (Color){55, 95, 55, 230});
    DrawRectangleLinesEx(btnContinue, 1, hoverC ? (Color){120, 200, 120, 255} : (Color){80, 120, 80, 200});
    DrawText("Continue", (int)(btnContinue.x + (btnW - MeasureText("Continue", 18)) / 2),
             (int)(btnContinue.y + 8), 18, WHITE);

    if (hoverC && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        gamePaused = false;
    }

    // Save & Quit button
    Rectangle btnQuit = { (float)(btnStartX + btnW + btnGap), (float)btnY, (float)btnW, (float)btnH };
    bool hoverQ = CheckCollisionPointRec(mouse, btnQuit);
    DrawRectangleRec(btnQuit, hoverQ ? (Color){140, 60, 60, 255} : (Color){100, 45, 45, 230});
    DrawRectangleLinesEx(btnQuit, 1, hoverQ ? (Color){200, 100, 100, 255} : (Color){130, 70, 70, 200});
    DrawText("Save & Quit", (int)(btnQuit.x + (btnW - MeasureText("Save & Quit", 18)) / 2),
             (int)(btnQuit.y + 8), 18, WHITE);

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
float GetDeathFadeTimer(void) { return deathFadeTimer; }

void DrawDeathScreen(float dt)
{
    if (!player.playerDead) {
        deathFadeTimer = 0.0f;
        return;
    }

    deathFadeTimer += dt;
    float alpha = deathFadeTimer < 1.0f ? deathFadeTimer : 1.0f;

    // Dark red overlay with vignette feel
    unsigned char overlayA = (unsigned char)(alpha * 160);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){80, 0, 0, overlayA});
    // Extra dark edges
    DrawRectangle(0, 0, SCREEN_WIDTH, 60, (Color){0, 0, 0, (unsigned char)(alpha * 80)});
    DrawRectangle(0, SCREEN_HEIGHT - 60, SCREEN_WIDTH, 60, (Color){0, 0, 0, (unsigned char)(alpha * 80)});

    if (alpha > 0.4f) {
        const char *text = "You Died!";
        int fontSize = 52;
        int textW = MeasureText(text, fontSize);
        unsigned char textA = (unsigned char)((alpha - 0.4f) / 0.6f * 255);
        int tx = (SCREEN_WIDTH - textW) / 2;
        int ty = SCREEN_HEIGHT / 2 - 50;
        // Shadow
        DrawText(text, tx + 3, ty + 3, fontSize, (Color){0, 0, 0, (unsigned char)(textA * 0.5f)});
        DrawText(text, tx, ty, fontSize, (Color){220, 40, 40, textA});
    }

    if (alpha > 0.8f) {
        const char *sub = "Press Space to respawn";
        int subW = MeasureText(sub, 18);
        unsigned char subA = (unsigned char)((alpha - 0.8f) * 5.0f * 255);
        DrawText(sub, (SCREEN_WIDTH - subW) / 2, SCREEN_HEIGHT / 2 + 20, 18, (Color){200, 200, 200, subA});
    }
}

//----------------------------------------------------------------------------------
// Main Menu
//----------------------------------------------------------------------------------
void DrawMainMenu(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 100});

    // Title with shadow
    const char *title = "MyWorld";
    int titleSize = 64;
    int titleW = MeasureText(title, titleSize);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    int titleY = 110;
    DrawText(title, titleX + 2, titleY + 2, titleSize, (Color){0, 0, 0, 100});
    DrawText(title, titleX, titleY, titleSize, (Color){240, 235, 250, 255});

    // Subtitle
    const char *sub = "A 2D Sandbox Adventure";
    int subW = MeasureText(sub, 18);
    DrawText(sub, (SCREEN_WIDTH - subW) / 2, 185, 18, (Color){190, 185, 200, 200});

    // Decorative line
    int lineW = 200;
    int lineX = (SCREEN_WIDTH - lineW) / 2;
    DrawRectangle(lineX, 215, lineW, 1, (Color){100, 95, 110, 150});

    int btnW = 260, btnH = 44;
    int btnX = (SCREEN_WIDTH - btnW) / 2;
    int btnY = 250;
    int spacing = 58;

    Vector2 mouse = GetMousePosition();

    // New Game
    bool selNew = (menuSelection == 0);
    Rectangle btnNew = { (float)btnX, (float)btnY, (float)btnW, (float)btnH };
    bool hovNew = CheckCollisionPointRec(mouse, btnNew);
    Color newBg = selNew ? (Color){70, 140, 70, 255} : (hovNew ? (Color){60, 115, 60, 240} : (Color){45, 85, 45, 220});
    DrawRectangleRec(btnNew, newBg);
    DrawRectangleLinesEx(btnNew, selNew ? 2 : 1, selNew ? (Color){180, 255, 180, 255} : (Color){80, 120, 80, 200});
    DrawText("New Game", btnX + (btnW - MeasureText("New Game", 20)) / 2, btnY + 11, 20, WHITE);

    // Load Game
    bool hasSave = SaveExists(SAVE_PATH);
    bool selLoad = (menuSelection == 1);
    Rectangle btnLoad = { (float)btnX, (float)(btnY + spacing), (float)btnW, (float)btnH };
    bool hovLoad = CheckCollisionPointRec(mouse, btnLoad);
    Color loadBg = hasSave ? (selLoad ? (Color){60, 100, 160, 255} : (hovLoad ? (Color){50, 85, 140, 240} : (Color){40, 65, 110, 220}))
                           : (Color){40, 40, 45, 150};
    DrawRectangleRec(btnLoad, loadBg);
    DrawRectangleLinesEx(btnLoad, selLoad ? 2 : 1,
        hasSave ? (selLoad ? (Color){150, 200, 255, 255} : (Color){70, 100, 150, 200}) : (Color){50, 50, 55, 120});
    DrawText("Load Game", btnX + (btnW - MeasureText("Load Game", 20)) / 2, btnY + spacing + 11, 20,
             hasSave ? WHITE : (Color){100, 100, 110, 150});

    // Quit
    bool selQuit = (menuSelection == 2);
    Rectangle btnQuit = { (float)btnX, (float)(btnY + spacing * 2), (float)btnW, (float)btnH };
    bool hovQuit = CheckCollisionPointRec(mouse, btnQuit);
    Color quitBg = selQuit ? (Color){150, 50, 50, 255} : (hovQuit ? (Color){120, 45, 45, 240} : (Color){90, 35, 35, 220});
    DrawRectangleRec(btnQuit, quitBg);
    DrawRectangleLinesEx(btnQuit, selQuit ? 2 : 1, selQuit ? (Color){255, 150, 150, 255} : (Color){130, 60, 60, 200});
    DrawText("Quit", btnX + (btnW - MeasureText("Quit", 20)) / 2, btnY + spacing * 2 + 11, 20, WHITE);

    // Bottom hints
    const char *hint1 = "Arrow keys / WASD: Navigate   |   Enter / Space: Select";
    DrawText(hint1, (SCREEN_WIDTH - MeasureText(hint1, 13)) / 2, SCREEN_HEIGHT - 50, 13,
             (Color){160, 155, 170, 180});
    const char *hint2 = "WASD: Move  |  Space: Jump  |  E: Inventory  |  ESC: Pause";
    DrawText(hint2, (SCREEN_WIDTH - MeasureText(hint2, 11)) / 2, SCREEN_HEIGHT - 30, 11,
             (Color){130, 125, 140, 150});

    // Version
    DrawText("v0.1", 10, SCREEN_HEIGHT - 20, 11, (Color){100, 95, 110, 120});
}

//----------------------------------------------------------------------------------
// Background (parallax clouds + mountains)
//----------------------------------------------------------------------------------
void DrawBackground(void)
{
    float camX = camera.target.x;
    float lerp = dayNight.lightLevel;

    // Sky gradient (top to bottom)
    {
        unsigned char tr = (unsigned char)(60 + lerp * 80);
        unsigned char tg = (unsigned char)(80 + lerp * 120);
        unsigned char tb = (unsigned char)(140 + lerp * 100);
        unsigned char br = (unsigned char)(100 + lerp * 100);
        unsigned char bg = (unsigned char)(130 + lerp * 90);
        unsigned char bb = (unsigned char)(180 + lerp * 60);
        for (int y = 0; y < 200; y += 4) {
            float t = y / 200.0f;
            unsigned char r = (unsigned char)(tr + (br - tr) * t);
            unsigned char g = (unsigned char)(tg + (bg - tg) * t);
            unsigned char b = (unsigned char)(tb + (bb - tb) * t);
            DrawRectangle(0, y, SCREEN_WIDTH, 4, (Color){r, g, b, 255});
        }
    }

    // Stars at night
    if (lerp < 0.5f) {
        unsigned char starA = (unsigned char)((0.5f - lerp) * 2.0f * 180);
        for (int i = 0; i < 20; i++) {
            int sx = (i * 137 + 50) % SCREEN_WIDTH;
            int sy = (i * 89 + 20) % 140;
            int sz = 1 + (i % 2);
            float twinkle = sinf((float)GetTime() * 2.0f + i * 1.7f) * 0.3f + 0.7f;
            unsigned char a = (unsigned char)(starA * twinkle);
            DrawRectangle(sx, sy, sz, sz, (Color){255, 255, 220, a});
        }
    }

    // Distant mountain silhouettes (parallax 0.1)
    {
        float parallax = 0.1f;
        float offsetX = camX * parallax;
        int baseY = 180;
        unsigned char mr = (unsigned char)(80 + lerp * 60);
        unsigned char mg = (unsigned char)(90 + lerp * 50);
        unsigned char mb = (unsigned char)(110 + lerp * 50);
        unsigned char ma = (unsigned char)(50 + lerp * 50);
        Color mountainColor = { mr, mg, mb, ma };

        for (int x = -100; x < SCREEN_WIDTH + 100; x += 3) {
            float wx = (float)x + offsetX;
            float h = 30.0f + 20.0f * sinf(wx * 0.003f) + 15.0f * sinf(wx * 0.007f + 1.5f);
            DrawRectangle(x, baseY - (int)h, 3, (int)h, mountainColor);
        }
    }

    // Nearer hills (parallax 0.2)
    {
        float parallax = 0.2f;
        float offsetX = camX * parallax;
        int baseY = 200;
        unsigned char hr = (unsigned char)(50 + lerp * 40);
        unsigned char hg = (unsigned char)(70 + lerp * 50);
        unsigned char hb = (unsigned char)(50 + lerp * 40);
        unsigned char ha = (unsigned char)(40 + lerp * 30);
        Color hillColor = { hr, hg, hb, ha };

        for (int x = -100; x < SCREEN_WIDTH + 100; x += 3) {
            float wx = (float)x + offsetX;
            float h = 15.0f + 12.0f * sinf(wx * 0.005f + 3.0f) + 8.0f * sinf(wx * 0.011f);
            DrawRectangle(x, baseY - (int)h, 3, (int)h, hillColor);
        }
    }

    // Clouds (parallax 0.05, slowly drifting)
    {
        float parallax = 0.05f;
        float offsetX = camX * parallax + (float)GetTime() * 8.0f;
        Color cloudColor = { 255, 255, 255, (unsigned char)(25 + lerp * 55) };

        for (int i = 0; i < 8; i++) {
            float cx = fmodf(i * 200.0f + offsetX, (float)(SCREEN_WIDTH + 400)) - 200;
            float cy = 30 + (i * 37) % 70;
            float w = 80 + (i * 53) % 60;
            float h = 10 + (i * 23) % 10;
            DrawRectangle((int)cx, (int)cy, (int)w, (int)h, cloudColor);
            DrawRectangle((int)(cx + w * 0.15f), (int)(cy - h * 0.4f), (int)(w * 0.7f), (int)(h * 0.8f), cloudColor);
            DrawRectangle((int)(cx + w * 0.4f), (int)(cy - h * 0.2f), (int)(w * 0.3f), (int)(h * 0.5f), cloudColor);
        }
    }
}
