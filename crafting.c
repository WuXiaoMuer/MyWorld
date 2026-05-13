#include "types.h"

void InitCraftingRecipes(void)
{
    craftRecipeCount = 0;

    // Basic material recipes
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_WOOD, 1, BLOCK_PLANKS, 4, "Wood -> 4 Planks"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 2, TOOL_WOOD_PICKAXE, 1, "2 Planks -> Wood Pick"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 2, TOOL_WOOD_AXE, 1, "2 Planks -> Wood Axe"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 1, TOOL_WOOD_SWORD, 1, "1 Plank -> Wood Sword"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 2, TOOL_WOOD_SHOVEL, 1, "2 Planks -> Wood Shovel"
    };

    // Stone tools
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 3, TOOL_STONE_PICKAXE, 1, "3 Cobble -> Stone Pick"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 3, TOOL_STONE_AXE, 1, "3 Cobble -> Stone Axe"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 2, TOOL_STONE_SWORD, 1, "2 Cobble -> Stone Sword"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 2, TOOL_STONE_SHOVEL, 1, "2 Cobble -> Stone Shovel"
    };

    // Iron tools
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_IRON_ORE, 3, TOOL_IRON_PICKAXE, 1, "3 Iron Ore -> Iron Pick"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_IRON_ORE, 3, TOOL_IRON_AXE, 1, "3 Iron Ore -> Iron Axe"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_IRON_ORE, 2, TOOL_IRON_SWORD, 1, "2 Iron Ore -> Iron Sword"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_IRON_ORE, 2, TOOL_IRON_SHOVEL, 1, "2 Iron Ore -> Iron Shovel"
    };

    // Block crafting
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 4, BLOCK_STONE, 4, "4 Cobble -> 4 Stone"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_SAND, 4, BLOCK_GLASS, 4, "4 Sand -> 4 Glass"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_STONE, 4, BLOCK_BRICK, 4, "4 Stone -> 4 Brick"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_STONE, 8, BLOCK_FURNACE, 1, "8 Stone -> Furnace"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 4, BLOCK_TORCH, 4, "4 Planks -> 4 Torches"
    };

    // New block recipes
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_SAND, 4, BLOCK_SANDSTONE, 4, "4 Sand -> 4 Sandstone"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_GRAVEL, 4, BLOCK_COBBLESTONE, 4, "4 Gravel -> 4 Cobblestone"
    };

    // Food recipes
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        FOOD_RAW_PORK, 1, FOOD_COOKED_PORK, 1, "Raw Pork -> Cooked Pork"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 3, FOOD_BREAD, 1, "3 Planks -> Bread"
    };
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_LEAVES, 4, FOOD_APPLE, 1, "4 Leaves -> Apple"
    };
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

void DrawCraftingPanel(int panelX, int panelY)
{
    int slotSize = 36;
    int padding = 3;
    int panelW = 360;
    int panelH = craftRecipeCount * (slotSize + padding) + 40;

    if (panelY + panelH > SCREEN_HEIGHT - 10) panelY = SCREEN_HEIGHT - panelH - 10;
    if (panelY < 10) panelY = 10;

    DrawRectangle(panelX, panelY, panelW, panelH, (Color){40, 40, 40, 220});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){100, 100, 100, 255});

    DrawText("Crafting", panelX + 10, panelY + 8, 18, WHITE);

    int y = panelY + 30;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < craftRecipeCount; i++) {
        CraftingRecipe *r = &craftRecipes[i];
        int x = panelX + 8;
        int slotY = y + i * (slotSize + padding);

        bool canCraft = CanCraft(i);
        Color bgColor = canCraft ? (Color){60, 80, 60, 200} : (Color){60, 40, 40, 200};

        Rectangle btnRect = { (float)x, (float)slotY, (float)(panelW - 16), (float)slotSize };
        bool hover = CheckCollisionPointRec(mouse, btnRect);
        if (hover) {
            bgColor = canCraft ? (Color){80, 120, 80, 220} : (Color){80, 50, 50, 220};
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && canCraft) {
                Craft(i);
            }
        }

        DrawRectangle((int)btnRect.x, (int)btnRect.y, (int)btnRect.width, (int)btnRect.height, bgColor);
        DrawRectangleLines((int)btnRect.x, (int)btnRect.y, (int)btnRect.width, (int)btnRect.height, DARKGRAY);

        // Input icon
        if (r->input < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle srcIn = { (float)(r->input * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dstIn = { (float)(x + 4), (float)(slotY + 2), 32, 32 };
            DrawTexturePro(blockAtlas, srcIn, dstIn, (Vector2){0, 0}, 0, WHITE);
        }
        DrawText(TextFormat("x%d", r->inputCount), x + 38, slotY + 10, 12, (Color){200, 200, 200, 255});

        DrawText("->", x + 72, slotY + 10, 12, WHITE);

        // Output icon
        if (r->output < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle srcOut = { (float)(r->output * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dstOut = { (float)(x + 100), (float)(slotY + 2), 32, 32 };
            DrawTexturePro(blockAtlas, srcOut, dstOut, (Vector2){0, 0}, 0, WHITE);
        }
        DrawText(TextFormat("x%d", r->outputCount), x + 134, slotY + 10, 12, (Color){200, 200, 200, 255});

        DrawText(r->name, x + 170, slotY + 10, 11, (Color){160, 160, 160, 255});
    }
}
