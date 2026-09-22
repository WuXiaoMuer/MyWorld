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
__declspec(dllimport) short __stdcall GetKeyState(int);
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
#define VK_OEM_PERIOD 0xBE
#define VK_OEM_2    0xBF
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
bool IsMoveLeftDown(void);
bool IsMoveRightDown(void);
bool IsJumpDown(void);
bool IsJumpPressed(void);
bool IsSprintDown(void);
bool IsSneakDown(void);
Vector2 Win32GetMousePosition(void);
Vector2 Win32GetMouseDelta(void);
bool Win32IsMouseButtonPressed(int button);
bool Win32IsMouseButtonReleased(int button);
bool Win32IsMouseButtonDown(int button);
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
#define SIM_TICK_RATE       20
#define SIM_TICK_DT         (1.0f / SIM_TICK_RATE)

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
#define ABYSS_START         236
#define ABYSS_END           254   // abyss band bottom (bedrock at 255)

#define PLAYER_WIDTH        12
#define PLAYER_HEIGHT       28
#define GRAVITY             980.0f
#define JUMP_VELOCITY       -380.0f
#define MOVE_SPEED          120.0f
#define SPRINT_SPEED_MULT   1.6f
#define SNEAK_SPEED_MULT    0.3f

// Player feel
#define COYOTE_TIME         0.1f
#define JUMP_BUFFER_TIME    0.1f
#define MOVE_ACCEL          400.0f
#define MOVE_DECEL          350.0f
#define JUMP_CUT_MULT       0.4f
#define CAMERA_LOOKAHEAD    40.0f
#define CAMERA_SHAKE_DECAY  12.0f

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

#define MAX_CRAFT_RECIPES   128
#define MAX_SMELT_RECIPES   20
#define MAX_CHESTS          64
#define CHEST_SLOTS         27
#define MAX_FURNACES        64

#define MAX_NET_PLAYERS     4

#define SAVE_MAGIC          "MWSV"
#define SAVE_VERSION        19
#define MAX_SAVED_LEVERS    1024   // cap on levers persisted per save (sparse x,y list)

// Number of world dimensions. Only the overworld exists today; the save format
// and block-access layer are already dimension-aware so this can grow later.
#define CURRENT_DIMENSION_COUNT 1
#define MAX_SAVE_SLOTS      8
#define SLOT_VISIBLE        4
#define SAVE_DIR            "saves"

#define DEATH_Y             (WORLD_HEIGHT * BLOCK_SIZE + 500)
#define MESSAGE_DURATION    2.0f
#define DEATH_RESPAWN_BUTTON_W  300
#define DEATH_RESPAWN_BUTTON_H  48
#define DEATH_RESPAWN_BUTTON_Y  (SCREEN_HEIGHT / 2 + 28)
#define DEATH_RESPAWN_READY_TIME 1.0f
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
#define DURABILITY_GOLD     48
#define DURABILITY_DIAMOND  800
#define DURABILITY_BOW      385

// Armor durability
#define ARMOR_DURABILITY_WOOD    55
#define ARMOR_DURABILITY_STONE   160
#define ARMOR_DURABILITY_IRON    300
#define ARMOR_DURABILITY_GOLD    175
#define ARMOR_DURABILITY_DIAMOND 500

// Food restoration values (hunger points)
#define FOOD_RAW_PORK_VALUE    3
#define FOOD_COOKED_PORK_VALUE 8

// Minimap
#define MINIMAP_SIZE        100
#define MINIMAP_SCALE       2    // pixels per block
#define MINIMAP_RANGE       50   // blocks visible around player
#define FOOD_APPLE_VALUE       4
#define FOOD_BREAD_VALUE       5
#define FOOD_RAW_BEEF_VALUE    3
#define FOOD_COOKED_BEEF_VALUE 8
#define FOOD_RAW_MUTTON_VALUE    3
#define FOOD_COOKED_MUTTON_VALUE 8
#define FOOD_RAW_CHICKEN_VALUE    2
#define FOOD_COOKED_CHICKEN_VALUE 6
#define FOOD_RAW_FISH_VALUE       2
#define FOOD_COOKED_FISH_VALUE    5

// Mob system
#define MAX_MOBS            64

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
#define MOB_DEATH_TIME      0.5f
#define MOB_DESPAWN_TIME    300.0f  // 5 minutes
#define MOB_DESPAWN_ENGAGE  200.0f  // reset timer when player within this range
#define MOB_GRAVITY         980.0f
#define MAX_PROJECTILES     32
#define PROJECTILE_SPEED    200.0f
#define PROJECTILE_DAMAGE   3
#define FISHING_ROD_MAX_RANGE   300.0f
#define FISHING_MIN_DELAY       5.0f
#define FISHING_MAX_DELAY       30.0f
#define PROJECTILE_LIFETIME 3.0f
#define ARROW_GRAVITY       400.0f
#define BOW_CHARGE_MAX      1.5f    // seconds to full charge

// Creeper explosion
#define CREEPER_FUSE_TIME    1.5f
#define CREEPER_EXPLODE_DIST 120.0f
#define CREEPER_EXPLODE_RADIUS 3
#define CREEPER_DAMAGE       14

// Spider
#define SPIDER_WALL_CLIMB_VEL -120.0f

// Weather system
#define MAX_RAIN_DROPS      200
#define WEATHER_MIN_DURATION 60.0f
#define WEATHER_MAX_DURATION 180.0f
#define LIGHTNING_CHANCE    0.002f

// Light system
#define MAX_LIGHT_LEVEL     15
#define TORCH_LIGHT         15
#define SUNLIGHT_LEVEL      15

