#ifndef TYPES_H
#define TYPES_H

#include <raylib.h>
#include <raymath.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

//----------------------------------------------------------------------------------
// Win32 Input Override (bypasses broken GLFW input on some systems)
//----------------------------------------------------------------------------------
#ifdef _WIN32
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef long LONG;
__declspec(dllimport) BOOL __stdcall GetCursorPos(LONG*);
__declspec(dllimport) BOOL __stdcall ScreenToClient(void*, LONG*);
__declspec(dllimport) short __stdcall GetAsyncKeyState(int);
__declspec(dllimport) void* __stdcall GetForegroundWindow(void);
__declspec(dllimport) void* __stdcall GetFocus(void);
#ifndef VK_LBUTTON
#define VK_LBUTTON  0x01
#define VK_RBUTTON  0x02
#define VK_MBUTTON  0x04
#define VK_BACK     0x08
#define VK_TAB      0x09
#define VK_RETURN   0x0D
#define VK_SHIFT    0x10
#define VK_CONTROL  0x11
#define VK_MENU     0x12
#define VK_ESCAPE   0x1B
#define VK_SPACE    0x20
#define VK_DELETE   0x2E
#define VK_LEFT     0x25
#define VK_UP       0x26
#define VK_RIGHT    0x27
#define VK_DOWN     0x28
#define VK_F3       0x72
#define VK_F11      0x7A
#endif
#endif

// Win32 input state (updated each frame by UpdateWin32Input)
extern int win32MouseX;
extern int win32MouseY;
extern int win32MousePrevX;
extern int win32MousePrevY;
extern bool win32LMB;
extern bool win32LMBPrev;
extern bool win32RMB;
extern bool win32RMBPrev;

// Win32 input functions
void UpdateWin32Input(void);
Vector2 Win32GetMousePosition(void);
Vector2 Win32GetMouseDelta(void);
bool Win32IsMouseButtonPressed(int button);
bool Win32IsMouseButtonReleased(int button);
bool Win32IsKeyPressed(int key);
bool Win32IsKeyDown(int key);
int Win32GetCharPressed(void);
float Win32GetMouseWheelMove(void);
void InitWin32WheelHook(void);

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

#define SEA_LEVEL           112
#define TERRAIN_BASE        100
#define TERRAIN_AMPLITUDE   35
#define CAVE_START          140
#define CAVE_END            240

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

#define MAX_CRAFT_RECIPES   64
#define MAX_SMELT_RECIPES   8

#define SAVE_MAGIC          "MWSV"
#define SAVE_VERSION        4
#define MAX_SAVE_SLOTS      8
#define SLOT_VISIBLE        4
#define SAVE_DIR            "saves"

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

// Armor durability
#define ARMOR_DURABILITY_WOOD    55
#define ARMOR_DURABILITY_STONE   160
#define ARMOR_DURABILITY_IRON    300

// Food restoration values (hunger points)
#define FOOD_RAW_PORK_VALUE    3
#define FOOD_COOKED_PORK_VALUE 8

// Minimap
#define MINIMAP_SIZE        100
#define MINIMAP_SCALE       2    // pixels per block
#define MINIMAP_RANGE       50   // blocks visible around player
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
#define MAX_PROJECTILES     32
#define PROJECTILE_SPEED    200.0f
#define PROJECTILE_DAMAGE   3
#define PROJECTILE_LIFETIME 3.0f
#define ARROW_GRAVITY       400.0f

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
    BLOCK_BED,
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
    // Interactive blocks
    BLOCK_CRAFTING_TABLE,
    // Armor (not placeable)
    ARMOR_WOOD_HELMET,
    ARMOR_WOOD_CHESTPLATE,
    ARMOR_WOOD_LEGGINGS,
    ARMOR_WOOD_BOOTS,
    ARMOR_STONE_HELMET,
    ARMOR_STONE_CHESTPLATE,
    ARMOR_STONE_LEGGINGS,
    ARMOR_STONE_BOOTS,
    ARMOR_IRON_HELMET,
    ARMOR_IRON_CHESTPLATE,
    ARMOR_IRON_LEGGINGS,
    ARMOR_IRON_BOOTS,
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
// Language & i18n
//----------------------------------------------------------------------------------
typedef enum {
    LANG_EN = 0,
    LANG_ZH_CN,
    LANG_JA,
    LANG_COUNT
} Language;

