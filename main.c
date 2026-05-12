/*******************************************************************************************
*
*  
*   A 2D sandbox game inspired by Minecraft, built with raylib 5.5
*   All assets are procedurally generated pixel art
*
********************************************************************************************/

#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------------
#define SCREEN_WIDTH        1280
#define SCREEN_HEIGHT       720

#define WORLD_WIDTH         2048
#define WORLD_HEIGHT        256

#define BLOCK_SIZE          16

#define CHUNK_SIZE          16
#define MAX_CHUNKS          64
#define CHUNKS_LOADED       12

#define SEA_LEVEL           128
#define TERRAIN_BASE        120
#define TERRAIN_AMPLITUDE   35
#define CAVE_START          150
#define CAVE_END            245

#define PLAYER_WIDTH        12
#define PLAYER_HEIGHT       28
#define GRAVITY             980.0f
#define JUMP_VELOCITY       -380.0f
#define MOVE_SPEED          120.0f

#define HOTBAR_SLOTS        9
#define INVENTORY_ROWS      4
#define INVENTORY_COLS      9

#define BREAK_RANGE         6
#define PLACE_RANGE         6

//----------------------------------------------------------------------------------
// Block Types
//----------------------------------------------------------------------------------
typedef enum {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_COBBLESTONE,
    BLOCK_WOOD,
    BLOCK_LEAVES,
    BLOCK_SAND,
    BLOCK_WATER,
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_PLANKS,
    BLOCK_BRICK,
    BLOCK_GLASS,
    BLOCK_BEDROCK,
    BLOCK_COUNT
} BlockType;

typedef struct {
    const char *name;
    Color baseColor;
    Color detailColor;
    bool solid;
    bool transparent;
    bool breakable;
} BlockInfo;

static const BlockInfo blockInfo[BLOCK_COUNT] = {
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
};

//----------------------------------------------------------------------------------
// Data Structures
//----------------------------------------------------------------------------------
typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool onGround;
    int selectedSlot;
    uint8_t inventory[HOTBAR_SLOTS];
    int inventoryCount[HOTBAR_SLOTS];
} Player;

typedef struct {
    int chunkX;
    Texture2D texture;
    bool textureValid;
} Chunk;

typedef struct {
    float timeOfDay;
    float daySpeed;
    float lightLevel;
} DayNightCycle;

//----------------------------------------------------------------------------------
// Global Variables
//----------------------------------------------------------------------------------
static uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
static Chunk loadedChunks[MAX_CHUNKS];
static int loadedChunkCount = 0;

static Player player = { 0 };
static Camera2D camera = { 0 };
static DayNightCycle dayNight = { 0 };

static Texture2D blockAtlas = { 0 };
static bool showDebug = false;

//----------------------------------------------------------------------------------
// Noise Functions
//----------------------------------------------------------------------------------
static unsigned int hash2D(int x, int y, unsigned int seed)
{
    unsigned int h = seed;
    h ^= (unsigned int)x * 374761393u;
    h ^= (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return h;
}

static float valueNoise(float x, float y, unsigned int seed)
{
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - ix;
    float fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    float v00 = (float)(hash2D(ix, iy, seed) & 0xFFFF) / 65535.0f;
    float v10 = (float)(hash2D(ix + 1, iy, seed) & 0xFFFF) / 65535.0f;
    float v01 = (float)(hash2D(ix, iy + 1, seed) & 0xFFFF) / 65535.0f;
    float v11 = (float)(hash2D(ix + 1, iy + 1, seed) & 0xFFFF) / 65535.0f;

    float top = v00 + fx * (v10 - v00);
    float bot = v01 + fx * (v11 - v01);
    return top + fy * (bot - top);
}

static float fbm(float x, float y, int octaves, float persistence, unsigned int seed)
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += valueNoise(x * frequency, y * frequency, seed + i * 1000) * amplitude;
        maxVal += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    return total / maxVal;
}