// Attack speeds (seconds between attacks)
#define ATTACK_SPEED_SWORD  0.6f
#define ATTACK_SPEED_AXE    0.8f
#define ATTACK_SPEED_PICK   1.0f
#define ATTACK_SPEED_SHOVEL 1.0f
#define ATTACK_SPEED_BARE   0.4f
#define CRIT_FALL_THRESHOLD 200.0f
#define CRIT_DAMAGE_MULT    1.5f

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
    TOOL_WOOD_HOE,
    TOOL_STONE_PICKAXE,
    TOOL_STONE_AXE,
    TOOL_STONE_SWORD,
    TOOL_STONE_SHOVEL,
    TOOL_STONE_HOE,
    TOOL_IRON_PICKAXE,
    TOOL_IRON_AXE,
    TOOL_IRON_SWORD,
    TOOL_IRON_SHOVEL,
    TOOL_IRON_HOE,
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
    // New ores
    BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE,
    BLOCK_REDSTONE_ORE,
    BLOCK_LAPIS_ORE,
    // New items
    ITEM_GOLD_INGOT,
    ITEM_DIAMOND,
    ITEM_REDSTONE,
    ITEM_LAPIS,
    // New interactive blocks
    BLOCK_CHEST,
    // Gold tools
    TOOL_GOLD_PICKAXE,
    TOOL_GOLD_AXE,
    TOOL_GOLD_SWORD,
    TOOL_GOLD_SHOVEL,
    TOOL_GOLD_HOE,
    // Diamond tools
    TOOL_DIAMOND_PICKAXE,
    TOOL_DIAMOND_AXE,
    TOOL_DIAMOND_SWORD,
    TOOL_DIAMOND_SHOVEL,
    TOOL_DIAMOND_HOE,
    // Gold armor
    ARMOR_GOLD_HELMET,
    ARMOR_GOLD_CHESTPLATE,
    ARMOR_GOLD_LEGGINGS,
    ARMOR_GOLD_BOOTS,
    // Diamond armor
    ARMOR_DIAMOND_HELMET,
    ARMOR_DIAMOND_CHESTPLATE,
    ARMOR_DIAMOND_LEGGINGS,
    ARMOR_DIAMOND_BOOTS,
    // Mob drops
    ITEM_GUNPOWDER,
    ITEM_STRING,
    // New items
    ITEM_BONE,
    ITEM_ARROW,
    ITEM_BOW,
    // New blocks
    BLOCK_MOSSY_COBBLESTONE,
    BLOCK_BOOKSHELF,
    BLOCK_LANTERN,
    BLOCK_BONE_BLOCK,
    // Phase 1: Biome blocks
    BLOCK_SNOW,
    BLOCK_ICE,
    BLOCK_PACKED_ICE,
    BLOCK_MUD,
    BLOCK_MOSS_BLOCK,
    BLOCK_JUNGLE_WOOD,
    BLOCK_JUNGLE_LEAVES,
    BLOCK_VINE,
    BLOCK_PUMPKIN,
    BLOCK_MELON,
    BLOCK_SNOWY_GRASS,
    BLOCK_COARSE_DIRT,
    BLOCK_PODZOL,
    // Phase 1: New items
    ITEM_SLIMEBALL,
    ITEM_ENDER_PEARL,
    // Farming
    ITEM_WHEAT_SEEDS,
    ITEM_WHEAT,
    BLOCK_FARMLAND,
    BLOCK_CROPS,        // Growing wheat (0-7 growth stages)
    BLOCK_HAY_BALE,
    // Animal drops
    ITEM_RAW_BEEF,
    ITEM_LEATHER,
    ITEM_RAW_MUTTON,
    ITEM_WOOL,
    ITEM_RAW_CHICKEN,
    ITEM_FEATHER,
    ITEM_EGG,
    // Cooked food
    ITEM_COOKED_BEEF,
    ITEM_COOKED_MUTTON,
    ITEM_COOKED_CHICKEN,
    // Utility items
    ITEM_BUCKET,
    ITEM_WATER_BUCKET,
    // Redstone blocks
    BLOCK_LEVER,
    BLOCK_REDSTONE_WIRE,
    BLOCK_REDSTONE_LAMP,
    BLOCK_STONE_PRESSURE_PLATE,
    // Lava system
    BLOCK_LAVA,
    BLOCK_OBSIDIAN,
    ITEM_LAVA_BUCKET,
    // Enchanting
    BLOCK_ENCHANTING_TABLE,
    // Fishing
    ITEM_FISHING_ROD,
    ITEM_RAW_FISH,
    ITEM_COOKED_FISH,
    // Cauldron
    BLOCK_CAULDRON,
    // Decorative blocks
    BLOCK_OAK_STAIRS,
    BLOCK_COBBLESTONE_STAIRS,
    BLOCK_STONE_BRICKS,
    BLOCK_CHISELED_STONE_BRICKS,
    BLOCK_OAK_SLAB,
    BLOCK_COBBLESTONE_SLAB,
    // New plants & items
    BLOCK_CACTUS,
    BLOCK_SUGAR_CANE,
    ITEM_PAPER,
    ITEM_BOOK,
    ITEM_SUGAR,
    BLOCK_TNT,          // explosive block; ignite by right-click or redstone
    // Phase 2: new biome blocks
    BLOCK_RED_SAND,
    BLOCK_MYCELIUM,
    BLOCK_MUSHROOM_BLOCK,   // giant mushroom cap (red with white spots)
    BLOCK_MUSHROOM_STEM,    // giant mushroom stalk
    // Redstone additions
    BLOCK_STONE_BUTTON,     // momentary pulse source
    BLOCK_REDSTONE_REPEATER,// refreshes signal strength, adds delay
    BLOCK_PISTON,           // pushes the block in front when powered
    BLOCK_IRON_DOOR,        // opens while powered
    // Potions & bottles
    ITEM_GLASS_BOTTLE,
    ITEM_POTION_WATER,
    ITEM_POTION_SPEED,
    ITEM_POTION_STRENGTH,
    ITEM_POTION_REGEN,
    ITEM_POTION_FIRE_RESISTANCE,
    ITEM_POTION_WATER_BREATHING,
    ITEM_POTION_POISON,
    BLOCK_BREWING_STAND,    // brews potions from a water bottle + ingredient
    // Abyss layer (deep caves, y 236-254)
    BLOCK_ABYSS_STONE,
    BLOCK_ABYSS_CRYSTAL_ORE,
    BLOCK_GLOWSHROOM,       // light-emitting cave plant
    ITEM_ABYSS_CRYSTAL,     // deep-tier material (boss altar later)
    BLOCK_ABYSS_ALTAR,      // summon the Abyss Warden (abyss layer only)
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

// Status effects (potions, beacons, boss debuffs all use this)
typedef enum {
    EFFECT_SPEED,
    EFFECT_STRENGTH,
    EFFECT_REGEN,
    EFFECT_FIRE_RESISTANCE,
    EFFECT_WATER_BREATHING,
    EFFECT_POISON,
    EFFECT_COUNT
} EffectType;

#define MAX_PLAYER_EFFECTS 8
typedef struct { uint8_t type; uint8_t level; float time; } StatusEffect;

typedef struct {
    int x, y;
    uint8_t items[CHEST_SLOTS];
    int counts[CHEST_SLOTS];
    int durability[CHEST_SLOTS];      // tool durability per slot (v13+)
    uint16_t enchantments[CHEST_SLOTS]; // packed enchantment per slot (v13+)
} ChestData;

