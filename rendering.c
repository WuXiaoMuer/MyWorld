#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Held item state for inventory drag-and-drop
uint8_t heldItem = BLOCK_AIR;
int heldCount = 0;
int heldDurability = 0;

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
// Inventory Sort
//----------------------------------------------------------------------------------
void SortInventory(void)
{
    // Collect all non-empty items with their counts and durability
    typedef struct { uint8_t item; int count; int durability; } SortEntry;
    SortEntry entries[INVENTORY_SLOTS];
    int entryCount = 0;

    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] != BLOCK_AIR) {
            entries[entryCount].item = player.inventory[i];
            entries[entryCount].count = player.inventoryCount[i];
            entries[entryCount].durability = player.toolDurability[i];
            entryCount++;
        }
    }

    // Sort by type (tools first, then blocks, then food), then by item ID
    for (int i = 0; i < entryCount - 1; i++) {
        for (int j = i + 1; j < entryCount; j++) {
            bool swap = false;
            int ti = (IsTool((BlockType)entries[i].item) || IsArmor((BlockType)entries[i].item)) ? 0 : (entries[i].item >= FOOD_RAW_PORK ? 2 : 1);
            int tj = (IsTool((BlockType)entries[j].item) || IsArmor((BlockType)entries[j].item)) ? 0 : (entries[j].item >= FOOD_RAW_PORK ? 2 : 1);
            if (tj < ti) swap = true;
            else if (tj == ti && entries[j].item < entries[i].item) swap = true;
            if (swap) {
                SortEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    // Try to merge stacks of the same item (non-tools, non-armor)
    for (int i = 0; i < entryCount; i++) {
        if (IsTool((BlockType)entries[i].item) || IsArmor((BlockType)entries[i].item)) continue;
        for (int j = i + 1; j < entryCount; j++) {
            if (entries[j].item == entries[i].item && !IsTool((BlockType)entries[j].item) && !IsArmor((BlockType)entries[j].item)) {
                int space = 64 - entries[i].count;
                int toAdd = entries[j].count > space ? space : entries[j].count;
                entries[i].count += toAdd;
                entries[j].count -= toAdd;
                if (entries[j].count <= 0) {
                    // Remove entry j
                    for (int k = j; k < entryCount - 1; k++) entries[k] = entries[k + 1];
                    entryCount--;
                    j--;
                }
            }
        }
    }

    // Write back to inventory
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (i < entryCount) {
            player.inventory[i] = entries[i].item;
            player.inventoryCount[i] = entries[i].count;
            player.toolDurability[i] = entries[i].durability;
        } else {
            player.inventory[i] = BLOCK_AIR;
            player.inventoryCount[i] = 0;
            player.toolDurability[i] = 0;
        }
    }
}

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

    // Armor slots: vertical column to the left of inventory
    int armorSlotSize = 40;
    int armorPad = 3;
    int armorColW = armorSlotSize + armorPad;

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
    int totalW = panelPad + armorColW + gridW + panelPad + dividerW + panelPad + craftPanelW + panelPad;
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
    int invX = containerX + panelPad + armorColW;
    int invY = containerY + panelPad;
    Vector2 mouse = Win32GetMousePosition();
    DrawText("Inventory", invX, invY, 16, (Color){220, 210, 230, 255});
    invY += 24;

    // Armor slots (vertical, to the left of inventory)
    {
        const char *armorLabels[] = {"H", "C", "L", "B"};
        int armorX = containerX + panelPad;
        int armorY = invY;
        for (int i = 0; i < 4; i++) {
            int ay = armorY + i * (armorSlotSize + armorPad);
            Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);

            Color bg = hover ? (Color){75, 70, 85, 210} : (Color){55, 52, 62, 200};
            Color border = (Color){80, 75, 90, 200};
            DrawRectangle(armorX, ay, armorSlotSize, armorSlotSize, bg);
            DrawRectangleLines(armorX, ay, armorSlotSize, armorSlotSize, border);

            // Draw equipped armor item
            if (player.armor[i] != BLOCK_AIR && blockAtlas.id > 0) {
                int item = player.armor[i];
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(armorX + 4), (float)(ay + 4), (float)(armorSlotSize - 8), (float)(armorSlotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                // Durability bar
                int maxDur = GetArmorMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    float pct = (float)player.armorDurability[i] / maxDur;
                    int barW = armorSlotSize - 8;
                    int barX = armorX + 4;
                    int barY = ay + armorSlotSize - 5;
                    Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                    DrawRectangle(barX, barY, barW, 3, (Color){0, 0, 0, 180});
                    DrawRectangle(barX, barY, (int)(barW * pct), 3, barColor);
                }
            } else {
                // Label for empty slot
                DrawText(armorLabels[i], armorX + armorSlotSize / 2 - 4, ay + armorSlotSize / 2 - 6, 12, (Color){100, 95, 110, 150});
            }

            // Click handling for armor slots
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                    // Shift-click: auto-equip from inventory
                    if (player.armor[i] == BLOCK_AIR) {
                        for (int s = 0; s < INVENTORY_SLOTS; s++) {
                            if (IsArmor((BlockType)player.inventory[s])) {
                                int slotType = (player.inventory[s] - ARMOR_WOOD_HELMET) % 4;
                                if (slotType == i) {
                                    player.armor[i] = player.inventory[s];
                                    player.armorDurability[i] = player.toolDurability[s];
                                    player.inventory[s] = BLOCK_AIR;
                                    player.inventoryCount[s] = 0;
                                    player.toolDurability[s] = 0;
                                    PlaySoundUIClick();
                                    break;
                                }
                            }
                        }
                    }
                } else if (heldItem == BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                    // Pick up equipped armor
                    heldItem = player.armor[i];
                    heldCount = 1;
                    heldDurability = player.armorDurability[i];
                    player.armor[i] = BLOCK_AIR;
                    player.armorDurability[i] = 0;
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.armor[i] == BLOCK_AIR) {
                    // Equip held armor
                    if (IsArmor((BlockType)heldItem)) {
                        int slotType = (heldItem - ARMOR_WOOD_HELMET) % 4;
                        if (slotType == i) {
                            player.armor[i] = heldItem;
                            player.armorDurability[i] = heldDurability;
                            heldItem = BLOCK_AIR;
                            heldCount = 0;
                            heldDurability = 0;
                            PlaySoundUIClick();
                        }
                    }
                } else if (heldItem != BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                    // Swap held armor with equipped
                    if (IsArmor((BlockType)heldItem)) {
                        int slotType = (heldItem - ARMOR_WOOD_HELMET) % 4;
                        if (slotType == i) {
                            uint8_t tmpItem = player.armor[i];
                            int tmpDur = player.armorDurability[i];
                            player.armor[i] = heldItem;
                            player.armorDurability[i] = heldDurability;
                            heldItem = tmpItem;
                            heldCount = 1;
                            heldDurability = tmpDur;
                            PlaySoundUIClick();
                        }
                    }
                }
            }
        }

        // Armor tooltip
        if (heldItem == BLOCK_AIR) {
            for (int i = 0; i < 4; i++) {
                int ay = armorY + i * (armorSlotSize + armorPad);
                Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
                if (CheckCollisionPointRec(mouse, slotRect) && player.armor[i] != BLOCK_AIR) {
                    BlockType bt = (BlockType)player.armor[i];
                    const char *name = blockInfo[bt].name;
                    int tw = MeasureText(name, 12);
                    int tx = (int)mouse.x + 14;
                    int ty = (int)mouse.y - 18;
                    if (tx + tw + 8 > SCREEN_WIDTH) tx = (int)mouse.x - tw - 14;
                    if (ty < 4) ty = (int)mouse.y + 14;
                    DrawRectangle(tx - 4, ty - 2, tw + 8, 16, (Color){20, 18, 25, 230});
                    DrawRectangleLines(tx - 4, ty - 2, tw + 8, 16, (Color){80, 75, 90, 200});
                    DrawText(name, tx, ty, 12, (Color){230, 225, 240, 255});

                    char info[64] = { 0 };
                    int armorVal = GetArmorValue(bt);
                    int maxDur = GetArmorMaxDurability(bt);
                    int pct = maxDur > 0 ? player.armorDurability[i] * 100 / maxDur : 0;
                    snprintf(info, sizeof(info), "+%d armor  Dur: %d%%", armorVal, pct);
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

    // Divider line
    int divX = containerX + panelPad + armorColW + gridW + panelPad;
    DrawRectangle(divX, containerY + 8, dividerW, totalH - 16, (Color){80, 75, 90, 200});


    // Sort button
    {
        int sortBtnX = invX + 80;
        int sortBtnY = invY - 24;
        int sortBtnW = 40;
        int sortBtnH = 16;
        Rectangle sortBtn = { (float)sortBtnX, (float)sortBtnY, (float)sortBtnW, (float)sortBtnH };
        bool sortHover = CheckCollisionPointRec(mouse, sortBtn);
        Color sortBg = sortHover ? (Color){90, 85, 100, 220} : (Color){60, 55, 70, 200};
        DrawRectangle(sortBtnX, sortBtnY, sortBtnW, sortBtnH, sortBg);
        DrawRectangleLines(sortBtnX, sortBtnY, sortBtnW, sortBtnH, (Color){100, 95, 110, 180});
        DrawText("Sort", sortBtnX + 6, sortBtnY + 2, 11, (Color){180, 175, 195, 220});
        if (sortHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SortInventory();
            PlaySoundUIClick();
        }
    }

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

            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
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
                        PlaySoundUIClick();
                    }
                } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    heldItem = player.inventory[idx];
                    heldCount = player.inventoryCount[idx];
                    heldDurability = player.toolDurability[idx];
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    player.toolDurability[idx] = heldDurability;
                    heldItem = BLOCK_AIR;
                    heldCount = 0;
                    heldDurability = 0;
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                    int space = 64 - player.inventoryCount[idx];
                    int toAdd = heldCount > space ? space : heldCount;
                    player.inventoryCount[idx] += toAdd;
                    heldCount -= toAdd;
                    if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; }
                    PlaySoundUIClick();
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
                    PlaySoundUIClick();
                }
            }

            if (hover && Win32IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                PlaySoundDrop();
                player.inventoryCount[idx]--;
                if (player.inventoryCount[idx] <= 0) {
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                }
            }

            // Right-click: split stack OR place one item into matching slot
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    // Split stack
                    heldItem = player.inventory[idx];
                    heldCount = (player.inventoryCount[idx] + 1) / 2;
                    player.inventoryCount[idx] -= heldCount;
                    if (player.inventoryCount[idx] <= 0) {
                        player.inventory[idx] = BLOCK_AIR;
                        player.inventoryCount[idx] = 0;
                    }
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem && player.inventoryCount[idx] < 64) {
                    // Place one item into matching stack
                    player.inventoryCount[idx]++;
                    heldCount--;
                    if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; }
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    // Place one item into empty slot
                    player.inventory[idx] = (uint8_t)heldItem;
                    player.inventoryCount[idx] = 1;
                    player.toolDurability[idx] = (IsTool((BlockType)heldItem) && heldDurability > 0) ? heldDurability : 0;
                    heldCount--;
                    if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; }
                    PlaySoundUIClick();
                }
            }
        }
    }

    // Crafting panel
    int craftX = divX + dividerW + panelPad;
    int craftY = containerY + panelPad;
    DrawCraftingPanel(craftX, craftY, craftPanelW, visibleRecipes, craftSlotH, craftPad, craftingTableOpen);

    // Drop held item by clicking outside the container
    if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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
                    } else if (IsArmor(bt)) {
                        int armorVal = GetArmorValue(bt);
                        int maxDur = GetArmorMaxDurability(bt);
                        int pct = maxDur > 0 ? player.toolDurability[idx] * 100 / maxDur : 0;
                        snprintf(info, sizeof(info), "+%d armor  Dur: %d%%", armorVal, pct);
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

    Vector2 mouse = Win32GetMousePosition();
    int hoveredSlot = -1;

    // Background
    DrawRectangle(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){20, 18, 25, 180});
    DrawRectangleLines(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){60, 55, 70, 180});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;
        Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
        bool hover = CheckCollisionPointRec(mouse, slotRect);
        bool selected = (i == player.selectedSlot);

        if (hover) hoveredSlot = i;

        if (selected) {
            DrawRectangle(x - 2, y - 2, slotSize + 4, slotSize + 4, (Color){140, 130, 170, 80});
        }
        Color slotColor, borderColor;
        if (selected) {
            slotColor = (Color){100, 95, 110, 230};
            borderColor = (Color){180, 170, 200, 255};
        } else if (hover) {
            slotColor = (Color){65, 62, 78, 220};
            borderColor = (Color){100, 95, 120, 220};
        } else {
            slotColor = (Color){50, 47, 58, 200};
            borderColor = (Color){70, 65, 80, 200};
        }
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, borderColor);

        int item = player.inventory[i];
        if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            if (player.inventoryCount[i] > 1) {
                DrawText(TextFormat("%d", player.inventoryCount[i]),
                         x + slotSize - 20, y + slotSize - 15, 11, WHITE);
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

        // Slot number - dimmer for empty slots
        Color numColor = (item != BLOCK_AIR) ? (Color){180, 170, 190, 120} : (Color){120, 115, 130, 80};
        DrawText(TextFormat("%d", i + 1), x + 2, y + 1, 9, numColor);
    }

    // Tooltip for hovered slot
    if (hoveredSlot >= 0) {
        int item = player.inventory[hoveredSlot];
        if (item != BLOCK_AIR && item < BLOCK_COUNT) {
            const char *name = blockInfo[item].name;
            int tw = MeasureText(name, 12) + 12;
            int tx = (int)mouse.x + 12;
            int ty = (int)mouse.y - 20;
            if (tx + tw > SCREEN_WIDTH) tx = SCREEN_WIDTH - tw - 4;
            if (ty < 4) ty = 4;
            DrawRectangle(tx, ty, tw, 18, (Color){30, 28, 40, 230});
            DrawRectangleLines(tx, ty, tw, 18, (Color){80, 75, 100, 200});
            DrawText(name, tx + 6, ty + 3, 12, (Color){220, 215, 240, 255});

            // Extra info for tools/food
            char info[32] = { 0 };
            if (IsFood((BlockType)item)) {
                snprintf(info, sizeof(info), "+%d hunger", GetFoodValue((BlockType)item));
            } else if (IsTool((BlockType)item)) {
                int maxDur = GetToolMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    int pct = player.toolDurability[hoveredSlot] * 100 / maxDur;
                    snprintf(info, sizeof(info), "Dur: %d%%", pct);
                }
            }
            if (info[0]) {
                int iw = MeasureText(info, 11) + 12;
                if (iw > tw) tw = iw;
                ty += 18;
                DrawRectangle(tx, ty, tw, 15, (Color){30, 28, 40, 230});
                DrawRectangleLines(tx, ty, tw, 15, (Color){80, 75, 90, 200});
                DrawText(info, tx + 6, ty + 2, 11, (Color){180, 200, 180, 255});
            }
        }
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

    // Hunger (flash when low)
    int hungerX = barX + (MAX_HEALTH / 2) * (iconSize + iconPad) + 10;
    bool hungerLow = player.hunger <= 6;
    float hungerFlash = hungerLow ? (sinf((float)GetTime() * 4.0f) * 0.3f + 0.7f) : 1.0f;
    for (int i = 0; i < MAX_HUNGER / 2; i++) {
        int x = hungerX + i * (iconSize + iconPad);
        bool filled = player.hunger >= (i + 1) * 2;
        bool half = !filled && player.hunger >= i * 2 + 1;
        Color c = filled ? (Color){170, 110, 35, 255} : (half ? (Color){110, 75, 28, 230} : (Color){45, 28, 12, 180});
        if (hungerLow && filled) {
            c.r = (unsigned char)(c.r * hungerFlash);
            c.g = (unsigned char)(c.g * hungerFlash);
            c.b = (unsigned char)(c.b * hungerFlash);
        }
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

    // XP bar (always visible)
    {
        int xpBarX = hungerX;
        int xpBarY = barY + iconSize + 3;
        int xpBarW = (MAX_HUNGER / 2) * (iconSize + iconPad) - iconPad;
        int xpBarH = 3;
        float xpPct = (float)player.xp / MAX_XP;
        DrawRectangle(xpBarX, xpBarY, xpBarW, xpBarH, (Color){25, 25, 25, 200});
        if (player.xp > 0) {
            DrawRectangle(xpBarX, xpBarY, (int)(xpBarW * xpPct), xpBarH, (Color){70, 200, 45, 220});
        }
    }
}

