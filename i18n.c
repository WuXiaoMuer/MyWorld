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
bool useCustomFont = false;
char customFontPath[256] = { 0 };

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

    // Status Messages
    [STR_MSG_GAME_SAVED] = "Game Saved",
    [STR_MSG_HEALED_XP] = "Healed with XP!",
    [STR_MSG_NOT_ENOUGH_XP] = "Not enough XP!",
    [STR_MSG_INVENTORY_FULL] = "Inventory full!",
    [STR_MSG_TOOL_BROKE] = "Tool broke!",
    [STR_MSG_TOOL_WEARING] = "Tool is wearing out!",
    [STR_MSG_FALL_DAMAGE] = "Ouch! Fall damage!",
    [STR_MSG_SPAWN_SET] = "Spawn point set!",
    [STR_MSG_HUNGRY] = "Hungry!",
    [STR_MSG_STARVING] = "Starving!",
    [STR_MSG_ATE] = "Ate %s (+%d hunger)",
    [STR_MSG_BROKE] = "%s broke!",

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

    // Recipe Names
    [STR_RECIPE_WOOD_PLANKS] = "Wood -> 4 Planks",
    [STR_RECIPE_PLANKS_STICKS] = "2 Planks -> 4 Sticks",
    [STR_RECIPE_WOOD_PICK] = "3 Planks + 2 Sticks -> Wood Pick",
    [STR_RECIPE_WOOD_AXE] = "3 Planks + 2 Sticks -> Wood Axe",
    [STR_RECIPE_WOOD_SWORD] = "2 Planks + 1 Stick -> Wood Sword",
    [STR_RECIPE_WOOD_SHOVEL] = "1 Plank + 2 Sticks -> Wood Shovel",
    [STR_RECIPE_STONE_PICK] = "3 Cobble + 2 Sticks -> Stone Pick",
    [STR_RECIPE_STONE_AXE] = "3 Cobble + 2 Sticks -> Stone Axe",
    [STR_RECIPE_STONE_SWORD] = "2 Cobble + 1 Stick -> Stone Sword",
    [STR_RECIPE_STONE_SHOVEL] = "1 Cobble + 2 Sticks -> Stone Shovel",
    [STR_RECIPE_IRON_PICK] = "3 Iron + 2 Sticks -> Iron Pick",
    [STR_RECIPE_IRON_AXE] = "3 Iron + 2 Sticks -> Iron Axe",
    [STR_RECIPE_IRON_SWORD] = "2 Iron + 1 Stick -> Iron Sword",
    [STR_RECIPE_IRON_SHOVEL] = "1 Iron + 2 Sticks -> Iron Shovel",
    [STR_RECIPE_BRICK] = "4 Clay -> 4 Bricks",
    [STR_RECIPE_FURNACE] = "8 Cobble -> Furnace",
    [STR_RECIPE_TORCHES] = "1 Coal + 1 Stick -> 4 Torches",
    [STR_RECIPE_SANDSTONE] = "4 Sand -> Sandstone",
    [STR_RECIPE_COBBLE] = "2 Cobble -> 2 Stone",
    [STR_RECIPE_BED] = "3 Planks + 3 Tall Grass -> Bed",
    [STR_RECIPE_CRAFTING_TABLE] = "8 Planks -> Crafting Table",
    [STR_RECIPE_BREAD] = "3 Wheat -> Bread",
    [STR_RECIPE_APPLE] = "1 Apple -> Apple",
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
    // Smelt
    [STR_SMELT_IRON] = "Iron Ore -> Iron Ingot",
    [STR_SMELT_PORK] = "Raw Pork -> Cooked Pork",
    [STR_SMELT_COBBLE] = "Cobblestone -> Stone",
    [STR_SMELT_SAND] = "Sand -> Glass",
    [STR_SMELT_GOLD] = "Gold Ore -> Gold Ingot",
    [STR_SMELT_REDSTONE] = "Redstone Ore -> Redstone",
    [STR_SMELT_LAPIS] = "Lapis Ore -> Lapis Lazuli",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "No fuel",
    [STR_MSG_SMELTING] = "Smelting: %s",
    [STR_MSG_CANNOT_SMELT] = "Cannot smelt this item",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "Toggle fullscreen",
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
    [STR_KEY_LCLICK] = "左键",                            // 左键
    [STR_KEY_RCLICK] = "右键",                            // 右键
    [STR_KEY_E] = "E",
    [STR_KEY_H] = "H",
    [STR_KEY_F3] = "F3",
    [STR_KEY_ESC] = "ESC",
    [STR_KEY_19] = "1-9",

    [STR_ACT_MOVE] = "移动",                              // 移动
    [STR_ACT_JUMP] = "跳跃/游泳",                // 跳跃/游泳
    [STR_ACT_SPRINT] = "冲刺",                            // 冲刺
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

    [STR_MSG_GAME_SAVED] = "游戏已保存",     // 游戏已保存
    [STR_MSG_HEALED_XP] = "用XP治疗了!",         // 用XP治疗了!
    [STR_MSG_NOT_ENOUGH_XP] = "XP不足!",                 // XP不足!
    [STR_MSG_INVENTORY_FULL] = "背包已满!",      // 背包已满!
    [STR_MSG_TOOL_BROKE] = "工具坏了!",          // 工具坏了!
    [STR_MSG_TOOL_WEARING] = "工具快坏了!",  // 工具快坏了!
    [STR_MSG_FALL_DAMAGE] = "哎哟! 摔伤了!", // 哎哟! 摔伤了!
    [STR_MSG_SPAWN_SET] = "已设置生成点!", // 已设置生成点!
    [STR_MSG_HUNGRY] = "饥饿!",                           // 饥饿!
    [STR_MSG_STARVING] = "饥饿难耐!",            // 饥饿难耐!
    [STR_MSG_ATE] = "吃了 %s (+%d饥饿度)",   // 吃了 %s (+%d饥饿度)
    [STR_MSG_BROKE] = "%s 坏了!",                         // %s 坏了!

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

    // Recipe names
    [STR_RECIPE_WOOD_PLANKS] = "木头 -> 4木板",   // 木头 -> 4木板
    [STR_RECIPE_PLANKS_STICKS] = "2木板 -> 4棒子", // 2木板 -> 4棍子
    [STR_RECIPE_WOOD_PICK] = "3木板+2棒子 -> 木镐",
    [STR_RECIPE_WOOD_AXE] = "3木板+2棒子 -> 木斟",
    [STR_RECIPE_WOOD_SWORD] = "2木板+1棒子 -> 木剑",
    [STR_RECIPE_WOOD_SHOVEL] = "1木板+2棒子 -> 木铲",
    [STR_RECIPE_STONE_PICK] = "3圆石+2棒子 -> 石镐",
    [STR_RECIPE_STONE_AXE] = "3圆石+2棒子 -> 石斟",
    [STR_RECIPE_STONE_SWORD] = "2圆石+1棒子 -> 石剑",
    [STR_RECIPE_STONE_SHOVEL] = "1圆石+2棒子 -> 石铲",
    [STR_RECIPE_IRON_PICK] = "3铁锭+2棒子 -> 铁镐",
    [STR_RECIPE_IRON_AXE] = "3铁锭+2棒子 -> 铁斟",
    [STR_RECIPE_IRON_SWORD] = "2铁锭+1棒子 -> 铁剑",
    [STR_RECIPE_IRON_SHOVEL] = "1铁锭+2棒子 -> 铁铲",
    [STR_RECIPE_BRICK] = "4粘土 -> 4砖块",
    [STR_RECIPE_FURNACE] = "8圆石 -> 熔炉",
    [STR_RECIPE_TORCHES] = "1煤炭+1棒子 -> 4火把",
    [STR_RECIPE_SANDSTONE] = "4沙子 -> 砂岩",
    [STR_RECIPE_COBBLE] = "2圆石 -> 2石头",
    [STR_RECIPE_BED] = "3木板+3高草 -> 床",
    [STR_RECIPE_CRAFTING_TABLE] = "8木板 -> 合成台",
    [STR_RECIPE_BREAD] = "3小麦 -> 面包",
    [STR_RECIPE_APPLE] = "1苹果 -> 苹果",
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
    [STR_SMELT_IRON] = "铁矿 -> 铁锭",            // 铁矿 -> 铁锭
    [STR_SMELT_PORK] = "生猪肉 -> 烤猪肉", // 生猪肉 -> 烤猪肉
    [STR_SMELT_COBBLE] = "圆石 -> 石头",          // 圆石 -> 石头
    [STR_SMELT_SAND] = "沙子 -> 玻璃",            // 沙子 -> 玻璃
    [STR_SMELT_GOLD] = "金矿 -> 金锭",
    [STR_SMELT_REDSTONE] = "红石矿 -> 红石",
    [STR_SMELT_LAPIS] = "青金石矿 -> 青金石",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "没有燃料",
    [STR_MSG_SMELTING] = "冶炼中: %s",
    [STR_MSG_CANNOT_SMELT] = "无法冶炼此物品",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "切换全屏",
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
    [STR_KEY_LCLICK] = "左クリック",          // 左クリック
    [STR_KEY_RCLICK] = "右クリック",          // 右クリック
    [STR_KEY_E] = "E",
    [STR_KEY_H] = "H",
    [STR_KEY_F3] = "F3",
    [STR_KEY_ESC] = "ESC",
    [STR_KEY_19] = "1-9",

    [STR_ACT_MOVE] = "移動",                               // 移動
    [STR_ACT_JUMP] = "ジャンプ/泳ぎ",     // ジャンプ/泳ぎ
    [STR_ACT_SPRINT] = "スプリント",          // スプリント
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

    [STR_MSG_GAME_SAVED] = "セーブしました",  // セーブしました
    [STR_MSG_HEALED_XP] = "XPで治癒しました!",  // XPで治療しました!
    [STR_MSG_NOT_ENOUGH_XP] = "XPが不足!",            // XPが不足!
    [STR_MSG_INVENTORY_FULL] = "インベントリが満です!",  // インベントリが満です!
    [STR_MSG_TOOL_BROKE] = "道具が壊れた!",  // 道具が壊れた!
    [STR_MSG_TOOL_WEARING] = "道具がほぼ壊れ!",  // 道具がほぼ壊れ!
    [STR_MSG_FALL_DAMAGE] = "いったっ! 落下ダメージ!",  // いたっ! 落下ダメージ!
    [STR_MSG_SPAWN_SET] = "スポーンポイントを設定!",  // スポーンポイントを設定!
    [STR_MSG_HUNGRY] = "腹が空いた!",        // 腹が空いた!
    [STR_MSG_STARVING] = "餓死!",                          // 餓死!
    [STR_MSG_ATE] = "%sを食べた (+%d満腹度)",  // %sを食べた (+%d満腹度)
    [STR_MSG_BROKE] = "%sが壊れた!",              // %sが壊れた!

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

    // Recipe names (Japanese)
    [STR_RECIPE_WOOD_PLANKS] = "材 -> 板×4",         // 材 -> 板×4
    [STR_RECIPE_PLANKS_STICKS] = "板×2 -> 棒×4",  // 板×2 -> 棒×4
    [STR_RECIPE_WOOD_PICK] = "板×3+棒×2 -> 木のつるはし",
    [STR_RECIPE_WOOD_AXE] = "板×3+棒×2 -> 木のオノ",
    [STR_RECIPE_WOOD_SWORD] = "板×2+棒×1 -> 木の剣",
    [STR_RECIPE_WOOD_SHOVEL] = "板×1+棒×2 -> 木のシャベル",
    [STR_RECIPE_STONE_PICK] = "コブル×3+棒×2 -> 石のつるはし",
    [STR_RECIPE_STONE_AXE] = "コブル×3+棒×2 -> 石のオノ",
    [STR_RECIPE_STONE_SWORD] = "コブル×2+棒×1 -> 石の剣",
    [STR_RECIPE_STONE_SHOVEL] = "コブル×1+棒×2 -> 石のシャベル",
    [STR_RECIPE_IRON_PICK] = "鉄の地金×3+棒×2 -> 鉄のつるはし",
    [STR_RECIPE_IRON_AXE] = "鉄の地金×3+棒×2 -> 鉄のオノ",
    [STR_RECIPE_IRON_SWORD] = "鉄の地金×2+棒×1 -> 鉄の剣",
    [STR_RECIPE_IRON_SHOVEL] = "鉄の地金×1+棒×2 -> 鉄のシャベル",
    [STR_RECIPE_BRICK] = "粘土×4 -> レンガ×4",
    [STR_RECIPE_FURNACE] = "コブル×8 -> 炉",
    [STR_RECIPE_TORCHES] = "炭×1+棒×1 -> 松明×4",
    [STR_RECIPE_SANDSTONE] = "砂×4 -> 砂岩",
    [STR_RECIPE_COBBLE] = "コブル×2 -> 石×2",
    [STR_RECIPE_BED] = "板×3+高い草×3 -> ベッド",
    [STR_RECIPE_CRAFTING_TABLE] = "板×8 -> クラフティングテーブル",
    [STR_RECIPE_BREAD] = "小麦×3 -> パン",
    [STR_RECIPE_APPLE] = "リンゴ×1 -> リンゴ",
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
    [STR_SMELT_IRON] = "鉄鉱石 -> 鉄の地金",  // 鉄鉱石 -> 鉄の地金
    [STR_SMELT_PORK] = "生ポーク -> 焼いたポーク",
    [STR_SMELT_COBBLE] = "コブルストーン -> 石",
    [STR_SMELT_SAND] = "砂 -> ガラス",            // 砂 -> ガラス
    [STR_SMELT_GOLD] = "金鉱石 -> 金の地金",
    [STR_SMELT_REDSTONE] = "レッドストーン鉱石 -> レッドストーン",
    [STR_SMELT_LAPIS] = "ラピスラズリ鉱石 -> ラピスラズリ",

    // Furnace messages
    [STR_MSG_NO_FUEL] = "燃料がない",
    [STR_MSG_SMELTING] = "精錬中: %s",
    [STR_MSG_CANNOT_SMELT] = "このアイテムは精錬できない",

    // Additional controls
    [STR_KEY_F11] = "F11",
    [STR_ACT_FULLSCREEN] = "フルスクリーン切替",
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