typedef struct {
    int x, y;
    uint8_t fuel; int fuelCount;
    uint8_t input; int inputCount;
    uint8_t output; int outputCount;
    float progress;
    float fuelBurn;
    float fuelBurnMax;
} FurnaceData;
typedef struct {
    int x, y;
    uint8_t bottle; int bottleCount;
    uint8_t ingredient; int ingredientCount;
    uint8_t output; int outputCount;
    float progress;
} BrewingData;

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
    STR_BTN_HOST_GAME,
    STR_BTN_JOIN_GAME,
    STR_HINT_NAVIGATE,
    STR_HINT_CONTROLS,

    // Slot Select
    STR_NEW_GAME_TITLE,
    STR_LOAD_GAME_TITLE,
    STR_NEW_GAME_SUB,
    STR_LOAD_GAME_SUB,
    STR_SEED,
    STR_RANDOM,
    STR_GENERATING_WORLD,
    STR_SLOT,
    STR_SEED_DISPLAY,
    STR_HOST_WAITING,
    STR_HOST_IP_HINT,
    STR_JOIN_TITLE,
    STR_JOIN_IP_HINT,
    STR_JOIN_CONNECTING,
    STR_HOST_PLAYERS_COUNT,
    STR_HOST_CANCEL_HINT,
    STR_HOST_START_HINT,
    STR_JOIN_HELP_HINT,
    STR_JOIN_TAB_HINT,
    STR_OPEN_TO_LAN,
    STR_LAN_OPENED,
    STR_LAN_STATUS,
    STR_LAN_FAILED,
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
    STR_KEY_CTRL,
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
    STR_ACT_SNEAK,
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
    STR_DEATH_FALL,
    STR_DEATH_DROWN,
    STR_DEATH_STARVE,
    STR_DEATH_MOB_ZOMBIE,
    STR_DEATH_MOB_SKELETON,
    STR_DEATH_MOB_CREEPER,
    STR_DEATH_MOB_SPIDER,
    STR_DEATH_MOB_SLIME,
    STR_DEATH_MOB_ENDERMAN,
    STR_DEATH_MOB_COW,
    STR_DEATH_MOB_SHEEP,
    STR_DEATH_MOB_CHICKEN,
    STR_DEATH_MOB_WOLF,
    STR_DEATH_MOB_WITCH,
    STR_DEATH_MOB_BAT,

    // Mob names
    STR_MOB_PIG,
    STR_MOB_COW,
    STR_MOB_SHEEP,
    STR_MOB_CHICKEN,
    STR_MOB_VILLAGER,
    STR_MOB_HORSE,
    STR_MOB_WOLF,
    STR_MOB_WITCH,
    STR_MOB_BAT,
    STR_DEATH_VOID,
    STR_DEATH_SCORE,

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
    STR_RESOLUTION,
    STR_RES_960,
    STR_RES_1280,
    STR_RES_1600,
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

    // Difficulty
    STR_DIFFICULTY,
    STR_DIFFICULTY_PEACEFUL,
    STR_DIFFICULTY_EASY,
    STR_DIFFICULTY_NORMAL,
    STR_DIFFICULTY_HARD,

    // Achievements
    STR_ACH_FIRST_STEPS,
    STR_ACH_DEEP_DIG,
    STR_ACH_MONSTER_HUNTER,
    STR_ACH_ARCHITECT,
    STR_ACH_REDSTONE_ENGINEER,
    STR_ACH_COLLECTOR,
    STR_ACH_ANGLER,
    STR_ACH_BREEDER,
    STR_ACH_ENCHANTER,
    STR_ACH_DEMOLITION,
    STR_COLLECTION_TITLE,
    STR_COLLECTION_CHALLENGES,
    STR_COLLECTION_UNLOCKED,
    STR_COLLECTION_LOCKED,
    STR_COLLECTION_CLOSE,

    // Status Messages
    STR_MSG_GAME_SAVED,
    STR_MSG_HEALED_XP,
    STR_MSG_NOT_ENOUGH_XP,
    STR_MSG_INVENTORY_FULL,
    STR_MSG_TOOL_BROKE,
    STR_MSG_TOOL_WEARING,
    STR_MSG_FALL_DAMAGE,
    STR_MSG_SPAWN_SET,
    STR_MSG_SLEEP,
    STR_MSG_SLEEP_ONLY_NIGHT,
    STR_MSG_HUNGRY,
    STR_MSG_STARVING,
    STR_MSG_ATE,
    STR_MSG_TRADE,
    STR_MSG_NOT_ENOUGH_ITEMS,
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
    STR_DBG_WEATHER,
    STR_DBG_MODE,
    STR_WEATHER_CLEAR,
    STR_WEATHER_RAIN,
    STR_WEATHER_THUNDER,
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
    STR_TOOL_WOOD_HOE,
    STR_TOOL_STONE_PICKAXE,
    STR_TOOL_STONE_AXE,
    STR_TOOL_STONE_SWORD,
    STR_TOOL_STONE_SHOVEL,
    STR_TOOL_STONE_HOE,
    STR_TOOL_IRON_PICKAXE,
    STR_TOOL_IRON_AXE,
    STR_TOOL_IRON_SWORD,
    STR_TOOL_IRON_SHOVEL,
    STR_TOOL_IRON_HOE,
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
    // New blocks/items
    STR_BLOCK_GOLD_ORE,
    STR_BLOCK_DIAMOND_ORE,
    STR_BLOCK_REDSTONE_ORE,
    STR_BLOCK_LAPIS_ORE,
    STR_ITEM_GOLD_INGOT,
    STR_ITEM_DIAMOND,
    STR_ITEM_REDSTONE,
    STR_ITEM_LAPIS,
    STR_BLOCK_CHEST,
    // Gold tools
    STR_TOOL_GOLD_PICKAXE,
    STR_TOOL_GOLD_AXE,
    STR_TOOL_GOLD_SWORD,
    STR_TOOL_GOLD_SHOVEL,
    STR_TOOL_GOLD_HOE,
    // Diamond tools
    STR_TOOL_DIAMOND_PICKAXE,
    STR_TOOL_DIAMOND_AXE,
    STR_TOOL_DIAMOND_SWORD,
    STR_TOOL_DIAMOND_SHOVEL,
    STR_TOOL_DIAMOND_HOE,
    // Gold armor
    STR_ARMOR_GOLD_HELMET,
    STR_ARMOR_GOLD_CHESTPLATE,
    STR_ARMOR_GOLD_LEGGINGS,
    STR_ARMOR_GOLD_BOOTS,
    // Diamond armor
    STR_ARMOR_DIAMOND_HELMET,
    STR_ARMOR_DIAMOND_CHESTPLATE,
    STR_ARMOR_DIAMOND_LEGGINGS,
    STR_ARMOR_DIAMOND_BOOTS,
    // Mob drops
    STR_ITEM_GUNPOWDER,
    STR_ITEM_STRING,
    // New items/blocks
    STR_ITEM_BONE,
    STR_ITEM_ARROW,
    STR_ITEM_BOW,
    STR_BLOCK_MOSSY_COBBLESTONE,
    STR_BLOCK_BOOKSHELF,
    STR_BLOCK_LANTERN,
    STR_BLOCK_BONE_BLOCK,
    // Phase 1: Biome blocks
    STR_BLOCK_SNOW,
    STR_BLOCK_ICE,
    STR_BLOCK_PACKED_ICE,
    STR_BLOCK_MUD,
    STR_BLOCK_MOSS_BLOCK,
    STR_BLOCK_JUNGLE_WOOD,
    STR_BLOCK_JUNGLE_LEAVES,
    STR_BLOCK_VINE,
    STR_BLOCK_PUMPKIN,
    STR_BLOCK_MELON,
    STR_BLOCK_SNOWY_GRASS,
    STR_BLOCK_COARSE_DIRT,
    STR_BLOCK_PODZOL,
    // Phase 1: New items
    STR_ITEM_SLIMEBALL,
    STR_ITEM_ENDER_PEARL,
    // Farming
    STR_ITEM_WHEAT_SEEDS,
    STR_ITEM_WHEAT,
    STR_BLOCK_FARMLAND,
    STR_BLOCK_CROPS,
    STR_BLOCK_HAY_BALE,
    // Animal drops
    STR_ITEM_RAW_BEEF,
    STR_ITEM_LEATHER,
    STR_ITEM_RAW_MUTTON,
    STR_ITEM_WOOL,
    STR_ITEM_RAW_CHICKEN,
    STR_ITEM_FEATHER,
    STR_ITEM_EGG,
    STR_ITEM_COOKED_BEEF,
    STR_ITEM_COOKED_MUTTON,
    STR_ITEM_COOKED_CHICKEN,
    // Utility items
    STR_ITEM_BUCKET,
    STR_ITEM_WATER_BUCKET,
    STR_ITEM_LAVA_BUCKET,
    // Lava system
    STR_BLOCK_LAVA,
    STR_BLOCK_OBSIDIAN,
    // Enchanting
    STR_BLOCK_ENCHANTING_TABLE,
    // Redstone blocks
    STR_BLOCK_LEVER,
    STR_BLOCK_REDSTONE_WIRE,
    STR_BLOCK_REDSTONE_LAMP,
    STR_BLOCK_STONE_PRESSURE_PLATE,
    STR_BLOCK_STONE_BUTTON,
    STR_BLOCK_REDSTONE_REPEATER,
    STR_BLOCK_PISTON,
    STR_BLOCK_IRON_DOOR,
    STR_ITEM_GLASS_BOTTLE,
    STR_ITEM_POTION_WATER,
    STR_ITEM_POTION_SPEED,
    STR_ITEM_POTION_STRENGTH,
    STR_ITEM_POTION_REGEN,
    STR_ITEM_POTION_FIRE_RESISTANCE,
    STR_ITEM_POTION_WATER_BREATHING,
    STR_ITEM_POTION_POISON,
    STR_MSG_DRANK_POTION,
    STR_MSG_BOTTLE_FILLED,
    STR_BLOCK_BREWING_STAND,
    STR_BREWING,
    STR_BOTTLE,
    STR_MSG_BREWING,
    STR_BLOCK_ABYSS_STONE,
    STR_BLOCK_ABYSS_CRYSTAL_ORE,
    STR_BLOCK_GLOWSHROOM,
    STR_ITEM_ABYSS_CRYSTAL,
    STR_ACH_ABYSS,
    STR_MOB_ABYSS_WARDEN,
    STR_DEATH_MOB_ABYSS_WARDEN,
    STR_ACH_WARDEN,
    STR_MSG_WARDEN_SUMMONED,
    STR_MSG_ALTAR_DEEP_ONLY,
    STR_MSG_WARDEN_ALREADY,
    STR_BLOCK_ABYSS_ALTAR,
    STR_RECIPE_ABYSS_ALTAR,    STR_RECIPE_BREWING_STAND,
    STR_MSG_IRON_DOOR_HINT,

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
    // Gold recipes
    STR_RECIPE_GOLD_PICK,
    STR_RECIPE_GOLD_AXE,
    STR_RECIPE_GOLD_SWORD,
    STR_RECIPE_GOLD_SHOVEL,
    STR_RECIPE_GOLD_HELMET,
    STR_RECIPE_GOLD_CHEST,
    STR_RECIPE_GOLD_LEGS,
    STR_RECIPE_GOLD_BOOTS,
    // Diamond recipes
    STR_RECIPE_DIAMOND_PICK,
    STR_RECIPE_DIAMOND_AXE,
    STR_RECIPE_DIAMOND_SWORD,
    STR_RECIPE_DIAMOND_SHOVEL,
    STR_RECIPE_DIAMOND_HELMET,
    STR_RECIPE_DIAMOND_CHEST,
    STR_RECIPE_DIAMOND_LEGS,
    STR_RECIPE_DIAMOND_BOOTS,
    // Chest recipe
    STR_RECIPE_CHEST,
    // New recipes
    STR_RECIPE_ARROW,
    STR_RECIPE_BOW,
    STR_RECIPE_BONE_BLOCK,
    STR_RECIPE_BONE_BLOCK_DECOMP,
    STR_RECIPE_BOOKSHELF,
    STR_RECIPE_LANTERN,
    // Utility recipes
    STR_RECIPE_BUCKET,
    STR_RECIPE_ENCHANTING_TABLE,
    // Redstone recipes
    STR_RECIPE_REDSTONE_WIRE,
    STR_RECIPE_LEVER,
    STR_RECIPE_REDSTONE_LAMP,
    STR_RECIPE_STONE_BUTTON,
    STR_RECIPE_REDSTONE_REPEATER,
    STR_RECIPE_PISTON,
    STR_RECIPE_IRON_DOOR,
    STR_RECIPE_GLASS_BOTTLE,
    STR_RECIPE_POTION_SPEED,
    STR_RECIPE_POTION_STRENGTH,
    STR_RECIPE_POTION_REGEN,
    STR_RECIPE_POTION_FIRE_RESISTANCE,
    STR_RECIPE_POTION_WATER_BREATHING,
    STR_RECIPE_POTION_POISON,
    STR_RECIPE_PRESSURE_PLATE,
    // Combat message
    STR_MSG_CRIT_HIT,
    // Smelt
    STR_SMELT_IRON,
    STR_SMELT_PORK,
    STR_SMELT_COBBLE,
    STR_SMELT_SAND,
    STR_SMELT_GOLD,
    STR_SMELT_REDSTONE,
    STR_SMELT_LAPIS,
    STR_SMELT_CLAY,
    STR_SMELT_ICE,
    STR_SMELT_BEEF,
    STR_SMELT_MUTTON,
    STR_SMELT_CHICKEN,
    STR_SMELT_FISH,

    // Furnace messages
    STR_MSG_NO_FUEL,
    STR_MSG_SMELTING,
    STR_MSG_CANNOT_SMELT,

    // Additional controls
    STR_KEY_F11,
    STR_ACT_FULLSCREEN,

    // Enchantment names
    STR_ENCH_SHARPNESS,
    STR_ENCH_EFFICIENCY,
    STR_ENCH_PROTECTION,
    STR_ENCH_FORTUNE,
    STR_ENCH_UNBREAKING,
    STR_ENCH_SILK_TOUCH,
    STR_ENCH_POWER,
    STR_ENCH_KNOCKBACK,
    STR_ENCH_FIRE_ASPECT,
    STR_ENCHANTED,

    // Enchanting messages
    STR_MSG_ENCHANTED,
    STR_MSG_ALREADY_ENCHANTED,

    // Tutorial and multiplayer
    STR_TUTORIAL_CONTROLS,
    STR_NET_PLAYER_JOINED,
    STR_NET_PLAYER_LEFT,
    STR_NET_HOST_DISCONNECTED,

    // Missing messages
    STR_MSG_NO_ARROWS,
    STR_DEATH_LAVA,
    STR_DEATH_CACTUS,

    // Ender pearl
    STR_MSG_ENDER_PEARL,

    // Item type labels
    STR_TYPE_TOOL,
    STR_TYPE_ARMOR,
    STR_TYPE_FOOD,
    STR_TYPE_BLOCK,
    // New recipes
    STR_RECIPE_SLIMEBALL_STRING,
    STR_RECIPE_WOOD_HOE,
    STR_RECIPE_STONE_HOE,
    STR_RECIPE_IRON_HOE,
    STR_RECIPE_GOLD_HOE,
    STR_RECIPE_DIAMOND_HOE,
    STR_RECIPE_HAY_BALE,
    STR_RECIPE_MOSSY_COBBLESTONE,
    STR_RECIPE_COARSE_DIRT,
    // Fishing
    STR_ITEM_FISHING_ROD,
    STR_ITEM_RAW_FISH,
    STR_ITEM_COOKED_FISH,
    STR_RECIPE_FISHING_ROD,
    STR_FISH_CAST,
    STR_FISH_BITE,
    STR_FISH_CATCH,
    STR_FISH_RETRACT,
    // Cauldron
    STR_BLOCK_CAULDRON,
    STR_MSG_CAULDRON_FILLED,
    STR_MSG_CAULDRON_EMPTY,
    STR_MSG_CAULDRON_DRINK,
    // Decorative blocks
    STR_BLOCK_OAK_STAIRS,
    STR_BLOCK_COBBLESTONE_STAIRS,
    STR_BLOCK_STONE_BRICKS,
    STR_BLOCK_CHISELED_STONE_BRICKS,
    STR_BLOCK_OAK_SLAB,
    STR_BLOCK_COBBLESTONE_SLAB,
    STR_RECIPE_OAK_STAIRS,
    STR_RECIPE_COBBLESTONE_STAIRS,
    STR_RECIPE_STONE_BRICKS,
    STR_RECIPE_CHISELED_STONE_BRICKS,
    STR_RECIPE_OAK_SLAB,
    STR_RECIPE_COBBLESTONE_SLAB,
    // New plants & items
    STR_BLOCK_CACTUS,
    STR_BLOCK_SUGAR_CANE,
    STR_ITEM_PAPER,
    STR_ITEM_BOOK,
    STR_ITEM_SUGAR,
    STR_BLOCK_TNT,
    // Phase 2: new biome blocks
    STR_BLOCK_RED_SAND,
    STR_BLOCK_MYCELIUM,
    STR_BLOCK_MUSHROOM_BLOCK,
    STR_BLOCK_MUSHROOM_STEM,
    STR_RECIPE_TNT,
    STR_RECIPE_PAPER,
    STR_RECIPE_BOOK,
    STR_RECIPE_SUGAR,
    STR_MSG_CACTUS_DAMAGE,
    STR_MSG_CANT_SLEEP_MOBS,

    // Creative mode
    STR_GAMEMODE,
    STR_MODE_SURVIVAL,
    STR_MODE_CREATIVE,
    STR_MSG_FLIGHT_ON,
    STR_MSG_FLIGHT_OFF,
    STR_CREATIVE_TITLE,
    STR_CREATIVE_SELECTED,
    STR_CREATIVE_SELECT_HINT,
    STR_CREATIVE_SEARCH,
    STR_CREATIVE_NO_MATCH,
    STR_MODE_SET,
    STR_CREATIVE_BACKPACK,
    STR_HINT_SCROLL,
    STR_MSG_HOST_ONLY,
    // Easter eggs
    STR_EGG_KONAMI_ON,
    STR_EGG_KONAMI_OFF,
    STR_EGG_TITLE_CLICK,
    STR_EGG_FACT1,
    STR_EGG_FACT2,
    STR_EGG_FACT3,

    STR_COUNT
} StringId;