//----------------------------------------------------------------------------------
// Crosshair
//----------------------------------------------------------------------------------
void DrawCrosshair(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(Win32GetMousePosition(), camera);
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

        // Block placement preview (ghost block)
        int heldItem = player.inventory[player.selectedSlot];
        BlockType cursorBlock = (BlockType)world[blockX][blockY];
        float playerCX = player.position.x + PLAYER_WIDTH / 2.0f;
        float playerCY = player.position.y + PLAYER_HEIGHT / 2.0f;
        float distBlocks = sqrtf(powf((blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCX, 2) +
                                 powf((blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCY, 2)) / BLOCK_SIZE;
        bool inRange = distBlocks <= PLACE_RANGE + 0.5f;
        if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && !IsTool((BlockType)heldItem) && !IsFood((BlockType)heldItem)
            && cursorBlock == BLOCK_AIR && blockAtlas.id > 0 && inRange) {
            Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { px, py, (float)BLOCK_SIZE, (float)BLOCK_SIZE };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, (Color){255, 255, 255, 100});
        }

        // Crosshair outline
        DrawRectangleLines((int)px - 1, (int)py - 1, BLOCK_SIZE + 2, BLOCK_SIZE + 2, (Color){0, 0, 0, 120});
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 200});

        // Mining progress bar (color by tool effectiveness)
        if (progress > 0.0f) {
            int barW = BLOCK_SIZE + 4;
            int barH = 4;
            int barX = (int)px - 2;
            int barY = (int)py - 8;
            // Determine bar color from mining speed
            BlockType heldTool = (BlockType)player.inventory[player.selectedSlot];
            BlockType minedBlock = (BlockType)world[mBlockX][mBlockY];
            float speed = GetToolMiningSpeed(heldTool, minedBlock);
            Color barColor;
            if (speed >= 3.0f) barColor = (Color){60, 220, 60, 240};       // green: fast
            else if (speed >= 2.0f) barColor = (Color){200, 200, 60, 240}; // yellow: ok
            else barColor = (Color){200, 80, 60, 240};                      // red: slow
            DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 200});
            DrawRectangle(barX + 1, barY + 1, (int)((barW - 2) * progress), barH - 2, barColor);
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
    DrawText(TextFormat("Underwater: %s", IsPlayerUnderwater() ? "yes" : "no"), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Seed: %u", worldSeed), 10, y, 14, c);
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
void DrawPauseMenu(void)
{
    if (!gamePaused) return;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});

    int boxW = 400;
    int boxH = 510;
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

    Vector2 mouse = Win32GetMousePosition();

    // --- Volume Sliders ---
    int sliderX = boxX + 28;
    int sliderW = boxW - 80;
    int sliderY = boxY + 56;

    static int activeSlider = -1; // 0=BGM, 1=SFX, -1=none

    // BGM Volume
    DrawText("Music Volume", sliderX, sliderY, 14, (Color){180, 175, 190, 255});
    sliderY += 20;
    Rectangle bgmTrack = { (float)sliderX, (float)sliderY, (float)sliderW, 6.0f };
    DrawRectangleRec(bgmTrack, (Color){35, 33, 42, 255});
    Rectangle bgmArea = { (float)(sliderX - 10), (float)(sliderY - 8), (float)(sliderW + 20), 24.0f };
    bool bgmHover = CheckCollisionPointRec(mouse, bgmArea);

    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bgmHover) activeSlider = 0;
    if (Win32IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) activeSlider = -1;
    if (activeSlider == 0) {
        bgmVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
        if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
        SetBGMVolume(bgmVolumeSlider);
    }
    DrawRectangle(sliderX, sliderY, (int)(bgmVolumeSlider * sliderW), 6, (Color){80, 180, 80, 255});
    Color bgmHandleColor = (activeSlider == 0 || bgmHover) ? (Color){255, 255, 255, 255} : (Color){200, 200, 200, 255};
    DrawRectangle((int)(sliderX + bgmVolumeSlider * sliderW) - 5, sliderY - 3, 10, 12, bgmHandleColor);
    char bgmText[16];
    sprintf(bgmText, "%d%%", (int)(bgmVolumeSlider * 100));
    DrawText(bgmText, sliderX + sliderW + 8, sliderY - 3, 13, (Color){180, 175, 190, 200});

    // SFX Volume
    sliderY += 34;
    DrawText("SFX Volume", sliderX, sliderY, 14, (Color){180, 175, 190, 255});
    sliderY += 20;
    Rectangle sfxTrack = { (float)sliderX, (float)sliderY, (float)sliderW, 6.0f };
    DrawRectangleRec(sfxTrack, (Color){35, 33, 42, 255});
    Rectangle sfxArea = { (float)(sliderX - 10), (float)(sliderY - 8), (float)(sliderW + 20), 24.0f };
    bool sfxHover = CheckCollisionPointRec(mouse, sfxArea);

    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sfxHover) activeSlider = 1;
    if (Win32IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) activeSlider = -1;
    if (activeSlider == 1) {
        sfxVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (sfxVolumeSlider < 0.0f) sfxVolumeSlider = 0.0f;
        if (sfxVolumeSlider > 1.0f) sfxVolumeSlider = 1.0f;
        SetSFXVolume(sfxVolumeSlider);
    }
    DrawRectangle(sliderX, sliderY, (int)(sfxVolumeSlider * sliderW), 6, (Color){80, 140, 200, 255});
    Color sfxHandleColor = (activeSlider == 1 || sfxHover) ? (Color){255, 255, 255, 255} : (Color){200, 200, 200, 255};
    DrawRectangle((int)(sliderX + sfxVolumeSlider * sliderW) - 5, sliderY - 3, 10, 12, sfxHandleColor);
    char sfxText[16];
    sprintf(sfxText, "%d%%", (int)(sfxVolumeSlider * 100));
    DrawText(sfxText, sliderX + sliderW + 8, sliderY - 3, 13, (Color){180, 175, 190, 200});

    // --- Controls ---
    int ctrlY = sliderY + 34;
    const char *ctrlTitle = "--- Controls ---";
    DrawText(ctrlTitle, boxX + (boxW - MeasureText(ctrlTitle, 14)) / 2, ctrlY, 14, (Color){200, 190, 100, 220});
    ctrlY += 22;

    int keyX = boxX + 28;
    int actX = boxX + 140;
    const char *keys[] = { "WASD", "Space", "Shift", "LClick", "RClick", "E", "H", "F3", "ESC", "1-9" };
    const char *acts[] = { "Move", "Jump/Swim", "Sprint", "Break/Attack", "Place/Eat", "Inventory", "Heal(XP)", "Debug", "Pause", "Hotbar" };
    int numControls = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < numControls; i++) {
        DrawText(keys[i], keyX, ctrlY, 11, (Color){220, 215, 230, 220});
        DrawText(acts[i], actX, ctrlY, 11, (Color){170, 165, 180, 220});
        ctrlY += 16;
    }

    // --- Buttons ---
    int btnW = 110;
    int btnH = 34;
    int btnY = boxY + boxH - 52;
    int btnGap = 10;
    int totalBtnW = btnW * 3 + btnGap * 2;
    int btnStartX = boxX + (boxW - totalBtnW) / 2;

    // Continue button
    Rectangle btnContinue = { (float)btnStartX, (float)btnY, (float)btnW, (float)btnH };
    bool hoverC = CheckCollisionPointRec(mouse, btnContinue);
    DrawRectangleRec(btnContinue, hoverC ? (Color){70, 130, 70, 255} : (Color){55, 95, 55, 230});
    DrawRectangleLinesEx(btnContinue, 1, hoverC ? (Color){120, 200, 120, 255} : (Color){80, 120, 80, 200});
    DrawText("Continue", (int)(btnContinue.x + (btnW - MeasureText("Continue", 16)) / 2),
             (int)(btnContinue.y + 8), 16, WHITE);

    if (hoverC && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        gamePaused = false;
    }

    // Main Menu button
    Rectangle btnMenu = { (float)(btnStartX + btnW + btnGap), (float)btnY, (float)btnW, (float)btnH };
    bool hoverM = CheckCollisionPointRec(mouse, btnMenu);
    DrawRectangleRec(btnMenu, hoverM ? (Color){70, 90, 140, 255} : (Color){50, 65, 100, 230});
    DrawRectangleLinesEx(btnMenu, 1, hoverM ? (Color){130, 170, 230, 255} : (Color){70, 90, 130, 200});
    DrawText("Main Menu", (int)(btnMenu.x + (btnW - MeasureText("Main Menu", 16)) / 2),
             (int)(btnMenu.y + 8), 16, WHITE);

    if (hoverM && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        if (currentSavePath[0]) SaveWorld(currentSavePath);
        gamePaused = false;
        inventoryOpen = false;
        gameState = STATE_MENU;
        menuSelection = 0;
    }

    // Quit button
    Rectangle btnQuit = { (float)(btnStartX + (btnW + btnGap) * 2), (float)btnY, (float)btnW, (float)btnH };
    bool hoverQ = CheckCollisionPointRec(mouse, btnQuit);
    DrawRectangleRec(btnQuit, hoverQ ? (Color){140, 60, 60, 255} : (Color){100, 45, 45, 230});
    DrawRectangleLinesEx(btnQuit, 1, hoverQ ? (Color){200, 100, 100, 255} : (Color){130, 70, 70, 200});
    DrawText("Quit", (int)(btnQuit.x + (btnW - MeasureText("Quit", 16)) / 2),
             (int)(btnQuit.y + 8), 16, WHITE);

    if (hoverQ && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        if (currentSavePath[0]) SaveWorld(currentSavePath);
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

        const char *escHint = "Press ESC to return to menu";
        int escW = MeasureText(escHint, 13);
        DrawText(escHint, (SCREEN_WIDTH - escW) / 2, SCREEN_HEIGHT / 2 + 48, 13, (Color){160, 155, 170, (unsigned char)(subA * 0.7f)});
    }
}

