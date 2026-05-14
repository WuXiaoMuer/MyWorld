#include "types.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void InitCraftingRecipes(void)
{
    craftRecipeCount = 0;

    #define ADD_RECIPE(in, inC, out, outC, n, adv) do { \
        if (craftRecipeCount < MAX_CRAFT_RECIPES) \
            craftRecipes[craftRecipeCount++] = (CraftingRecipe){in, inC, out, outC, n, adv}; \
    } while(0)

    // --- Basic materials ---
    ADD_RECIPE(BLOCK_WOOD, 1, BLOCK_PLANKS, 4, "Wood -> 4 Planks", false);
    ADD_RECIPE(BLOCK_PLANKS, 2, ITEM_STICK, 4, "2 Planks -> 4 Sticks", false);

    // --- Wood tools ---
    ADD_RECIPE(BLOCK_PLANKS, 3, TOOL_WOOD_PICKAXE, 1, "3 Planks -> Wood Pick", false);
    ADD_RECIPE(BLOCK_PLANKS, 3, TOOL_WOOD_AXE, 1, "3 Planks -> Wood Axe", false);
    ADD_RECIPE(BLOCK_PLANKS, 2, TOOL_WOOD_SWORD, 1, "2 Planks -> Wood Sword", false);
    ADD_RECIPE(BLOCK_PLANKS, 2, TOOL_WOOD_SHOVEL, 1, "2 Planks -> Wood Shovel", false);

    // --- Stone tools ---
    ADD_RECIPE(BLOCK_COBBLESTONE, 3, TOOL_STONE_PICKAXE, 1, "3 Cobble -> Stone Pick", false);
    ADD_RECIPE(BLOCK_COBBLESTONE, 3, TOOL_STONE_AXE, 1, "3 Cobble -> Stone Axe", false);
    ADD_RECIPE(BLOCK_COBBLESTONE, 2, TOOL_STONE_SWORD, 1, "2 Cobble -> Stone Sword", false);
    ADD_RECIPE(BLOCK_COBBLESTONE, 2, TOOL_STONE_SHOVEL, 1, "2 Cobble -> Stone Shovel", false);

    // --- Iron tools ---
    ADD_RECIPE(ITEM_IRON_INGOT, 3, TOOL_IRON_PICKAXE, 1, "3 Iron -> Iron Pick", false);
    ADD_RECIPE(ITEM_IRON_INGOT, 3, TOOL_IRON_AXE, 1, "3 Iron -> Iron Axe", false);
    ADD_RECIPE(ITEM_IRON_INGOT, 2, TOOL_IRON_SWORD, 1, "2 Iron -> Iron Sword", false);
    ADD_RECIPE(ITEM_IRON_INGOT, 2, TOOL_IRON_SHOVEL, 1, "2 Iron -> Iron Shovel", false);

    // --- Block crafting ---
    ADD_RECIPE(BLOCK_STONE, 4, BLOCK_BRICK, 4, "4 Stone -> 4 Brick", false);
    ADD_RECIPE(BLOCK_STONE, 8, BLOCK_FURNACE, 1, "8 Stone -> Furnace", false);
    ADD_RECIPE(ITEM_STICK, 2, BLOCK_TORCH, 4, "2 Sticks -> 4 Torches", false);
    ADD_RECIPE(BLOCK_SAND, 4, BLOCK_SANDSTONE, 4, "4 Sand -> 4 Sandstone", false);
    ADD_RECIPE(BLOCK_GRAVEL, 4, BLOCK_COBBLESTONE, 4, "4 Gravel -> 4 Cobble", false);
    ADD_RECIPE(BLOCK_PLANKS, 6, BLOCK_BED, 1, "6 Planks -> Bed", false);
    ADD_RECIPE(BLOCK_PLANKS, 8, BLOCK_CRAFTING_TABLE, 1, "8 Planks -> Crafting Table", false);

    // --- Food ---
    ADD_RECIPE(BLOCK_PLANKS, 8, FOOD_BREAD, 1, "8 Planks -> Bread", false);
    ADD_RECIPE(BLOCK_LEAVES, 8, FOOD_APPLE, 1, "8 Leaves -> Apple", false);

    // --- Advanced recipes (require crafting table) ---
    // Wood armor
    ADD_RECIPE(BLOCK_PLANKS, 5, ARMOR_WOOD_HELMET, 1, "5 Planks -> Wood Helmet", true);
    ADD_RECIPE(BLOCK_PLANKS, 8, ARMOR_WOOD_CHESTPLATE, 1, "8 Planks -> Wood Chest", true);
    ADD_RECIPE(BLOCK_PLANKS, 7, ARMOR_WOOD_LEGGINGS, 1, "7 Planks -> Wood Legs", true);
    ADD_RECIPE(BLOCK_PLANKS, 4, ARMOR_WOOD_BOOTS, 1, "4 Planks -> Wood Boots", true);
    // Stone armor
    ADD_RECIPE(BLOCK_COBBLESTONE, 5, ARMOR_STONE_HELMET, 1, "5 Cobble -> Stone Helmet", true);
    ADD_RECIPE(BLOCK_COBBLESTONE, 8, ARMOR_STONE_CHESTPLATE, 1, "8 Cobble -> Stone Chest", true);
    ADD_RECIPE(BLOCK_COBBLESTONE, 7, ARMOR_STONE_LEGGINGS, 1, "7 Cobble -> Stone Legs", true);
    ADD_RECIPE(BLOCK_COBBLESTONE, 4, ARMOR_STONE_BOOTS, 1, "4 Cobble -> Stone Boots", true);
    // Iron armor
    ADD_RECIPE(ITEM_IRON_INGOT, 5, ARMOR_IRON_HELMET, 1, "5 Iron -> Iron Helmet", true);
    ADD_RECIPE(ITEM_IRON_INGOT, 8, ARMOR_IRON_CHESTPLATE, 1, "8 Iron -> Iron Chest", true);
    ADD_RECIPE(ITEM_IRON_INGOT, 7, ARMOR_IRON_LEGGINGS, 1, "7 Iron -> Iron Legs", true);
    ADD_RECIPE(ITEM_IRON_INGOT, 4, ARMOR_IRON_BOOTS, 1, "4 Iron -> Iron Boots", true);

    #undef ADD_RECIPE
}