//----------------------------------------------------------------------------------
// Crafting
//----------------------------------------------------------------------------------
typedef struct {
    BlockType input;
    int inputCount;
    BlockType input2;       // optional second ingredient (BLOCK_AIR = none)
    int input2Count;
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

// Trading system
#define MAX_TRADES 16
typedef struct {
    uint8_t giveItem;       // Item the player gives
    int giveCount;
    uint8_t receiveItem;    // Item the player receives
    int receiveCount;
    StringId nameId;
} Trade;

//----------------------------------------------------------------------------------
// Mob System
//----------------------------------------------------------------------------------
typedef enum {
    MOB_NONE = 0,
    MOB_PIG,
    MOB_ZOMBIE,
    MOB_SKELETON,
    MOB_CREEPER,
    MOB_SPIDER,
    MOB_SLIME,
    MOB_ENDERMAN,
    MOB_COW,
    MOB_SHEEP,
    MOB_CHICKEN,
    MOB_VILLAGER,
    // Phase 3: new mobs
    MOB_HORSE,      // passive, rideable-looking grazer
    MOB_WOLF,       // neutral: wanders, retaliates when hit
    MOB_WITCH,      // hostile ranged caster
    MOB_BAT,        // ambient flyer
    MOB_ABYSS_WARDEN,   // endgame boss: summoned at abyss altars
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
    float fireTimer;        // fire aspect damage accumulator (counts down)
    float fireDmgAccum;     // per-mob fire damage tick accumulator
    float attackTimer;  // cooldown for ranged attacks / creeper fuse
    float fuseTimer;    // creeper explosion fuse countdown
    float despawnTimer; // time-based despawn to prevent mob cap saturation
    int slimeType;      // 0=large, 1=small (for slime splitting)
    float loveTimer;    // >0 = in love mode, counts down
    bool isBaby;        // true = baby mob (smaller, grows over time)
    float growTimer;    // time until baby becomes adult
    bool active;
} Mob;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
    bool fromPlayer;
    bool isFishing;
    float fishTimer;
    bool hasBite;
    int catchValue;
    int damage;         // damage dealt to a mob on hit (base = PROJECTILE_DAMAGE, +ENCH_POWER)
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
    uint16_t itemEnchantments[INVENTORY_SLOTS];
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
    float lavaDamageAccum;    // Fractional lava damage accumulator
    float knockbackTimer;    // Preserves horizontal velocity during knockback
    bool sprinting;
    bool playerDead;
    bool facingRight;
    bool wasInWater;         // for water splash detection
    float footstepTimer;     // for footstep sound intervals
    float fallPeakVel;       // peak downward velocity during current fall
    float fallDistance;      // accumulated fall distance (pixels) for fall damage
    float coyoteTimer;
    float jumpBufferTimer;
    float cameraShakeIntensity;
    float cameraShakeTimer;
    float attackCooldown;
    float attackAnim;         // total duration of the current attack swing (for animation)
    float landSquashTimer;    // >0 = recently landed, drives squash animation
    float bowChargeTimer;     // 0.0 to 1.0 (full charge)
    bool bowCharging;         // true while holding right-click with bow
    bool sneaking;            // shift key held — crouch
    float walkTimer;          // accumulated walk cycle timer (for animation)
    int spawnX, spawnY;      // bed spawn point (-1 = use default)
    // Armor slots: 0=helmet, 1=chestplate, 2=leggings, 3=boots
    uint8_t armor[4];
    int armorDurability[4];
    uint16_t armorEnchantments[4];
    // Network control
    bool netControlled;      // true = driven by network input, not keyboard
    float moveInput;         // -1.0 to 1.0, used when netControlled
    bool jumpHeld;           // jump key state from network
    char playerName[32];     // display name in multiplayer
    // Creative flight
    bool flying;             // true = creative flight active
    float lastJumpTapTimer;  // time since last jump tap (for double-tap toggle)
    StatusEffect effects[MAX_PLAYER_EFFECTS];   // active status effects (potions etc.)
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
// XP Orb System
//----------------------------------------------------------------------------------
#define MAX_XP_ORBS       64
#define XP_ORB_ATTRACT_DIST 48.0f
typedef struct {
    Vector2 position;
    Vector2 velocity;
    float lifetime;
    bool active;
    int xpValue;        // 1, 3, or 7
    float bobPhase;
    float attractTimer;
} XpOrb;
extern XpOrb xpOrbs[MAX_XP_ORBS];
void InitXpOrbs(void);
void SpawnXpOrb(float x, float y, int value);
void UpdateXpOrbs(float dt);
void DrawXpOrbs(void);

//----------------------------------------------------------------------------------
// Game State
//----------------------------------------------------------------------------------
typedef enum {
    STATE_MENU = 0,
    STATE_PLAYING,
    STATE_SETTINGS,
    STATE_SLOT_SELECT,
    STATE_HOST_WAITING,
    STATE_LOADING,   // world generation / load in progress (draws a progress screen)
    STATE_JOIN_GAME
} GameState;

typedef enum {
    DIFFICULTY_PEACEFUL = 0,
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD,
    DIFFICULTY_COUNT
} Difficulty;

typedef enum {
    GAME_SURVIVAL = 0,
    GAME_CREATIVE,
    GAME_MODE_COUNT
} GameMode;

typedef enum {
    ACH_FIRST_STEPS = 0,    // Craft a wooden pickaxe
    ACH_DEEP_DIG,           // Reach bedrock layer (y >= 240)
    ACH_ABYSS,              // Descend into the abyss (y >= 248)
    ACH_MONSTER_HUNTER,     // Kill 100 mobs
    ACH_ARCHITECT,          // Place 1000 blocks
    ACH_REDSTONE_ENGINEER,  // Build a working redstone circuit
    ACH_COLLECTOR,          // Have 20 unique item types in inventory
    ACH_ANGLER,             // Catch a fish
    ACH_BREEDER,            // Breed two animals into a baby
    ACH_ENCHANTER,          // Enchant an item
    ACH_DEMOLITION,         // Detonate TNT
    ACH_WARDEN,             // Defeat the Abyss Warden (appended: save-file index)
    ACH_COUNT
} Achievement;

// Enchantment types
typedef enum {
    ENCH_NONE = 0,
    ENCH_SHARPNESS,         // +1.5 damage per level (swords)
    ENCH_EFFICIENCY,        // +50% mining speed per level (tools)
    ENCH_PROTECTION,        // +2% damage reduction per level (armor)
    ENCH_FORTUNE,           // +15% chance per level for extra drop (pickaxes)
    ENCH_UNBREAKING,        // 1/(level+1) chance to skip durability loss (all)
    ENCH_SILK_TOUCH,        // Mine blocks in their original form
    ENCH_POWER,             // +2 arrow damage per level (bows)
    ENCH_KNOCKBACK,         // +50% knockback per level (swords)
    ENCH_FIRE_ASPECT,       // sets mobs on fire for 1.5s per level (swords)
    ENCH_COUNT
} EnchantmentType;

// Encode enchantment: type in low 8 bits, level (1-5) in high 4 bits
#define ENCH_TYPE(e)        ((e) & 0xFF)
#define ENCH_LEVEL(e)       (((e) >> 8) & 0xF)
#define ENCH_PACK(t, l)     ((uint16_t)(((t) & 0xFF) | (((l) & 0xF) << 8)))

typedef struct {
    EnchantmentType type;
    int level;           // 1-3
    int xpCost;
} EnchantOption;

#define MAX_ENCHANT_OPTIONS 3

typedef struct {
    bool open;
    int blockX, blockY;
    uint8_t heldItem;
    int heldItemSlot;
    int optionCount;
    EnchantOption options[MAX_ENCHANT_OPTIONS];
} EnchantSession;

extern EnchantSession localEnchantSession;

// Obsolete global aliases removed; use localEnchantSession.*

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
// Multiplayer
//----------------------------------------------------------------------------------
typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool facingRight;
    bool sprinting;
    bool onGround;
    int selectedSlot;
    uint8_t armor[4];
    int health;
    bool active;
    float interpX, interpY; // Interpolation targets
    char playerName[32];    // display name (received from peer)
} RemotePlayer;