//----------------------------------------------------------------------------------
// Block Pixel Art Generation
//----------------------------------------------------------------------------------
static void DrawBlockPattern(Image *img, int px, int py, BlockType bt, int worldX, int worldY)
{
    unsigned int varSeed = (unsigned int)(worldX * 7919 + worldY * 104729);
    Color base = blockInfo[bt].baseColor;
    Color detail = blockInfo[bt].detailColor;

    switch (bt) {
    case BLOCK_GRASS:
        // Dirt part
        for (int y = 3; y < 16; y++)
            for (int x = 0; x < 16; x++) {
                Color c = detail;
                if ((hash2D(x + worldX * 16, y + worldY * 16, 1) % 5) == 0) c = (Color){100, 70, 45, 255};
                ImageDrawPixel(img, px + x, py + y, c);
            }
        // Grass top with wavy edge
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
                if (h % 17 == 0) c = (Color){0, 0, 0, 0}; // holes
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
                // Coal clusters
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
                // Iron clusters
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

    default:
        break;
    }
}

static void GenerateBlockAtlas(void)
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
static void GenerateWorld(unsigned int seed)
{
    memset(world, 0, sizeof(world));

    // Generate terrain
    for (int x = 0; x < WORLD_WIDTH; x++) {
        float noise1 = fbm(x * 0.005f, 0.0f, 4, 0.5f, seed);
        float noise2 = fbm(x * 0.02f, 0.0f, 2, 0.5f, seed + 1000) * 0.3f;
        int surfaceY = TERRAIN_BASE + (int)((noise1 + noise2) * TERRAIN_AMPLITUDE);
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

    // Generate caves
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

    // Generate ores
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

    // Generate sand near sea level
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_GRASS || world[x][y] == BLOCK_DIRT) {
                if (y >= SEA_LEVEL - 3 && y <= SEA_LEVEL + 2) {
                    world[x][y] = BLOCK_SAND;
                }
            }
        }
    }

    // Generate water
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_AIR && y >= SEA_LEVEL) {
                world[x][y] = BLOCK_WATER;
            }
        }
    }

    // Generate trees
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
        // Top leaves
        for (int dx = -1; dx <= 1; dx++) {
            int bx = x + dx;
            int by = canopyTop - 1;
            if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                if (world[bx][by] == BLOCK_AIR) world[bx][by] = BLOCK_LEAVES;
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Chunk System
//----------------------------------------------------------------------------------
static Chunk* GetChunk(int chunkX)
{
    for (int i = 0; i < loadedChunkCount; i++) {
        if (loadedChunks[i].chunkX == chunkX) return &loadedChunks[i];
    }
    return NULL;
}

static void UnloadChunk(int index)
{
    if (loadedChunks[index].textureValid) {
        UnloadTexture(loadedChunks[index].texture);
    }
    // Shift remaining chunks
    for (int i = index; i < loadedChunkCount - 1; i++) {
        loadedChunks[i] = loadedChunks[i + 1];
    }
    loadedChunkCount--;
}

static void GenerateChunkTexture(Chunk *chunk)
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
}

static void InvalidateChunkAt(int worldBlockX, int worldBlockY)
{
    int cx = worldBlockX / CHUNK_SIZE;
    Chunk *c = GetChunk(cx);
    if (c) c->textureValid = false;
}

static void UpdateChunks(void)
{
    int playerBlockX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerChunkX = playerBlockX / CHUNK_SIZE;

    // Unload distant chunks
    for (int i = loadedChunkCount - 1; i >= 0; i--) {
        if (abs(loadedChunks[i].chunkX - playerChunkX) > CHUNKS_LOADED) {
            UnloadChunk(i);
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
            if (loadedChunkCount >= MAX_CHUNKS) continue;
            c = &loadedChunks[loadedChunkCount++];
            c->chunkX = cx;
            c->textureValid = false;
        }
        if (!c->textureValid) {
            GenerateChunkTexture(c);
        }
    }
}

