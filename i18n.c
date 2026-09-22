/*******************************************************************************************
*
*   MyWorld - Internationalization (i18n) and Font System
*
********************************************************************************************/

#include "types.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//----------------------------------------------------------------------------------
// Global State
//----------------------------------------------------------------------------------
Language language = LANG_ZH_CN;

Font gameFont = { 0 };
Font gameFontLarge = { 0 };   // 42px atlas for titles; body text uses gameFont
bool useCustomFont = false;
char customFontPath[256] = { 0 };

// The glyph atlas is baked once at this size. UI text is mostly 12-16px, so
// baking near that range keeps small text sharp instead of downscaling a large
// atlas (which looked blurry). Large titles scale this up acceptably.
#define FONT_ATLAS_SIZE 14
#define FONT_ATLAS_SIZE_LARGE 42   // titles draw at ~42px, so bake a matching atlas

//----------------------------------------------------------------------------------
// String Table
//----------------------------------------------------------------------------------
// Format: strings[language][stringId]
// NULL entries fall back to English
static const char *strings[LANG_COUNT][STR_COUNT] = {

// ===== ENGLISH =====
[LANG_EN] = {
    [STR_NONE] = "",

    // Main Menu
    [STR_TITLE] = "MyWorld",
    [STR_SUBTITLE] = "A 2D Sandbox Adventure",
    [STR_BTN_NEW_GAME] = "New Game",
    [STR_BTN_LOAD_GAME] = "Load Game",
    [STR_BTN_SETTINGS] = "Settings",
    [STR_BTN_QUIT] = "Quit",
    [STR_HINT_NAVIGATE] = "Arrow keys / WASD: Navigate   |   Enter / Space: Select",
    [STR_HINT_CONTROLS] = "WASD: Move  |  Space: Jump  |  E: Inventory  |  ESC: Pause",

    // Slot Select
    [STR_NEW_GAME_TITLE] = "New Game",
    [STR_LOAD_GAME_TITLE] = "Load Game",
    [STR_NEW_GAME_SUB] = "Choose a slot to start a new world",
    [STR_LOAD_GAME_SUB] = "Choose a save to continue",
    [STR_SEED] = "Seed:",
    [STR_RANDOM] = "Random",
    [STR_GENERATING_WORLD] = "Generating world...",
    [STR_SLOT] = "Slot %d",
    [STR_SEED_DISPLAY] = "Seed: %u",
    [STR_OCCUPIED] = "Occupied",
    [STR_EMPTY_NEW] = "Empty - Click to create",
    [STR_EMPTY_LOAD] = "No save data",
    [STR_BACK] = "Back",
    [STR_HINT_SLOT] = "Arrows: Navigate  |  Enter: Select  |  DEL: Delete  |  ESC: Back",

    // Confirm Dialog
    [STR_DELETE_SAVE] = "Delete Save?",
    [STR_OVERWRITE_SAVE] = "Overwrite Save?",
    [STR_SLOT_HAS_DATA] = "Slot %d already has data.",
    [STR_CANNOT_UNDO] = "This cannot be undone!",
    [STR_YES_DELETE] = "Yes, Delete",
    [STR_YES_OVERWRITE] = "Yes, Overwrite",
    [STR_CANCEL] = "Cancel",
    [STR_CONFIRM_KEYS] = "[Y] Confirm    [N / ESC] Cancel",

    // Pause Menu
    [STR_PAUSED] = "PAUSED",
    [STR_MUSIC_VOLUME] = "Music Volume",
    [STR_SFX_VOLUME] = "SFX Volume",
    [STR_CONTROLS_TITLE] = "--- Controls ---",
    [STR_CONTINUE] = "Continue",
    [STR_MAIN_MENU] = "Main Menu",

    // Control Keys
    [STR_KEY_WASD] = "WASD",
    [STR_KEY_SPACE] = "Space",
    [STR_KEY_SHIFT] = "Shift",
    [STR_KEY_CTRL] = "Ctrl",
    [STR_KEY_LCLICK] = "LClick",
    [STR_KEY_RCLICK] = "RClick",
    [STR_KEY_E] = "E",
    [STR_KEY_H] = "H",
    [STR_KEY_F3] = "F3",
    [STR_KEY_ESC] = "ESC",
    [STR_KEY_19] = "1-9",

    // Control Actions
    [STR_ACT_MOVE] = "Move",
    [STR_ACT_JUMP] = "Jump/Swim",
    [STR_ACT_SPRINT] = "Sprint",
    [STR_ACT_SNEAK] = "Sneak",
    [STR_ACT_BREAK] = "Break/Attack",
    [STR_ACT_PLACE] = "Place/Eat",
    [STR_ACT_INVENTORY] = "Inventory",
    [STR_ACT_HEAL] = "Heal(XP)",
    [STR_ACT_DEBUG] = "Debug",
    [STR_ACT_PAUSE] = "Pause",
    [STR_ACT_HOTBAR] = "Hotbar",

    // Death Screen
    [STR_YOU_DIED] = "You Died!",
    [STR_PRESS_SPACE_RESPAWN] = "Press Space to respawn",
    [STR_PRESS_ESC_MENU] = "Press ESC to return to menu",
    [STR_DEATH_FALL] = "Fell from a high place",
    [STR_DEATH_DROWN] = "Drowned",
    [STR_DEATH_STARVE] = "Starved to death",
    [STR_DEATH_MOB_ZOMBIE] = "Slain by Zombie",
    [STR_DEATH_MOB_SKELETON] = "Shot by Skeleton",
    [STR_DEATH_MOB_CREEPER] = "Blown up by Creeper",
    [STR_DEATH_MOB_SPIDER] = "Slain by Spider",
    [STR_DEATH_MOB_SLIME] = "Slain by Slime",
    [STR_DEATH_MOB_ENDERMAN] = "Slain by Enderman",
    [STR_DEATH_VOID] = "Fell out of the world",
    [STR_DEATH_SCORE] = "Score: %d XP",

    // Inventory
    [STR_INVENTORY] = "Inventory",
    [STR_SORT] = "Sort",

    // Map
    [STR_WORLD_MAP] = "World Map",
    [STR_PRESS_M_CLOSE] = "Press M to close",
    [STR_PLAYER_COORD] = "Player: %d, %d",

    // Crafting
    [STR_CRAFTING] = "Crafting",
    [STR_CRAFTING_TABLE] = "Crafting Table",
    [STR_SEARCH] = "Search...",
    [STR_NO_MATCHES] = "No matches",
    [STR_NO_RECIPES] = "No recipes",

    // Furnace
    [STR_FURNACE] = "Furnace",
    [STR_FUEL] = "Fuel",
    [STR_INPUT] = "Input",
    [STR_OUTPUT] = "Output",
    [STR_PRESS_E_ESC_CLOSE] = "Press E or ESC to close",

    // Settings
    [STR_SETTINGS] = "Settings",
    [STR_SECTION_AUDIO] = "--- Audio ---",
    [STR_SOUND_EFFECTS] = "Sound Effects",
    [STR_SECTION_DISPLAY] = "--- Display ---",
    [STR_WINDOW_MODE] = "Window Mode",
    [STR_WINDOWED] = "Windowed",
    [STR_FULLSCREEN] = "Fullscreen",
    [STR_BORDERLESS] = "Borderless",
    [STR_RESOLUTION] = "Resolution",
    [STR_RES_960] = "960 x 540",
    [STR_RES_1280] = "1280 x 720",
    [STR_RES_1600] = "1600 x 900",
    [STR_SECTION_LANGUAGE] = "--- Language ---",
    [STR_LANGUAGE] = "Language",
    [STR_SECTION_FONT] = "--- Font ---",
    [STR_FONT] = "Font",
    [STR_FONT_BUILTIN] = "Built-in",
    [STR_FONT_CUSTOM] = "Custom Path...",

    // Language names
    [STR_LANG_NAME_EN] = "English",
    [STR_LANG_NAME_ZH] = "简体中文",
    [STR_LANG_NAME_JA] = "日本語",

    // Font names
    [STR_FONT_NAME_BUILTIN] = "Built-in",
    [STR_FONT_NAME_LXGW] = "LXGW WenKai",

    // Difficulty
    [STR_DIFFICULTY] = "Difficulty",
    [STR_DIFFICULTY_PEACEFUL] = "Peaceful",
    [STR_DIFFICULTY_EASY] = "Easy",
    [STR_DIFFICULTY_NORMAL] = "Normal",
    [STR_DIFFICULTY_HARD] = "Hard",

    // Achievements
    [STR_ACH_FIRST_STEPS] = "First Steps - Craft a wooden pickaxe",
    [STR_ACH_DEEP_DIG] = "Deep Dig - Reach the depths",
    [STR_ACH_MONSTER_HUNTER] = "Monster Hunter - Slay 100 mobs",
    [STR_ACH_ARCHITECT] = "Architect - Place 1000 blocks",
    [STR_ACH_REDSTONE_ENGINEER] = "Redstone Engineer - Power a lamp",
    [STR_ACH_COLLECTOR] = "Collector - Gather 20 item types",
    [STR_ACH_ANGLER] = "Angler - Catch a fish",
    [STR_ACH_BREEDER] = "Breeder - Raise a baby animal",
    [STR_ACH_ENCHANTER] = "Enchanter - Enchant an item",
    [STR_ACH_DEMOLITION] = "Demolition - Detonate TNT",
    [STR_COLLECTION_TITLE] = "Collection",
    [STR_COLLECTION_CHALLENGES] = "Electronic Challenges",
    [STR_COLLECTION_UNLOCKED] = "Unlocked",
    [STR_COLLECTION_LOCKED] = "Locked",
    [STR_COLLECTION_CLOSE] = "I / ESC: Close",

    // Status Messages
    [STR_MSG_GAME_SAVED] = "Game Saved",
    [STR_MSG_HEALED_XP] = "Healed with XP!",
    [STR_MSG_NOT_ENOUGH_XP] = "Not enough XP!",
    [STR_MSG_INVENTORY_FULL] = "Inventory full!",
    [STR_MSG_TOOL_BROKE] = "Tool broke!",
    [STR_MSG_TOOL_WEARING] = "Tool is wearing out!",
    [STR_MSG_FALL_DAMAGE] = "Ouch! Fall damage!",
    [STR_MSG_SPAWN_SET] = "Spawn point set!",
    [STR_MSG_SLEEP] = "Good morning!",
    [STR_MSG_SLEEP_ONLY_NIGHT] = "You can only sleep at night",
    [STR_MSG_HUNGRY] = "Hungry!",
    [STR_MSG_STARVING] = "Starving!",
    [STR_MSG_ATE] = "Ate %s (+%d hunger)",
    [STR_MSG_TRADE] = "Traded %s for %s",
    [STR_MSG_NOT_ENOUGH_ITEMS] = "Not enough items!",
    [STR_MSG_BROKE] = "%s broke!",
    [STR_MSG_CRIT_HIT] = "Critical Hit!",

    // Tooltips
    [STR_TOOLTIP_ARMOR] = "+%d armor  Dur: %d%%",
    [STR_TOOLTIP_HUNGER] = "+%d hunger",
    [STR_TOOLTIP_DURABILITY] = "Durability: %d%%",
    [STR_TOOLTIP_DUR_SHORT] = "Dur: %d%%",

    // Debug
    [STR_DBG_FPS] = "FPS: %d",
    [STR_DBG_POS] = "Pos: %.1f, %.1f",
    [STR_DBG_BLOCK] = "Block: %d, %d",
    [STR_DBG_CHUNKS] = "Chunks: %d",
    [STR_DBG_TIME] = "Time: %.2f",
    [STR_DBG_LIGHT] = "Light: %.2f",
    [STR_DBG_GROUND] = "OnGround: %s",
    [STR_DBG_HP] = "HP: %d/%d  Hunger: %d/%d",
    [STR_DBG_OXYGEN] = "Oxygen: %d/%d  XP: %d/%d",
    [STR_DBG_UNDERWATER] = "Underwater: %s",
    [STR_DBG_SEED] = "Seed: %u",
    [STR_DBG_WEATHER] = "Weather: %s",
    [STR_DBG_MODE] = "Mode: %s",
    [STR_WEATHER_CLEAR] = "Clear",
    [STR_WEATHER_RAIN] = "Rain",
    [STR_WEATHER_THUNDER] = "Thunder",
    [STR_YES] = "yes",
    [STR_NO] = "no",

    // Block/Item Names (order matches BlockType enum exactly)
    [STR_BLOCK_AIR] = "Air",
    [STR_BLOCK_GRASS] = "Grass",
    [STR_BLOCK_DIRT] = "Dirt",
    [STR_BLOCK_STONE] = "Stone",
    [STR_BLOCK_COBBLESTONE] = "Cobblestone",
    [STR_BLOCK_WOOD] = "Wood",
    [STR_BLOCK_LEAVES] = "Leaves",
    [STR_BLOCK_SAND] = "Sand",
    [STR_BLOCK_WATER] = "Water",
    [STR_BLOCK_COAL_ORE] = "Coal Ore",
    [STR_BLOCK_IRON_ORE] = "Iron Ore",
    [STR_BLOCK_PLANKS] = "Planks",
    [STR_BLOCK_BRICK] = "Brick",
    [STR_BLOCK_GLASS] = "Glass",
    [STR_BLOCK_BEDROCK] = "Bedrock",
    [STR_BLOCK_GRAVEL] = "Gravel",
    [STR_BLOCK_CLAY] = "Clay",
    [STR_BLOCK_SANDSTONE] = "Sandstone",
    [STR_BLOCK_TORCH] = "Torch",
    [STR_BLOCK_FLOWER] = "Flower",
    [STR_BLOCK_TALL_GRASS] = "Tall Grass",
    [STR_BLOCK_FURNACE] = "Furnace",
    [STR_BLOCK_BED] = "Bed",
    [STR_ITEM_STICK] = "Stick",
    [STR_ITEM_COAL] = "Coal",
    [STR_ITEM_IRON_INGOT] = "Iron Ingot",
    [STR_TOOL_WOOD_PICKAXE] = "Wood Pickaxe",
    [STR_TOOL_WOOD_AXE] = "Wood Axe",
    [STR_TOOL_WOOD_SWORD] = "Wood Sword",
    [STR_TOOL_WOOD_SHOVEL] = "Wood Shovel",
    [STR_TOOL_STONE_PICKAXE] = "Stone Pickaxe",
    [STR_TOOL_STONE_AXE] = "Stone Axe",
    [STR_TOOL_STONE_SWORD] = "Stone Sword",
    [STR_TOOL_STONE_SHOVEL] = "Stone Shovel",
    [STR_TOOL_IRON_PICKAXE] = "Iron Pickaxe",
    [STR_TOOL_IRON_AXE] = "Iron Axe",
    [STR_TOOL_IRON_SWORD] = "Iron Sword",
    [STR_TOOL_IRON_SHOVEL] = "Iron Shovel",
    [STR_FOOD_RAW_PORK] = "Raw Pork",
    [STR_FOOD_COOKED_PORK] = "Cooked Pork",
    [STR_FOOD_APPLE] = "Apple",
    [STR_FOOD_BREAD] = "Bread",
    [STR_BLOCK_CRAFTING_TABLE] = "Crafting Table",
    [STR_ARMOR_WOOD_HELMET] = "Wood Helmet",
    [STR_ARMOR_WOOD_CHESTPLATE] = "Wood Chestplate",
    [STR_ARMOR_WOOD_LEGGINGS] = "Wood Leggings",
    [STR_ARMOR_WOOD_BOOTS] = "Wood Boots",
    [STR_ARMOR_STONE_HELMET] = "Stone Helmet",
    [STR_ARMOR_STONE_CHESTPLATE] = "Stone Chestplate",
    [STR_ARMOR_STONE_LEGGINGS] = "Stone Leggings",
    [STR_ARMOR_STONE_BOOTS] = "Stone Boots",
    [STR_ARMOR_IRON_HELMET] = "Iron Helmet",
    [STR_ARMOR_IRON_CHESTPLATE] = "Iron Chestplate",
    [STR_ARMOR_IRON_LEGGINGS] = "Iron Leggings",
    [STR_ARMOR_IRON_BOOTS] = "Iron Boots",
    [STR_BLOCK_GOLD_ORE] = "Gold Ore",
    [STR_BLOCK_DIAMOND_ORE] = "Diamond Ore",
    [STR_BLOCK_REDSTONE_ORE] = "Redstone Ore",
    [STR_BLOCK_LAPIS_ORE] = "Lapis Lazuli Ore",
    [STR_ITEM_GOLD_INGOT] = "Gold Ingot",
    [STR_ITEM_DIAMOND] = "Diamond",
    [STR_ITEM_REDSTONE] = "Redstone",
    [STR_ITEM_LAPIS] = "Lapis Lazuli",
    [STR_BLOCK_CHEST] = "Chest",
    [STR_TOOL_GOLD_PICKAXE] = "Gold Pickaxe",
    [STR_TOOL_GOLD_AXE] = "Gold Axe",
    [STR_TOOL_GOLD_SWORD] = "Gold Sword",
    [STR_TOOL_GOLD_SHOVEL] = "Gold Shovel",
    [STR_TOOL_DIAMOND_PICKAXE] = "Diamond Pickaxe",
    [STR_TOOL_DIAMOND_AXE] = "Diamond Axe",
    [STR_TOOL_DIAMOND_SWORD] = "Diamond Sword",
    [STR_TOOL_DIAMOND_SHOVEL] = "Diamond Shovel",
    [STR_ARMOR_GOLD_HELMET] = "Gold Helmet",
    [STR_ARMOR_GOLD_CHESTPLATE] = "Gold Chestplate",
    [STR_ARMOR_GOLD_LEGGINGS] = "Gold Leggings",
    [STR_ARMOR_GOLD_BOOTS] = "Gold Boots",
    [STR_ARMOR_DIAMOND_HELMET] = "Diamond Helmet",
    [STR_ARMOR_DIAMOND_CHESTPLATE] = "Diamond Chestplate",
    [STR_ARMOR_DIAMOND_LEGGINGS] = "Diamond Leggings",
    [STR_ARMOR_DIAMOND_BOOTS] = "Diamond Boots",
    [STR_ITEM_GUNPOWDER] = "Gunpowder",
    [STR_ITEM_STRING] = "String",
    [STR_ITEM_BONE] = "Bone",
    [STR_ITEM_ARROW] = "Arrow",
    [STR_ITEM_BOW] = "Bow",
    [STR_BLOCK_MOSSY_COBBLESTONE] = "Mossy Cobblestone",
    [STR_BLOCK_BOOKSHELF] = "Bookshelf",
    [STR_BLOCK_LANTERN] = "Lantern",
    [STR_BLOCK_BONE_BLOCK] = "Bone Block",
    [STR_BLOCK_SNOW] = "Snow",
    [STR_BLOCK_ICE] = "Ice",
    [STR_BLOCK_PACKED_ICE] = "Packed Ice",
    [STR_BLOCK_MUD] = "Mud",
    [STR_BLOCK_MOSS_BLOCK] = "Moss Block",
    [STR_BLOCK_JUNGLE_WOOD] = "Jungle Wood",
    [STR_BLOCK_JUNGLE_LEAVES] = "Jungle Leaves",
    [STR_BLOCK_VINE] = "Vine",
    [STR_BLOCK_PUMPKIN] = "Pumpkin",
    [STR_BLOCK_MELON] = "Melon",
    [STR_BLOCK_SNOWY_GRASS] = "Snowy Grass",
    [STR_BLOCK_COARSE_DIRT] = "Coarse Dirt",
    [STR_BLOCK_PODZOL] = "Podzol",
    [STR_ITEM_SLIMEBALL] = "Slimeball",
    [STR_ITEM_ENDER_PEARL] = "Ender Pearl",
    [STR_ITEM_BUCKET] = "Bucket",
    [STR_ITEM_WATER_BUCKET] = "Water Bucket",
    [STR_ITEM_LAVA_BUCKET] = "Lava Bucket",
    [STR_BLOCK_LAVA] = "Lava",
    [STR_BLOCK_OBSIDIAN] = "Obsidian",
    [STR_BLOCK_ENCHANTING_TABLE] = "Enchanting Table",
    // Redstone blocks
    [STR_BLOCK_LEVER] = "Lever",
    [STR_BLOCK_REDSTONE_WIRE] = "Redstone Wire",
    [STR_BLOCK_REDSTONE_LAMP] = "Redstone Lamp",
    [STR_BLOCK_STONE_PRESSURE_PLATE] = "Stone Pressure Plate",

    // Recipe Names
    [STR_RECIPE_WOOD_PLANKS] = "Wood -> 4 Planks",
    [STR_RECIPE_PLANKS_STICKS] = "2 Planks -> 4 Sticks",
    [STR_RECIPE_WOOD_PICK] = "3 Planks -> Wood Pickaxe",
    [STR_RECIPE_WOOD_AXE] = "3 Planks -> Wood Axe",
    [STR_RECIPE_WOOD_SWORD] = "2 Planks -> Wood Sword",
    [STR_RECIPE_WOOD_SHOVEL] = "2 Planks -> Wood Shovel",
    [STR_RECIPE_STONE_PICK] = "3 Cobble -> Stone Pickaxe",
    [STR_RECIPE_STONE_AXE] = "3 Cobble -> Stone Axe",
    [STR_RECIPE_STONE_SWORD] = "2 Cobble -> Stone Sword",
    [STR_RECIPE_STONE_SHOVEL] = "2 Cobble -> Stone Shovel",
    [STR_RECIPE_IRON_PICK] = "3 Iron -> Iron Pickaxe",
    [STR_RECIPE_IRON_AXE] = "3 Iron -> Iron Axe",
    [STR_RECIPE_IRON_SWORD] = "2 Iron -> Iron Sword",
    [STR_RECIPE_IRON_SHOVEL] = "2 Iron -> Iron Shovel",
    [STR_RECIPE_BRICK] = "4 Clay -> 4 Bricks",
    [STR_RECIPE_FURNACE] = "8 Cobble -> Furnace",
    [STR_RECIPE_TORCHES] = "2 Sticks -> 4 Torches",
    [STR_RECIPE_SANDSTONE] = "4 Sand -> Sandstone",
    [STR_RECIPE_COBBLE] = "4 Gravel -> 4 Cobblestone",
    [STR_RECIPE_BED] = "6 Planks -> Bed",
    [STR_RECIPE_CRAFTING_TABLE] = "8 Planks -> Crafting Table",
    // Bread recipe name
    [STR_RECIPE_APPLE] = "8 Leaves -> Apple",
    // Advanced armor
    [STR_RECIPE_WOOD_HELMET] = "5 Planks -> Wood Helmet",
    [STR_RECIPE_WOOD_CHEST] = "8 Planks -> Wood Chestplate",
    [STR_RECIPE_WOOD_LEGS] = "7 Planks -> Wood Leggings",
    [STR_RECIPE_WOOD_BOOTS] = "4 Planks -> Wood Boots",
    [STR_RECIPE_STONE_HELMET] = "5 Cobble -> Stone Helmet",
    [STR_RECIPE_STONE_CHEST] = "8 Cobble -> Stone Chestplate",
    [STR_RECIPE_STONE_LEGS] = "7 Cobble -> Stone Leggings",
    [STR_RECIPE_STONE_BOOTS] = "4 Cobble -> Stone Boots",
    [STR_RECIPE_IRON_HELMET] = "5 Iron -> Iron Helmet",
    [STR_RECIPE_IRON_CHEST] = "8 Iron -> Iron Chestplate",
    [STR_RECIPE_IRON_LEGS] = "7 Iron -> Iron Leggings",
    [STR_RECIPE_IRON_BOOTS] = "4 Iron -> Iron Boots",
    [STR_RECIPE_GOLD_PICK] = "3 Gold -> Gold Pickaxe",
    [STR_RECIPE_GOLD_AXE] = "3 Gold -> Gold Axe",
    [STR_RECIPE_GOLD_SWORD] = "2 Gold -> Gold Sword",
    [STR_RECIPE_GOLD_SHOVEL] = "2 Gold -> Gold Shovel",
    [STR_RECIPE_GOLD_HELMET] = "5 Gold -> Gold Helmet",
    [STR_RECIPE_GOLD_CHEST] = "8 Gold -> Gold Chestplate",
    [STR_RECIPE_GOLD_LEGS] = "7 Gold -> Gold Leggings",
    [STR_RECIPE_GOLD_BOOTS] = "4 Gold -> Gold Boots",
    [STR_RECIPE_DIAMOND_PICK] = "3 Diamond -> Diamond Pickaxe",
    [STR_RECIPE_DIAMOND_AXE] = "3 Diamond -> Diamond Axe",
    [STR_RECIPE_DIAMOND_SWORD] = "2 Diamond -> Diamond Sword",
    [STR_RECIPE_DIAMOND_SHOVEL] = "2 Diamond -> Diamond Shovel",
    [STR_RECIPE_DIAMOND_HELMET] = "5 Diamond -> Diamond Helmet",
    [STR_RECIPE_DIAMOND_CHEST] = "8 Diamond -> Diamond Chestplate",
    [STR_RECIPE_DIAMOND_LEGS] = "7 Diamond -> Diamond Leggings",
    [STR_RECIPE_DIAMOND_BOOTS] = "4 Diamond -> Diamond Boots",
    [STR_RECIPE_CHEST] = "8 Planks -> Chest",
    [STR_RECIPE_ARROW] = "Stick -> 4 Arrows",
    [STR_RECIPE_BOW] = "3 Sticks + 3 String -> Bow",
    [STR_RECIPE_BONE_BLOCK] = "9 Bone -> Bone Block",
    [STR_RECIPE_BONE_BLOCK_DECOMP] = "Bone Block -> 9 Bone",
    [STR_RECIPE_BOOKSHELF] = "6 Planks -> Bookshelf",
    [STR_RECIPE_LANTERN] = "4 Torches -> Lantern",
    [STR_RECIPE_BUCKET] = "3 Iron Ingots -> Bucket",
    [STR_RECIPE_ENCHANTING_TABLE] = "4 Obsidian + 2 Diamond -> Table",
    // Redstone recipes
    [STR_RECIPE_REDSTONE_WIRE] = "Redstone -> Wire",
    [STR_RECIPE_LEVER] = "Cobblestone -> Lever",
    [STR_RECIPE_REDSTONE_LAMP] = "4 Redstone -> Lamp",
    [STR_RECIPE_STONE_BUTTON] = "石のボタン",
    [STR_RECIPE_REDSTONE_REPEATER] = "レッドストーンリピーター",
    [STR_RECIPE_PISTON] = "ピストン",
    [STR_RECIPE_IRON_DOOR] = "鉄の扉",
    [STR_RECIPE_GLASS_BOTTLE] = "ガラス瓶",
    [STR_RECIPE_POTION_SPEED] = "速度のポーション",
    [STR_RECIPE_POTION_STRENGTH] = "力のポーション",
    [STR_RECIPE_POTION_REGEN] = "再生のポーション",
    [STR_RECIPE_POTION_FIRE_RESISTANCE] = "火炎耐性のポーション",
    [STR_RECIPE_POTION_WATER_BREATHING] = "水中呼吸のポーション",
    [STR_RECIPE_POTION_POISON] = "毒のポーション",
    [STR_RECIPE_PRESSURE_PLATE] = "2 Stone -> Pressure Plate",
    // Smelt
    [STR_SMELT_IRON] = "Iron Ore -> Iron Ingot",
    [STR_SMELT_PORK] = "Raw Pork -> Cooked Pork",
    [STR_SMELT_COBBLE] = "Cobblestone -> Stone",
    [STR_SMELT_SAND] = "Sand -> Glass",
    [STR_SMELT_GOLD] = "Gold Ore -> Gold Ingot",
    [STR_SMELT_REDSTONE] = "Redstone Ore -> Redstone",
    [STR_SMELT_LAPIS] = "Lapis Ore -> Lapis Lazuli",
    [STR_SMELT_CLAY] = "Clay -> Bricks",
    [STR_SMELT_ICE] = "Ice -> Water",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "No fuel",
    [STR_MSG_SMELTING] = "Smelting: %s",
    [STR_MSG_CANNOT_SMELT] = "Cannot smelt this item",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "Toggle fullscreen",

    // Multiplayer
    [STR_BTN_HOST_GAME] = "Host Game",
    [STR_BTN_JOIN_GAME] = "Join Game",
    [STR_HOST_WAITING] = "Waiting for players...",
    [STR_HOST_IP_HINT] = "Your IP: %s:%d",
    [STR_JOIN_TITLE] = "Join Game",
    [STR_JOIN_IP_HINT] = "Enter host IP address:",
    [STR_JOIN_CONNECTING] = "Connecting...",
    [STR_HOST_PLAYERS_COUNT] = "Players: %d / %d",
    [STR_HOST_CANCEL_HINT] = "ESC: Cancel hosting",
    [STR_HOST_START_HINT] = "ENTER / SPACE: Start game",
    [STR_JOIN_HELP_HINT] = "ESC: Back     Enter: Connect",
    [STR_JOIN_TAB_HINT] = "Tab: switch field",
    [STR_OPEN_TO_LAN] = "Open to LAN",
    [STR_LAN_OPENED] = "Opened to LAN - others can join at %s:%d",
    [STR_LAN_STATUS] = "LAN open - %s:%d  (%d/%d)",
    [STR_LAN_FAILED] = "Failed to open to LAN",

    // Enchantment names
    [STR_ENCH_SHARPNESS] = "Sharpness",
    [STR_ENCH_EFFICIENCY] = "Efficiency",
    [STR_ENCH_PROTECTION] = "Protection",
    [STR_ENCH_FORTUNE] = "Fortune",
    [STR_ENCH_UNBREAKING] = "Unbreaking",
    [STR_ENCH_SILK_TOUCH] = "Silk Touch",
    [STR_ENCH_POWER] = "Power",
    [STR_ENCH_KNOCKBACK] = "Knockback",
    [STR_ENCH_FIRE_ASPECT] = "Fire Aspect",
    // Fishing items
    [STR_ITEM_FISHING_ROD] = "Fishing Rod",
    [STR_ITEM_RAW_FISH] = "Raw Fish",
    [STR_ITEM_COOKED_FISH] = "Cooked Fish",
    // Cauldron
    [STR_BLOCK_CAULDRON] = "Cauldron",
    [STR_ENCHANTED] = "Enchanted",

    // Enchanting messages
    [STR_MSG_ENCHANTED] = "Enchanted!",
    [STR_MSG_ALREADY_ENCHANTED] = "Already enchanted",

    // Tutorial and multiplayer
    [STR_TUTORIAL_CONTROLS] = "WASD: Move | Space: Jump | LMB: Break | RMB: Place | E: Inventory",
    [STR_NET_PLAYER_JOINED] = "Player joined!",
    [STR_NET_PLAYER_LEFT] = "Player disconnected",
    [STR_NET_HOST_DISCONNECTED] = "Host disconnected",

    // Missing messages
    [STR_MSG_NO_ARROWS] = "No arrows!",
    [STR_DEATH_LAVA] = " tried to swim in lava",
    [STR_DEATH_CACTUS] = " was pricked by a cactus",

    // Ender pearl
    [STR_MSG_ENDER_PEARL] = "Teleported!",
    [STR_FISH_CAST] = "Cast line!",
    [STR_FISH_BITE] = "Fish biting!",
    [STR_FISH_CATCH] = "Caught something!",
    [STR_FISH_RETRACT] = "Line retracted",
    [STR_MSG_CAULDRON_FILLED] = "Cauldron filled!",
    [STR_MSG_CAULDRON_EMPTY] = "Cauldron emptied!",
    [STR_MSG_CAULDRON_DRINK] = "Drank from cauldron",
    [STR_BLOCK_OAK_STAIRS] = "Oak Stairs",
    [STR_BLOCK_COBBLESTONE_STAIRS] = "Cobblestone Stairs",
    [STR_BLOCK_STONE_BRICKS] = "Stone Bricks",
    [STR_BLOCK_CHISELED_STONE_BRICKS] = "Chiseled Stone Bricks",
    [STR_BLOCK_OAK_SLAB] = "Oak Slab",
    [STR_BLOCK_COBBLESTONE_SLAB] = "Cobblestone Slab",
    [STR_RECIPE_OAK_STAIRS] = "4 Planks + -> 4 Oak Stairs",
    [STR_RECIPE_COBBLESTONE_STAIRS] = "4 Cobblestone + -> 4 Cobblestone Stairs",
    [STR_RECIPE_STONE_BRICKS] = "4 Stone -> 4 Stone Bricks",
    [STR_RECIPE_CHISELED_STONE_BRICKS] = "2 Stone Brick Slab -> Chiseled Stone Bricks",
    [STR_RECIPE_OAK_SLAB] = "3 Planks -> 6 Oak Slabs",
    [STR_RECIPE_COBBLESTONE_SLAB] = "3 Cobblestone -> 6 Cobblestone Slabs",
    [STR_BLOCK_CACTUS] = "Cactus",
    [STR_BLOCK_SUGAR_CANE] = "Sugar Cane",
    [STR_ITEM_PAPER] = "Paper",
    [STR_ITEM_BOOK] = "Book",
    [STR_ITEM_SUGAR] = "Sugar",
    [STR_BLOCK_TNT] = "TNT",
    [STR_BLOCK_RED_SAND] = "Red Sand",
    [STR_BLOCK_MYCELIUM] = "Mycelium",
    [STR_BLOCK_MUSHROOM_BLOCK] = "Mushroom Block",
    [STR_BLOCK_MUSHROOM_STEM] = "Mushroom Stem",
    [STR_BLOCK_STONE_BUTTON] = "Stone Button",
    [STR_BLOCK_REDSTONE_REPEATER] = "Redstone Repeater",
    [STR_BLOCK_PISTON] = "Piston",
    [STR_BLOCK_IRON_DOOR] = "Iron Door",
    [STR_ITEM_GLASS_BOTTLE] = "Glass Bottle",
    [STR_ITEM_POTION_WATER] = "Water Bottle",
    [STR_ITEM_POTION_SPEED] = "Potion of Speed",
    [STR_ITEM_POTION_STRENGTH] = "Potion of Strength",
    [STR_ITEM_POTION_REGEN] = "Potion of Regeneration",
    [STR_ITEM_POTION_FIRE_RESISTANCE] = "Potion of Fire Resistance",
    [STR_ITEM_POTION_WATER_BREATHING] = "Potion of Water Breathing",
    [STR_ITEM_POTION_POISON] = "Potion of Poison",
    [STR_MSG_DRANK_POTION] = "You feel the effect!",
    [STR_MSG_BOTTLE_FILLED] = "Bottle filled with water",
    [STR_MSG_IRON_DOOR_HINT] = "Iron doors open with redstone.",
    [STR_RECIPE_TNT] = "TNT",
    [STR_RECIPE_PAPER] = "3 Sugar Cane -> 3 Paper",
    [STR_RECIPE_BOOK] = "2 Paper + 1 Leather -> 1 Book",
    [STR_RECIPE_SUGAR] = "1 Sugar Cane -> 1 Sugar",
    [STR_MSG_CACTUS_DAMAGE] = "Ouch! Cactus hurts!",
    [STR_TYPE_TOOL] = "Tool",
    [STR_TYPE_ARMOR] = "Armor",
    [STR_TYPE_FOOD] = "Food",
    [STR_TYPE_BLOCK] = "Block",
    [STR_RECIPE_SLIMEBALL_STRING] = "Slimeball Fiber",

    // Farming
    [STR_TOOL_WOOD_HOE] = "Wood Hoe",
    [STR_TOOL_STONE_HOE] = "Stone Hoe",
    [STR_TOOL_IRON_HOE] = "Iron Hoe",
    [STR_TOOL_GOLD_HOE] = "Gold Hoe",
    [STR_TOOL_DIAMOND_HOE] = "Diamond Hoe",
    [STR_ITEM_WHEAT_SEEDS] = "Wheat Seeds",
    [STR_ITEM_WHEAT] = "Wheat",
    [STR_BLOCK_FARMLAND] = "Farmland",
    [STR_BLOCK_CROPS] = "Crops",
    [STR_BLOCK_HAY_BALE] = "Hay Bale",
    [STR_RECIPE_WOOD_HOE] = "Wood Hoe",
    [STR_RECIPE_STONE_HOE] = "Stone Hoe",
    [STR_RECIPE_IRON_HOE] = "Iron Hoe",
    [STR_RECIPE_GOLD_HOE] = "Gold Hoe",
    [STR_RECIPE_DIAMOND_HOE] = "Diamond Hoe",
    [STR_RECIPE_BREAD] = "Bread",
    [STR_RECIPE_HAY_BALE] = "Hay Bale",
    [STR_RECIPE_FISHING_ROD] = "3 Sticks + 2 String -> Fishing Rod",
    [STR_RECIPE_MOSSY_COBBLESTONE] = "Cobblestone -> Mossy Cobblestone",
    [STR_RECIPE_COARSE_DIRT] = "2 Dirt -> 2 Coarse Dirt",

    // New mob deaths
    [STR_DEATH_MOB_COW] = " was kicked by a cow",
    [STR_DEATH_MOB_SHEEP] = " was rammed by a sheep",
    [STR_DEATH_MOB_CHICKEN] = " was pecked to death",
    [STR_DEATH_MOB_WOLF] = " was mauled by a wolf",
    [STR_DEATH_MOB_WITCH] = " was slain by a witch",
    [STR_DEATH_MOB_BAT] = " was startled to death by a bat",

    // Mob names
    [STR_MOB_PIG] = "Pig",
    [STR_MOB_COW] = "Cow",
    [STR_MOB_SHEEP] = "Sheep",
    [STR_MOB_CHICKEN] = "Chicken",
    [STR_MOB_VILLAGER] = "Villager",
    [STR_MOB_HORSE] = "Horse",
    [STR_MOB_WOLF] = "Wolf",
    [STR_MOB_WITCH] = "Witch",
    [STR_MOB_BAT] = "Bat",

    // Animal drops
    [STR_ITEM_RAW_BEEF] = "Raw Beef",
    [STR_ITEM_LEATHER] = "Leather",
    [STR_ITEM_RAW_MUTTON] = "Raw Mutton",
    [STR_ITEM_WOOL] = "Wool",
    [STR_ITEM_RAW_CHICKEN] = "Raw Chicken",
    [STR_ITEM_FEATHER] = "Feather",
    [STR_ITEM_EGG] = "Egg",
    [STR_ITEM_COOKED_BEEF] = "Cooked Beef",
    [STR_ITEM_COOKED_MUTTON] = "Cooked Mutton",
    [STR_ITEM_COOKED_CHICKEN] = "Cooked Chicken",

    // Smelt recipes
    [STR_SMELT_BEEF] = "Cook Beef",
    [STR_SMELT_MUTTON] = "Cook Mutton",
    [STR_SMELT_CHICKEN] = "Cook Chicken",
    [STR_SMELT_FISH] = "Cook Fish",
    [STR_MSG_CANT_SLEEP_MOBS] = "Monsters are nearby!",
    // Creative mode
    [STR_GAMEMODE] = "Mode",
    [STR_MODE_SURVIVAL] = "Survival",
    [STR_MODE_CREATIVE] = "Creative",
    [STR_MSG_FLIGHT_ON] = "Flight enabled",
    [STR_MSG_FLIGHT_OFF] = "Flight disabled",
    [STR_CREATIVE_TITLE] = "Creative Inventory",
    [STR_CREATIVE_SELECTED] = "Selected: %s",
    [STR_CREATIVE_SELECT_HINT] = "Click an item to pick it up (infinite)",
    [STR_CREATIVE_SEARCH] = "Search:",
    [STR_CREATIVE_NO_MATCH] = "No matches",
    [STR_MODE_SET] = "Game mode: %s",
    [STR_CREATIVE_BACKPACK] = "Backpack",
    [STR_HINT_SCROLL] = "Scroll to browse",
    [STR_MSG_HOST_ONLY] = "Only the host can change the game mode",
    // Easter eggs
    [STR_EGG_KONAMI_ON] = "Konami code! Party mode ON",
    [STR_EGG_KONAMI_OFF] = "Party mode OFF",
    [STR_EGG_TITLE_CLICK] = "Hey, stop poking the title!",
    [STR_EGG_FACT1] = "Fun fact: bedrock is unbreakable for a reason...",
    [STR_EGG_FACT2] = "Fun fact: the void is a one-way ticket.",
    [STR_EGG_FACT3] = "Fun fact: sheep prefer the color white.",
},

// ===== SIMPLIFIED CHINESE =====
[LANG_ZH_CN] = {
    [STR_NONE] = "",

    [STR_TITLE] = "MyWorld",
    [STR_SUBTITLE] = "2D沙箱冒险游戏",  // 2D沙箱冒险游戏
    [STR_BTN_NEW_GAME] = "新建世界",             // 新建世界
    [STR_BTN_LOAD_GAME] = "加载世界",            // 加载世界
    [STR_BTN_SETTINGS] = "设置",                          // 设置
    [STR_BTN_QUIT] = "退出",                              // 退出
    [STR_HINT_NAVIGATE] = "方向键/WASD: 导航  |  Enter/空格: 确认",
    [STR_HINT_CONTROLS] = "WASD: 移动  |  空格: 跳跃  |  E: 背包  |  ESC: 暂停",

    [STR_NEW_GAME_TITLE] = "新建世界",
    [STR_LOAD_GAME_TITLE] = "加载世界",
    [STR_NEW_GAME_SUB] = "选择一个存档位创建新世界",
    [STR_LOAD_GAME_SUB] = "选择一个存档继续游戏",
    [STR_SEED] = "种子:",                                 // 种子:
    [STR_RANDOM] = "随机",                                // 随机
    [STR_GENERATING_WORLD] = "正在生成世界...",                  // 正在生成世界
    [STR_SLOT] = "存档 %d",                               // 存档 %d
    [STR_SEED_DISPLAY] = "种子: %u",
    [STR_OCCUPIED] = "已占用",                        // 已占用
    [STR_EMPTY_NEW] = "空 - 点击创建",       // 空 - 点击创建
    [STR_EMPTY_LOAD] = "无存档数据",         // 无存档数据
    [STR_BACK] = "返回",                                  // 返回
    [STR_HINT_SLOT] = "方向键: 导航  |  Enter: 选择  |  DEL: 删除  |  ESC: 返回",

    [STR_DELETE_SAVE] = "删除存档?",             // 删除存档?
    [STR_OVERWRITE_SAVE] = "覆盖存档?",          // 覆盖存档?
    [STR_SLOT_HAS_DATA] = "存档 %d 已有数据。",
    [STR_CANNOT_UNDO] = "此操作无法撤销!",  // 此操作无法撤销!
    [STR_YES_DELETE] = "是,删除",                     // 是,删除
    [STR_YES_OVERWRITE] = "是,覆盖",                  // 是,覆盖
    [STR_CANCEL] = "取消",                                // 取消
    [STR_CONFIRM_KEYS] = "[Y] 确认    [N / ESC] 取消",

    [STR_PAUSED] = "已暂停",                          // 已暂停
    [STR_MUSIC_VOLUME] = "音乐音量",              // 音乐音量
    [STR_SFX_VOLUME] = "音效音量",               // 音效音量
    [STR_CONTROLS_TITLE] = "--- 操作 ---",               // --- 操作 ---
    [STR_CONTINUE] = "继续",                              // 继续
    [STR_MAIN_MENU] = "主菜单",                       // 主菜单

    [STR_KEY_WASD] = "WASD",
    [STR_KEY_SPACE] = "空格",                             // 空格
    [STR_KEY_SHIFT] = "Shift",
    [STR_KEY_CTRL] = "Ctrl",
    [STR_KEY_LCLICK] = "左键",                            // 左键
    [STR_KEY_RCLICK] = "右键",                            // 右键
    [STR_KEY_E] = "E",
    [STR_KEY_H] = "H",
    [STR_KEY_F3] = "F3",
    [STR_KEY_ESC] = "ESC",
    [STR_KEY_19] = "1-9",

    [STR_ACT_MOVE] = "移动",                              // 移动
    [STR_ACT_JUMP] = "跳跃/游泳",                // 跳跃/游泳
    [STR_ACT_SPRINT] = "冲刺",
    [STR_ACT_SNEAK] = "潜行",                            // 冲刺
    [STR_ACT_BREAK] = "破坏/攻击",               // 破坏/攻击
    [STR_ACT_PLACE] = "放置/食用",               // 放置/食用
    [STR_ACT_INVENTORY] = "背包",                          // 背包
    [STR_ACT_HEAL] = "治疗(XP)",                          // 治疗(XP)
    [STR_ACT_DEBUG] = "调试",                             // 调试
    [STR_ACT_PAUSE] = "暂停",                             // 暂停
    [STR_ACT_HOTBAR] = "快捷栏",                      // 快捷栏

    [STR_YOU_DIED] = "你死了!",                      // 你死了!
    [STR_PRESS_SPACE_RESPAWN] = "按空格键复活",  // 按空格键复活
    [STR_PRESS_ESC_MENU] = "按ESC返回主菜单",    // 按ESC返回主菜单
    [STR_DEATH_FALL] = "从高处摔落",
    [STR_DEATH_DROWN] = "溺水身亡",
    [STR_DEATH_STARVE] = "饥饿致死",
    [STR_DEATH_MOB_ZOMBIE] = "被僵尸杀死",
    [STR_DEATH_MOB_SKELETON] = "被骷髅射杀",
    [STR_DEATH_MOB_CREEPER] = "被苦力怕炸死",
    [STR_DEATH_MOB_SPIDER] = "被蜘蛛杀死",
    [STR_DEATH_MOB_SLIME] = "被史莱姆杀死",
    [STR_DEATH_MOB_ENDERMAN] = "被末影人杀死",
    [STR_DEATH_VOID] = "掉出了世界",
    [STR_DEATH_SCORE] = "得分: %d XP",

    [STR_INVENTORY] = "背包",                              // 背包
    [STR_SORT] = "整理",                                  // 整理

    [STR_WORLD_MAP] = "世界地図",                // 世界地図
    [STR_PRESS_M_CLOSE] = "按M关闭",                 // 按M关闭
    [STR_PLAYER_COORD] = "玩家: %d, %d",                 // 玩家: %d, %d

    [STR_CRAFTING] = "合成",                              // 合成
    [STR_CRAFTING_TABLE] = "合成台",                  // 合成台
    [STR_SEARCH] = "搜索...",                             // 搜索...
    [STR_NO_MATCHES] = "无匹配项",               // 无匹配项
    [STR_NO_RECIPES] = "无配方",                     // 无配方

    [STR_FURNACE] = "熔炉",                               // 熔炉
    [STR_FUEL] = "燃料",                                  // 燃料
    [STR_INPUT] = "输入",                                 // 输入
    [STR_OUTPUT] = "输出",                                // 输出
    [STR_PRESS_E_ESC_CLOSE] = "按E或ESC关闭",    // 按E或ESC关闭

    [STR_SETTINGS] = "设置",                              // 设置
    [STR_SECTION_AUDIO] = "--- 音频 ---",                 // --- 音频 ---
    [STR_SOUND_EFFECTS] = "音效",                         // 音效
    [STR_SECTION_DISPLAY] = "--- 显示 ---",               // --- 显示 ---
    [STR_WINDOW_MODE] = "窗口模式",              // 窗口模式
    [STR_WINDOWED] = "窗口",                              // 窗口
    [STR_FULLSCREEN] = "全屏",                            // 全屏
    [STR_BORDERLESS] = "无边框",                      // 无边框
    [STR_RESOLUTION] = "分辨率",
    [STR_RES_960] = "960 × 540",
    [STR_RES_1280] = "1280 × 720",
    [STR_RES_1600] = "1600 × 900",
    [STR_SECTION_LANGUAGE] = "--- 语言 ---",              // --- 语言 ---
    [STR_LANGUAGE] = "语言",                              // 语言
    [STR_SECTION_FONT] = "--- 字体 ---",                  // --- 字体 ---
    [STR_FONT] = "字体",                                  // 字体
    [STR_FONT_BUILTIN] = "内置",                          // 内置
    [STR_FONT_CUSTOM] = "自定义路径...",     // 自定义路径...

    // Language names
    [STR_LANG_NAME_EN] = "English",
    [STR_LANG_NAME_ZH] = "简体中文",
    [STR_LANG_NAME_JA] = "日本語",

    // Font names
    [STR_FONT_NAME_BUILTIN] = "内置",
    [STR_FONT_NAME_LXGW] = "LXGW WenKai",

    // Difficulty
    [STR_DIFFICULTY] = "难度",
    [STR_DIFFICULTY_PEACEFUL] = "和平",
    [STR_DIFFICULTY_EASY] = "简单",
    [STR_DIFFICULTY_NORMAL] = "普通",
    [STR_DIFFICULTY_HARD] = "困难",

    // Achievements
    [STR_ACH_FIRST_STEPS] = "初出茅庐 - 制作木镐",
    [STR_ACH_DEEP_DIG] = "深入地底 - 到达基岩层",
    [STR_ACH_MONSTER_HUNTER] = "怪物猎人 - 击杀100只怪物",
    [STR_ACH_ARCHITECT] = "建筑师 - 放置1000个方块",
    [STR_ACH_REDSTONE_ENGINEER] = "红石工程师 - 点亮红石灯",
    [STR_ACH_COLLECTOR] = "收藏家 - 收集20种物品",
    [STR_ACH_ANGLER] = "垂钓者 - 钓到一条鱼",
    [STR_ACH_BREEDER] = "繁育者 - 培育幼崽动物",
    [STR_ACH_ENCHANTER] = "附魔师 - 附魔一件物品",
    [STR_ACH_DEMOLITION] = "爆破手 - 引爆 TNT",
    [STR_COLLECTION_TITLE] = "藏品",
    [STR_COLLECTION_CHALLENGES] = "电子竞赛挑战",
    [STR_COLLECTION_UNLOCKED] = "已解锁",
    [STR_COLLECTION_LOCKED] = "未解锁",
    [STR_COLLECTION_CLOSE] = "I / ESC：关闭",

    [STR_MSG_GAME_SAVED] = "游戏已保存",     // 游戏已保存
    [STR_MSG_HEALED_XP] = "用XP治疗了!",         // 用XP治疗了!
    [STR_MSG_NOT_ENOUGH_XP] = "XP不足!",                 // XP不足!
    [STR_MSG_INVENTORY_FULL] = "背包已满!",      // 背包已满!
    [STR_MSG_TOOL_BROKE] = "工具坏了!",          // 工具坏了!
    [STR_MSG_TOOL_WEARING] = "工具快坏了!",  // 工具快坏了!
    [STR_MSG_FALL_DAMAGE] = "哎哟! 摔伤了!", // 哎哟! 摔伤了!
    [STR_MSG_SPAWN_SET] = "已设置生成点!", // 已设置生成点!
    [STR_MSG_SLEEP] = "早安！",
    [STR_MSG_SLEEP_ONLY_NIGHT] = "只能在夜晚睡觉",
    [STR_MSG_HUNGRY] = "饥饿!",                           // 饥饿!
    [STR_MSG_STARVING] = "饥饿难耐!",            // 饥饿难耐!
    [STR_MSG_ATE] = "吃了 %s (+%d饥饿度)",   // 吃了 %s (+%d饥饿度)
    [STR_MSG_TRADE] = "用 %s 换 %s",
    [STR_MSG_NOT_ENOUGH_ITEMS] = "物品不够！",
    [STR_MSG_BROKE] = "%s 坏了!",                         // %s 坏了!
    [STR_MSG_CRIT_HIT] = "暴击!",

    [STR_TOOLTIP_ARMOR] = "+%d护甲  耐久: %d%%",  // +%d护甲  耐久: %d%%
    [STR_TOOLTIP_HUNGER] = "+%d饥饿度",               // +%d饥饿度
    [STR_TOOLTIP_DURABILITY] = "耐久度: %d%%",        // 耐久度: %d%%
    [STR_TOOLTIP_DUR_SHORT] = "耐久: %d%%",               // 耐久: %d%%

    [STR_DBG_FPS] = "帧率: %d",                           // 帧率: %d
    [STR_DBG_POS] = "位置: %.1f, %.1f",                  // 位置: %.1f, %.1f
    [STR_DBG_BLOCK] = "方块: %d, %d",                     // 方块: %d, %d
    [STR_DBG_CHUNKS] = "区块: %d",                        // 区块: %d
    [STR_DBG_TIME] = "时间: %.2f",                        // 时间: %.2f
    [STR_DBG_LIGHT] = "光照: %.2f",                       // 光照: %.2f
    [STR_DBG_GROUND] = "着地: %s",                        // 着地: %s
    [STR_DBG_HP] = "生命: %d/%d  饥饿: %d/%d",   // 生命: %d/%d  饥饿: %d/%d
    [STR_DBG_OXYGEN] = "氧气: %d/%d  XP: %d/%d",         // 氧气: %d/%d  XP: %d/%d
    [STR_DBG_UNDERWATER] = "水下: %s",                    // 水下: %s
    [STR_DBG_SEED] = "种子: %u",                          // 种子: %u
    [STR_DBG_WEATHER] = "天气: %s",
    [STR_DBG_MODE] = "模式：%s",
    [STR_WEATHER_CLEAR] = "晴",
    [STR_WEATHER_RAIN] = "雨",
    [STR_WEATHER_THUNDER] = "雷暴",
    [STR_YES] = "是",                                          // 是
    [STR_NO] = "否",                                           // 否

    // Block names
    [STR_BLOCK_AIR] = "空气",                             // 空气
    [STR_BLOCK_GRASS] = "草地",                           // 草地
    [STR_BLOCK_DIRT] = "泥土",                            // 泥土
    [STR_BLOCK_STONE] = "石头",                           // 石头
    [STR_BLOCK_COBBLESTONE] = "圆石",                     // 圆石
    [STR_BLOCK_WOOD] = "木头",                            // 木头
    [STR_BLOCK_LEAVES] = "树叶",                          // 树叶
    [STR_BLOCK_SAND] = "沙子",                            // 沙子
    [STR_BLOCK_WATER] = "水",                                 // 水
    [STR_BLOCK_COAL_ORE] = "煤矿",                        // 煤矿
    [STR_BLOCK_IRON_ORE] = "铁矿",                        // 铁矿
    [STR_BLOCK_PLANKS] = "木板",                          // 木板
    [STR_BLOCK_BRICK] = "砖块",                           // 砖块
    [STR_BLOCK_GLASS] = "玻璃",                           // 玻璃
    [STR_BLOCK_BEDROCK] = "基岩",                         // 基岩
    [STR_BLOCK_GRAVEL] = "碎石",                          // 碎石
    [STR_BLOCK_CLAY] = "粘土",                            // 粘土
    [STR_BLOCK_SANDSTONE] = "砂岩",                       // 砂岩
    [STR_BLOCK_TORCH] = "火把",                           // 火把
    [STR_BLOCK_FLOWER] = "花",                                // 花
    [STR_BLOCK_TALL_GRASS] = "高草",                      // 高草
    [STR_BLOCK_FURNACE] = "熔炉",                         // 熔炉
    [STR_BLOCK_BED] = "床",                                   // 床
    [STR_ITEM_STICK] = "棒子",                            // 棍子
    [STR_ITEM_COAL] = "煤炭",                             // 煤炭
    [STR_ITEM_IRON_INGOT] = "铁锭",                       // 铁锭
    [STR_TOOL_WOOD_PICKAXE] = "木镐",                     // 木镐
    [STR_TOOL_WOOD_AXE] = "木斟",                         // 木斧
    [STR_TOOL_WOOD_SWORD] = "木剑",                       // 木剑
    [STR_TOOL_WOOD_SHOVEL] = "木铲",                      // 木铲
    [STR_TOOL_STONE_PICKAXE] = "石镐",                    // 石镐
    [STR_TOOL_STONE_AXE] = "石斟",                        // 石斧
    [STR_TOOL_STONE_SWORD] = "石剑",                      // 石剑
    [STR_TOOL_STONE_SHOVEL] = "石铲",                     // 石铲
    [STR_TOOL_IRON_PICKAXE] = "铁镐",                     // 铁镐
    [STR_TOOL_IRON_AXE] = "铁斟",                         // 铁斧
    [STR_TOOL_IRON_SWORD] = "铁剑",                       // 铁剑
    [STR_TOOL_IRON_SHOVEL] = "铁铲",                      // 铁铲
    [STR_FOOD_RAW_PORK] = "生猪肉",                   // 生猪肉
    [STR_FOOD_COOKED_PORK] = "烤猪肉",                // 烤猪肉
    [STR_FOOD_APPLE] = "苹果",                            // 苹果
    [STR_FOOD_BREAD] = "面包",                            // 面包
    [STR_BLOCK_CRAFTING_TABLE] = "合成台",            // 合成台
    [STR_ARMOR_WOOD_HELMET] = "木头盔",               // 木头盔
    [STR_ARMOR_WOOD_CHESTPLATE] = "木胸甲",           // 木胸甲
    [STR_ARMOR_WOOD_LEGGINGS] = "木护腿",             // 木护腿
    [STR_ARMOR_WOOD_BOOTS] = "木靴子",                // 木靴子
    [STR_ARMOR_STONE_HELMET] = "石头盔",              // 石头盔
    [STR_ARMOR_STONE_CHESTPLATE] = "石胸甲",          // 石胸甲
    [STR_ARMOR_STONE_LEGGINGS] = "石护腿",            // 石护腿
    [STR_ARMOR_STONE_BOOTS] = "石靴子",               // 石靴子
    [STR_ARMOR_IRON_HELMET] = "铁头盔",               // 铁头盔
    [STR_ARMOR_IRON_CHESTPLATE] = "铁胸甲",           // 铁胸甲
    [STR_ARMOR_IRON_LEGGINGS] = "铁护腿",             // 铁护腿
    [STR_ARMOR_IRON_BOOTS] = "铁靴子",                // 铁靴子
    [STR_BLOCK_GOLD_ORE] = "金矿石",
    [STR_BLOCK_DIAMOND_ORE] = "钻石矿石",
    [STR_BLOCK_REDSTONE_ORE] = "红石矿石",
    [STR_BLOCK_LAPIS_ORE] = "青金石矿石",
    [STR_ITEM_GOLD_INGOT] = "金锭",
    [STR_ITEM_DIAMOND] = "钻石",
    [STR_ITEM_REDSTONE] = "红石",
    [STR_ITEM_LAPIS] = "青金石",
    [STR_BLOCK_CHEST] = "箱子",
    [STR_TOOL_GOLD_PICKAXE] = "金镐",
    [STR_TOOL_GOLD_AXE] = "金斧",
    [STR_TOOL_GOLD_SWORD] = "金剑",
    [STR_TOOL_GOLD_SHOVEL] = "金铲",
    [STR_TOOL_DIAMOND_PICKAXE] = "钻石镐",
    [STR_TOOL_DIAMOND_AXE] = "钻石斧",
    [STR_TOOL_DIAMOND_SWORD] = "钻石剑",
    [STR_TOOL_DIAMOND_SHOVEL] = "钻石铲",
    [STR_ARMOR_GOLD_HELMET] = "金头盔",
    [STR_ARMOR_GOLD_CHESTPLATE] = "金胸甲",
    [STR_ARMOR_GOLD_LEGGINGS] = "金护腿",
    [STR_ARMOR_GOLD_BOOTS] = "金靴子",
    [STR_ARMOR_DIAMOND_HELMET] = "钻石头盔",
    [STR_ARMOR_DIAMOND_CHESTPLATE] = "钻石胸甲",
    [STR_ARMOR_DIAMOND_LEGGINGS] = "钻石护腿",
    [STR_ARMOR_DIAMOND_BOOTS] = "钻石靴子",
    [STR_ITEM_GUNPOWDER] = "火药",
    [STR_ITEM_STRING] = "线",
    [STR_ITEM_BONE] = "骨头",
    [STR_ITEM_ARROW] = "箭",
    [STR_ITEM_BOW] = "弓",
    [STR_BLOCK_MOSSY_COBBLESTONE] = "苔石",
    [STR_BLOCK_BOOKSHELF] = "书架",
    [STR_BLOCK_LANTERN] = "灯笼",
    [STR_BLOCK_BONE_BLOCK] = "骨块",
    [STR_BLOCK_SNOW] = "雪",
    [STR_BLOCK_ICE] = "冰",
    [STR_BLOCK_PACKED_ICE] = "浮冰",
    [STR_BLOCK_MUD] = "泥巴",
    [STR_BLOCK_MOSS_BLOCK] = "苔藓块",
    [STR_BLOCK_JUNGLE_WOOD] = "丛林木",
    [STR_BLOCK_JUNGLE_LEAVES] = "丛林树叶",
    [STR_BLOCK_VINE] = "藤蔓",
    [STR_BLOCK_PUMPKIN] = "南瓜",
    [STR_BLOCK_MELON] = "西瓜",
    [STR_BLOCK_SNOWY_GRASS] = "雪草方块",
    [STR_BLOCK_COARSE_DIRT] = "砂土",
    [STR_BLOCK_PODZOL] = "灰化土",
    [STR_ITEM_SLIMEBALL] = "粘液球",
    [STR_ITEM_ENDER_PEARL] = "末影珍珠",
    [STR_ITEM_BUCKET] = "铁桶",
    [STR_ITEM_WATER_BUCKET] = "水桶",
    [STR_ITEM_LAVA_BUCKET] = "岩浆桶",
    [STR_BLOCK_LAVA] = "岩浆",
    [STR_BLOCK_OBSIDIAN] = "黑曜石",
    [STR_BLOCK_ENCHANTING_TABLE] = "附魔台",
    // Redstone blocks
    [STR_BLOCK_LEVER] = "拉杆",
    [STR_BLOCK_REDSTONE_WIRE] = "红石线",
    [STR_BLOCK_REDSTONE_LAMP] = "红石灯",
    [STR_BLOCK_STONE_PRESSURE_PLATE] = "石质压力板",

    // Recipe names
    [STR_RECIPE_WOOD_PLANKS] = "木头 -> 4木板",   // 木头 -> 4木板
    [STR_RECIPE_PLANKS_STICKS] = "2木板 -> 4棒子", // 2木板 -> 4棍子
    [STR_RECIPE_WOOD_PICK] = "3木板 -> 木镐",
    [STR_RECIPE_WOOD_AXE] = "3木板 -> 木斧",
    [STR_RECIPE_WOOD_SWORD] = "2木板 -> 木剑",
    [STR_RECIPE_WOOD_SHOVEL] = "2木板 -> 木铲",
    [STR_RECIPE_STONE_PICK] = "3圆石 -> 石镐",
    [STR_RECIPE_STONE_AXE] = "3圆石 -> 石斧",
    [STR_RECIPE_STONE_SWORD] = "2圆石 -> 石剑",
    [STR_RECIPE_STONE_SHOVEL] = "2圆石 -> 石铲",
    [STR_RECIPE_IRON_PICK] = "3铁锭 -> 铁镐",
    [STR_RECIPE_IRON_AXE] = "3铁锭 -> 铁斧",
    [STR_RECIPE_IRON_SWORD] = "2铁锭 -> 铁剑",
    [STR_RECIPE_IRON_SHOVEL] = "2铁锭 -> 铁铲",
    [STR_RECIPE_BRICK] = "4粘土 -> 4砖块",
    [STR_RECIPE_FURNACE] = "8圆石 -> 熔炉",
    [STR_RECIPE_TORCHES] = "2棒子 -> 4火把",
    [STR_RECIPE_SANDSTONE] = "4沙子 -> 砂岩",
    [STR_RECIPE_COBBLE] = "4砂砾 -> 4圆石",
    [STR_RECIPE_BED] = "6木板 -> 床",
    [STR_RECIPE_CRAFTING_TABLE] = "8木板 -> 合成台",
    [STR_RECIPE_APPLE] = "8树叶 -> 苹果",
    [STR_RECIPE_WOOD_HELMET] = "5木板 -> 木头盔",
    [STR_RECIPE_WOOD_CHEST] = "8木板 -> 木胸甲",
    [STR_RECIPE_WOOD_LEGS] = "7木板 -> 木护腿",
    [STR_RECIPE_WOOD_BOOTS] = "4木板 -> 木靴子",
    [STR_RECIPE_STONE_HELMET] = "5圆石 -> 石头盔",
    [STR_RECIPE_STONE_CHEST] = "8圆石 -> 石胸甲",
    [STR_RECIPE_STONE_LEGS] = "7圆石 -> 石护腿",
    [STR_RECIPE_STONE_BOOTS] = "4圆石 -> 石靴子",
    [STR_RECIPE_IRON_HELMET] = "5铁锭 -> 铁头盔",
    [STR_RECIPE_IRON_CHEST] = "8铁锭 -> 铁胸甲",
    [STR_RECIPE_IRON_LEGS] = "7铁锭 -> 铁护腿",
    [STR_RECIPE_IRON_BOOTS] = "4铁锭 -> 铁靴子",
    [STR_RECIPE_GOLD_PICK] = "3金锭 -> 金镐",
    [STR_RECIPE_GOLD_AXE] = "3金锭 -> 金斧",
    [STR_RECIPE_GOLD_SWORD] = "2金锭 -> 金剑",
    [STR_RECIPE_GOLD_SHOVEL] = "2金锭 -> 金铲",
    [STR_RECIPE_GOLD_HELMET] = "5金锭 -> 金头盔",
    [STR_RECIPE_GOLD_CHEST] = "8金锭 -> 金胸甲",
    [STR_RECIPE_GOLD_LEGS] = "7金锭 -> 金护腿",
    [STR_RECIPE_GOLD_BOOTS] = "4金锭 -> 金靴子",
    [STR_RECIPE_DIAMOND_PICK] = "3钻石 -> 钻石镐",
    [STR_RECIPE_DIAMOND_AXE] = "3钻石 -> 钻石斧",
    [STR_RECIPE_DIAMOND_SWORD] = "2钻石 -> 钻石剑",
    [STR_RECIPE_DIAMOND_SHOVEL] = "2钻石 -> 钻石铲",
    [STR_RECIPE_DIAMOND_HELMET] = "5钻石 -> 钻石头盔",
    [STR_RECIPE_DIAMOND_CHEST] = "8钻石 -> 钻石胸甲",
    [STR_RECIPE_DIAMOND_LEGS] = "7钻石 -> 钻石护腿",
    [STR_RECIPE_DIAMOND_BOOTS] = "4钻石 -> 钻石靴子",
    [STR_RECIPE_CHEST] = "8木板 -> 箱子",
    [STR_RECIPE_ARROW] = "木棍 -> 4箭",
    [STR_RECIPE_BOW] = "3木棍 + 3线 -> 弓",
    [STR_RECIPE_BONE_BLOCK] = "9骨头 -> 骨块",
    [STR_RECIPE_BONE_BLOCK_DECOMP] = "骨块 -> 9骨头",
    [STR_RECIPE_BOOKSHELF] = "6木板 -> 书架",
    [STR_RECIPE_LANTERN] = "4火把 -> 灯笼",
    [STR_RECIPE_BUCKET] = "3铁锭 -> 铁桶",
    [STR_RECIPE_ENCHANTING_TABLE] = "4黑曜石+2钻石 -> 附魔台",
    // Redstone recipes
    [STR_RECIPE_REDSTONE_WIRE] = "红石 -> 红石线",
    [STR_RECIPE_LEVER] = "圆石 -> 拉杆",
    [STR_RECIPE_REDSTONE_LAMP] = "4红石 -> 红石灯",
    [STR_RECIPE_STONE_BUTTON] = "石制按钮",
    [STR_RECIPE_REDSTONE_REPEATER] = "红石中继器",
    [STR_RECIPE_PISTON] = "活塞",
    [STR_RECIPE_IRON_DOOR] = "铁门",
    [STR_RECIPE_GLASS_BOTTLE] = "玻璃瓶",
    [STR_RECIPE_POTION_SPEED] = "速度药水",
    [STR_RECIPE_POTION_STRENGTH] = "力量药水",
    [STR_RECIPE_POTION_REGEN] = "再生药水",
    [STR_RECIPE_POTION_FIRE_RESISTANCE] = "防火药水",
    [STR_RECIPE_POTION_WATER_BREATHING] = "水下呼吸药水",
    [STR_RECIPE_POTION_POISON] = "剧毒药水",
    [STR_RECIPE_PRESSURE_PLATE] = "2石头 -> 压力板",
    [STR_SMELT_IRON] = "铁矿 -> 铁锭",            // 铁矿 -> 铁锭
    [STR_SMELT_PORK] = "生猪肉 -> 烤猪肉", // 生猪肉 -> 烤猪肉
    [STR_SMELT_COBBLE] = "圆石 -> 石头",          // 圆石 -> 石头
    [STR_SMELT_SAND] = "沙子 -> 玻璃",            // 沙子 -> 玻璃
    [STR_SMELT_GOLD] = "金矿 -> 金锭",
    [STR_SMELT_REDSTONE] = "红石矿 -> 红石",
    [STR_SMELT_LAPIS] = "青金石矿 -> 青金石",
    [STR_SMELT_CLAY] = "粘土 -> 砖块",
    [STR_SMELT_ICE] = "冰 -> 水",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "没有燃料",
    [STR_MSG_SMELTING] = "冶炼中: %s",
    [STR_MSG_CANNOT_SMELT] = "无法冶炼此物品",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "切换全屏",

    // Multiplayer
    [STR_BTN_HOST_GAME] = "创建主机",
    [STR_BTN_JOIN_GAME] = "加入游戏",
    [STR_HOST_WAITING] = "等待玩家加入...",
    [STR_HOST_IP_HINT] = "你的IP: %s:%d",
    [STR_JOIN_TITLE] = "加入游戏",
    [STR_JOIN_IP_HINT] = "输入主机IP地址:",
    [STR_JOIN_CONNECTING] = "连接中...",
    [STR_HOST_PLAYERS_COUNT] = "玩家: %d / %d",
    [STR_HOST_CANCEL_HINT] = "ESC: 取消联机",
    [STR_HOST_START_HINT] = "ENTER / SPACE：开始游戏",
    [STR_JOIN_HELP_HINT] = "ESC: 返回     回车: 连接",
    [STR_JOIN_TAB_HINT] = "Tab: 切换输入框",
    [STR_OPEN_TO_LAN] = "开放局域网",
    [STR_LAN_OPENED] = "已开放局域网 - 其他玩家可加入 %s:%d",
    [STR_LAN_STATUS] = "局域网已开放 - %s:%d  (%d/%d)",
    [STR_LAN_FAILED] = "开放局域网失败",

    // Enchantment names
    [STR_ENCH_SHARPNESS] = "锋利",
    [STR_ENCH_EFFICIENCY] = "效率",
    [STR_ENCH_PROTECTION] = "保护",
    [STR_ENCH_FORTUNE] = "时运",
    [STR_ENCH_UNBREAKING] = "耐久",
    [STR_ENCH_SILK_TOUCH] = "精准采集",
    [STR_ENCH_POWER] = "力量",
    [STR_ENCH_KNOCKBACK] = "击退",
    [STR_ENCH_FIRE_ASPECT] = "火焰附加",
    // Fishing items
    [STR_ITEM_FISHING_ROD] = "钓鱼竿",
    [STR_ITEM_RAW_FISH] = "生鱼",
    [STR_ITEM_COOKED_FISH] = "烤鱼",
    // Cauldron
    [STR_BLOCK_CAULDRON] = "炼药釜",
    [STR_ENCHANTED] = "已附魔",

    // Enchanting messages
    [STR_MSG_ENCHANTED] = "附魔成功！",
    [STR_MSG_ALREADY_ENCHANTED] = "已经附魔过了",

    // Tutorial and multiplayer
    [STR_TUTORIAL_CONTROLS] = "WASD: 移动 | 空格: 跳跃 | 左键: 挖掘 | 右键: 放置 | E: 背包",
    [STR_NET_PLAYER_JOINED] = "玩家加入了！",
    [STR_NET_PLAYER_LEFT] = "玩家断开连接",
    [STR_NET_HOST_DISCONNECTED] = "主机断开连接",

    // Missing messages
    [STR_MSG_NO_ARROWS] = "没有箭！",
    [STR_DEATH_LAVA] = " 在岩浆中游泳了",
    [STR_DEATH_CACTUS] = " 被仙人掌扎死了",

    // Ender pearl
    [STR_MSG_ENDER_PEARL] = "已传送！",
    [STR_FISH_CAST] = "抛竿！",
    [STR_FISH_BITE] = "鱼上钩了！",
    [STR_FISH_CATCH] = "钓到东西了！",
    [STR_FISH_RETRACT] = "收线",
    [STR_MSG_CAULDRON_FILLED] = "炼药锅已装满！",
    [STR_MSG_CAULDRON_EMPTY] = "炼药锅已倒空！",
    [STR_MSG_CAULDRON_DRINK] = "从炼药锅喝了水",
    [STR_BLOCK_OAK_STAIRS] = "橡木楼梯",
    [STR_BLOCK_COBBLESTONE_STAIRS] = "圆石楼梯",
    [STR_BLOCK_STONE_BRICKS] = "石砖",
    [STR_BLOCK_CHISELED_STONE_BRICKS] = "錾制石砖",
    [STR_BLOCK_OAK_SLAB] = "橡木台阶",
    [STR_BLOCK_COBBLESTONE_SLAB] = "圆石台阶",
    [STR_RECIPE_OAK_STAIRS] = "4 木板 -> 4 橡木楼梯",
    [STR_RECIPE_COBBLESTONE_STAIRS] = "4 圆石 -> 4 圆石楼梯",
    [STR_RECIPE_STONE_BRICKS] = "4 石头 -> 4 石砖",
    [STR_RECIPE_CHISELED_STONE_BRICKS] = "2 石砖台阶 -> 錾制石砖",
    [STR_RECIPE_OAK_SLAB] = "3 木板 -> 6 橡木台阶",
    [STR_RECIPE_COBBLESTONE_SLAB] = "3 圆石 -> 6 圆石台阶",
    [STR_BLOCK_CACTUS] = "仙人掌",
    [STR_BLOCK_SUGAR_CANE] = "甘蔗",
    [STR_ITEM_PAPER] = "纸",
    [STR_ITEM_BOOK] = "书",
    [STR_ITEM_SUGAR] = "糖",
    [STR_BLOCK_TNT] = "TNT炸药",
    [STR_BLOCK_RED_SAND] = "红沙",
    [STR_BLOCK_MYCELIUM] = "菌丝土",
    [STR_BLOCK_MUSHROOM_BLOCK] = "蘑菇方块",
    [STR_BLOCK_MUSHROOM_STEM] = "蘑菇柄",
    [STR_BLOCK_STONE_BUTTON] = "石制按钮",
    [STR_BLOCK_REDSTONE_REPEATER] = "红石中继器",
    [STR_BLOCK_PISTON] = "活塞",
    [STR_BLOCK_IRON_DOOR] = "铁门",
    [STR_ITEM_GLASS_BOTTLE] = "玻璃瓶",
    [STR_ITEM_POTION_WATER] = "水瓶",
    [STR_ITEM_POTION_SPEED] = "速度药水",
    [STR_ITEM_POTION_STRENGTH] = "力量药水",
    [STR_ITEM_POTION_REGEN] = "再生药水",
    [STR_ITEM_POTION_FIRE_RESISTANCE] = "防火药水",
    [STR_ITEM_POTION_WATER_BREATHING] = "水下呼吸药水",
    [STR_ITEM_POTION_POISON] = "剧毒药水",
    [STR_MSG_DRANK_POTION] = "你感到药效发作！",
    [STR_MSG_BOTTLE_FILLED] = "瓶子装满了水",
    [STR_MSG_IRON_DOOR_HINT] = "铁门需要红石信号才能打开",
    [STR_RECIPE_TNT] = "TNT炸药",
    [STR_RECIPE_PAPER] = "3 甘蔗 -> 3 纸",
    [STR_RECIPE_BOOK] = "2 纸 + 1 皮革 -> 1 书",
    [STR_RECIPE_SUGAR] = "1 甘蔗 -> 1 糖",
    [STR_MSG_CACTUS_DAMAGE] = "哎哟！仙人掌扎手！",
    [STR_TYPE_TOOL] = "工具",
    [STR_TYPE_ARMOR] = "盔甲",
    [STR_TYPE_FOOD] = "食物",
    [STR_TYPE_BLOCK] = "方块",
    [STR_RECIPE_SLIMEBALL_STRING] = "粘液球纤维",

    // Farming
    [STR_TOOL_WOOD_HOE] = "木锄",
    [STR_TOOL_STONE_HOE] = "石锄",
    [STR_TOOL_IRON_HOE] = "铁锄",
    [STR_TOOL_GOLD_HOE] = "金锄",
    [STR_TOOL_DIAMOND_HOE] = "钻石锄",
    [STR_ITEM_WHEAT_SEEDS] = "小麦种子",
    [STR_ITEM_WHEAT] = "小麦",
    [STR_BLOCK_FARMLAND] = "耕地",
    [STR_BLOCK_CROPS] = "农作物",
    [STR_BLOCK_HAY_BALE] = "干草块",
    [STR_RECIPE_WOOD_HOE] = "木锄",
    [STR_RECIPE_STONE_HOE] = "石锄",
    [STR_RECIPE_IRON_HOE] = "铁锄",
    [STR_RECIPE_GOLD_HOE] = "金锄",
    [STR_RECIPE_DIAMOND_HOE] = "钻石锄",
    [STR_RECIPE_BREAD] = "面包",
    [STR_RECIPE_HAY_BALE] = "干草块",
    [STR_RECIPE_FISHING_ROD] = "3根木棒+2根线 -> 钓鱼竿",
    [STR_RECIPE_MOSSY_COBBLESTONE] = "圆石 -> 苔石砖",
    [STR_RECIPE_COARSE_DIRT] = "2泥土 -> 2粗泥",

    // New mob deaths
    [STR_DEATH_MOB_COW] = "被牛踢死了",
    [STR_DEATH_MOB_SHEEP] = "被羊撞死了",
    [STR_DEATH_MOB_CHICKEN] = "被鸡啄死了",
    [STR_DEATH_MOB_WOLF] = "被狼咬死了",
    [STR_DEATH_MOB_WITCH] = "被女巫杀害了",
    [STR_DEATH_MOB_BAT] = "被蝙蝠吓死了",

    // Mob names
    [STR_MOB_PIG] = "猪",
    [STR_MOB_COW] = "牛",
    [STR_MOB_SHEEP] = "羊",
    [STR_MOB_CHICKEN] = "鸡",
    [STR_MOB_VILLAGER] = "村民",
    [STR_MOB_HORSE] = "马",
    [STR_MOB_WOLF] = "狼",
    [STR_MOB_WITCH] = "女巫",
    [STR_MOB_BAT] = "蝙蝠",

    // Animal drops
    [STR_ITEM_RAW_BEEF] = "生牛肉",
    [STR_ITEM_LEATHER] = "皮革",
    [STR_ITEM_RAW_MUTTON] = "生羊肉",
    [STR_ITEM_WOOL] = "羊毛",
    [STR_ITEM_RAW_CHICKEN] = "生鸡肉",
    [STR_ITEM_FEATHER] = "羽毛",
    [STR_ITEM_EGG] = "鸡蛋",
    [STR_ITEM_COOKED_BEEF] = "熟牛肉",
    [STR_ITEM_COOKED_MUTTON] = "熟羊肉",
    [STR_ITEM_COOKED_CHICKEN] = "熟鸡肉",

    // Smelt recipes
    [STR_SMELT_BEEF] = "烤牛肉",
    [STR_SMELT_MUTTON] = "烤羊肉",
    [STR_SMELT_CHICKEN] = "烤鸡肉",
    [STR_SMELT_FISH] = "烤鱼",
    [STR_MSG_CANT_SLEEP_MOBS] = "附近有怪物！",
    // Creative mode
    [STR_GAMEMODE] = "模式",
    [STR_MODE_SURVIVAL] = "生存",
    [STR_MODE_CREATIVE] = "创造",
    [STR_MSG_FLIGHT_ON] = "已开启飞行",
    [STR_MSG_FLIGHT_OFF] = "已关闭飞行",
    [STR_CREATIVE_TITLE] = "创造物品栏",
    [STR_CREATIVE_SELECTED] = "已选择：%s",
    [STR_CREATIVE_SELECT_HINT] = "点击物品获取（无限）",
    [STR_CREATIVE_SEARCH] = "搜索：",
    [STR_CREATIVE_NO_MATCH] = "无匹配项",
    [STR_MODE_SET] = "游戏模式：%s",
    [STR_CREATIVE_BACKPACK] = "背包",
    [STR_HINT_SCROLL] = "滚动浏览",
    [STR_MSG_HOST_ONLY] = "只有主机可以更改游戏模式",
    // Easter eggs
    [STR_EGG_KONAMI_ON] = "上上下下左右左右BA！派对模式开启",
    [STR_EGG_KONAMI_OFF] = "派对模式关闭",
    [STR_EGG_TITLE_CLICK] = "嘿，别一直戳标题！",
    [STR_EGG_FACT1] = "冷知识：基岩不可破坏是有原因的……",
    [STR_EGG_FACT2] = "冷知识：虚空是单程票。",
    [STR_EGG_FACT3] = "冷知识：羊更喜欢白色。",
},

// ===== JAPANESE =====
[LANG_JA] = {
    [STR_NONE] = "",

    [STR_TITLE] = "MyWorld",
    [STR_SUBTITLE] = "2Dサンドボックスアドベンチャー",  // 2Dサンドボックスアドベンチャー
    [STR_BTN_NEW_GAME] = "新規ワールド",   // 新規ワールド
    [STR_BTN_LOAD_GAME] = "ロード",                     // ロード
    [STR_BTN_SETTINGS] = "設定",                            // 設定
    [STR_BTN_QUIT] = "終了",                                // 終了
    [STR_HINT_NAVIGATE] = "矢印キー/WASD: 移動  |  Enter/スペース: 決定",
    [STR_HINT_CONTROLS] = "WASD: 移動  |  スペース: ジャンプ  |  E: インベントリ  |  ESC: 一時停止",

    [STR_NEW_GAME_TITLE] = "新規ワールド",
    [STR_LOAD_GAME_TITLE] = "ロード",
    [STR_NEW_GAME_SUB] = "スロットを選んで新しい世界を作成",
    [STR_LOAD_GAME_SUB] = "セーブデータを選んで続ける",
    [STR_SEED] = "シード:",                             // シード:
    [STR_RANDOM] = "ランダム",                     // ランダム
    [STR_GENERATING_WORLD] = "世界を生成中...",                  // 世界を生成中
    [STR_SLOT] = "スロット %d",                    // スロット %d
    [STR_SEED_DISPLAY] = "シード: %u",
    [STR_OCCUPIED] = "使用中",                         // 使用中
    [STR_EMPTY_NEW] = "空 - クリックで作成",  // 空 - クリックで作成
    [STR_EMPTY_LOAD] = "セーブデータなし",    // セーブデータなし
    [STR_BACK] = "戻る",                                    // 戻る
    [STR_HINT_SLOT] = "矢印: 移動  |  Enter: 選択  |  DEL: 削除  |  ESC: 戻る",

    [STR_DELETE_SAVE] = "セーブを削除?",   // セーブを削除?
    [STR_OVERWRITE_SAVE] = "上書き?",                  // 上書き?
    [STR_SLOT_HAS_DATA] = "スロット %d にデータがあります。",
    [STR_CANNOT_UNDO] = "これは元に戻せません!",  // これは元に戻せません!
    [STR_YES_DELETE] = "はい、削除",            // はい、削除
    [STR_YES_OVERWRITE] = "はい、上書き",  // はい、上書き
    [STR_CANCEL] = "キャンセル",               // キャンセル
    [STR_CONFIRM_KEYS] = "[Y] 確認    [N / ESC] キャンセル",

    [STR_PAUSED] = "一時停止",                     // 一時停止
    [STR_MUSIC_VOLUME] = "音楽ボリューム",  // 音楽ボリューム
    [STR_SFX_VOLUME] = "効果音ボリューム",  // 効果音ボリューム
    [STR_CONTROLS_TITLE] = "--- 操作 ---",                  // --- 操作 ---
    [STR_CONTINUE] = "続ける",                          // 続ける
    [STR_MAIN_MENU] = "メインメニュー",  // メインメニュー

    [STR_KEY_WASD] = "WASD",
    [STR_KEY_SPACE] = "スペース",                  // スペース
    [STR_KEY_SHIFT] = "Shift",
    [STR_KEY_CTRL] = "Ctrl",
    [STR_KEY_LCLICK] = "左クリック",          // 左クリック
    [STR_KEY_RCLICK] = "右クリック",          // 右クリック
    [STR_KEY_E] = "E",
    [STR_KEY_H] = "H",
    [STR_KEY_F3] = "F3",
    [STR_KEY_ESC] = "ESC",
    [STR_KEY_19] = "1-9",

    [STR_ACT_MOVE] = "移動",                               // 移動
    [STR_ACT_JUMP] = "ジャンプ/泳ぎ",     // ジャンプ/泳ぎ
    [STR_ACT_SPRINT] = "スプリント",
    [STR_ACT_SNEAK] = "しゃがむ",          // スプリント
    [STR_ACT_BREAK] = "破壊/攻撃",                 // 破壊/攻撃
    [STR_ACT_PLACE] = "置く/食べる",          // 置く/食べる
    [STR_ACT_INVENTORY] = "インベントリ",  // インベントリ
    [STR_ACT_HEAL] = "治癒(XP)",                           // 治療(XP)
    [STR_ACT_DEBUG] = "デバッグ",                  // デバッグ
    [STR_ACT_PAUSE] = "一時停止",                  // 一時停止
    [STR_ACT_HOTBAR] = "ホットバー",          // ホットバー

    [STR_YOU_DIED] = "死んだ!",                        // 死んだ!
    [STR_PRESS_SPACE_RESPAWN] = "スペースキーで復活",
    [STR_PRESS_ESC_MENU] = "ESCでメインメニューに戻る",
    [STR_DEATH_FALL] = "高い所から落ちた",
    [STR_DEATH_DROWN] = "溺れた",
    [STR_DEATH_STARVE] = "飢え死にした",
    [STR_DEATH_MOB_ZOMBIE] = "ゾンビに殺された",
    [STR_DEATH_MOB_SKELETON] = "スケルトンに射殺された",
    [STR_DEATH_MOB_CREEPER] = "クリーパーに爆破された",
    [STR_DEATH_MOB_SPIDER] = "スパイダーに殺された",
    [STR_DEATH_MOB_SLIME] = "スライムに殺された",
    [STR_DEATH_MOB_ENDERMAN] = "エンダーマンに殺された",
    [STR_DEATH_VOID] = "世界から落ちた",
    [STR_DEATH_SCORE] = "スコア: %d XP",

    [STR_INVENTORY] = "インベントリ",     // インベントリ
    [STR_SORT] = "並べ替え",                       // 並べ替え

    [STR_WORLD_MAP] = "世界地図",                  // 世界地図
    [STR_PRESS_M_CLOSE] = "Mで閉じる",            // Mで閉じる
    [STR_PLAYER_COORD] = "プレイヤー: %d, %d", // プレイヤー: %d, %d

    [STR_CRAFTING] = "クラフティング", // クラフティング
    [STR_CRAFTING_TABLE] = "クラフティングテーブル",
    [STR_SEARCH] = "検索...",                               // 検索...
    [STR_NO_MATCHES] = "一致なし",                 // 一致なし
    [STR_NO_RECIPES] = "レシピなし",          // レシピなし

    [STR_FURNACE] = "炉",                                      // 炉
    [STR_FUEL] = "燃料",                                   // 燃料
    [STR_INPUT] = "入力",                                  // 入力
    [STR_OUTPUT] = "出力",                                 // 出力
    [STR_PRESS_E_ESC_CLOSE] = "EまたはESCで閉じる",

    [STR_SETTINGS] = "設定",                               // 設定
    [STR_SECTION_AUDIO] = "--- オーディオ ---",  // --- オーディオ ---
    [STR_SOUND_EFFECTS] = "効果音",                    // 効果音
    [STR_SECTION_DISPLAY] = "--- 表示 ---",                // --- 表示 ---
    [STR_WINDOW_MODE] = "ウィンドウモード",  // ウィンドウモード
    [STR_WINDOWED] = "ウィンドウ",           // ウィンドウ
    [STR_FULLSCREEN] = "フルスクリーン",  // フルスクリーン
    [STR_BORDERLESS] = "ボーダレス",         // ボーダレス
    [STR_RESOLUTION] = "解像度",
    [STR_RES_960] = "960 × 540",
    [STR_RES_1280] = "1280 × 720",
    [STR_RES_1600] = "1600 × 900",
    [STR_SECTION_LANGUAGE] = "--- 言語 ---",               // --- 言語 ---
    [STR_LANGUAGE] = "言語",                               // 言語
    [STR_SECTION_FONT] = "--- フォント ---",     // --- フォント ---
    [STR_FONT] = "フォント",                      // フォント
    [STR_FONT_BUILTIN] = "組み込み",              // 組み込み
    [STR_FONT_CUSTOM] = "カスタムパス...",  // カスタムパス...

    // Language names
    [STR_LANG_NAME_EN] = "English",
    [STR_LANG_NAME_ZH] = "简体中文",
    [STR_LANG_NAME_JA] = "日本語",

    // Font names
    [STR_FONT_NAME_BUILTIN] = "標準",
    [STR_FONT_NAME_LXGW] = "LXGW WenKai",

    // Difficulty
    [STR_DIFFICULTY] = "難易度",
    [STR_DIFFICULTY_PEACEFUL] = "ピースフル",
    [STR_DIFFICULTY_EASY] = "イージー",
    [STR_DIFFICULTY_NORMAL] = "ノーマル",
    [STR_DIFFICULTY_HARD] = "ハード",

    // Achievements
    [STR_ACH_FIRST_STEPS] = "第一歩 - 木のピッケルをクラフト",
    [STR_ACH_DEEP_DIG] = "深掘り - 岩盤層に到達",
    [STR_ACH_MONSTER_HUNTER] = "モンスターハンター - 100体討伐",
    [STR_ACH_ARCHITECT] = "建築家 - 1000ブロック設置",
    [STR_ACH_REDSTONE_ENGINEER] = "レッドストーン技師 - ランプ点灯",
    [STR_ACH_COLLECTOR] = "コレクター - 20種類のアイテム収集",
    [STR_ACH_ANGLER] = "釣り人 - 魚を釣る",
    [STR_ACH_BREEDER] = "繁殖家 - 赤ちゃん動物を育てる",
    [STR_ACH_ENCHANTER] = "エンチャンター - アイテムにエンチャント",
    [STR_ACH_DEMOLITION] = "爆破屋 - TNTを爆発させる",
    [STR_COLLECTION_TITLE] = "コレクション",
    [STR_COLLECTION_CHALLENGES] = "電子チャレンジ",
    [STR_COLLECTION_UNLOCKED] = "解除済み",
    [STR_COLLECTION_LOCKED] = "未解除",
    [STR_COLLECTION_CLOSE] = "I / ESC：閉じる",

    [STR_MSG_GAME_SAVED] = "セーブしました",  // セーブしました
    [STR_MSG_HEALED_XP] = "XPで治癒しました!",  // XPで治療しました!
    [STR_MSG_NOT_ENOUGH_XP] = "XPが不足!",            // XPが不足!
    [STR_MSG_INVENTORY_FULL] = "インベントリが満です!",  // インベントリが満です!
    [STR_MSG_TOOL_BROKE] = "道具が壊れた!",  // 道具が壊れた!
    [STR_MSG_TOOL_WEARING] = "道具がほぼ壊れ!",  // 道具がほぼ壊れ!
    [STR_MSG_FALL_DAMAGE] = "いったっ! 落下ダメージ!",  // いたっ! 落下ダメージ!
    [STR_MSG_SPAWN_SET] = "スポーンポイントを設定!",  // スポーンポイントを設定!
    [STR_MSG_SLEEP] = "おはよう！",
    [STR_MSG_SLEEP_ONLY_NIGHT] = "夜しか寝られない",
    [STR_MSG_HUNGRY] = "腹が空いた!",        // 腹が空いた!
    [STR_MSG_STARVING] = "餓死!",                          // 餓死!
    [STR_MSG_ATE] = "%sを食べた (+%d満腹度)",  // %sを食べた (+%d満腹度)
    [STR_MSG_TRADE] = "%sと%sを交換した",
    [STR_MSG_NOT_ENOUGH_ITEMS] = "アイテムが足りない！",
    [STR_MSG_BROKE] = "%sが壊れた!",              // %sが壊れた!
    [STR_MSG_CRIT_HIT] = "会心の一撃!",

    [STR_TOOLTIP_ARMOR] = "+%dアーマー  耐久: %d%%",  // +%dアーマー  耐久: %d%%
    [STR_TOOLTIP_HUNGER] = "+%d満腹度",               // +%d満腹度
    [STR_TOOLTIP_DURABILITY] = "耐久力: %d%%",        // 耐久力: %d%%
    [STR_TOOLTIP_DUR_SHORT] = "耐久: %d%%",               // 耐久: %d%%

    [STR_DBG_FPS] = "FPS: %d",
    [STR_DBG_POS] = "座標: %.1f, %.1f",                  // 座標: %.1f, %.1f
    [STR_DBG_BLOCK] = "ブロック: %d, %d",        // ブロック: %d, %d
    [STR_DBG_CHUNKS] = "チャンク: %d",           // チャンク: %d
    [STR_DBG_TIME] = "時間: %.2f",                        // 時間: %.2f
    [STR_DBG_LIGHT] = "光: %.2f",                             // 光: %.2f
    [STR_DBG_GROUND] = "地面: %s",                        // 地面: %s
    [STR_DBG_HP] = "HP: %d/%d  満腹: %d/%d",              // HP: %d/%d  満腹: %d/%d
    [STR_DBG_OXYGEN] = "酸素: %d/%d  XP: %d/%d",         // 酸素: %d/%d  XP: %d/%d
    [STR_DBG_UNDERWATER] = "水中: %s",                    // 水中: %s
    [STR_DBG_SEED] = "シード: %u",                    // シード: %u
    [STR_DBG_WEATHER] = "天気: %s",
    [STR_DBG_MODE] = "モード: %s",
    [STR_WEATHER_CLEAR] = "晴れ",
    [STR_WEATHER_RAIN] = "雨",
    [STR_WEATHER_THUNDER] = "雷雨",
    [STR_YES] = "はい",                                    // はい
    [STR_NO] = "いいえ",                               // いいえ

    // Block names (Japanese)
    [STR_BLOCK_AIR] = "空気",                              // 空気
    [STR_BLOCK_GRASS] = "草ブロック",         // 草ブロック
    [STR_BLOCK_DIRT] = "土",                                   // 土
    [STR_BLOCK_STONE] = "石",                                  // 石
    [STR_BLOCK_COBBLESTONE] = "コブルストーン",  // コブルストーン
    [STR_BLOCK_WOOD] = "材",                                   // 材
    [STR_BLOCK_LEAVES] = "葉",                                 // 葉
    [STR_BLOCK_SAND] = "砂",                                   // 砂
    [STR_BLOCK_WATER] = "水",                                  // 水
    [STR_BLOCK_COAL_ORE] = "炭鉱石",                   // 炭鉱石
    [STR_BLOCK_IRON_ORE] = "鉄鉱石",                   // 鉄鉱石
    [STR_BLOCK_PLANKS] = "板",                                 // 板
    [STR_BLOCK_BRICK] = "レンガ",                     // レンガ
    [STR_BLOCK_GLASS] = "ガラス",                     // ガラス
    [STR_BLOCK_BEDROCK] = "基岩",                          // 基岩
    [STR_BLOCK_GRAVEL] = "グラベル",              // グラベル
    [STR_BLOCK_CLAY] = "粘土",                             // 粘土
    [STR_BLOCK_SANDSTONE] = "砂岩",                        // 砂岩
    [STR_BLOCK_TORCH] = "松明",                            // 松明
    [STR_BLOCK_FLOWER] = "花",                                 // 花
    [STR_BLOCK_TALL_GRASS] = "高い草",                 // 高い草
    [STR_BLOCK_FURNACE] = "炉",                                // 炉
    [STR_BLOCK_BED] = "ベッド",                        // ベッド
    [STR_ITEM_STICK] = "棒",                                   // 棒
    [STR_ITEM_COAL] = "炭",                                    // 炭
    [STR_ITEM_IRON_INGOT] = "鉄の地金",           // 鉄の地金
    [STR_TOOL_WOOD_PICKAXE] = "木のつるはし",  // 木のつるはし
    [STR_TOOL_WOOD_AXE] = "木のオノ",             // 木のオノ
    [STR_TOOL_WOOD_SWORD] = "木の剣",                 // 木の剣
    [STR_TOOL_WOOD_SHOVEL] = "木のシャベル",   // 木のシャベル
    [STR_TOOL_STONE_PICKAXE] = "石のつるはし",  // 石のつるはし
    [STR_TOOL_STONE_AXE] = "石のオノ",            // 石のオノ
    [STR_TOOL_STONE_SWORD] = "石の剣",                // 石の剣
    [STR_TOOL_STONE_SHOVEL] = "石のシャベル",  // 石のシャベル
    [STR_TOOL_IRON_PICKAXE] = "鉄のつるはし",  // 鉄のつるはし
    [STR_TOOL_IRON_AXE] = "鉄のオノ",             // 鉄のオノ
    [STR_TOOL_IRON_SWORD] = "鉄の剣",                 // 鉄の剣
    [STR_TOOL_IRON_SHOVEL] = "鉄のシャベル",   // 鉄のシャベル
    [STR_FOOD_RAW_PORK] = "生ポーク",             // 生ポーク
    [STR_FOOD_COOKED_PORK] = "焼いたポーク",  // 焼いたポーク
    [STR_FOOD_APPLE] = "リンゴ",                      // リンゴ
    [STR_FOOD_BREAD] = "パン",                            // パン
    [STR_BLOCK_CRAFTING_TABLE] = "クラフティングテーブル",
    [STR_ARMOR_WOOD_HELMET] = "木のヘルメット",     // 木のヘルメット
    [STR_ARMOR_WOOD_CHESTPLATE] = "木のチェスト",       // 木のチェスト
    [STR_ARMOR_WOOD_LEGGINGS] = "木のレギンス",   // 木のレギンス
    [STR_ARMOR_WOOD_BOOTS] = "木のブーツ",                  // 木のブーツ
    [STR_ARMOR_STONE_HELMET] = "石のヘルメット",    // 石のヘルメット
    [STR_ARMOR_STONE_CHESTPLATE] = "石のチェスト",      // 石のチェスト
    [STR_ARMOR_STONE_LEGGINGS] = "石のレギンス",  // 石のレギンス
    [STR_ARMOR_STONE_BOOTS] = "石のブーツ",                 // 石のブーツ
    [STR_ARMOR_IRON_HELMET] = "鉄のヘルメット",     // 鉄のヘルメット
    [STR_ARMOR_IRON_CHESTPLATE] = "鉄のチェスト",       // 鉄のチェスト
    [STR_ARMOR_IRON_LEGGINGS] = "鉄のレギンス",   // 鉄のレギンス
    [STR_ARMOR_IRON_BOOTS] = "鉄のブーツ",                  // 鉄のブーツ
    [STR_BLOCK_GOLD_ORE] = "金鉱石",
    [STR_BLOCK_DIAMOND_ORE] = "ダイヤモンド鉱石",
    [STR_BLOCK_REDSTONE_ORE] = "レッドストーン鉱石",
    [STR_BLOCK_LAPIS_ORE] = "ラピスラズリ鉱石",
    [STR_ITEM_GOLD_INGOT] = "金の地金",
    [STR_ITEM_DIAMOND] = "ダイヤモンド",
    [STR_ITEM_REDSTONE] = "レッドストーン",
    [STR_ITEM_LAPIS] = "ラピスラズリ",
    [STR_BLOCK_CHEST] = "チェスト",
    [STR_TOOL_GOLD_PICKAXE] = "金のつるはし",
    [STR_TOOL_GOLD_AXE] = "金のオノ",
    [STR_TOOL_GOLD_SWORD] = "金の剣",
    [STR_TOOL_GOLD_SHOVEL] = "金のシャベル",
    [STR_TOOL_DIAMOND_PICKAXE] = "ダイヤのつるはし",
    [STR_TOOL_DIAMOND_AXE] = "ダイヤのオノ",
    [STR_TOOL_DIAMOND_SWORD] = "ダイヤの剣",
    [STR_TOOL_DIAMOND_SHOVEL] = "ダイヤのシャベル",
    [STR_ARMOR_GOLD_HELMET] = "金のヘルメット",
    [STR_ARMOR_GOLD_CHESTPLATE] = "金のチェスト",
    [STR_ARMOR_GOLD_LEGGINGS] = "金のレギンス",
    [STR_ARMOR_GOLD_BOOTS] = "金のブーツ",
    [STR_ARMOR_DIAMOND_HELMET] = "ダイヤのヘルメット",
    [STR_ARMOR_DIAMOND_CHESTPLATE] = "ダイヤのチェスト",
    [STR_ARMOR_DIAMOND_LEGGINGS] = "ダイヤのレギンス",
    [STR_ARMOR_DIAMOND_BOOTS] = "ダイヤのブーツ",
    [STR_ITEM_GUNPOWDER] = "火薬",
    [STR_ITEM_STRING] = "糸",
    [STR_ITEM_BONE] = "骨",
    [STR_ITEM_ARROW] = "矢",
    [STR_ITEM_BOW] = "弓",
    [STR_BLOCK_MOSSY_COBBLESTONE] = "苔むした丸石",
    [STR_BLOCK_BOOKSHELF] = "本棚",
    [STR_BLOCK_LANTERN] = "ランタン",
    [STR_BLOCK_BONE_BLOCK] = "骨ブロック",
    [STR_BLOCK_SNOW] = "雪",
    [STR_BLOCK_ICE] = "氷",
    [STR_BLOCK_PACKED_ICE] = "氷塊",
    [STR_BLOCK_MUD] = "泥",
    [STR_BLOCK_MOSS_BLOCK] = "苔ブロック",
    [STR_BLOCK_JUNGLE_WOOD] = "ジャングルの木",
    [STR_BLOCK_JUNGLE_LEAVES] = "ジャングルの葉",
    [STR_BLOCK_VINE] = "ツタ",
    [STR_BLOCK_PUMPKIN] = "カボチャ",
    [STR_BLOCK_MELON] = "スイカ",
    [STR_BLOCK_SNOWY_GRASS] = "雪草ブロック",
    [STR_BLOCK_COARSE_DIRT] = "粗い土",
    [STR_BLOCK_PODZOL] = "ポドゾル",
    [STR_ITEM_SLIMEBALL] = "スライムボール",
    [STR_ITEM_ENDER_PEARL] = "エンダーパール",
    [STR_ITEM_BUCKET] = "バケツ",
    [STR_ITEM_WATER_BUCKET] = "水入りバケツ",
    [STR_ITEM_LAVA_BUCKET] = "溶岩入りバケツ",
    [STR_BLOCK_LAVA] = "溶岩",
    [STR_BLOCK_OBSIDIAN] = "黒曜石",
    [STR_BLOCK_ENCHANTING_TABLE] = "エンチャント台",
    // Redstone blocks
    [STR_BLOCK_LEVER] = "レバー",
    [STR_BLOCK_REDSTONE_WIRE] = "レッドストーン線",
    [STR_BLOCK_REDSTONE_LAMP] = "レッドストーンランプ",
    [STR_BLOCK_STONE_PRESSURE_PLATE] = "石の感圧板",

    // Recipe names (Japanese)
    [STR_RECIPE_WOOD_PLANKS] = "材 -> 板×4",         // 材 -> 板×4
    [STR_RECIPE_PLANKS_STICKS] = "板×2 -> 棒×4",  // 板×2 -> 棒×4
    [STR_RECIPE_WOOD_PICK] = "板×3 -> 木のつるはし",
    [STR_RECIPE_WOOD_AXE] = "板×3 -> 木のオノ",
    [STR_RECIPE_WOOD_SWORD] = "板×2 -> 木の剣",
    [STR_RECIPE_WOOD_SHOVEL] = "板×2 -> 木のシャベル",
    [STR_RECIPE_STONE_PICK] = "コブル×3 -> 石のつるはし",
    [STR_RECIPE_STONE_AXE] = "コブル×3 -> 石のオノ",
    [STR_RECIPE_STONE_SWORD] = "コブル×2 -> 石の剣",
    [STR_RECIPE_STONE_SHOVEL] = "コブル×2 -> 石のシャベル",
    [STR_RECIPE_IRON_PICK] = "鉄の地金×3 -> 鉄のつるはし",
    [STR_RECIPE_IRON_AXE] = "鉄の地金×3 -> 鉄のオノ",
    [STR_RECIPE_IRON_SWORD] = "鉄の地金×2 -> 鉄の剣",
    [STR_RECIPE_IRON_SHOVEL] = "鉄の地金×2 -> 鉄のシャベル",
    [STR_RECIPE_BRICK] = "粘土×4 -> レンガ×4",
    [STR_RECIPE_FURNACE] = "コブル×8 -> 炉",
    [STR_RECIPE_TORCHES] = "棒×2 -> 松明×4",
    [STR_RECIPE_SANDSTONE] = "砂×4 -> 砂岩",
    [STR_RECIPE_COBBLE] = "砂利×4 -> コブル×4",
    [STR_RECIPE_BED] = "板×6 -> ベッド",
    [STR_RECIPE_CRAFTING_TABLE] = "板×8 -> クラフティングテーブル",
    [STR_RECIPE_APPLE] = "葉×8 -> リンゴ",
    [STR_RECIPE_WOOD_HELMET] = "板×5 -> 木のヘルメット",
    [STR_RECIPE_WOOD_CHEST] = "板×8 -> 木のチェスト",
    [STR_RECIPE_WOOD_LEGS] = "板×7 -> 木のレギンス",
    [STR_RECIPE_WOOD_BOOTS] = "板×4 -> 木のブーツ",
    [STR_RECIPE_STONE_HELMET] = "コブル×5 -> 石のヘルメット",
    [STR_RECIPE_STONE_CHEST] = "コブル×8 -> 石のチェスト",
    [STR_RECIPE_STONE_LEGS] = "コブル×7 -> 石のレギンス",
    [STR_RECIPE_STONE_BOOTS] = "コブル×4 -> 石のブーツ",
    [STR_RECIPE_IRON_HELMET] = "鉄の地金×5 -> 鉄のヘルメット",
    [STR_RECIPE_IRON_CHEST] = "鉄の地金×8 -> 鉄のチェスト",
    [STR_RECIPE_IRON_LEGS] = "鉄の地金×7 -> 鉄のレギンス",
    [STR_RECIPE_IRON_BOOTS] = "鉄の地金×4 -> 鉄のブーツ",
    [STR_RECIPE_GOLD_PICK] = "金の地金×3+棒×2 -> 金のつるはし",
    [STR_RECIPE_GOLD_AXE] = "金の地金×3+棒×2 -> 金のオノ",
    [STR_RECIPE_GOLD_SWORD] = "金の地金×2+棒×1 -> 金の剣",
    [STR_RECIPE_GOLD_SHOVEL] = "金の地金×1+棒×2 -> 金のシャベル",
    [STR_RECIPE_GOLD_HELMET] = "金の地金×5 -> 金のヘルメット",
    [STR_RECIPE_GOLD_CHEST] = "金の地金×8 -> 金のチェスト",
    [STR_RECIPE_GOLD_LEGS] = "金の地金×7 -> 金のレギンス",
    [STR_RECIPE_GOLD_BOOTS] = "金の地金×4 -> 金のブーツ",
    [STR_RECIPE_DIAMOND_PICK] = "ダイヤ×3+棒×2 -> ダイヤのつるはし",
    [STR_RECIPE_DIAMOND_AXE] = "ダイヤ×3+棒×2 -> ダイヤのオノ",
    [STR_RECIPE_DIAMOND_SWORD] = "ダイヤ×2+棒×1 -> ダイヤの剣",
    [STR_RECIPE_DIAMOND_SHOVEL] = "ダイヤ×1+棒×2 -> ダイヤのシャベル",
    [STR_RECIPE_DIAMOND_HELMET] = "ダイヤ×5 -> ダイヤのヘルメット",
    [STR_RECIPE_DIAMOND_CHEST] = "ダイヤ×8 -> ダイヤのチェスト",
    [STR_RECIPE_DIAMOND_LEGS] = "ダイヤ×7 -> ダイヤのレギンス",
    [STR_RECIPE_DIAMOND_BOOTS] = "ダイヤ×4 -> ダイヤのブーツ",
    [STR_RECIPE_CHEST] = "板×8 -> チェスト",
    [STR_RECIPE_ARROW] = "棒 -> 矢×4",
    [STR_RECIPE_BOW] = "棒×3 + 糸×3 -> 弓",
    [STR_RECIPE_BONE_BLOCK] = "骨×9 -> 骨ブロック",
    [STR_RECIPE_BONE_BLOCK_DECOMP] = "骨ブロック -> 骨×9",
    [STR_RECIPE_BOOKSHELF] = "板×6 -> 本棚",
    [STR_RECIPE_LANTERN] = "松明×4 -> ランタン",
    [STR_RECIPE_BUCKET] = "鉄インゴット×3 -> バケツ",
    [STR_RECIPE_ENCHANTING_TABLE] = "黒曜石×4+ダイヤ×2 -> エンチャント台",
    // Redstone recipes
    [STR_RECIPE_REDSTONE_WIRE] = "レッドストーン -> 線",
    [STR_RECIPE_LEVER] = "丸石 -> レバー",
    [STR_RECIPE_REDSTONE_LAMP] = "レッドストーン×4 -> ランプ",
    [STR_RECIPE_STONE_BUTTON] = "石のボタン",
    [STR_RECIPE_REDSTONE_REPEATER] = "レッドストーンリピーター",
    [STR_RECIPE_PISTON] = "ピストン",
    [STR_RECIPE_IRON_DOOR] = "鉄の扉",
    [STR_RECIPE_GLASS_BOTTLE] = "ガラス瓶",
    [STR_RECIPE_POTION_SPEED] = "速度のポーション",
    [STR_RECIPE_POTION_STRENGTH] = "力のポーション",
    [STR_RECIPE_POTION_REGEN] = "再生のポーション",
    [STR_RECIPE_POTION_FIRE_RESISTANCE] = "火炎耐性のポーション",
    [STR_RECIPE_POTION_WATER_BREATHING] = "水中呼吸のポーション",
    [STR_RECIPE_POTION_POISON] = "毒のポーション",
    [STR_RECIPE_PRESSURE_PLATE] = "石×2 -> 感圧板",
    [STR_SMELT_IRON] = "鉄鉱石 -> 鉄の地金",  // 鉄鉱石 -> 鉄の地金
    [STR_SMELT_PORK] = "生ポーク -> 焼いたポーク",
    [STR_SMELT_COBBLE] = "コブルストーン -> 石",
    [STR_SMELT_SAND] = "砂 -> ガラス",            // 砂 -> ガラス
    [STR_SMELT_GOLD] = "金鉱石 -> 金の地金",
    [STR_SMELT_REDSTONE] = "レッドストーン鉱石 -> レッドストーン",
    [STR_SMELT_LAPIS] = "ラピスラズリ鉱石 -> ラピスラズリ",
    [STR_SMELT_CLAY] = "粘土 -> レンガ",
    [STR_SMELT_ICE] = "氷 -> 水",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "燃料がない",
    [STR_MSG_SMELTING] = "精錬中: %s",
    [STR_MSG_CANNOT_SMELT] = "このアイテムは精錬できない",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "フルスクリーン切替",

    // Multiplayer
    [STR_BTN_HOST_GAME] = "ホストゲーム",
    [STR_BTN_JOIN_GAME] = "ゲーム参加",
    [STR_HOST_WAITING] = "プレイヤーを待っています...",
    [STR_HOST_IP_HINT] = "あなたのIP: %s:%d",
    [STR_JOIN_TITLE] = "ゲーム参加",
    [STR_JOIN_IP_HINT] = "ホストのIPアドレスを入力:",
    [STR_JOIN_CONNECTING] = "接続中...",
    [STR_HOST_PLAYERS_COUNT] = "プレイヤー: %d / %d",
    [STR_HOST_CANCEL_HINT] = "ESC: ホストを中止",
    [STR_HOST_START_HINT] = "ENTER / SPACE：ゲーム開始",
    [STR_JOIN_HELP_HINT] = "ESC: 戻る     Enter: 接続",
    [STR_JOIN_TAB_HINT] = "Tab: 切り替え",
    [STR_OPEN_TO_LAN] = "LANに公開",
    [STR_LAN_OPENED] = "LANに公開しました - %s:%d で参加できます",
    [STR_LAN_STATUS] = "LAN公開中 - %s:%d  (%d/%d)",
    [STR_LAN_FAILED] = "LAN公開に失敗しました",

    // Enchantment names
    [STR_ENCH_SHARPNESS] = "鋭さ",
    [STR_ENCH_EFFICIENCY] = "効率",
    [STR_ENCH_PROTECTION] = "保護",
    [STR_ENCH_FORTUNE] = "幸運",
    [STR_ENCH_UNBREAKING] = "耐久力",
    [STR_ENCH_SILK_TOUCH] = "シルクの触手",
    [STR_ENCH_POWER] = "パワー",
    [STR_ENCH_KNOCKBACK] = "ノックバック",
    [STR_ENCH_FIRE_ASPECT] = "フレイムアスペクト",
    // Fishing items
    [STR_ITEM_FISHING_ROD] = "釣竿",
    [STR_ITEM_RAW_FISH] = "生の魚",
    [STR_ITEM_COOKED_FISH] = "焼いた魚",
    // Cauldron
    [STR_BLOCK_CAULDRON] = "釜",
    [STR_ENCHANTED] = "エンチャント済み",

    // Enchanting messages
    [STR_MSG_ENCHANTED] = "エンチャント成功！",
    [STR_MSG_ALREADY_ENCHANTED] = "既にエンチャント済み",

    // Tutorial and multiplayer
    [STR_TUTORIAL_CONTROLS] = "WASD: 移動 | スペース: ジャンプ | 左クリック: 採掘 | 右クリック: 設置 | E: インベントリ",
    [STR_NET_PLAYER_JOINED] = "プレイヤーが参加しました！",
    [STR_NET_PLAYER_LEFT] = "プレイヤーが切断しました",
    [STR_NET_HOST_DISCONNECTED] = "ホストが切断しました",

    // Missing messages
    [STR_MSG_NO_ARROWS] = "矢がありません！",
    [STR_DEATH_LAVA] = "は溶岩で泳ごうとした",
    [STR_DEATH_CACTUS] = "はサボテンに刺された",

    // Ender pearl
    [STR_MSG_ENDER_PEARL] = "テレポート！",
    [STR_FISH_CAST] = "竿を投げた！",
    [STR_FISH_BITE] = "魚がかかった！",
    [STR_FISH_CATCH] = "何か釣れた！",
    [STR_FISH_RETRACT] = "竿を回収",
    [STR_MSG_CAULDRON_FILLED] = "大釜が満たされた！",
    [STR_MSG_CAULDRON_EMPTY] = "大釜を空にした！",
    [STR_MSG_CAULDRON_DRINK] = "大釜から水を飲んだ",
    [STR_BLOCK_OAK_STAIRS] = "オークの階段",
    [STR_BLOCK_COBBLESTONE_STAIRS] = "コブルストーンの階段",
    [STR_BLOCK_STONE_BRICKS] = "石レンガ",
    [STR_BLOCK_CHISELED_STONE_BRICKS] = "模様入りの石レンガ",
    [STR_BLOCK_OAK_SLAB] = "オークのハーフブロック",
    [STR_BLOCK_COBBLESTONE_SLAB] = "コブルストーンのハーフブロック",
    [STR_RECIPE_OAK_STAIRS] = "4 木材 -> 4 オークの階段",
    [STR_RECIPE_COBBLESTONE_STAIRS] = "4 コブルストーン -> 4 コブルストーンの階段",
    [STR_RECIPE_STONE_BRICKS] = "4 石 -> 4 石レンガ",
    [STR_RECIPE_CHISELED_STONE_BRICKS] = "2 石レンガハーフブロック -> 模様入りの石レンガ",
    [STR_RECIPE_OAK_SLAB] = "3 木材 -> 6 オークのハーフブロック",
    [STR_RECIPE_COBBLESTONE_SLAB] = "3 コブルストーン -> 6 コブルストーンのハーフブロック",
    [STR_BLOCK_CACTUS] = "サボテン",
    [STR_BLOCK_SUGAR_CANE] = "サトウキビ",
    [STR_ITEM_PAPER] = "紙",
    [STR_ITEM_BOOK] = "本",
    [STR_ITEM_SUGAR] = "砂糖",
    [STR_BLOCK_TNT] = "TNT",
    [STR_BLOCK_RED_SAND] = "赤い砂",
    [STR_BLOCK_MYCELIUM] = "菌糸",
    [STR_BLOCK_MUSHROOM_BLOCK] = "キノコブロック",
    [STR_BLOCK_MUSHROOM_STEM] = "キノコの柄",
    [STR_BLOCK_STONE_BUTTON] = "石のボタン",
    [STR_BLOCK_REDSTONE_REPEATER] = "レッドストーンリピーター",
    [STR_BLOCK_PISTON] = "ピストン",
    [STR_BLOCK_IRON_DOOR] = "鉄の扉",
    [STR_ITEM_GLASS_BOTTLE] = "ガラス瓶",
    [STR_ITEM_POTION_WATER] = "水入り瓶",
    [STR_ITEM_POTION_SPEED] = "速度のポーション",
    [STR_ITEM_POTION_STRENGTH] = "力のポーション",
    [STR_ITEM_POTION_REGEN] = "再生のポーション",
    [STR_ITEM_POTION_FIRE_RESISTANCE] = "火炎耐性のポーション",
    [STR_ITEM_POTION_WATER_BREATHING] = "水中呼吸のポーション",
    [STR_ITEM_POTION_POISON] = "毒のポーション",
    [STR_MSG_DRANK_POTION] = "効果を感じた！",
    [STR_MSG_BOTTLE_FILLED] = "瓶に水を入れた",
    [STR_MSG_IRON_DOOR_HINT] = "鉄の扉はレッドストーンで開く",
    [STR_RECIPE_TNT] = "TNT",
    [STR_RECIPE_PAPER] = "3 サトウキビ -> 3 紙",
    [STR_RECIPE_BOOK] = "2 紙 + 1 革 -> 1 本",
    [STR_RECIPE_SUGAR] = "1 サトウキビ -> 1 砂糖",
    [STR_MSG_CACTUS_DAMAGE] = "痛い！サボテンが刺さった！",
    [STR_TYPE_TOOL] = "道具",
    [STR_TYPE_ARMOR] = "防具",
    [STR_TYPE_FOOD] = "食べ物",
    [STR_TYPE_BLOCK] = "ブロック",
    [STR_RECIPE_SLIMEBALL_STRING] = "スライムボール繊維",

    // Farming
    [STR_TOOL_WOOD_HOE] = "木のクワ",
    [STR_TOOL_STONE_HOE] = "石のクワ",
    [STR_TOOL_IRON_HOE] = "鉄のクワ",
    [STR_TOOL_GOLD_HOE] = "金のクワ",
    [STR_TOOL_DIAMOND_HOE] = "ダイヤのクワ",
    [STR_ITEM_WHEAT_SEEDS] = "小麦の種",
    [STR_ITEM_WHEAT] = "小麦",
    [STR_BLOCK_FARMLAND] = "耕地",
    [STR_BLOCK_CROPS] = "作物",
    [STR_BLOCK_HAY_BALE] = "干草の俵",
    [STR_RECIPE_WOOD_HOE] = "木のクワ",
    [STR_RECIPE_STONE_HOE] = "石のクワ",
    [STR_RECIPE_IRON_HOE] = "鉄のクワ",
    [STR_RECIPE_GOLD_HOE] = "金のクワ",
    [STR_RECIPE_DIAMOND_HOE] = "ダイヤのクワ",
    [STR_RECIPE_BREAD] = "パン",
    [STR_RECIPE_HAY_BALE] = "干草の俵",
    [STR_RECIPE_FISHING_ROD] = "棒3本+糸2本 -> 釣り竿",
    [STR_RECIPE_MOSSY_COBBLESTONE] = "丸石 -> 苔むした丸石",
    [STR_RECIPE_COARSE_DIRT] = "土2つ -> 粗い土",

    // New mob deaths
    [STR_DEATH_MOB_COW] = "は牛に蹴られた",
    [STR_DEATH_MOB_SHEEP] = "は羊に突かれた",
    [STR_DEATH_MOB_CHICKEN] = "は鶏に突かれ死んだ",
    [STR_DEATH_MOB_WOLF] = "は狼に襲われ死んだ",
    [STR_DEATH_MOB_WITCH] = "は魔女に殺された",
    [STR_DEATH_MOB_BAT] = "は蝙蝠に驚かされ死んだ",

    // Mob names
    [STR_MOB_PIG] = "豚",
    [STR_MOB_COW] = "牛",
    [STR_MOB_SHEEP] = "羊",
    [STR_MOB_CHICKEN] = "鶏",
    [STR_MOB_VILLAGER] = "村人",
    [STR_MOB_HORSE] = "馬",
    [STR_MOB_WOLF] = "狼",
    [STR_MOB_WITCH] = "魔女",
    [STR_MOB_BAT] = "蝙蝠",

    // Animal drops
    [STR_ITEM_RAW_BEEF] = "生の牛肉",
    [STR_ITEM_LEATHER] = "革",
    [STR_ITEM_RAW_MUTTON] = "生の羊肉",
    [STR_ITEM_WOOL] = "羊毛",
    [STR_ITEM_RAW_CHICKEN] = "生の鶏肉",
    [STR_ITEM_FEATHER] = "羽",
    [STR_ITEM_EGG] = "卵",
    [STR_ITEM_COOKED_BEEF] = "焼いた牛肉",
    [STR_ITEM_COOKED_MUTTON] = "焼いた羊肉",
    [STR_ITEM_COOKED_CHICKEN] = "焼いた鶏肉",

    // Smelt recipes
    [STR_SMELT_BEEF] = "牛肉を焼く",
    [STR_SMELT_MUTTON] = "羊肉を焼く",
    [STR_SMELT_CHICKEN] = "鶏肉を焼く",
    [STR_SMELT_FISH] = "魚を焼く",
    [STR_MSG_CANT_SLEEP_MOBS] = "近くにモンスターがいる！",
    // Creative mode
    [STR_GAMEMODE] = "モード",
    [STR_MODE_SURVIVAL] = "サバイバル",
    [STR_MODE_CREATIVE] = "クリエイティブ",
    [STR_MSG_FLIGHT_ON] = "飛行モードON",
    [STR_MSG_FLIGHT_OFF] = "飛行モードOFF",
    [STR_CREATIVE_TITLE] = "クリエイティブインベントリ",
    [STR_CREATIVE_SELECTED] = "選択中: %s",
    [STR_CREATIVE_SELECT_HINT] = "アイテムをクリックして取得（無限）",
    [STR_CREATIVE_SEARCH] = "検索：",
    [STR_CREATIVE_NO_MATCH] = "該当なし",
    [STR_MODE_SET] = "ゲームモード: %s",
    [STR_CREATIVE_BACKPACK] = "バックパック",
    [STR_HINT_SCROLL] = "スクロールで閲覧",
    [STR_MSG_HOST_ONLY] = "ゲームモードを変更できるのはホストのみです",
    // Easter eggs
    [STR_EGG_KONAMI_ON] = "コナミコマンド！パーティーモードON",
    [STR_EGG_KONAMI_OFF] = "パーティーモードOFF",
    [STR_EGG_TITLE_CLICK] = "ねえ、タイトルを連打しないで！",
    [STR_EGG_FACT1] = "豆知識：岩盤は理由があって壊せない……",
    [STR_EGG_FACT2] = "豆知識：奈落は一方通行です。",
    [STR_EGG_FACT3] = "豆知識：羊は白がお好き。",
},
};