//----------------------------------------------------------------------------------
// Weather System
//----------------------------------------------------------------------------------
typedef enum {
    WEATHER_CLEAR = 0,
    WEATHER_RAIN,
    WEATHER_THUNDER
} WeatherType;

typedef struct {
    WeatherType type;
    float duration;       // seconds remaining in current weather
    float transitionTimer; // smooth transition between weather states
    float rainAlpha;      // current rain opacity (0-1)
    float lightningFlash; // >0 = screen flash intensity
    float thunderTimer;   // delay for thunder sound after lightning
} WeatherState;

typedef struct {
    float x, y;
    float speed;
    float length;
} RainDrop;

//----------------------------------------------------------------------------------
// Extern Globals
//----------------------------------------------------------------------------------
extern uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
extern uint8_t lightMap[WORLD_WIDTH][WORLD_HEIGHT];
extern Chunk loadedChunks[MAX_CHUNKS];

extern Player players[MAX_NET_PLAYERS];
extern int localPlayerId;
#define player (players[localPlayerId])
extern RemotePlayer remotePlayers[MAX_NET_PLAYERS];
extern Camera2D camera;
extern DayNightCycle dayNight;
extern bool isSleeping;
extern float sleepFade;

extern Texture2D blockAtlas;
#define CRACK_STAGES 10
extern Texture2D crackTextures[CRACK_STAGES];
extern bool showDebug;
extern bool showLargeMap;
extern bool inventoryOpen;
extern bool gamePaused;
extern bool audioReady;
extern unsigned int worldSeed;
extern uint64_t simulationTick;
extern float simulationAccumulator;