//----------------------------------------------------------------------------------
// Player System
//----------------------------------------------------------------------------------
static bool IsBlockSolid(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return true;
    return blockInfo[world[bx][by]].solid;
}

static void InitPlayer(void)
{
    // Place player at world center, above terrain
    int spawnX = WORLD_WIDTH / 2;
    int spawnY = 0;
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        if (world[spawnX][y] != BLOCK_AIR && world[spawnX][y] != BLOCK_WATER) {
            spawnY = y - 3;
            break;
        }
    }
    player.position = (Vector2){ spawnX * BLOCK_SIZE, spawnY * BLOCK_SIZE };
    player.velocity = (Vector2){ 0, 0 };
    player.onGround = false;
    player.selectedSlot = 0;

    // Starting inventory
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        player.inventory[i] = BLOCK_AIR;
        player.inventoryCount[i] = 0;
    }
    player.inventory[0] = BLOCK_PLANKS;
    player.inventoryCount[0] = 64;
    player.inventory[1] = BLOCK_DIRT;
    player.inventoryCount[1] = 32;
    player.inventory[2] = BLOCK_COBBLESTONE;
    player.inventoryCount[2] = 32;
}

static bool AddToInventory(BlockType item)
{
    // Try to stack
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (player.inventory[i] == item && player.inventoryCount[i] < 64) {
            player.inventoryCount[i]++;
            return true;
        }
    }
    // Find empty slot
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            player.inventory[i] = item;
            player.inventoryCount[i] = 1;
            return true;
        }
    }
    return false;
}

