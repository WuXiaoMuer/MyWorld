#include "types.h"

void InitCraftingRecipes(void)
{
    craftRecipeCount = 0;

    #define ADD_RECIPE(in, inC, out, outC, n) do { \
        if (craftRecipeCount < MAX_CRAFT_RECIPES) \
            craftRecipes[craftRecipeCount++] = (CraftingRecipe){in, inC, out, outC, n}; \
    } while(0)

    // --- Basic materials ---
    ADD_RECIPE(BLOCK_WOOD, 1, BLOCK_PLANKS, 4, "Wood -> 4 Planks");
    ADD_RECIPE(BLOCK_PLANKS, 2, ITEM_STICK, 4, "2 Planks -> 4 Sticks");

    // --- Wood tools ---
    ADD_RECIPE(BLOCK_PLANKS, 3, TOOL_WOOD_PICKAXE, 1, "3 Planks -> Wood Pick");
    ADD_RECIPE(BLOCK_PLANKS, 3, TOOL_WOOD_AXE, 1, "3 Planks -> Wood Axe");
    ADD_RECIPE(BLOCK_PLANKS, 2, TOOL_WOOD_SWORD, 1, "2 Planks -> Wood Sword");
    ADD_RECIPE(BLOCK_PLANKS, 2, TOOL_WOOD_SHOVEL, 1, "2 Planks -> Wood Shovel");

    // --- Stone tools ---
    ADD_RECIPE(BLOCK_COBBLESTONE, 3, TOOL_STONE_PICKAXE, 1, "3 Cobble -> Stone Pick");
    ADD_RECIPE(BLOCK_COBBLESTONE, 3, TOOL_STONE_AXE, 1, "3 Cobble -> Stone Axe");
    ADD_RECIPE(BLOCK_COBBLESTONE, 2, TOOL_STONE_SWORD, 1, "2 Cobble -> Stone Sword");
    ADD_RECIPE(BLOCK_COBBLESTONE, 2, TOOL_STONE_SHOVEL, 1, "2 Cobble -> Stone Shovel");

    // --- Iron tools ---
    ADD_RECIPE(ITEM_IRON_INGOT, 3, TOOL_IRON_PICKAXE, 1, "3 Iron Ingot -> Iron Pick");
    ADD_RECIPE(ITEM_IRON_INGOT, 3, TOOL_IRON_AXE, 1, "3 Iron Ingot -> Iron Axe");
    ADD_RECIPE(ITEM_IRON_INGOT, 2, TOOL_IRON_SWORD, 1, "2 Iron Ingot -> Iron Sword");
    ADD_RECIPE(ITEM_IRON_INGOT, 2, TOOL_IRON_SHOVEL, 1, "2 Iron Ingot -> Iron Shovel");

    // --- Smelting ---
    ADD_RECIPE(BLOCK_IRON_ORE, 1, ITEM_IRON_INGOT, 1, "Iron Ore -> Iron Ingot");
    ADD_RECIPE(FOOD_RAW_PORK, 1, FOOD_COOKED_PORK, 1, "Raw Pork -> Cooked Pork");
    ADD_RECIPE(BLOCK_COBBLESTONE, 4, BLOCK_STONE, 4, "4 Cobble -> 4 Stone");
    ADD_RECIPE(BLOCK_SAND, 4, BLOCK_GLASS, 4, "4 Sand -> 4 Glass");

    // --- Block crafting ---
    ADD_RECIPE(BLOCK_STONE, 4, BLOCK_BRICK, 4, "4 Stone -> 4 Brick");
    ADD_RECIPE(BLOCK_STONE, 8, BLOCK_FURNACE, 1, "8 Stone -> Furnace");
    ADD_RECIPE(ITEM_STICK, 2, BLOCK_TORCH, 4, "2 Sticks -> 4 Torches");
    ADD_RECIPE(BLOCK_SAND, 4, BLOCK_SANDSTONE, 4, "4 Sand -> 4 Sandstone");
    ADD_RECIPE(BLOCK_GRAVEL, 4, BLOCK_COBBLESTONE, 4, "4 Gravel -> 4 Cobble");

    // --- Food ---
    ADD_RECIPE(BLOCK_PLANKS, 8, FOOD_BREAD, 1, "8 Planks -> Bread");
    ADD_RECIPE(BLOCK_LEAVES, 8, FOOD_APPLE, 1, "8 Leaves -> Apple");

    #undef ADD_RECIPE
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
        // Tools don't stack, always use new slot
        if (!IsTool(r->output)) {
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

void DrawCraftingPanel(int panelX, int panelY, int panelW, int visibleCount, int slotH, int pad)
{
    extern int craftScrollOffset; // from rendering.c

    Vector2 mouse = GetMousePosition();

    // Title
    DrawText("Crafting", panelX, panelY, 16, (Color){220, 210, 230, 255});
    panelY += 22;

    // Scroll with mouse wheel
    Rectangle panelArea = { (float)panelX, (float)panelY, (float)panelW, (float)(visibleCount * (slotH + pad)) };
    if (CheckCollisionPointRec(mouse, panelArea)) {
        int wheel = (int)GetMouseWheelMove();
        if (wheel != 0) {
            craftScrollOffset -= wheel * 2;
            if (craftScrollOffset < 0) craftScrollOffset = 0;
            if (craftScrollOffset > craftRecipeCount - visibleCount)
                craftScrollOffset = craftRecipeCount - visibleCount;
            if (craftScrollOffset < 0) craftScrollOffset = 0;
        }
    }

    int startIdx = craftScrollOffset;
    int endIdx = startIdx + visibleCount;
    if (endIdx > craftRecipeCount) endIdx = craftRecipeCount;

    for (int i = startIdx; i < endIdx; i++) {
        CraftingRecipe *r = &craftRecipes[i];
        int x = panelX;
        int slotY = panelY + (i - startIdx) * (slotH + pad);

        bool canCraft = CanCraft(i);
        Color bgColor = canCraft ? (Color){55, 75, 55, 200} : (Color){55, 45, 45, 180};

        Rectangle btnRect = { (float)x, (float)slotY, (float)panelW, (float)slotH };
        bool hover = CheckCollisionPointRec(mouse, btnRect);
        if (hover) {
            bgColor = canCraft ? (Color){70, 100, 70, 220} : (Color){70, 50, 50, 200};
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canCraft) {
                Craft(i);
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
        // Show player's count / required count
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
    if (craftRecipeCount > visibleCount) {
        int trackX = panelX + panelW - 5;
        int trackY = panelY;
        int trackH = visibleCount * (slotH + pad);
        DrawRectangle(trackX, trackY, 4, trackH, (Color){30, 28, 35, 200});

        float viewRatio = (float)visibleCount / craftRecipeCount;
        float scrollRatio = (float)craftScrollOffset / (craftRecipeCount - visibleCount);
        int thumbH = (int)(trackH * viewRatio);
        if (thumbH < 12) thumbH = 12;
        int thumbY = trackY + (int)((trackH - thumbH) * scrollRatio);
        DrawRectangle(trackX, thumbY, 4, thumbH, (Color){120, 115, 130, 200});
    }

    // Fade arrows at top/bottom when scrollable
    int listH = visibleCount * (slotH + pad);
    if (craftScrollOffset > 0) {
        DrawRectangle(panelX, panelY, panelW - 6, 8, (Color){45, 42, 50, 180});
        DrawText("^", panelX + panelW / 2 - 4, panelY - 2, 11, (Color){180, 175, 190, 150});
    }
    if (endIdx < craftRecipeCount) {
        DrawRectangle(panelX, panelY + listH - 8, panelW - 6, 8, (Color){45, 42, 50, 180});
        DrawText("v", panelX + panelW / 2 - 4, panelY + listH - 13, 11, (Color){180, 175, 190, 150});
    }

    // "No recipes" if empty
    if (craftRecipeCount == 0) {
        DrawText("No recipes", panelX + 8, panelY + 20, 14, (Color){120, 115, 130, 180});
    }
}