//----------------------------------------------------------------------------------
// Minimap
//----------------------------------------------------------------------------------
void DrawMinimap(void)
{
    int mapSize = MINIMAP_SIZE;
    int mapX = SCREEN_WIDTH - mapSize - 10;
    int mapY = 30; // below FPS counter

    // Background
    DrawRectangle(mapX - 2, mapY - 2, mapSize + 4, mapSize + 4, (Color){20, 18, 25, 200});
    DrawRectangleLines(mapX - 2, mapY - 2, mapSize + 4, mapSize + 4, (Color){80, 75, 95, 180});

    // Calculate player's block position
    int playerBX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;

    int range = MINIMAP_RANGE;

    // Draw terrain - scan columns for first non-air block (surface or water)
    for (int px = 0; px < mapSize; px++) {
        int worldBX = playerBX + (px - mapSize / 2) * range / (mapSize / 2);
        if (worldBX < 0 || worldBX >= WORLD_WIDTH) continue;

        // Find first non-air block
        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[worldBX][y] != BLOCK_AIR) {
                surfaceY = y;
                break;
            }
        }

        if (surfaceY >= 0) {
            BlockType bt = (BlockType)world[worldBX][surfaceY];
            Color c = {100, 100, 100, 255};
            if (bt == BLOCK_GRASS) c = (Color){80, 160, 60, 255};
            else if (bt == BLOCK_DIRT) c = (Color){130, 90, 50, 255};
            else if (bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE) c = (Color){120, 120, 120, 255};
            else if (bt == BLOCK_SAND) c = (Color){210, 200, 140, 255};
            else if (bt == BLOCK_WATER) c = (Color){40, 80, 160, 255};
            else if (bt == BLOCK_WOOD) c = (Color){140, 100, 50, 255};
            else if (bt == BLOCK_LEAVES) c = (Color){50, 130, 40, 255};
            else if (bt == BLOCK_COAL_ORE) c = (Color){60, 60, 60, 255};
            else if (bt == BLOCK_IRON_ORE) c = (Color){160, 140, 130, 255};
            else if (bt == BLOCK_SANDSTONE) c = (Color){190, 170, 120, 255};

            int mapPY = mapY + (surfaceY * mapSize / WORLD_HEIGHT);
            if (mapPY >= mapY && mapPY < mapY + mapSize) {
                // Draw water with slight transparency feel, terrain solid
                if (bt == BLOCK_WATER) {
                    DrawRectangle(mapX + px, mapPY, 1, mapSize - (mapPY - mapY), c);
                    // Show terrain under water if shallow
                    for (int y2 = surfaceY + 1; y2 < WORLD_HEIGHT; y2++) {
                        if (IsBlockSolid(worldBX, y2)) {
                            int underPY = mapY + (y2 * mapSize / WORLD_HEIGHT);
                            if (underPY < mapY + mapSize) {
                                DrawRectangle(mapX + px, underPY, 1, mapSize - (underPY - mapY), (Color){130, 90, 50, 200});
                            }
                            break;
                        }
                    }
                } else {
                    DrawRectangle(mapX + px, mapPY, 1, mapSize - (mapPY - mapY), c);
                }
            }
        }
    }

    // Draw mobs as dots
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        int mobBX = (int)(mobs[i].position.x + 8) / BLOCK_SIZE;
        int mobBY = (int)(mobs[i].position.y + 8) / BLOCK_SIZE;
        int dx = (mobBX - playerBX) * (mapSize / 2) / range + mapSize / 2;
        int dy = mobBY * mapSize / WORLD_HEIGHT;
        int dotX = mapX + dx;
        int dotY = mapY + dy;
        if (dotX >= mapX && dotX < mapX + mapSize && dotY >= mapY && dotY < mapY + mapSize) {
            Color dotColor;
            if (mobs[i].type == MOB_ZOMBIE) dotColor = (Color){80, 180, 60, 255};
            else if (mobs[i].type == MOB_SKELETON) dotColor = (Color){200, 200, 190, 255};
            else dotColor = (Color){220, 150, 140, 255}; // pig
            DrawRectangle(dotX - 1, dotY - 1, 3, 3, dotColor);
        }
    }

    // Draw player as white dot (center)
    int pdx = mapX + mapSize / 2;
    int pdy = mapY + playerBY * mapSize / WORLD_HEIGHT;
    DrawRectangle(pdx - 1, pdy - 1, 3, 3, WHITE);
    DrawRectangle(pdx, pdy, 1, 1, (Color){255, 255, 100, 255});

    // Border highlight
    DrawRectangleLines(mapX - 1, mapY - 1, mapSize + 2, mapSize + 2, (Color){100, 95, 115, 120});
}