void InitSmeltingRecipes(void)
{
    smeltRecipeCount = 0;
    smeltRecipes[smeltRecipeCount++] = (SmeltRecipe){BLOCK_IRON_ORE, ITEM_IRON_INGOT, "Iron Ore -> Ingot"};
    smeltRecipes[smeltRecipeCount++] = (SmeltRecipe){FOOD_RAW_PORK, FOOD_COOKED_PORK, "Raw Pork -> Cooked"};
    smeltRecipes[smeltRecipeCount++] = (SmeltRecipe){BLOCK_COBBLESTONE, BLOCK_STONE, "Cobble -> Stone"};
    smeltRecipes[smeltRecipeCount++] = (SmeltRecipe){BLOCK_SAND, BLOCK_GLASS, "Sand -> Glass"};
}

int FindSmeltRecipe(BlockType input)
{
    for (int i = 0; i < smeltRecipeCount; i++) {
        if (smeltRecipes[i].input == input) return i;
    }
    return -1;
}

bool CanCraft(int recipeIndex)
{
    if (recipeIndex < 0 || recipeIndex >= craftRecipeCount) return false;
    CraftingRecipe *r = &craftRecipes[recipeIndex];

    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == r->input && player.inventoryCount[i] >= r->inputCount) {
            return true;
        }
    }
    return false;
}