//----------------------------------------------------------------------------------
// Lookup Functions
//----------------------------------------------------------------------------------
const char *S(StringId id)
{
    if (id <= STR_NONE || id >= STR_COUNT) return "";
    const char *s = strings[language][id];
    if (s) return s;
    // Fallback to English
    s = strings[LANG_EN][id];
    return s ? s : "";
}

// The StringId enum's block-name section is not laid out contiguously in
// BlockType order (other UI strings such as recipe names are interleaved, and
// the redstone/lava/bucket/enchanting entries are ordered differently), so we
// cannot use STR_BLOCK_AIR + bt. An explicit per-BlockType table is required.
static const StringId g_blockNameIds[BLOCK_COUNT] = {
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
    STR_BLOCK_GOLD_ORE,
    STR_BLOCK_DIAMOND_ORE,
    STR_BLOCK_REDSTONE_ORE,
    STR_BLOCK_LAPIS_ORE,
    STR_ITEM_GOLD_INGOT,
    STR_ITEM_DIAMOND,
    STR_ITEM_REDSTONE,
    STR_ITEM_LAPIS,
    STR_BLOCK_CHEST,
    STR_TOOL_GOLD_PICKAXE,
    STR_TOOL_GOLD_AXE,
    STR_TOOL_GOLD_SWORD,
    STR_TOOL_GOLD_SHOVEL,
    STR_TOOL_GOLD_HOE,
    STR_TOOL_DIAMOND_PICKAXE,
    STR_TOOL_DIAMOND_AXE,
    STR_TOOL_DIAMOND_SWORD,
    STR_TOOL_DIAMOND_SHOVEL,
    STR_TOOL_DIAMOND_HOE,
    STR_ARMOR_GOLD_HELMET,
    STR_ARMOR_GOLD_CHESTPLATE,
    STR_ARMOR_GOLD_LEGGINGS,
    STR_ARMOR_GOLD_BOOTS,
    STR_ARMOR_DIAMOND_HELMET,
    STR_ARMOR_DIAMOND_CHESTPLATE,
    STR_ARMOR_DIAMOND_LEGGINGS,
    STR_ARMOR_DIAMOND_BOOTS,
    STR_ITEM_GUNPOWDER,
    STR_ITEM_STRING,
    STR_ITEM_BONE,
    STR_ITEM_ARROW,
    STR_ITEM_BOW,
    STR_BLOCK_MOSSY_COBBLESTONE,
    STR_BLOCK_BOOKSHELF,
    STR_BLOCK_LANTERN,
    STR_BLOCK_BONE_BLOCK,
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
    STR_ITEM_SLIMEBALL,
    STR_ITEM_ENDER_PEARL,
    STR_ITEM_WHEAT_SEEDS,
    STR_ITEM_WHEAT,
    STR_BLOCK_FARMLAND,
    STR_BLOCK_CROPS,
    STR_BLOCK_HAY_BALE,
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
    STR_ITEM_BUCKET,
    STR_ITEM_WATER_BUCKET,
    STR_BLOCK_LEVER,
    STR_BLOCK_REDSTONE_WIRE,
    STR_BLOCK_REDSTONE_LAMP,
    STR_BLOCK_STONE_PRESSURE_PLATE,
    STR_BLOCK_LAVA,
    STR_BLOCK_OBSIDIAN,
    STR_ITEM_LAVA_BUCKET,
    STR_BLOCK_ENCHANTING_TABLE,
    STR_ITEM_FISHING_ROD,
    STR_ITEM_RAW_FISH,
    STR_ITEM_COOKED_FISH,
    STR_BLOCK_CAULDRON,
    STR_BLOCK_OAK_STAIRS,
    STR_BLOCK_COBBLESTONE_STAIRS,
    STR_BLOCK_STONE_BRICKS,
    STR_BLOCK_CHISELED_STONE_BRICKS,
    STR_BLOCK_OAK_SLAB,
    STR_BLOCK_COBBLESTONE_SLAB,
    STR_BLOCK_CACTUS,
    STR_BLOCK_SUGAR_CANE,
    STR_ITEM_PAPER,
    STR_ITEM_BOOK,
    STR_ITEM_SUGAR,
    STR_BLOCK_TNT,
    STR_BLOCK_RED_SAND,
    STR_BLOCK_MYCELIUM,
    STR_BLOCK_MUSHROOM_BLOCK,
    STR_BLOCK_MUSHROOM_STEM,
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
};