typedef enum {
    STR_NONE = 0,

    // Main Menu
    STR_TITLE,
    STR_SUBTITLE,
    STR_BTN_NEW_GAME,
    STR_BTN_LOAD_GAME,
    STR_BTN_SETTINGS,
    STR_BTN_QUIT,
    STR_HINT_NAVIGATE,
    STR_HINT_CONTROLS,

    // Slot Select
    STR_NEW_GAME_TITLE,
    STR_LOAD_GAME_TITLE,
    STR_NEW_GAME_SUB,
    STR_LOAD_GAME_SUB,
    STR_SEED,
    STR_RANDOM,
    STR_SLOT,
    STR_SEED_DISPLAY,
    STR_OCCUPIED,
    STR_EMPTY_NEW,
    STR_EMPTY_LOAD,
    STR_BACK,
    STR_HINT_SLOT,

    // Confirm Dialog
    STR_DELETE_SAVE,
    STR_OVERWRITE_SAVE,
    STR_SLOT_HAS_DATA,
    STR_CANNOT_UNDO,
    STR_YES_DELETE,
    STR_YES_OVERWRITE,
    STR_CANCEL,
    STR_CONFIRM_KEYS,

    // Pause Menu
    STR_PAUSED,
    STR_MUSIC_VOLUME,
    STR_SFX_VOLUME,
    STR_CONTROLS_TITLE,
    STR_CONTINUE,
    STR_MAIN_MENU,

    // Control Keys
    STR_KEY_WASD,
    STR_KEY_SPACE,
    STR_KEY_SHIFT,
    STR_KEY_LCLICK,
    STR_KEY_RCLICK,
    STR_KEY_E,
    STR_KEY_H,
    STR_KEY_F3,
    STR_KEY_ESC,
    STR_KEY_19,

    // Control Actions
    STR_ACT_MOVE,
    STR_ACT_JUMP,
    STR_ACT_SPRINT,
    STR_ACT_BREAK,
    STR_ACT_PLACE,
    STR_ACT_INVENTORY,
    STR_ACT_HEAL,
    STR_ACT_DEBUG,
    STR_ACT_PAUSE,
    STR_ACT_HOTBAR,

    // Death Screen
    STR_YOU_DIED,
    STR_PRESS_SPACE_RESPAWN,
    STR_PRESS_ESC_MENU,

    // Inventory
    STR_INVENTORY,
    STR_SORT,

    // Map
    STR_WORLD_MAP,
    STR_PRESS_M_CLOSE,
    STR_PLAYER_COORD,

    // Crafting
    STR_CRAFTING,
    STR_CRAFTING_TABLE,
    STR_SEARCH,
    STR_NO_MATCHES,
    STR_NO_RECIPES,

    // Furnace
    STR_FURNACE,
    STR_FUEL,
    STR_INPUT,
    STR_OUTPUT,
    STR_PRESS_E_ESC_CLOSE,

    // Settings
    STR_SETTINGS,
    STR_SECTION_AUDIO,
    STR_SOUND_EFFECTS,
    STR_SECTION_DISPLAY,
    STR_WINDOW_MODE,
    STR_WINDOWED,
    STR_FULLSCREEN,
    STR_BORDERLESS,
    STR_SECTION_LANGUAGE,
    STR_LANGUAGE,
    STR_SECTION_FONT,
    STR_FONT,
    STR_FONT_BUILTIN,
    STR_FONT_CUSTOM,

    // Language names (for selector buttons)
    STR_LANG_NAME_EN,
    STR_LANG_NAME_ZH,
    STR_LANG_NAME_JA,

    // Font names
    STR_FONT_NAME_BUILTIN,
    STR_FONT_NAME_LXGW,

    // Status Messages
    STR_MSG_GAME_SAVED,
    STR_MSG_HEALED_XP,
    STR_MSG_NOT_ENOUGH_XP,
    STR_MSG_INVENTORY_FULL,
    STR_MSG_TOOL_BROKE,
    STR_MSG_TOOL_WEARING,
    STR_MSG_FALL_DAMAGE,
    STR_MSG_SPAWN_SET,
    STR_MSG_HUNGRY,
    STR_MSG_STARVING,
    STR_MSG_ATE,
    STR_MSG_BROKE,

    // Tooltips
    STR_TOOLTIP_ARMOR,
    STR_TOOLTIP_HUNGER,
    STR_TOOLTIP_DURABILITY,
    STR_TOOLTIP_DUR_SHORT,

    // Debug
    STR_DBG_FPS,
    STR_DBG_POS,
    STR_DBG_BLOCK,
    STR_DBG_CHUNKS,
    STR_DBG_TIME,
    STR_DBG_LIGHT,
    STR_DBG_GROUND,
    STR_DBG_HP,
    STR_DBG_OXYGEN,
    STR_DBG_UNDERWATER,
    STR_DBG_SEED,
    STR_YES,
    STR_NO,

    // Block/Item Names (order matches BlockType enum exactly)
    STR_BLOCK_AIR,
    STR_BLOCK_GRASS,
    STR_BLOCK_DIRT,
    STR_BLOCK_STONE,
    STR_BLOCK_COBBLESTONE,
    STR_BLOCK_WOOD,
    STR_BLOCK_LEAVES,
    STR_BLOCK_SAND,
    STR_BLOCK_WATER,
    STR_BLOCK_COAL_ORE,
    STR_BLOCK_IRON_ORE,
    STR_BLOCK_PLANKS,
    STR_BLOCK_BRICK,
    STR_BLOCK_GLASS,
    STR_BLOCK_BEDROCK,
    STR_BLOCK_GRAVEL,
    STR_BLOCK_CLAY,
    STR_BLOCK_SANDSTONE,
    STR_BLOCK_TORCH,
    STR_BLOCK_FLOWER,
    STR_BLOCK_TALL_GRASS,
    STR_BLOCK_FURNACE,
    STR_BLOCK_BED,
    STR_ITEM_STICK,
    STR_ITEM_COAL,
    STR_ITEM_IRON_INGOT,
    STR_TOOL_WOOD_PICKAXE,
    STR_TOOL_WOOD_AXE,
    STR_TOOL_WOOD_SWORD,
    STR_TOOL_WOOD_SHOVEL,
    STR_TOOL_STONE_PICKAXE,
    STR_TOOL_STONE_AXE,
    STR_TOOL_STONE_SWORD,
    STR_TOOL_STONE_SHOVEL,
    STR_TOOL_IRON_PICKAXE,
    STR_TOOL_IRON_AXE,
    STR_TOOL_IRON_SWORD,
    STR_TOOL_IRON_SHOVEL,
    STR_FOOD_RAW_PORK,
    STR_FOOD_COOKED_PORK,
    STR_FOOD_APPLE,
    STR_FOOD_BREAD,
    STR_BLOCK_CRAFTING_TABLE,
    STR_ARMOR_WOOD_HELMET,
    STR_ARMOR_WOOD_CHESTPLATE,
    STR_ARMOR_WOOD_LEGGINGS,
    STR_ARMOR_WOOD_BOOTS,
    STR_ARMOR_STONE_HELMET,
    STR_ARMOR_STONE_CHESTPLATE,
    STR_ARMOR_STONE_LEGGINGS,
    STR_ARMOR_STONE_BOOTS,
    STR_ARMOR_IRON_HELMET,
    STR_ARMOR_IRON_CHESTPLATE,
    STR_ARMOR_IRON_LEGGINGS,
    STR_ARMOR_IRON_BOOTS,

    // Recipe Names
    STR_RECIPE_WOOD_PLANKS,
    STR_RECIPE_PLANKS_STICKS,
    STR_RECIPE_WOOD_PICK,
    STR_RECIPE_WOOD_AXE,
    STR_RECIPE_WOOD_SWORD,
    STR_RECIPE_WOOD_SHOVEL,
    STR_RECIPE_STONE_PICK,
    STR_RECIPE_STONE_AXE,
    STR_RECIPE_STONE_SWORD,
    STR_RECIPE_STONE_SHOVEL,
    STR_RECIPE_IRON_PICK,
    STR_RECIPE_IRON_AXE,
    STR_RECIPE_IRON_SWORD,
    STR_RECIPE_IRON_SHOVEL,
    STR_RECIPE_BRICK,
    STR_RECIPE_FURNACE,
    STR_RECIPE_TORCHES,
    STR_RECIPE_SANDSTONE,
    STR_RECIPE_COBBLE,
    STR_RECIPE_BED,
    STR_RECIPE_CRAFTING_TABLE,
    STR_RECIPE_BREAD,
    STR_RECIPE_APPLE,
    // Advanced armor
    STR_RECIPE_WOOD_HELMET,
    STR_RECIPE_WOOD_CHEST,
    STR_RECIPE_WOOD_LEGS,
    STR_RECIPE_WOOD_BOOTS,
    STR_RECIPE_STONE_HELMET,
    STR_RECIPE_STONE_CHEST,
    STR_RECIPE_STONE_LEGS,
    STR_RECIPE_STONE_BOOTS,
    STR_RECIPE_IRON_HELMET,
    STR_RECIPE_IRON_CHEST,
    STR_RECIPE_IRON_LEGS,
    STR_RECIPE_IRON_BOOTS,
    // Smelt
    STR_SMELT_IRON,
    STR_SMELT_PORK,
    STR_SMELT_COBBLE,
    STR_SMELT_SAND,

    // Furnace messages
    STR_MSG_NO_FUEL,
    STR_MSG_SMELTING,
    STR_MSG_CANNOT_SMELT,

    // Additional controls
    STR_KEY_F11,
    STR_ACT_FULLSCREEN,

    STR_COUNT
} StringId;