static void PlayerPhysics(float dt)
{
    // Horizontal input
    player.velocity.x = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) player.velocity.x = -MOVE_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) player.velocity.x = MOVE_SPEED;

    // Jump
    if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_SPACE)) && player.onGround) {
        player.velocity.y = JUMP_VELOCITY;
        player.onGround = false;
    }

    // Gravity
    player.velocity.y += GRAVITY * dt;
    if (player.velocity.y > 800.0f) player.velocity.y = 800.0f;

    // Horizontal collision
    float newX = player.position.x + player.velocity.x * dt;
    float left = newX;
    float right = newX + PLAYER_WIDTH;
    float top = player.position.y;
    float bottom = player.position.y + PLAYER_HEIGHT;

    bool blocked = false;
    int minBX = (int)(left) / BLOCK_SIZE;
    int maxBX = (int)(right - 0.01f) / BLOCK_SIZE;
    int minBY = (int)(top) / BLOCK_SIZE;
    int maxBY = (int)(bottom - 0.01f) / BLOCK_SIZE;

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    if (blocked) {
        if (player.velocity.x > 0) {
            newX = (int)(right / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_WIDTH - 0.01f;
        } else if (player.velocity.x < 0) {
            newX = (int)(left / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.x = 0;
    }
    player.position.x = newX;

    // Vertical collision
    float newY = player.position.y + player.velocity.y * dt;
    left = player.position.x;
    right = player.position.x + PLAYER_WIDTH;
    top = newY;
    bottom = newY + PLAYER_HEIGHT;

    blocked = false;
    minBX = (int)(left) / BLOCK_SIZE;
    maxBX = (int)(right - 0.01f) / BLOCK_SIZE;
    minBY = (int)(top) / BLOCK_SIZE;
    maxBY = (int)(bottom - 0.01f) / BLOCK_SIZE;

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    player.onGround = false;
    if (blocked) {
        if (player.velocity.y > 0) {
            newY = (int)(bottom / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_HEIGHT;
            player.onGround = true;
        } else if (player.velocity.y < 0) {
            newY = (int)(top / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.y = 0;
    }
    player.position.y = newY;

    // Clamp to world bounds
    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > (WORLD_WIDTH - 1) * BLOCK_SIZE)
        player.position.x = (WORLD_WIDTH - 1) * BLOCK_SIZE;
}

static void PlayerBlockInteraction(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX < 0 || blockX >= WORLD_WIDTH || blockY < 0 || blockY >= WORLD_HEIGHT) return;

    // Distance check
    float playerCenterX = player.position.x + PLAYER_WIDTH / 2.0f;
    float playerCenterY = player.position.y + PLAYER_HEIGHT / 2.0f;
    float distX = fabsf((blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterX) / BLOCK_SIZE;
    float distY = fabsf((blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterY) / BLOCK_SIZE;
    float dist = sqrtf(distX * distX + distY * distY);

    if (dist > BREAK_RANGE) return;

    // Break block
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        BlockType bt = (BlockType)world[blockX][blockY];
        if (bt != BLOCK_AIR && bt != BLOCK_WATER && blockInfo[bt].breakable) {
            world[blockX][blockY] = BLOCK_AIR;
            AddToInventory(bt);
            InvalidateChunkAt(blockX, blockY);
            // Invalidate adjacent chunks at edges
            if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
            if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
        }
    }

    // Place block
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        BlockType selected = (BlockType)player.inventory[player.selectedSlot];
        if (selected != BLOCK_AIR && player.inventoryCount[player.selectedSlot] > 0) {
            if (world[blockX][blockY] == BLOCK_AIR || world[blockX][blockY] == BLOCK_WATER) {
                // Check no overlap with player
                float bLeft = blockX * BLOCK_SIZE;
                float bRight = bLeft + BLOCK_SIZE;
                float bTop = blockY * BLOCK_SIZE;
                float bBottom = bTop + BLOCK_SIZE;

                float pLeft = player.position.x;
                float pRight = pLeft + PLAYER_WIDTH;
                float pTop = player.position.y;
                float pBottom = pTop + PLAYER_HEIGHT;

                if (!(pRight > bLeft && pLeft < bRight && pBottom > bTop && pTop < bBottom)) {
                    world[blockX][blockY] = selected;
                    player.inventoryCount[player.selectedSlot]--;
                    if (player.inventoryCount[player.selectedSlot] <= 0) {
                        player.inventory[player.selectedSlot] = BLOCK_AIR;
                    }
                    InvalidateChunkAt(blockX, blockY);
                    if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                    if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                }
            }
        }
    }
}

static void UpdatePlayer(float dt)
{
    PlayerPhysics(dt);
    PlayerBlockInteraction();
}

//----------------------------------------------------------------------------------
// Camera System
//----------------------------------------------------------------------------------
static void InitCameraSystem(void)
{
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.target = (Vector2){ player.position.x + PLAYER_WIDTH / 2, player.position.y + PLAYER_HEIGHT / 2 };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;
}

static void UpdateCameraSystem(float dt)
{
    Vector2 playerCenter = {
        player.position.x + PLAYER_WIDTH / 2.0f,
        player.position.y + PLAYER_HEIGHT / 2.0f
    };
    camera.target = Vector2Lerp(camera.target, playerCenter, 8.0f * dt);
}

//----------------------------------------------------------------------------------
// Inventory / Hotbar
//----------------------------------------------------------------------------------
static void UpdateHotbar(void)
{
    // Number keys
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (IsKeyPressed(KEY_ONE + i)) player.selectedSlot = i;
    }

    // Mouse wheel
    float wheel = GetMouseWheelMove();
    if (wheel < 0) player.selectedSlot = (player.selectedSlot + 1) % HOTBAR_SLOTS;
    if (wheel > 0) player.selectedSlot = (player.selectedSlot - 1 + HOTBAR_SLOTS) % HOTBAR_SLOTS;
}

//----------------------------------------------------------------------------------
// Day/Night Cycle
//----------------------------------------------------------------------------------
static void InitDayNight(void)
{
    dayNight.timeOfDay = 0.35f; // Start at morning
    dayNight.daySpeed = 0.008f; // ~125 second cycle
    dayNight.lightLevel = 1.0f;
}

static void UpdateDayNight(float dt)
{
    dayNight.timeOfDay += dayNight.daySpeed * dt;
    if (dayNight.timeOfDay >= 1.0f) dayNight.timeOfDay -= 1.0f;

    float t = dayNight.timeOfDay;
    if (t < 0.2f) dayNight.lightLevel = 0.2f;
    else if (t < 0.3f) dayNight.lightLevel = 0.2f + (t - 0.2f) / 0.1f * 0.8f;
    else if (t < 0.7f) dayNight.lightLevel = 1.0f;
    else if (t < 0.8f) dayNight.lightLevel = 1.0f - (t - 0.7f) / 0.1f * 0.8f;
    else dayNight.lightLevel = 0.2f;
}

static Color GetSkyColor(void)
{
    float l = dayNight.lightLevel;
    unsigned char r = (unsigned char)(10 + l * 125);
    unsigned char g = (unsigned char)(10 + l * 196);
    unsigned char b = (unsigned char)(40 + l * 195);
    return (Color){r, g, b, 255};
}

//----------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------
static void DrawWorld(void)
{
    // Compute visible area
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    for (int i = 0; i < loadedChunkCount; i++) {
        Chunk *c = &loadedChunks[i];
        if (c->chunkX < minCX || c->chunkX > maxCX) continue;
        if (!c->textureValid) continue;
        float x = (float)(c->chunkX * CHUNK_SIZE * BLOCK_SIZE);
        DrawTexture(c->texture, (int)x, 0, WHITE);
    }
}

static void DrawWater(void)
{
    // Animated water drawn per-frame
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewTop = camera.target.y - (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;

    int minBX = (int)(viewLeft / BLOCK_SIZE) - 1;
    int maxBX = (int)(viewRight / BLOCK_SIZE) + 1;
    int minBY = (int)(viewTop / BLOCK_SIZE) - 1;
    int maxBY = (int)(viewBottom / BLOCK_SIZE) + 1;

    if (minBX < 0) minBX = 0;
    if (maxBX >= WORLD_WIDTH) maxBX = WORLD_WIDTH - 1;
    if (minBY < 0) minBY = 0;
    if (maxBY >= WORLD_HEIGHT) maxBY = WORLD_HEIGHT - 1;

    float time = (float)GetTime();

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (world[bx][by] == BLOCK_WATER) {
                float wave = sinf(bx * 0.5f + time * 2.0f) * 1.5f;
                Color wc = blockInfo[BLOCK_WATER].baseColor;
                DrawRectangle(bx * BLOCK_SIZE, (int)(by * BLOCK_SIZE + wave), BLOCK_SIZE, BLOCK_SIZE, wc);
            }
        }
    }
}

static void DrawPlayerSprite(void)
{
    float px = player.position.x;
    float py = player.position.y;
    float time = (float)GetTime();
    bool moving = fabsf(player.velocity.x) > 10.0f;
    float armSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;
    float legSwing = moving ? sinf(time * 10.0f) * 4.0f : 0;

    // Head
    DrawRectangle((int)(px + 2), (int)py, 8, 8, (Color){220, 180, 140, 255});
    // Hair
    DrawRectangle((int)(px + 2), (int)py, 8, 3, (Color){80, 50, 30, 255});
    // Eyes
    DrawRectangle((int)(px + 3), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});
    DrawRectangle((int)(px + 7), (int)(py + 4), 2, 2, (Color){40, 40, 40, 255});

    // Body
    DrawRectangle((int)(px + 1), (int)(py + 8), 10, 10, (Color){0, 100, 200, 255});

    // Arms
    DrawRectangle((int)(px - 2), (int)(py + 8 + armSwing), 3, 10, (Color){220, 180, 140, 255});
    DrawRectangle((int)(px + 11), (int)(py + 8 - armSwing), 3, 10, (Color){220, 180, 140, 255});

    // Legs
    DrawRectangle((int)(px + 1), (int)(py + 18 + legSwing), 4, 10, (Color){60, 40, 20, 255});
    DrawRectangle((int)(px + 7), (int)(py + 18 - legSwing), 4, 10, (Color){60, 40, 20, 255});
}

