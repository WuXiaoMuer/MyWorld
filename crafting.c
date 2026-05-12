#include "types.h"

void InitCraftingRecipes(void)
{
    craftRecipeCount = 0;

    // Wood -> 4 Planks
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_WOOD, 1, BLOCK_PLANKS, 4, "Wood -> Planks"
    };
    // Cobblestone -> Stone
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_COBBLESTONE, 4, BLOCK_STONE, 4, "Cobble -> Stone"
    };
    // Sand -> Glass
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_SAND, 4, BLOCK_GLASS, 4, "Sand -> Glass"
    };
    // Stone -> Brick
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_STONE, 4, BLOCK_BRICK, 4, "Stone -> Brick"
    };
    // Planks -> Wood (reverse)
    craftRecipes[craftRecipeCount++] = (CraftingRecipe){
        BLOCK_PLANKS, 4, BLOCK_WOOD, 1, "Planks -> Wood"
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

    // Add output (may need multiple adds if outputCount > 64)
    int remaining = r->outputCount;
    while (remaining > 0) {
        int toAdd = remaining > 64 ? 64 : remaining;
        // Try to stack
        bool stacked = false;
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (player.inventory[i] == r->output && player.inventoryCount[i] + toAdd <= 64) {
                player.inventoryCount[i] += toAdd;
                stacked = true;
                break;
            }
        }
        if (!stacked) {
            // Find empty slot
            for (int i = 0; i < INVENTORY_SLOTS; i++) {
                if (player.inventory[i] == BLOCK_AIR) {
                    player.inventory[i] = r->output;
                    player.inventoryCount[i] = toAdd;
                    stacked = true;
                    break;
                }
            }
        }
        if (!stacked) break; // Inventory full
        remaining -= toAdd;
    }
}

void DrawCraftingPanel(int panelX, int panelY)
{
    int slotSize = 44;
    int padding = 4;
    int panelW = 320;
    int panelH = craftRecipeCount * (slotSize + padding) + 40;

    // Panel background
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){40, 40, 40, 220});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){100, 100, 100, 255});

    DrawText("Crafting", panelX + 10, panelY + 8, 18, WHITE);

    int y = panelY + 32;
    Vector2 mouse = GetMousePosition();

    for (int i = 0; i < craftRecipeCount; i++) {
        CraftingRecipe *r = &craftRecipes[i];
        int x = panelX + 10;
        int slotY = y + i * (slotSize + padding);

        bool canCraft = CanCraft(i);
        Color bgColor = canCraft ? (Color){60, 80, 60, 200} : (Color){60, 40, 40, 200};

        // Check hover and click
        Rectangle btnRect = { (float)x, (float)slotY, (float)(panelW - 20), (float)slotSize };
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
        Rectangle srcIn = { (float)(r->input * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dstIn = { (float)(x + 4), (float)(slotY + 4), 36, 36 };
        DrawTexturePro(blockAtlas, srcIn, dstIn, (Vector2){0, 0}, 0, WHITE);

        // Input count
        DrawText(TextFormat("x%d", r->inputCount), x + 44, slotY + 14, 14, (Color){200, 200, 200, 255});

        // Arrow
        DrawText("->", x + 90, slotY + 14, 14, WHITE);

        // Output icon
        Rectangle srcOut = { (float)(r->output * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dstOut = { (float)(x + 130), (float)(slotY + 4), 36, 36 };
        DrawTexturePro(blockAtlas, srcOut, dstOut, (Vector2){0, 0}, 0, WHITE);

        // Output count
        DrawText(TextFormat("x%d", r->outputCount), x + 170, slotY + 14, 14, (Color){200, 200, 200, 255});

        // Recipe name
        DrawText(r->name, x + 210, slotY + 14, 12, (Color){160, 160, 160, 255});
    }
}
