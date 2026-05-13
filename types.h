#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

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
#define CHUNK_EMPTY         INT_MAX

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
#define SPRINT_SPEED_MULT   1.6f

// Water physics
#define WATER_GRAVITY_MULT  0.15f
#define WATER_SPEED_MULT    0.55f
#define WATER_SWIM_VEL      -180.0f
#define WATER_MAX_FALL      200.0f

#define HOTBAR_SLOTS        9
#define INVENTORY_ROWS      4
#define INVENTORY_COLS      9
#define INVENTORY_SLOTS     (INVENTORY_ROWS * INVENTORY_COLS)

#define BREAK_RANGE         6
#define PLACE_RANGE         6

#define MAX_CRAFT_RECIPES   32

#define SAVE_MAGIC          "MWSV"
#define SAVE_VERSION        3
#define SAVE_PATH           "saves/world.mwsav"

#define DEATH_Y             (WORLD_HEIGHT * BLOCK_SIZE + 500)
#define MESSAGE_DURATION    2.0f

// Player status
#define MAX_HEALTH          20
#define MAX_HUNGER          20
#define MAX_OXYGEN          10
#define MAX_XP              100
#define XP_HEAL_COST        10
#define XP_HEAL_AMOUNT      5
#define HUNGER_DRAIN_RATE   (0.5f / 60.0f)  // 0.5 per minute
#define HUNGER_SPRINT_MULT  2.0f
#define HEALTH_REGEN_RATE   (1.0f / 4.0f)   // 1 HP per 4 seconds
#define HEALTH_REGEN_HUNGER 18              // Need hunger > 18 to regen
#define OXYGEN_DRAIN_RATE   1.0f            // 1 per second underwater
#define DROWN_DAMAGE_RATE   (2.0f / 1.0f)   // 2 hearts per second when out of oxygen
#define HUNGER_DAMAGE_RATE  (1.0f / 2.0f)   // 1 heart per 2 seconds at 0 hunger

// Tool durability
#define DURABILITY_WOOD     60
#define DURABILITY_STONE    132
#define DURABILITY_IRON     251

// Food restoration values (hunger points)
#define FOOD_RAW_PORK_VALUE    3
#define FOOD_COOKED_PORK_VALUE 8
#define FOOD_APPLE_VALUE       4
#define FOOD_BREAD_VALUE       5

// Mob system
#define MAX_MOBS            32

// Item entity system
#define MAX_ENTITIES        128
#define ENTITY_GRAVITY      980.0f
#define ENTITY_PICKUP_DIST  24.0f
#define ENTITY_PICKUP_DELAY 0.5f
#define ENTITY_LIFETIME     300.0f
#define ENTITY_BOUNCE       0.3f
#define MOB_SPAWN_INTERVAL  3.0f
#define MOB_SPAWN_DIST_MIN  400.0f
#define MOB_SPAWN_DIST_MAX  800.0f
#define MOB_DESPAWN_DIST    1200.0f
#define MOB_AI_INTERVAL     0.5f
#define MOB_CONTACT_COOLDOWN 1.0f
#define MOB_DEATH_TIME      0.3f
#define MOB_GRAVITY         980.0f

// Light system
#define MAX_LIGHT_LEVEL     15
#define TORCH_LIGHT         15
#define SUNLIGHT_LEVEL      15

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
    // New terrain blocks
    BLOCK_GRAVEL,
    BLOCK_CLAY,
    BLOCK_SANDSTONE,
    // Decorative blocks
    BLOCK_TORCH,
    BLOCK_FLOWER,
    BLOCK_TALL_GRASS,
    BLOCK_FURNACE,
    // Items (not placeable)
    ITEM_STICK,
    ITEM_COAL,
    ITEM_IRON_INGOT,
    // Tools (not placeable)
    TOOL_WOOD_PICKAXE,
    TOOL_WOOD_AXE,
    TOOL_WOOD_SWORD,
    TOOL_WOOD_SHOVEL,
    TOOL_STONE_PICKAXE,
    TOOL_STONE_AXE,
    TOOL_STONE_SWORD,
    TOOL_STONE_SHOVEL,
    TOOL_IRON_PICKAXE,
    TOOL_IRON_AXE,
    TOOL_IRON_SWORD,
    TOOL_IRON_SHOVEL,
    // Food (not placeable)
    FOOD_RAW_PORK,
    FOOD_COOKED_PORK,
    FOOD_APPLE,
    FOOD_BREAD,
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
// Mob System
//----------------------------------------------------------------------------------
typedef enum {
    MOB_NONE = 0,
    MOB_PIG,
    MOB_ZOMBIE,
    MOB_TYPE_COUNT
} MobType;

typedef struct {
    MobType type;
    Vector2 position;
    Vector2 velocity;
    int health;
    int maxHealth;
    bool facingRight;
    bool onGround;
    float aiTimer;
    int aiState;        // 0=idle, 1=wander/chase
    float contactCooldown;
    float deathTimer;   // >0 = dying
    float burnTimer;    // sunlight damage accumulator
    bool active;
} Mob;

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
    int toolDurability[INVENTORY_SLOTS];
    // Status
    int health;
    int hunger;
    int oxygen;
    int xp;
    float oxygenTimer;
    float hungerTimer;
    float regenTimer;
    float drownTimer;
    float hungerDamageTimer;
    float damageFlashTimer;  // Red flash when taking damage
    float knockbackTimer;    // Preserves horizontal velocity during knockback
    bool sprinting;
    bool playerDead;
    bool facingRight;
    bool wasInWater;         // for water splash detection
    float footstepTimer;     // for footstep sound intervals
    float fallPeakVel;       // peak downward velocity during current fall
} Player;