const char *GetBlockName(BlockType bt)
{
    if (bt < 0 || bt >= BLOCK_COUNT) return "";
    return S(g_blockNameIds[bt]);
}

// Format string helper: Sf(STR_MSG_ATE, "Apple", 4) -> "Ate Apple (+4 hunger)"
const char *Sf(StringId id, ...)
{
    static char buf[256];
    va_list args;
    va_start(args, id);
    vsnprintf(buf, sizeof(buf), S(id), args);
    va_end(args);
    return buf;
}

//----------------------------------------------------------------------------------
// Font Loading
//----------------------------------------------------------------------------------

// Collect unique codepoints from all translation strings
static int *g_codepoints = NULL;
static int g_codepointCount = 0;

static void AddCodepoint(int *set, int *count, int cp)
{
    for (int i = 0; i < *count; i++) {
        if (set[i] == cp) return;
    }
    set[(*count)++] = cp;
}

static void GenerateCJKCodepoints(void)
{
    if (g_codepoints) return;

    // First pass: collect all unique codepoints from string table
    int maxCp = 4096;
    int *tempSet = (int *)RL_CALLOC(maxCp, sizeof(int));
    int tempCount = 0;
    if (!tempSet) return;

    // Always include ASCII printable
    for (int c = 0x0020; c <= 0x007F; c++) {
        tempSet[tempCount++] = c;
    }

    // Scan all translation strings for unique codepoints
    for (int lang = 0; lang < LANG_COUNT; lang++) {
        for (int id = 0; id < STR_COUNT; id++) {
            const char *s = strings[lang][id];
            if (!s) continue;
            while (*s) {
                int cp = 0;
                unsigned char c = (unsigned char)*s;
                if (c < 0x80) {
                    cp = c;
                    s++;
                } else if ((c & 0xE0) == 0xC0) {
                    cp = ((c & 0x1F) << 6) | (s[1] & 0x3F);
                    s += 2;
                } else if ((c & 0xF0) == 0xE0) {
                    cp = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
                    s += 3;
                } else if ((c & 0xF8) == 0xF0) {
                    cp = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
                    s += 4;
                } else {
                    s++;
                    continue;
                }
                if (cp >= 0x0020 && tempCount < maxCp) {
                    AddCodepoint(tempSet, &tempCount, cp);
                }
            }
        }
    }

    // Add common CJK ranges for completeness (numbers, common punctuation)
    for (int c = 0x3000; c <= 0x303F; c++) AddCodepoint(tempSet, &tempCount, c); // CJK punctuation
    for (int c = 0x3040; c <= 0x309F; c++) AddCodepoint(tempSet, &tempCount, c); // Hiragana
    for (int c = 0x30A0; c <= 0x30FF; c++) AddCodepoint(tempSet, &tempCount, c); // Katakana

    g_codepoints = tempSet;
    g_codepointCount = tempCount;
}

