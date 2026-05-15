#include "types.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

//----------------------------------------------------------------------------------
// Smooth hover animation system
//----------------------------------------------------------------------------------
#define MAX_HOVER_SLOTS 128
static float hoverAlphas[MAX_HOVER_SLOTS] = {0};

float GetHoverAlpha(int slotId, bool hovered, float dt)
{
    if (slotId < 0 || slotId >= MAX_HOVER_SLOTS) return hovered ? 1.0f : 0.0f;
    float target = hovered ? 1.0f : 0.0f;
    float speed = 8.0f; // reaches target in ~0.125s
    hoverAlphas[slotId] += (target - hoverAlphas[slotId]) * speed * dt;
    if (hoverAlphas[slotId] < 0.01f) hoverAlphas[slotId] = 0.0f;
    if (hoverAlphas[slotId] > 0.99f) hoverAlphas[slotId] = 1.0f;
    return hoverAlphas[slotId];
}

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

    // --- Minecraft-style combined furnace + inventory screen ---
    if (furnaceOpen) {
        int padding = 3;
        int armorSlotSize = 40;
        int armorPad = 3;
        int armorColW = armorSlotSize + armorPad;
        int previewW = 60;
        int previewPad = 6;
        int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;

        int contW = 14 + previewW + previewPad + armorColW + gridW + 14;
        int furnaceH = 175;
        int dividerH = 2;
        int invTitleH = 20;
        int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
        int contH = furnaceH + dividerH + invTitleH + gridH + 14;

        int contX = (SCREEN_WIDTH - contW) / 2;
        int contY = (SCREEN_HEIGHT - contH) / 2;
        int panelPad = 14;

        Vector2 mouse = Win32GetMousePosition();

        // Light overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 100});

        // Container
        DrawRectangle(contX, contY, contW, contH, (Color){45, 42, 50, 240});
        DrawRectangleLines(contX, contY, contW, contH, (Color){90, 85, 100, 255});
        DrawRectangleLines(contX + 1, contY + 1, contW - 2, contH - 2, (Color){65, 60, 75, 200});

        // --- Furnace section ---
        int fuSlotSize = 40;
        int fuY = contY + panelPad;
        DrawGameText(S(STR_FURNACE), contX + contW / 2 - MeasureGameTextWidth(S(STR_FURNACE), 16) / 2, fuY, 16, (Color){220, 210, 230, 255});
        fuY += 28;

        int fuSlotY = fuY;
        int fuCenterX = contX + contW / 2;
        int fuelX = fuCenterX - 120;
        int inputX = fuCenterX - fuSlotSize / 2;
        int outputX = fuCenterX + 80;

        // Fuel slot
        Rectangle fuelRect = { (float)fuelX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool fuelHover = CheckCollisionPointRec(mouse, fuelRect);
        int fuelHoverId = 500;
        float fuelHA = GetHoverAlpha(fuelHoverId, fuelHover, GetFrameTime());
        Color fuelBg = { (unsigned char)(55 + 20 * fuelHA), (unsigned char)(52 + 18 * fuelHA), (unsigned char)(62 + 23 * fuelHA), (unsigned char)(200 + 10 * fuelHA) };
        Color fuelBd = { (unsigned char)(80 + 30 * fuelHA), (unsigned char)(75 + 30 * fuelHA), (unsigned char)(90 + 40 * fuelHA), 200 };
        if (fuelHA > 0.01f) DrawRectangle(fuelX - 1, fuSlotY - 1, fuSlotSize + 2, fuSlotSize + 2, (Color){100, 95, 120, (unsigned char)(35 * fuelHA)});
        DrawRectangle(fuelX, fuSlotY, fuSlotSize, fuSlotSize, fuelBg);
        DrawRectangle(fuelX, fuSlotY, fuSlotSize, fuSlotSize / 3, (Color){(unsigned char)(fuelBg.r + 8), (unsigned char)(fuelBg.g + 8), (unsigned char)(fuelBg.b + 8), fuelBg.a});
        { int sc = 3; DrawCircleV((Vector2){(float)(fuelX+sc),(float)(fuSlotY+sc)},sc,fuelBg); DrawCircleV((Vector2){(float)(fuelX+fuSlotSize-sc),(float)(fuSlotY+sc)},sc,fuelBg); DrawCircleV((Vector2){(float)(fuelX+sc),(float)(fuSlotY+fuSlotSize-sc)},sc,fuelBg); DrawCircleV((Vector2){(float)(fuelX+fuSlotSize-sc),(float)(fuSlotY+fuSlotSize-sc)},sc,fuelBg); }
        DrawRectangleLines(fuelX, fuSlotY, fuSlotSize, fuSlotSize, fuelBd);
        DrawGameText(S(STR_FUEL), fuelX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_FUEL), 10) / 2, fuSlotY - 13, 10, (Color){180, 175, 190, 200});

        // Input slot
        Rectangle inputRect = { (float)inputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool inputHover = CheckCollisionPointRec(mouse, inputRect);
        int inputHoverId = 501;
        float inputHA = GetHoverAlpha(inputHoverId, inputHover, GetFrameTime());
        Color inputBg = { (unsigned char)(55 + 20 * inputHA), (unsigned char)(52 + 18 * inputHA), (unsigned char)(62 + 23 * inputHA), (unsigned char)(200 + 10 * inputHA) };
        Color inputBd = { (unsigned char)(80 + 30 * inputHA), (unsigned char)(75 + 30 * inputHA), (unsigned char)(90 + 40 * inputHA), 200 };
        if (inputHA > 0.01f) DrawRectangle(inputX - 1, fuSlotY - 1, fuSlotSize + 2, fuSlotSize + 2, (Color){100, 95, 120, (unsigned char)(35 * inputHA)});
        DrawRectangle(inputX, fuSlotY, fuSlotSize, fuSlotSize, inputBg);
        DrawRectangle(inputX, fuSlotY, fuSlotSize, fuSlotSize / 3, (Color){(unsigned char)(inputBg.r + 8), (unsigned char)(inputBg.g + 8), (unsigned char)(inputBg.b + 8), inputBg.a});
        { int sc = 3; DrawCircleV((Vector2){(float)(inputX+sc),(float)(fuSlotY+sc)},sc,inputBg); DrawCircleV((Vector2){(float)(inputX+fuSlotSize-sc),(float)(fuSlotY+sc)},sc,inputBg); DrawCircleV((Vector2){(float)(inputX+sc),(float)(fuSlotY+fuSlotSize-sc)},sc,inputBg); DrawCircleV((Vector2){(float)(inputX+fuSlotSize-sc),(float)(fuSlotY+fuSlotSize-sc)},sc,inputBg); }
        DrawRectangleLines(inputX, fuSlotY, fuSlotSize, fuSlotSize, inputBd);
        DrawGameText(S(STR_INPUT), inputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_INPUT), 10) / 2, fuSlotY - 13, 10, (Color){180, 175, 190, 200});

        // Output slot
        Rectangle outputRect = { (float)outputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool outputHover = CheckCollisionPointRec(mouse, outputRect);
        int outputHoverId = 502;
        float outputHA = GetHoverAlpha(outputHoverId, outputHover, GetFrameTime());
        Color outputBg = { (unsigned char)(55 + 20 * outputHA), (unsigned char)(52 + 18 * outputHA), (unsigned char)(62 + 23 * outputHA), (unsigned char)(200 + 10 * outputHA) };
        Color outputBd = { (unsigned char)(80 + 30 * outputHA), (unsigned char)(75 + 30 * outputHA), (unsigned char)(90 + 40 * outputHA), 200 };
        if (outputHA > 0.01f) DrawRectangle(outputX - 1, fuSlotY - 1, fuSlotSize + 2, fuSlotSize + 2, (Color){100, 95, 120, (unsigned char)(35 * outputHA)});
        DrawRectangle(outputX, fuSlotY, fuSlotSize, fuSlotSize, outputBg);
        DrawRectangle(outputX, fuSlotY, fuSlotSize, fuSlotSize / 3, (Color){(unsigned char)(outputBg.r + 8), (unsigned char)(outputBg.g + 8), (unsigned char)(outputBg.b + 8), outputBg.a});
        { int sc = 3; DrawCircleV((Vector2){(float)(outputX+sc),(float)(fuSlotY+sc)},sc,outputBg); DrawCircleV((Vector2){(float)(outputX+fuSlotSize-sc),(float)(fuSlotY+sc)},sc,outputBg); DrawCircleV((Vector2){(float)(outputX+sc),(float)(fuSlotY+fuSlotSize-sc)},sc,outputBg); DrawCircleV((Vector2){(float)(outputX+fuSlotSize-sc),(float)(fuSlotY+fuSlotSize-sc)},sc,outputBg); }
        DrawRectangleLines(outputX, fuSlotY, fuSlotSize, fuSlotSize, outputBd);
        DrawGameText(S(STR_OUTPUT), outputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_OUTPUT), 10) / 2, fuSlotY - 13, 10, (Color){180, 175, 190, 200});

        // Progress arrow between input and output
        int arrowX = inputX + fuSlotSize + 6;
        int arrowW = outputX - inputX - fuSlotSize - 12;
        int arrowY = fuSlotY + fuSlotSize / 2 - 4;
        DrawRectangle(arrowX, arrowY, arrowW, 8, (Color){40, 38, 48, 200});
        if (furnaceProgress > 0.0f) {
            DrawRectangle(arrowX, arrowY, (int)(arrowW * furnaceProgress), 8, (Color){220, 160, 60, 255});
        }
        DrawTriangle(
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY - 4)},
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY + 12)},
            (Vector2){(float)(arrowX + arrowW + 8), (float)(arrowY + 4)},
            (Color){220, 160, 60, 255});

        // Draw items in furnace slots
        if (furnaceFuel != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceFuel * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(fuelX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceFuelCount > 1) {
                DrawGameText(TextFormat("%d", furnaceFuelCount), fuelX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 13, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceFuelCount), fuelX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (furnaceInput != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceInput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(inputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceInputCount > 1) {
                DrawGameText(TextFormat("%d", furnaceInputCount), inputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 13, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceInputCount), inputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (furnaceOutput != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceOutput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(outputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceOutputCount > 1) {
                DrawGameText(TextFormat("%d", furnaceOutputCount), outputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 13, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceOutputCount), outputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }

        // Fuel bar
        int flameY = fuSlotY + fuSlotSize + 8;
        DrawGameText(TextFormat("%s:", S(STR_FUEL)), fuelX, flameY, 10, (Color){180, 175, 190, 200});
        if (furnaceFuelBurn > 0.0f) {
            int flameW = 60;
            float fuelPct = furnaceFuelBurn / 8.0f;
            DrawRectangle(fuelX, flameY + 14, flameW, 8, (Color){40, 38, 48, 200});
            DrawRectangle(fuelX, flameY + 14, (int)(flameW * fuelPct), 8, (Color){220, 120, 40, 255});
        } else {
            DrawGameText(S(STR_MSG_NO_FUEL), fuelX, flameY + 14, 10, (Color){150, 80, 80, 200});
        }

        // Smelting info
        if (furnaceInput != BLOCK_AIR) {
            int ri = FindSmeltRecipe((BlockType)furnaceInput);
            if (ri >= 0) {
                DrawGameText(Sf(STR_MSG_SMELTING, S(smeltRecipes[ri].nameId)), contX + panelPad, contY + furnaceH - 18, 10, (Color){150, 145, 160, 200});
            } else {
                DrawGameText(S(STR_MSG_CANNOT_SMELT), contX + panelPad, contY + furnaceH - 18, 10, (Color){200, 100, 100, 200});
            }
        }

        // --- Divider ---
        int divY = contY + furnaceH;
        DrawRectangle(contX + 8, divY, contW - 16, dividerH, (Color){80, 75, 90, 200});

        // --- Inventory section ---
        int invX = contX + panelPad + previewW + previewPad + armorColW;
        int invY = divY + dividerH + 4;
        DrawGameText(S(STR_INVENTORY), invX, invY, 14, (Color){220, 210, 230, 255});
        invY += 20;

        // Player preview (compact in furnace view)
        {
            int prevX = contX + panelPad;
            int prevY = invY;
            int prevH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
            DrawRectangle(prevX, prevY, previewW, prevH, (Color){30, 28, 38, 220});
            DrawRectangleLines(prevX, prevY, previewW, prevH, (Color){60, 55, 75, 180});
            int sc = 2;
            int charW = 12 * sc, charH = 28 * sc;
            int cx = prevX + (previewW - charW) / 2;
            int cy = prevY + (prevH - charH) / 2;
            Color skin = {220,180,140,255}, hair = {80,50,30,255}, shirt = {0,100,200,255}, pants_ = {60,40,20,255};
            Color hc={0,0,0,0},cc={0,0,0,0},lc={0,0,0,0},bc={0,0,0,0};
            for (int i = 0; i < 4; i++) { if (player.armor[i] != BLOCK_AIR) { BlockType a=(BlockType)player.armor[i]; Color ac; if(a>=ARMOR_IRON_HELMET) ac=(Color){200,210,220,255}; else if(a>=ARMOR_STONE_HELMET) ac=(Color){140,140,140,255}; else ac=(Color){160,120,60,255}; if(i==0)hc=ac;else if(i==1)cc=ac;else if(i==2)lc=ac;else bc=ac; } }
            DrawRectangle(cx+2*sc,cy,8*sc,8*sc,skin); DrawRectangle(cx+2*sc,cy,8*sc,3*sc,hair);
            if(hc.a>0){DrawRectangle(cx+1*sc,cy-1*sc,10*sc,5*sc,hc);DrawRectangle(cx+2*sc,cy+4*sc,8*sc,2*sc,hc);}
            DrawRectangle(cx+3*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255}); DrawRectangle(cx+7*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255});
            DrawRectangle(cx+1*sc,cy+8*sc,10*sc,10*sc,shirt);
            if(cc.a>0){DrawRectangle(cx,cy+7*sc,12*sc,11*sc,cc);DrawRectangle(cx+1*sc,cy+8*sc,10*sc,9*sc,(Color){(unsigned char)(cc.r*0.8f),(unsigned char)(cc.g*0.8f),(unsigned char)(cc.b*0.8f),255});}
            DrawRectangle(cx-2*sc,cy+8*sc,3*sc,10*sc,skin); DrawRectangle(cx+11*sc,cy+8*sc,3*sc,10*sc,skin);
            DrawRectangle(cx+1*sc,cy+18*sc,4*sc,10*sc,pants_); DrawRectangle(cx+7*sc,cy+18*sc,4*sc,10*sc,pants_);
            if(lc.a>0){DrawRectangle(cx,cy+18*sc,5*sc,10*sc,lc);DrawRectangle(cx+6*sc,cy+18*sc,5*sc,10*sc,lc);}
            if(bc.a>0){DrawRectangle(cx,cy+25*sc,5*sc,3*sc,bc);DrawRectangle(cx+6*sc,cy+25*sc,5*sc,3*sc,bc);}
        }

        // Armor slots (compact)
        {
            const char *armorLabels[] = {"H", "C", "L", "B"};
            int armorX = contX + panelPad + previewW + previewPad;
            int armorY = invY;
            for (int i = 0; i < 4; i++) {
                int ay = armorY + i * (armorSlotSize + armorPad);
                Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                Color bg = hover ? (Color){75, 70, 85, 210} : (Color){55, 52, 62, 200};
                DrawRectangle(armorX, ay, armorSlotSize, armorSlotSize, bg);
                DrawRectangleLines(armorX, ay, armorSlotSize, armorSlotSize, (Color){80, 75, 90, 200});
                if (player.armor[i] != BLOCK_AIR && blockAtlas.id > 0) {
                    int item = player.armor[i];
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(armorX + 4), (float)(ay + 4), (float)(armorSlotSize - 8), (float)(armorSlotSize - 8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
                } else {
                    DrawGameText(armorLabels[i], armorX + armorSlotSize / 2 - 4, ay + armorSlotSize / 2 - 6, 14, (Color){100, 95, 110, 150});
                }
                // Armor click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (heldItem == BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                        heldItem = player.armor[i]; heldCount = 1; heldDurability = player.armorDurability[i];
                        player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.armor[i] == BLOCK_AIR && IsArmor((BlockType)heldItem)) {
                        int slotType = (heldItem - ARMOR_WOOD_HELMET) % 4;
                        if (slotType == i) { player.armor[i] = heldItem; player.armorDurability[i] = heldDurability; heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; PlaySoundUIClick(); }
                    }
                }
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
                int hoverId = 100 + idx;
                float hA = GetHoverAlpha(hoverId, hover && !selected, GetFrameTime());

                unsigned char baseR = 55, baseG = 52, baseB = 62;
                unsigned char hoverR = 75, hoverG = 70, hoverB = 85;
                Color bg;
                if (selected) { bg = (Color){100, 95, 110, 220}; }
                else { bg = (Color){(unsigned char)(baseR+(int)((hoverR-baseR)*hA)),(unsigned char)(baseG+(int)((hoverG-baseG)*hA)),(unsigned char)(baseB+(int)((hoverB-baseB)*hA)),(unsigned char)(200+(int)(10*hA))}; }
                Color border = selected ? (Color){160,150,180,255} : (Color){(unsigned char)(80+(int)(30*hA)),(unsigned char)(75+(int)(30*hA)),(unsigned char)(90+(int)(40*hA)),(unsigned char)(200+(int)(30*hA))};
                if (hA > 0.01f && !selected) DrawRectangle(x-1,y-1,slotSize+2,slotSize+2,(Color){100,95,120,(unsigned char)(35*hA)});
                DrawRectangle(x,y,slotSize,slotSize,bg);
                DrawRectangle(x,y,slotSize,slotSize/3,(Color){(unsigned char)(bg.r+8),(unsigned char)(bg.g+8),(unsigned char)(bg.b+8),bg.a});
                { int sc=3; DrawCircleV((Vector2){(float)(x+sc),(float)(y+sc)},sc,bg); DrawCircleV((Vector2){(float)(x+slotSize-sc),(float)(y+sc)},sc,bg); DrawCircleV((Vector2){(float)(x+sc),(float)(y+slotSize-sc)},sc,bg); DrawCircleV((Vector2){(float)(x+slotSize-sc),(float)(y+slotSize-sc)},sc,bg); }
                DrawRectangleLines(x,y,slotSize,slotSize,border);

                int item = player.inventory[idx];
                if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(x+4), (float)(y+4), (float)(slotSize-8), (float)(slotSize-8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
                    if (player.inventoryCount[idx] > 1) {
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-19, y+slotSize-13, 13, (Color){0,0,0,150});
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-20, y+slotSize-14, 13, WHITE);
                    }
                    if (IsTool((BlockType)item)) {
                        int maxDur = GetToolMaxDurability((BlockType)item);
                        if (maxDur > 0) {
                            float pct = (float)player.toolDurability[idx] / maxDur;
                            int barW = slotSize-8, barX = x+4, barY = y+slotSize-5;
                            Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                            DrawRectangle(barX,barY,barW,3,(Color){0,0,0,180});
                            DrawRectangle(barX,barY,(int)(barW*pct),3,barColor);
                        }
                    }
                }
                if (row == 0) DrawGameText(TextFormat("%d",col+1), x+2, y+1, 10, (Color){180,170,190,120});

                // Click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                        if (player.inventory[idx] != BLOCK_AIR) {
                            bool transferred = false;
                            uint8_t itm = player.inventory[idx]; int cnt = player.inventoryCount[idx];
                            if (itm == ITEM_COAL) {
                                if (furnaceFuel == BLOCK_AIR) { furnaceFuel = ITEM_COAL; furnaceFuelCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (furnaceFuel == ITEM_COAL) { int sp = 64 - furnaceFuelCount; int ta = cnt > sp ? sp : cnt; furnaceFuelCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            } else if (FindSmeltRecipe((BlockType)itm) >= 0) {
                                if (furnaceInput == BLOCK_AIR) { furnaceInput = itm; furnaceInputCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (furnaceInput == itm) { int sp = 64 - furnaceInputCount; int ta = cnt > sp ? sp : cnt; furnaceInputCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            }
                            if (!transferred) {
                                int startDest = (row == 0) ? HOTBAR_SLOTS : 0;
                                int endDest = (row == 0) ? INVENTORY_SLOTS : HOTBAR_SLOTS;
                                bool moved = false;
                                for (int d = startDest; d < endDest; d++) {
                                    if (player.inventory[d] == player.inventory[idx] && player.inventoryCount[d] < 64) {
                                        int sp = 64 - player.inventoryCount[d]; int ta = player.inventoryCount[idx] > sp ? sp : player.inventoryCount[idx];
                                        player.inventoryCount[d] += ta; player.inventoryCount[idx] -= ta;
                                        if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; }
                                        moved = true; break;
                                    }
                                }
                                if (!moved) { for (int d = startDest; d < endDest; d++) { if (player.inventory[d] == BLOCK_AIR) { player.inventory[d] = player.inventory[idx]; player.inventoryCount[d] = player.inventoryCount[idx]; player.toolDurability[d] = player.toolDurability[idx]; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; break; } } }
                            }
                            PlaySoundUIClick();
                        }
                    } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        heldItem = player.inventory[idx]; heldCount = player.inventoryCount[idx]; heldDurability = player.toolDurability[idx];
                        player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability;
                        heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                        int sp = 64 - player.inventoryCount[idx]; int ta = heldCount > sp ? sp : heldCount;
                        player.inventoryCount[idx] += ta; heldCount -= ta;
                        if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; } PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        uint8_t ti = player.inventory[idx]; int tc = player.inventoryCount[idx]; int td = player.toolDurability[idx];
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability;
                        heldItem = ti; heldCount = tc; heldDurability = td; PlaySoundUIClick();
                    }
                }
                if (hover && Win32IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                    PlaySoundDrop(); player.inventoryCount[idx]--;
                    if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; }
                }
            }
        }

        // Furnace slot click handling
        if (CheckCollisionPointRec(mouse, fuelRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceFuel != BLOCK_AIR) {
                heldItem = furnaceFuel; heldCount = furnaceFuelCount; heldDurability = 0;
                furnaceFuel = BLOCK_AIR; furnaceFuelCount = 0;
            } else if (heldItem != BLOCK_AIR && furnaceFuel == BLOCK_AIR && heldItem == ITEM_COAL) {
                furnaceFuel = heldItem; furnaceFuelCount = heldCount;
                heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0;
            } else if (heldItem != BLOCK_AIR && furnaceFuel == heldItem && heldItem == ITEM_COAL) {
                int sp = 64 - furnaceFuelCount; int ta = heldCount > sp ? sp : heldCount;
                furnaceFuelCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, inputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceInput != BLOCK_AIR) {
                heldItem = furnaceInput; heldCount = furnaceInputCount; heldDurability = 0;
                furnaceInput = BLOCK_AIR; furnaceInputCount = 0; furnaceProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && furnaceInput == BLOCK_AIR && FindSmeltRecipe((BlockType)heldItem) >= 0) {
                furnaceInput = heldItem; furnaceInputCount = heldCount;
                heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0; furnaceProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && furnaceInput == heldItem) {
                int sp = 64 - furnaceInputCount; int ta = heldCount > sp ? sp : heldCount;
                furnaceInputCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, outputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceOutput != BLOCK_AIR) {
                heldItem = furnaceOutput; heldCount = furnaceOutputCount; heldDurability = 0;
                furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0;
            } else if (heldItem == furnaceOutput && furnaceOutput != BLOCK_AIR) {
                int sp = 64 - heldCount; int ta = furnaceOutputCount > sp ? sp : furnaceOutputCount;
                heldCount += ta; furnaceOutputCount -= ta;
                if (furnaceOutputCount <= 0) { furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0; }
            }
        }

        // Drop held item outside container
        if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle container = { (float)contX, (float)contY, (float)contW, (float)contH };
            if (!CheckCollisionPointRec(mouse, container)) {
                PlaySoundDrop();
                SpawnItemEntity(heldItem, heldCount, player.position.x + PLAYER_WIDTH / 2, player.position.y);
                heldItem = BLOCK_AIR; heldCount = 0; heldDurability = 0;
            }
        }

        // Tooltip for inventory slots
        if (heldItem == BLOCK_AIR) {
            for (int row = 0; row < INVENTORY_ROWS; row++) {
                for (int col = 0; col < INVENTORY_COLS; col++) {
                    int idx = row * INVENTORY_COLS + col;
                    int x = invX + col * (slotSize + padding);
                    int y = invY + row * (slotSize + padding);
                    Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                    if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                        BlockType bt = (BlockType)player.inventory[idx];
                        const char *name = GetBlockName(bt);
                        int tw = MeasureGameTextWidth(name, 14);
                        int tx = (int)mouse.x + 14; int ty = (int)mouse.y - 18;
                        if (tx + tw + 8 > SCREEN_WIDTH) tx = (int)mouse.x - tw - 14;
                        if (ty < 4) ty = (int)mouse.y + 14;
                        const char *typeLabel = NULL; Color typeColor = {180,180,190,200};
                        if (IsTool(bt)) { typeLabel = "Tool"; typeColor = (Color){100,160,220,255}; }
                        else if (IsArmor(bt)) { typeLabel = "Armor"; typeColor = (Color){180,100,220,255}; }
                        else if (IsFood(bt)) { typeLabel = "Food"; typeColor = (Color){100,200,100,255}; }
                        else { typeLabel = "Block"; typeColor = (Color){180,175,190,200}; }
                        int typeW = MeasureGameTextWidth(typeLabel, 11); int maxW = tw > typeW ? tw : typeW;
                        char info[64] = {0};
                        if (IsFood(bt)) snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                        else if (IsArmor(bt)) { int av = GetArmorValue(bt); int md = GetArmorMaxDurability(bt); int pct = md > 0 ? player.toolDurability[idx]*100/md : 0; snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), av, pct); }
                        else if (IsTool(bt)) { int md = GetToolMaxDurability(bt); if (md > 0) { int pct = player.toolDurability[idx]*100/md; snprintf(info, sizeof(info), S(STR_TOOLTIP_DURABILITY), pct); } }
                        int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0; if (infoW > maxW) maxW = infoW;
                        int ttH = 36 + (info[0] ? 16 : 0);
                        DrawRectangle(tx-3, ty-1, maxW+10, ttH+2, (Color){0,0,0,60});
                        DrawRectangle(tx-4, ty-2, maxW+10, ttH+2, (Color){25,22,32,240});
                        DrawRectangleLines(tx-4, ty-2, maxW+10, ttH+2, (Color){90,85,110,220});
                        DrawGameText(typeLabel, tx, ty, 11, typeColor);
                        DrawGameText(name, tx, ty+14, 14, (Color){230,225,240,255});
                        if (info[0]) DrawGameText(info, tx, ty+30, 13, (Color){180,200,180,255});
                    }
                }
            }
        }

        // Close hint
        DrawGameText(S(STR_PRESS_E_ESC_CLOSE),
            contX + contW / 2 - MeasureGameTextWidth(S(STR_PRESS_E_ESC_CLOSE), 10) / 2,
            contY + contH - 14, 10, (Color){120, 115, 130, 180});

        // Held item follows mouse
        if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
            int mx = (int)mouse.x - slotSize / 2; int my = (int)mouse.y - slotSize / 2;
            Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(mx+4), (float)(my+4), (float)(slotSize-8), (float)(slotSize-8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
            if (heldCount > 1) DrawGameText(TextFormat("%d", heldCount), mx+slotSize-20, my+slotSize-16, 14, WHITE);
        }

        return;
    }

    // --- Normal inventory screen (when furnace is NOT open) ---
    int padding = 3;
    int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;
    int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;

    // Player preview dimensions
    int previewW = 80;
    int previewPad = 8;

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
    int totalW = panelPad + previewW + previewPad + armorColW + gridW + panelPad + dividerW + panelPad + craftPanelW + panelPad;
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
    int invX = containerX + panelPad + previewW + previewPad + armorColW;
    int invY = containerY + panelPad;
    Vector2 mouse = Win32GetMousePosition();
    DrawGameText(S(STR_INVENTORY), invX, invY, 16, (Color){220, 210, 230, 255});
    invY += 24;

    // --- Player Preview ---
    {
        int prevX = containerX + panelPad;
        int prevY = invY;
        int prevH = gridH;

        // Preview background with subtle pattern
        DrawRectangle(prevX, prevY, previewW, prevH, (Color){30, 28, 38, 220});
        DrawRectangleLines(prevX, prevY, previewW, prevH, (Color){60, 55, 75, 180});
        // Inner shadow (top darker, bottom lighter)
        DrawRectangle(prevX + 1, prevY + 1, previewW - 2, 3, (Color){20, 18, 28, 100});
        DrawRectangle(prevX + 1, prevY + prevH - 4, previewW - 2, 3, (Color){50, 47, 60, 80});

        // Draw scaled player character (3x scale)
        int scale = 3;
        int charW = 12 * scale;
        int charH = 28 * scale;
        int cx = prevX + (previewW - charW) / 2;
        int cy = prevY + (prevH - charH) / 2;

        // Player colors
        Color skin = (Color){220, 180, 140, 255};
        Color hair = (Color){80, 50, 30, 255};
        Color shirt = (Color){0, 100, 200, 255};
        Color pants = (Color){60, 40, 20, 255};

        // Armor colors
        Color hc = {0,0,0,0}, cc = {0,0,0,0}, lc = {0,0,0,0}, bc = {0,0,0,0};
        for (int i = 0; i < 4; i++) {
            if (player.armor[i] != BLOCK_AIR) {
                BlockType a = (BlockType)player.armor[i];
                Color ac;
                if (a >= ARMOR_IRON_HELMET) ac = (Color){200, 210, 220, 255};
                else if (a >= ARMOR_STONE_HELMET) ac = (Color){140, 140, 140, 255};
                else ac = (Color){160, 120, 60, 255};
                if (i == 0) hc = ac; else if (i == 1) cc = ac; else if (i == 2) lc = ac; else bc = ac;
            }
        }

        // Head
        DrawRectangle(cx + 2*scale, cy, 8*scale, 8*scale, skin);
        DrawRectangle(cx + 2*scale, cy, 8*scale, 3*scale, hair);
        // Helmet overlay
        if (hc.a > 0) {
            DrawRectangle(cx + 1*scale, cy - 1*scale, 10*scale, 5*scale, hc);
            DrawRectangle(cx + 2*scale, cy + 4*scale, 8*scale, 2*scale, hc);
        }
        // Eyes
        DrawRectangle(cx + 3*scale, cy + 4*scale, 2*scale, 2*scale, (Color){40, 40, 40, 255});
        DrawRectangle(cx + 7*scale, cy + 4*scale, 2*scale, 2*scale, (Color){40, 40, 40, 255});

        // Body
        DrawRectangle(cx + 1*scale, cy + 8*scale, 10*scale, 10*scale, shirt);
        // Chestplate overlay
        if (cc.a > 0) {
            DrawRectangle(cx, cy + 7*scale, 12*scale, 11*scale, cc);
            DrawRectangle(cx + 1*scale, cy + 8*scale, 10*scale, 9*scale, (Color){
                (unsigned char)(cc.r * 0.8f), (unsigned char)(cc.g * 0.8f), (unsigned char)(cc.b * 0.8f), 255});
        }

        // Arms
        DrawRectangle(cx - 2*scale, cy + 8*scale, 3*scale, 10*scale, skin);
        DrawRectangle(cx + 11*scale, cy + 8*scale, 3*scale, 10*scale, skin);

        // Legs
        DrawRectangle(cx + 1*scale, cy + 18*scale, 4*scale, 10*scale, pants);
        DrawRectangle(cx + 7*scale, cy + 18*scale, 4*scale, 10*scale, pants);
        // Leggings overlay
        if (lc.a > 0) {
            DrawRectangle(cx, cy + 18*scale, 5*scale, 10*scale, lc);
            DrawRectangle(cx + 6*scale, cy + 18*scale, 5*scale, 10*scale, lc);
        }
        // Boots overlay
        if (bc.a > 0) {
            DrawRectangle(cx, cy + 25*scale, 5*scale, 3*scale, bc);
            DrawRectangle(cx + 6*scale, cy + 25*scale, 5*scale, 3*scale, bc);
        }

        // Health bar under preview
        int hpBarX = prevX + 4;
        int hpBarY = prevY + prevH - 14;
        int hpBarW = previewW - 8;
        int hpBarH = 4;
        float hpPct = (float)player.health / MAX_HEALTH;
        DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, (Color){60, 20, 20, 200});
        DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpPct), hpBarH, hpPct > 0.5f ? (Color){200, 50, 50, 255} : (Color){255, 80, 80, 255});

        // Hunger bar
        int hungerBarY = hpBarY + hpBarH + 2;
        float hungerPct = (float)player.hunger / MAX_HUNGER;
        DrawRectangle(hpBarX, hungerBarY, hpBarW, hpBarH, (Color){60, 50, 10, 200});
        DrawRectangle(hpBarX, hungerBarY, (int)(hpBarW * hungerPct), hpBarH, (Color){180, 140, 40, 255});

        // Armor value display
        int totalArmor = 0;
        for (int i = 0; i < 4; i++) {
            if (player.armor[i] != BLOCK_AIR) totalArmor += GetArmorValue((BlockType)player.armor[i]);
        }
        if (totalArmor > 0) {
            int armorIconX = prevX + 4;
            int armorIconY = prevY + 4;
            // Shield icon (small)
            DrawRectangle(armorIconX, armorIconY, 8, 10, (Color){100, 100, 120, 200});
            DrawRectangle(armorIconX + 1, armorIconY + 1, 6, 4, (Color){140, 140, 160, 200});
            DrawGameText(TextFormat("%d", totalArmor), armorIconX + 10, armorIconY, 11, (Color){180, 200, 220, 255});
        }
    }

    // Armor slots (vertical, to the left of inventory)
    {
        const char *armorLabels[] = {"H", "C", "L", "B"};
        int armorX = containerX + panelPad + previewW + previewPad;
        int armorY = invY;
        for (int i = 0; i < 4; i++) {
            int ay = armorY + i * (armorSlotSize + armorPad);
            Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect) && !furnaceOpen;

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
                DrawGameText(armorLabels[i], armorX + armorSlotSize / 2 - 4, ay + armorSlotSize / 2 - 6,14, (Color){100, 95, 110, 150});
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
                    const char *name = GetBlockName(bt);
                    int tw = MeasureGameTextWidth(name,14);
                    int tx = (int)mouse.x + 14;
                    int ty = (int)mouse.y - 18;
                    if (tx + tw + 8 > SCREEN_WIDTH) tx = (int)mouse.x - tw - 14;
                    if (ty < 4) ty = (int)mouse.y + 14;
                    // Tooltip shadow
                    DrawRectangle(tx - 3, ty - 1, tw + 8, 16, (Color){0, 0, 0, 60});
                    // Tooltip background
                    DrawRectangle(tx - 4, ty - 2, tw + 8, 16, (Color){25, 22, 32, 240});
                    DrawRectangleLines(tx - 4, ty - 2, tw + 8, 16, (Color){90, 85, 110, 220});
                    DrawGameText(name, tx, ty,14, (Color){230, 225, 240, 255});

                    char info[64] = { 0 };
                    int armorVal = GetArmorValue(bt);
                    int maxDur = GetArmorMaxDurability(bt);
                    int pct = maxDur > 0 ? player.armorDurability[i] * 100 / maxDur : 0;
                    snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), armorVal, pct);
                    int iw = MeasureGameTextWidth(info,13);
                    if (iw > tw) tw = iw;
                    ty += 16;
                    DrawRectangle(tx - 4, ty - 2, tw + 8, 15, (Color){20, 18, 25, 230});
                    DrawRectangleLines(tx - 4, ty - 2, tw + 8, 15, (Color){80, 75, 90, 200});
                    DrawGameText(info, tx, ty,13, (Color){180, 200, 180, 255});
                }
            }
        }
    }

    // Divider line
    int divX = containerX + panelPad + previewW + previewPad + armorColW + gridW + panelPad;
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
        DrawGameText(S(STR_SORT), sortBtnX + 6, sortBtnY + 2,13, (Color){180, 175, 195, 220});
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
            int hoverId = 100 + idx;
            float hA = GetHoverAlpha(hoverId, hover && !selected, GetFrameTime());

            // Slot background with smooth hover
            unsigned char baseR = 55, baseG = 52, baseB = 62;
            unsigned char hoverR = 75, hoverG = 70, hoverB = 85;
            Color bg;
            if (selected) {
                bg = (Color){100, 95, 110, 220};
            } else {
                bg = (Color){
                    (unsigned char)(baseR + (int)((hoverR - baseR) * hA)),
                    (unsigned char)(baseG + (int)((hoverG - baseG) * hA)),
                    (unsigned char)(baseB + (int)((hoverB - baseB) * hA)),
                    (unsigned char)(200 + (int)(10 * hA))
                };
            }
            Color border = selected ? (Color){160, 150, 180, 255} :
                (Color){(unsigned char)(80 + (int)(30 * hA)), (unsigned char)(75 + (int)(30 * hA)), (unsigned char)(90 + (int)(40 * hA)), (unsigned char)(200 + (int)(30 * hA))};
            // Hover glow (smooth)
            if (hA > 0.01f && !selected) {
                unsigned char glowA = (unsigned char)(35 * hA);
                DrawRectangle(x - 1, y - 1, slotSize + 2, slotSize + 2, (Color){100, 95, 120, glowA});
            }
            DrawRectangle(x, y, slotSize, slotSize, bg);
            // Subtle gradient (lighter top)
            DrawRectangle(x, y, slotSize, slotSize / 3, (Color){(unsigned char)(bg.r + 8), (unsigned char)(bg.g + 8), (unsigned char)(bg.b + 8), bg.a});
            // Rounded corners
            int scr = 3;
            DrawCircleV((Vector2){(float)(x + scr), (float)(y + scr)}, scr, bg);
            DrawCircleV((Vector2){(float)(x + slotSize - scr), (float)(y + scr)}, scr, bg);
            DrawCircleV((Vector2){(float)(x + scr), (float)(y + slotSize - scr)}, scr, bg);
            DrawCircleV((Vector2){(float)(x + slotSize - scr), (float)(y + slotSize - scr)}, scr, bg);
            DrawRectangleLines(x, y, slotSize, slotSize, border);

            int item = player.inventory[idx];
            // Item type color indicator (small bar at bottom)
            if (item != BLOCK_AIR) {
                Color typeColor = {0, 0, 0, 0};
                if (IsTool((BlockType)item)) typeColor = (Color){80, 140, 200, 180};
                else if (IsArmor((BlockType)item)) typeColor = (Color){160, 80, 200, 180};
                else if (IsFood((BlockType)item)) typeColor = (Color){80, 180, 80, 180};
                if (typeColor.a > 0) {
                    DrawRectangle(x + 3, y + slotSize - 4, slotSize - 6, 2, typeColor);
                }
            }
            if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                if (player.inventoryCount[idx] > 1) {
                    // Shadow for readability
                    DrawGameText(TextFormat("%d", player.inventoryCount[idx]),
                             x + slotSize - 19, y + slotSize - 13, 13, (Color){0, 0, 0, 150});
                    DrawGameText(TextFormat("%d", player.inventoryCount[idx]),
                             x + slotSize - 20, y + slotSize - 14, 13, WHITE);
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
                DrawGameText(TextFormat("%d", col + 1), x + 2, y + 1,10, (Color){180, 170, 190, 120});
            }

            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                    if (player.inventory[idx] != BLOCK_AIR) {
                        bool transferred = false;
                        if (furnaceOpen) {
                            uint8_t item = player.inventory[idx];
                            int count = player.inventoryCount[idx];
                            if (item == ITEM_COAL) {
                                // Transfer coal to fuel slot
                                if (furnaceFuel == BLOCK_AIR) {
                                    furnaceFuel = ITEM_COAL;
                                    furnaceFuelCount = count;
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    transferred = true;
                                } else if (furnaceFuel == ITEM_COAL) {
                                    int space = 64 - furnaceFuelCount;
                                    int toAdd = count > space ? space : count;
                                    furnaceFuelCount += toAdd;
                                    player.inventoryCount[idx] -= toAdd;
                                    if (player.inventoryCount[idx] <= 0) {
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                    }
                                    transferred = true;
                                }
                            } else if (FindSmeltRecipe((BlockType)item) >= 0) {
                                // Transfer smeltable to input slot
                                if (furnaceInput == BLOCK_AIR) {
                                    furnaceInput = item;
                                    furnaceInputCount = count;
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    transferred = true;
                                } else if (furnaceInput == item) {
                                    int space = 64 - furnaceInputCount;
                                    int toAdd = count > space ? space : count;
                                    furnaceInputCount += toAdd;
                                    player.inventoryCount[idx] -= toAdd;
                                    if (player.inventoryCount[idx] <= 0) {
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                    }
                                    transferred = true;
                                }
                            }
                        }
                        if (!transferred) {
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

    // Drop held item by clicking outside the container (skip when furnace is open)
    if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !furnaceOpen) {
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

    // Tooltip for hovered slot (skip when furnace is open)
    if (heldItem == BLOCK_AIR && !furnaceOpen) {
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int idx = row * INVENTORY_COLS + col;
                int x = invX + col * (slotSize + padding);
                int y = invY + row * (slotSize + padding);
                Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                    BlockType bt = (BlockType)player.inventory[idx];
                    const char *name = GetBlockName(bt);
                    int tw = MeasureGameTextWidth(name,14);
                    int tx = (int)mouse.x + 14;
                    int ty = (int)mouse.y - 18;
                    if (tx + tw + 8 > SCREEN_WIDTH) tx = (int)mouse.x - tw - 14;
                    if (ty < 4) ty = (int)mouse.y + 14;

                    // Item type label
                    const char *typeLabel = NULL;
                    Color typeColor = {180, 180, 190, 200};
                    if (IsTool(bt)) { typeLabel = "Tool"; typeColor = (Color){100, 160, 220, 255}; }
                    else if (IsArmor(bt)) { typeLabel = "Armor"; typeColor = (Color){180, 100, 220, 255}; }
                    else if (IsFood(bt)) { typeLabel = "Food"; typeColor = (Color){100, 200, 100, 255}; }
                    else { typeLabel = "Block"; typeColor = (Color){180, 175, 190, 200}; }

                    int typeW = MeasureGameTextWidth(typeLabel, 11);
                    int maxW = tw > typeW ? tw : typeW;

                    // Extra info line
                    char info[64] = { 0 };
                    if (IsFood(bt)) {
                        snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                    } else if (IsArmor(bt)) {
                        int armorVal = GetArmorValue(bt);
                        int maxDur = GetArmorMaxDurability(bt);
                        int pct = maxDur > 0 ? player.toolDurability[idx] * 100 / maxDur : 0;
                        snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), armorVal, pct);
                    } else if (IsTool(bt)) {
                        int maxDur = GetToolMaxDurability(bt);
                        if (maxDur > 0) {
                            int pct = player.toolDurability[idx] * 100 / maxDur;
                            snprintf(info, sizeof(info), S(STR_TOOLTIP_DURABILITY), pct);
                        }
                    }
                    int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0;
                    if (infoW > maxW) maxW = infoW;

                    // Tooltip background (multi-line)
                    int ttH = 36 + (info[0] ? 16 : 0);
                    DrawRectangle(tx - 3, ty - 1, maxW + 10, ttH + 2, (Color){0, 0, 0, 60});
                    DrawRectangle(tx - 4, ty - 2, maxW + 10, ttH + 2, (Color){25, 22, 32, 240});
                    DrawRectangleLines(tx - 4, ty - 2, maxW + 10, ttH + 2, (Color){90, 85, 110, 220});

                    // Type label (small, colored)
                    DrawGameText(typeLabel, tx, ty, 11, typeColor);
                    // Item name
                    DrawGameText(name, tx, ty + 14, 14, (Color){230, 225, 240, 255});

                    // Info line
                    if (info[0]) {
                        DrawGameText(info, tx, ty + 30, 13, (Color){180, 200, 180, 255});
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
            DrawGameText(TextFormat("%d", heldCount), mx + slotSize - 20, my + slotSize - 16,14, WHITE);
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

    // Armor colors based on tier
    Color helmetColor = {0, 0, 0, 0}, chestColor = {0, 0, 0, 0}, legColor = {0, 0, 0, 0}, bootColor = {0, 0, 0, 0};
    if (player.armor[0] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[0];
        if (a <= ARMOR_STONE_BOOTS) helmetColor = (Color){140, 140, 140, 255}; // stone
        else if (a <= ARMOR_IRON_BOOTS) helmetColor = (Color){200, 210, 220, 255}; // iron
        else helmetColor = (Color){160, 120, 60, 255}; // wood
    }
    if (player.armor[1] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[1];
        if (a <= ARMOR_STONE_BOOTS) chestColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) chestColor = (Color){200, 210, 220, 255};
        else chestColor = (Color){160, 120, 60, 255};
    }
    if (player.armor[2] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[2];
        if (a <= ARMOR_STONE_BOOTS) legColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) legColor = (Color){200, 210, 220, 255};
        else legColor = (Color){160, 120, 60, 255};
    }
    if (player.armor[3] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[3];
        if (a <= ARMOR_STONE_BOOTS) bootColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) bootColor = (Color){200, 210, 220, 255};
        else bootColor = (Color){160, 120, 60, 255};
    }

    if (facing) {
        // Facing right
        DrawRectangle((int)(px + 2), (int)py, 8, 8, skin);
        DrawRectangle((int)(px + 2), (int)py, 8, 3, hair);
        // Helmet overlay
        if (helmetColor.a > 0) {
            DrawRectangle((int)(px + 1), (int)(py - 1), 10, 5, helmetColor);
            DrawRectangle((int)(px + 2), (int)(py + 4), 8, 2, helmetColor);
        }
        DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, shirt);
        // Chestplate overlay
        if (chestColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 7), 12, 11, chestColor);
            DrawRectangle((int)(px + 1), (int)(py + 8), 10, 9, (Color){
                (unsigned char)(chestColor.r * 0.8f), (unsigned char)(chestColor.g * 0.8f), (unsigned char)(chestColor.b * 0.8f), 255
            });
        }
        // Back arm (no item)
        DrawRectangle((int)(px - 2), (int)(py + 8 + armSwing), 3, 10, skin);
        // Front arm (holds item)
        DrawRectangle((int)(px + 11), (int)(py + 8 - armSwing), 3, 10, skin);
        // Legs
        DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, pants);
        DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, pants);
        // Leggings overlay
        if (legColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 18 + legSwing), 5, 10, legColor);
            DrawRectangle((int)(px + 6), (int)(py + 18 - legSwing), 5, 10, legColor);
        }
        // Boots overlay
        if (bootColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 25 + legSwing), 5, 3, bootColor);
            DrawRectangle((int)(px + 6), (int)(py + 25 - legSwing), 5, 3, bootColor);
        }
    } else {
        // Facing left (mirrored)
        DrawRectangle((int)(px + 2), (int)py, 8, 8, skin);
        DrawRectangle((int)(px + 2), (int)py, 8, 3, hair);
        // Helmet overlay
        if (helmetColor.a > 0) {
            DrawRectangle((int)(px + 1), (int)(py - 1), 10, 5, helmetColor);
            DrawRectangle((int)(px + 2), (int)(py + 4), 8, 2, helmetColor);
        }
        DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
        DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, shirt);
        // Chestplate overlay
        if (chestColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 7), 12, 11, chestColor);
            DrawRectangle((int)(px + 1), (int)(py + 8), 10, 9, (Color){
                (unsigned char)(chestColor.r * 0.8f), (unsigned char)(chestColor.g * 0.8f), (unsigned char)(chestColor.b * 0.8f), 255
            });
        }
        // Front arm (holds item) - left side when facing left
        DrawRectangle((int)(px - 2), (int)(py + 8 - armSwing), 3, 10, skin);
        // Back arm
        DrawRectangle((int)(px + 11), (int)(py + 8 + armSwing), 3, 10, skin);
        // Legs
        DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, pants);
        DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, pants);
        // Leggings overlay
        if (legColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 18 + legSwing), 5, 10, legColor);
            DrawRectangle((int)(px + 6), (int)(py + 18 - legSwing), 5, 10, legColor);
        }
        // Boots overlay
        if (bootColor.a > 0) {
            DrawRectangle((int)(px), (int)(py + 25 + legSwing), 5, 3, bootColor);
            DrawRectangle((int)(px + 6), (int)(py + 25 - legSwing), 5, 3, bootColor);
        }
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

    // Background shadow
    DrawRectangle(startX - 2, startY - 2, totalW + 8, slotSize + 8, (Color){0, 0, 0, 50});
    // Background
    DrawRectangle(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){25, 22, 32, 200});
    // Gradient top
    DrawRectangle(startX - 4, startY - 4, totalW + 8, 12, (Color){30, 27, 38, 200});
    DrawRectangleLines(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){60, 55, 75, 180});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;
        Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
        bool hover = CheckCollisionPointRec(mouse, slotRect);
        bool selected = (i == player.selectedSlot);
        float hA = GetHoverAlpha(400 + i, hover && !selected, GetFrameTime());

        if (hover) hoveredSlot = i;

        if (selected) {
            DrawRectangle(x - 2, y - 2, slotSize + 4, slotSize + 4, (Color){140, 130, 170, 80});
        } else if (hA > 0.01f) {
            unsigned char glowA = (unsigned char)(30 * hA);
            DrawRectangle(x - 1, y - 1, slotSize + 2, slotSize + 2, (Color){100, 95, 120, glowA});
        }
        Color slotColor, borderColor;
        if (selected) {
            slotColor = (Color){100, 95, 110, 230};
            borderColor = (Color){180, 170, 200, 255};
        } else {
            slotColor = (Color){
                (unsigned char)(50 + (int)(15 * hA)),
                (unsigned char)(47 + (int)(15 * hA)),
                (unsigned char)(58 + (int)(20 * hA)),
                (unsigned char)(200 + (int)(20 * hA))
            };
            borderColor = (Color){
                (unsigned char)(70 + (int)(30 * hA)),
                (unsigned char)(65 + (int)(30 * hA)),
                (unsigned char)(80 + (int)(40 * hA)),
                (unsigned char)(200 + (int)(20 * hA))
            };
        }
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        // Gradient (lighter top)
        DrawRectangle(x, y, slotSize, slotSize / 3, (Color){(unsigned char)(slotColor.r + 8), (unsigned char)(slotColor.g + 8), (unsigned char)(slotColor.b + 8), slotColor.a});
        // Rounded corners
        int hcr = 3;
        DrawCircleV((Vector2){(float)(x + hcr), (float)(y + hcr)}, hcr, slotColor);
        DrawCircleV((Vector2){(float)(x + slotSize - hcr), (float)(y + hcr)}, hcr, slotColor);
        DrawCircleV((Vector2){(float)(x + hcr), (float)(y + slotSize - hcr)}, hcr, slotColor);
        DrawCircleV((Vector2){(float)(x + slotSize - hcr), (float)(y + slotSize - hcr)}, hcr, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, borderColor);

        int item = player.inventory[i];
        if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            if (player.inventoryCount[i] > 1) {
                // Shadow for readability
                DrawGameText(TextFormat("%d", player.inventoryCount[i]),
                         x + slotSize - 19, y + slotSize - 14, 13, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", player.inventoryCount[i]),
                         x + slotSize - 20, y + slotSize - 15, 13, WHITE);
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
        DrawGameText(TextFormat("%d", i + 1), x + 2, y + 1,10, numColor);
    }

    // Tooltip for hovered slot
    if (hoveredSlot >= 0) {
        int item = player.inventory[hoveredSlot];
        if (item != BLOCK_AIR && item < BLOCK_COUNT) {
            const char *name = GetBlockName(item);
            int tw = MeasureGameTextWidth(name,14) + 12;
            int tx = (int)mouse.x + 12;
            int ty = (int)mouse.y - 20;
            if (tx + tw > SCREEN_WIDTH) tx = SCREEN_WIDTH - tw - 4;
            if (ty < 4) ty = 4;
            // Tooltip shadow
            DrawRectangle(tx + 2, ty + 2, tw, 18, (Color){0, 0, 0, 50});
            // Tooltip background
            DrawRectangle(tx, ty, tw, 18, (Color){25, 22, 32, 240});
            DrawRectangleLines(tx, ty, tw, 18, (Color){90, 85, 110, 220});
            DrawGameText(name, tx + 6, ty + 3,14, (Color){230, 225, 240, 255});

            // Extra info for tools/food
            char info[32] = { 0 };
            if (IsFood((BlockType)item)) {
                snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue((BlockType)item));
            } else if (IsTool((BlockType)item)) {
                int maxDur = GetToolMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    int pct = player.toolDurability[hoveredSlot] * 100 / maxDur;
                    snprintf(info, sizeof(info), S(STR_TOOLTIP_DUR_SHORT), pct);
                }
            }
            if (info[0]) {
                int iw = MeasureGameTextWidth(info,13) + 12;
                if (iw > tw) tw = iw;
                ty += 18;
                DrawRectangle(tx + 2, ty + 2, tw, 15, (Color){0, 0, 0, 50});
                DrawRectangle(tx, ty, tw, 15, (Color){25, 22, 32, 240});
                DrawRectangleLines(tx, ty, tw, 15, (Color){90, 85, 110, 220});
                DrawGameText(info, tx + 6, ty + 2,13, (Color){180, 200, 180, 255});
            }
        }
    }

    // Selected item name (always visible above hotbar center)
    int selItem = player.inventory[player.selectedSlot];
    if (selItem != BLOCK_AIR && selItem < BLOCK_COUNT) {
        const char *selName = GetBlockName((BlockType)selItem);
        int selW = MeasureGameTextWidth(selName, 13);
        int selX = (SCREEN_WIDTH - selW) / 2;
        int selY = startY - 18;
        DrawRectangle(selX - 4, selY - 2, selW + 8, 16, (Color){25, 22, 32, 180});
        DrawGameText(selName, selX, selY, 13, (Color){220, 215, 240, 200});
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
    int barY = SCREEN_HEIGHT - slotSize - 46;

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

    DrawGameText(TextFormat(S(STR_DBG_FPS), GetFPS()), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_POS), player.position.x, player.position.y), 10, y,16, c); y += lineH;
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    DrawGameText(TextFormat(S(STR_DBG_BLOCK), bx, by), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_CHUNKS), chunkCount), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_TIME), dayNight.timeOfDay), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_LIGHT), dayNight.lightLevel), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_GROUND), player.onGround ? S(STR_YES) : S(STR_NO)), 10, y,16, c); y += lineH;
    // Player status
    DrawGameText(TextFormat(S(STR_DBG_HP), player.health, MAX_HEALTH, player.hunger, MAX_HUNGER), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_OXYGEN), player.oxygen, MAX_OXYGEN, player.xp, MAX_XP), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_UNDERWATER), IsPlayerUnderwater() ? S(STR_YES) : S(STR_NO)), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_SEED), worldSeed), 10, y,16, c);
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
    int textW = MeasureGameTextWidth(messageText, fontSize);
    int x = (SCREEN_WIDTH - textW) / 2;
    int y = SCREEN_HEIGHT / 2 + 80;

    float alpha = messageTimer > 0.5f ? 1.0f : messageTimer * 2.0f;
    unsigned char a = (unsigned char)(alpha * 255);

    // Message shadow
    DrawRectangle(x - 12, y - 4, textW + 28, fontSize + 12, (Color){0, 0, 0, (unsigned char)(a * 0.4f)});
    // Message background
    DrawRectangle(x - 14, y - 6, textW + 28, fontSize + 12, (Color){25, 22, 32, (unsigned char)(a * 0.85f)});
    DrawRectangleLines(x - 14, y - 6, textW + 28, fontSize + 12, (Color){90, 85, 110, (unsigned char)(a * 0.6f)});
    // Text shadow
    DrawGameText(messageText, x + 1, y + 1, fontSize, (Color){0, 0, 0, (unsigned char)(a * 0.5f)});
    DrawGameText(messageText, x, y, fontSize, (Color){messageColor.r, messageColor.g, messageColor.b, a});
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

    // Container shadow
    DrawRectangle(boxX + 4, boxY + 4, boxW, boxH, (Color){0, 0, 0, 60});
    // Container
    DrawRectangle(boxX, boxY, boxW, boxH, (Color){45, 42, 50, 240});
    // Gradient top
    DrawRectangle(boxX, boxY, boxW, 50, (Color){50, 47, 56, 240});
    DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){90, 85, 100, 255});
    DrawRectangleLines(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (Color){65, 60, 75, 200});

    // Title
    const char *title = S(STR_PAUSED);
    int titleW = MeasureGameTextWidth(title, 26);
    DrawGameText(title, boxX + (boxW - titleW) / 2, boxY + 16, 26, (Color){220, 210, 230, 255});

    Vector2 mouse = Win32GetMousePosition();

    // --- Volume Sliders ---
    int sliderX = boxX + 28;
    int sliderW = boxW - 80;
    int sliderY = boxY + 56;

    static int activeSlider = -1; // 0=BGM, 1=SFX, -1=none

    // BGM Volume
    DrawGameText(S(STR_MUSIC_VOLUME), sliderX, sliderY,16, (Color){180, 175, 190, 255});
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
    DrawGameText(bgmText, sliderX + sliderW + 8, sliderY - 3,14, (Color){180, 175, 190, 200});

    // SFX Volume
    sliderY += 34;
    DrawGameText(S(STR_SFX_VOLUME), sliderX, sliderY,16, (Color){180, 175, 190, 255});
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
    DrawGameText(sfxText, sliderX + sliderW + 8, sliderY - 3,14, (Color){180, 175, 190, 200});

    // --- Controls ---
    int ctrlY = sliderY + 34;
    const char *ctrlTitle = S(STR_CONTROLS_TITLE);
    DrawGameText(ctrlTitle, boxX + (boxW - MeasureGameTextWidth(ctrlTitle,16)) / 2, ctrlY,16, (Color){200, 190, 100, 220});
    ctrlY += 22;

    int keyX = boxX + 28;
    int actX = boxX + 140;
    const char *keys[] = { S(STR_KEY_WASD), S(STR_KEY_SPACE), S(STR_KEY_SHIFT), S(STR_KEY_LCLICK), S(STR_KEY_RCLICK), S(STR_KEY_E), S(STR_KEY_H), S(STR_KEY_F3), S(STR_KEY_ESC), S(STR_KEY_19) };
    const char *acts[] = { S(STR_ACT_MOVE), S(STR_ACT_JUMP), S(STR_ACT_SPRINT), S(STR_ACT_BREAK), S(STR_ACT_PLACE), S(STR_ACT_INVENTORY), S(STR_ACT_HEAL), S(STR_ACT_DEBUG), S(STR_ACT_PAUSE), S(STR_ACT_HOTBAR) };
    int numControls = sizeof(keys) / sizeof(keys[0]);

    for (int i = 0; i < numControls; i++) {
        DrawGameText(keys[i], keyX, ctrlY,13, (Color){220, 215, 230, 220});
        DrawGameText(acts[i], actX, ctrlY,13, (Color){170, 165, 180, 220});
        ctrlY += 16;
    }

    // --- Buttons ---
    int btnW = 110;
    int btnH = 34;
    int btnY = boxY + boxH - 52;
    int btnGap = 10;
    int totalBtnW = btnW * 3 + btnGap * 2;
    int btnStartX = boxX + (boxW - totalBtnW) / 2;
    float dt = GetFrameTime();

    // Continue button
    Rectangle btnContinue = { (float)btnStartX, (float)btnY, (float)btnW, (float)btnH };
    bool hoverC = CheckCollisionPointRec(mouse, btnContinue);
    float hC = GetHoverAlpha(50, hoverC, dt);
    unsigned char cR = (unsigned char)(55 + (int)(15 * hC));
    unsigned char cG = (unsigned char)(95 + (int)(35 * hC));
    unsigned char cB = (unsigned char)(55 + (int)(15 * hC));
    Color bgC = (Color){cR, cG, cB, 230};
    DrawRectangleRec(btnContinue, bgC);
    DrawRectangle((int)btnContinue.x, (int)btnContinue.y, btnW, btnH / 3,
                 (Color){(unsigned char)(cR + 12), (unsigned char)(cG + 12), (unsigned char)(cB + 12), 230});
    int cr = 3;
    DrawCircleV((Vector2){btnContinue.x + cr, btnContinue.y + cr}, cr, bgC);
    DrawCircleV((Vector2){btnContinue.x + btnW - cr, btnContinue.y + cr}, cr, bgC);
    DrawCircleV((Vector2){btnContinue.x + cr, btnContinue.y + btnH - cr}, cr, bgC);
    DrawCircleV((Vector2){btnContinue.x + btnW - cr, btnContinue.y + btnH - cr}, cr, bgC);
    unsigned char cBorderA = (unsigned char)(80 + (int)(40 * hC));
    DrawRectangleLinesEx(btnContinue, 1, (Color){120, 200, 120, cBorderA});
    if (hoverC && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DrawRectangle((int)btnContinue.x, (int)btnContinue.y, btnW, btnH, (Color){0, 0, 0, 30});
    }
    DrawGameText(S(STR_CONTINUE), (int)(btnContinue.x + (btnW - MeasureGameTextWidth(S(STR_CONTINUE), 16)) / 2),
             (int)(btnContinue.y + 8), 16, WHITE);

    if (hoverC && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        gamePaused = false;
    }

    // Main Menu button
    Rectangle btnMenu = { (float)(btnStartX + btnW + btnGap), (float)btnY, (float)btnW, (float)btnH };
    bool hoverM = CheckCollisionPointRec(mouse, btnMenu);
    float hM = GetHoverAlpha(51, hoverM, dt);
    unsigned char mR = (unsigned char)(50 + (int)(20 * hM));
    unsigned char mG = (unsigned char)(65 + (int)(25 * hM));
    unsigned char mB = (unsigned char)(100 + (int)(40 * hM));
    Color bgM = (Color){mR, mG, mB, 230};
    DrawRectangleRec(btnMenu, bgM);
    DrawRectangle((int)btnMenu.x, (int)btnMenu.y, btnW, btnH / 3,
                 (Color){(unsigned char)(mR + 12), (unsigned char)(mG + 12), (unsigned char)(mB + 12), 230});
    DrawCircleV((Vector2){btnMenu.x + cr, btnMenu.y + cr}, cr, bgM);
    DrawCircleV((Vector2){btnMenu.x + btnW - cr, btnMenu.y + cr}, cr, bgM);
    DrawCircleV((Vector2){btnMenu.x + cr, btnMenu.y + btnH - cr}, cr, bgM);
    DrawCircleV((Vector2){btnMenu.x + btnW - cr, btnMenu.y + btnH - cr}, cr, bgM);
    unsigned char mBorderA = (unsigned char)(70 + (int)(50 * hM));
    DrawRectangleLinesEx(btnMenu, 1, (Color){130, 170, 230, mBorderA});
    if (hoverM && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DrawRectangle((int)btnMenu.x, (int)btnMenu.y, btnW, btnH, (Color){0, 0, 0, 30});
    }
    DrawGameText(S(STR_MAIN_MENU), (int)(btnMenu.x + (btnW - MeasureGameTextWidth(S(STR_MAIN_MENU), 16)) / 2),
             (int)(btnMenu.y + 8), 16, WHITE);

    if (hoverM && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        if (currentSavePath[0]) SaveWorld(currentSavePath);
        gamePaused = false;
        inventoryOpen = false;
        StartTransition(STATE_MENU);
        menuSelection = 0;
    }

    // Quit button
    Rectangle btnQuit = { (float)(btnStartX + (btnW + btnGap) * 2), (float)btnY, (float)btnW, (float)btnH };
    bool hoverQ = CheckCollisionPointRec(mouse, btnQuit);
    float hQ = GetHoverAlpha(52, hoverQ, dt);
    unsigned char qR = (unsigned char)(100 + (int)(40 * hQ));
    unsigned char qG = (unsigned char)(45 + (int)(15 * hQ));
    unsigned char qB = (unsigned char)(45 + (int)(15 * hQ));
    Color bgQ = (Color){qR, qG, qB, 230};
    DrawRectangleRec(btnQuit, bgQ);
    DrawRectangle((int)btnQuit.x, (int)btnQuit.y, btnW, btnH / 3,
                 (Color){(unsigned char)(qR + 12), (unsigned char)(qG + 12), (unsigned char)(qB + 12), 230});
    DrawCircleV((Vector2){btnQuit.x + cr, btnQuit.y + cr}, cr, bgQ);
    DrawCircleV((Vector2){btnQuit.x + btnW - cr, btnQuit.y + cr}, cr, bgQ);
    DrawCircleV((Vector2){btnQuit.x + cr, btnQuit.y + btnH - cr}, cr, bgQ);
    DrawCircleV((Vector2){btnQuit.x + btnW - cr, btnQuit.y + btnH - cr}, cr, bgQ);
    unsigned char qBorderA = (unsigned char)(70 + (int)(60 * hQ));
    DrawRectangleLinesEx(btnQuit, 1, (Color){200, 100, 100, qBorderA});
    if (hoverQ && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        DrawRectangle((int)btnQuit.x, (int)btnQuit.y, btnW, btnH, (Color){0, 0, 0, 30});
    }
    DrawGameText(S(STR_BTN_QUIT), (int)(btnQuit.x + (btnW - MeasureGameTextWidth(S(STR_BTN_QUIT), 16)) / 2),
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
        const char *text = S(STR_YOU_DIED);
        int fontSize = 52;
        int textW = MeasureGameTextWidth(text, fontSize);
        unsigned char textA = (unsigned char)((alpha - 0.4f) / 0.6f * 255);
        int tx = (SCREEN_WIDTH - textW) / 2;
        int ty = SCREEN_HEIGHT / 2 - 50;
        // Shadow
        DrawGameText(text, tx + 3, ty + 3, fontSize, (Color){0, 0, 0, (unsigned char)(textA * 0.5f)});
        DrawGameText(text, tx, ty, fontSize, (Color){220, 40, 40, textA});
    }

    if (alpha > 0.8f) {
        const char *sub = S(STR_PRESS_SPACE_RESPAWN);
        int subW = MeasureGameTextWidth(sub, 18);
        unsigned char subA = (unsigned char)((alpha - 0.8f) * 5.0f * 255);
        DrawGameText(sub, (SCREEN_WIDTH - subW) / 2, SCREEN_HEIGHT / 2 + 20, 18, (Color){200, 200, 200, subA});

        const char *escHint = S(STR_PRESS_ESC_MENU);
        int escW = MeasureGameTextWidth(escHint,14);
        DrawGameText(escHint, (SCREEN_WIDTH - escW) / 2, SCREEN_HEIGHT / 2 + 48,14, (Color){160, 155, 170, (unsigned char)(subA * 0.7f)});
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
    DrawGameText(S(STR_WORLD_MAP), mapX, mapY - 22, 16, (Color){200, 195, 210, 255});
    DrawGameText(S(STR_PRESS_M_CLOSE), SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT - margin + 5,16, (Color){150, 145, 165, 200});

    // Player coordinates
    DrawGameText(TextFormat(S(STR_PLAYER_COORD), playerBX, playerBY), mapX + mapW - 150, mapY - 22,16, (Color){180, 175, 195, 200});
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
    const char *title = S(STR_TITLE);
    int titleSize = 72;
    int titleW = MeasureGameTextWidth(title, titleSize);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    int titleY = 80;

    // Glow behind title
    float glow = sinf(time * 1.5f) * 0.3f + 0.7f;
    unsigned char glowA = (unsigned char)(40 * glow);
    for (int r = 4; r > 0; r--) {
        DrawGameText(title, titleX - r, titleY, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawGameText(title, titleX + r, titleY, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawGameText(title, titleX, titleY - r, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
        DrawGameText(title, titleX, titleY + r, titleSize, (Color){100, 180, 255, (unsigned char)(glowA / r)});
    }
    // Main title
    DrawGameText(title, titleX + 2, titleY + 2, titleSize, (Color){0, 0, 0, 120});
    DrawGameText(title, titleX, titleY, titleSize, (Color){240, 235, 255, 255});

    // Subtitle with fade
    const char *sub = S(STR_SUBTITLE);
    int subW = MeasureGameTextWidth(sub, 18);
    unsigned char subA = (unsigned char)(180 + sinf(time * 2.0f) * 30);
    DrawGameText(sub, (SCREEN_WIDTH - subW) / 2, 165, 18, (Color){180, 175, 200, subA});

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
    const char *btnLabels[] = { S(STR_BTN_NEW_GAME), S(STR_BTN_LOAD_GAME), S(STR_BTN_SETTINGS), S(STR_BTN_QUIT) };
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

    float dt = GetFrameTime();

    for (int i = 0; i < btnCount; i++) {
        int by = btnY + i * spacing;
        Rectangle btn = { (float)btnX, (float)by, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (menuSelection == i);
        float hAlpha = GetHoverAlpha(i, hover && btnEnabled[i], dt);

        Color bg, border;
        if (!btnEnabled[i]) {
            bg = (Color){40, 40, 45, 150};
            border = (Color){50, 50, 55, 120};
        } else if (sel) {
            bg = btnSelColors[i];
            border = btnBorderSel[i];
        } else {
            // Smooth hover interpolation
            unsigned char r = (unsigned char)(btnNormColors[i].r + (int)(15 * hAlpha));
            unsigned char g = (unsigned char)(btnNormColors[i].g + (int)(15 * hAlpha));
            unsigned char b = (unsigned char)(btnNormColors[i].b + (int)(15 * hAlpha));
            unsigned char a = (unsigned char)(btnNormColors[i].a + (int)(20 * hAlpha));
            bg = (Color){r, g, b, a};
            border = btnBorderNorm[i];
        }

        // Button glow on selected or hover (smooth)
        if (sel && btnEnabled[i]) {
            DrawRectangle(btnX - 2, by - 2, btnW + 4, btnH + 4, (Color){
                btnBorderSel[i].r, btnBorderSel[i].g, btnBorderSel[i].b, 40
            });
        } else if (hAlpha > 0.01f && btnEnabled[i]) {
            unsigned char glowA = (unsigned char)(25 * hAlpha);
            DrawRectangle(btnX - 1, by - 1, btnW + 2, btnH + 2, (Color){
                btnBorderNorm[i].r, btnBorderNorm[i].g, btnBorderNorm[i].b, glowA
            });
        }

        // Button background with gradient (lighter top)
        DrawRectangleRec(btn, bg);
        DrawRectangle(btnX, btnY + i * spacing, btnW, btnH / 3,
                     (Color){(unsigned char)(bg.r + 12), (unsigned char)(bg.g + 12), (unsigned char)(bg.b + 12), bg.a});
        // Rounded corners (draw circles at corners)
        int cr = 4;
        DrawCircleV((Vector2){(float)(btnX + cr), (float)(by + cr)}, cr, bg);
        DrawCircleV((Vector2){(float)(btnX + btnW - cr), (float)(by + cr)}, cr, bg);
        DrawCircleV((Vector2){(float)(btnX + cr), (float)(by + btnH - cr)}, cr, bg);
        DrawCircleV((Vector2){(float)(btnX + btnW - cr), (float)(by + btnH - cr)}, cr, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);

        // Click feedback (brief darken flash)
        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && btnEnabled[i]) {
            DrawRectangle(btnX, by, btnW, btnH, (Color){0, 0, 0, 40});
        }

        Color textColor = btnEnabled[i] ? WHITE : (Color){100, 100, 110, 150};
        // Text shadow for better readability
        int textW = MeasureGameTextWidth(btnLabels[i],22);
        DrawGameText(btnLabels[i], btnX + (btnW - textW) / 2 + 1, by + 12,22, (Color){0, 0, 0, 100});
        DrawGameText(btnLabels[i], btnX + (btnW - textW) / 2, by + 11,22, textColor);
    }

    // Bottom hints
    const char *hint1 = S(STR_HINT_NAVIGATE);
    DrawGameText(hint1, (SCREEN_WIDTH - MeasureGameTextWidth(hint1,14)) / 2, SCREEN_HEIGHT - 50, 13,
             (Color){160, 155, 170, 180});
    const char *hint2 = S(STR_HINT_CONTROLS);
    DrawGameText(hint2, (SCREEN_WIDTH - MeasureGameTextWidth(hint2,13)) / 2, SCREEN_HEIGHT - 30, 13,
             (Color){130, 125, 140, 150});

    // Version
    DrawGameText("v0.3", 10, SCREEN_HEIGHT - 20,13, (Color){100, 95, 110, 120});
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
    const char *title = (slotSelectMode == 0) ? S(STR_NEW_GAME_TITLE) : S(STR_LOAD_GAME_TITLE);
    int titleSize = 48;
    int titleW = MeasureGameTextWidth(title, titleSize);
    DrawGameText(title, (SCREEN_WIDTH - titleW) / 2 + 2, 42, titleSize, (Color){0, 0, 0, 100});
    DrawGameText(title, (SCREEN_WIDTH - titleW) / 2, 40, titleSize, (Color){220, 215, 240, 255});

    // Subtitle
    const char *sub = (slotSelectMode == 0) ? S(STR_NEW_GAME_SUB) : S(STR_LOAD_GAME_SUB);
    int subW = MeasureGameTextWidth(sub, 16);
    DrawGameText(sub, (SCREEN_WIDTH - subW) / 2, 100, 16, (Color){160, 155, 180, 200});

    Vector2 mouse = Win32GetMousePosition();

    // Seed input (new game mode only)
    int seedBoxY = 126;
    if (slotSelectMode == 0) {
        int seedBoxW = 240;
        int seedBoxH = 22;
        int seedBoxX = (SCREEN_WIDTH - seedBoxW) / 2;
        Rectangle seedBox = { (float)seedBoxX, (float)seedBoxY, (float)seedBoxW, (float)seedBoxH };
        bool seedFocused = CheckCollisionPointRec(mouse, seedBox);

        DrawGameText(S(STR_SEED), seedBoxX - 42, seedBoxY + 4,14, (Color){180, 175, 195, 200});
        DrawRectangle(seedBoxX, seedBoxY, seedBoxW, seedBoxH, (Color){30, 28, 38, 220});
        DrawRectangleLines(seedBoxX, seedBoxY, seedBoxW, seedBoxH,
                           seedFocused ? (Color){100, 150, 220, 200} : (Color){60, 55, 75, 180});

        if (seedInputLen > 0) {
            DrawGameText(seedInputBuf, seedBoxX + 6, seedBoxY + 5,14, (Color){200, 220, 200, 255});
        } else {
            DrawGameText(S(STR_RANDOM), seedBoxX + 6, seedBoxY + 5,14, (Color){100, 95, 115, 150});
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
        float hA = GetHoverAlpha(60 + i, hover && !sel, GetFrameTime());

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
        } else {
            bg = (Color){
                (unsigned char)(38 + (int)(10 * hA)),
                (unsigned char)(42 + (int)(13 * hA)),
                (unsigned char)(55 + (int)(13 * hA)),
                (unsigned char)(210 + (int)(10 * hA))
            };
            border = (Color){
                (unsigned char)(70 + (int)(20 * hA)),
                (unsigned char)(75 + (int)(25 * hA)),
                (unsigned char)(95 + (int)(35 * hA)),
                (unsigned char)(180 + (int)(20 * hA))
            };
        }

        // Selection glow
        if (sel && usable) {
            DrawRectangle(slotX - 2, sy - 2, slotW + 4, slotH + 4, (Color){100, 150, 220, 35});
        }
        // Hover glow (smooth)
        if (hA > 0.01f && !sel && usable) {
            unsigned char glowA = (unsigned char)(30 * hA);
            DrawRectangle(slotX - 1, sy - 1, slotW + 2, slotH + 2, (Color){90, 100, 130, glowA});
        }

        DrawRectangle(slotX, sy, slotW, slotH, bg);
        DrawRectangleLinesEx(slotRect, sel ? 2 : 1, border);

        // Slot number
        char slotLabel[16];
        snprintf(slotLabel, sizeof(slotLabel), S(STR_SLOT), i + 1);
        Color labelColor = usable ? (Color){200, 195, 220, 255} : (Color){100, 95, 110, 150};
        DrawGameText(slotLabel, slotX + 16, sy + 10,22, labelColor);

        if (info.exists) {
            char seedText[32];
            snprintf(seedText, sizeof(seedText), S(STR_SEED_DISPLAY), info.seed);
            DrawGameText(seedText, slotX + 16, sy + 36,14, (Color){140, 180, 140, 200});

            char sizeText[32];
            snprintf(sizeText, sizeof(sizeText), "%dx%d", info.worldW, info.worldH);
            DrawGameText(sizeText, slotX + 16, sy + 54,14, (Color){130, 130, 150, 180});

            const char *status = S(STR_OCCUPIED);
            int statusW = MeasureGameTextWidth(status,14);
            DrawGameText(status, slotX + slotW - statusW - 16, sy + 12,14, (Color){100, 180, 100, 200});

            if (blockAtlas.id > 0) {
                Rectangle src = { (float)(BLOCK_GRASS * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(slotX + slotW - 48), (float)(sy + 36), 24, 24 };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, (Color){255, 255, 255, 150});
            }
        } else {
            const char *emptyText = (slotSelectMode == 0) ? S(STR_EMPTY_NEW) : S(STR_EMPTY_LOAD);
            Color emptyColor = (slotSelectMode == 0) ? (Color){140, 160, 180, 180} : (Color){100, 95, 110, 120};
            DrawGameText(emptyText, slotX + 16, sy + 40,16, emptyColor);
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
    const char *backText = S(STR_BACK);
    int backTextW = MeasureGameTextWidth(backText, 18);
    DrawGameText(backText, backBtnX + (backBtnW - backTextW) / 2, backBtnY + 9, 18, WHITE);

    // Hints
    const char *hint = S(STR_HINT_SLOT);
    DrawGameText(hint, (SCREEN_WIDTH - MeasureGameTextWidth(hint,14)) / 2, SCREEN_HEIGHT - 30, 14,
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
    const char *title = isDelete ? S(STR_DELETE_SAVE) : S(STR_OVERWRITE_SAVE);
    Color titleColor = isDelete ? (Color){230, 100, 100, 255} : (Color){230, 180, 80, 255};
    int titleW = MeasureGameTextWidth(title,24);
    DrawGameText(title, dlgX + (dlgW - titleW) / 2, dlgY + 18,24, titleColor);

    // Message
    char msg[64];
    snprintf(msg, sizeof(msg), S(STR_SLOT_HAS_DATA), confirmDialogSlot + 1);
    int msgW = MeasureGameTextWidth(msg,16);
    DrawGameText(msg, dlgX + (dlgW - msgW) / 2, dlgY + 52,16, (Color){190, 185, 200, 255});

    const char *warn = S(STR_CANNOT_UNDO);
    int warnW = MeasureGameTextWidth(warn,14);
    DrawGameText(warn, dlgX + (dlgW - warnW) / 2, dlgY + 72,14, (Color){200, 120, 100, 220});

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
    const char *yesText = isDelete ? S(STR_YES_DELETE) : S(STR_YES_OVERWRITE);
    int yesTextW = MeasureGameTextWidth(yesText,16);
    DrawGameText(yesText, (int)(yesBtn.x + (btnW - yesTextW) / 2), btnY + 9,16, WHITE);

    // No button
    Rectangle noBtn = { (float)(dlgX + dlgW - btnW - 30), (float)btnY, (float)btnW, (float)btnH };
    bool noHover = CheckCollisionPointRec(mouse, noBtn);
    Color noBg = noHover ? (Color){80, 80, 100, 255} : (Color){60, 58, 75, 230};
    DrawRectangleRec(noBtn, noBg);
    DrawRectangleLinesEx(noBtn, 1, (Color){90, 85, 110, 200});
    const char *noText = S(STR_CANCEL);
    int noTextW = MeasureGameTextWidth(noText,16);
    DrawGameText(noText, (int)(noBtn.x + (btnW - noTextW) / 2), btnY + 9,16, WHITE);

    // Key hints
    const char *keyHint = S(STR_CONFIRM_KEYS);
    int keyHintW = MeasureGameTextWidth(keyHint,13);
    DrawGameText(keyHint, dlgX + (dlgW - keyHintW) / 2, dlgY + dlgH - 14,13, (Color){130, 125, 145, 180});
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
    const char *title = S(STR_SETTINGS);
    int titleW = MeasureGameTextWidth(title, 48);
    DrawGameText(title, (SCREEN_WIDTH - titleW) / 2 + 2, 18, 48, (Color){0, 0, 0, 100});
    DrawGameText(title, (SCREEN_WIDTH - titleW) / 2, 16, 48, (Color){220, 215, 240, 255});

    // Decorative line
    int lineW = 200;
    int lineX = (SCREEN_WIDTH - lineW) / 2;
    DrawRectangle(lineX, 70, lineW, 1, (Color){100, 95, 110, 150});

    Vector2 mouse = Win32GetMousePosition();

    // Settings panel - fits entirely on screen
    int panelW = 520;
    int panelH = 620;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = 80;

    // Panel shadow
    DrawRectangle(panelX + 3, panelY + 3, panelW, panelH, (Color){0, 0, 0, 50});
    // Panel background
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){25, 22, 32, 240});
    // Panel gradient (lighter top)
    DrawRectangle(panelX, panelY, panelW, panelH / 3, (Color){30, 27, 40, 240});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){70, 65, 85, 200});
    DrawRectangleLines(panelX + 1, panelY + 1, panelW - 2, panelH - 2, (Color){50, 45, 65, 150});

    int leftX = panelX + 30;
    int rightX = panelX + panelW / 2 + 10;
    int sliderW = 180;
    char volText[16];

    // ============================================================
    // Section: Audio
    // ============================================================
    int sectionY = panelY + 12;
    // Section header with line
    DrawRectangle(leftX, sectionY + 8, 4, 14, (Color){200, 190, 100, 220});
    DrawGameText(S(STR_SECTION_AUDIO), leftX + 12, sectionY, 15, (Color){200, 190, 100, 220});
    sectionY += 26;

    // Music Volume
    DrawGameText(S(STR_MUSIC_VOLUME), leftX, sectionY, 13, (Color){200, 195, 215, 255});
    int musicSliderX = leftX + MeasureGameTextWidth(S(STR_MUSIC_VOLUME), 13) + 10;
    int musicSliderW = panelX + panelW - 30 - musicSliderX - 40;
    DrawRectangle(musicSliderX, sectionY + 6, musicSliderW, 6, (Color){40, 38, 50, 200});
    DrawRectangleLines(musicSliderX, sectionY + 6, musicSliderW, 6, (Color){60, 55, 75, 150});

    extern float bgmVolumeSlider;
    float bgmVal = bgmVolumeSlider;
    int handleX = musicSliderX + (int)(bgmVal * musicSliderW);
    Rectangle bgmHandle = { (float)(handleX - 6), (float)sectionY, 12, 18 };
    bool bgmHover = CheckCollisionPointRec(mouse, bgmHandle);
    // Slider fill with gradient
    DrawRectangle(musicSliderX, sectionY + 6, (int)(bgmVal * musicSliderW), 6, (Color){80, 160, 80, 200});
    DrawRectangle(musicSliderX, sectionY + 6, (int)(bgmVal * musicSliderW), 3, (Color){100, 200, 100, 100});
    // Handle
    DrawRectangle(handleX - 6, sectionY, 12, 18, bgmHover ? (Color){120, 180, 120, 255} : (Color){80, 150, 80, 255});
    DrawRectangleLines(handleX - 6, sectionY, 12, 18, (Color){160, 220, 160, 200});
    sprintf(volText, "%d%%", (int)(bgmVal * 100));
    DrawGameText(volText, musicSliderX + musicSliderW + 8, sectionY, 13, (Color){180, 175, 195, 220});

    Rectangle bgmTrack = { (float)musicSliderX, (float)(sectionY - 4), (float)musicSliderW, 26 };
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (bgmHover || CheckCollisionPointRec(mouse, bgmTrack))) {
        bgmVolumeSlider = (mouse.x - musicSliderX) / (float)musicSliderW;
        if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
        if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
        SetBGMVolume(bgmVolumeSlider);
    }
    sectionY += 26;

    // SFX Volume
    DrawGameText(S(STR_SOUND_EFFECTS), leftX, sectionY, 13, (Color){200, 195, 215, 255});
    int sfxSliderX = leftX + MeasureGameTextWidth(S(STR_SOUND_EFFECTS), 13) + 10;
    int sfxSliderW = panelX + panelW - 30 - sfxSliderX - 40;
    DrawRectangle(sfxSliderX, sectionY + 6, sfxSliderW, 6, (Color){40, 38, 50, 200});
    DrawRectangleLines(sfxSliderX, sectionY + 6, sfxSliderW, 6, (Color){60, 55, 75, 150});

    extern float sfxVolumeSlider;
    float sfxVal = sfxVolumeSlider;
    int sfxHandleX = sfxSliderX + (int)(sfxVal * sfxSliderW);
    Rectangle sfxHandle = { (float)(sfxHandleX - 6), (float)sectionY, 12, 18 };
    bool sfxHover = CheckCollisionPointRec(mouse, sfxHandle);
    DrawRectangle(sfxSliderX, sectionY + 6, (int)(sfxVal * sfxSliderW), 6, (Color){80, 120, 200, 200});
    DrawRectangle(sfxSliderX, sectionY + 6, (int)(sfxVal * sfxSliderW), 3, (Color){100, 150, 230, 100});
    DrawRectangle(sfxHandleX - 6, sectionY, 12, 18, sfxHover ? (Color){120, 150, 200, 255} : (Color){80, 120, 180, 255});
    DrawRectangleLines(sfxHandleX - 6, sectionY, 12, 18, (Color){140, 180, 230, 200});
    sprintf(volText, "%d%%", (int)(sfxVal * 100));
    DrawGameText(volText, sfxSliderX + sfxSliderW + 8, sectionY, 13, (Color){180, 175, 195, 220});

    Rectangle sfxTrack = { (float)sfxSliderX, (float)(sectionY - 4), (float)sfxSliderW, 26 };
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && (sfxHover || CheckCollisionPointRec(mouse, sfxTrack))) {
        sfxVolumeSlider = (mouse.x - sfxSliderX) / (float)sfxSliderW;
        if (sfxVolumeSlider < 0.0f) sfxVolumeSlider = 0.0f;
        if (sfxVolumeSlider > 1.0f) sfxVolumeSlider = 1.0f;
        SetSFXVolume(sfxVolumeSlider);
    }
    sectionY += 26;

    // ============================================================
    // Section: Display
    // ============================================================
    DrawRectangle(leftX, sectionY + 8, 4, 14, (Color){100, 160, 220, 220});
    DrawGameText(S(STR_SECTION_DISPLAY), leftX + 12, sectionY, 15, (Color){100, 160, 220, 220});
    sectionY += 24;

    // Window mode buttons
    DrawGameText(S(STR_WINDOW_MODE), leftX, sectionY, 13, (Color){200, 195, 215, 255});
    sectionY += 20;

    int btnW = 100;
    int btnH = 28;
    int btnGap = 6;
    const char *modeNames[] = { S(STR_WINDOWED), S(STR_FULLSCREEN), S(STR_BORDERLESS) };
    Color modeColors[] = {
        {55, 80, 55, 230}, {55, 65, 100, 230}, {80, 55, 80, 230}
    };
    Color modeColorsSel[] = {
        {80, 140, 80, 255}, {80, 100, 160, 255}, {140, 80, 140, 255}
    };

    for (int i = 0; i < 3; i++) {
        int bx = leftX + i * (btnW + btnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (windowMode == i);
        float hA = GetHoverAlpha(300 + i, hover && !sel, GetFrameTime());

        Color bg;
        if (sel) {
            bg = modeColorsSel[i];
        } else {
            bg = (Color){
                (unsigned char)(modeColors[i].r + (int)(12 * hA)),
                (unsigned char)(modeColors[i].g + (int)(12 * hA)),
                (unsigned char)(modeColors[i].b + (int)(12 * hA)),
                (unsigned char)(230 + (int)(10 * hA))
            };
        }
        Color border = sel ? (Color){200, 200, 220, 255} :
            (Color){(unsigned char)(80 + (int)(20 * hA)), (unsigned char)(75 + (int)(20 * hA)), (unsigned char)(95 + (int)(25 * hA)), (unsigned char)(180 + (int)(20 * hA))};

        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);
        int textW = MeasureGameTextWidth(modeNames[i], 13);
        DrawGameText(modeNames[i], bx + (btnW - textW) / 2, sectionY + 7, 13,
                 sel ? WHITE : (Color){180, 175, 195, 220});

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            ApplyWindowMode(i);
            PlaySoundUIClick();
        }
    }
    sectionY += btnH + 10;

    // DPI info
    Vector2 dpiScale = GetWindowScaleDPI();
    int mon = GetCurrentMonitor();
    int monW = GetMonitorWidth(mon);
    int monH = GetMonitorHeight(mon);
    char dpiInfo[64];
    snprintf(dpiInfo, sizeof(dpiInfo), "Monitor: %dx%d  DPI: %.0f%%", monW, monH, dpiScale.x * 100);
    DrawGameText(dpiInfo, leftX, sectionY, 12, (Color){140, 135, 155, 180});

    char resInfo[64];
    snprintf(resInfo, sizeof(resInfo), "Window: %dx%d", GetScreenWidth(), GetScreenHeight());
    DrawGameText(resInfo, rightX, sectionY, 12, (Color){140, 135, 155, 180});
    sectionY += 20;

    // ============================================================
    // Section: Language & Font (side by side)
    // ============================================================
    DrawRectangle(leftX, sectionY + 8, 4, 14, (Color){180, 120, 200, 220});
    DrawGameText(S(STR_SECTION_LANGUAGE), leftX + 12, sectionY, 15, (Color){180, 120, 200, 220});
    sectionY += 24;

    // Language buttons
    DrawGameText(S(STR_LANGUAGE), leftX, sectionY, 13, (Color){200, 195, 215, 255});
    sectionY += 18;

    const char *langNames[] = { S(STR_LANG_NAME_EN), S(STR_LANG_NAME_ZH), S(STR_LANG_NAME_JA) };
    int langBtnW = 100;
    int langBtnH = 26;
    int langBtnGap = 6;
    Color langColors[] = {
        {55, 65, 100, 230}, {80, 55, 55, 230}, {55, 80, 55, 230}
    };
    Color langColorsSel[] = {
        {80, 100, 160, 255}, {140, 80, 80, 255}, {80, 140, 80, 255}
    };

    for (int i = 0; i < LANG_COUNT; i++) {
        int bx = leftX + i * (langBtnW + langBtnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)langBtnW, (float)langBtnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (language == i);
        float hA = GetHoverAlpha(310 + i, hover && !sel, GetFrameTime());

        Color bg;
        if (sel) {
            bg = langColorsSel[i];
        } else {
            bg = (Color){
                (unsigned char)(langColors[i].r + (int)(12 * hA)),
                (unsigned char)(langColors[i].g + (int)(12 * hA)),
                (unsigned char)(langColors[i].b + (int)(12 * hA)),
                (unsigned char)(230 + (int)(10 * hA))
            };
        }
        Color border = sel ? (Color){200, 200, 220, 255} :
            (Color){(unsigned char)(80 + (int)(20 * hA)), (unsigned char)(75 + (int)(20 * hA)), (unsigned char)(95 + (int)(25 * hA)), (unsigned char)(180 + (int)(20 * hA))};

        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);
        int textW = MeasureGameTextWidth(langNames[i], 13);
        DrawGameText(langNames[i], bx + (langBtnW - textW) / 2, sectionY + 6, 13,
                 sel ? WHITE : (Color){180, 175, 195, 220});

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            language = (Language)i;
            PlaySoundUIClick();
        }
    }
    sectionY += langBtnH + 14;

    // Font buttons
    DrawGameText(S(STR_FONT), leftX, sectionY, 13, (Color){200, 195, 215, 255});
    sectionY += 18;

    const char *fontNames[] = { S(STR_FONT_NAME_BUILTIN), S(STR_FONT_NAME_LXGW) };
    int fontBtnW = 130;
    int fontBtnH = 26;
    int fontBtnGap = 6;

    for (int i = 0; i < 2; i++) {
        int bx = leftX + i * (fontBtnW + fontBtnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)fontBtnW, (float)fontBtnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (i == 0 && !useCustomFont) || (i == 1 && useCustomFont);
        float hA = GetHoverAlpha(320 + i, hover && !sel, GetFrameTime());

        Color bg;
        if (sel) {
            bg = (Color){80, 100, 140, 255};
        } else {
            bg = (Color){
                (unsigned char)(40 + (int)(10 * hA)),
                (unsigned char)(42 + (int)(13 * hA)),
                (unsigned char)(58 + (int)(17 * hA)),
                (unsigned char)(230 + (int)(10 * hA))
            };
        }
        Color border = sel ? (Color){200, 200, 220, 255} :
            (Color){(unsigned char)(80 + (int)(20 * hA)), (unsigned char)(75 + (int)(20 * hA)), (unsigned char)(95 + (int)(25 * hA)), (unsigned char)(180 + (int)(20 * hA))};

        DrawRectangleRec(btn, bg);
        DrawRectangleLinesEx(btn, sel ? 2 : 1, border);
        int textW = MeasureGameTextWidth(fontNames[i], 13);
        DrawGameText(fontNames[i], bx + (fontBtnW - textW) / 2, sectionY + 6, 13,
                 sel ? WHITE : (Color){180, 175, 195, 220});

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            if (i == 0) {
                UnloadGameFont();
            } else {
                ReloadGameFont("assets/fonts/LXGWWenKaiLite-Regular.ttf");
            }
            PlaySoundUIClick();
        }
    }
    sectionY += fontBtnH + 18;

    // ============================================================
    // Section: Controls (compact 2-column)
    // ============================================================
    DrawRectangle(leftX, sectionY + 8, 4, 14, (Color){100, 200, 140, 220});
    DrawGameText(S(STR_CONTROLS_TITLE), leftX + 12, sectionY, 15, (Color){100, 200, 140, 220});
    sectionY += 22;

    const char *controls[] = {
        S(STR_KEY_WASD),    S(STR_ACT_MOVE),
        S(STR_KEY_SPACE),   S(STR_ACT_JUMP),
        S(STR_KEY_SHIFT),   S(STR_ACT_SPRINT),
        S(STR_KEY_LCLICK),  S(STR_ACT_BREAK),
        S(STR_KEY_RCLICK),  S(STR_ACT_PLACE),
        S(STR_KEY_E),       S(STR_ACT_INVENTORY),
        S(STR_KEY_H),       S(STR_ACT_HEAL),
        S(STR_KEY_F3),      S(STR_ACT_DEBUG),
        S(STR_KEY_F11),     S(STR_ACT_FULLSCREEN),
        S(STR_KEY_ESC),     S(STR_ACT_PAUSE),
        S(STR_KEY_19),      S(STR_ACT_HOTBAR)
    };
    int numControls = sizeof(controls) / sizeof(controls[0]) / 2;
    int keyX1 = leftX + 6;
    int actX1 = leftX + 100;
    int keyX2 = rightX + 6;
    int actX2 = rightX + 100;
    int halfCtrl = (numControls + 1) / 2;
    for (int i = 0; i < halfCtrl; i++) {
        // Left column
        DrawGameText(controls[i * 2], keyX1, sectionY, 12, (Color){200, 195, 215, 220});
        DrawGameText(controls[i * 2 + 1], actX1, sectionY, 12, (Color){150, 145, 165, 200});
        // Right column
        int ri = i + halfCtrl;
        if (ri < numControls) {
            DrawGameText(controls[ri * 2], keyX2, sectionY, 12, (Color){200, 195, 215, 220});
            DrawGameText(controls[ri * 2 + 1], actX2, sectionY, 12, (Color){150, 145, 165, 200});
        }
        sectionY += 14;
    }

    // Back button - inside panel at bottom
    int backBtnW = 140;
    int backBtnH = 32;
    int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
    int backBtnY = panelY + panelH - backBtnH - 12;
    Rectangle backBtn = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
    bool backHover = CheckCollisionPointRec(mouse, backBtn);
    float hBack = GetHoverAlpha(330, backHover, GetFrameTime());
    Color backBg = (Color){
        (unsigned char)(55 + (int)(25 * hBack)),
        (unsigned char)(50 + (int)(25 * hBack)),
        (unsigned char)(70 + (int)(30 * hBack)),
        (unsigned char)(220 + (int)(20 * hBack))
    };
    DrawRectangleRec(backBtn, backBg);
    DrawRectangleLinesEx(backBtn, backHover ? 2 : 1, (Color){100, 95, 120, 200});
    const char *backText = S(STR_BACK);
    int backTextW = MeasureGameTextWidth(backText, 16);
    DrawGameText(backText, backBtnX + (backBtnW - backTextW) / 2, backBtnY + 8, 16, WHITE);

    // ESC to go back
    if (Win32IsKeyPressed(KEY_ESCAPE) || (backHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        StartTransition(STATE_MENU);
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