//----------------------------------------------------------------------------------
// Large Map (M key)
//----------------------------------------------------------------------------------
void DrawLargeMap(void)
{
    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 200});

    int margin = 40;
    int mapX = margin;
    int mapY = margin;
    int mapW = SCREEN_WIDTH - margin * 2;
    int mapH = SCREEN_HEIGHT - margin * 2 - 30; // leave room for hint text

    // Background
    DrawRectangle(mapX - 2, mapY - 2, mapW + 4, mapH + 4, (Color){20, 18, 25, 240});
    DrawRectangleLines(mapX - 2, mapY - 2, mapW + 4, mapH + 4, (Color){80, 75, 95, 220});

    int playerBX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;

    // Show a wider range than minimap
    int rangeX = 300; // blocks visible horizontally
    int rangeY = rangeX * mapH / mapW; // maintain aspect ratio

    // Draw terrain columns
    for (int px = 0; px < mapW; px++) {
        int worldBX = playerBX + (px - mapW / 2) * rangeX / (mapW / 2);
        if (worldBX < 0 || worldBX >= WORLD_WIDTH) continue;

        for (int py = 0; py < mapH; py++) {
            int worldBY = playerBY + (py - mapH / 2) * rangeY / (mapH / 2);
            if (worldBY < 0 || worldBY >= WORLD_HEIGHT) continue;

            uint8_t bt = world[worldBX][worldBY];
            if (bt == BLOCK_AIR) continue;

            Color c = {0, 0, 0, 0};
            switch (bt) {
                case BLOCK_GRASS: c = (Color){80, 160, 60, 255}; break;
                case BLOCK_DIRT: c = (Color){130, 90, 50, 255}; break;
                case BLOCK_STONE: case BLOCK_COBBLESTONE: c = (Color){120, 120, 120, 255}; break;
                case BLOCK_SAND: c = (Color){210, 200, 140, 255}; break;
                case BLOCK_WATER: c = (Color){40, 80, 160, 180}; break;
                case BLOCK_WOOD: c = (Color){140, 100, 50, 255}; break;
                case BLOCK_LEAVES: c = (Color){50, 130, 40, 200}; break;
                case BLOCK_COAL_ORE: c = (Color){60, 60, 60, 255}; break;
                case BLOCK_IRON_ORE: c = (Color){160, 140, 130, 255}; break;
                case BLOCK_SANDSTONE: c = (Color){190, 170, 120, 255}; break;
                case BLOCK_BEDROCK: c = (Color){40, 35, 45, 255}; break;
                case BLOCK_GRAVEL: c = (Color){100, 95, 90, 255}; break;
                case BLOCK_CLAY: c = (Color){160, 150, 140, 255}; break;
                case BLOCK_BRICK: c = (Color){150, 80, 70, 255}; break;
                case BLOCK_TORCH: c = (Color){240, 200, 80, 255}; break;
                case BLOCK_FLOWER: c = (Color){220, 80, 120, 255}; break;
                case BLOCK_FURNACE: c = (Color){100, 80, 70, 255}; break;
                case BLOCK_BED: c = (Color){180, 60, 60, 255}; break;
                case BLOCK_GLASS: c = (Color){180, 200, 220, 100}; break;
                default: c = (Color){100, 100, 100, 255}; break;
            }
            if (c.a > 0) {
                DrawRectangle(mapX + px, mapY + py, 1, 1, c);
            }
        }
    }

    // Draw mobs as dots
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        int mobBX = (int)(mobs[i].position.x + 8) / BLOCK_SIZE;
        int mobBY = (int)(mobs[i].position.y + 8) / BLOCK_SIZE;
        int dx = (mobBX - playerBX) * (mapW / 2) / rangeX + mapW / 2;
        int dy = (mobBY - playerBY) * (mapH / 2) / rangeY + mapH / 2;
        int dotX = mapX + dx;
        int dotY = mapY + dy;
        if (dotX >= mapX && dotX < mapX + mapW && dotY >= mapY && dotY < mapY + mapH) {
            Color dotColor;
            if (mobs[i].type == MOB_ZOMBIE) dotColor = (Color){80, 180, 60, 255};
            else if (mobs[i].type == MOB_SKELETON) dotColor = (Color){220, 220, 200, 255};
            else dotColor = (Color){230, 160, 150, 255};
            DrawRectangle(dotX - 1, dotY - 1, 3, 3, dotColor);
        }
    }

    // Draw player as bright yellow dot
    int pdx = mapX + mapW / 2;
    int pdy = mapY + mapH / 2;
    DrawRectangle(pdx - 2, pdy - 2, 5, 5, (Color){255, 255, 100, 255});
    DrawRectangle(pdx - 1, pdy - 1, 3, 3, (Color){255, 255, 200, 255});

    // Title and hints
    DrawText("World Map", mapX, mapY - 22, 16, (Color){200, 195, 210, 255});
    DrawText("Press M to close", SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT - margin + 5, 14, (Color){150, 145, 165, 200});

    // Player coordinates
    DrawText(TextFormat("Player: %d, %d", playerBX, playerBY), mapX + mapW - 150, mapY - 22, 14, (Color){180, 175, 195, 200});
}

