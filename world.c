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

    // Terrain with continental shelf - center guaranteed land
    int centerX = WORLD_WIDTH / 2;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float noise1 = fbm(x * 0.005f, 0.0f, 4, 0.5f, seed);
        float noise2 = fbm(x * 0.02f, 0.0f, 2, 0.5f, seed + 1000) * 0.3f;

        // Continental factor: center is higher land, edges can be ocean
        float distFromCenter = fabsf((float)(x - centerX)) / (WORLD_WIDTH * 0.5f);
        float continental = 1.0f - distFromCenter * 0.4f; // 1.0 at center, 0.6 at edges

        int surfaceY = TERRAIN_BASE + (int)((noise1 + noise2) * TERRAIN_AMPLITUDE * continental);
        // Force center area to be land (above sea level)
        if (distFromCenter < 0.2f) {
            int minSurfaceY = SEA_LEVEL - 5;
            if (surfaceY > minSurfaceY) surfaceY = minSurfaceY;
        }
        if (surfaceY < 20) surfaceY = 20;
        if (surfaceY >= WORLD_HEIGHT - 5) surfaceY = WORLD_HEIGHT - 6;

        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (y < surfaceY) {
                world[x][y] = BLOCK_AIR;
            } else if (y == surfaceY) {
                world[x][y] = BLOCK_GRASS;
            } else if (y < surfaceY + 4) {
                world[x][y] = BLOCK_DIRT;
            } else if (y < WORLD_HEIGHT - 1) {
                world[x][y] = BLOCK_STONE;
            } else {
                world[x][y] = BLOCK_BEDROCK;
            }
        }
    }

    // Caves
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = CAVE_START; y < CAVE_END; y++) {
            if (world[x][y] != BLOCK_STONE) continue;
            float c1 = fbm(x * 0.04f, y * 0.04f, 3, 0.5f, seed + 5000);
            float c2 = fbm(x * 0.08f, y * 0.08f, 2, 0.5f, seed + 7000);
            if (c1 > 0.52f && c2 > 0.45f) {
                world[x][y] = BLOCK_AIR;
            }
        }
    }

    // Ores
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] != BLOCK_STONE) continue;
            float coal = fbm(x * 0.1f, y * 0.1f, 2, 0.5f, seed + 3000);
            if (coal > 0.72f) world[x][y] = BLOCK_COAL_ORE;
            if (y > 160) {
                float iron = fbm(x * 0.12f, y * 0.12f, 2, 0.5f, seed + 4000);
                if (iron > 0.74f) world[x][y] = BLOCK_IRON_ORE;
            }
        }
    }

    // Sand near sea level
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_GRASS || world[x][y] == BLOCK_DIRT) {
                if (y >= SEA_LEVEL - 3 && y <= SEA_LEVEL + 2) {
                    world[x][y] = BLOCK_SAND;
                }
            }
        }
    }

    // Sandstone under sand, clay near water, gravel underwater
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_SAND && y + 1 < WORLD_HEIGHT && world[x][y + 1] == BLOCK_DIRT) {
                world[x][y + 1] = BLOCK_SANDSTONE;
            }
            if (world[x][y] == BLOCK_DIRT && y >= SEA_LEVEL - 1 && y <= SEA_LEVEL + 1) {
                if (hash2D(x, y, 55) % 3 == 0) {
                    world[x][y] = BLOCK_CLAY;
                }
            }
        }
    }

    // Water
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_AIR && y >= SEA_LEVEL) {
                world[x][y] = BLOCK_WATER;
            }
        }
    }

    // Trees
    for (int x = 5; x < WORLD_WIDTH - 5; x++) {
        if (hash2D(x, 0, seed + 999) % 12 != 0) continue;

        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_GRASS) { surfaceY = y; break; }
        }
        if (surfaceY < 0 || surfaceY >= SEA_LEVEL) continue;

        int trunkH = 4 + (hash2D(x, 1, seed + 888) % 3);
        for (int i = 1; i <= trunkH && surfaceY - i >= 0; i++) {
            world[x][surfaceY - i] = BLOCK_WOOD;
        }

        int canopyTop = surfaceY - trunkH;
        for (int dy = -2; dy <= 0; dy++) {
            for (int dx = -2; dx <= 2; dx++) {
                int bx = x + dx;
                int by = canopyTop + dy;
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    if (world[bx][by] == BLOCK_AIR) {
                        if (!(dx == 0 && dy == 0)) {
                            world[bx][by] = BLOCK_LEAVES;
                        }
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

    // Flowers and tall grass on grass blocks
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT - 1; y++) {
            if (world[x][y] == BLOCK_GRASS && world[x][y - 1] == BLOCK_AIR) {
                unsigned int h = hash2D(x, y, seed + 6000);
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