//----------------------------------------------------------------------------------
// Crafting
//----------------------------------------------------------------------------------
typedef struct {
    BlockType input;
    int inputCount;
    BlockType output;
    int outputCount;
    StringId nameId;
    bool advanced;          // true = requires crafting table
} CraftingRecipe;

typedef struct {
    BlockType input;
    BlockType output;
    StringId nameId;
} SmeltRecipe;

//----------------------------------------------------------------------------------
// Mob System
//----------------------------------------------------------------------------------
typedef enum {
    MOB_NONE = 0,
    MOB_PIG,
    MOB_ZOMBIE,
    MOB_SKELETON,
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
    float attackTimer;  // cooldown for ranged attacks
    bool active;
} Mob;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
} Projectile;

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
    int spawnX, spawnY;      // bed spawn point (-1 = use default)
    // Armor slots: 0=helmet, 1=chestplate, 2=leggings, 3=boots
    uint8_t armor[4];
    int armorDurability[4];
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
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_SLOT_SELECT
} GameState;

typedef struct {
    bool exists;
    unsigned int seed;
    int worldW, worldH;
} SaveSlotInfo;

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
extern bool showLargeMap;
extern bool inventoryOpen;
extern bool gamePaused;
extern bool audioReady;
extern unsigned int worldSeed;