//----------------------------------------------------------------------------------
// Main Menu
//----------------------------------------------------------------------------------
void DrawMainMenu(void)
{
    float time = (float)GetTime();

    // Animated gradient background
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        float t = (float)y / SCREEN_HEIGHT;
        unsigned char r = (unsigned char)(15 + t * 25 + sinf(time * 0.3f) * 5);
        unsigned char g = (unsigned char)(18 + t * 20 + sinf(time * 0.4f + 1.0f) * 5);
        unsigned char b = (unsigned char)(35 + t * 30 + sinf(time * 0.2f + 2.0f) * 8);
        DrawRectangle(0, y, SCREEN_WIDTH, 4, (Color){r, g, b, 255});
    }

    // Floating block particles
    for (int i = 0; i < 15; i++) {
        float px = fmodf(i * 97.0f + time * (15.0f + i * 3.0f), (float)(SCREEN_WIDTH + 100)) - 50;
        float py = 80.0f + (i * 43) % 350 + sinf(time * 0.8f + i * 1.3f) * 20.0f;
        float size = 8.0f + (i % 4) * 4.0f;
        unsigned char alpha = (unsigned char)(30 + (i % 3) * 15);
        float rot = time * (20.0f + i * 5.0f);
        // Simple rotating square
        float cx = px + size / 2;
        float cy = py + size / 2;
        float s = size / 2;
        float cosR = cosf(rot * 0.01745f);
        float sinR = sinf(rot * 0.01745f);
        Color blockColor;
        switch (i % 6) {
            case 0: blockColor = (Color){80, 160, 60, alpha}; break;  // grass green
            case 1: blockColor = (Color){140, 100, 50, alpha}; break; // wood brown
            case 2: blockColor = (Color){120, 120, 120, alpha}; break; // stone gray
            case 3: blockColor = (Color){210, 200, 140, alpha}; break; // sand
            case 4: blockColor = (Color){50, 100, 180, alpha}; break; // water blue
            default: blockColor = (Color){180, 50, 50, alpha}; break; // brick red
        }
        DrawRectangle((int)(cx - s), (int)(cy - s), (int)size, (int)size, blockColor);
    }

    // Stars/sparkles
    for (int i = 0; i < 30; i++) {
        int sx = (i * 137 + 50) % SCREEN_WIDTH;
        int sy = (i * 89 + 30) % (SCREEN_HEIGHT - 100);
        float twinkle = sinf(time * 3.0f + i * 2.1f) * 0.5f + 0.5f;
        unsigned char a = (unsigned char)(twinkle * 120);
        DrawRectangle(sx, sy, 2, 2, (Color){200, 200, 255, a});
    }

    // Title with glow effect
    const char *title = "MyWorld";
    int titleSize = 72;
    int titleW = MeasureText(title, titleSize);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    int titleY = 80;

    // Glow behind title
    float glow = sinf(time * 1.5f) * 0.3f + 0.7f;
    unsigned char glowA = (unsigned char)(40 * glow);
    for (int r = 4; r > 0; r--) {
        DrawText(title, titleX - r, titleY, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawText(title, titleX + r, titleY, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawText(title, titleX, titleY - r, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawText(title, titleX, titleY + r, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
    }
    // Main title
    DrawText(title, titleX + 2, titleY + 2, titleSize, (Color){0, 0, 0, 120});
    DrawText(title, titleX, titleY, titleSize, (Color){240, 235, 255, 255});

    // Subtitle with fade
    const char *sub = "A 2D Sandbox Adventure";
    int subW = MeasureText(sub, 18);
    unsigned char subA = (unsigned char)(180 + sinf(time * 2.0f) * 30);
    DrawText(sub, (SCREEN_WIDTH - subW) / 2, 165, 18, (Color){180, 175, 200, subA});

    // Decorative animated line
    int lineW = 240;
    int lineX = (SCREEN_WIDTH - lineW) / 2;
    float lineProgress = sinf(time * 0.8f) * 0.5f + 0.5f;
    DrawRectangle(lineX, 195, lineW, 1, (Color){80, 75, 95, 100});
    DrawRectangle(lineX, 195, (int)(lineW * lineProgress), 1, (Color){120, 160, 220, 180});

    // Buttons
    int btnW = 260, btnH = 44;
    int btnX = (SCREEN_WIDTH - btnW) / 2;
    int btnY = 220;
    int spacing = 54;

    Vector2 mouse = Win32GetMousePosition();

    // Button data: label, selection index, colors
    const char *btnLabels[] = { "New Game", "Load Game", "Settings", "Quit" };
    int btnCount = 4;
    bool hasAnySave = false;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SaveSlotInfo info;
        if (GetSlotInfo(i, &info) && info.exists) { hasAnySave = true; break; }
    }
    bool btnEnabled[] = { true, hasAnySave, true, true };
    Color btnSelColors[] = {
        {70, 140, 70, 255}, {60, 100, 160, 255}, {100, 80, 150, 255}, {150, 50, 50, 255}
    };
    Color btnNormColors[] = {
        {45, 85, 45, 220}, {40, 65, 110, 220}, {65, 50, 100, 220}, {90, 35, 35, 220}
    };
    Color btnBorderSel[] = {
        {180, 255, 180, 255}, {150, 200, 255, 255}, {180, 160, 220, 255}, {255, 150, 150, 255}
    };
    Color btnBorderNorm[] = {
        {80, 120, 80, 200}, {70, 100, 150, 200}, {90, 75, 130, 200}, {130, 60, 60, 200}
    };

    for (int i = 0; i < btnCount; i++) {
        int by = btnY + i * spacing;
        Rectangle btn = { (float)btnX, (float)by, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (menuSelection == i);

        Color bg, border;
        if (!btnEnabled[i]) {
            bg = (Color){40, 40, 45, 150};
            border = (Color){50, 50, 55, 120};
        } else if (sel) {
            bg = btnSelColors[i];
            border = btnBorderSel[i];
        } else if (hover) {
            bg = (Color){
                (unsigned char)(btnNormColors[i].r + 15),
                (unsigned char)(btnNormColors[i].g + 15),
                (unsigned char)(btnNormColors[i].b + 15), 240
            };
            border = btnBorderNorm[i];
        } else {
            bg = btnNormColors[i];
            border = btnBorderNorm[i];
        }

        // Button glow on selected
        if (sel && btnEnabled[i]) {
            DrawRectangle(btnX - 2, by - 2, btnW + 4, btnH + 4, (Color){
                btnBorderSel[i].r, btnBorderSel[i].g, btnBorderSel[i].b, 40
            });
        }

        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);

        Color textColor = btnEnabled[i] ? WHITE : (Color){100, 100, 110, 150};
        int textW = MeasureText(btnLabels[i], 20);
        DrawText(btnLabels[i], btnX + (btnW - textW) / 2, by + 11, 20, textColor);
    }

    // Bottom hints
    const char *hint1 = "Arrow keys / WASD: Navigate   |   Enter / Space: Select";
    DrawText(hint1, (SCREEN_WIDTH - MeasureText(hint1, 13)) / 2, SCREEN_HEIGHT - 50, 13,
             (Color){160, 155, 170, 180});
    const char *hint2 = "WASD: Move  |  Space: Jump  |  E: Inventory  |  ESC: Pause  |  H: Heal (XP)";
    DrawText(hint2, (SCREEN_WIDTH - MeasureText(hint2, 11)) / 2, SCREEN_HEIGHT - 30, 11,
             (Color){130, 125, 140, 150});

    // Version
    DrawText("v0.3", 10, SCREEN_HEIGHT - 20, 11, (Color){100, 95, 110, 120});
}

//----------------------------------------------------------------------------------
// Save Slot Selection Screen
//----------------------------------------------------------------------------------
void DrawSlotSelectScreen(void)
{
    float time = (float)GetTime();

    // Background gradient
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        float t = (float)y / SCREEN_HEIGHT;
        unsigned char r = (unsigned char)(15 + t * 20);
        unsigned char g = (unsigned char)(18 + t * 15);
        unsigned char b = (unsigned char)(35 + t * 25);
        DrawRectangle(0, y, SCREEN_WIDTH, 4, (Color){r, g, b, 255});
    }

    // Title
    const char *title = (slotSelectMode == 0) ? "New Game" : "Load Game";
    int titleSize = 48;
    int titleW = MeasureText(title, titleSize);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2 + 2, 42, titleSize, (Color){0, 0, 0, 100});
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, 40, titleSize, (Color){220, 215, 240, 255});

    // Subtitle
    const char *sub = (slotSelectMode == 0) ? "Choose a slot to start a new world" : "Choose a save to continue";
    int subW = MeasureText(sub, 16);
    DrawText(sub, (SCREEN_WIDTH - subW) / 2, 100, 16, (Color){160, 155, 180, 200});

    Vector2 mouse = Win32GetMousePosition();

    // Seed input (new game mode only)
    int seedBoxY = 126;
    if (slotSelectMode == 0) {
        int seedBoxW = 240;
        int seedBoxH = 22;
        int seedBoxX = (SCREEN_WIDTH - seedBoxW) / 2;
        Rectangle seedBox = { (float)seedBoxX, (float)seedBoxY, (float)seedBoxW, (float)seedBoxH };
        bool seedFocused = CheckCollisionPointRec(mouse, seedBox);

        DrawText("Seed:", seedBoxX - 42, seedBoxY + 4, 13, (Color){180, 175, 195, 200});
        DrawRectangle(seedBoxX, seedBoxY, seedBoxW, seedBoxH, (Color){30, 28, 38, 220});
        DrawRectangleLines(seedBoxX, seedBoxY, seedBoxW, seedBoxH,
                           seedFocused ? (Color){100, 150, 220, 200} : (Color){60, 55, 75, 180});

        if (seedInputLen > 0) {
            DrawText(seedInputBuf, seedBoxX + 6, seedBoxY + 5, 12, (Color){200, 220, 200, 255});
        } else {
            DrawText("Random", seedBoxX + 6, seedBoxY + 5, 12, (Color){100, 95, 115, 150});
        }
        seedBoxY += seedBoxH + 8;
    }

    // Decorative line
    int lineW = 240;
    int lineX = (SCREEN_WIDTH - lineW) / 2;
    DrawRectangle(lineX, seedBoxY + 6, lineW, 1, (Color){100, 95, 110, 150});

    int slotW = 340, slotH = 80;
    int slotX = (SCREEN_WIDTH - slotW) / 2;
    int slotY = (slotSelectMode == 0) ? 185 : 160;
    int spacing = 96;

    // Draw visible slots
    for (int vi = 0; vi < SLOT_VISIBLE; vi++) {
        int i = vi + slotScrollOffset;
        if (i >= MAX_SAVE_SLOTS) break;

        SaveSlotInfo info;
        GetSlotInfo(i, &info);

        int sy = slotY + vi * spacing;
        Rectangle slotRect = { (float)slotX, (float)sy, (float)slotW, (float)slotH };
        bool hover = CheckCollisionPointRec(mouse, slotRect);
        bool sel = (menuSelection == i);

        // Determine if this slot is usable
        bool usable = (slotSelectMode == 0) || info.exists;

        // Background
        Color bg, border;
        if (!usable) {
            bg = (Color){35, 33, 40, 150};
            border = (Color){50, 48, 55, 120};
        } else if (sel) {
            bg = (Color){55, 70, 90, 230};
            border = (Color){120, 170, 230, 255};
        } else if (hover) {
            bg = (Color){48, 55, 68, 220};
            border = (Color){90, 100, 130, 200};
        } else {
            bg = (Color){38, 42, 55, 210};
            border = (Color){70, 75, 95, 180};
        }

        // Selection glow
        if (sel && usable) {
            DrawRectangle(slotX - 2, sy - 2, slotW + 4, slotH + 4, (Color){100, 150, 220, 35});
        }

        DrawRectangle(slotX, sy, slotW, slotH, bg);
        DrawRectangleLinesEx(slotRect, sel ? 2 : 1, border);

        // Slot number
        char slotLabel[16];
        snprintf(slotLabel, sizeof(slotLabel), "Slot %d", i + 1);
        Color labelColor = usable ? (Color){200, 195, 220, 255} : (Color){100, 95, 110, 150};
        DrawText(slotLabel, slotX + 16, sy + 10, 20, labelColor);

        if (info.exists) {
            char seedText[32];
            snprintf(seedText, sizeof(seedText), "Seed: %u", info.seed);
            DrawText(seedText, slotX + 16, sy + 36, 13, (Color){140, 180, 140, 200});

            char sizeText[32];
            snprintf(sizeText, sizeof(sizeText), "%dx%d", info.worldW, info.worldH);
            DrawText(sizeText, slotX + 16, sy + 54, 12, (Color){130, 130, 150, 180});

            const char *status = "Occupied";
            int statusW = MeasureText(status, 13);
            DrawText(status, slotX + slotW - statusW - 16, sy + 12, 13, (Color){100, 180, 100, 200});

            if (blockAtlas.id > 0) {
                Rectangle src = { (float)(BLOCK_GRASS * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(slotX + slotW - 48), (float)(sy + 36), 24, 24 };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, (Color){255, 255, 255, 150});
            }
        } else {
            const char *emptyText = (slotSelectMode == 0) ? "Empty - Click to create" : "No save data";
            Color emptyColor = (slotSelectMode == 0) ? (Color){140, 160, 180, 180} : (Color){100, 95, 110, 120};
            DrawText(emptyText, slotX + 16, sy + 40, 14, emptyColor);
        }
    }

    // Scrollbar
    if (MAX_SAVE_SLOTS > SLOT_VISIBLE) {
        int trackX = slotX + slotW + 8;
        int trackY = slotY;
        int trackH = SLOT_VISIBLE * spacing - 16;
        DrawRectangle(trackX, trackY, 4, trackH, (Color){30, 28, 35, 200});

        float viewRatio = (float)SLOT_VISIBLE / MAX_SAVE_SLOTS;
        int maxScroll = MAX_SAVE_SLOTS - SLOT_VISIBLE;
        float scrollRatio = maxScroll > 0 ? (float)slotScrollOffset / maxScroll : 0;
        int thumbH = (int)(trackH * viewRatio);
        if (thumbH < 12) thumbH = 12;
        int thumbY = trackY + (int)((trackH - thumbH) * scrollRatio);
        DrawRectangle(trackX, thumbY, 4, thumbH, (Color){120, 115, 130, 200});
    }

    // Back button
    int backBtnW = 160;
    int backBtnH = 38;
    int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
    int backBtnY = slotY + SLOT_VISIBLE * spacing + 10;
    Rectangle backBtn = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
    bool backHover = CheckCollisionPointRec(mouse, backBtn);
    Color backBg = backHover ? (Color){80, 75, 100, 240} : (Color){55, 50, 70, 220};
    DrawRectangleRec(backBtn, backBg);
    DrawRectangleLinesEx(backBtn, backHover ? 2 : 1, (Color){100, 95, 120, 200});
    const char *backText = "Back";
    int backTextW = MeasureText(backText, 18);
    DrawText(backText, backBtnX + (backBtnW - backTextW) / 2, backBtnY + 9, 18, WHITE);

    // Hints
    const char *hint = "Arrow keys / WASD: Navigate   |   Enter / Space: Select   |   DEL: Delete   |   ESC: Back";
    DrawText(hint, (SCREEN_WIDTH - MeasureText(hint, 12)) / 2, SCREEN_HEIGHT - 30, 12,
             (Color){140, 135, 155, 160});

    // Confirmation dialog overlay
    if (confirmDialogActive) {
        DrawConfirmDialog();
    }
}

