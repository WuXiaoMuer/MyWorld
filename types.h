#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdbool.h>

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
#define INVENTORY_SLOTS     (INVENTORY_ROWS * INVENTORY_COLS)

#define BREAK_RANGE         6
#define PLACE_RANGE         6

#define MAX_CRAFT_RECIPES   16

#define SAVE_MAGIC          "MWSV"
#define SAVE_VERSION        2
#define SAVE_PATH           "saves/world.mwsav"

#define DEATH_Y             (WORLD_HEIGHT * BLOCK_SIZE + 500)

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

//----------------------------------------------------------------------------------
// Crafting
//----------------------------------------------------------------------------------
typedef struct {
    BlockType input;
    int inputCount;
    BlockType output;
    int outputCount;
    const char *name;
} CraftingRecipe;

//----------------------------------------------------------------------------------
// Data Structures
//----------------------------------------------------------------------------------
typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool onGround;
    int selectedSlot;
    uint8_t inventory[INVENTORY_SLOTS];
    int inventoryCount[INVENTORY_SLOTS];
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
// Extern Globals
//----------------------------------------------------------------------------------
extern uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
extern Chunk loadedChunks[MAX_CHUNKS];
extern int loadedChunkCount;

extern Player player;
extern Camera2D camera;
extern DayNightCycle dayNight;

extern Texture2D blockAtlas;
extern bool showDebug;
extern bool inventoryOpen;
extern unsigned int worldSeed;

extern const BlockInfo blockInfo[BLOCK_COUNT];
extern CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
extern int craftRecipeCount;

//----------------------------------------------------------------------------------
// Function Declarations
//----------------------------------------------------------------------------------

// noise.c
unsigned int hash2D(int x, int y, unsigned int seed);
float valueNoise(float x, float y, unsigned int seed);
float fbm(float x, float y, int octaves, float persistence, unsigned int seed);

// world.c
void DrawBlockPattern(Image *img, int px, int py, BlockType bt, int worldX, int worldY);
void GenerateBlockAtlas(void);
void GenerateWorld(unsigned int seed);
Chunk* GetChunk(int chunkX);
void UnloadChunk(int index);
void GenerateChunkTexture(Chunk *chunk);
void InvalidateChunkAt(int worldBlockX, int worldBlockY);
void UpdateChunks(void);
bool IsBlockSolid(int bx, int by);

// player.c
void InitPlayer(void);
bool AddToInventory(BlockType item);
void PlayerPhysics(float dt);
void PlayerBlockInteraction(void);
void UpdatePlayer(float dt);
void InitCameraSystem(void);
void UpdateCameraSystem(float dt);
void UpdateHotbar(void);

// daynight.c
void InitDayNight(void);
void UpdateDayNight(float dt);
Color GetSkyColor(void);

// rendering.c
void DrawWorld(void);
void DrawWater(void);
void DrawPlayerSprite(void);
void DrawHotbar(void);
void DrawCrosshair(void);
void DrawDebugInfo(void);
void DrawInventoryScreen(void);

// save.c
bool SaveExists(const char *path);
bool SaveWorld(const char *path);
bool LoadWorld(const char *path);

// crafting.c
void InitCraftingRecipes(void);
bool CanCraft(int recipeIndex);
void Craft(int recipeIndex);
void DrawCraftingPanel(int panelX, int panelY);

// game.c
void InitGame(void);
void UpdateGame(float dt);
void DrawGame(void);
void UnloadGame(void);
void UpdateDrawFrame(void);

#endif // TYPES_H