void Craft(int recipeIndex)
{
    if (!CanCraft(recipeIndex)) return;
    CraftingRecipe *r = &craftRecipes[recipeIndex];

    // Remove input
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == r->input && player.inventoryCount[i] >= r->inputCount) {
            player.inventoryCount[i] -= r->inputCount;
            if (player.inventoryCount[i] <= 0) {
                player.inventory[i] = BLOCK_AIR;
                player.inventoryCount[i] = 0;
            }
            break;
        }
    }

    // Add output
    int remaining = r->outputCount;
    while (remaining > 0) {
        int toAdd = remaining > 64 ? 64 : remaining;
        bool stacked = false;
        // Tools and armor don't stack, always use new slot
        if (!IsTool(r->output) && !IsArmor(r->output)) {
            for (int i = 0; i < INVENTORY_SLOTS; i++) {
                if (player.inventory[i] == r->output && player.inventoryCount[i] + toAdd <= 64) {
                    player.inventoryCount[i] += toAdd;
                    stacked = true;
                    break;
                }
            }
        }
        if (!stacked) {
            for (int i = 0; i < INVENTORY_SLOTS; i++) {
                if (player.inventory[i] == BLOCK_AIR) {
                    player.inventory[i] = r->output;
                    player.inventoryCount[i] = toAdd;
                    if (IsTool(r->output)) {
                        player.toolDurability[i] = GetToolMaxDurability(r->output);
                    } else if (IsArmor(r->output)) {
                        player.toolDurability[i] = GetArmorMaxDurability(r->output);
                    }
                    stacked = true;
                    break;
                }
            }
        }
        if (!stacked) break;
        remaining -= toAdd;
    }
    PlaySoundCraft();
}