const char *GetBlockName(BlockType bt)
{
    if (bt < 0 || bt >= BLOCK_COUNT) return "";
    return S(STR_BLOCK_AIR + bt);
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
        GenTextureMipmaps(&f.texture);
        SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    }
    return f;
}

void LoadGameFont(void)
{
    // Try bundled font first
    const char *bundled = "assets/fonts/LXGWWenKaiLite-Regular.ttf";
    gameFont = TryLoadFont(bundled, 48);
    if (gameFont.texture.id > 0) {
        useCustomFont = true;
        return;
    }

    // Try custom path if set
    if (customFontPath[0]) {
        gameFont = TryLoadFont(customFontPath, 48);
        if (gameFont.texture.id > 0) {
            useCustomFont = true;
            return;
        }
    }

    // Try system CJK fonts (prefer .ttf over .ttc for better raylib compatibility)
    const char *systemFonts[] = {
        "C:/Windows/Fonts/simhei.ttf", // SimHei (plain TTF, most reliable)
        "C:/Windows/Fonts/msyh.ttc",   // Microsoft YaHei
        "C:/Windows/Fonts/simsun.ttc", // SimSun
        "C:/Windows/Fonts/msgothic.ttc", // MS Gothic (Japanese)
        NULL
    };
    for (int i = 0; systemFonts[i]; i++) {
        gameFont = TryLoadFont(systemFonts[i], 48);
        if (gameFont.texture.id > 0) {
            useCustomFont = true;
            return;
        }
    }

    // Fallback: use raylib default font
    useCustomFont = false;
}

void UnloadGameFont(void)
{
    if (gameFont.texture.id > 0) {
        UnloadFont(gameFont);
        gameFont = (Font){0};
    }
    useCustomFont = false;
}

bool ReloadGameFont(const char *path)
{
    UnloadGameFont();
    gameFont = TryLoadFont(path, 48);
    if (gameFont.texture.id > 0) {
        useCustomFont = true;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------
// Font Helper Wrappers
//----------------------------------------------------------------------------------
void DrawGameText(const char *text, int posX, int posY, int fsize, Color color)
{
    if (useCustomFont && gameFont.texture.id > 0) {
        DrawTextEx(gameFont, text, (Vector2){(float)posX, (float)posY}, (float)fsize, 1.0f, color);
    } else {
        DrawText(text, posX, posY, fsize, color);
    }
}

Vector2 MeasureGameText(const char *text, int fsize)
{
    if (useCustomFont && gameFont.texture.id > 0) {
        return MeasureTextEx(gameFont, text, (float)fsize, 1.0f);
    }
    return (Vector2){ (float)MeasureText(text, fsize), (float)fsize };
}

int MeasureGameTextWidth(const char *text, int fsize)
{
    return (int)MeasureGameText(text, fsize).x;
}