extern char messageText[128];
extern float messageTimer;
extern float messageSlide;
extern Color messageColor;

extern Mob mobs[MAX_MOBS];
extern int bossMobIndex;            // index of the active Abyss Warden, -1 = none
extern Projectile projectiles[MAX_PROJECTILES];
extern int pendingProjectileHitCount;
extern int pendingProjectileHitIndex[MAX_PROJECTILES];
extern int pendingProjectileHitDamage[MAX_PROJECTILES];
extern float mobSpawnTimer;

extern Particle particles[MAX_PARTICLES];
extern ItemEntity entities[MAX_ENTITIES];
extern GameState gameState;
extern Difficulty gameDifficulty;
extern GameMode gameMode;
extern int pendingGameMode; // survival/creative selected on the new-game screen
extern bool creativeOpen;   // creative inventory palette overlay
extern bool achievementsOpen; // collection/achievement overlay
extern bool menuPartyMode;  // Konami-code easter egg: confetti rain on the main menu
extern int menuTitleClicks; // consecutive clicks on the main-menu title

// Achievement tracking
extern bool achievements[ACH_COUNT];
void UnlockAchievement(Achievement ach);
extern int totalMobsKilled;
extern int totalBlocksPlaced;

// Modified block tracking for multiplayer world sync
#define MAX_MODIFIED_BLOCKS 16384
typedef struct { uint16_t x, y; uint8_t blockType; } ModifiedBlock;
typedef struct { uint16_t x, y; uint8_t on; } ModifiedLever;
extern ModifiedBlock modifiedBlocks[];
extern int modifiedBlockCount;
#define MAX_MODIFIED_LEVERS 512
extern ModifiedLever modifiedLevers[];
extern int modifiedLeverCount;

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
extern int resolutionPreset; // 0=960x540, 1=1280x720, 2=1600x900
extern RenderTexture2D logicalCanvas;
extern bool logicalCanvasReady;
extern Rectangle logicalViewport;
extern float logicalScale;
void ApplyResolution(int preset);
void UpdateLogicalViewport(void);
Vector2 Win32GetLogicalMousePosition(void);
extern char seedInputBuf[32];
extern int seedInputLen;
extern bool seedFieldFocused;

// Confirmation dialog
extern bool confirmDialogActive;
extern int confirmDialogSlot;
extern int confirmDialogMode; // 0=overwrite, 1=delete

extern Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
extern Sound sndHurt, sndDeath, sndEat, sndDrink, sndClick, sndCraft, sndXP, sndDrop;
extern Sound sndFootstep, sndZombie, sndPig, sndSplash, sndVillager;
extern Sound sndSkeleton, sndCreeperHiss, sndSpider, sndSlime, sndEnderman;
extern Sound sndHorse, sndWolf, sndWitch, sndBat;
extern Sound sndRain, sndCreeperFuse, sndThunder;
extern Sound sndCaveDrip, sndCaveAmbient, sndWind;
extern Sound sndPickup, sndBowFire;
extern Music bgm;

extern const BlockInfo blockInfo[BLOCK_COUNT];
extern CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
extern int craftRecipeCount;