void DrawCraftingPanel(int panelX, int panelY, int panelW, int visibleCount, int slotH, int pad, bool showAdvanced)
{
    extern int craftScrollOffset; // from rendering.c

    Vector2 mouse = Win32GetMousePosition();

    // Title
    const char *title = showAdvanced ? "Crafting Table" : "Crafting";
    DrawText(title, panelX, panelY, 16, (Color){220, 210, 230, 255});
    panelY += 22;

    // Search box
    int searchBoxW = panelW - 10;
    int searchBoxH = 18;
    Rectangle searchBox = { (float)panelX, (float)panelY, (float)searchBoxW, (float)searchBoxH };
    bool searchFocused = CheckCollisionPointRec(mouse, searchBox);

    DrawRectangle(panelX, panelY, searchBoxW, searchBoxH, (Color){30, 28, 38, 220});
    DrawRectangleLines(panelX, panelY, searchBoxW, searchBoxH,
                       searchFocused ? (Color){100, 150, 220, 200} : (Color){60, 55, 75, 180});

    // Handle text input for search
    if (searchFocused || craftSearchLen > 0) {
        int key = Win32GetCharPressed();
        while (key > 0) {
            if (craftSearchLen < 30 && key >= 32 && key < 127) {
                craftSearchBuf[craftSearchLen++] = (char)key;
                craftSearchBuf[craftSearchLen] = '\0';
                craftScrollOffset = 0;
            }
            key = Win32GetCharPressed();
        }
        if (Win32IsKeyPressed(KEY_BACKSPACE) && craftSearchLen > 0) {
            craftSearchLen--;
            craftSearchBuf[craftSearchLen] = '\0';
            craftScrollOffset = 0;
        }
    }

    if (craftSearchLen > 0) {
        DrawText(craftSearchBuf, panelX + 4, panelY + 3, 11, (Color){200, 200, 220, 255});
        if (searchFocused && ((int)(GetTime() * 2.0f) % 2 == 0)) {
            int cursorX = panelX + 4 + MeasureText(craftSearchBuf, 11);
            DrawRectangle(cursorX, panelY + 3, 1, 11, (Color){200, 200, 220, 200});
        }
        int clearX = panelX + searchBoxW - 14;
        DrawText("x", clearX, panelY + 3, 11, (Color){180, 100, 100, 200});
        if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse.x >= clearX && mouse.x <= clearX + 12
            && mouse.y >= panelY && mouse.y <= panelY + searchBoxH) {
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
            craftScrollOffset = 0;
        }
    } else {
        DrawText("Search...", panelX + 4, panelY + 3, 11, (Color){100, 95, 115, 150});
        if (searchFocused && ((int)(GetTime() * 2.0f) % 2 == 0)) {
            DrawRectangle(panelX + 4, panelY + 3, 1, 11, (Color){150, 145, 165, 150});
        }
    }
    panelY += searchBoxH + 4;

    // Build filtered recipe indices
    int filteredIndices[MAX_CRAFT_RECIPES];
    int filteredCount = 0;
    for (int i = 0; i < craftRecipeCount; i++) {
        // Filter by advanced: if not showing advanced, skip advanced recipes
        if (!showAdvanced && craftRecipes[i].advanced) continue;

        if (craftSearchLen == 0) {
            filteredIndices[filteredCount++] = i;
        } else {
            const char *name = craftRecipes[i].name;
            bool match = false;
            for (const char *h = name; *h; h++) {
                const char *n = craftSearchBuf;
                const char *hay = h;
                while (*n && *hay) {
                    char a = *n, b = *hay;
                    if (a >= 'a' && a <= 'z') a -= 32;
                    if (b >= 'a' && b <= 'z') b -= 32;
                    if (a != b) break;
                    n++; hay++;
                }
                if (*n == '\0') { match = true; break; }
            }
            if (match) filteredIndices[filteredCount++] = i;
        }
    }

    // Scroll with mouse wheel
    Rectangle panelArea = { (float)panelX, (float)panelY, (float)panelW, (float)(visibleCount * (slotH + pad)) };
    if (CheckCollisionPointRec(mouse, panelArea)) {
        int wheel = (int)GetMouseWheelMove();
        if (wheel != 0) {
            craftScrollOffset -= wheel * 2;
            if (craftScrollOffset < 0) craftScrollOffset = 0;
            if (craftScrollOffset > filteredCount - visibleCount)
                craftScrollOffset = filteredCount - visibleCount;
            if (craftScrollOffset < 0) craftScrollOffset = 0;
        }
    }

    int startIdx = craftScrollOffset;
    int endIdx = startIdx + visibleCount;
    if (endIdx > filteredCount) endIdx = filteredCount;

    for (int i = startIdx; i < endIdx; i++) {
        int ri = filteredIndices[i];
        CraftingRecipe *r = &craftRecipes[ri];
        int x = panelX;
        int slotY = panelY + (i - startIdx) * (slotH + pad);

        bool canCraft = CanCraft(ri);
        Color bgColor = canCraft ? (Color){55, 75, 55, 200} : (Color){55, 45, 45, 180};

        Rectangle btnRect = { (float)x, (float)slotY, (float)panelW, (float)slotH };
        bool hover = CheckCollisionPointRec(mouse, btnRect);
        if (hover) {
            bgColor = canCraft ? (Color){70, 100, 70, 220} : (Color){70, 50, 50, 200};
            if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canCraft) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                    int count = 0;
                    while (count < 10 && CanCraft(ri)) {
                        Craft(ri);
                        count++;
                    }
                } else {
                    Craft(ri);
                }
            }
        }

        DrawRectangle((int)btnRect.x, (int)btnRect.y, (int)btnRect.width, (int)btnRect.height, bgColor);
        DrawRectangleLines((int)btnRect.x, (int)btnRect.y, (int)btnRect.width, (int)btnRect.height, (Color){80, 75, 90, 150});

        int iconSize = slotH - 6;
        int textY = slotY + (slotH - 12) / 2;

        // Input icon
        if (r->input < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle srcIn = { (float)(r->input * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dstIn = { (float)(x + 3), (float)(slotY + 3), (float)iconSize, (float)iconSize };
            DrawTexturePro(blockAtlas, srcIn, dstIn, (Vector2){0, 0}, 0, WHITE);
        }
        {
            int have = 0;
            for (int s = 0; s < INVENTORY_SLOTS; s++) {
                if (player.inventory[s] == r->input) have += player.inventoryCount[s];
            }
            Color countColor = have >= r->inputCount ? (Color){120, 220, 120, 255} : (Color){200, 160, 160, 255};
            DrawText(TextFormat("%d/%d", have, r->inputCount), x + iconSize + 5, textY, 11, countColor);
        }

        DrawText(">", x + iconSize + 35, textY, 11, (Color){180, 180, 180, 200});

        // Output icon
        if (r->output < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle srcOut = { (float)(r->output * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dstOut = { (float)(x + iconSize + 50), (float)(slotY + 3), (float)iconSize, (float)iconSize };
            DrawTexturePro(blockAtlas, srcOut, dstOut, (Vector2){0, 0}, 0, WHITE);
        }
        DrawText(TextFormat("x%d", r->outputCount), x + iconSize * 2 + 52, textY, 11, (Color){200, 200, 200, 255});

        // Recipe name
        DrawText(r->name, x + iconSize * 2 + 85, textY, 10, (Color){150, 145, 160, 220});
    }

    // Scrollbar
    if (filteredCount > visibleCount) {
        int trackX = panelX + panelW - 5;
        int trackY = panelY;
        int trackH = visibleCount * (slotH + pad);
        DrawRectangle(trackX, trackY, 4, trackH, (Color){30, 28, 35, 200});

        float viewRatio = (float)visibleCount / filteredCount;
        float scrollRatio = (filteredCount > visibleCount) ? (float)craftScrollOffset / (filteredCount - visibleCount) : 0;
        int thumbH = (int)(trackH * viewRatio);
        if (thumbH < 12) thumbH = 12;
        int thumbY = trackY + (int)((trackH - thumbH) * scrollRatio);
        DrawRectangle(trackX, thumbY, 4, thumbH, (Color){120, 115, 130, 200});
    }

    // Fade arrows
    int listH = visibleCount * (slotH + pad);
    if (craftScrollOffset > 0) {
        DrawRectangle(panelX, panelY, panelW - 6, 8, (Color){45, 42, 50, 180});
        DrawText("^", panelX + panelW / 2 - 4, panelY - 2, 11, (Color){180, 175, 190, 150});
    }
    if (endIdx < filteredCount) {
        DrawRectangle(panelX, panelY + listH - 8, panelW - 6, 8, (Color){45, 42, 50, 180});
        DrawText("v", panelX + panelW / 2 - 4, panelY + listH - 13, 11, (Color){180, 175, 190, 150});
    }

    if (filteredCount == 0) {
        const char *msg = craftSearchLen > 0 ? "No matches" : "No recipes";
        DrawText(msg, panelX + 8, panelY + 20, 14, (Color){120, 115, 130, 180});
    }
}

//----------------------------------------------------------------------------------
// Furnace UI
//----------------------------------------------------------------------------------
void DrawFurnaceUI(void)
{
    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});

    int panelW = 300;
    int panelH = 220;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = (SCREEN_HEIGHT - panelH) / 2;

    // Background
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){45, 42, 50, 240});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){90, 85, 100, 255});

    // Title
    DrawText("Furnace", panelX + panelW / 2 - MeasureText("Furnace", 16) / 2, panelY + 8, 16, (Color){220, 210, 230, 255});

    int slotSize = 40;
    int slotY = panelY + 40;

    // Fuel slot (left)
    int fuelX = panelX + 30;
    DrawRectangle(fuelX, slotY, slotSize, slotSize, (Color){30, 28, 38, 220});
    DrawRectangleLines(fuelX, slotY, slotSize, slotSize, (Color){80, 75, 95, 180});
    DrawText("Fuel", fuelX, slotY - 14, 10, (Color){180, 175, 190, 200});

    // Input slot (center)
    int inputX = panelX + panelW / 2 - slotSize / 2;
    DrawRectangle(inputX, slotY, slotSize, slotSize, (Color){30, 28, 38, 220});
    DrawRectangleLines(inputX, slotY, slotSize, slotSize, (Color){80, 75, 95, 180});
    DrawText("Input", inputX, slotY - 14, 10, (Color){180, 175, 190, 200});

    // Output slot (right)
    int outputX = panelX + panelW - 30 - slotSize;
    DrawRectangle(outputX, slotY, slotSize, slotSize, (Color){30, 28, 38, 220});
    DrawRectangleLines(outputX, slotY, slotSize, slotSize, (Color){80, 75, 95, 180});
    DrawText("Output", outputX, slotY - 14, 10, (Color){180, 175, 190, 200});

    // Draw items in slots
    if (furnaceFuel != BLOCK_AIR && blockAtlas.id > 0) {
        Rectangle src = { (float)(furnaceFuel * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(fuelX + 4), (float)(slotY + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (furnaceFuelCount > 1)
            DrawText(TextFormat("%d", furnaceFuelCount), fuelX + slotSize - 18, slotY + slotSize - 14, 10, WHITE);
    }
    if (furnaceInput != BLOCK_AIR && blockAtlas.id > 0) {
        Rectangle src = { (float)(furnaceInput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(inputX + 4), (float)(slotY + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (furnaceInputCount > 1)
            DrawText(TextFormat("%d", furnaceInputCount), inputX + slotSize - 18, slotY + slotSize - 14, 10, WHITE);
    }
    if (furnaceOutput != BLOCK_AIR && blockAtlas.id > 0) {
        Rectangle src = { (float)(furnaceOutput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(outputX + 4), (float)(slotY + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (furnaceOutputCount > 1)
            DrawText(TextFormat("%d", furnaceOutputCount), outputX + slotSize - 18, slotY + slotSize - 14, 10, WHITE);
    }

    // Progress arrow between input and output
    int arrowX = inputX + slotSize + 8;
    int arrowW = outputX - inputX - slotSize - 16;
    int arrowY = slotY + slotSize / 2 - 4;
    DrawRectangle(arrowX, arrowY, arrowW, 8, (Color){40, 38, 48, 200});
    if (furnaceProgress > 0.0f) {
        int fillW = (int)(arrowW * furnaceProgress);
        DrawRectangle(arrowX, arrowY, fillW, 8, (Color){220, 160, 60, 255});
    }
    // Arrow head
    DrawTriangle(
        (Vector2){(float)(arrowX + arrowW), (float)(arrowY - 4)},
        (Vector2){(float)(arrowX + arrowW), (float)(arrowY + 12)},
        (Vector2){(float)(arrowX + arrowW + 8), (float)(arrowY + 4)},
        (Color){220, 160, 60, 255}
    );

    // Fuel flame indicator
    int flameY = slotY + slotSize + 10;
    DrawText("Fuel:", fuelX, flameY, 10, (Color){180, 175, 190, 200});
    if (furnaceFuelBurn > 0.0f) {
        int flameW = 60;
        int flameH = 8;
        float fuelPct = furnaceFuelBurn / 8.0f; // coal burns 8s
        DrawRectangle(fuelX, flameY + 14, flameW, flameH, (Color){40, 38, 48, 200});
        DrawRectangle(fuelX, flameY + 14, (int)(flameW * fuelPct), flameH, (Color){220, 120, 40, 255});
    } else {
        DrawText("No fuel", fuelX, flameY + 14, 10, (Color){150, 80, 80, 200});
    }

    // Smelting recipe info
    if (furnaceInput != BLOCK_AIR) {
        int ri = FindSmeltRecipe((BlockType)furnaceInput);
        if (ri >= 0) {
            DrawText(TextFormat("Smelting: %s", smeltRecipes[ri].name),
                     panelX + 10, panelY + panelH - 40, 10, (Color){150, 145, 160, 200});
        } else {
            DrawText("Cannot smelt this item",
                     panelX + 10, panelY + panelH - 40, 10, (Color){200, 100, 100, 200});
        }
    }

    // Close hint
    DrawText("Press E or ESC to close", panelX + panelW / 2 - MeasureText("Press E or ESC to close", 10) / 2,
             panelY + panelH - 18, 10, (Color){120, 115, 130, 180});

    // --- Click handling for furnace slots ---
    Vector2 mouse = Win32GetMousePosition();

    // Fuel slot click
    Rectangle fuelRect = { (float)fuelX, (float)slotY, (float)slotSize, (float)slotSize };
    if (CheckCollisionPointRec(mouse, fuelRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (heldItem == BLOCK_AIR && furnaceFuel != BLOCK_AIR) {
            // Pick up fuel
            heldItem = furnaceFuel;
            heldCount = furnaceFuelCount;
            heldDurability = 0;
            furnaceFuel = BLOCK_AIR;
            furnaceFuelCount = 0;
        } else if (heldItem != BLOCK_AIR && furnaceFuel == BLOCK_AIR) {
            // Place fuel (only coal accepted)
            if (heldItem == ITEM_COAL) {
                furnaceFuel = heldItem;
                furnaceFuelCount = heldCount;
                heldItem = BLOCK_AIR;
                heldCount = 0;
                heldDurability = 0;
            }
        } else if (heldItem != BLOCK_AIR && furnaceFuel == heldItem && heldItem == ITEM_COAL) {
            // Stack fuel
            int space = 64 - furnaceFuelCount;
            int toAdd = heldCount > space ? space : heldCount;
            furnaceFuelCount += toAdd;
            heldCount -= toAdd;
            if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
        }
    }

    // Input slot click
    Rectangle inputRect = { (float)inputX, (float)slotY, (float)slotSize, (float)slotSize };
    if (CheckCollisionPointRec(mouse, inputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (heldItem == BLOCK_AIR && furnaceInput != BLOCK_AIR) {
            // Pick up input
            heldItem = furnaceInput;
            heldCount = furnaceInputCount;
            heldDurability = 0;
            furnaceInput = BLOCK_AIR;
            furnaceInputCount = 0;
            furnaceProgress = 0.0f;
        } else if (heldItem != BLOCK_AIR && furnaceInput == BLOCK_AIR) {
            // Place input (only smeltable items)
            if (FindSmeltRecipe((BlockType)heldItem) >= 0) {
                furnaceInput = heldItem;
                furnaceInputCount = heldCount;
                heldItem = BLOCK_AIR;
                heldCount = 0;
                heldDurability = 0;
                furnaceProgress = 0.0f;
            }
        } else if (heldItem != BLOCK_AIR && furnaceInput == heldItem) {
            // Stack input
            int space = 64 - furnaceInputCount;
            int toAdd = heldCount > space ? space : heldCount;
            furnaceInputCount += toAdd;
            heldCount -= toAdd;
            if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
        }
    }

    // Output slot click (take only)
    Rectangle outputRect = { (float)outputX, (float)slotY, (float)slotSize, (float)slotSize };
    if (CheckCollisionPointRec(mouse, outputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (heldItem == BLOCK_AIR && furnaceOutput != BLOCK_AIR) {
            heldItem = furnaceOutput;
            heldCount = furnaceOutputCount;
            heldDurability = 0;
            furnaceOutput = BLOCK_AIR;
            furnaceOutputCount = 0;
        } else if (heldItem == furnaceOutput && furnaceOutput != BLOCK_AIR) {
            int space = 64 - heldCount;
            int toAdd = furnaceOutputCount > space ? space : furnaceOutputCount;
            heldCount += toAdd;
            furnaceOutputCount -= toAdd;
            if (furnaceOutputCount <= 0) { furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0; }
        }
    }
}