//----------------------------------------------------------------------------------
// Confirmation Dialog (overwrite / delete save)
//----------------------------------------------------------------------------------
void DrawConfirmDialog(void)
{
    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    int dlgW = 340, dlgH = 160;
    int dlgX = (SCREEN_WIDTH - dlgW) / 2;
    int dlgY = (SCREEN_HEIGHT - dlgH) / 2;

    // Dialog box
    DrawRectangle(dlgX, dlgY, dlgW, dlgH, (Color){40, 38, 50, 245});
    DrawRectangleLinesEx((Rectangle){(float)dlgX, (float)dlgY, (float)dlgW, (float)dlgH}, 2,
                         (Color){100, 90, 120, 255});
    // Inner highlight
    DrawRectangle(dlgX + 2, dlgY + 2, dlgW - 4, dlgH - 4, (Color){50, 48, 62, 200});

    // Title
    bool isDelete = (confirmDialogMode == 1);
    const char *title = isDelete ? "Delete Save?" : "Overwrite Save?";
    Color titleColor = isDelete ? (Color){230, 100, 100, 255} : (Color){230, 180, 80, 255};
    int titleW = MeasureText(title, 22);
    DrawText(title, dlgX + (dlgW - titleW) / 2, dlgY + 18, 22, titleColor);

    // Message
    char msg[64];
    snprintf(msg, sizeof(msg), "Slot %d already has data.", confirmDialogSlot + 1);
    int msgW = MeasureText(msg, 14);
    DrawText(msg, dlgX + (dlgW - msgW) / 2, dlgY + 52, 14, (Color){190, 185, 200, 255});

    const char *warn = "This cannot be undone!";
    int warnW = MeasureText(warn, 13);
    DrawText(warn, dlgX + (dlgW - warnW) / 2, dlgY + 72, 13, (Color){200, 120, 100, 220});

    // Buttons
    Vector2 mouse = Win32GetMousePosition();
    int btnW = 130, btnH = 34;
    int btnY = dlgY + dlgH - 50;

    // Yes button
    Rectangle yesBtn = { (float)(dlgX + 30), (float)btnY, (float)btnW, (float)btnH };
    bool yesHover = CheckCollisionPointRec(mouse, yesBtn);
    Color yesBg = yesHover ? (Color){180, 60, 50, 255} : (Color){140, 50, 40, 230};
    DrawRectangleRec(yesBtn, yesBg);
    DrawRectangleLinesEx(yesBtn, 1, (Color){200, 80, 70, 200});
    const char *yesText = isDelete ? "Yes, Delete" : "Yes, Overwrite";
    int yesTextW = MeasureText(yesText, 14);
    DrawText(yesText, (int)(yesBtn.x + (btnW - yesTextW) / 2), btnY + 9, 14, WHITE);

    // No button
    Rectangle noBtn = { (float)(dlgX + dlgW - btnW - 30), (float)btnY, (float)btnW, (float)btnH };
    bool noHover = CheckCollisionPointRec(mouse, noBtn);
    Color noBg = noHover ? (Color){80, 80, 100, 255} : (Color){60, 58, 75, 230};
    DrawRectangleRec(noBtn, noBg);
    DrawRectangleLinesEx(noBtn, 1, (Color){90, 85, 110, 200});
    const char *noText = "Cancel";
    int noTextW = MeasureText(noText, 14);
    DrawText(noText, (int)(noBtn.x + (btnW - noTextW) / 2), btnY + 9, 14, WHITE);

    // Key hints
    const char *keyHint = "[Y] Confirm    [N / ESC] Cancel";
    int keyHintW = MeasureText(keyHint, 11);
    DrawText(keyHint, dlgX + (dlgW - keyHintW) / 2, dlgY + dlgH - 14, 11, (Color){130, 125, 145, 180});
}