extern char messageText[128];
extern float messageTimer;
extern Color messageColor;

extern Mob mobs[MAX_MOBS];
extern Projectile projectiles[MAX_PROJECTILES];
extern float mobSpawnTimer;

extern Particle particles[MAX_PARTICLES];
extern ItemEntity entities[MAX_ENTITIES];
extern GameState gameState;

// Crafting search
extern char craftSearchBuf[32];
extern int craftSearchLen;
extern float bgmVolumeSlider;
extern float sfxVolumeSlider;
extern int menuSelection;
extern int selectedSaveSlot;
extern int slotSelectMode; // 0=new game, 1=load game
extern char currentSavePath[256];
extern int slotScrollOffset;
extern int windowMode; // 0=windowed, 1=fullscreen, 2=borderless
extern char seedInputBuf[32];
extern int seedInputLen;

// Confirmation dialog
extern bool confirmDialogActive;
extern int confirmDialogSlot;
extern int confirmDialogMode; // 0=overwrite, 1=delete

extern Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
extern Sound sndHurt, sndDeath, sndEat, sndClick, sndCraft, sndXP, sndDrop;
extern Sound sndFootstep, sndZombie, sndPig, sndSplash;
extern Music bgm;

extern const BlockInfo blockInfo[BLOCK_COUNT];
extern CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
extern int craftRecipeCount;

