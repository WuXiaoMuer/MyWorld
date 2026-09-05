#include "types.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

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
    {"Wood Hoe",    {180,140,80,255},   {140,100,50,255},   false, false, false},
    {"Stone Pick",  {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Axe",   {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Sword", {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Shovel",{128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Stone Hoe",   {128,128,128,255},  {100,100,100,255},  false, false, false},
    {"Iron Pick",   {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Axe",    {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Sword",  {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Shovel", {200,180,160,255},  {170,150,130,255},  false, false, false},
    {"Iron Hoe",    {200,180,160,255},  {170,150,130,255},  false, false, false},
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
    // New ores
    {"Gold Ore",      {130,110,100,255}, {220,180,50,255},  true,  false, true},
    {"Diamond Ore",   {130,110,100,255}, {80,220,230,255},  true,  false, true},
    {"Redstone Ore",  {130,110,100,255}, {200,30,30,255},   true,  false, true},
    {"Lapis Ore",     {130,110,100,255}, {30,50,200,255},   true,  false, true},
    // New items
    {"Gold Ingot",    {220,180,50,255},  {180,140,30,255},  false, false, false},
    {"Diamond",       {80,220,230,255},  {50,180,200,255},  false, false, false},
    {"Redstone",      {200,30,30,255},   {150,20,20,255},   false, false, false},
    {"Lapis Lazuli",  {30,50,200,255},   {20,35,150,255},   false, false, false},
    // Interactive blocks
    {"Chest",       {140,100,50,255},  {100,70,30,255},   true,  false, true},
    // Gold tools
    {"Gold Pick",   {220,180,50,255},  {180,140,30,255},  false, false, false},
    {"Gold Axe",    {220,180,50,255},  {180,140,30,255},  false, false, false},
    {"Gold Sword",  {220,180,50,255},  {180,140,30,255},  false, false, false},
    {"Gold Shovel", {220,180,50,255},  {180,140,30,255},  false, false, false},
    {"Gold Hoe",    {220,180,50,255},  {180,140,30,255},  false, false, false},
    // Diamond tools
    {"Diamond Pick",   {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Axe",    {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Sword",  {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Shovel", {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Hoe",    {80,220,230,255}, {50,180,200,255}, false, false, false},
    // Gold armor
    {"Gold Helmet",    {220,180,50,255},  {180,140,30,255}, false, false, false},
    {"Gold Chest",     {220,180,50,255},  {180,140,30,255}, false, false, false},
    {"Gold Leggings",  {220,180,50,255},  {180,140,30,255}, false, false, false},
    {"Gold Boots",     {220,180,50,255},  {180,140,30,255}, false, false, false},
    // Diamond armor
    {"Diamond Helmet",    {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Chest",     {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Leggings",  {80,220,230,255}, {50,180,200,255}, false, false, false},
    {"Diamond Boots",     {80,220,230,255}, {50,180,200,255}, false, false, false},
    // Mob drops
    {"Gunpowder",    {80,80,80,255},   {50,50,50,255},    false, false, false},
    {"String",       {200,200,200,255},{160,160,160,255},  false, false, false},
    // New items
    {"Bone",         {230,220,200,255},{200,190,170,255},  false, false, false},
    {"Arrow",        {180,140,80,255}, {120,120,120,255},  false, false, false},
    {"Bow",          {140,100,50,255}, {100,70,30,255},    false, false, false},
    // New blocks
    {"Mossy Cobble", {90,110,80,255},  {70,90,60,255},    true,  false, true},
    {"Bookshelf",    {140,110,60,255}, {100,80,40,255},   true,  false, true},
    {"Lantern",      {200,180,140,255},{255,220,100,255}, false, true,  true},
    {"Bone Block",   {230,220,200,255},{200,190,170,255}, true,  false, true},
    // Phase 1: Biome blocks
    {"Snow",         {240,245,255,255},{220,225,240,255}, true,  false, true},
    {"Ice",          {180,210,240,255},{150,185,220,255}, true,  true,  true},
    {"Packed Ice",   {160,195,230,255},{130,170,210,255}, true,  false, true},
    {"Mud",          {100,80,60,255},  {80,60,45,255},    true,  false, true},
    {"Moss Block",   {80,120,60,255},  {60,100,45,255},   true,  false, true},
    {"Jungle Wood",  {110,80,40,255},  {85,60,30,255},    true,  false, true},
    {"Jungle Leaves",{30,140,20,255},  {20,110,15,255},   true,  true,  true},
    {"Vine",         {40,130,25,255},  {30,100,18,255},   false, true,  true},
    {"Pumpkin",      {220,150,30,255}, {180,100,20,255},  true,  false, true},
    {"Melon",        {100,180,60,255}, {70,140,40,255},   true,  false, true},
    {"Snowy Grass",  {220,230,240,255},{200,210,220,255}, true,  false, true},
    {"Coarse Dirt",  {120,85,55,255},  {100,70,45,255},   true,  false, true},
    {"Podzol",       {110,80,45,255},  {85,60,35,255},    true,  false, true},
    // Phase 1: New items
    {"Slimeball",    {120,200,80,255}, {90,170,60,255},   false, false, false},
    {"Ender Pearl",  {20,20,30,255},   {120,80,200,255},  false, false, false},
    // Farming
    {"Wheat Seeds",  {180,160,80,255}, {150,130,60,255},  false, false, false},
    {"Wheat",        {200,180,80,255}, {170,150,60,255},  false, false, false},
    {"Farmland",     {120,85,55,255},  {100,70,45,255},   true,  false, true},
    {"Crops",        {80,180,40,255},  {60,140,30,255},   false, true,  true},
    {"Hay Bale",     {180,160,60,255}, {150,130,40,255},  true,  false, true},
    // Animal drops
    {"Raw Beef",     {180,60,60,255},  {140,40,40,255},   false, false, false},
    {"Leather",      {160,100,60,255}, {130,80,40,255},   false, false, false},
    {"Raw Mutton",   {180,80,80,255},  {150,60,60,255},   false, false, false},
    {"Wool",         {220,220,220,255},{200,200,200,255}, false, false, false},
    {"Raw Chicken",  {200,150,130,255},{170,120,100,255}, false, false, false},
    {"Feather",      {230,230,230,255},{210,210,210,255}, false, false, false},
    {"Egg",          {230,220,200,255},{210,200,180,255}, false, false, false},
    {"Cooked Beef",  {140,80,40,255},  {110,60,30,255},   false, false, false},
    {"Cooked Mutton",{140,70,50,255},  {110,50,40,255},   false, false, false},
    {"Cooked Chkn",  {160,120,80,255}, {130,90,60,255},   false, false, false},
    // Utility items
    {"Bucket",       {160,160,160,255},{120,120,120,255}, false, false, false},
    {"Water Bucket", {40,100,200,255}, {30,80,180,255},   false, false, false},
    // Redstone blocks
    {"Lever",            {100,100,100,255}, {60,60,60,255},     false, true,  true},
    {"Redstone Wire",    {200,30,30,255},   {150,20,20,255},    false, true,  true},
    {"Redstone Lamp",    {220,180,60,255},  {180,140,40,255},   true,  false, true},
    {"Pressure Plate",   {130,130,130,255}, {100,100,100,255},  false, true,  true},
    // Lava system
    {"Lava",          {200,80,20,200},  {240,120,30,200},  false, true,  false},
    {"Obsidian",      {30,20,40,255},   {60,40,80,255},    true,  false, true},
    {"Lava Bucket",   {200,80,20,255},  {240,120,30,255},  false, false, false},
    // Enchanting
    {"Enchant Table", {80,50,120,255},  {120,80,180,255},  true,  false, true},
    // Fishing
    {"Fishing Rod",      {150,140,100,255}, {120,110,80,255},  false, false, false},
    {"Raw Fish",         {160,180,200,255}, {130,150,170,255}, false, false, false},
    {"Cooked Fish",      {180,120,80,255},  {150,100,60,255},  false, false, false},
    // Cauldron
    {"Cauldron",               {100,95,110,255},  {80,75,90,255},    true,  false, true},
    // Decorative blocks
    {"Oak Stairs",             {140,110,70,255},  {120,90,50,255},   true,  false, true},
    {"Cobblestone Stairs",     {100,100,100,255}, {80,80,80,255},    true,  false, true},
    {"Stone Bricks",           {115,115,120,255}, {95,95,100,255},   true,  false, true},
    {"Chiseled Stone Bricks",  {115,115,120,255}, {130,110,70,255},  true,  false, true},
    {"Oak Slab",               {140,110,70,255},  {120,90,50,255},   true,  false, true},
    {"Cobblestone Slab",       {100,100,100,255}, {80,80,80,255},    true,  false, true},
    // New plants & items
    {"Cactus",                 {50,180,50,255},   {30,140,30,255},   true,  false, true},
    {"Sugar Cane",             {80,200,50,255},   {60,160,35,255},   false, true,  true},
    {"Paper",                  {230,225,210,255}, {200,195,180,255},  false, false, false},
    {"Book",                   {180,120,60,255},  {150,100,50,255},  false, false, false},
    {"Sugar",                  {240,235,230,255}, {200,195,190,255},  false, false, false},
    {"TNT",                    {200,50,40,255},   {235,90,70,255},   true,  false, true},
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
        // Dirt base with pebble details
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 1);
                Color c = detail;
                if (h % 7 == 0) c = (Color){100, 70, 45, 255};
                else if (h % 11 == 0) c = (Color){150, 110, 80, 255};
                else if (h % 19 == 0) c = (Color){90, 60, 35, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Grass top with multiple green shades and blade details
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 16; x++) {
                int wave = (int)(sinf(x * 0.7f + varSeed * 0.1f) * 2.0f);
                int grassEdge = 3 + wave;
                if (y < grassEdge) {
                    unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 2);
                    Color c = base;
                    if (h % 5 == 0) c = (Color){60, 140, 0, 255};
                    else if (h % 8 == 0) c = (Color){85, 165, 10, 255};
                    else if (h % 13 == 0) c = (Color){50, 120, 0, 255};
                    // Grass blade tips
                    if (y == grassEdge - 1 && h % 4 == 0) c = (Color){90, 170, 20, 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                } else if (y == grassEdge) {
                    // Transition: mix green and brown
                    unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 3);
                    Color c = (h % 2 == 0) ? (Color){80, 110, 30, 255} : detail;
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        break;

    case BLOCK_DIRT:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 3);
                Color c = base;
                if (h % 7 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){150, 110, 80, 255};
                else if (h % 19 == 0) c = (Color){100, 65, 40, 255};
                // Small pebble/rock details
                if ((x == 4 && y == 5) || (x == 11 && y == 10) || (x == 7 && y == 13))
                    c = (Color){120, 90, 60, 255};
                // Shadow at bottom
                if (y > 13) c = (Color){(unsigned char)(base.r - 12), (unsigned char)(base.g - 10), (unsigned char)(base.b - 8), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_STONE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                Color c = base;
                // Natural color variation
                if (h % 13 == 0) c = (Color){140, 140, 140, 255};
                else if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Crack lines (horizontal and diagonal)
                if (y == 4 || y == 11) { c = detail; }
                if ((x + y) % 9 == 0 && h % 3 == 0) c = detail;
                // Shadow at bottom for depth
                if (y > 13) c = (Color){(unsigned char)(base.r - 15), (unsigned char)(base.g - 15), (unsigned char)(base.b - 15), 255};
                // Highlight at top
                if (y < 2 && h % 4 == 0) c = (Color){145, 145, 145, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_COBBLESTONE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 5);
                Color c = base;
                // Mortar lines between stones
                if ((x % 8 < 1) || (y % 8 < 1)) { c = detail; }
                else {
                    if (h % 6 == 0) c = (Color){115, 115, 115, 255};
                    // Stone surface variation
                    else if (h % 11 == 0) c = (Color){90, 90, 90, 255};
                    // Highlight on top-left of each stone
                    int lx = x % 8, ly = y % 8;
                    if (lx == 1 && ly > 0 && ly < 4) c = (Color){120, 120, 120, 255};
                    // Shadow on bottom-right
                    if (lx == 7 && ly > 4) c = (Color){80, 80, 80, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_WOOD:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 6);
                Color c = base;
                // Bark vertical lines
                if (x == 4 || x == 12) c = detail;
                // Horizontal grain lines
                if (y % 5 == 0) c = detail;
                // Bark texture variation
                if (h % 9 == 0) c = (Color){88, 58, 28, 255};
                else if (h % 14 == 0) c = (Color){115, 80, 40, 255};
                // Bark knot detail
                if ((x == 8 && y == 8) || (x == 9 && y == 8) || (x == 8 && y == 9))
                    c = (Color){80, 50, 22, 255};
                // Shadow between bark grooves
                if (x == 5 || x == 13) c = (Color){(unsigned char)(base.r - 15), (unsigned char)(base.g - 12), (unsigned char)(base.b - 8), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_LEAVES:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 7);
                Color c = base;
                // Multiple green shades for depth
                if (h % 5 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){45, 140, 20, 255};
                else if (h % 13 == 0) c = (Color){30, 100, 10, 255};
                // Light spots (sun hitting leaves)
                if (h % 23 == 0) c = (Color){60, 150, 30, 255};
                // Small gaps/holes
                if (h % 17 == 0) c = (Color){0, 0, 0, 0};
                // Shadow at bottom for depth
                if (y > 12 && h % 3 == 0) c = (Color){(unsigned char)(base.r - 12), (unsigned char)(base.g - 10), (unsigned char)(base.b - 5), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_SAND:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 8);
                Color c = base;
                if (h % 6 == 0) c = detail;
                else if (h % 14 == 0) c = (Color){220, 200, 130, 255};
                else if (h % 19 == 0) c = (Color){200, 180, 110, 255};
                // Grain pattern: subtle ripple effect
                if ((x + y * 3) % 7 == 0) c = (Color){(unsigned char)(base.r - 8), (unsigned char)(base.g - 6), (unsigned char)(base.b - 4), 255};
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

    case BLOCK_LAVA:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 77);
                if (h % 5 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){255, 200, 50, 220};
                // Bright hot spots
                if ((x + y) % 8 == 0 && h % 3 == 0) c = (Color){255, 180, 60, 240};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_OBSIDIAN:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 99);
                if (h % 7 == 0) c = detail;
                else if (h % 13 == 0) c = (Color){50, 30, 70, 255};
                // Purple sheen
                if ((x == 4 && y == 4) || (x == 11 && y == 10)) c = (Color){80, 50, 120, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_LAVA_BUCKET:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 4 && y <= 14 && x >= 4 && x <= 11) {
                    c = base;
                    if (x == 4 || x == 11) c = detail;
                    if (y == 4 || y == 14) c = detail;
                    if (y >= 2 && y <= 5 && x >= 6 && x <= 9) c = detail;
                    if (y == 2 && (x == 6 || x == 9)) c = (Color){0, 0, 0, 0};
                    // Lava inside
                    if (y >= 6 && y <= 12 && x >= 5 && x <= 10) {
                        c = (Color){220, 100, 20, 255};
                        if (y == 6) c = (Color){240, 150, 40, 255};
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_ENCHANTING_TABLE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 55);
                // Stone base
                if (y >= 10) {
                    c = (Color){80, 80, 80, 255};
                    if (h % 5 == 0) c = (Color){60, 60, 60, 255};
                }
                // Purple book on top
                if (y >= 3 && y <= 9 && x >= 3 && x <= 12) {
                    c = detail;
                    if (x == 7 || x == 8) c = (Color){60, 30, 90, 255}; // spine
                    // Glowing runes
                    if ((x + y) % 4 == 0 && h % 3 == 0) c = (Color){180, 120, 255, 255};
                }
                // Gold corner accents
                if ((y == 10 || y == 11) && (x == 2 || x == 13)) c = (Color){220, 180, 60, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_COAL_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Coal deposits with depth
                if ((x >= 3 && x <= 5 && y >= 3 && y <= 5) ||
                    (x >= 10 && x <= 12 && y >= 9 && y <= 11) ||
                    (x >= 6 && x <= 7 && y >= 10 && y <= 11)) {
                    c = base;
                    // Highlight on top-left of deposit
                    if ((x == 3 && y == 3) || (x == 10 && y == 9))
                        c = (Color){90, 90, 90, 255};
                    // Shadow on bottom-right
                    if ((x == 5 && y == 5) || (x == 12 && y == 11))
                        c = (Color){30, 30, 30, 255};
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
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Iron ore deposits with metallic sheen
                if ((x >= 2 && x <= 4 && y >= 6 && y <= 8) ||
                    (x >= 11 && x <= 13 && y >= 3 && y <= 4) ||
                    (x >= 7 && x <= 8 && y >= 12 && y <= 14)) {
                    c = detail;
                    // Metallic highlight
                    if ((x == 2 && y == 6) || (x == 11 && y == 3) || (x == 7 && y == 12))
                        c = (Color){230, 210, 190, 255};
                    // Shadow
                    if ((x == 4 && y == 8) || (x == 13 && y == 4) || (x == 8 && y == 14))
                        c = (Color){100, 85, 75, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_PLANKS:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Plank dividers
                if (y % 4 == 0) c = detail;
                if (x == 8) c = detail;
                // Wood grain
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 9);
                if (h % 10 == 0) c = (Color){170, 130, 70, 255};
                // Knots
                if ((x == 4 && y == 2) || (x == 12 && y == 10)) c = (Color){130, 95, 45, 255};
                // Highlights on top edge of each plank
                if (y % 4 == 1 && h % 3 == 0) c = (Color){195, 155, 90, 255};
                // Shadow on bottom edge of each plank
                if (y % 4 == 3 && h % 4 == 0) c = (Color){(unsigned char)(base.r*0.85f), (unsigned char)(base.g*0.85f), (unsigned char)(base.b*0.85f), 255};
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
                    // Frame with slight bevel
                    Color frame = {190, 210, 245, 150};
                    if (x == 0 || y == 0) frame = (Color){210, 230, 255, 160}; // light edge
                    if (x == 15 || y == 15) frame = (Color){170, 190, 225, 140}; // dark edge
                    ImageDrawPixel(img, px + x, py + y, frame);
                } else {
                    // Glass surface with subtle reflections
                    Color c = base;
                    // Diagonal reflection streak
                    if (x == y && x >= 2 && x <= 6) c = (Color){220, 235, 255, 120};
                    // Corner shine
                    if ((x == 2 && y == 2) || (x == 3 && y == 2)) c = (Color){255, 255, 255, 160};
                    // Bottom-right subtle shadow
                    if (x >= 12 && y >= 12) c = (Color){180, 200, 235, 90};
                    ImageDrawPixel(img, px + x, py + y, c);
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
        // Stick with wood grain
        for (int y = 5; y < 16; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){130, 85, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){110, 70, 22, 255});
            if (y % 3 == 0) ImageDrawPixel(img, px + 7, py + y, (Color){100, 65, 20, 255});
        }
        // Flame - layered with glow
        for (int y = 0; y < 6; y++)
            for (int x = 6; x < 10; x++) {
                Color flame;
                if (y == 0) flame = (Color){255, 255, 200, 255}; // bright tip
                else if (y <= 2) flame = (Color){255, 220, 50, 255}; // yellow core
                else if (y <= 4) flame = (Color){255, 150, 20, 255}; // orange
                else flame = (Color){200, 80, 10, 255}; // red base
                // Flame shape (tapers at top)
                if (y == 0 && (x < 7 || x > 8)) continue;
                if (y >= 4 && (x < 7 || x > 8)) continue;
                ImageDrawPixel(img, px + x, py + y, flame);
            }
        break;

    case BLOCK_FLOWER:
        // Stem with leaf
        for (int y = 8; y < 16; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){45, 130, 22, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){35, 110, 18, 255});
        }
        // Small leaf on stem
        ImageDrawPixel(img, px + 9, py + 11, (Color){50, 140, 25, 255});
        ImageDrawPixel(img, px + 10, py + 10, (Color){60, 150, 30, 255});
        // Petals with shading
        for (int y = 2; y < 8; y++)
            for (int x = 4; x < 12; x++) {
                // Flower shape: 5 petals in cross pattern
                int cx = 8, cy = 5;
                int dx = abs(x - cx), dy = abs(y - cy);
                bool inPetal = false;
                if (dx <= 1 && dy <= 3) inPetal = true; // vertical petals
                if (dy <= 1 && dx <= 3) inPetal = true; // horizontal petals
                if (dx <= 2 && dy <= 2 && dx + dy <= 3) inPetal = true; // center
                if (inPetal) {
                    Color c = base;
                    // Gradient: lighter toward tips
                    if (dy == 3 || dx == 3) c = detail;
                    // Highlight
                    if (y == 3 && x >= 7 && x <= 9) c = (Color){(unsigned char)(base.r+30>255?255:base.r+30), (unsigned char)(base.g+30>255?255:base.g+30), (unsigned char)(base.b+30>255?255:base.b+30), 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Center pistil
        ImageDrawPixel(img, px + 7, py + 5, (Color){255, 220, 50, 255});
        ImageDrawPixel(img, px + 8, py + 5, (Color){255, 230, 60, 255});
        ImageDrawPixel(img, px + 7, py + 4, (Color){255, 210, 40, 255});
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
                // Stone border
                if (x == 0 || x == 15 || y == 0 || y == 15) c = detail;
                // Corner bricks
                if ((x <= 1 || x >= 14) && (y <= 1 || y >= 14)) c = (Color){90, 90, 90, 255};
                // Opening (dark)
                if (x >= 5 && x <= 10 && y >= 5 && y <= 10) c = (Color){30, 30, 30, 255};
                // Opening border
                if ((x == 5 || x == 10) && y >= 5 && y <= 10) c = (Color){70, 70, 70, 255};
                if ((y == 5 || y == 10) && x >= 5 && x <= 10) c = (Color){70, 70, 70, 255};
                // Fire glow inside
                if (x >= 6 && x <= 9 && y >= 7 && y <= 9) c = (Color){120, 50, 10, 255};
                if (x >= 7 && x <= 8 && y >= 8 && y <= 9) c = (Color){200, 100, 20, 255};
                // Surface variation
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 55);
                if (h % 12 == 0 && x > 1 && x < 14 && y > 1 && y < 14) c = (Color){110, 110, 110, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_BED:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Blanket (top 2/3)
                if (y >= 0 && y <= 10) {
                    c = base;
                    // Blanket folds/creases
                    if (y == 0 || y == 10) c = detail;
                    if (y == 5 && x >= 6) c = detail;
                    // Highlight on upper blanket
                    if (y >= 1 && y <= 3 && x >= 6 && x <= 13) c = (Color){220, 70, 70, 255};
                    // Shadow on lower blanket
                    if (y >= 8 && y <= 9 && x >= 6 && x <= 13) c = (Color){140, 35, 35, 255};
                }
                // Pillow (left side) with puff shape
                if (x >= 1 && x <= 5 && y >= 1 && y <= 4) {
                    c = (Color){240, 240, 240, 255};
                    if (y == 1) c = (Color){250, 250, 250, 255}; // top highlight
                    if (y == 4) c = (Color){220, 220, 220, 255}; // bottom shadow
                    if (x == 1) c = (Color){225, 225, 225, 255}; // left shadow
                    if (x == 5) c = (Color){225, 225, 225, 255}; // right shadow
                }
                // Wooden frame (bottom)
                if (y >= 11 && y <= 15) {
                    c = (Color){140, 100, 50, 255};
                    if (y == 11) c = (Color){120, 80, 40, 255};
                    // Frame leg shadows
                    if ((x == 1 || x == 14) && y >= 13) c = (Color){100, 70, 35, 255};
                    // Frame plank detail
                    if (y == 13 && x >= 2 && x <= 13) c = (Color){155, 115, 60, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    // Item icons (atlas only)
    case ITEM_STICK:
        // Stick with wood grain and knot
        for (int y = 2; y < 15; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){130, 90, 40, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){110, 75, 30, 255});
            if (y % 4 == 0) ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 25, 255});
        }
        // Knot detail
        ImageDrawPixel(img, px + 7, py + 8, (Color){90, 60, 20, 255});
        break;
    case ITEM_COAL:
        // Coal chunk with glossy highlights
        for (int y = 4; y < 12; y++) for (int x = 4; x < 12; x++) {
            Color c = base;
            if (x == 4 || x == 11 || y == 4 || y == 11) c = (Color){50, 50, 50, 255};
            // Glossy highlight
            if (x >= 6 && x <= 8 && y >= 5 && y <= 7) c = (Color){70, 70, 70, 255};
            if (x == 7 && y == 6) c = (Color){90, 90, 90, 255}; // bright spot
            // Cracks
            if ((x == 9 && y >= 7 && y <= 9) || (x == 6 && y == 9)) c = detail;
            ImageDrawPixel(img, px + x, py + y, c);
        }
        break;
    case ITEM_IRON_INGOT:
        // Ingot shape with 3D shading
        for (int y = 5; y < 11; y++) for (int x = 3; x < 13; x++) {
            Color c = base;
            if (y == 5) c = (Color){240, 230, 220, 255}; // top highlight
            if (y == 10) c = (Color){(unsigned char)(detail.r*0.8f), (unsigned char)(detail.g*0.8f), (unsigned char)(detail.b*0.8f), 255}; // bottom shadow
            if (x == 3 || x == 12) c = detail; // side edges
            if (y >= 6 && y <= 9 && x >= 4 && x <= 11) c = detail; // inner shade
            if (y == 6 && x >= 5 && x <= 10) c = (Color){245, 235, 225, 255}; // top face shine
            ImageDrawPixel(img, px + x, py + y, c);
        }
        break;

    case BLOCK_GOLD_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Gold deposits with shimmer
                if ((x >= 3 && x <= 5 && y >= 3 && y <= 5) ||
                    (x >= 10 && x <= 12 && y >= 9 && y <= 11) ||
                    (x >= 6 && x <= 7 && y >= 10 && y <= 11)) {
                    c = detail;
                    // Bright gold sparkle highlight
                    if ((x == 3 && y == 3) || (x == 11 && y == 9))
                        c = (Color){255, 230, 100, 255};
                    // Deep gold shadow
                    if ((x == 5 && y == 5) || (x == 12 && y == 11))
                        c = (Color){160, 120, 20, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_DIAMOND_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Diamond crystal deposits with sparkle
                if ((x >= 4 && x <= 6 && y >= 5 && y <= 7) ||
                    (x >= 9 && x <= 11 && y >= 2 && y <= 3) ||
                    (x >= 2 && x <= 3 && y >= 11 && y <= 13)) {
                    c = detail;
                    // Bright crystal sparkle
                    if ((x == 4 && y == 5) || (x == 10 && y == 2))
                        c = (Color){150, 255, 255, 255};
                    // Deep crystal shadow
                    if ((x == 6 && y == 7) || (x == 3 && y == 13))
                        c = (Color){30, 140, 160, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_REDSTONE_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Redstone deposits with glow
                if ((x >= 3 && x <= 5 && y >= 4 && y <= 6) ||
                    (x >= 10 && x <= 12 && y >= 8 && y <= 10) ||
                    (x >= 6 && x <= 8 && y >= 11 && y <= 13)) {
                    c = detail;
                    // Bright red glow
                    if ((x == 4 && y == 5) || (x == 11 && y == 9))
                        c = (Color){255, 80, 80, 255};
                    // Dark crack
                    if ((x == 5 && y == 6) || (x == 12 && y == 10))
                        c = (Color){100, 15, 15, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_LAPIS_ORE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = blockInfo[BLOCK_STONE].baseColor;
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 4);
                if (h % 8 == 0) c = blockInfo[BLOCK_STONE].detailColor;
                if (h % 17 == 0) c = (Color){115, 115, 115, 255};
                // Lapis deposits
                if ((x >= 4 && x <= 6 && y >= 3 && y <= 5) ||
                    (x >= 9 && x <= 11 && y >= 7 && y <= 9) ||
                    (x >= 3 && x <= 5 && y >= 10 && y <= 12)) {
                    c = detail;
                    // Bright blue highlight
                    if ((x == 5 && y == 4) || (x == 10 && y == 8))
                        c = (Color){60, 100, 255, 255};
                    // Dark blue shadow
                    if ((x == 6 && y == 5) || (x == 5 && y == 12))
                        c = (Color){15, 25, 100, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_GOLD_INGOT:
        // Gold ingot with rich 3D shading
        for (int y = 5; y < 11; y++) for (int x = 3; x < 13; x++) {
            Color c = base;
            if (y == 5) c = (Color){255, 220, 80, 255}; // top highlight
            if (y == 10) c = (Color){(unsigned char)(detail.r*0.8f), (unsigned char)(detail.g*0.8f), (unsigned char)(detail.b*0.8f), 255}; // bottom shadow
            if (x == 3 || x == 12) c = detail; // side edges
            if (y >= 6 && y <= 9 && x >= 4 && x <= 11) c = detail; // inner shade
            if (y == 6 && x >= 5 && x <= 10) c = (Color){255, 230, 100, 255}; // top face shine
            ImageDrawPixel(img, px + x, py + y, c);
        }
        break;

    case ITEM_DIAMOND:
        // Diamond gem - faceted shape with light refraction
        for (int y = 2; y < 14; y++)
            for (int x = 3; x < 13; x++) {
                bool inShape = false;
                if (y < 6 && x >= 6 - (y - 2) && x <= 9 + (y - 2)) inShape = true;
                if (y >= 6 && y < 10 && x >= 3 && x <= 12) inShape = true;
                if (y >= 10 && x >= 3 + (y - 10) * 2 && x <= 12 - (y - 10) * 2) inShape = true;
                if (inShape) {
                    Color c = base;
                    // Light refraction facets
                    if (y < 6 && x < 7) c = detail; // upper-left darker
                    if (y < 6 && x >= 8) c = (Color){120, 240, 250, 255}; // upper-right lighter
                    if (y >= 6 && y < 10 && x < 6) c = detail; // left facet
                    if (y >= 6 && y < 10 && x >= 9) c = (Color){100, 230, 240, 255}; // right facet bright
                    // Sparkle highlights
                    if ((x == 6 && y == 4) || (x == 9 && y == 7)) c = (Color){200, 255, 255, 255};
                    // Bottom facet shadow
                    if (y >= 10) c = (Color){(unsigned char)(detail.r*0.8f), (unsigned char)(detail.g*0.8f), (unsigned char)(detail.b*0.8f), 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        break;

    case ITEM_REDSTONE:
        // Redstone dust - small red crystals
        for (int y = 5; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                unsigned int h = hash2D(x, y, 500);
                if (h % 3 == 0) {
                    Color c = base;
                    if (h % 5 == 0) c = (Color){255, 60, 60, 255};
                    if (h % 7 == 0) c = detail;
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Bright dust specks
        ImageDrawPixel(img, px + 6, py + 7, (Color){255, 100, 100, 255});
        ImageDrawPixel(img, px + 9, py + 9, (Color){255, 80, 80, 255});
        break;

    case ITEM_LAPIS:
        // Lapis lazuli - blue chunks
        for (int y = 4; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                unsigned int h = hash2D(x, y, 501);
                if (h % 3 == 0) {
                    Color c = base;
                    if (h % 5 == 0) c = (Color){50, 80, 255, 255};
                    if (h % 7 == 0) c = detail;
                    // Gold flecks (pyrite in lapis)
                    if (h % 11 == 0) c = (Color){200, 180, 50, 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Bright blue spots
        ImageDrawPixel(img, px + 7, py + 6, (Color){70, 120, 255, 255});
        ImageDrawPixel(img, px + 9, py + 9, (Color){60, 100, 255, 255});
        break;

    case ITEM_GUNPOWDER:
        // Dark gray powder grains
        for (int y = 5; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                unsigned int h = hash2D(x, y, 502);
                if (h % 3 == 0) {
                    Color c = base;
                    if (h % 5 == 0) c = (Color){60, 60, 60, 255};
                    if (h % 7 == 0) c = detail;
                    if (h % 11 == 0) c = (Color){100, 100, 100, 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Bright specks
        ImageDrawPixel(img, px + 6, py + 7, (Color){110, 110, 110, 255});
        ImageDrawPixel(img, px + 9, py + 8, (Color){90, 90, 90, 255});
        break;

    case ITEM_STRING:
        // Coiled string
        for (int y = 4; y < 12; y++)
            for (int x = 4; x < 12; x++) {
                unsigned int h = hash2D(x, y, 503);
                if (h % 4 == 0) {
                    Color c = base;
                    if (h % 6 == 0) c = detail;
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // String line
        for (int i = 0; i < 8; i++) {
            int sx = 4 + i;
            int sy = 7 + (i % 3 == 0 ? -1 : 0);
            ImageDrawPixel(img, px + sx, py + sy, (Color){220, 220, 220, 255});
        }
        break;

    case BLOCK_CHEST:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Border/frame
                if (x == 0 || x == 15 || y == 0 || y == 15) c = detail;
                // Lid line (top half)
                if (y == 5) c = (Color){80, 55, 25, 255};
                // Bottom edge
                if (y == 15) c = (Color){80, 55, 25, 255};
                // Planks vertical lines
                if (x == 4 || x == 8 || x == 12) c = (Color){(unsigned char)(detail.r*0.9f), (unsigned char)(detail.g*0.9f), (unsigned char)(detail.b*0.9f), 255};
                // Lock/clasp (gold)
                if (x >= 7 && x <= 8 && y >= 4 && y <= 6) c = (Color){180, 150, 50, 255};
                if (x == 7 && y == 5) c = (Color){220, 190, 70, 255}; // lock highlight
                // Wood grain highlights on lid
                if (y >= 1 && y <= 4 && (x == 2 || x == 6 || x == 10 || x == 14)) c = (Color){160, 120, 65, 255};
                // Shadow on bottom half
                if (y >= 10 && y <= 14) c = (Color){(unsigned char)(base.r*0.85f), (unsigned char)(base.g*0.85f), (unsigned char)(base.b*0.85f), 255};
                // Top lid highlight
                if (y == 1 && x >= 2 && x <= 13) c = (Color){170, 130, 70, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    // Tool icons (atlas only, not used for world rendering)
    case TOOL_WOOD_PICKAXE:
    case TOOL_STONE_PICKAXE:
    case TOOL_IRON_PICKAXE:
    case TOOL_GOLD_PICKAXE:
    case TOOL_DIAMOND_PICKAXE: {
        Color tc = base; Color th = detail;
        // Handle with wood grain
        for (int y = 8; y < 15; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255});
            if (y % 3 == 0) ImageDrawPixel(img, px + 7, py + y, (Color){90, 60, 25, 255});
        }
        // Head with metallic gradient
        for (int x = 3; x < 13; x++) for (int y = 3; y < 7; y++) {
            Color c = tc;
            if (y == 3) c = th; // top edge highlight
            if (y == 6) c = (Color){(unsigned char)(tc.r*0.7f), (unsigned char)(tc.g*0.7f), (unsigned char)(tc.b*0.7f), 255}; // bottom shadow
            ImageDrawPixel(img, px + x, py + y, c);
        }
        // Head tips
        ImageDrawPixel(img, px + 2, py + 4, th);
        ImageDrawPixel(img, px + 13, py + 4, th);
        break;
    }
    case TOOL_WOOD_AXE:
    case TOOL_STONE_AXE:
    case TOOL_IRON_AXE:
    case TOOL_GOLD_AXE:
    case TOOL_DIAMOND_AXE: {
        Color tc = base; Color th = detail;
        // Handle
        for (int y = 6; y < 15; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255});
        }
        // Axe head with edge highlight
        for (int x = 4; x < 9; x++) for (int y = 2; y < 8; y++) {
            Color c = tc;
            if (y == 2) c = th; // top edge
            if (x == 4) c = (Color){(unsigned char)(tc.r*0.8f), (unsigned char)(tc.g*0.8f), (unsigned char)(tc.b*0.8f), 255};
            ImageDrawPixel(img, px + x, py + y, c);
        }
        // Sharp edge
        for (int y = 3; y < 7; y++) ImageDrawPixel(img, px + 3, py + y, th);
        break;
    }
    case TOOL_WOOD_SWORD:
    case TOOL_STONE_SWORD:
    case TOOL_IRON_SWORD:
    case TOOL_GOLD_SWORD:
    case TOOL_DIAMOND_SWORD: {
        Color tc = base; Color th = detail;
        // Blade with highlight and shadow
        for (int y = 1; y < 12; y++) {
            ImageDrawPixel(img, px + 7, py + y, tc);
            ImageDrawPixel(img, px + 8, py + y, th);
            // Edge highlight
            if (y > 1 && y < 11) ImageDrawPixel(img, px + 6, py + y, (Color){(unsigned char)(tc.r*0.85f), (unsigned char)(tc.g*0.85f), (unsigned char)(tc.b*0.85f), 255});
        }
        // Blade tip
        ImageDrawPixel(img, px + 7, py + 0, th);
        // Guard with detail
        for (int x = 5; x < 11; x++) ImageDrawPixel(img, px + x, py + 12, (Color){150, 130, 80, 255});
        ImageDrawPixel(img, px + 5, py + 12, (Color){170, 150, 90, 255});
        ImageDrawPixel(img, px + 10, py + 12, (Color){120, 100, 60, 255});
        // Handle with wrap
        for (int y = 13; y < 16; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){100, 70, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){80, 55, 20, 255});
        }
        // Pommel
        ImageDrawPixel(img, px + 7, py + 15, (Color){130, 100, 50, 255});
        ImageDrawPixel(img, px + 8, py + 15, (Color){110, 80, 40, 255});
        break;
    }
    case TOOL_WOOD_SHOVEL:
    case TOOL_STONE_SHOVEL:
    case TOOL_IRON_SHOVEL:
    case TOOL_GOLD_SHOVEL:
    case TOOL_DIAMOND_SHOVEL: {
        Color tc = base; Color th = detail;
        // Handle with grip
        for (int y = 2; y < 14; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){120, 80, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){100, 65, 20, 255});
        }
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
    case TOOL_WOOD_HOE:
    case TOOL_STONE_HOE:
    case TOOL_IRON_HOE:
    case TOOL_GOLD_HOE:
    case TOOL_DIAMOND_HOE: {
        Color tc = base; Color th = detail;
        // Handle
        for (int y = 3; y < 14; y++) {
            ImageDrawPixel(img, px + 7, py + y, (Color){120, 80, 30, 255});
            ImageDrawPixel(img, px + 8, py + y, (Color){100, 65, 20, 255});
        }
        // Hoe blade (horizontal at top)
        for (int x = 4; x < 12; x++) {
            ImageDrawPixel(img, px + x, py + 2, tc);
            ImageDrawPixel(img, px + x, py + 3, th);
        }
        // Grip
        ImageDrawPixel(img, px + 7, py + 14, (Color){80, 55, 20, 255});
        ImageDrawPixel(img, px + 8, py + 14, (Color){80, 55, 20, 255});
        break;
    }
    // Food items
    case FOOD_RAW_PORK: {
        // Raw meat - pinkish chunk with marbling
        for (int y = 3; y < 13; y++)
            for (int x = 3; x < 13; x++) {
                Color c = base;
                // Outer edge
                if (x == 3 || x == 12 || y == 3 || y == 12) c = detail;
                // Meat texture
                unsigned int h = hash2D(x, y, 42);
                if (h % 5 == 0) c = (Color){220, 150, 150, 255};
                else if (h % 7 == 0) c = (Color){180, 110, 110, 255};
                // Fat marbling
                if ((x == 5 && y >= 6 && y <= 8) || (x == 9 && y >= 7 && y <= 9)) c = (Color){240, 210, 200, 255};
                // Top highlight
                if (y == 4 && x >= 5 && x <= 10) c = (Color){230, 170, 170, 255};
                // Bottom shadow
                if (y == 11 && x >= 5 && x <= 10) c = (Color){(unsigned char)(detail.r*0.8f), (unsigned char)(detail.g*0.8f), (unsigned char)(detail.b*0.8f), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Bone sticking out
        ImageDrawPixel(img, px + 7, py + 2, (Color){230, 220, 200, 255});
        ImageDrawPixel(img, px + 8, py + 2, (Color){240, 230, 210, 255});
        ImageDrawPixel(img, px + 8, py + 1, (Color){220, 210, 190, 255});
        break;
    }
    case FOOD_COOKED_PORK: {
        // Cooked meat - golden-brown with grill marks
        for (int y = 3; y < 13; y++)
            for (int x = 3; x < 13; x++) {
                Color c = base;
                if (x == 3 || x == 12 || y == 3 || y == 12) c = detail;
                unsigned int h = hash2D(x, y, 43);
                if (h % 4 == 0) c = (Color){200, 120, 70, 255};
                else if (h % 6 == 0) c = (Color){160, 90, 50, 255};
                // Grill marks (diagonal dark lines)
                if ((x + y) % 5 == 0 && y >= 5 && y <= 10) c = (Color){120, 70, 30, 255};
                // Top glaze highlight
                if (y == 4 && x >= 5 && x <= 10) c = (Color){210, 140, 80, 255};
                // Juicy shine
                if (x == 7 && y == 7) c = (Color){220, 150, 90, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case FOOD_APPLE: {
        // Apple - round with gradient shading
        for (int y = 3; y < 14; y++)
            for (int x = 4; x < 12; x++) {
                float dx = x - 8.0f, dy = y - 8.5f;
                if (dx * dx + dy * dy < 20) {
                    Color c = base;
                    // Left shadow
                    if (dx < -2) c = (Color){170, 35, 35, 255};
                    // Right highlight
                    if (dx > 1 && dy < 0) c = (Color){230, 70, 70, 255};
                    // Top shine spot
                    if (dx >= 0 && dx <= 1 && dy >= -3 && dy <= -1) c = (Color){255, 120, 120, 255};
                    // Bottom dark
                    if (dy > 2) c = (Color){(unsigned char)(base.r*0.7f), (unsigned char)(base.g*0.7f), (unsigned char)(base.b*0.7f), 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        // Stem
        ImageDrawPixel(img, px + 8, py + 3, (Color){90, 60, 25, 255});
        ImageDrawPixel(img, px + 8, py + 2, (Color){80, 55, 20, 255});
        // Leaf
        ImageDrawPixel(img, px + 9, py + 2, (Color){50, 130, 25, 255});
        ImageDrawPixel(img, px + 10, py + 1, (Color){60, 140, 30, 255});
        break;
    }
    case FOOD_BREAD: {
        // Bread - golden loaf with crust texture
        for (int y = 4; y < 12; y++)
            for (int x = 3; x < 13; x++) {
                Color c = base;
                // Top crust (golden brown)
                if (y == 4) c = (Color){230, 200, 120, 255};
                if (y == 5) c = (Color){220, 190, 110, 255};
                // Bottom crust
                if (y == 11) c = detail;
                // Side edges
                if (x == 3 || x == 12) c = detail;
                // Inner bread texture
                unsigned int h = hash2D(x, y, 44);
                if (h % 8 == 0 && y >= 6 && y <= 10) c = (Color){200, 170, 90, 255};
                // Crust highlights
                if (y == 4 && x >= 5 && x <= 10) c = (Color){240, 210, 130, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Score marks on top
        ImageDrawPixel(img, px + 5, py + 4, detail);
        ImageDrawPixel(img, px + 6, py + 4, detail);
        ImageDrawPixel(img, px + 8, py + 4, detail);
        ImageDrawPixel(img, px + 9, py + 4, detail);
        ImageDrawPixel(img, px + 11, py + 4, detail);
        break;
    }
    case BLOCK_CRAFTING_TABLE: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                if (y <= 5) {
                    // Table top surface with wood grain
                    c = base;
                    if (y == 0 || y == 5) c = detail;
                    // 3x3 grid lines (crafting grid)
                    if (y >= 1 && y <= 4) {
                        if (x == 3 || x == 7 || x == 11) c = (Color){170, 135, 75, 255};
                        if (y == 2) c = (Color){170, 135, 75, 255};
                    }
                    // Wood grain
                    if (y >= 1 && y <= 4 && (x == 1 || x == 5 || x == 9 || x == 14)) c = (Color){155, 120, 65, 255};
                    // Top highlight
                    if (y == 1 && x >= 1 && x <= 14) c = (Color){165, 130, 72, 255};
                } else if (y <= 6) {
                    c = detail; // table edge/lip
                } else {
                    // Legs with wood texture
                    if ((x >= 2 && x <= 4) || (x >= 11 && x <= 13)) {
                        c = detail;
                        if (x == 2 || x == 11) c = (Color){(unsigned char)(detail.r*0.8f), (unsigned char)(detail.g*0.8f), (unsigned char)(detail.b*0.8f), 255};
                    } else {
                        c = (Color){0, 0, 0, 0}; // transparent between legs
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_HELMET: case ARMOR_STONE_HELMET: case ARMOR_IRON_HELMET: case ARMOR_GOLD_HELMET: case ARMOR_DIAMOND_HELMET: {
        Color tc = base; Color th = detail;
        // Helmet dome
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 2 && y <= 13) {
                    int cx = 8, cy = 7;
                    int dx = x - cx, dy = y - cy;
                    if (dx * dx + dy * dy <= 30) {
                        c = tc;
                        // Top highlight
                        if (y <= 4) c = (Color){(unsigned char)(tc.r+30>255?255:tc.r+30), (unsigned char)(tc.g+30>255?255:tc.g+30), (unsigned char)(tc.b+30>255?255:tc.b+30), 255};
                        // Bottom shadow
                        if (y >= 11) c = (Color){(unsigned char)(tc.r*0.7f), (unsigned char)(tc.g*0.7f), (unsigned char)(tc.b*0.7f), 255};
                        // Visor slit
                        if (y >= 8 && y <= 9 && x >= 5 && x <= 11) c = (Color){20, 20, 20, 255};
                        // Side rivets
                        if ((x == 4 && y == 7) || (x == 12 && y == 7)) c = th;
                        // Top ridge
                        if (y == 3 && x >= 6 && x <= 10) c = th;
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_CHESTPLATE: case ARMOR_STONE_CHESTPLATE: case ARMOR_IRON_CHESTPLATE: case ARMOR_GOLD_CHESTPLATE: case ARMOR_DIAMOND_CHESTPLATE: {
        Color tc = base; Color th = detail;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 1 && y <= 14) {
                    // Main body
                    if (x >= 4 && x <= 11) {
                        c = tc;
                        // Top/bottom edges
                        if (y == 1 || y == 14) c = th;
                        // Side edges
                        if (x == 4 || x == 11) c = th;
                        // Center seam
                        if (x == 7 || x == 8) c = (Color){(unsigned char)(tc.r*0.85f), (unsigned char)(tc.g*0.85f), (unsigned char)(tc.b*0.85f), 255};
                        // Upper body highlight
                        if (y >= 3 && y <= 6 && x >= 5 && x <= 10) c = (Color){(unsigned char)(tc.r+20>255?255:tc.r+20), (unsigned char)(tc.g+20>255?255:tc.g+20), (unsigned char)(tc.b+20>255?255:tc.b+20), 255};
                        // Belt
                        if (y >= 10 && y <= 11) c = (Color){(unsigned char)(tc.r*0.6f), (unsigned char)(tc.g*0.6f), (unsigned char)(tc.b*0.6f), 255};
                        // Belt buckle
                        if (y >= 10 && y <= 11 && x >= 7 && x <= 8) c = th;
                    }
                    // Shoulder guards (pauldrons)
                    if (x >= 2 && x <= 4 && y >= 2 && y <= 6) {
                        c = th;
                        if (y == 2) c = (Color){(unsigned char)(th.r+20>255?255:th.r+20), (unsigned char)(th.g+20>255?255:th.g+20), (unsigned char)(th.b+20>255?255:th.b+20), 255};
                    }
                    if (x >= 11 && x <= 13 && y >= 2 && y <= 6) {
                        c = th;
                        if (y == 2) c = (Color){(unsigned char)(th.r+20>255?255:th.r+20), (unsigned char)(th.g+20>255?255:th.g+20), (unsigned char)(th.b+20>255?255:th.b+20), 255};
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_LEGGINGS: case ARMOR_STONE_LEGGINGS: case ARMOR_IRON_LEGGINGS: case ARMOR_GOLD_LEGGINGS: case ARMOR_DIAMOND_LEGGINGS: {
        Color tc = base; Color th = detail;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 1 && y <= 14) {
                    // Left leg
                    if (x >= 3 && x <= 6) {
                        c = tc;
                        if (x == 3 || x == 6) c = th;
                        if (y == 1 || y == 14) c = th;
                        // Knee highlight
                        if (y >= 7 && y <= 8 && x >= 4 && x <= 5) c = (Color){(unsigned char)(tc.r+20>255?255:tc.r+20), (unsigned char)(tc.g+20>255?255:tc.g+20), (unsigned char)(tc.b+20>255?255:tc.b+20), 255};
                        // Shin shadow
                        if (y >= 12 && x == 5) c = (Color){(unsigned char)(tc.r*0.75f), (unsigned char)(tc.g*0.75f), (unsigned char)(tc.b*0.75f), 255};
                    }
                    // Right leg
                    if (x >= 9 && x <= 12) {
                        c = tc;
                        if (x == 9 || x == 12) c = th;
                        if (y == 1 || y == 14) c = th;
                        // Knee highlight
                        if (y >= 7 && y <= 8 && x >= 10 && x <= 11) c = (Color){(unsigned char)(tc.r+20>255?255:tc.r+20), (unsigned char)(tc.g+20>255?255:tc.g+20), (unsigned char)(tc.b+20>255?255:tc.b+20), 255};
                        // Shin shadow
                        if (y >= 12 && x == 10) c = (Color){(unsigned char)(tc.r*0.75f), (unsigned char)(tc.g*0.75f), (unsigned char)(tc.b*0.75f), 255};
                    }
                    // Waist band with buckle
                    if (y >= 1 && y <= 3 && x >= 3 && x <= 12) {
                        c = th;
                        if (y == 2 && x >= 7 && x <= 8) c = (Color){(unsigned char)(th.r+30>255?255:th.r+30), (unsigned char)(th.g+30>255?255:th.g+30), (unsigned char)(th.b+30>255?255:th.b+30), 255};
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ARMOR_WOOD_BOOTS: case ARMOR_STONE_BOOTS: case ARMOR_IRON_BOOTS: case ARMOR_GOLD_BOOTS: case ARMOR_DIAMOND_BOOTS: {
        Color tc = base; Color th = detail;
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                if (y >= 5 && y <= 14) {
                    // Left boot
                    if (x >= 2 && x <= 6) {
                        c = tc;
                        if (y == 5 || y == 14) c = th;
                        if (x == 2 || x == 6) c = th;
                        // Toe cap
                        if (y >= 13 && x <= 3) c = th;
                        // Ankle highlight
                        if (y >= 6 && y <= 7 && x >= 3 && x <= 5) c = (Color){(unsigned char)(tc.r+20>255?255:tc.r+20), (unsigned char)(tc.g+20>255?255:tc.g+20), (unsigned char)(tc.b+20>255?255:tc.b+20), 255};
                        // Sole
                        if (y == 14) c = (Color){(unsigned char)(tc.r*0.5f), (unsigned char)(tc.g*0.5f), (unsigned char)(tc.b*0.5f), 255};
                    }
                    // Right boot
                    if (x >= 9 && x <= 13) {
                        c = tc;
                        if (y == 5 || y == 14) c = th;
                        if (x == 9 || x == 13) c = th;
                        // Toe cap
                        if (y >= 13 && x >= 12) c = th;
                        // Ankle highlight
                        if (y >= 6 && y <= 7 && x >= 10 && x <= 12) c = (Color){(unsigned char)(tc.r+20>255?255:tc.r+20), (unsigned char)(tc.g+20>255?255:tc.g+20), (unsigned char)(tc.b+20>255?255:tc.b+20), 255};
                        // Sole
                        if (y == 14) c = (Color){(unsigned char)(tc.r*0.5f), (unsigned char)(tc.g*0.5f), (unsigned char)(tc.b*0.5f), 255};
                    }
                    // Lace/buckle details
                    if (y == 7 && ((x >= 4 && x <= 5) || (x >= 10 && x <= 11))) c = th;
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    case ITEM_BONE:
        // White/cream bone with knobby ends
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = BLANK;
                // Shaft (diagonal)
                if (x >= 4 && x <= 11 && y >= 5 && y <= 10) {
                    c = base;
                    // Center hole
                    if (x >= 7 && x <= 8 && y >= 7 && y <= 8)
                        c = (Color){180, 170, 150, 255};
                }
                // Knobby ends
                if ((x >= 2 && x <= 5 && y >= 3 && y <= 6) ||
                    (x >= 10 && x <= 13 && y >= 9 && y <= 12))
                    c = detail;
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_ARROW:
        // Shaft + arrowhead + fletching
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = BLANK;
                // Shaft (diagonal line)
                if (x + y >= 8 && x + y <= 11 && x >= 2 && x <= 13) {
                    c = base;
                }
                // Arrowhead (triangle at top-right)
                if (x >= 11 && x <= 14 && y >= 1 && y <= 4) {
                    if (x - 11 + y <= 3) c = detail;
                }
                // Fletching (at bottom-left)
                if (x >= 1 && x <= 4 && y >= 11 && y <= 14) {
                    if ((x + y) % 2 == 0) c = (Color){200, 50, 50, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_BOW:
        // Bow: curved arc + string
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = BLANK;
                // Bow limb (curved arc on the right side)
                int cx2 = 12, cy2 = 8;
                int dx = x - cx2, dy = y - cy2;
                int dist = (int)sqrtf((float)(dx*dx + dy*dy));
                if (dist >= 5 && dist <= 7 && x >= 8) {
                    c = base;
                    // Thicker at center of limb
                    if (dist == 6) c = detail;
                }
                // String (vertical line on left)
                if (x >= 4 && x <= 5 && y >= 3 && y <= 13) {
                    c = (Color){200, 200, 200, 255};
                }
                // Grip wrap
                if (x >= 5 && x <= 7 && y >= 7 && y <= 9) {
                    c = (Color){80, 50, 20, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_MOSSY_COBBLESTONE:
        // Cobblestone with green moss patches
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x, y, varSeed);
                // Cobblestone pattern
                if ((x + y) % 5 == 0 || h % 7 == 0) c = detail;
                // Stone highlights
                if (h % 11 == 0) c = (Color){120, 120, 110, 255};
                // Moss patches (green)
                unsigned int mh = hash2D(x + 3, y + 7, varSeed + 50);
                if (mh % 9 == 0) c = (Color){60, 130, 50, 255};
                if (mh % 13 == 0) c = (Color){50, 110, 40, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_BOOKSHELF:
        // Plank border + vertical colored book spines
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Top and bottom plank border
                if (y <= 1 || y >= 14) c = detail;
                // Left and right plank border
                if (x <= 1 || x >= 14) c = detail;
                // Book spines (vertical lines with different colors)
                if (y >= 2 && y <= 13 && x >= 2 && x <= 13) {
                    int bookIdx = (x - 2) / 2;
                    unsigned int bh = hash2D(bookIdx, 0, varSeed + 100);
                    Color bookColors[] = {
                        {180, 40, 40, 255}, {40, 40, 180, 255}, {40, 140, 40, 255},
                        {180, 140, 40, 255}, {140, 40, 140, 255}, {40, 140, 140, 255}
                    };
                    c = bookColors[bh % 6];
                    // Spine detail lines
                    if (y == 5 || y == 10) c = (Color){(unsigned char)(c.r*0.7f), (unsigned char)(c.g*0.7f), (unsigned char)(c.b*0.7f), 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_LANTERN:
        // Iron frame + warm yellow glow center
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = BLANK;
                // Iron frame (border)
                if ((x >= 4 && x <= 11 && (y == 2 || y == 13)) ||
                    (y >= 2 && y <= 13 && (x == 4 || x == 11))) {
                    c = base;
                }
                // Top hook
                if (x >= 7 && x <= 8 && y >= 0 && y <= 2) c = base;
                // Bottom point
                if (x >= 7 && x <= 8 && y == 14) c = base;
                // Cross bars
                if (x >= 5 && x <= 10 && (y == 7 || y == 8)) c = (Color){160, 140, 100, 255};
                // Warm glow center
                if (x >= 6 && x <= 9 && y >= 4 && y <= 11) {
                    c = detail;
                    // Brighter center
                    if (x >= 7 && x <= 8 && y >= 5 && y <= 10)
                        c = (Color){255, 240, 150, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_BONE_BLOCK:
        // Cream base with bone cross pattern
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x, y, varSeed + 200);
                // Subtle texture
                if (h % 8 == 0) c = detail;
                // Bone cross pattern (X shape)
                if ((x == y || x == 15 - y) && x >= 2 && x <= 13)
                    c = (Color){250, 240, 220, 255};
                // Center circle
                if ((x-8)*(x-8) + (y-8)*(y-8) <= 4)
                    c = (Color){250, 240, 220, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_SNOW:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 50);
                Color c = base;
                if (h % 9 == 0) c = (Color){230, 235, 250, 255};
                else if (h % 13 == 0) c = (Color){215, 220, 235, 255};
                if (y > 13) c = (Color){(unsigned char)(base.r - 8), (unsigned char)(base.g - 8), (unsigned char)(base.b - 5), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_ICE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 51);
                Color c = base;
                if (h % 7 == 0) c = (Color){200, 225, 250, 255};
                // Crack lines
                if ((x + y * 3) % 11 == 0) c = (Color){140, 180, 220, 255};
                // Highlight
                if (x < 3 && y < 3) c = (Color){220, 240, 255, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_PACKED_ICE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 52);
                Color c = base;
                if (h % 6 == 0) c = detail;
                if ((x + y) % 8 == 0) c = (Color){145, 180, 215, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_MUD:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 53);
                Color c = base;
                if (h % 7 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){110, 90, 70, 255};
                if (y > 13) c = (Color){70, 55, 40, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_MOSS_BLOCK:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 54);
                Color c = base;
                if (h % 5 == 0) c = (Color){60, 140, 40, 255};
                else if (h % 8 == 0) c = detail;
                else if (h % 13 == 0) c = (Color){90, 130, 50, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_JUNGLE_WOOD:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 55);
                Color c = base;
                // Vertical bark lines
                if (x % 4 == 0 || x % 4 == 1) c = detail;
                if (h % 9 == 0) c = (Color){95, 65, 35, 255};
                // Knot details
                if ((x == 6 && y == 5) || (x == 10 && y == 11))
                    c = (Color){75, 50, 25, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_JUNGLE_LEAVES:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 56);
                Color c = base;
                if (h % 4 == 0) c = (Color){40, 155, 25, 255};
                else if (h % 7 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){25, 130, 15, 255};
                // Leaf gaps (transparent feel)
                if (h % 15 == 0) c = (Color){20, 100, 10, 200};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_VINE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 57);
                Color c = {0, 0, 0, 0};
                // Vine strands
                if (x == 4 || x == 8 || x == 12) {
                    c = base;
                    if (h % 5 == 0) c = detail;
                }
                // Hanging leaves
                if (y > 8 && (x == 3 || x == 7 || x == 11)) {
                    c = (Color){50, 145, 30, 255};
                }
                if (h % 17 == 0 && c.a > 0) c = (Color){35, 120, 20, 255};
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_PUMPKIN:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 58);
                Color c = base;
                // Vertical segments
                if (x % 5 == 0) c = (Color){190, 120, 20, 255};
                if (h % 8 == 0) c = detail;
                // Face (top area)
                if (y < 4) c = (Color){80, 120, 30, 255}; // stem
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_MELON:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 59);
                Color c = base;
                // Vertical stripes
                if (x % 3 == 0) c = detail;
                if (h % 10 == 0) c = (Color){80, 160, 50, 255};
                // Darker spots
                if (h % 15 == 0) c = (Color){60, 130, 35, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_SNOWY_GRASS:
        // Dirt base
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 60);
                Color c = (Color){134, 96, 67, 255};
                if (h % 7 == 0) c = (Color){115, 80, 55, 255};
                else if (h % 11 == 0) c = (Color){150, 110, 80, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Snow top with icy edge
        for (int y = 0; y < 5; y++)
            for (int x = 0; x < 16; x++) {
                int wave = (int)(sinf(x * 0.7f + varSeed * 0.1f) * 1.5f);
                int snowEdge = 3 + wave;
                if (y < snowEdge) {
                    unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 61);
                    Color c = (Color){235, 240, 250, 255};
                    if (h % 5 == 0) c = (Color){220, 230, 245, 255};
                    ImageDrawPixel(img, px + x, py + y, c);
                }
            }
        break;

    case BLOCK_COARSE_DIRT:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 62);
                Color c = base;
                if (h % 5 == 0) c = detail;
                else if (h % 9 == 0) c = (Color){100, 70, 42, 255};
                // Gravel patches
                if (h % 13 == 0) c = (Color){130, 115, 100, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_PODZOL:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 63);
                Color c = base;
                // Dark organic layer on top
                if (y < 4) {
                    c = (Color){60, 40, 20, 255};
                    if (h % 7 == 0) c = (Color){50, 35, 18, 255};
                } else {
                    if (h % 6 == 0) c = detail;
                    else if (h % 11 == 0) c = (Color){130, 100, 65, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_SLIMEBALL:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                Color c = {0, 0, 0, 0};
                if (dx * dx + dy * dy < 30) {
                    c = base;
                    unsigned int h = hash2D(x, y, 64);
                    if (h % 5 == 0) c = (Color){100, 220, 70, 255};
                    // Highlight
                    if (dx < 0 && dy < 0 && dx * dx + dy * dy < 12)
                        c = (Color){150, 230, 110, 255};
                    // Dark spot
                    if (dx > 1 && dy > 1 && dx * dx + dy * dy < 8)
                        c = detail;
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_ENDER_PEARL:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                int dx = x - 8, dy = y - 8;
                Color c = {0, 0, 0, 0};
                if (dx * dx + dy * dy < 28) {
                    c = base;
                    unsigned int h = hash2D(x, y, 65);
                    // Purple swirl
                    if ((x + y) % 3 == 0) c = detail;
                    if (h % 7 == 0) c = (Color){80, 40, 160, 255};
                    // Bright center
                    if (dx * dx + dy * dy < 6) c = (Color){160, 100, 220, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_BUCKET: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Bucket body
                if (y >= 4 && y <= 14 && x >= 4 && x <= 11) {
                    c = base;
                    if (x == 4 || x == 11) c = detail;
                    if (y == 4 || y == 14) c = detail;
                    // Handle
                    if (y >= 2 && y <= 5 && x >= 6 && x <= 9) c = detail;
                    if (y == 2 && (x == 6 || x == 9)) c = (Color){0, 0, 0, 0};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }
    case ITEM_WATER_BUCKET: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Bucket body
                if (y >= 4 && y <= 14 && x >= 4 && x <= 11) {
                    c = base;
                    if (x == 4 || x == 11) c = detail;
                    if (y == 4 || y == 14) c = detail;
                    // Handle
                    if (y >= 2 && y <= 5 && x >= 6 && x <= 9) c = detail;
                    if (y == 2 && (x == 6 || x == 9)) c = (Color){0, 0, 0, 0};
                    // Water inside
                    if (y >= 6 && y <= 12 && x >= 5 && x <= 10) {
                        c = (Color){60, 140, 220, 255};
                        if (y == 6) c = (Color){80, 160, 240, 255};
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    case BLOCK_LEVER:
        // Stone base with a stick lever
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {80, 80, 80, 255};
                unsigned int h = hash2D(x, y, 70);
                if (h % 5 == 0) c = (Color){70, 70, 70, 255};
                // Stick handle
                if (x >= 7 && x <= 9 && y >= 2 && y <= 10) {
                    c = (Color){160, 120, 60, 255};
                    if (x == 8) c = (Color){180, 140, 70, 255};
                }
                // Lever knob
                if (x >= 6 && x <= 10 && y >= 1 && y <= 3) {
                    int dx = x - 8, dy = y - 2;
                    if (dx * dx + dy * dy < 6) c = (Color){200, 160, 80, 255};
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_REDSTONE_WIRE:
        // Thin red wire pattern on transparent background
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Cross pattern wire
                if ((y >= 7 && y <= 9) || (x >= 7 && x <= 9)) {
                    c = base;
                    unsigned int h = hash2D(x, y, 71);
                    if (h % 4 == 0) c = detail;
                    // Bright center line
                    if ((y == 8 && (x >= 3 && x <= 13)) || (x == 8 && (y >= 3 && y <= 13)))
                        c = (Color){240, 60, 40, 255};
                }
                // Connection dots at ends
                if ((x == 8 && (y <= 2 || y >= 14)) || (y == 8 && (x <= 2 || x >= 14)))
                    c = (Color){220, 40, 30, 255};
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_REDSTONE_LAMP:
        // Glowing lamp block
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                unsigned int h = hash2D(x, y, 72);
                if (h % 6 == 0) c = detail;
                // Inner glow
                int dx = x - 8, dy = y - 8;
                float dist = sqrtf((float)(dx * dx + dy * dy));
                if (dist < 5.0f) {
                    float bright = 1.0f - dist / 5.0f;
                    c.r = (unsigned char)(c.r + (255 - c.r) * bright * 0.6f);
                    c.g = (unsigned char)(c.g + (255 - c.g) * bright * 0.5f);
                    c.b = (unsigned char)(c.b * (1.0f - bright * 0.3f));
                }
                // Border
                if (x == 0 || x == 15 || y == 0 || y == 15)
                    c = (Color){100, 80, 30, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_STONE_PRESSURE_PLATE:
        // Thin stone plate on ground
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Only draw the plate area (middle portion)
                if (y >= 4 && y <= 11 && x >= 2 && x <= 13) {
                    c = base;
                    unsigned int h = hash2D(x, y, 73);
                    if (h % 5 == 0) c = detail;
                    // Top highlight
                    if (y == 4) c = (Color){160, 160, 160, 255};
                    // Bottom shadow
                    if (y == 11) c = (Color){90, 90, 90, 255};
                    // Edge darkening
                    if (x == 2 || x == 13) c = (Color){100, 100, 100, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    // Farming blocks
    case BLOCK_FARMLAND:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 80);
                Color c = base;
                if (h % 7 == 0) c = detail;
                // Furrow lines
                if (y % 4 == 0) c = (Color){100, 70, 45, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_CROPS: {
        // Render wheat growth stages 0-7: the plant grows taller as it matures and
        // golden wheat tops appear once nearly ripe. worldX/worldY are the block's
        // world coords; the block atlas is baked with an out-of-range worldY, so
        // out-of-bounds coords fall back to a fully-grown icon.
        int growth = (worldY >= 0 && worldY < WORLD_HEIGHT && worldX >= 0 && worldX < WORLD_WIDTH)
                     ? GetCropGrowth(worldX, worldY) : 7;
        if (growth < 0) growth = 0;
        if (growth > 7) growth = 7;
        int stalkTop = 14 - (3 + growth);   // stage 0 -> y11 (short sprout), stage 7 -> y4 (tall)
        bool ripe = (growth >= 6);
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Two green stalks growing upward from the soil
                if (((x >= 3 && x <= 5) || (x >= 10 && x <= 12)) && y >= stalkTop && y <= 14) {
                    c = (Color){80, 180, 40, 255};
                    if (y % 3 == 0) c = (Color){60, 150, 30, 255};
                }
                // Wheat tops only once nearly ripe; golden at full maturity
                if (ripe && y >= stalkTop - 2 && y <= stalkTop + 1 &&
                    ((x >= 2 && x <= 6) || (x >= 9 && x <= 13))) {
                    c = (growth >= 7) ? (Color){210, 185, 75, 255} : (Color){170, 175, 70, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    case BLOCK_HAY_BALE:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 81);
                Color c = base;
                if (h % 5 == 0) c = detail;
                // Hay strand lines
                if (y % 3 == 0) c = (Color){160, 140, 40, 255};
                if (x % 4 == 0) c = (Color){170, 150, 50, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_TNT:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                // Red explosive body with a white "TNT" band across the middle
                Color c = base;
                if ((x + y) % 7 == 0) c = detail;            // subtle speckle
                if (y >= 6 && y <= 9) {
                    c = (Color){235, 235, 225, 255};          // white label band
                    // crude "TNT" lettering in dark red
                    bool letter =
                        (x == 2 || x == 4) || (x == 3 && y == 6) ||                 // T
                        (x == 6 && y >= 6 && y <= 9) || (x == 8 && y >= 6 && y <= 9) || // N sides
                        (x == 7 && (y == 7 || y == 8)) ||                            // N diagonal
                        (x == 11 || x == 13) || (x == 12 && y == 6);                 // T
                    if (letter) c = (Color){150, 30, 25, 255};
                }
                // Dark banding top and bottom edges
                if (y == 0 || y == 15) c = (Color){120, 30, 25, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    // Animal drop items (small icons)
    case ITEM_RAW_BEEF:
    case ITEM_COOKED_BEEF:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Steak shape
                if ((x-8)*(x-8) + (y-8)*(y-8) <= 36) {
                    c = base;
                    unsigned int h = hash2D(x, y, 82);
                    if (h % 4 == 0) c = detail;
                    // Fat marbling
                    if ((x + y) % 5 == 0) c = (Color){220, 200, 180, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_LEATHER:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Leather hide shape
                if (x >= 2 && x <= 13 && y >= 3 && y <= 12) {
                    c = base;
                    unsigned int h = hash2D(x, y, 83);
                    if (h % 6 == 0) c = detail;
                    // Stitching marks
                    if ((x == 4 || x == 11) && y % 2 == 0) c = (Color){100, 60, 30, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_RAW_MUTTON:
    case ITEM_COOKED_MUTTON:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Chop shape
                if ((x-8)*(x-8) + (y-8)*(y-8) <= 30) {
                    c = base;
                    unsigned int h = hash2D(x, y, 84);
                    if (h % 5 == 0) c = detail;
                    // Bone
                    if (x >= 10 && x <= 12 && y >= 5 && y <= 11)
                        c = (Color){230, 220, 200, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_WOOL:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x, y, 85);
                Color c = base;
                if (h % 4 == 0) c = detail;
                // Fluffy texture
                if (h % 7 == 0) c = (Color){250, 250, 250, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_RAW_CHICKEN:
    case ITEM_COOKED_CHICKEN:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Drumstick shape
                if ((x-8)*(x-8) + (y-8)*(y-8) <= 25) {
                    c = base;
                    unsigned int h = hash2D(x, y, 86);
                    if (h % 5 == 0) c = detail;
                }
                // Bone
                if (x >= 10 && x <= 12 && y >= 3 && y <= 7)
                    c = (Color){230, 220, 200, 255};
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_FEATHER:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Feather quill
                if (x >= 7 && x <= 8 && y >= 2 && y <= 14)
                    c = (Color){180, 170, 160, 255};
                // Feather barbs
                if (y >= 4 && y <= 12) {
                    if (x >= 4 && x <= 6) c = (Color){240, 240, 240, 200};
                    if (x >= 9 && x <= 11) c = (Color){240, 240, 240, 200};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_EGG:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Egg shape (slightly oval)
                float dx = (x - 8) / 5.0f, dy = (y - 8) / 6.0f;
                if (dx*dx + dy*dy <= 1.0f) {
                    c = base;
                    unsigned int h = hash2D(x, y, 87);
                    if (h % 8 == 0) c = (Color){240, 230, 210, 255};
                    // Highlight
                    if (x >= 5 && x <= 7 && y >= 5 && y <= 7) c = (Color){250, 245, 235, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_WHEAT_SEEDS:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Small seed dots scattered
                unsigned int h = hash2D(x, y, 88);
                if (h % 7 == 0 && x >= 3 && x <= 12 && y >= 4 && y <= 12) {
                    c = (Color){160, 140, 60, 255};
                }
                if (h % 11 == 0 && x >= 4 && x <= 11 && y >= 5 && y <= 11) {
                    c = (Color){140, 120, 50, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case ITEM_WHEAT:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = {0, 0, 0, 0};
                // Wheat stalk bundle
                if (x >= 5 && x <= 10 && y >= 2 && y <= 14) {
                    c = (Color){200, 170, 60, 255};
                    if (x == 5 || x == 10) c = (Color){180, 150, 50, 255};
                }
                // Grain heads at top
                if (y >= 1 && y <= 4 && x >= 4 && x <= 11) {
                    unsigned int h = hash2D(x, y, 89);
                    if (h % 3 == 0) c = (Color){220, 190, 80, 255};
                }
                if (c.a > 0) ImageDrawPixel(img, px + x, py + y, c);
            }
        break;

    case BLOCK_CAULDRON: {
        // Find cauldron fill level
        uint8_t fill = 0;
        for (int ci = 0; ci < cauldronCount; ci++) {
            if (cauldrons[ci].x == worldX && cauldrons[ci].y == worldY) {
                fill = cauldrons[ci].fillLevel;
                break;
            }
        }
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = base;
                // Cauldron rim (top edge)
                if (y == 0 || y == 1) {
                    if (x > 0 && x < 15) {
                        c = (Color){80, 75, 90, 255}; // top rim
                    } else {
                        c = (Color){60, 55, 70, 255}; // side rim
                    }
                }
                // Side walls
                else if (x == 0 || x == 1 || x == 14 || x == 15) {
                    c = (Color){70, 65, 80, 255}; // iron wall
                }
                // Bottom
                else if (y == 15) {
                    c = (Color){60, 55, 70, 255};
                }
                // Interior (dark)
                else if (y > 2 && x > 1 && x < 14) {
                    c = (Color){40, 35, 50, 255};
                    // Water fill
                    if (fill > 0) {
                        int waterTop = 2 + (4 - fill) * 3; // higher fill = higher water line
                        if (y >= waterTop) {
                            float waterAlpha = (float)(y - waterTop) / (float)(16 - waterTop);
                            if (waterAlpha < 0) waterAlpha = 0;
                            c = (Color){
                                (unsigned char)(60 + 20 * waterAlpha),
                                (unsigned char)(140 + 40 * waterAlpha),
                                (unsigned char)(220 + 30 * waterAlpha),
                                (unsigned char)(180 * waterAlpha + 60)
                            };
                        }
                    }
                }
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Oak Stairs: sawtooth pattern
    case BLOCK_OAK_STAIRS: {
        for (int y = 0; y < 16; y++) {
            int step = 15 - y;
            for (int x = step; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 90);
                Color c = base;
                if (h % 5 == 0) c = detail;
                else if (h % 9 == 0) c = (Color){(unsigned char)(base.r + 20), (unsigned char)(base.g + 15), (unsigned char)(base.b + 10), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        }
        break;
    }

    // Cobblestone Stairs: sawtooth with cobblestone texture
    case BLOCK_COBBLESTONE_STAIRS: {
        for (int y = 0; y < 16; y++) {
            int step = 15 - y;
            for (int x = step; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 33);
                Color c = (h % 2 == 0) ? base : detail;
                if (h % 7 == 0) c = (Color){(unsigned char)(base.r + 15), (unsigned char)(base.g + 10), (unsigned char)(base.b + 10), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        }
        break;
    }

    // Stone Bricks: grid pattern
    case BLOCK_STONE_BRICKS: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 45);
                Color c = base;
                bool edge = (x % 8 == 0 || y % 8 == 0 || x == 15 || y == 15);
                if (edge) c = detail;
                else if (h % 7 == 0) c = (Color){(unsigned char)(base.r + 10), (unsigned char)(base.g + 10), (unsigned char)(base.b + 15), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Chiseled Stone Bricks: ornate carved pattern
    case BLOCK_CHISELED_STONE_BRICKS: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 67);
                Color c = base;
                bool border = (x < 2 || x >= 14 || y < 2 || y >= 14);
                bool innerBorder = (x >= 4 && x < 12 && y >= 4 && y < 12 && (x == 4 || x == 11 || y == 4 || y == 11));
                if (border) c = detail;
                else if (innerBorder) c = (Color){(unsigned char)(detail.r), (unsigned char)(detail.g), (unsigned char)(detail.b + 15), 255};
                else if (h % 5 == 0) c = (Color){(unsigned char)(base.r + 15), (unsigned char)(base.g + 15), (unsigned char)(base.b + 20), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Oak Slab: half-height block
    case BLOCK_OAK_SLAB: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 12);
                bool bottomHalf = (y >= 8);
                if (!bottomHalf) { ImageDrawPixel(img, px + x, py + y, (Color){0,0,0,0}); continue; }
                Color c = base;
                if (h % 5 == 0) c = detail;
                else if (h % 9 == 0) c = (Color){(unsigned char)(base.r + 20), (unsigned char)(base.g + 15), (unsigned char)(base.b + 10), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Cobblestone Slab: half-height with stone texture
    case BLOCK_COBBLESTONE_SLAB: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 27);
                bool bottomHalf = (y >= 8);
                if (!bottomHalf) { ImageDrawPixel(img, px + x, py + y, (Color){0,0,0,0}); continue; }
                Color c = (h % 2 == 0) ? base : detail;
                if (h % 7 == 0) c = (Color){(unsigned char)(base.r + 15), (unsigned char)(base.g + 10), (unsigned char)(base.b + 10), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Cactus: green block with spikes
    case BLOCK_CACTUS: {
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 55);
                Color c = base;
                bool spike = ((x == 0 || x == 15) && (h % 3 == 0)) ||
                             ((y == 0 || y == 15) && (x >= 6 && x <= 9 && h % 2 == 0));
                if (spike) c = (Color){(unsigned char)(base.r + 40), (unsigned char)(base.g + 30), (unsigned char)(base.b + 20), 255};
                else if (h % 7 == 0) c = detail;
                else if (h % 11 == 0) c = (Color){(unsigned char)(base.r + 10), (unsigned char)(base.g + 15), (unsigned char)(base.b + 5), 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        break;
    }

    // Sugar Cane: tall green stalk
    case BLOCK_SUGAR_CANE: {
        for (int y = 0; y < 16; y++) {
            unsigned int h = hash2D(worldX * 16 + 3, y, 77);
            ImageDrawPixel(img, px + 6, py + y, base);
            ImageDrawPixel(img, px + 7, py + y, detail);
            ImageDrawPixel(img, px + 8, py + y, (Color){(unsigned char)(base.r - 10), (unsigned char)(base.g + 5), (unsigned char)(base.b - 5), 255});
            if (h % 3 == 0) ImageDrawPixel(img, px + 9, py + y - 1, (Color){(unsigned char)(base.r + 20), (unsigned char)(base.g + 15), (unsigned char)(base.b + 5), 255});
        }
        for (int y = 3; y < 13; y += 4)
            for (int x = 3; x < 12; x++) {
                unsigned int h = hash2D(x, y + worldY * 16, 33);
                if (h % 2 == 0) ImageDrawPixel(img, px + x, py + y, (Color){(unsigned char)(base.r + 15), (unsigned char)(base.g + 10), (unsigned char)(base.b + 5), 255});
            }
        break;
    }

    // Generic item rendering for any remaining items
    default:
        for (int y = 0; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                unsigned int h = hash2D(x + worldX * 16, y + worldY * 16, 99);
                Color c = base;
                if (h % 6 == 0) c = detail;
                ImageDrawPixel(img, px + x, py + y, c);
            }
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

    // Generate crack overlay textures (10 stages)
    for (int stage = 0; stage < CRACK_STAGES; stage++) {
        Image crack = GenImageColor(BLOCK_SIZE, BLOCK_SIZE, BLANK);
        float intensity = (float)(stage + 1) / CRACK_STAGES;
        // Seed crack lines based on stage
        for (int i = 0; i < 3 + stage * 2; i++) {
            int sx = (hash2D(i, stage, 900) % BLOCK_SIZE);
            int sy = (hash2D(i, stage, 901) % BLOCK_SIZE);
            int len = 3 + (int)(intensity * 8);
            unsigned char a = (unsigned char)(80 + intensity * 120);
            for (int j = 0; j < len; j++) {
                int ex = sx + (hash2D(i + j, stage, 902) % 5) - 2;
                int ey = sy + (hash2D(i + j, stage, 903) % 5) - 2;
                if (ex >= 0 && ex < BLOCK_SIZE && ey >= 0 && ey < BLOCK_SIZE) {
                    ImageDrawPixel(&crack, ex, ey, (Color){0, 0, 0, a});
                    // Wider cracks at later stages
                    if (stage >= 5 && ex + 1 < BLOCK_SIZE)
                        ImageDrawPixel(&crack, ex + 1, ey, (Color){0, 0, 0, (unsigned char)(a * 0.6f)});
                    if (stage >= 7 && ey + 1 < BLOCK_SIZE)
                        ImageDrawPixel(&crack, ex, ey + 1, (Color){0, 0, 0, (unsigned char)(a * 0.4f)});
                }
                sx = ex; sy = ey;
            }
        }
        crackTextures[stage] = LoadTextureFromImage(crack);
        UnloadImage(crack);
    }
}

//----------------------------------------------------------------------------------
// Forward declarations for water level (defined in Water Flow System below)
//----------------------------------------------------------------------------------
static uint8_t waterLevel[WORLD_WIDTH][WORLD_HEIGHT];

//----------------------------------------------------------------------------------
// Lava Flow System
//----------------------------------------------------------------------------------
#define LAVA_MAX_LEVEL      5
#define WATER_MAX_LEVEL     7
#define FLUID_QUEUE_CAP     8192
typedef struct { uint16_t x, y; uint8_t type, level; } FluidNode;
static FluidNode fluidQueue[FLUID_QUEUE_CAP];
static int fluidQueueHead = 0, fluidQueueTail = 0, fluidQueueCount = 0;
static float fluidWaterTimer = 0.0f, fluidLavaTimer = 0.0f;
static uint8_t lavaLevel[WORLD_WIDTH][WORLD_HEIGHT];
static bool waterSource[WORLD_WIDTH][WORLD_HEIGHT];
static bool lavaSource[WORLD_WIDTH][WORLD_HEIGHT];
#define MAX_FLUID_CHANGES 4096
static FluidChange fluidChanges[MAX_FLUID_CHANGES];
static int fluidChangeCount = 0;
static int16_t fluidChangeIndex[WORLD_WIDTH][WORLD_HEIGHT];
static void QueueFluid(int x, int y, int type, int level);

void InitLava(void)
{
    memset(lavaLevel, 0, sizeof(lavaLevel));
    memset(waterSource, 0, sizeof(waterSource));
    memset(lavaSource, 0, sizeof(lavaSource));
    memset(fluidChangeIndex, 0xFF, sizeof(fluidChangeIndex));
    fluidChangeCount = 0;
}

static void MarkFluidChange(int x, int y)
{
    uint8_t bt, kind, level; bool source;
    GetFluidState(x, y, &bt, &kind, &level, &source);
    int existing = fluidChangeIndex[x][y];
    if (existing >= 0 && existing < fluidChangeCount) {
        fluidChanges[existing] = (FluidChange){(uint16_t)x, (uint16_t)y, bt, kind, level, source};
        return;
    }
    if (fluidChangeCount >= MAX_FLUID_CHANGES) return;
    fluidChangeIndex[x][y] = (int16_t)fluidChangeCount;
    fluidChanges[fluidChangeCount++] = (FluidChange){(uint16_t)x, (uint16_t)y, bt, kind, level, source};
}

int GetFluidChangeCount(void) { return fluidChangeCount; }
bool GetFluidChange(int index, FluidChange *change)
{
    if (!change || index < 0 || index >= fluidChangeCount) return false;
    *change = fluidChanges[index];
    return true;
}
void ClearFluidChanges(void)
{
    for (int i = 0; i < fluidChangeCount; i++) {
        fluidChangeIndex[fluidChanges[i].x][fluidChanges[i].y] = -1;
    }
    fluidChangeCount = 0;
}
void QueueFluidSources(void)
{
    for (int x = 0; x < WORLD_WIDTH; x++) for (int y = 0; y < WORLD_HEIGHT; y++) {
        if (waterSource[x][y] && world[x][y] == BLOCK_WATER) QueueFluid(x, y, BLOCK_WATER, waterLevel[x][y]);
        if (lavaSource[x][y] && world[x][y] == BLOCK_LAVA) QueueFluid(x, y, BLOCK_LAVA, lavaLevel[x][y]);
    }
}
void GetFluidState(int bx, int by, uint8_t *blockType, uint8_t *kind, uint8_t *level, bool *source)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    uint8_t block = world[bx][by];
    if (blockType) *blockType = block;
    if (kind) *kind = block == BLOCK_LAVA ? BLOCK_LAVA : (block == BLOCK_WATER ? BLOCK_WATER : 0);
    if (level) *level = block == BLOCK_LAVA ? lavaLevel[bx][by] : (block == BLOCK_WATER ? waterLevel[bx][by] : 0);
    if (source) *source = block == BLOCK_LAVA ? lavaSource[bx][by] : (block == BLOCK_WATER ? waterSource[bx][by] : false);
}

void ClearFluidStateAt(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    waterLevel[bx][by] = lavaLevel[bx][by] = 0;
    waterSource[bx][by] = lavaSource[bx][by] = false;
}

void RestoreFluidState(int bx, int by, uint8_t blockType, uint8_t kind, uint8_t level, bool source)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    world[bx][by] = blockType;
    waterLevel[bx][by] = lavaLevel[bx][by] = 0;
    waterSource[bx][by] = lavaSource[bx][by] = false;
    if (kind == BLOCK_WATER && blockType == BLOCK_WATER) {
        waterLevel[bx][by] = level; waterSource[bx][by] = source;
    } else if (kind == BLOCK_LAVA && blockType == BLOCK_LAVA) {
        lavaLevel[bx][by] = level; lavaSource[bx][by] = source;
    }
}
int GetLavaLevel(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return 0;
    if (world[bx][by] != BLOCK_LAVA) return 0;
    return lavaLevel[bx][by];
}

static void QueueFluid(int x, int y, int type, int level)
{
    if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT || level <= 0) return;
    if (fluidQueueCount >= FLUID_QUEUE_CAP) return;
    FluidNode *n = &fluidQueue[fluidQueueTail];
    n->x = (uint16_t)x; n->y = (uint16_t)y; n->type = (uint8_t)type; n->level = (uint8_t)level;
    fluidQueueTail = (fluidQueueTail + 1) % FLUID_QUEUE_CAP;
    fluidQueueCount++;
}

static void ProcessFluidNode(FluidNode node)
{
    int x = node.x, y = node.y, level = node.level;
    uint8_t fluid = node.type == BLOCK_LAVA ? BLOCK_LAVA : BLOCK_WATER;
    if (world[x][y] != fluid || level <= 1) return;
    int down = y + 1;
    if (down < WORLD_HEIGHT) {
        if (world[x][down] == BLOCK_AIR) {
            InvalidateChunkAt(x, down);
            MarkFluidChange(x, down);
            QueueFluid(x, down, fluid, level);
        } else if (world[x][down] == (fluid == BLOCK_LAVA ? BLOCK_WATER : BLOCK_LAVA)) {
            world[x][down] = fluid == BLOCK_LAVA ? BLOCK_OBSIDIAN : BLOCK_COBBLESTONE;
            waterLevel[x][down] = 0; lavaLevel[x][down] = 0;
            waterSource[x][down] = false; lavaSource[x][down] = false;
            MarkFluidChange(x, down);
            InvalidateChunkAt(x, down);
        }
    }
    int next = level - 1;
    if (next <= 0) return;
    static const int dirs[2] = { -1, 1 };
    for (int i = 0; i < 2; i++) {
        int nx = x + dirs[i];
        if (nx < 0 || nx >= WORLD_WIDTH) continue;
        if (world[nx][y] == BLOCK_AIR && (y + 1 >= WORLD_HEIGHT || world[nx][y + 1] != BLOCK_AIR)) {
            InvalidateChunkAt(nx, y);
            MarkFluidChange(nx, y);
            QueueFluid(nx, y, fluid, next);
        } else if (world[nx][y] == (fluid == BLOCK_LAVA ? BLOCK_WATER : BLOCK_LAVA)) {
            world[nx][y] = fluid == BLOCK_LAVA ? BLOCK_COBBLESTONE : BLOCK_OBSIDIAN;
            waterLevel[nx][y] = 0; lavaLevel[nx][y] = 0;
            waterSource[nx][y] = false; lavaSource[nx][y] = false;
            MarkFluidChange(nx, y);
            InvalidateChunkAt(nx, y);
        }
    }
}

void UpdateFluidTick(float dt)
{
    fluidWaterTimer += dt;
    fluidLavaTimer += dt;
    bool waterReady = fluidWaterTimer >= 0.18f;
    bool lavaReady = fluidLavaTimer >= 0.55f;
    if (!waterReady && !lavaReady) return;
    if (waterReady) fluidWaterTimer = 0.0f;
    if (lavaReady) fluidLavaTimer = 0.0f;
    int budget = 96;
    int scanned = fluidQueueCount;
    while (fluidQueueCount > 0 && budget-- > 0 && scanned-- > 0) {
        FluidNode node = fluidQueue[fluidQueueHead];
        fluidQueueHead = (fluidQueueHead + 1) % FLUID_QUEUE_CAP;
        fluidQueueCount--;
        if ((node.type == BLOCK_LAVA && !lavaReady) || (node.type == BLOCK_WATER && !waterReady)) {
            QueueFluid(node.x, node.y, node.type, node.level);
            continue;
        }
        ProcessFluidNode(node);
    }
}

void SetLavaSource(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    // Check for water contact 鈫?obsidian
    static const int dx[] = {0, 0, 1, -1};
    static const int dy[] = {1, -1, 0, 0};
    for (int d = 0; d < 4; d++) {
        int nx = bx + dx[d], ny = by + dy[d];
        if (nx >= 0 && nx < WORLD_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
            if (world[nx][ny] == BLOCK_WATER) {
                world[bx][by] = BLOCK_OBSIDIAN;
                waterLevel[bx][by] = 0;
                InvalidateChunkAt(bx, by);
                return;
            }
        }
    }
    world[bx][by] = BLOCK_LAVA;
    lavaLevel[bx][by] = LAVA_MAX_LEVEL;
    lavaSource[bx][by] = true;
    QueueFluid(bx, by, BLOCK_LAVA, LAVA_MAX_LEVEL);
    MarkFluidChange(bx, by);
}

void RemoveLavaAt(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    if (world[bx][by] != BLOCK_LAVA) return;
    world[bx][by] = BLOCK_AIR;
    lavaLevel[bx][by] = 0;
    lavaSource[bx][by] = false;
    MarkFluidChange(bx, by);
    InvalidateChunkAt(bx, by);
    int radius = LAVA_MAX_LEVEL + 1;
    int minX = bx - radius, maxX = bx + radius;
    int minY = by - radius, maxY = by + radius;
    if (minX < 0) minX = 0; if (maxX >= WORLD_WIDTH) maxX = WORLD_WIDTH - 1;
    if (minY < 0) minY = 0; if (maxY >= WORLD_HEIGHT) maxY = WORLD_HEIGHT - 1;
    for (int x = minX; x <= maxX; x++) for (int y = minY; y <= maxY; y++) {
        if (world[x][y] == BLOCK_LAVA) {
            bool dynamic = lavaSource[x][y] || lavaLevel[x][y] > 0;
            lavaLevel[x][y] = 0;
            if (dynamic && !lavaSource[x][y]) {
                world[x][y] = BLOCK_AIR;
                MarkFluidChange(x, y);
            }
        }
    }
    for (int x = minX; x <= maxX; x++) for (int y = minY; y <= maxY; y++) {
        if (lavaSource[x][y] && world[x][y] == BLOCK_LAVA) {
            lavaLevel[x][y] = LAVA_MAX_LEVEL;
            QueueFluid(x, y, BLOCK_LAVA, LAVA_MAX_LEVEL);
        }
    }
}

//----------------------------------------------------------------------------------
// Gravity System
//----------------------------------------------------------------------------------
bool IsGravityBlock(uint8_t block)
{
    return block == BLOCK_SAND || block == BLOCK_GRAVEL || block == BLOCK_MUD;
}

void ApplyGravityAt(int bx, int by)
{
    for (int fy = by - 1; fy >= 0; fy--) {
        uint8_t above = world[bx][fy];
        if (above == BLOCK_AIR || above == BLOCK_WATER) break;
        if (!IsGravityBlock(above)) break;
        world[bx][fy] = BLOCK_AIR;
        int landY = fy + 1;
        // Fall through air and water
        while (landY < WORLD_HEIGHT && (world[bx][landY] == BLOCK_AIR || world[bx][landY] == BLOCK_WATER)) landY++;
        landY--;
        world[bx][landY] = above;
        InvalidateChunkAt(bx, fy);
        InvalidateChunkAt(bx, landY);
        NetSyncBlockChange(bx, fy, BLOCK_AIR);
        NetSyncBlockChange(bx, landY, above);
        SpawnBlockParticles(bx, landY, (BlockType)above);
        PlaySoundLand();
    }
}

//----------------------------------------------------------------------------------
// Water Flow System
//----------------------------------------------------------------------------------
#define WATER_MAX_LEVEL     7
// waterLevel declared above (forward declaration for lava system)

void InitWater(void)
{
    memset(waterLevel, 0, sizeof(waterLevel));
    memset(waterSource, 0, sizeof(waterSource));
}

int GetWaterLevel(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return 0;
    if (world[bx][by] != BLOCK_WATER) return 0;
    return waterLevel[bx][by];
}

static void PropagateWaterBFS(int startX, int startY, int level)
{
    if (level <= 0) return;
    // BFS queue
    static int qx[WORLD_WIDTH * 2];
    static int qy[WORLD_WIDTH * 2];
    static int ql[WORLD_WIDTH * 2];
    int head = 0, tail = 0;

    waterLevel[startX][startY] = level;
    qx[tail] = startX; qy[tail] = startY; ql[tail] = level; tail++;

    while (head != tail) {
        int cx = qx[head], cy = qy[head], cl = ql[head];
        head = (head + 1) % (WORLD_WIDTH * 2);
        if (cl <= 1) continue;

        // Flow down first (full level)
        if (cy + 1 < WORLD_HEIGHT && world[cx][cy + 1] == BLOCK_AIR) {
            world[cx][cy + 1] = BLOCK_WATER;
            waterLevel[cx][cy + 1] = WATER_MAX_LEVEL;
            int next = tail % (WORLD_WIDTH * 2);
            qx[next] = cx; qy[next] = cy + 1; ql[next] = WATER_MAX_LEVEL;
            tail++;
        }

        // Flow sideways (level - 1)
        int nextL = cl - 1;
        if (nextL > 0) {
            static const int dx[] = {1, -1};
            for (int d = 0; d < 2; d++) {
                int nx = cx + dx[d];
                if (nx < 0 || nx >= WORLD_WIDTH) continue;
                if (world[nx][cy] == BLOCK_AIR && (cy + 1 >= WORLD_HEIGHT || world[nx][cy + 1] != BLOCK_AIR)) {
                    world[nx][cy] = BLOCK_WATER;
                    waterLevel[nx][cy] = (uint8_t)nextL;
                    int next = tail % (WORLD_WIDTH * 2);
                    qx[next] = nx; qy[next] = cy; ql[next] = nextL;
                    tail++;
                }
            }
        }
    }
}

void SetWaterSource(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    world[bx][by] = BLOCK_WATER;
    waterLevel[bx][by] = WATER_MAX_LEVEL;
    waterSource[bx][by] = true;
    QueueFluid(bx, by, BLOCK_WATER, WATER_MAX_LEVEL);
    MarkFluidChange(bx, by);
}

void RemoveWaterAt(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    if (world[bx][by] != BLOCK_WATER) return;

    // Clear this water block
    world[bx][by] = BLOCK_AIR;
    waterLevel[bx][by] = 0;
    waterSource[bx][by] = false;
    MarkFluidChange(bx, by);
    InvalidateChunkAt(bx, by);

    // Recalculate water in affected area (radius = WATER_MAX_LEVEL)
    int radius = WATER_MAX_LEVEL + 1;
    int minX = bx - radius, maxX = bx + radius;
    int minY = by - radius, maxY = by + radius;
    if (minX < 0) minX = 0;
    if (maxX >= WORLD_WIDTH) maxX = WORLD_WIDTH - 1;
    if (minY < 0) minY = 0;
    if (maxY >= WORLD_HEIGHT) maxY = WORLD_HEIGHT - 1;

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            if (world[x][y] == BLOCK_WATER) {
                bool dynamic = waterSource[x][y] || waterLevel[x][y] > 0;
                waterLevel[x][y] = 0;
                if (dynamic && !waterSource[x][y]) {
                    world[x][y] = BLOCK_AIR;
                    MarkFluidChange(x, y);
                }
            }
            if (world[x][y] == BLOCK_LAVA) {
                bool dynamic = lavaSource[x][y] || lavaLevel[x][y] > 0;
                lavaLevel[x][y] = 0;
                if (dynamic && !lavaSource[x][y]) {
                    world[x][y] = BLOCK_AIR;
                    MarkFluidChange(x, y);
                }
            }
        }
    }

    // Re-propagate only from explicit remaining sources; descendants are queued.
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            if (waterSource[x][y] && world[x][y] == BLOCK_WATER) {
                waterLevel[x][y] = WATER_MAX_LEVEL;
                QueueFluid(x, y, BLOCK_WATER, WATER_MAX_LEVEL);
            }
            if (lavaSource[x][y] && world[x][y] == BLOCK_LAVA) {
                lavaLevel[x][y] = LAVA_MAX_LEVEL;
                QueueFluid(x, y, BLOCK_LAVA, LAVA_MAX_LEVEL);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Redstone System
//----------------------------------------------------------------------------------
#define REDSTONE_MAX_POWER  15
#define REDSTONE_WIRE_MAX   15

static uint8_t redstonePower[WORLD_WIDTH][WORLD_HEIGHT];
static bool leverState[WORLD_WIDTH][WORLD_HEIGHT];
static uint8_t cropGrowth[WORLD_WIDTH][WORLD_HEIGHT]; // 0-7 growth stage for crops

// Pressure plate tracking 鈥?avoids O(524K) scan in UpdateRedstoneTick
#define MAX_PRESSURE_PLATES 256
static int pressurePlatesX[MAX_PRESSURE_PLATES];
static int pressurePlatesY[MAX_PRESSURE_PLATES];
static int pressurePlateCount;

#define MAX_CROP_CELLS 8192
static int cropCellsX[MAX_CROP_CELLS];
static int cropCellsY[MAX_CROP_CELLS];
static int cropCellCount;

void InitRedstone(void)
{
    memset(redstonePower, 0, sizeof(redstonePower));
    memset(leverState, 0, sizeof(leverState));
    memset(cropGrowth, 0, sizeof(cropGrowth));
    pressurePlateCount = 0;
    cropCellCount = 0;
}

void RegisterPressurePlate(int bx, int by)
{
    for (int i = 0; i < pressurePlateCount; i++) {
        if (pressurePlatesX[i] == bx && pressurePlatesY[i] == by) return;
    }
    if (pressurePlateCount < MAX_PRESSURE_PLATES) {
        pressurePlatesX[pressurePlateCount] = bx;
        pressurePlatesY[pressurePlateCount] = by;
        pressurePlateCount++;
    }
}

void UnregisterPressurePlate(int bx, int by)
{
    for (int i = 0; i < pressurePlateCount; i++) {
        if (pressurePlatesX[i] == bx && pressurePlatesY[i] == by) {
            pressurePlatesX[i] = pressurePlatesX[pressurePlateCount - 1];
            pressurePlatesY[i] = pressurePlatesY[pressurePlateCount - 1];
            pressurePlateCount--;
            return;
        }
    }
}

void RebuildPressurePlateList(void)
{
    pressurePlateCount = 0;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_STONE_PRESSURE_PLATE) {
                RegisterPressurePlate(x, y);
            }
        }
    }
}

// Crop cell tracking — UpdateCrops iterates this list instead of scanning the
// whole 2048x256 world every 5s. Cells are registered on creation (plant / world
// gen / load-rescan); stale cells (broken, exploded, flooded) are pruned lazily
// during the scan, so no explicit unregister is needed on every destruction path.
void RegisterCrop(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    for (int i = 0; i < cropCellCount; i++) {
        if (cropCellsX[i] == bx && cropCellsY[i] == by) return; // already tracked
    }
    if (cropCellCount < MAX_CROP_CELLS) {
        cropCellsX[cropCellCount] = bx;
        cropCellsY[cropCellCount] = by;
        cropCellCount++;
    }
}

void RebuildCropList(void)
{
    cropCellCount = 0;
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_CROPS && cropCellCount < MAX_CROP_CELLS) {
                cropCellsX[cropCellCount] = x;
                cropCellsY[cropCellCount] = y;
                cropCellCount++;
            }
        }
    }
}

bool IsLeverOn(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;
    return leverState[bx][by];
}

void ToggleLever(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    leverState[bx][by] = !leverState[bx][by];
    UpdateRedstoneAt(bx, by);
}


static bool IsRedstoneSource(uint8_t block, int bx, int by)
{
    if (block == BLOCK_LEVER) return leverState[bx][by];
    if (block == BLOCK_STONE_PRESSURE_PLATE) {
        // Check if player is standing on it
        float px = player.position.x;
        float py = player.position.y;
        float pRight = px + PLAYER_WIDTH;
        float pBottom = py + PLAYER_HEIGHT;
        float bLeft = bx * BLOCK_SIZE;
        float bRight = bLeft + BLOCK_SIZE;
        float bTop = by * BLOCK_SIZE;
        float bBottom = bTop + BLOCK_SIZE;
        return (pRight > bLeft && px < bRight && pBottom >= bTop && pBottom <= bBottom + 4);
    }
    return false;
}

static void PropagateRedstoneBFS(int startX, int startY, int power)
{
    // BFS queue
    static int qx[WORLD_WIDTH * 4];
    static int qy[WORLD_WIDTH * 4];
    static uint8_t qp[WORLD_WIDTH * 4];
    int head = 0, tail = 0;

    redstonePower[startX][startY] = power;
    qx[tail] = startX;
    qy[tail] = startY;
    qp[tail] = power;
    tail++;

    while (head != tail) {
        int cx = qx[head];
        int cy = qy[head];
        int cp = qp[head];
        head = (head + 1) % (WORLD_WIDTH * 4);

        // Spread to adjacent blocks
        static const int dx[] = {1, -1, 0, 0};
        static const int dy[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx < 0 || nx >= WORLD_WIDTH || ny < 0 || ny >= WORLD_HEIGHT) continue;

            uint8_t nblock = world[nx][ny];
            int newPower = cp - 1;
            if (newPower <= 0) continue;

            // Wire carries signal
            if (nblock == BLOCK_REDSTONE_WIRE) {
                if (newPower > redstonePower[nx][ny]) {
                    redstonePower[nx][ny] = (uint8_t)newPower;
                    int next = tail % (WORLD_WIDTH * 4);
                    qx[next] = nx;
                    qy[next] = ny;
                    qp[next] = (uint8_t)newPower;
                    tail++;
                }
            }
            // Lamp receives signal
            else if (nblock == BLOCK_REDSTONE_LAMP) {
                if (newPower > redstonePower[nx][ny]) {
                    redstonePower[nx][ny] = (uint8_t)newPower;
                }
            }
            // TNT ignites when a redstone signal reaches it (terminal, like a lamp).
            // PrimeTnt() dedups, so repeated propagation won't re-prime it.
            else if (nblock == BLOCK_TNT) {
                redstonePower[nx][ny] = (uint8_t)newPower;
                PrimeTnt(nx, ny);
            }
        }
    }
}

void UpdateRedstoneAt(int bx, int by)
{
    // Clear all power in affected area (radius 16)
    int minX = bx - REDSTONE_MAX_POWER - 1;
    int maxX = bx + REDSTONE_MAX_POWER + 1;
    int minY = by - REDSTONE_MAX_POWER - 1;
    int maxY = by + REDSTONE_MAX_POWER + 1;
    if (minX < 0) minX = 0;
    if (maxX >= WORLD_WIDTH) maxX = WORLD_WIDTH - 1;
    if (minY < 0) minY = 0;
    if (maxY >= WORLD_HEIGHT) maxY = WORLD_HEIGHT - 1;

    for (int x = minX; x <= maxX; x++)
        for (int y = minY; y <= maxY; y++)
            redstonePower[x][y] = 0;

    // Find all power sources in the affected area and propagate
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            uint8_t block = world[x][y];
            if (IsRedstoneSource(block, x, y)) {
                PropagateRedstoneBFS(x, y, REDSTONE_MAX_POWER);
            }
        }
    }

    // Update light for lamps in the area
    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            if (world[x][y] == BLOCK_REDSTONE_LAMP) {
                UpdateLightAt(x, y);
                InvalidateChunkAt(x, y);
            }
        }
    }
}

void UpdateRedstoneTick(void)
{
    // Update pressure plate states (player position may have changed)
    for (int i = 0; i < pressurePlateCount; i++) {
        int x = pressurePlatesX[i];
        int y = pressurePlatesY[i];
        bool wasPowered = redstonePower[x][y] > 0;
        bool nowPowered = IsRedstoneSource(BLOCK_STONE_PRESSURE_PLATE, x, y);
        if (wasPowered != nowPowered) {
            UpdateRedstoneAt(x, y);
        }
    }
}

int GetRedstonePowerAt(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return 0;
    return redstonePower[bx][by];
}

bool IsRedstoneLampPowered(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;
    return world[bx][by] == BLOCK_REDSTONE_LAMP && redstonePower[bx][by] > 0;
}

//----------------------------------------------------------------------------------
// Crop Growth System
//----------------------------------------------------------------------------------
static float cropGrowTimer = 0.0f;

void UpdateCrops(float dt)
{
    cropGrowTimer += dt;
    if (cropGrowTimer < 5.0f) return; // Grow every 5 seconds
    cropGrowTimer = 0.0f;

    bool grew = false;
    for (int idx = 0; idx < cropCellCount; idx++) {
        int x = cropCellsX[idx];
        int y = cropCellsY[idx];

        // Lazy prune: cell is no longer a crop (broken / exploded / flooded) or
        // out of the growable range -> swap-remove and re-check the swapped entry.
        if (x < 0 || x >= WORLD_WIDTH || y < 1 || y >= WORLD_HEIGHT - 1 ||
            world[x][y] != BLOCK_CROPS) {
            cropCellsX[idx] = cropCellsX[cropCellCount - 1];
            cropCellsY[idx] = cropCellsY[cropCellCount - 1];
            cropCellCount--;
            idx--;
            continue;
        }

        if (cropGrowth[x][y] >= 7) continue;        // Already mature (kept in list)
        if (world[x][y - 1] != BLOCK_FARMLAND) continue;

        // Check for water nearby (within 4 blocks)
        bool hasWater = false;
        for (int dx = -4; dx <= 4 && !hasWater; dx++) {
            for (int dy = -4; dy <= 4 && !hasWater; dy++) {
                int nx = x + dx, ny = y + dy;
                if (nx >= 0 && nx < WORLD_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                    if (world[nx][ny] == BLOCK_WATER) hasWater = true;
                }
            }
        }

        // Check for light above
        uint8_t light = GetLightLevel(x, y - 1);
        if (light < 8) continue;

        // Growth chance: base 30%, +30% if near water
        int chance = 30;
        if (hasWater) chance += 30;

        if (rand() % 100 < chance) {
            cropGrowth[x][y]++;
            // Crops get taller with growth; mark the chunk so its baked
            // texture is regenerated with the new stage (see DrawBlockPattern).
            InvalidateChunkAt(x, y);
            grew = true;
        }
    }

    // UpdateCrops() runs after UpdateChunks() in the frame, so rebuild the
    // invalidated chunks now — otherwise DrawWorld would skip them for one
    // frame and the crop chunk would flicker. UpdateChunks() is idempotent.
    if (grew) UpdateChunks();
}

//----------------------------------------------------------------------------------
// TNT Explosives
//   Primed TNT blocks count down a fuse, then ExplodeAt(): destroy blocks in a
//   radius, damage the player and mobs by distance falloff, and chain-detonate any
//   TNT caught in the blast. ExplodeAt is intentionally separate from the creeper's
//   inline explosion so this feature can't regress creeper behaviour.
//----------------------------------------------------------------------------------
#define MAX_PRIMED_TNT      64
#define TNT_FUSE_TIME       2.0f
#define TNT_EXPLODE_RADIUS  4
#define TNT_DAMAGE          34
static int   primedTntX[MAX_PRIMED_TNT];
static int   primedTntY[MAX_PRIMED_TNT];
static float primedTntFuse[MAX_PRIMED_TNT];
static int   primedTntCount;

void InitPrimedTnt(void)
{
    primedTntCount = 0;
}

void PrimeTnt(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    if (world[bx][by] != BLOCK_TNT) return;
    for (int i = 0; i < primedTntCount; i++)
        if (primedTntX[i] == bx && primedTntY[i] == by) return; // already counting down
    if (primedTntCount >= MAX_PRIMED_TNT) return;
    primedTntX[primedTntCount] = bx;
    primedTntY[primedTntCount] = by;
    primedTntFuse[primedTntCount] = TNT_FUSE_TIME;
    primedTntCount++;
    PlaySoundCreeperFuse();
}

void ExplodeAt(float worldX, float worldY, int radius)
{
    int cx = (int)(worldX) / BLOCK_SIZE;
    int cy = (int)(worldY) / BLOCK_SIZE;
    float blastPix = radius * BLOCK_SIZE * 1.2f;

    // Destroy blocks within the radius; chain-prime any TNT caught in the blast.
    for (int bx = cx - radius; bx <= cx + radius; bx++) {
        for (int by = cy - radius; by <= cy + radius; by++) {
            if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) continue;
            float ddx = (float)((bx - cx) * BLOCK_SIZE);
            float ddy = (float)((by - cy) * BLOCK_SIZE);
            float rPix = (float)(radius * BLOCK_SIZE);
            if (ddx * ddx + ddy * ddy > rPix * rPix) continue;
            BlockType bt = (BlockType)world[bx][by];
            if (bt == BLOCK_AIR || bt == BLOCK_BEDROCK) continue;
            if (bt == BLOCK_TNT) { PrimeTnt(bx, by); continue; } // chain reaction
            SpawnBlockParticles(bx, by, bt);
            ClearFluidStateAt(bx, by);
            world[bx][by] = BLOCK_AIR;
            NetSyncBlockChange(bx, by, BLOCK_AIR);
            InvalidateChunkAt(bx, by);
            UpdateLightAt(bx, by);
        }
    }
    for (int bx = cx - radius; bx <= cx + radius; bx++)
        if (bx >= 0 && bx < WORLD_WIDTH) ApplyGravityAt(bx, cy + radius);

    // Damage the player with distance falloff + knockback.
    float pcx = player.position.x + PLAYER_WIDTH / 2.0f;
    float pcy = player.position.y + PLAYER_HEIGHT / 2.0f;
    float pdx = pcx - worldX, pdy = pcy - worldY;
    float pdist = sqrtf(pdx * pdx + pdy * pdy);
    if (pdist < blastPix) {
        int dmg = (int)(TNT_DAMAGE * (1.0f - pdist / blastPix));
        if (gameDifficulty == DIFFICULTY_EASY) dmg = dmg * 3 / 4;
        else if (gameDifficulty == DIFFICULTY_HARD) dmg = dmg * 3 / 2;
        dmg = (int)(dmg * (1.0f - GetArmorDamageReduction()));
        if (dmg > 0 && gameMode != GAME_CREATIVE) { // Creative: invincible
            player.health -= dmg;
            if (player.health < 0) player.health = 0;
            if (player.health <= 0) SetDeathCause(STR_DEATH_MOB_CREEPER);
            DamageArmor();
            player.damageFlashTimer = 0.5f;
            player.knockbackTimer = 0.3f;
            player.velocity.x = (pdx < 0 ? 1.0f : -1.0f) * 300.0f;
            player.velocity.y = -250.0f;
            PlaySoundHurt();
        }
    }

    // Damage nearby mobs.
    for (int m = 0; m < MAX_MOBS; m++) {
        if (!mobs[m].active || mobs[m].deathTimer > 0) continue;
        float mcx = mobs[m].position.x + GetMobWidth(mobs[m].type) / 2.0f;
        float mcy = mobs[m].position.y + GetMobHeight(mobs[m].type) / 2.0f;
        float mdx = mcx - worldX, mdy = mcy - worldY;
        float md = sqrtf(mdx * mdx + mdy * mdy);
        if (md < blastPix) {
            int dmg = (int)(TNT_DAMAGE * (1.0f - md / blastPix));
            if (dmg > 0) DamageMob(&mobs[m], dmg);
        }
    }

    // Visual flash + boom.
    for (int p = 0; p < 24; p++) {
        float angle = (float)(rand() % 628) / 100.0f;
        float pd = 4.0f + (float)(rand() % (radius * 6));
        SpawnDamageParticles(worldX + cosf(angle) * pd, worldY + sinf(angle) * pd,
                             (Color){255, 150, 50, 255});
    }
    TriggerCameraShake(11.0f, 0.6f);
    PlaySoundThunder();
}

void UpdatePrimedTnt(float dt)
{
    for (int i = 0; i < primedTntCount; i++) {
        primedTntFuse[i] -= dt;
        if (primedTntFuse[i] > 0.0f) continue;
        int bx = primedTntX[i], by = primedTntY[i];
        // Clear the block first so the blast doesn't re-prime itself.
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT && world[bx][by] == BLOCK_TNT) {
            world[bx][by] = BLOCK_AIR;
            NetSyncBlockChange(bx, by, BLOCK_AIR);
            InvalidateChunkAt(bx, by);
            UpdateLightAt(bx, by);
        }
        ExplodeAt(bx * BLOCK_SIZE + BLOCK_SIZE / 2.0f, by * BLOCK_SIZE + BLOCK_SIZE / 2.0f, TNT_EXPLODE_RADIUS);
        UnlockAchievement(ACH_DEMOLITION);
        // swap-remove this entry and re-check the swapped-in one
        primedTntX[i] = primedTntX[primedTntCount - 1];
        primedTntY[i] = primedTntY[primedTntCount - 1];
        primedTntFuse[i] = primedTntFuse[primedTntCount - 1];
        primedTntCount--;
        i--;
    }
}

int GetCropGrowth(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return 0;
    return cropGrowth[bx][by];
}

void SetCropGrowth(int bx, int by, int stage)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return;
    cropGrowth[bx][by] = (uint8_t)stage;
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
        // 0=plains, 1=desert, 2=forest, 3=tundra, 4=swamp, 5=jungle, 6=taiga
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0;
        if (biomeNoise > 0.55f) biome = 1;       // desert
        else if (biomeNoise > 0.35f) biome = 6;   // taiga
        else if (biomeNoise > 0.15f) biome = 0;   // plains
        else if (biomeNoise > -0.05f) biome = 4;  // swamp
        else if (biomeNoise > -0.25f) biome = 2;  // forest
        else if (biomeNoise > -0.45f) biome = 5;  // jungle
        else biome = 3;                            // tundra

        // --- Calculate surface Y ---
        // Terrain is lower Y = higher on screen
        int surfaceY = TERRAIN_BASE;
        // Apply base noise (gentle continent shape)
        surfaceY += (int)(base * 30.0f * continental);
        // Apply hills (biome-dependent amplitude)
        float hillAmp;
        switch (biome) {
            case 1: hillAmp = 12.0f; break;  // desert: flat
            case 2: hillAmp = 28.0f; break;  // forest: hilly
            case 3: hillAmp = 15.0f; break;  // tundra: gentle
            case 4: hillAmp = 8.0f; break;   // swamp: very flat
            case 5: hillAmp = 32.0f; break;  // jungle: very hilly
            case 6: hillAmp = 20.0f; break;  // taiga: moderate
            default: hillAmp = 22.0f; break; // plains
        }
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
                else if (biome == 3) world[x][y] = BLOCK_SNOWY_GRASS; // tundra
                else if (biome == 4) world[x][y] = BLOCK_MUD;      // swamp
                else if (biome == 6) world[x][y] = BLOCK_SNOWY_GRASS; // taiga
                else world[x][y] = BLOCK_GRASS;                     // plains/forest/jungle
            } else if (y < surfaceY + 4) {
                if (isStonePeak) world[x][y] = BLOCK_STONE;        // mountain subsurface
                else if (biome == 1) world[x][y] = BLOCK_SAND;     // desert sand layers
                else if (biome == 4) world[x][y] = BLOCK_MUD;      // swamp mud layers
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
            // Gold: deep, rarer than iron
            if (y > 180) {
                float goldThreshold = 0.78f - (y - 180) * 0.0003f;
                float gold = fbm(x * 0.14f, y * 0.14f, 2, 0.5f, seed + 5000);
                if (gold > goldThreshold) { world[x][y] = BLOCK_GOLD_ORE; continue; }
            }
            // Diamond: very deep, rarest
            if (y > 210) {
                float diamondThreshold = 0.82f - (y - 210) * 0.0002f;
                float diamond = fbm(x * 0.16f, y * 0.16f, 2, 0.5f, seed + 6000);
                if (diamond > diamondThreshold) { world[x][y] = BLOCK_DIAMOND_ORE; continue; }
            }
            // Redstone: deep, similar to diamond
            if (y > 190) {
                float redstoneThreshold = 0.80f - (y - 190) * 0.0003f;
                float redstone = fbm(x * 0.15f, y * 0.15f, 2, 0.5f, seed + 7000);
                if (redstone > redstoneThreshold) { world[x][y] = BLOCK_REDSTONE_ORE; continue; }
            }
            // Lapis: medium depth, moderate rarity
            if (y > 160) {
                float lapisThreshold = 0.77f - (y - 160) * 0.0004f;
                float lapis = fbm(x * 0.13f, y * 0.13f, 2, 0.5f, seed + 8000);
                if (lapis > lapisThreshold) { world[x][y] = BLOCK_LAPIS_ORE; continue; }
            }
        }
    }

    // ============================================================
    // Pass 5: Sand near sea level (for non-desert biomes)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        bool isDesert = biomeNoise > 0.55f;
        bool isTundra = biomeNoise < -0.45f;
        if (isDesert || isTundra) continue; // desert has sand, tundra has ice/snow
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
    // Pass 9: Underground Dungeons
    // ============================================================
    for (int d = 0; d < WORLD_WIDTH / 150; d++) {
        // Try multiple positions per dungeon for better placement
        int placed = 0;
        for (int attempt = 0; attempt < 5 && !placed; attempt++) {
            int dx = hash2D(d, attempt, seed + 11000) % (WORLD_WIDTH - 20) + 10;
            int dy = CAVE_START + 10 + hash2D(d, attempt + 10, seed + 11100) % (CAVE_END - CAVE_START - 20);

            // Variable room size: 5x5 to 9x9
            int halfW = 2 + hash2D(d, attempt + 20, seed + 11200) % 3; // 2,3,4 -> 5,7,9 wide
            int halfH = 2 + hash2D(d, attempt + 30, seed + 11250) % 3;

            // Check if the area is mostly stone
            int solidCount = 0, totalCheck = 0;
            for (int tx = dx - halfW - 1; tx <= dx + halfW + 1; tx++) {
                for (int ty = dy - halfH - 1; ty <= dy + halfH + 1; ty++) {
                    if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                        totalCheck++;
                        if (world[tx][ty] == BLOCK_STONE) solidCount++;
                    }
                }
            }
            if (solidCount < totalCheck * 7 / 10) continue;

            // Build room
            bool useMossy = hash2D(dx, dy, seed + 11300) % 2 == 0;
            for (int tx = dx - halfW; tx <= dx + halfW; tx++) {
                for (int ty = dy - halfH; ty <= dy + halfH; ty++) {
                    if (tx < 0 || tx >= WORLD_WIDTH || ty < 0 || ty >= WORLD_HEIGHT) continue;
                    if (tx == dx - halfW || tx == dx + halfW || ty == dy - halfH || ty == dy + halfH) {
                        world[tx][ty] = useMossy ? BLOCK_MOSSY_COBBLESTONE : BLOCK_COBBLESTONE;
                    } else {
                        world[tx][ty] = BLOCK_AIR;
                        // Floor decoration: some mossy cobblestone
                        if (ty == dy + halfH - 1 && hash2D(tx, ty, seed + 11350) % 5 == 0) {
                            world[tx][ty] = BLOCK_MOSSY_COBBLESTONE;
                        }
                    }
                }
            }

            // Entrance: dig a corridor from the wall to the nearest cave/air
            int side = hash2D(dx, dy, seed + 11400) % 4;
            int ex = dx, ey = dy;
            if (side == 0) ey = dy - halfH;      // North
            else if (side == 1) ey = dy + halfH;  // South
            else if (side == 2) ex = dx - halfW;  // West
            else ex = dx + halfW;                  // East

            // Clear entrance gap
            for (int i = -1; i <= 1; i++) {
                if (side <= 1) world[dx + i][ey] = BLOCK_AIR;
                else world[ex][dy + i] = BLOCK_AIR;
            }

            // Dig corridor outward until we hit air or max 15 blocks
            int cx = ex, cy = ey;
            int dxDir = (side == 2) ? -1 : (side == 3) ? 1 : 0;
            int dyDir = (side == 0) ? -1 : (side == 1) ? 1 : 0;
            for (int step = 0; step < 15; step++) {
                cx += dxDir;
                cy += dyDir;
                if (cx < 0 || cx >= WORLD_WIDTH || cy < 0 || cy >= WORLD_HEIGHT) break;
                if (world[cx][cy] == BLOCK_AIR || world[cx][cy] == BLOCK_WATER) break; // Connected!
                // Carve 2-wide corridor
                for (int i = -1; i <= 1; i++) {
                    int rx = cx + (dyDir != 0 ? i : 0);
                    int ry = cy + (dxDir != 0 ? i : 0);
                    if (rx >= 0 && rx < WORLD_WIDTH && ry >= 0 && ry < WORLD_HEIGHT) {
                        world[rx][ry] = BLOCK_AIR;
                    }
                }
            }

            // Place chest in center
            if (chestCount < MAX_CHESTS) {
                world[dx][dy] = BLOCK_CHEST;
                ChestData *c = &chestData[chestCount];
                c->x = dx;
                c->y = dy;
                memset(c->items, 0, sizeof(c->items));
                memset(c->counts, 0, sizeof(c->counts));

                typedef struct { uint8_t item; int minCount; int maxCount; int weight; } LootEntry;
                LootEntry loot[] = {
                    {ITEM_IRON_INGOT, 1, 3, 20},
                    {ITEM_GOLD_INGOT, 1, 2, 15},
                    {ITEM_COAL, 3, 8, 25},
                    {ITEM_DIAMOND, 1, 1, 5},
                    {FOOD_BREAD, 2, 4, 20},
                    {BLOCK_TORCH, 4, 8, 25},
                    {ITEM_BONE, 2, 5, 20},
                    {ITEM_STRING, 2, 4, 15},
                    {ITEM_ARROW, 4, 8, 15},
                    {ITEM_BOW, 1, 1, 5},
                };
                int lootCount = sizeof(loot) / sizeof(loot[0]);

                int slots = 3 + hash2D(dx, dy, seed + 11500) % 4;
                for (int s = 0; s < slots && s < CHEST_SLOTS; s++) {
                    int totalWeight = 0;
                    for (int l = 0; l < lootCount; l++) totalWeight += loot[l].weight;
                    int roll = hash2D(dx + s, dy, seed + 11600 + s) % totalWeight;
                    int chosen = 0;
                    for (int l = 0; l < lootCount; l++) {
                        roll -= loot[l].weight;
                        if (roll < 0) { chosen = l; break; }
                    }
                    c->items[s] = loot[chosen].item;
                    int range = loot[chosen].maxCount - loot[chosen].minCount + 1;
                    c->counts[s] = loot[chosen].minCount + hash2D(dx, dy + s, seed + 11700 + s) % range;
                }
                // v13: per-slot durability/enchantments (looted tools at full durability)
                memset(c->enchantments, 0, sizeof(c->enchantments));
                for (int s = 0; s < CHEST_SLOTS; s++)
                    c->durability[s] = IsTool((BlockType)c->items[s]) ? GetToolMaxDurability((BlockType)c->items[s]) : 0;
                chestCount++;
            }
            placed = 1;
        }
    }

    // ============================================================
    // Pass 10: Water fill (sky-connected only)
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
    // Pass 10b: Freeze water surface in tundra biome
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        bool isTundra = biomeNoise < -0.45f;
        if (!isTundra) continue;
        for (int y = SEA_LEVEL; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_WATER) {
                // Freeze the top surface layer
                bool hasAirAbove = (y > 0 && world[x][y - 1] == BLOCK_AIR);
                if (hasAirAbove) {
                    world[x][y] = BLOCK_ICE;
                }
            } else if (IsBlockSolid(x, y)) {
                break;
            }
        }
    }

    // ============================================================
    // Pass 11: Trees (biome-aware density and type)
    // ============================================================
    for (int x = 5; x < WORLD_WIDTH - 5; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0;
        if (biomeNoise > 0.55f) biome = 1;       // desert
        else if (biomeNoise > 0.35f) biome = 6;   // taiga
        else if (biomeNoise > 0.15f) biome = 0;   // plains
        else if (biomeNoise > -0.05f) biome = 4;  // swamp
        else if (biomeNoise > -0.25f) biome = 2;  // forest
        else if (biomeNoise > -0.45f) biome = 5;  // jungle
        else biome = 3;                            // tundra

        // Biome-specific tree density
        int treeChance;
        switch (biome) {
            case 1: treeChance = 999; break;  // desert: no trees
            case 2: treeChance = 6; break;    // forest: dense
            case 3: treeChance = 20; break;   // tundra: sparse
            case 4: treeChance = 10; break;   // swamp: moderate
            case 5: treeChance = 4; break;    // jungle: very dense
            case 6: treeChance = 8; break;    // taiga: moderate-dense
            default: treeChance = 12; break;  // plains
        }
        if (hash2D(x, 0, seed + 999) % treeChance != 0) continue;

        // Find surface - check for grass, snowy grass, or mud
        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            uint8_t b = world[x][y];
            if (b == BLOCK_GRASS || b == BLOCK_SNOWY_GRASS || b == BLOCK_MUD) {
                surfaceY = y;
                break;
            }
        }
        if (surfaceY < 0 || surfaceY >= SEA_LEVEL) continue;

        int trunkH;
        BlockType trunkBlock = BLOCK_WOOD;
        BlockType leafBlock = BLOCK_LEAVES;

        switch (biome) {
            case 3: // tundra: short sparse trees
                trunkH = 3 + (hash2D(x, 1, seed + 888) % 2);
                break;
            case 4: // swamp: short wide trees
                trunkH = 3 + (hash2D(x, 1, seed + 888) % 2);
                break;
            case 5: // jungle: tall trees
                trunkH = 8 + (hash2D(x, 1, seed + 888) % 5);
                trunkBlock = BLOCK_JUNGLE_WOOD;
                leafBlock = BLOCK_JUNGLE_LEAVES;
                break;
            case 6: // taiga: medium narrow trees
                trunkH = 5 + (hash2D(x, 1, seed + 888) % 3);
                break;
            default: // plains/forest
                trunkH = (biome == 2) ? (5 + (hash2D(x, 1, seed + 888) % 4)) : (4 + (hash2D(x, 1, seed + 888) % 3));
                break;
        }

        // Place trunk
        for (int i = 1; i <= trunkH && surfaceY - i >= 0; i++) {
            world[x][surfaceY - i] = trunkBlock;
        }

        int canopyTop = surfaceY - trunkH;

        if (biome == 5) {
            // Jungle: large canopy with vines
            for (int dy = -3; dy <= 0; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    int bx = x + dx;
                    int by = canopyTop + dy;
                    if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                        if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                            world[bx][by] = leafBlock;
                        }
                    }
                }
            }
            // Top cap
            for (int dx = -1; dx <= 1; dx++) {
                int bx = x + dx;
                int by = canopyTop - 2;
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    if (world[bx][by] == BLOCK_AIR) world[bx][by] = leafBlock;
                }
            }
            // Vines hanging from canopy edges
            for (int dx = -3; dx <= 3; dx += 2) {
                int bx = x + dx;
                if (bx < 0 || bx >= WORLD_WIDTH) continue;
                for (int dy = 1; dy <= 3; dy++) {
                    int by = canopyTop + dy;
                    if (by >= 0 && by < WORLD_HEIGHT && world[bx][by] == BLOCK_AIR) {
                        if (hash2D(bx, by, seed + 998) % 3 == 0)
                            world[bx][by] = BLOCK_VINE;
                    }
                }
            }
        } else if (biome == 6) {
            // Taiga: triangular/narrow canopy
            for (int dy = -3; dy <= 0; dy++) {
                int width = 1 + (dy + 3); // narrows toward top
                for (int dx = -width; dx <= width; dx++) {
                    int bx = x + dx;
                    int by = canopyTop + dy;
                    if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                        if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                            world[bx][by] = leafBlock;
                        }
                    }
                }
            }
        } else if (biome == 3) {
            // Tundra: small sparse canopy
            for (int dy = -1; dy <= 0; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int bx = x + dx;
                    int by = canopyTop + dy;
                    if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                        if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                            world[bx][by] = leafBlock;
                        }
                    }
                }
            }
        } else if (biome == 4) {
            // Swamp: wide flat canopy
            for (int dy = -1; dy <= 0; dy++) {
                for (int dx = -3; dx <= 3; dx++) {
                    int bx = x + dx;
                    int by = canopyTop + dy;
                    if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                        if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                            world[bx][by] = leafBlock;
                        }
                    }
                }
            }
            // Moss patches under swamp trees
            if (surfaceY + 1 < WORLD_HEIGHT && world[x][surfaceY + 1] != BLOCK_WATER) {
                for (int dx = -2; dx <= 2; dx++) {
                    int bx = x + dx;
                    if (bx >= 0 && bx < WORLD_WIDTH && world[bx][surfaceY] == BLOCK_MUD) {
                        if (hash2D(bx, surfaceY, seed + 997) % 3 == 0)
                            world[bx][surfaceY] = BLOCK_MOSS_BLOCK;
                    }
                }
            }
        } else {
            // Default canopy (plains/forest)
            for (int dy = -2; dy <= 0; dy++) {
                for (int dx = -2; dx <= 2; dx++) {
                    int bx = x + dx;
                    int by = canopyTop + dy;
                    if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                        if (world[bx][by] == BLOCK_AIR && !(dx == 0 && dy == 0)) {
                            world[bx][by] = leafBlock;
                        }
                    }
                }
            }
            for (int dx = -1; dx <= 1; dx++) {
                int bx = x + dx;
                int by = canopyTop - 1;
                if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                    if (world[bx][by] == BLOCK_AIR) world[bx][by] = leafBlock;
                }
            }
        }
    }

    // ============================================================
    // Pass 12: Flowers, tall grass, decorations (biome-aware)
    // ============================================================
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0;
        if (biomeNoise > 0.55f) biome = 1;       // desert
        else if (biomeNoise > 0.35f) biome = 6;   // taiga
        else if (biomeNoise > 0.15f) biome = 0;   // plains
        else if (biomeNoise > -0.05f) biome = 4;  // swamp
        else if (biomeNoise > -0.25f) biome = 2;  // forest
        else if (biomeNoise > -0.45f) biome = 5;  // jungle
        else biome = 3;                            // tundra

        for (int y = 1; y < WORLD_HEIGHT - 1; y++) {
            uint8_t surface = world[x][y];
            if (surface != BLOCK_GRASS && surface != BLOCK_SAND &&
                surface != BLOCK_SNOWY_GRASS && surface != BLOCK_MUD) continue;
            if (world[x][y - 1] != BLOCK_AIR) continue;

            unsigned int h = hash2D(x, y, seed + 6000);
            switch (biome) {
                case 1: // Desert: sparse tall grass, cactus
                    if (h % 30 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    else if (h % 50 == 0 && world[x - 1][y] == BLOCK_SAND &&
                             world[x + 1][y] == BLOCK_SAND && y > SEA_LEVEL + 2)
                        world[x][y - 1] = BLOCK_CACTUS;
                    break;
                case 2: // Forest: more flowers and grass
                    if (h % 12 == 0) world[x][y - 1] = BLOCK_FLOWER;
                    else if (h % 4 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    break;
                case 3: // Tundra: very sparse, some flowers
                    if (h % 25 == 0) world[x][y - 1] = BLOCK_FLOWER;
                    else if (h % 15 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    break;
                case 4: // Swamp: dense grass, pumpkins
                    if (h % 5 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    else if (h % 40 == 0) world[x][y - 1] = BLOCK_PUMPKIN;
                    break;
                case 5: // Jungle: very dense, melons
                    if (h % 3 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    else if (h % 8 == 0) world[x][y - 1] = BLOCK_FLOWER;
                    else if (h % 30 == 0) world[x][y - 1] = BLOCK_MELON;
                    break;
                case 6: // Taiga: moderate, flowers
                    if (h % 10 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    else if (h % 20 == 0) world[x][y - 1] = BLOCK_FLOWER;
                    break;
                default: // Plains
                    if (h % 20 == 0) world[x][y - 1] = BLOCK_FLOWER;
                    else if (h % 8 == 0) world[x][y - 1] = BLOCK_TALL_GRASS;
                    break;
            }
        }
    }

    // ============================================================
    // Pass 12b: Sugar cane near water (warm biomes)
    // ============================================================
    for (int x = 2; x < WORLD_WIDTH - 2; x++) {
        float biomeNoise = fbm(x * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0;
        if (biomeNoise > 0.55f) biome = 1;       // desert
        else if (biomeNoise > 0.35f) biome = 6;   // taiga
        else if (biomeNoise > 0.15f) biome = 0;   // plains
        else if (biomeNoise > -0.05f) biome = 4;  // swamp
        else if (biomeNoise > -0.25f) biome = 2;  // forest
        else if (biomeNoise > -0.45f) biome = 5;  // jungle
        else biome = 3;                            // tundra

        if (biome == 3) continue; // No sugar cane in tundra

        for (int y = 1; y < SEA_LEVEL + 6 && y < WORLD_HEIGHT - 1; y++) {
            if (world[x][y] != BLOCK_AIR) continue;
            // Must have sand or grass below
            uint8_t below = world[x][y - 1];
            if (below != BLOCK_SAND && below != BLOCK_GRASS && below != BLOCK_MUD) continue;
            // Must have water nearby
            bool hasWater = false;
            for (int dx = -1; dx <= 1 && !hasWater; dx++)
                for (int dy = 0; dy <= 1 && !hasWater; dy++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < WORLD_WIDTH && ny >= 0 && ny < WORLD_HEIGHT && world[nx][ny] == BLOCK_WATER)
                        hasWater = true;
                }
            if (!hasWater) continue;
            unsigned int h = hash2D(x, y, seed + 7000);
            if (h % 12 == 0) world[x][y] = BLOCK_SUGAR_CANE;
        }
    }

    // ============================================================
    // Pass 13: Villages
    // ============================================================
    #define VILLAGE_SPACING 400
    #define MAX_VILLAGE_BUILDINGS 6

    for (int v = 0; v < WORLD_WIDTH / VILLAGE_SPACING; v++) {
        // Determine village X position
        int vx = (int)(hash2D(v, 0, seed + 20000) % (WORLD_WIDTH - 60)) + 30;

        // Check biome at village center
        float biomeNoise = fbm(vx * 0.008f, 0.0f, 2, 0.5f, seed + 8000);
        int biome = 0;
        if (biomeNoise > 0.55f) biome = 1;
        else if (biomeNoise > 0.35f) biome = 6;
        else if (biomeNoise > 0.15f) biome = 0;
        else if (biomeNoise > -0.05f) biome = 4;
        else if (biomeNoise > -0.25f) biome = 2;
        else if (biomeNoise > -0.45f) biome = 5;
        else biome = 3;

        // Only place villages in plains or forest
        if (biome != 0 && biome != 2 && biome != 6) continue;

        // Find surface at village center
        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT - 10; y++) {
            if (world[vx][y] == BLOCK_GRASS || world[vx][y] == BLOCK_SNOWY_GRASS) {
                surfaceY = y;
                break;
            }
        }
        if (surfaceY < 10 || surfaceY >= SEA_LEVEL) continue;

        // Check flatness: scan 卤20 blocks for consistent surface
        bool flat = true;
        for (int dx = -20; dx <= 20; dx += 4) {
            int cx = vx + dx;
            if (cx < 0 || cx >= WORLD_WIDTH) { flat = false; break; }
            for (int y = surfaceY - 5; y <= surfaceY + 5; y++) {
                if (y >= 0 && y < WORLD_HEIGHT && (world[cx][y] == BLOCK_GRASS || world[cx][y] == BLOCK_SNOWY_GRASS)) {
                    if (abs(y - surfaceY) > 3) { flat = false; break; }
                    break;
                }
            }
            if (!flat) break;
        }
        if (!flat) continue;

        // Check no water in village area
        bool hasWater = false;
        for (int dx = -20; dx <= 20 && !hasWater; dx++) {
            for (int dy = -3; dy <= 0 && !hasWater; dy++) {
                int cx = vx + dx, cy = surfaceY + dy;
                if (cx >= 0 && cx < WORLD_WIDTH && cy >= 0 && cy < WORLD_HEIGHT) {
                    if (world[cx][cy] == BLOCK_WATER) hasWater = true;
                }
            }
        }
        if (hasWater) continue;

        // Place buildings
        int numBuildings = 3 + (int)(hash2D(v, 1, seed + 20100) % 4);
        int buildX = vx - (numBuildings * 10) / 2;

        for (int b = 0; b < numBuildings && b < MAX_VILLAGE_BUILDINGS; b++) {
            int bx = buildX + b * 10 + (int)(hash2D(v, b + 10, seed + 20200) % 3);

            // Find surface at this building position
            int buildSurface = -1;
            for (int y = 0; y < WORLD_HEIGHT - 10; y++) {
                if (world[bx][y] == BLOCK_GRASS || world[bx][y] == BLOCK_SNOWY_GRASS) {
                    buildSurface = y;
                    break;
                }
            }
            if (buildSurface < 5) continue;

            int btype = (int)(hash2D(v, b, seed + 20300) % 4);

            if (btype == 0 || btype == 3) {
                // House (type 0) or Blacksmith (type 3)
                int w = (btype == 3) ? 8 : 7;
                int h = 6;
                uint8_t wallBlock = (btype == 3) ? BLOCK_COBBLESTONE : BLOCK_PLANKS;
                uint8_t floorBlock = (btype == 3) ? BLOCK_STONE : BLOCK_COBBLESTONE;

                // Clear interior and build walls
                for (int dx = 0; dx < w; dx++) {
                    for (int dy = 0; dy < h; dy++) {
                        int wx = bx + dx, wy = buildSurface - dy;
                        if (wx < 0 || wx >= WORLD_WIDTH || wy < 0 || wy >= WORLD_HEIGHT) continue;
                        if (dy == 0) {
                            // Floor
                            world[wx][wy] = floorBlock;
                        } else if (dy == h - 1 || dx == 0 || dx == w - 1) {
                            // Walls and roof
                            world[wx][wy] = wallBlock;
                        } else {
                            // Interior air
                            world[wx][wy] = BLOCK_AIR;
                        }
                    }
                }

                // Door opening (2 blocks high in front wall)
                int doorX = bx + w / 2;
                if (doorX >= 0 && doorX < WORLD_WIDTH) {
                    if (buildSurface - 1 >= 0) world[doorX][buildSurface - 1] = BLOCK_AIR;
                    if (buildSurface - 2 >= 0) world[doorX][buildSurface - 2] = BLOCK_AIR;
                }

                // Windows (glass)
                if (btype == 0) {
                    // Side windows
                    int winY = buildSurface - 3;
                    if (winY >= 0 && winY < WORLD_HEIGHT) {
                        if (bx + 1 >= 0 && bx + 1 < WORLD_WIDTH) world[bx + 1][winY] = BLOCK_GLASS;
                        if (bx + w - 2 >= 0 && bx + w - 2 < WORLD_WIDTH) world[bx + w - 2][winY] = BLOCK_GLASS;
                    }
                }

                // Torch inside
                int torchX = bx + w / 2;
                int torchY = buildSurface - 4;
                if (torchX >= 0 && torchX < WORLD_WIDTH && torchY >= 0 && torchY < WORLD_HEIGHT) {
                    world[torchX][torchY] = BLOCK_TORCH;
                }

                // Furnace and crafting table in blacksmith
                if (btype == 3) {
                    int furnX = bx + 1;
                    int furnY = buildSurface - 1;
                    if (furnX >= 0 && furnX < WORLD_WIDTH && furnY >= 0 && furnY < WORLD_HEIGHT) {
                        world[furnX][furnY] = BLOCK_FURNACE;
                    }
                    int craftX = bx + w - 2;
                    int craftY = buildSurface - 1;
                    if (craftX >= 0 && craftX < WORLD_WIDTH && craftY >= 0 && craftY < WORLD_HEIGHT) {
                        world[craftX][craftY] = BLOCK_CRAFTING_TABLE;
                    }
                }

                // Bed in house
                if (btype == 0) {
                    int bedX = bx + 1;
                    int bedY = buildSurface - 1;
                    if (bedX >= 0 && bedX < WORLD_WIDTH && bedY >= 0 && bedY < WORLD_HEIGHT) {
                        world[bedX][bedY] = BLOCK_BED;
                    }
                }

                // Place chest
                if (chestCount < MAX_CHESTS) {
                    int chestX = bx + ((btype == 3) ? w - 2 : w - 2);
                    int chestY = buildSurface - 1;
                    if (chestX >= 0 && chestX < WORLD_WIDTH && chestY >= 0 && chestY < WORLD_HEIGHT) {
                        world[chestX][chestY] = BLOCK_CHEST;
                        ChestData *c = &chestData[chestCount];
                        c->x = chestX;
                        c->y = chestY;
                        memset(c->items, 0, sizeof(c->items));
                        memset(c->counts, 0, sizeof(c->counts));

                        typedef struct { uint8_t item; int minCount; int maxCount; int weight; } LootEntry;
                        LootEntry loot[] = {
                            {FOOD_BREAD, 2, 5, 25},
                            {ITEM_WHEAT_SEEDS, 4, 8, 20},
                            {FOOD_APPLE, 1, 3, 15},
                            {ITEM_COAL, 2, 6, 20},
                            {ITEM_IRON_INGOT, 1, 3, (btype == 3) ? 25 : 10},
                            {ITEM_DIAMOND, 1, 1, (btype == 3) ? 8 : 0},
                            {ITEM_GOLD_INGOT, 1, 2, (btype == 3) ? 15 : 5},
                            {BLOCK_TORCH, 3, 6, 15},
                            {ITEM_LEATHER, 2, 4, 10},
                        };
                        int lootCount = sizeof(loot) / sizeof(loot[0]);

                        int slots = 3 + (int)(hash2D(chestX, chestY, seed + 20400) % 3);
                        for (int s = 0; s < slots && s < CHEST_SLOTS; s++) {
                            // Skip items with weight 0
                            int totalWeight = 0;
                            for (int l = 0; l < lootCount; l++) totalWeight += loot[l].weight;
                            if (totalWeight <= 0) break;
                            int roll = (int)(hash2D(chestX + s, chestY, seed + 20500 + s) % totalWeight);
                            int chosen = 0;
                            for (int l = 0; l < lootCount; l++) {
                                roll -= loot[l].weight;
                                if (roll < 0) { chosen = l; break; }
                            }
                            c->items[s] = loot[chosen].item;
                            int range = loot[chosen].maxCount - loot[chosen].minCount + 1;
                            c->counts[s] = loot[chosen].minCount + (int)(hash2D(chestX, chestY + s, seed + 20600 + s) % range);
                        }
                        // v13: per-slot durability/enchantments (looted tools at full durability)
                        memset(c->enchantments, 0, sizeof(c->enchantments));
                        for (int s = 0; s < CHEST_SLOTS; s++)
                            c->durability[s] = IsTool((BlockType)c->items[s]) ? GetToolMaxDurability((BlockType)c->items[s]) : 0;
                        chestCount++;
                    }
                }

            } else if (btype == 1) {
                // Farm: 9x4 area with farmland, crops, and water
                int fw = 9, fh = 4;
                for (int dx = 0; dx < fw; dx++) {
                    for (int dy = 0; dy < fh; dy++) {
                        int fx = bx + dx, fy = buildSurface - dy;
                        if (fx < 0 || fx >= WORLD_WIDTH || fy < 0 || fy >= WORLD_HEIGHT) continue;
                        if (dy == 0) {
                            // Bottom row: farmland with water in center
                            if (dx == fw / 2) {
                                world[fx][fy] = BLOCK_WATER;
                            } else {
                                world[fx][fy] = BLOCK_FARMLAND;
                            }
                        } else if (dy == 1) {
                            // Crops on farmland
                            if (dx != fw / 2) {
                                world[fx][fy] = BLOCK_CROPS;
                                // Generated village farms look established: give each
                                // crop a deterministic initial growth (1-7) from the seed.
                                SetCropGrowth(fx, fy, 1 + (int)(hash2D(fx, fy, seed + 20700) % 7));
                            } else {
                                world[fx][fy] = BLOCK_AIR;
                            }
                        } else if (dy == fh - 1) {
                            // Fence posts (use cobblestone as fence substitute)
                            if (dx == 0 || dx == fw - 1) {
                                world[fx][fy] = BLOCK_COBBLESTONE;
                            } else {
                                world[fx][fy] = BLOCK_AIR;
                            }
                        } else {
                            world[fx][fy] = BLOCK_AIR;
                        }
                    }
                }

            } else if (btype == 2) {
                // Well: 5x5 cobblestone with water center
                int ww = 5;
                for (int dx = 0; dx < ww; dx++) {
                    for (int dy = 0; dy < 4; dy++) {
                        int wx = bx + dx, wy = buildSurface - dy;
                        if (wx < 0 || wx >= WORLD_WIDTH || wy < 0 || wy >= WORLD_HEIGHT) continue;
                        if (dy == 0) {
                            world[wx][wy] = BLOCK_COBBLESTONE;
                        } else if (dy <= 2 && (dx == 0 || dx == ww - 1)) {
                            // Walls (2 high)
                            world[wx][wy] = BLOCK_COBBLESTONE;
                        } else if (dy == 3) {
                            // Roof edge
                            world[wx][wy] = BLOCK_COBBLESTONE;
                        } else if (dx == ww / 2 && dy == 1) {
                            // Water in center
                            world[wx][wy] = BLOCK_WATER;
                        } else {
                            world[wx][wy] = BLOCK_AIR;
                        }
                    }
                }
            }

            // Place dirt path between buildings
            if (b < numBuildings - 1) {
                int nextBx = buildX + (b + 1) * 10 + (int)(hash2D(v, b + 11, seed + 20200) % 3);
                int pathStart = bx + ((btype == 3) ? 8 : 7);
                int pathEnd = nextBx;
                for (int px = pathStart; px < pathEnd && px < WORLD_WIDTH; px++) {
                    if (px >= 0) {
                        // Find surface at path position
                        for (int y = 0; y < WORLD_HEIGHT - 10; y++) {
                            if (world[px][y] == BLOCK_GRASS || world[px][y] == BLOCK_SNOWY_GRASS) {
                                world[px][y] = BLOCK_DIRT;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Spawn villagers near houses (after all buildings placed)
        for (int b = 0; b < numBuildings; b++) {
            int btype = (int)(hash2D(v, b, seed + 20300) % 4);
            if (btype != 0 && btype != 3) continue; // Only houses and blacksmiths
            int bx = buildX + b * 10 + (int)(hash2D(v, b + 10, seed + 20200) % 3);
            int w = (btype == 3) ? 8 : 7;
            // Scan for intact grass outside building (right side)
            int buildSurface = -1;
            for (int y = 0; y < WORLD_HEIGHT - 10; y++) {
                if (world[bx + w][y] == BLOCK_GRASS || world[bx + w][y] == BLOCK_SNOWY_GRASS) {
                    buildSurface = y;
                    break;
                }
            }
            if (buildSurface < 5) {
                // Fallback: scan left side
                for (int y = 0; y < WORLD_HEIGHT - 10; y++) {
                    if (bx > 0 && (world[bx - 1][y] == BLOCK_GRASS || world[bx - 1][y] == BLOCK_SNOWY_GRASS)) {
                        buildSurface = y;
                        break;
                    }
                }
            }
            if (buildSurface < 5) continue;
            // Spawn 1-2 villagers outside the building
            int numVillagers = 1 + (int)(hash2D(v, b + 20, seed + 20700) % 2);
            for (int vi = 0; vi < numVillagers; vi++) {
                int spawnX = bx + 3 + vi * 2;
                if (spawnX >= 0 && spawnX < WORLD_WIDTH) {
                    SpawnMob(MOB_VILLAGER, spawnX * BLOCK_SIZE, (buildSurface - 2) * BLOCK_SIZE);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Chunk System - Hash table with linear probing, O(1) lookup
//----------------------------------------------------------------------------------
void InitChunkTable(void)
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
    (void)worldBlockY;
    int cx = worldBlockX / CHUNK_SIZE;
    Chunk *c = GetChunk(cx);
    if (c) {
        if (c->textureValid) UnloadTexture(c->texture);
        c->textureValid = false;
        BuildWaterCache(c);
    }
}

void UpdateChunks(void)
{
    int playerBlockX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerChunkX = playerBlockX / CHUNK_SIZE;

    // Unload distant chunks (collect first to avoid modifying hash table during iteration)
    int toUnload[MAX_CHUNKS];
    int unloadCount = 0;
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (abs(loadedChunks[i].chunkX - playerChunkX) > CHUNKS_LOADED) {
            toUnload[unloadCount++] = loadedChunks[i].chunkX;
        }
    }
    for (int i = 0; i < unloadCount; i++) {
        UnloadChunk(toUnload[i]);
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