static Font TryLoadFont(const char *path, int fontSize)
{
    if (!FileExists(path)) return (Font){0};

    GenerateCJKCodepoints();
    if (!g_codepoints || g_codepointCount == 0) return (Font){0};

    Font f = LoadFontEx(path, fontSize, g_codepoints, g_codepointCount);
    if (f.texture.id > 0) {
        // Point filtering (no mipmaps): UI text is drawn at 9-16px while the
        // atlas is baked at FONT_ATLAS_SIZE. Linear filtering + mipmaps makes
        // small glyphs blurry; nearest-neighbour keeps them crisp.
        SetTextureFilter(f.texture, TEXTURE_FILTER_POINT);
    }
    return f;
}

void LoadGameFont(void)
{
    // Resolve a usable font path first, then bake both atlases from it. Baking a
    // second, larger atlas matters for titles: the body atlas is sized for ~14px
    // text, so drawing a 42px title from it means scaling up 3x and the strokes
    // smear. Two atlases keep both ends of the size range at (or near) 1:1.
    const char *chosen = NULL;

    const char *bundled = "assets/fonts/LXGWWenKaiLite-Regular.ttf";
    if (FileExists(bundled)) chosen = bundled;

    if (!chosen && customFontPath[0] && FileExists(customFontPath)) chosen = customFontPath;

    // System CJK fonts, plain .ttf first (more reliable in raylib than .ttc).
    const char *systemFonts[] = {
        "C:/Windows/Fonts/Deng.ttf",     // DengXian - cleaner vector CJK than SimHei
        "C:/Windows/Fonts/simhei.ttf",   // SimHei
        "C:/Windows/Fonts/msyh.ttc",     // Microsoft YaHei
        "C:/Windows/Fonts/simsun.ttc",   // SimSun
        "C:/Windows/Fonts/msgothic.ttc", // MS Gothic (Japanese)
        NULL
    };
    if (!chosen) {
        for (int i = 0; systemFonts[i]; i++) {
            if (FileExists(systemFonts[i])) { chosen = systemFonts[i]; break; }
        }
    }

    if (!chosen) {
        useCustomFont = false;
        return;
    }

    gameFont      = TryLoadFont(chosen, FONT_ATLAS_SIZE);
    gameFontLarge = TryLoadFont(chosen, FONT_ATLAS_SIZE_LARGE);
    useCustomFont = (gameFont.texture.id > 0);
}