// Furnace UI state
extern bool furnaceOpen;
extern int furnaceBlockX, furnaceBlockY;
extern uint8_t furnaceFuel;
extern int furnaceFuelCount;
extern uint8_t furnaceInput;
extern int furnaceInputCount;
extern uint8_t furnaceOutput;
extern int furnaceOutputCount;
extern float furnaceProgress;
extern float furnaceFuelBurn;

// Drag-and-drop held item (shared between rendering.c and crafting.c)
extern uint8_t heldItem;
extern int heldCount;
extern int heldDurability;

// Crafting table state
extern bool craftingTableOpen;

// Smelting recipes
extern SmeltRecipe smeltRecipes[MAX_SMELT_RECIPES];
extern int smeltRecipeCount;

// i18n / Font
extern Language language;
extern Font gameFont;
extern bool useCustomFont;
extern char customFontPath[256];

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
bool IsArmor(BlockType item);
int GetArmorValue(BlockType item);
int GetArmorMaxDurability(BlockType item);
int GetTotalArmorPoints(void);
float GetArmorDamageReduction(void);
void DamageArmor(void);
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
void SortInventory(void);
void DrawMessage(void);
void DrawPauseMenu(void);
void DrawDeathScreen(float dt);
float GetDeathFadeTimer(void);
void DrawMainMenu(void);
void DrawSlotSelectScreen(void);
void DrawConfirmDialog(void);
void DrawBackground(void);
void DrawMinimap(void);
void DrawLargeMap(void);
void DrawSettingsScreen(void);
void ReturnHeldItem(void);
void ShowMessage(const char *msg, Color color);

// save.c
bool SaveExists(const char *path);
bool SaveWorld(const char *path);
bool LoadWorld(const char *path);
void GetSavePath(int slot, char *buf, int bufSize);
bool GetSlotInfo(int slot, SaveSlotInfo *info);
void DeleteSaveSlot(int slot);

// crafting.c
void InitCraftingRecipes(void);
bool CanCraft(int recipeIndex);
void Craft(int recipeIndex);
void DrawCraftingPanel(int panelX, int panelY, int panelW, int visibleCount, int slotH, int pad, bool showAdvanced);
void InitSmeltingRecipes(void);
int FindSmeltRecipe(BlockType input);
void DrawFurnaceUI(void);
void ReturnFurnaceItems(void);

// mob.c
void InitMobs(void);
void UpdateMobs(float dt);
void DrawMobs(void);
Mob* SpawnMob(MobType type, float x, float y);
void InitProjectiles(void);
void SpawnProjectile(float x, float y, float vx, float vy);
void UpdateProjectiles(float dt);
void DrawProjectiles(void);
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
void SetSFXVolume(float volume);
void UpdateBGM(void);
void SetBGMVolume(float volume);

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
void ApplyWindowMode(int mode);
void LoadSettings(void);

// Smooth hover animation
float GetHoverAlpha(int slotId, bool hovered, float dt);

// Screen transitions
typedef enum {
    TRANSITION_NONE = 0,
    TRANSITION_FADE_OUT,
    TRANSITION_FADE_IN
} TransitionState;

extern TransitionState transitionState;
extern float transitionAlpha;
extern GameState transitionTarget;

void StartTransition(GameState target);
void UpdateTransition(float dt);
void DrawTransition(void);
bool IsTransitioning(void);

// i18n.c
const char *S(StringId id);
const char *Sf(StringId id, ...);
const char *GetBlockName(BlockType bt);
void LoadGameFont(void);
void UnloadGameFont(void);
bool ReloadGameFont(const char *path);
void DrawGameText(const char *text, int posX, int posY, int fsize, Color color);
Vector2 MeasureGameText(const char *text, int fsize);
int MeasureGameTextWidth(const char *text, int fsize);

#endif // TYPES_H