// Furnace UI state
extern bool furnaceOpen;
extern int furnaceBlockX, furnaceBlockY;
extern int brewingBlockX, brewingBlockY;
extern uint8_t furnaceFuel;
extern int furnaceFuelCount;
extern uint8_t furnaceInput;
extern int furnaceInputCount;
extern uint8_t furnaceOutput;
extern int furnaceOutputCount;
extern float furnaceProgress;
extern float furnaceFuelBurn;
extern float furnaceFuelBurnMax;

// Multi-furnace support
extern FurnaceData furnaces[MAX_FURNACES];
#define MAX_BREWING_STANDS 64
extern BrewingData brewingStands[MAX_BREWING_STANDS];
extern int brewingCount;
extern bool brewingOpen;
extern int activeBrewing;
extern uint8_t brewBottle; extern int brewBottleCount;
extern uint8_t brewIngredient; extern int brewIngredientCount;
extern uint8_t brewOutput; extern int brewOutputCount;
extern float brewProgress;
extern int furnaceCount;
extern int activeFurnace;
int FindFurnace(int x, int y);
int GetOrCreateFurnace(int x, int y);
void SyncFurnaceToActive(int idx);
void SyncActiveToFurnace(int idx);
void RequestOpenFurnace(int bx, int by);
void RequestOpenBrewing(int bx, int by);
void SyncBrewingToActive(int idx);
void SyncActiveToBrewing(int idx);
void UpdateBrewingTick(float dt);
void RemoveBrewing(int x, int y);
void ReturnBrewingItems(void);
void CloseBrewingUI(void);
void DestroyBrewing(int x, int y);
int FindBrewing(int x, int y);
int GetOrCreateBrewing(int x, int y);
void SyncFurnaceToHost(void);
void SyncFurnaceToAll(void);
void CloseFurnaceNetwork(void);

// Inventory multiplayer sync helpers
void SyncInventoryToHost(void);
void SyncInventoryToAll(void);

// Drag-and-drop held item (shared between rendering.c and crafting.c)
extern uint8_t heldItem;
extern int heldCount;
extern int heldDurability;
extern uint16_t heldItemEnchant;

// Crafting table state
extern bool craftingTableOpen;

// Chest state
extern bool chestOpen;
extern int chestBlockX, chestBlockY;
extern ChestData chestData[MAX_CHESTS];
extern int chestCount;

// Cauldron data
typedef struct {
    int x, y;
    uint8_t fillLevel; // 0=empty, 1, 2, 3=full
} CauldronData;
#define MAX_CAULDRONS 64
extern CauldronData cauldrons[MAX_CAULDRONS];
extern int cauldronCount;

// Smelting recipes
extern SmeltRecipe smeltRecipes[MAX_SMELT_RECIPES];
extern int smeltRecipeCount;

// Trading
extern Trade trades[MAX_TRADES];
extern int tradeCount;
extern bool tradeOpen;

// Enchanting UI
extern EnchantSession localEnchantSession;
extern int villagerTradeIndex;

// Chat
#define MAX_CHAT_MESSAGES 32
#define MAX_CHAT_INPUT 128
typedef struct {
    uint8_t playerId;      // 255 = system
    char message[MAX_CHAT_INPUT];
    float timer;           // seconds remaining visible
} ChatMessage;
extern bool chatOpen;
extern char chatInput[MAX_CHAT_INPUT];
extern int chatInputLen;
extern ChatMessage chatHistory[MAX_CHAT_MESSAGES];
extern int chatHistoryCount;

// Weather
extern WeatherState weather;
extern RainDrop rainDrops[MAX_RAIN_DROPS];

// i18n / Font
extern Language language;
extern Font gameFont;
extern Font gameFontLarge;
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
int GetBiomeAtX(int worldX, unsigned int seed);
Chunk* GetChunk(int chunkX);
void UnloadChunk(int chunkX);
void GenerateChunkTexture(Chunk *chunk);
void InvalidateChunkAt(int worldBlockX, int worldBlockY);
void InitChunkTable(void);
void UpdateChunks(void);
bool IsBlockSolid(int bx, int by);

// Block access layer (world.c). All block reads/writes should go through these;
// they are the single place a dimension index would be threaded through.
uint8_t GetBlock(int x, int y);
void SetBlock(int x, int y, uint8_t type);
bool IsGravityBlock(uint8_t block);
void ApplyGravityAt(int bx, int by);
bool ValidateBlockInfo(void);

// Water flow system (world.c)
void InitWater(void);
int GetWaterLevel(int bx, int by);
void SetWaterSource(int bx, int by);
void UpdateFluidTick(float dt);
void GetFluidState(int bx, int by, uint8_t *blockType, uint8_t *kind, uint8_t *level, bool *source);
void RestoreFluidState(int bx, int by, uint8_t blockType, uint8_t kind, uint8_t level, bool source);
void ClearFluidStateAt(int bx, int by);
typedef struct {
    uint16_t x, y;
    uint8_t blockType, kind, level;
    bool source;
} FluidChange;
void QueueFluidSources(void);
int GetFluidChangeCount(void);
bool GetFluidChange(int index, FluidChange *change);
void ClearFluidChanges(void);
void RemoveWaterAt(int bx, int by);

//----------------------------------------------------------------------------------
// Block metadata layer (world.c)
//
// Per-cell state for blocks that need more than their type id: piston facing and
// extension, repeater delay, door open/closed, button countdown. Stored sparsely
// because only a small fraction of cells ever carry metadata, and because the
// alternative - a 512KB array per new state - does not scale.
//
// Blocks without metadata simply have no entry; GetBlockMeta returns false.
//----------------------------------------------------------------------------------
typedef struct {
    uint16_t x, y;
    uint8_t kind;   // BlockMetaKind - which block owns this entry
    uint8_t a, b;   // kind-specific payload
} BlockMeta;

#define MAX_BLOCK_METAS 4096
#define BLOCK_META_NONE 0

void InitBlockMeta(void);
bool SetBlockMeta(int x, int y, uint8_t kind, uint8_t a, uint8_t b);
bool GetBlockMeta(int x, int y, uint8_t *kind, uint8_t *a, uint8_t *b);
void ClearBlockMeta(int x, int y);
int  GetBlockMetaCount(void);
bool GetBlockMetaAt(int index, BlockMeta *out);
void ClearAllBlockMeta(void);

// Lava flow system (world.c)
void InitLava(void);
int GetLavaLevel(int bx, int by);
void SetLavaSource(int bx, int by);
void RemoveLavaAt(int bx, int by);