void UnloadGameFont(void)
{
    if (gameFont.texture.id > 0) {
        UnloadFont(gameFont);
        gameFont = (Font){0};
    }
    if (gameFontLarge.texture.id > 0) {
        UnloadFont(gameFontLarge);
        gameFontLarge = (Font){0};
    }
    useCustomFont = false;
}

bool ReloadGameFont(const char *path)
{
    UnloadGameFont();
    gameFont = TryLoadFont(path, FONT_ATLAS_SIZE);
    if (gameFont.texture.id > 0) {
        useCustomFont = true;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------
// Font Helper Wrappers
//----------------------------------------------------------------------------------
// Pick the atlas that matches the requested size. The body atlas is baked for
// ~14px and the large one for ~42px, so a 42px title comes from the atlas that
// actually contains 42px glyphs instead of a 3x upscale of the small one.
static Font PickFont(int fsize)
{
    if (fsize >= 28 && gameFontLarge.texture.id > 0) return gameFontLarge;
    return gameFont;
}

void DrawGameText(const char *text, int posX, int posY, int fsize, Color color)
{
    Font f = PickFont(fsize);
    if (useCustomFont && f.texture.id > 0) {
        DrawTextEx(f, text, (Vector2){(float)posX, (float)posY}, (float)fsize, 1.0f, color);
    } else {
        DrawText(text, posX, posY, fsize, color);
    }
}

Vector2 MeasureGameText(const char *text, int fsize)
{
    Font f = PickFont(fsize);   // must match DrawGameText or text will be misaligned
    if (useCustomFont && f.texture.id > 0) {
        return MeasureTextEx(f, text, (float)fsize, 1.0f);
    }
    return (Vector2){ (float)MeasureText(text, fsize), (float)fsize };
}

int MeasureGameTextWidth(const char *text, int fsize)
{
    return (int)MeasureGameText(text, fsize).x;
}