static void DrawHotbar(void)
{
    int slotSize = 44;
    int padding = 4;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int startY = SCREEN_HEIGHT - slotSize - 10;

    // Background
    DrawRectangle(startX - 4, startY - 4, totalW + 8, slotSize + 8, (Color){0, 0, 0, 150});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;

        // Slot background
        Color slotColor = (i == player.selectedSlot) ? (Color){255, 255, 255, 200} : (Color){80, 80, 80, 200};
        DrawRectangle(x, y, slotSize, slotSize, slotColor);
        DrawRectangleLines(x, y, slotSize, slotSize, DARKGRAY);

        // Block icon
        if (player.inventory[i] != BLOCK_AIR) {
            Rectangle src = { (float)(player.inventory[i] * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

            // Count
            if (player.inventoryCount[i] > 1) {
                const char *countText = TextFormat("%d", player.inventoryCount[i]);
                DrawText(countText, x + slotSize - 20, y + slotSize - 16, 12, WHITE);
            }
        }

        // Slot number
        DrawText(TextFormat("%d", i + 1), x + 3, y + 2, 10, (Color){200, 200, 200, 150});
    }
}

static void DrawCrosshair(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX >= 0 && blockX < WORLD_WIDTH && blockY >= 0 && blockY < WORLD_HEIGHT) {
        float px = (float)(blockX * BLOCK_SIZE);
        float py = (float)(blockY * BLOCK_SIZE);
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 180});
    }
}