// Redstone system (world.c)
void InitRedstone(void);
bool IsLeverOn(int bx, int by);
int CollectLeversOn(uint16_t *out, int maxPairs);
void ApplyLeverStates(const uint16_t *pairs, int count);
void SetLeverState(int bx, int by, bool on);
void ToggleLever(int bx, int by);
// Redstone device kinds (see world.c)
#define RSD_BUTTON    0
#define RSD_REPEATER  1
#define RSD_PISTON    2
#define RSD_DOOR      3
void RegisterRedstoneDevice(int bx, int by, uint8_t kind, uint8_t dir);
void UnregisterRedstoneDevice(int bx, int by);
void RebuildRedstoneDevices(void);
void PressStoneButton(int bx, int by);
bool IsButtonPressedAt(int bx, int by);
void SetRedstoneDeviceState(int bx, int by, bool on);
// Status effects (player.c)
void ApplyEffect(Player *p, EffectType type, int level, float duration);
bool HasEffect(const Player *p, EffectType type);
int GetEffectLevel(const Player *p, EffectType type);
void UpdatePlayerEffects(Player *p, float dt);
bool TrySummonWarden(int blockX, int blockY);
void ClearEffects(Player *p);
bool IsIronDoorOpen(int bx, int by);
void NotifyLeverToggled(int x, int y);   // game.c: record + broadcast after a local toggle
void UpdateRedstoneAt(int bx, int by);
void UpdateRedstoneTick(void);
void UpdateCrops(float dt);
int GetCropGrowth(int bx, int by);
void SetCropGrowth(int bx, int by, int stage);
int GetRedstonePowerAt(int bx, int by);
bool IsRedstoneLampPowered(int bx, int by);
void RegisterPressurePlate(int bx, int by);
void UnregisterPressurePlate(int bx, int by);
void RebuildPressurePlateList(void);
void RegisterCrop(int bx, int by);
void RebuildCropList(void);
// TNT explosives
void InitPrimedTnt(void);
void PrimeTnt(int bx, int by);
void UpdatePrimedTnt(float dt);
void ExplodeAt(float worldX, float worldY, int radius);
// Multiplayer container sync
void RequestOpenChest(int bx, int by);
void SyncChestToHost(int chestIdx);
void SyncChestToAll(int chestIdx);
void CloseChestNetwork(void);

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
int AddToInventoryCount(BlockType item, int count);
int GetMaxStack(BlockType item);
bool ConsumeItemFromSlot(Player *p, int slot, int count);
int CountItemInInventory(const Player *p, BlockType item);
int RemoveItemFromInventory(Player *p, BlockType item, int count);
float GetToolMiningSpeed(BlockType tool, BlockType block);
bool CanToolMineBlock(BlockType tool, BlockType block);
bool IsTool(BlockType item);
bool IsSword(BlockType tool);
bool IsPickaxe(BlockType tool);
bool IsHoe(BlockType tool);
float GetToolTier(BlockType tool);
int GetSwordDamage(BlockType tool);
int GetToolMaxDurability(BlockType tool);
bool IsFood(BlockType item);
int GetFoodValue(BlockType item);
bool IsArmor(BlockType item);
int GetArmorSlot(BlockType item);
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
void TriggerCameraShake(float intensity, float duration);
void UpdateHotbar(void);
float GetMiningProgress(void);
int GetMiningBlockX(void);
int GetMiningBlockY(void);
void DrawMiningCrack(void);
bool IsPlayerUnderwater(void);
float GetAttackSpeed(BlockType tool);

// daynight.c
void InitDayNight(void);
void UpdateDayNight(float dt);
bool TrySleep(void);
Color GetSkyColor(void);

// rendering.c
void DrawWorld(void);
void DrawFireEffects(float dt);
void DrawWater(void);
void DrawPlayerSprite(void);
void DrawHotbar(void);
void DrawActiveEffects(void);
void DrawPlayerStatus(void);
void DrawCrosshair(void);
void DrawDebugInfo(void);
void DrawInventoryScreen(void);
void SortInventory(void);
void DrawUiButton(float x, float y, float w, float h, const char *label,
                  int fontSize, bool hover, bool selected, bool enabled, float alpha);
void DrawUiSlot(int x, int y, int size, bool hover, bool selected, float alpha);
void GetUiTooltipPos(int mouseX, int mouseY, int w, int h, int *outX, int *outY);
void DrawMessage(void);
void DrawPauseMenu(void);
void DrawAchievementsUI(void);
void DrawDeathScreen(float dt);
float GetDeathFadeTimer(void);
void SetDeathCause(StringId cause);
void DrawMainMenu(void);
void DrawLoadingScreen(const char *message, float progress);
void DrawSlotSelectScreen(void);
void DrawConfirmDialog(void);
void DrawBackground(void);
void DrawMinimap(void);
void DrawLargeMap(void);
void DrawSettingsScreen(void);
void DrawCreativeScreen(void);
void ReturnHeldItem(void);
void ShowMessage(const char *msg, Color color);
void AddChatMessage(uint8_t playerId, const char *msg);
void SendChatMessage(const char *msg);
void BroadcastChatMessage(uint8_t playerId, const char *msg);
void DrawChatUI(void);
void NetSyncBlockChange(int x, int y, uint8_t blockType);

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
bool CanCraftForPlayer(Player *p, int recipeIndex);
void Craft(int recipeIndex);
void CraftForPlayer(Player *p, int recipeIndex);
void DrawCraftingPanel(int panelX, int panelY, int panelW, int visibleCount, int slotH, int pad, bool showAdvanced);
void InitSmeltingRecipes(void);
int FindSmeltRecipe(BlockType input);
BlockType FindBrewOutput(BlockType ingredient);
void InitTrades(void);
void DrawTradeUI(void);
void DrawEnchantingTableUI(void);
void DrawFurnaceUI(void);
void ReturnFurnaceItems(void);
float GetFuelBurnTime(uint8_t item);

// mob.c
void InitMobs(void);
void UpdateMobs(float dt);
void DrawMobs(void);
Mob* SpawnMob(MobType type, float x, float y);
void InitProjectiles(void);
int SpawnProjectile(float x, float y, float vx, float vy, bool fromPlayer);
void UpdateProjectiles(float dt);
void DrawProjectiles(void);
void DamageMob(Mob *mob, int damage);
int GetMobWidth(MobType type);
int GetMobHeight(MobType type);
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
void PlaySoundDrink(void);
void PlaySoundUIClick(void);
void PlaySoundCraft(void);
void PlaySoundXP(void);
void PlaySoundDrop(void);
void PlaySoundFootstep(void);
void PlaySoundMob(MobType type);
void PlaySoundSplash(void);
void PlaySoundCreeperFuse(void);
void PlaySoundThunder(void);
void UpdateRainAmbient(void);
void UpdateAmbientSounds(void);
void PlaySoundMobAt(MobType type, float mobX, float mobY);
void PlaySoundPickup(void);
void PlaySoundBowFire(void);
void SetSFXVolume(float volume);
void UpdateBGM(void);
void SetBGMVolume(float volume);

// particles.c
void InitParticles(void);
void SpawnBlockParticles(int blockX, int blockY, BlockType block);
void SpawnMiningParticles(int blockX, int blockY, BlockType block);
void SpawnDamageParticles(float x, float y, Color color);
void SpawnSprintDust(float x, float y);
void SpawnLandingDust(float x, float y, float intensity);
void SpawnBubble(float x, float y);
void SpawnFireParticle(float x, float y);
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

// weather.c
void InitWeather(void);
void UpdateWeather(float dt);
void DrawWeather(void);
float GetWeatherLightModifier(void);

// Smooth hover animation
float GetHoverAlpha(int slotId, bool hovered, float dt);

// Held item helpers (rendering.c)
void ClearHeldItem(void);

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

// network.c (multiplayer)
void InitNetwork(void);
void ShutdownNetwork(void);
void UpdateNetworkGame(float dt);
void NetCaptureAndSendInput(void);
void NetReceiveAndApplyState(void);
void NetBroadcastBlockChange(int x, int y, uint8_t block);
void NetApplyRemoteInput(int playerId, float moveX, bool jump, bool sprint,
                         float cursorX, float cursorY, int selectedSlot);
void DrawRemotePlayers(void);

#endif // TYPES_H