//----------------------------------------------------------------------------------
// Particle System
//----------------------------------------------------------------------------------
#define MAX_PARTICLES       256
#define PARTICLE_GRAVITY    400.0f

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
    float maxLifetime;
    float size;
    bool active;
} Particle;

//----------------------------------------------------------------------------------
// Item Entity System
//----------------------------------------------------------------------------------
typedef struct {
    uint8_t itemType;
    int count;
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    float pickupDelay;
    bool active;
} ItemEntity;

//----------------------------------------------------------------------------------
// Game State
//----------------------------------------------------------------------------------
typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING
} GameState;

typedef struct {
    int chunkX;
    Texture2D texture;
    bool textureValid;
    int waterTopY[CHUNK_SIZE]; // Topmost water Y per column, -1 if none
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
extern uint8_t lightMap[WORLD_WIDTH][WORLD_HEIGHT];
extern Chunk loadedChunks[MAX_CHUNKS];

extern Player player;
extern Camera2D camera;
extern DayNightCycle dayNight;

extern Texture2D blockAtlas;
extern bool showDebug;
extern bool inventoryOpen;
extern bool gamePaused;
extern bool audioReady;
extern unsigned int worldSeed;

extern char messageText[128];
extern float messageTimer;
extern Color messageColor;

extern Mob mobs[MAX_MOBS];
extern float mobSpawnTimer;

extern Particle particles[MAX_PARTICLES];
extern ItemEntity entities[MAX_ENTITIES];
extern GameState gameState;
extern int menuSelection;

extern Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
extern Sound sndHurt, sndDeath, sndEat, sndClick, sndCraft, sndXP, sndDrop;
extern Sound sndFootstep, sndZombie, sndPig, sndSplash;
extern Music bgm;

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
void UnloadChunk(int chunkX);
void GenerateChunkTexture(Chunk *chunk);
void InvalidateChunkAt(int worldBlockX, int worldBlockY);
void UpdateChunks(void);
bool IsBlockSolid(int bx, int by);

// light.c
void InitLightMap(void);
void CalculateSunlight(void);
void PropagateLight(int startX, int startY, int level);
void RemoveLight(int startX, int startY);
void UpdateLightAt(int bx, int by);
void RecalculateAllLight(void);
uint8_t GetLightLevel(int bx, int by);
Color ApplyLighting(Color base, int bx, int by);

// player.c
void InitPlayer(void);
bool AddToInventory(BlockType item);
float GetToolMiningSpeed(BlockType tool, BlockType block);
bool IsTool(BlockType item);
int GetToolMaxDurability(BlockType tool);
bool IsFood(BlockType item);
int GetFoodValue(BlockType item);
void PlayerPhysics(float dt);
void PlayerBlockInteraction(void);
void UpdatePlayer(float dt);
void UpdatePlayerStatus(float dt);
void RespawnPlayer(void);
void InitCameraSystem(void);
void UpdateCameraSystem(float dt);
void UpdateHotbar(void);
float GetMiningProgress(void);
int GetMiningBlockX(void);
int GetMiningBlockY(void);
bool IsPlayerUnderwater(void);

// daynight.c
void InitDayNight(void);
void UpdateDayNight(float dt);
Color GetSkyColor(void);

// rendering.c
void DrawWorld(void);
void DrawWater(void);
void DrawPlayerSprite(void);
void DrawHotbar(void);
void DrawPlayerStatus(void);
void DrawCrosshair(void);
void DrawDebugInfo(void);
void DrawInventoryScreen(void);
void DrawMessage(void);
void DrawPauseMenu(void);
void DrawDeathScreen(float dt);
float GetDeathFadeTimer(void);
void DrawMainMenu(void);
void DrawBackground(void);
void ReturnHeldItem(void);
void ShowMessage(const char *msg, Color color);

// save.c
bool SaveExists(const char *path);
bool SaveWorld(const char *path);
bool LoadWorld(const char *path);

// crafting.c
void InitCraftingRecipes(void);
bool CanCraft(int recipeIndex);
void Craft(int recipeIndex);
void DrawCraftingPanel(int panelX, int panelY, int panelW, int visibleCount, int slotH, int pad);

// mob.c
void InitMobs(void);
void UpdateMobs(float dt);
void DrawMobs(void);
Mob* SpawnMob(MobType type, float x, float y);
void DamageMob(Mob *mob, int damage);
bool IsPlayerNearMob(Mob *mob, float range);

// sound.c
void InitSounds(void);
void UnloadSounds(void);
void PlaySoundBreak(BlockType block);
void PlaySoundPlace(BlockType block);
void PlaySoundJump(void);
void PlaySoundLand(void);
void PlaySoundHurt(void);
void PlaySoundDeath(void);
void PlaySoundEat(void);
void PlaySoundUIClick(void);
void PlaySoundCraft(void);
void PlaySoundXP(void);
void PlaySoundDrop(void);
void PlaySoundFootstep(void);
void PlaySoundMob(MobType type);
void PlaySoundSplash(void);
void UpdateBGM(void);
void SetBGMVolume(float volume);
void LoadSoundsDeferred(void);

// particles.c
void InitParticles(void);
void SpawnBlockParticles(int blockX, int blockY, BlockType block);
void SpawnDamageParticles(float x, float y, Color color);
void UpdateParticles(float dt);
void DrawParticles(void);

// entities.c
void InitEntities(void);
void SpawnItemEntity(uint8_t itemType, int count, float x, float y);
void UpdateEntities(float dt);
void DrawEntities(void);
void PickupNearbyItems(float px, float py);

// game.c
void InitGame(void);
void UpdateGame(float dt);
void DrawGame(void);
void UnloadGame(void);
void UpdateDrawFrame(void);

#endif // TYPES_H