//----------------------------------------------------------------------------------
// Settings Screen
//----------------------------------------------------------------------------------
void DrawSettingsScreen(void)
{
    // Background gradient
    for (int y = 0; y < SCREEN_HEIGHT; y += 4) {
        float t = (float)y / SCREEN_HEIGHT;
        unsigned char r = (unsigned char)(15 + t * 20);
        unsigned char g = (unsigned char)(18 + t * 15);
        unsigned char b = (unsigned char)(35 + t * 25);
        DrawRectangle(0, y, SCREEN_WIDTH, 4, (Color){r, g, b, 255});
    }

    // Title
    const char *title = "Settings";
    int titleW = MeasureText(title, 48);
    DrawText(title, (SCREEN_WIDTH - titleW) / 2 + 2, 32, 48, (Color){0, 0, 0, 100});
    DrawText(title, (SCREEN_WIDTH - titleW) / 2, 30, 48, (Color){220, 215, 240, 255});

    // Decorative line
    int lineW = 200;
    int lineX = (SCREEN_WIDTH - lineW) / 2;
    DrawRectangle(lineX, 88, lineW, 1, (Color){100, 95, 110, 150});

    Vector2 mouse = Win32GetMousePosition();

    // Settings panel (taller to fit all options)
    int panelW = 440;
    int panelH = 530;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = 100;

    // Panel background
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){25, 22, 32, 220});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){70, 65, 85, 200});
    DrawRectangleLines(panelX + 1, panelY + 1, panelW - 2, panelH - 2, (Color){50, 45, 65, 150});

    int sliderX = panelX + 40;
    int sliderW = 260;
    char volText[16];

    // ============================================================
    // Section: Audio
    // ============================================================
    int sectionY = panelY + 16;
    DrawText("--- Audio ---", panelX + (panelW - MeasureText("--- Audio ---", 14)) / 2, sectionY, 14, (Color){200, 190, 100, 220});
    sectionY += 24;

    // Music Volume
    DrawText("Music Volume", sliderX, sectionY, 14, (Color){200, 195, 215, 255});
    sectionY += 20;
    DrawRectangle(sliderX, sectionY + 6, sliderW, 6, (Color){40, 38, 50, 200});
    DrawRectangleLines(sliderX, sectionY + 6, sliderW, 6, (Color){60, 55, 75, 150});

    extern float bgmVolumeSlider;
    float bgmVal = bgmVolumeSlider;
    int handleX = sliderX + (int)(bgmVal * sliderW);
    Rectangle bgmHandle = { (float)(handleX - 6), (float)sectionY, 12, 18 };
    bool bgmHover = CheckCollisionPointRec(mouse, bgmHandle);
    DrawRectangle(handleX - 6, sectionY, 12, 18, bgmHover ? (Color){120, 180, 120, 255} : (Color){80, 150, 80, 255});
    DrawRectangleLines(handleX - 6, sectionY, 12, 18, (Color){160, 220, 160, 200});
    DrawRectangle(sliderX, sectionY + 6, (int)(bgmVal * sliderW), 6, (Color){80, 160, 80, 200});
    sprintf(volText, "%d%%", (int)(bgmVal * 100));
    DrawText(volText, sliderX + sliderW + 12, sectionY, 13, (Color){180, 175, 195, 220});

    Rectangle bgmTrack = { (float)sliderX, (float)(sectionY - 4), (float)sliderW, 26 };
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (bgmHover || CheckCollisionPointRec(mouse, bgmTrack))) {
        bgmVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
        if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
        SetBGMVolume(bgmVolumeSlider);
    }
    sectionY += 30;

    // SFX Volume
    DrawText("Sound Effects", sliderX, sectionY, 14, (Color){200, 195, 215, 255});
    sectionY += 20;
    DrawRectangle(sliderX, sectionY + 6, sliderW, 6, (Color){40, 38, 50, 200});
    DrawRectangleLines(sliderX, sectionY + 6, sliderW, 6, (Color){60, 55, 75, 150});

    extern float sfxVolumeSlider;
    float sfxVal = sfxVolumeSlider;
    int sfxHandleX = sliderX + (int)(sfxVal * sliderW);
    Rectangle sfxHandle = { (float)(sfxHandleX - 6), (float)sectionY, 12, 18 };
    bool sfxHover = CheckCollisionPointRec(mouse, sfxHandle);
    DrawRectangle(sfxHandleX - 6, sectionY, 12, 18, sfxHover ? (Color){120, 150, 200, 255} : (Color){80, 120, 180, 255});
    DrawRectangleLines(sfxHandleX - 6, sectionY, 12, 18, (Color){140, 180, 230, 200});
    DrawRectangle(sliderX, sectionY + 6, (int)(sfxVal * sliderW), 6, (Color){80, 120, 200, 200});
    sprintf(volText, "%d%%", (int)(sfxVal * 100));
    DrawText(volText, sliderX + sliderW + 12, sectionY, 13, (Color){180, 175, 195, 220});

    Rectangle sfxTrack = { (float)sliderX, (float)(sectionY - 4), (float)sliderW, 26 };
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (sfxHover || CheckCollisionPointRec(mouse, sfxTrack))) {
        sfxVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (sfxVolumeSlider < 0.0f) sfxVolumeSlider = 0.0f;
        if (sfxVolumeSlider > 1.0f) sfxVolumeSlider = 1.0f;
        SetSFXVolume(sfxVolumeSlider);
    }
    sectionY += 40;

    // ============================================================
    // Section: Display
    // ============================================================
    DrawText("--- Display ---", panelX + (panelW - MeasureText("--- Display ---", 14)) / 2, sectionY, 14, (Color){200, 190, 100, 220});
    sectionY += 26;

    // Window mode buttons
    DrawText("Window Mode", sliderX, sectionY, 14, (Color){200, 195, 215, 255});
    sectionY += 22;

    int btnW = 110;
    int btnH = 32;
    int btnGap = 8;
    int btnStartX = sliderX;
    const char *modeNames[] = { "Windowed", "Fullscreen", "Borderless" };
    Color modeColors[] = {
        {55, 80, 55, 230}, {55, 65, 100, 230}, {80, 55, 80, 230}
    };
    Color modeColorsSel[] = {
        {80, 140, 80, 255}, {80, 100, 160, 255}, {140, 80, 140, 255}
    };

    for (int i = 0; i < 3; i++) {
        int bx = btnStartX + i * (btnW + btnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (windowMode == i);

        Color bg = sel ? modeColorsSel[i] : (hover ? (Color){
            (unsigned char)(modeColors[i].r + 12),
            (unsigned char)(modeColors[i].g + 12),
            (unsigned char)(modeColors[i].b + 12), 240
        } : modeColors[i]);
        Color border = sel ? (Color){200, 200, 220, 255} : (Color){80, 75, 95, 180};

        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);
        int textW = MeasureText(modeNames[i], 13);
        DrawText(modeNames[i], bx + (btnW - textW) / 2, sectionY + 9, 13,
                 sel ? WHITE : (Color){180, 175, 195, 220});

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            ApplyWindowMode(i);
            PlaySoundUIClick();
        }
    }
    sectionY += btnH + 14;

    // DPI info
    Vector2 dpiScale = GetWindowScaleDPI();
    int mon = GetCurrentMonitor();
    int monW = GetMonitorWidth(mon);
    int monH = GetMonitorHeight(mon);
    char dpiInfo[64];
    snprintf(dpiInfo, sizeof(dpiInfo), "Monitor: %dx%d  DPI Scale: %.0f%%", monW, monH, dpiScale.x * 100);
    DrawText(dpiInfo, sliderX, sectionY, 12, (Color){140, 135, 155, 180});
    sectionY += 18;

    char resInfo[64];
    snprintf(resInfo, sizeof(resInfo), "Window: %dx%d", GetScreenWidth(), GetScreenHeight());
    DrawText(resInfo, sliderX, sectionY, 12, (Color){140, 135, 155, 180});
    sectionY += 28;

    // ============================================================
    // Section: Controls
    // ============================================================
    DrawText("--- Controls ---", panelX + (panelW - MeasureText("--- Controls ---", 14)) / 2, sectionY, 14, (Color){200, 190, 100, 220});
    sectionY += 24;

    const char *controls[] = {
        "WASD / Arrows",    "Move",
        "Space",            "Jump / Swim",
        "Shift",            "Sprint",
        "Left Click",       "Mine / Attack",
        "Right Click",      "Place / Eat",
        "E",                "Inventory",
        "H",                "Heal (costs XP)",
        "F3",               "Debug info",
        "F11",              "Toggle fullscreen",
        "ESC",              "Pause",
        "1-9 / Scroll",     "Select hotbar"
    };
    int numControls = sizeof(controls) / sizeof(controls[0]) / 2;
    int keyX = sliderX + 6;
    int actX = sliderX + 130;
    for (int i = 0; i < numControls; i++) {
        DrawText(controls[i * 2], keyX, sectionY, 11, (Color){200, 195, 215, 220});
        DrawText(controls[i * 2 + 1], actX, sectionY, 11, (Color){150, 145, 165, 200});
        sectionY += 16;
    }

    // Back button
    int backBtnW = 160;
    int backBtnH = 38;
    int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
    int backBtnY = panelY + panelH + 14;
    Rectangle backBtn = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
    bool backHover = CheckCollisionPointRec(mouse, backBtn);
    Color backBg = backHover ? (Color){80, 75, 100, 240} : (Color){55, 50, 70, 220};
    DrawRectangleRec(backBtn, backBg);
    DrawRectangleLinesEx(backBtn, backHover ? 2 : 1, (Color){100, 95, 120, 200});
    const char *backText = "Back";
    int backTextW = MeasureText(backText, 18);
    DrawText(backText, backBtnX + (backBtnW - backTextW) / 2, backBtnY + 9, 18, WHITE);

    // ESC to go back
    if (Win32IsKeyPressed(KEY_ESCAPE) || (backHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        gameState = STATE_MENU;
        PlaySoundUIClick();
    }
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

    // Rain (heavier at night, occasional during day)
    {
        static float rainDrops[60][2]; // x, y positions
        static bool rainInit = false;
        static float rainIntensity = 0.0f;
        float time = (float)GetTime();

        // Rain intensity: higher at night, varies with a slow sine wave
        float nightFactor = 1.0f - lerp; // 1 at night, 0 at day
        float waveFactor = sinf(time * 0.05f) * 0.3f + 0.5f; // slow variation
        float targetIntensity = nightFactor * waveFactor * 0.8f;
        // Occasional light rain during day
        if (lerp > 0.5f && sinf(time * 0.02f) > 0.7f) {
            targetIntensity = 0.15f;
        }
        rainIntensity += (targetIntensity - rainIntensity) * 0.01f;

        if (!rainInit) {
            for (int i = 0; i < 60; i++) {
                rainDrops[i][0] = (float)(rand() % SCREEN_WIDTH);
                rainDrops[i][1] = (float)(rand() % SCREEN_HEIGHT);
            }
            rainInit = true;
        }

        if (rainIntensity > 0.05f) {
            int dropCount = (int)(rainIntensity * 60);
            unsigned char rainA = (unsigned char)(rainIntensity * 120);
            for (int i = 0; i < dropCount; i++) {
                rainDrops[i][1] += 8.0f + rainIntensity * 4.0f; // fall speed
                rainDrops[i][0] += sinf(time * 3.0f + i) * 0.5f; // slight wind
                if (rainDrops[i][1] > SCREEN_HEIGHT) {
                    rainDrops[i][0] = (float)(rand() % SCREEN_WIDTH);
                    rainDrops[i][1] = -5.0f;
                }
                DrawLine((int)rainDrops[i][0], (int)rainDrops[i][1],
                         (int)(rainDrops[i][0] - 1), (int)(rainDrops[i][1] + 6),
                         (Color){150, 170, 220, rainA});
            }
        }
    }
}
