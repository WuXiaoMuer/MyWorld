#include "types.h"
#include <math.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Block Info Table
//----------------------------------------------------------------------------------
const BlockInfo blockInfo[BLOCK_COUNT] = {
    {"Air",         {0,0,0,0},          {0,0,0,0},          false, true,  false},
    {"Grass",       {76,153,0,255},     {56,118,29,255},    true,  false, true},
    {"Dirt",        {134,96,67,255},    {115,80,55,255},    true,  false, true},
    {"Stone",       {128,128,128,255},  {105,105,105,255},  true,  false, true},
    {"Cobblestone", {100,100,100,255},  {80,80,80,255},     true,  false, true},
    {"Wood",        {101,67,33,255},    {76,50,25,255},     true,  false, true},
    {"Leaves",      {34,120,15,255},    {25,90,10,255},     true,  true,  true},
    {"Sand",        {210,190,120,255},  {190,170,100,255},  true,  false, true},
    {"Water",       {30,100,200,180},   {20,80,180,180},    false, true,  false},
    {"Coal Ore",    {80,80,80,255},     {40,40,40,255},     true,  false, true},
    {"Iron Ore",    {130,110,100,255},  {200,180,160,255},  true,  false, true},
    {"Planks",      {180,140,80,255},   {160,120,60,255},   true,  false, true},
    {"Brick",       {170,74,68,255},    {140,55,50,255},    true,  false, true},
    {"Glass",       {200,220,255,100},  {180,200,240,100},  true,  true,  true},
    {"Bedrock",     {50,50,50,255},     {30,30,30,255},     true,  false, false},
    // New terrain blocks
    {"Gravel",      {140,130,120,255},  {110,100,90,255},   true,  false, true},
    {"Clay",        {170,150,130,255},  {140,120,100,255},  true,  false, true},
    {"Sandstone",   {200,180,130,255},  {170,150,100,255},  true,  false, true},
    // Decorative blocks
    {"Torch",       {255,200,50,255},   {200,150,20,255},   false, true,  true},
    {"Flower",      {255,80,80,255},    {255,200,50,255},   false, true,  true},
    {"Tall Grass",  {50,160,30,255},    {40,130,20,255},    false, true,  true},
    {"Furnace",     {100,100,100,255},  {60,60,60,255},     true,  false, true},
    {"Bed",         {200,60,60,255},    {160,40,40,255},    true,  false, true},
    // Items (not placeable, not solid)
    {"Stick",       {160,120,60,255},   {130,95,45,255},    false, false, false},
    {"Coal",        {40,40,40,255},     {25,25,25,255},     false, false, false},
    {"Iron Ingot",  {220,210,200,255},  {190,180,170,255},  false, false, false},
    // Tools (not placeable, not solid)
    {"Wood Pick",   {180,140,80,255},   {140,100,50,255},   false, false, false},
    {"Wood Axe",    {180,140,80,255},   {140,100,50,255},   false, false, false},
    {"Wood Sword",  {180,140,80,255},   {140,100,50,255},   false, false, false},
    {"Wood Shovel", {180,140,80,255},   {140,100,50,255},   false, false, false},
    {"Stone Pick",  {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Axe",   {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Sword", {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Shovel",{128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Iron Pick",   {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Axe",    {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Sword",  {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Shovel", {200,180,160,255},  {170,150,130,255},  false, false, false},
    // Food (not placeable, not solid)
    {"Raw Pork",    {200,130,130,255},  {170,100,100,255},  false, false, false},
    {"Cooked Pork", {180,100,60,255},   {150,70,40,255},    false, false, false},
    {"Apple",       {200,50,50,255},    {150,30,30,255},    false, false, false},
    {"Bread",       {210,180,100,255},  {180,150,70,255},   false, false, false},
    // Interactive blocks
    {"Crafting Table",{140,110,60,255}, {100,80,40,255},    true,  false, true},
    // Armor (not placeable, not solid)
    {"Wood Helmet",    {180,140,80,255},  {140,100,50,255},  false, false, false},
    {"Wood Chest",     {180,140,80,255},  {140,100,50,255},  false, false, false},
    {"Wood Leggings",  {180,140,80,255},  {140,100,50,255},  false, false, false},
    {"Wood Boots",     {180,140,80,255},  {140,100,50,255},  false, false, false},
    {"Stone Helmet",   {128,128,128,255}, {100,100,100,255}, false, false, false},
    {"Stone Chest",    {128,128,128,255}, {100,100,100,255}, false, false, false},
    {"Stone Leggings", {128,128,128,255}, {100,100,100,255}, false, false, false},
    {"Stone Boots",    {128,128,128,255}, {100,100,100,255}, false, false, false},
    {"Iron Helmet",    {200,180,160,255}, {170,150,130,255}, false, false, false},
    {"Iron Chest",     {200,180,160,255}, {170,150,130,255}, false, false, false},
    {"Iron Leggings",  {200,180,160,255}, {170,150,130,255}, false, false, false},
    {"Iron Boots",     {200,180,160,255}, {170,150,130,255}, false, false, false},
};

//----------------------------------------------------------------------------------
// Block Pixel Art Generation
//----------------------------------------------------------------------------------
void DrawBlockPattern(Image *img, int px, int py, BlockType bt, int worldX, int worldY)
{
    unsigned int varSeed = (unsigned int)(worldX * 7919 + worldY * 104729);
    Color base = blockInfo[bt].baseColor;
    Color detail = blockInfo[bt].detailColor;

    switch (bt) {
    case BLOCK_GRASS:
        for (int y = 3; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = detail;
                if ((hash2D(x + worldX * 16, y + worldY * 16, 1) % 5) == 0) c = (Color){100, 70, 45, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 16; x++) {
                int wave = (int)(sinf(x * 0.8f + varSeed * 0.1f) * 1.5f);
                if (y + wave < 3) {
                    Color c = base;
                    if ((hash2D(x + worldX * 16, y + worldY * 16, 2) % 4) == 0)
                        c = (Color){60, 130, 0, 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                } else {
                    ImageDrawPixel(img, px + x, py + y, detail);
                }
            }
        break;

    case BLOCK_DIRT:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 3);
                if (h % 7 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){150, 110, 80, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_STONE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = detail;
                else if (h % 13 == 0) c = (Color){140, 140, 140, 255};
                if (y == 4 || y == 11) c = detail;
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_COBBLESTONE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 5);
                if ((x % 8 < 1) || (y % 8 < 1)) c = detail;
                else if (h % 6 == 0) c = (Color){115, 115, 115, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_WOOD:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if (x == 4 || x == 12) c = detail;
                if (y % 5 == 0) c = detail;
                if ((hash2D(x + worldX * 16, y + worldY * 16, 6) % 9) == 0) c = (Color){88, 58, 28, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_LEAVES:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 7);
                if (h % 5 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){45, 140, 20, 255};
                if (h % 17 == 0) c = (Color){0, 0, 0, 0};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_SAND:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 8);
                if (h % 6 == 0) c = detail;
                else if (h % 14 == 0) c = (Color){220, 200, 130, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_WATER:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if ((y + (int)(sinf(x * 0.5f) * 1.5f)) % 6 == 0) c = detail;
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_COAL_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if ((x >= 3 && x <= 5 && y >= 3 && y <= 5) ||
                    (x >= 10 && x <= 12 && y >= 9 && y <= 11) ||
                    (x >= 6 && x <= 7 && y >= 10 && y <= 11)) {
                    c = base;
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_IRON_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if ((x >= 2 && x <= 4 && y >= 6 && y <= 8) ||
                    (x >= 11 && x <= 13 && y >= 3 && y <= 4) ||
                    (x >= 7 && x <= 8 && y >= 12 && y <= 14)) {
                    c = detail;
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_PLANKS:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if (y % 4 == 0) c = detail;
                if (x == 8) c = detail;
                if ((hash2D(x + worldX * 16, y + worldY * 16, 9) % 10) == 0) c = (Color){170, 130, 70, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_BRICK:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color mortar = (Color){190, 180, 170, 255};
                int row = y / 4;
                int offset = (row % 2) * 4;
                int bx = (x + offset) % 8;
                if (y % 4 == 0 || bx == 0) {
                    ImageDrawPixel(img, px + x, py + y, mortar);
                } else {
                    Color c = base;
                    if ((hash2D(x + worldX * 16, y + worldY * 16, 10) % 8) == 0) c = detail;
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        break;

    case BLOCK_GLASS:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                if (x == 0 || x == 15 || y == 0 || y == 15) {
                    ImageDrawPixel(img, px + x, py + y, (Color){200, 220, 255, 150});
                } else if ((x == 3 && y == 3) || (x == 4 && y == 3)) {
                    ImageDrawPixel(img, px + x, py + y, (Color){255, 255, 255, 180});
                } else {
                    ImageDrawPixel(img, px + x, py + y, base);
                }
            }
        break;

    case BLOCK_BEDROCK:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 11);
                if (h % 3 == 0) c = detail;
                else if (h % 5 == 0) c = (Color){65, 65, 65, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_GRAVEL:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 50);
                if (h % 5 == 0) c = detail;
                else if (h % 7 == 0) c = (Color){160, 150, 140, 255};
                else if (h % 11 == 0) c = (Color){120, 110, 100, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_CLAY:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 51);
                if (h % 8 == 0) c = detail;
                if (y % 4 == 0) c = detail;
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_SANDSTONE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 52);
                if (y == 0 || y == 8) c = detail;
                else if (h % 6 == 0) c = detail;
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_TORCH:
        // Stick
        for (int y = 4; y < 16; y++) ImageDrawPixel(img, px + 7, py + y, (Color){120, 80, 30, 255});
        for (int y = 4; y < 16; y++) ImageDrawPixel(img, px + 8, py + y, (Color){100, 65, 20, 255});
        // Flame
        for (int y = 0; y < 5; y++)
            for (int x = 6; x < 10; x++) {
                Color flame = (y < 2) ? (Color){255, 220, 50, 255} : (Color){255, 150, 20, 255};
                if (x == 6 && y > 2) continue;
                if (x == 9 && y > 2) continue;
                ImageDrawPixel(img, px + x, py + y, flame);
            }
        break;

    case BLOCK_FLOWER:
        // Stem
        for (int y = 8; y < 16; y++) ImageDrawPixel(img, px + 7, py + y, (Color){40, 120, 20, 255});
        for (int y = 8; y < 16; y++) ImageDrawPixel(img, px + 8, py + y, (Color){30, 100, 15, 255});
        // Petals
        for (int y = 3; y < 8; y++)
            for (int x = 5; x < 11; x++) {
                if ((x == 5 || x == 10) && y < 5) continue;
                if ((x == 6 || x == 9) && y < 4) continue;
                Color petal = ((x + y) % 2 == 0) ? base : detail;
                ImageDrawPixel(img, px + x, py + y, petal);
            }
        // Center
        ImageDrawPixel(img, px + 7, py + 5, (Color){255, 220, 50, 255});
        ImageDrawPixel(img, px + 8, py + 5, (Color){255, 220, 50, 255});
        break;

    case BLOCK_TALL_GRASS:
        for (int y = 4; y < 16; y++) {
            unsigned int h = hash2D(worldX * 16 + 3, y, 12);
            ImageDrawPixel(img, px + 3, py + y, (h % 2 == 0) ? base : detail);
        }
        for (int y = 2; y < 16; y++) {
            unsigned int h = hash2D(worldX * 16 + 7, y, 13);
            ImageDrawPixel(img, px + 7, py + y, (h % 2 == 0) ? base : detail);
        }
        for (int y = 5; y < 16; y++) {
            unsigned int h = hash2D(worldX * 16 + 12, y, 14);
            ImageDrawPixel(img, px + 12, py + y, (h % 2 == 0) ? base : detail);
        }
        break;

    case BLOCK_FURNACE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if (x == 0 || x == 15 || y == 0 || y == 15) c = detail;
                if (x >= 5 && x <= 10 && y >= 5 && y <= 10) c = (Color){40, 40, 40, 255};
                if (x >= 6 && x <= 9 && y >= 7 && y <= 9) c = (Color){80, 40, 10, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_BED:
        // Bed: red blanket on top, white pillow, wooden frame
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Blanket (top 2/3)
                if (y >= 0 && y <= 10) {
                    c = base;
                    if (y == 0 || y == 10) c = detail; // blanket edge
                }
                // Pillow (left side)
                if (x >= 1 && x <= 5 && y >= 1 && y <= 4) {
                    c = (Color){240, 240, 240, 255}; // white pillow
                }
                // Wooden frame (bottom)
                if (y >= 11 && y <= 15) {
                    c = (Color){140, 100, 50, 255}; // wood color
                    if (y == 11) c = (Color){120, 80, 40, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    // Item icons (atlas only)
    case ITEM_STICK:
        for (int y = 3; y < 14; y++) { ImageDrawPixel(img, px + 7, py + y, base); ImageDrawPixel(img, px + 8, py + y, detail); }
        break;
    case ITEM_COAL:
        for (int y = 5; y < 12; y++) for (int x = 5; x < 12; x++) ImageDrawPixel(img, px + x, py + y, base);
        for (int y = 6; y < 10; y++) for (int x = 7; x < 10; x++) ImageDrawPixel(img, px + x, py + y, detail);
        break;
    case ITEM_IRON_INGOT:
        for (int y = 6; y < 11; y++) for (int x = 4; x < 13; x++) ImageDrawPixel(img, px + x, py + y, base);
        for (int y = 7; y < 10; y++) for (int x = 5; x < 12; x++) ImageDrawPixel(img, px + x, py + y, detail);
        for (int x = 4; x < 13; x++) ImageDrawPixel(img, px + x, py + 6, (Color){240, 230, 220, 255});
        break;

    // Tool icons (atlas only, not used for world rendering)
    case TOOL_WOOD_PICKAXE:
    case TOOL_STONE_PICKAXE:
    case TOOL_IRON_PICKAXE: {
        Color tc = base; Color th = detail;
        // Handle
        for (int y = 8; y < 15; y++) { ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255}); ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255}); }
        // Head
        for (int x = 3; x < 13; x++) for (int y = 3; y < 7; y++) ImageDrawPixel(img, px + x, py + y, tc);
        for (int x = 2; x < 14; x++) ImageDrawPixel(img, px + x, py + 3, th);
        for (int x = 4; x < 12; x++) ImageDrawPixel(img, px + x, py + 7, th);
        break;
    }
    case TOOL_WOOD_AXE:
    case TOOL_STONE_AXE:
    case TOOL_IRON_AXE: {
        Color tc = base; Color th = detail;
        for (int y = 6; y < 15; y++) { ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255}); ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255}); }
        for (int x = 4; x < 9; x++) for (int y = 2; y < 8; y++) ImageDrawPixel(img, px + x, py + y, tc);
        for (int x = 3; x < 10; x++) ImageDrawPixel(img, px + x, py + 2, th);
        break;
    }
    case TOOL_WOOD_SWORD:
    case TOOL_STONE_SWORD:
    case TOOL_IRON_SWORD: {
        Color tc = base; Color th = detail;
        // Blade
        for (int y = 2; y < 12; y++) { ImageDrawPixel(img, px + 7, py + y, tc); ImageDrawPixel(img, px + 8, py + y, th); }
        ImageDrawPixel(img, px + 7, py + 1, th); ImageDrawPixel(img, px + 8, py + 1, th);
        // Guard
        for (int x = 5; x < 11; x++) ImageDrawPixel(img, px + x, py + 12, (Color){150, 130, 80, 255});
        // Handle
        for (int y = 13; y < 16; y++) { ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255}); ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255}); }
        break;
    }
    case TOOL_WOOD_SHOVEL:
    case TOOL_STONE_SHOVEL:
    case TOOL_IRON_SHOVEL: {
        Color tc = base; Color th = detail;
        // Handle
        for (int y = 2; y < 14; y++) { ImageDrawPixel(img, px + 7, py + y, (Color){120, 80, 30, 255}); ImageDrawPixel(img, px + 8, py + y, (Color){100, 65, 20, 255}); }
        // Blade (shovel head)
        for (int y = 0; y < 3; y++)
            for (int x = 5; x < 11; x++) {
                ImageDrawPixel(img, px + x, py + y, (y < 2) ? tc : th);
            }
        // Grip
        ImageDrawPixel(img, px + 7, py + 14, (Color){80, 55, 20, 255});
        ImageDrawPixel(img, px + 8, py + 14, (Color){80, 55, 20, 255});
        break;
    }
    // Food items
    case FOOD_RAW_PORK: {
        // Raw meat - pinkish chunk
        for (int y = 4; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                Color c = base;
                if (x == 4 || x == 11 || y == 4 || y == 11) c = detail;
                if ((hash2D(x, y, 42) % 5) == 0) c = (Color){220, 150, 150, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Bone
        ImageDrawPixel(img, px + 7, py + 3, (Color){230, 220, 200, 255});
        ImageDrawPixel(img, px + 8, py + 3, (Color){230, 220, 200, 255});
        break;
    }
    case FOOD_COOKED_PORK: {
        // Cooked meat - brown chunk
        for (int y = 4; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                Color c = base;
                if (x == 4 || x == 11 || y == 4 || y == 11) c = detail;
                if ((hash2D(x, y, 43) % 4) == 0) c = (Color){200, 120, 70, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case FOOD_APPLE: {
        // Apple - red circle with stem
        for (int y = 4; y < 13; y++)
            for (int x = 4; x < 12; x++) {
                float dx = x - 8.0f, dy = y - 8.5f;
                if (dx * dx + dy * dy < 16) {
                    Color c = base;
                    if (dx < -1) c = (Color){180, 40, 40, 255}; // shadow
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Stem
        ImageDrawPixel(img, px + 8, py + 3, (Color){100, 70, 30, 255});
        ImageDrawPixel(img, px + 9, py + 2, (Color){60, 120, 30, 255}); // leaf
        break;
    }
    case FOOD_BREAD: {
        // Bread - golden loaf
        for (int y = 5; y < 12; y++)
            for (int x = 3; x < 13; x++) {
                Color c = base;
                if (y == 5) c = (Color){230, 200, 120, 255}; // top crust
                if (y == 11) c = detail; // bottom
                if (x == 3 || x == 12) c = detail; // sides
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Score marks
        ImageDrawPixel(img, px + 5, py + 5, detail);
        ImageDrawPixel(img, px + 8, py + 5, detail);
        ImageDrawPixel(img, px + 11, py + 5, detail);
        break;
    }
    case BLOCK_CRAFTING_TABLE: {
        // Crafting table: wooden top with 3x3 grid pattern
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if (y <= 5) {
                    // Table top surface
                    c = base;
                    if (y == 0 || y == 5) c = detail;
                    // 3x3 grid lines on top
                    if (y >= 1 && y <= 4) {
                        if (x == 3 || x == 7 || x == 11) c = (Color){200, 170, 100, 255};
                        if (y == 2) c = (Color){200, 170, 100, 255};
                    }
                } else if (y <= 6) {
                    c = detail; // table edge
                } else {
                    // Legs
                    if ((x >= 2 && x <= 4) || (x >= 11 && x <= 13)) {
                        c = detail;
                    } else {
                        c = (Color){0, 0, 0, 0}; // transparent between legs
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_HELMET: case ARMOR_STONE_HELMET: case ARMOR_IRON_HELMET: {
        // Helmet: dome shape
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 3 && y <= 12) {
                    int cx = 8, cy = 7;
                    int dx = x - cx, dy = y - cy;
                    if (dx * dx + dy * dy <= 25) {
                        c = base;
                        if (y >= 9 && y <= 10 && x >= 5 && x <= 11) c = detail; // visor
                        if (y == 3) c = detail; // top edge
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_CHESTPLATE: case ARMOR_STONE_CHESTPLATE: case ARMOR_IRON_CHESTPLATE: {
        // Chestplate: torso with shoulder guards
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 2 && y <= 13) {
                    if (x >= 4 && x <= 11) {
                        c = base;
                        if (y == 2 || y == 13) c = detail;
                        if (x == 4 || x == 11) c = detail;
                        // Center line
                        if (x == 7 || x == 8) c = detail;
                    }
                    // Shoulder guards
                    if ((x >= 2 && x <= 4) || (x >= 11 && x <= 13)) {
                        if (y >= 2 && y <= 5) c = detail;
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_LEGGINGS: case ARMOR_STONE_LEGGINGS: case ARMOR_IRON_LEGGINGS: {
        // Leggings: two leg shapes
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 1 && y <= 14) {
                    // Left leg
                    if (x >= 3 && x <= 6) {
                        c = base;
                        if (x == 3 || x == 6) c = detail;
                        if (y == 1 || y == 14) c = detail;
                    }
                    // Right leg
                    if (x >= 9 && x <= 12) {
                        c = base;
                        if (x == 9 || x == 12) c = detail;
                        if (y == 1 || y == 14) c = detail;
                    }
                    // Waist band
                    if (y >= 1 && y <= 3 && x >= 3 && x <= 12) c = detail;
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_BOOTS: case ARMOR_STONE_BOOTS: case ARMOR_IRON_BOOTS: {
        // Boots: low boot shapes
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 6 && y <= 14) {
                    // Left boot
                    if (x >= 2 && x <= 6) {
                        c = base;
                        if (y == 6 || y == 14) c = detail;
                        if (x == 2 || x == 6) c = detail;
                        if (y >= 13 && x <= 3) c = detail; // toe
                    }
                    // Right boot
                    if (x >= 9 && x <= 13) {
                        c = base;
                        if (y == 6 || y == 14) c = detail;
                        if (x == 9 || x == 13) c = detail;
                        if (y >= 13 && x >= 12) c = detail; // toe
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    default:
        break;
    }
}

//----------------------------------------------------------------------------------
// Block Atlas
//----------------------------------------------------------------------------------
void GenerateBlockAtlas(void)
{
    Image atlas = GenImageColor(BLOCK_SIZE * BLOCK_COUNT, BLOCK_SIZE, BLANK);
    for (int i = 1; i < BLOCK_COUNT; i++) {
        DrawBlockPattern(&atlas, i * BLOCK_SIZE, 0, (BlockType)i, i * 31, i * 17);
    }
    blockAtlas = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
}

//----------------------------------------------------------------------------------
// World Generation
//----------------------------------------------------------------------------------
void GenerateWorld(unsigned int seed)
{
    memset(world, 0, sizeof(world));

    // ============================================================
    // Pass 1: Base terrain with biomes and varied landforms
    // ============================================================
    int centerX = WORLD_WIDTH / 2;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        // --- Continental base (gentle large-scale shape) ---
        float distFromCenter = fabsf((float)(x - centerX)) / (WORLD_WIDTH * 0.5f);
        float continental = 1.0f - distFromCenter * 0.3f;

        // --- Multi-scale terrain noise ---
        // Base rolling hills (large wavelength)
        float base = fbm(x * 0.003f, 0.0f, 4, 0.5f, seed);
        // Medium hills (mid wavelength, adds variety)
        float hills = fbm(x * 0.012f, 0.0f, 3, 0.5f, seed + 1000) * 0.4f;
        // Small detail bumps
        float detail = fbm(x * 0.04f, 0.0f, 2, 0.5f, seed + 1500) * 0.15f;

        // --- Mountain ridges ---
        float mountain = fbm(x * 0.004f, 0.0f, 3, 0.6f, seed + 2000);
        // Mountain threshold: areas where noise is high become mountains
        float mountainFactor = 0.0f;
        if (mountain > 0.35f) {
            mountainFactor = (mountain - 0.35f) * 1.54f; // 0 to ~1.0
            mountainFactor = mountainFactor * mountainFactor; // sharpen peaks
        }

        // --- River carving ---
        float river = fbm(x * 0.01f, 0.0f, 2, 0.5f, seed + 3000);
        float riverCut = 0.0f;
        if (river > 0.62f && river < 0.72f) {
            // Narrow river valley
            float t = (river - 0.62f) / 0.10f; // 0 to 1
            float edge = t < 0.5f ? t * 2.0f : (1.0f - t) * 2.0f;
            riverCut = edge * 18.0f;
        }

        // --- Biome ---
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0; // 0=plains, 1=desert, 2=forest
        if (biomeNoise > 0.55f) biome = 1;      // desert
        else if (biomeNoise < -0.25f) biome = 2; // forest

        // --- Calculate surface Y ---
        // Terrain is lower Y = higher on screen
        int surfaceY = TERRAIN_BASE;
        // Apply base noise (gentle continent shape)
        surfaceY += (int)(base * 30.0f * continental);
        // Apply hills (biome-dependent amplitude)
        float hillAmp = (biome == 1) ? 12.0f : (biome == 2) ? 28.0f : 22.0f;
        surfaceY += (int)(hills * hillAmp * continental);
        // Detail bumps
        surfaceY += (int)(detail * 10.0f * continental);
        // Mountains (big elevation gain)
        surfaceY -= (int)(mountainFactor * 55.0f * continental);
        // River carving (cuts into terrain)
        surfaceY += (int)(riverCut);

        // Force center area to be land
        if (distFromCenter < 0.15f) {
            int minSurfaceY = SEA_LEVEL - 8;
            if (surfaceY > minSurfaceY) surfaceY = minSurfaceY;
        }
        if (surfaceY < 8) surfaceY = 8;
        if (surfaceY >= WORLD_HEIGHT - 5) surfaceY = WORLD_HEIGHT - 6;

        // --- Determine surface block type ---
        // Stone peaks for tall mountains (surface above a threshold)
        bool isStonePeak = (surfaceY < TERRAIN_BASE - 30);

        // Fill column
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (y < surfaceY) {
                world[x][y] = BLOCK_AIR;
            } else if (y == surfaceY) {
                if (isStonePeak) world[x][y] = BLOCK_STONE;        // mountain peak
                else if (biome == 1) world[x][y] = BLOCK_SAND;     // desert
                else world[x][y] = BLOCK_GRASS;                     // plains/forest
            } else if (y < surfaceY + 4) {
                if (isStonePeak) world[x][y] = BLOCK_STONE;        // mountain subsurface
                else if (biome == 1) world[x][y] = BLOCK_SAND;     // desert sand layers
                else world[x][y] = BLOCK_DIRT;
            } else if (y < WORLD_HEIGHT - 1) {
                world[x][y] = BLOCK_STONE;
            } else {
                world[x][y] = BLOCK_BEDROCK;
            }
        }
    }

    // ============================================================
    // Pass 2: Caves (extended range, more variety)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = CAVE_START; y < CAVE_END; y++) {
            if (world[x][y] != BLOCK_STONE) continue;
            float c1 = fbm(x * 0.04f, y * 0.04f, 3, 0.5f, seed + 5000);
            float c2 = fbm(x * 0.08f, y * 0.08f, 2, 0.5f, seed + 7000);
            // Wider caves near y=180-220
            float depthBonus = (y > 180 && y < 220) ? 0.03f : 0.0f;
            if (c1 > (0.52f - depthBonus) && c2 > (0.45f - depthBonus)) {
                world[x][y] = BLOCK_AIR;
            }
        }
    }

    // ============================================================
    // Pass 3: Ravines (deep vertical cuts)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float ravine = fbm(x * 0.015f, 0.0f, 2, 0.5f, seed + 9000);
        if (ravine > 0.7f) {
            // This column is a ravine
            int ravineTop = SEA_LEVEL + 5 + (int)(fbm(x * 0.1f, 0.0f, 2, 0.5f, seed + 9100) * 30.0f);
            int ravineWidth = 1 + (hash2D(x, 0, seed + 9200) % 2);
            for (int dx = 0; dx < ravineWidth; dx++) {
                int rx = x + dx;
                if (rx >= WORLD_WIDTH) break;
                for (int y = ravineTop; y < CAVE_END - 5; y++) {
                    if (world[rx][y] == BLOCK_STONE) {
                        // Narrow ravine: 1-2 blocks wide
                        float narrow = fbm(rx * 0.1f, y * 0.05f, 2, 0.5f, seed + 9300);
                        if (narrow > 0.3f) {
                            world[rx][y] = BLOCK_AIR;
                        }
                    }
                }
            }
        }
    }

    // ============================================================
    // Pass 4: Ores (depth-based rarity)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = CAVE_START - 30; y < WORLD_HEIGHT - 1; y++) {
            if (world[x][y] != BLOCK_STONE) continue;
            // Coal: common, all depths
            float coal = fbm(x * 0.1f, y * 0.1f, 2, 0.5f, seed + 3000);
            if (coal > 0.72f) { world[x][y] = BLOCK_COAL_ORE; continue; }
            // Iron: deeper = more common
            if (y > 155) {
                float ironThreshold = 0.76f - (y - 155) * 0.0005f;
                float iron = fbm(x * 0.12f, y * 0.12f, 2, 0.5f, seed + 4000);
                if (iron > ironThreshold) { world[x][y] = BLOCK_IRON_ORE; continue; }
            }
        }
    }

    // ============================================================
    // Pass 5: Sand near sea level (for non-desert biomes)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        bool isDesert = biomeNoise > 0.55f;
        if (isDesert) continue; // desert already has sand
        for (int y = SEA_LEVEL - 3; y <= SEA_LEVEL + 2; y++) {
            if (y < 0 || y >= WORLD_HEIGHT) continue;
            if (world[x][y] == BLOCK_GRASS || world[x][y] == BLOCK_DIRT) {
                world[x][y] = BLOCK_SAND;
            }
        }
    }

    // ============================================================
    // Pass 6: Sandstone under sand (all layers), clay, gravel
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT - 1; y++) {
            // Sandstone: all dirt/sand blocks directly under sand become sandstone
            if (world[x][y] == BLOCK_SAND) {
                for (int dy = 1; dy <= 4 && y + dy < WORLD_HEIGHT; dy++) {
                    if (world[x][y + dy] == BLOCK_DIRT) {
                        world[x][y + dy] = BLOCK_SANDSTONE;
                    } else if (world[x][y + dy] != BLOCK_SAND) {
                        break;
                    }
                }
            }
            // Clay near water level
            if (world[x][y] == BLOCK_DIRT && y >= SEA_LEVEL - 1 && y <= SEA_LEVEL + 1) {
                if (hash2D(x, y, 55) % 3 == 0) {
                    world[x][y] = BLOCK_CLAY;
                }
            }
        }
    }

    // ============================================================
    // Pass 7: Gravel in caves and underwater
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = CAVE_START; y < CAVE_END; y++) {
            if (world[x][y] != BLOCK_AIR) continue;
            // Gravel on cave floors: air with solid below
            if (y + 1 < WORLD_HEIGHT && IsBlockSolid(x, y + 1)) {
                if (hash2D(x, y, seed + 7500) % 5 == 0) {
                    world[x][y] = BLOCK_GRAVEL;
                }
            }
        }
        // Gravel patches underwater (below sea level, in sand/dirt areas)
        for (int y = SEA_LEVEL; y < SEA_LEVEL + 10 && y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_SAND || world[x][y] == BLOCK_DIRT) {
                if (hash2D(x, y, seed + 7600) % 7 == 0) {
                    world[x][y] = BLOCK_GRAVEL;
                }
            }
        }
    }

    // ============================================================
    // Pass 8: Underground water pockets
    // ============================================================
    for (int x = 2; x < WORLD_WIDTH - 2; x++) {
        for (int y = CAVE_START + 10; y < CAVE_END - 10; y++) {
            if (world[x][y] != BLOCK_AIR) continue;
            // Check for a small hollow: 3x2 air pocket
            bool hollow = true;
            for (int dx = -1; dx <= 1 && hollow; dx++) {
                for (int dy = 0; dy <= 1 && hollow; dy++) {
                    if (world[x + dx][y + dy] != BLOCK_AIR) hollow = false;
                }
            }
            // Must have solid floor
            if (hollow && y + 2 < WORLD_HEIGHT && IsBlockSolid(x, y + 2)) {
                float waterNoise = fbm(x * 0.1f, y * 0.1f, 2, 0.5f, seed + 6500);
                if (waterNoise > 0.65f) {
                    // Fill the bottom row of the pocket with water
                    for (int dx = -1; dx <= 1; dx++) {
                        if (world[x + dx][y + 1] == BLOCK_AIR) {
                            world[x + dx][y + 1] = BLOCK_WATER;
                        }
                    }
                }
            }
        }
    }

    // ============================================================
    // Pass 9: Water fill (sky-connected only)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (IsBlockSolid(x, y)) break;
            if (world[x][y] == BLOCK_AIR && y >= SEA_LEVEL) {
                world[x][y] = BLOCK_WATER;
            }
        }
    }

    // ============================================================
    // Pass 10: Trees (biome-aware density)
    // ============================================================
    for (int x = 5; x < WORLD_WIDTH - 5; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        bool isDesert = biomeNoise > 0.55f;
        bool isForest = biomeNoise < -0.25f;

        // Desert: no trees. Forest: denser trees.
        int treeChance = isDesert ? 999 : (isForest ? 6 : 12);
        if (hash2D(x, 0, seed + 999) % treeChance != 0) continue;

        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_GRASS) { surfaceY = y; break; }
        }
        if (surfaceY < 0 || surfaceY >= SEA_LEVEL) continue;

        int trunkH = isForest ? (5 + (hash2D(x, 1, seed + 888) % 4)) : (4 + (hash2D(x, 1, seed + 888) % 3));
        for (int i = 1; i <= trunkH && surfaceY - i >= 0; i++) {
            world[x][surfaceY - i] = BLOCK_WOOD;
        }

        int canopyTop = surfaceY - trunkH;
        for (int dy = -2; dy <= 0; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int bx = x + dx;
                int by = canopyTop + dy;
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                        world[bx][by] = BLOCK_LEAVES;
                    }
                }
            }
        }
        for (int dx = -1; dx <= 1; dx++) {
            int bx = x + dx;
            int by = canopyTop - 1;
            if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                if (world[bx][by] == BLOCK_AIR) world[bx][by] = BLOCK_LEAVES;
            }
        }
    }

    // ============================================================
    // Pass 11: Flowers, tall grass, cacti (biome-aware)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        bool isDesert = biomeNoise > 0.55f;
        bool isForest = biomeNoise < -0.25f;

        for (int y = 1; y < WORLD_HEIGHT - 1; y++) {
            if (world[x][y] != BLOCK_GRASS && world[x][y] != BLOCK_SAND) continue;
            if (world[x][y - 1] != BLOCK_AIR) continue;

            unsigned int h = hash2D(x, y, seed + 6000);
            if (isDesert) {
                // Desert: rare cacti (tall grass as placeholder)
                if (h % 30 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
            } else if (isForest) {
                // Forest: more flowers and grass
                if (h % 12 == 0) world[x][y - 1] = BLOCK_FLOWER;
                else if (h % 4 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
            } else {
                // Plains
                if (h % 20 == 0) world[x][y - 1] = BLOCK_FLOWER;
                else if (h % 8 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Chunk System - Hash table with linear probing, O(1) lookup
//----------------------------------------------------------------------------------
static void InitChunkTable(void)
{
    for (int i = 0; i < MAX_CHUNKS; i++) {
        loadedChunks[i].chunkX = CHUNK_EMPTY;
        loadedChunks[i].textureValid = false;
    }
}

Chunk* GetChunk(int chunkX)
{
    int idx = ((chunkX % MAX_CHUNKS) + MAX_CHUNKS) % MAX_CHUNKS;
    for (int i = 0; i < MAX_CHUNKS; i++) {
        int slot = (idx + i) % MAX_CHUNKS;
        if (loadedChunks[slot].chunkX == CHUNK_EMPTY) return NULL;
        if (loadedChunks[slot].chunkX == chunkX) return &loadedChunks[slot];
    }
    return NULL;
}

void UnloadChunk(int chunkX)
{
    Chunk *c = GetChunk(chunkX);
    if (!c) return;
    if (c->textureValid) UnloadTexture(c->texture);
    c->chunkX = CHUNK_EMPTY;
    c->textureValid = false;

    // Rehash entries that might have been displaced by this slot
    int idx = ((chunkX % MAX_CHUNKS) + MAX_CHUNKS) % MAX_CHUNKS;
    int slot = (idx + 1) % MAX_CHUNKS;
    for (int i = 1; i < MAX_CHUNKS && loadedChunks[slot].chunkX != CHUNK_EMPTY; i++) {
        int cx = loadedChunks[slot].chunkX;
        int ideal = ((cx % MAX_CHUNKS) + MAX_CHUNKS) % MAX_CHUNKS;
        // Check if this entry is displaced past the deleted slot
        bool displaced = false;
        if (ideal <= idx) {
            displaced = (slot > idx) || (slot < ideal);
        } else {
            displaced = (slot > idx) && (slot < ideal);
        }
        if (displaced) {
            // Move this entry to fill the gap
            *c = loadedChunks[slot];
            loadedChunks[slot].chunkX = CHUNK_EMPTY;
            loadedChunks[slot].textureValid = false;
            c = &loadedChunks[slot];
        }
        slot = (slot + 1) % MAX_CHUNKS;
    }
}

static Chunk* InsertChunk(int chunkX)
{
    int idx = ((chunkX % MAX_CHUNKS) + MAX_CHUNKS) % MAX_CHUNKS;
    for (int i = 0; i < MAX_CHUNKS; i++) {
        int slot = (idx + i) % MAX_CHUNKS;
        if (loadedChunks[slot].chunkX == CHUNK_EMPTY || loadedChunks[slot].chunkX == chunkX) {
            loadedChunks[slot].chunkX = chunkX;
            loadedChunks[slot].textureValid = false;
            for (int w = 0; w < CHUNK_SIZE; w++) loadedChunks[slot].waterTopY[w] = -1;
            return &loadedChunks[slot];
        }
    }
    return NULL;
}

static void BuildWaterCache(Chunk *chunk)
{
    int startX = chunk->chunkX * CHUNK_SIZE;
    for (int bx = 0; bx < CHUNK_SIZE; bx++) {
        int wx = startX + bx;
        chunk->waterTopY[bx] = -1;
        if (wx < 0 || wx >= WORLD_WIDTH) continue;
        for (int by = 0; by < WORLD_HEIGHT; by++) {
            if (world[wx][by] == BLOCK_WATER) {
                chunk->waterTopY[bx] = by;
                break;
            }
        }
    }
}

void GenerateChunkTexture(Chunk *chunk)
{
    int imgW = CHUNK_SIZE * BLOCK_SIZE;
    int imgH = WORLD_HEIGHT * BLOCK_SIZE;
    Image img = GenImageColor(imgW, imgH, BLANK);

    int startX = chunk->chunkX * CHUNK_SIZE;
    for (int bx = 0; bx < CHUNK_SIZE; bx++) {
        int wx = startX + bx;
        if (wx < 0 || wx >= WORLD_WIDTH) continue;
        for (int by = 0; by < WORLD_HEIGHT; by++) {
            BlockType bt = (BlockType)world[wx][by];
            if (bt == BLOCK_AIR || bt == BLOCK_WATER) continue;
            DrawBlockPattern(&img, bx * BLOCK_SIZE, by * BLOCK_SIZE, bt, wx, by);
        }
    }

    if (chunk->textureValid) UnloadTexture(chunk->texture);
    chunk->texture = LoadTextureFromImage(img);
    UnloadImage(img);
    chunk->textureValid = true;

    BuildWaterCache(chunk);
}

void InvalidateChunkAt(int worldBlockX, int worldBlockY)
{
    int cx = worldBlockX / CHUNK_SIZE;
    Chunk *c = GetChunk(cx);
    if (c) {
        c->textureValid = false;
        BuildWaterCache(c);
    }
}

void UpdateChunks(void)
{
    // Initialize chunk table on first call
    static bool initialized = false;
    if (!initialized) {
        InitChunkTable();
        initialized = true;
    }

    int playerBlockX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerChunkX = playerBlockX / CHUNK_SIZE;

    // Unload distant chunks
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (abs(loadedChunks[i].chunkX - playerChunkX) > CHUNKS_LOADED) {
            UnloadChunk(loadedChunks[i].chunkX);
        }
    }

    // Load nearby chunks
    int minCX = playerChunkX - CHUNKS_LOADED;
    int maxCX = playerChunkX + CHUNKS_LOADED;
    if (minCX < 0) minCX = 0;
    if (maxCX >= WORLD_WIDTH / CHUNK_SIZE) maxCX = WORLD_WIDTH / CHUNK_SIZE - 1;

    for (int cx = minCX; cx <= maxCX; cx++) {
        Chunk *c = GetChunk(cx);
        if (!c) {
            c = InsertChunk(cx);
            if (!c) continue;
        }
        if (!c->textureValid) {
            GenerateChunkTexture(c);
        }
    }
}

bool IsBlockSolid(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return true;
    return blockInfo[world[bx][by]].solid;
}