static void DrawDebugInfo(void)
{
    if (!showDebug) return;
    int y = 10;
    int lineH = 16;
    Color c = (Color){255, 255, 0, 200};

    DrawText(TextFormat("FPS: %d", GetFPS()), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Pos: %.1f, %.1f", player.position.x, player.position.y), 10, y, 14, c); y += lineH;
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    DrawText(TextFormat("Block: %d, %d", bx, by), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Chunks: %d", loadedChunkCount), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Time: %.2f", dayNight.timeOfDay), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("Light: %.2f", dayNight.lightLevel), 10, y, 14, c); y += lineH;
    DrawText(TextFormat("OnGround: %s", player.onGround ? "yes" : "no"), 10, y, 14, c);
}

//----------------------------------------------------------------------------------
// Main Game Functions
//----------------------------------------------------------------------------------
static void InitGame(void)
{
    srand((unsigned int)time(NULL));
    unsigned int seed = (unsigned int)rand();

    GenerateBlockAtlas();
    GenerateWorld(seed);
    InitPlayer();
    InitCameraSystem();
    InitDayNight();
    UpdateChunks();
}

static void UpdateGame(float dt)
{
    if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    UpdatePlayer(dt);
    UpdateCameraSystem(dt);
    UpdateChunks();
    UpdateHotbar();
    UpdateDayNight(dt);
}

static void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(GetSkyColor());

    BeginMode2D(camera);
    DrawWorld();
    DrawWater();
    DrawCrosshair();
    DrawPlayerSprite();
    EndMode2D();

    // Dark overlay for night
    if (dayNight.lightLevel < 1.0f) {
        unsigned char alpha = (unsigned char)(200 * (1.0f - dayNight.lightLevel));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 20, alpha});
    }

    DrawHotbar();
    DrawDebugInfo();
    DrawFPS(SCREEN_WIDTH - 80, 10);

    EndDrawing();
}

static void UnloadGame(void)
{
    for (int i = 0; i < loadedChunkCount; i++) {
        if (loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
    }
    UnloadTexture(blockAtlas);
}

static void UpdateDrawFrame(void)
{
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f; // Cap delta to prevent physics explosion
    UpdateGame(dt);
    DrawGame();
}

//----------------------------------------------------------------------------------
// Program Entry Point
//----------------------------------------------------------------------------------
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MyWorld");
    SetTargetFPS(60);
    SetExitKey(0);

    InitGame();

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}
