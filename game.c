#include "types.h"
#include "net.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#include <string.h>
#include <math.h>

//----------------------------------------------------------------------------------
// Window Mode Management
//----------------------------------------------------------------------------------
static int savedWinX = 0, savedWinY = 0, savedWinW = SCREEN_WIDTH, savedWinH = SCREEN_HEIGHT;
static const int resolutionWidths[] = { 960, 1280, 1600 };
static const int resolutionHeights[] = { 540, 720, 900 };

void ApplyResolution(int preset)
{
    if (preset < 0 || preset >= 3) preset = 1;
    resolutionPreset = preset;
    if (windowMode == 0) {
        int width = resolutionWidths[preset];
        int height = resolutionHeights[preset];
        int monW = GetMonitorWidth(GetCurrentMonitor());
        int monH = GetMonitorHeight(GetCurrentMonitor());
        if (width > monW - 32) width = monW - 32;
        if (height > monH - 64) height = monH - 64;
        if (width < 640) width = 640;
        if (height < 360) height = 360;
        SetWindowSize(width, height);
    }
}


void ApplyWindowMode(int mode)
{
    if (mode < 0 || mode > 2) mode = 0;

    int monW = GetMonitorWidth(GetCurrentMonitor());
    int monH = GetMonitorHeight(GetCurrentMonitor());

    if (mode == 0) {
        // Windowed: restore the selected preset and previous position
        if (IsWindowFullscreen()) ToggleFullscreen();
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(resolutionWidths[resolutionPreset], resolutionHeights[resolutionPreset]);
        SetWindowPosition(savedWinX, savedWinY);
    } else if (mode == 1) {
        // Exclusive fullscreen
        if (!IsWindowFullscreen()) {
            // Save current window pos/size
            savedWinX = GetWindowPosition().x;
            savedWinY = GetWindowPosition().y;
            savedWinW = GetScreenWidth();
            savedWinH = GetScreenHeight();
            ToggleFullscreen();
        }
    } else if (mode == 2) {
        // Borderless fullscreen
        if (IsWindowFullscreen()) ToggleFullscreen();
        savedWinX = GetWindowPosition().x;
        savedWinY = GetWindowPosition().y;
        savedWinW = GetScreenWidth();
        savedWinH = GetScreenHeight();
        SetWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(monW, monH);
        SetWindowPosition(0, 0);
    }
    windowMode = mode;
}

char currentSavePath[256] = { 0 };
uint64_t simulationTick = 0;
float simulationAccumulator = 0.0f;
static uint16_t fluidSnapshotNext[MAX_NET_PLAYERS] = { 0 };
static uint16_t fluidSnapshotId[MAX_NET_PLAYERS] = { 0 };
static bool fluidSnapshotActive[MAX_NET_PLAYERS] = { false };

static int SendFluidSnapshotBatch(int clientId, uint16_t batchIndex)
{
    FluidChange *cells = NULL;
    int total = 0, cap = 0;
    for (int x = 0; x < WORLD_WIDTH; x++) for (int y = 0; y < WORLD_HEIGHT; y++) {
        uint8_t bt, kind, level; bool source;
        GetFluidState(x, y, &bt, &kind, &level, &source);
        if (!kind || (!level && !source)) continue;
        if (total >= cap) {
            cap = cap ? cap * 2 : 256;
            FluidChange *next = (FluidChange *)realloc(cells, (size_t)cap * sizeof(FluidChange));
            if (!next) { free(cells); return 0; }
            cells = next;
        }
        cells[total++] = (FluidChange){ (uint16_t)x, (uint16_t)y, bt, kind, level, source };
    }
    int cellBytes = (int)sizeof(PktFluidCell);
    int maxCells = (NET_PACKET_MAX - 1 - (int)sizeof(PktFluidSnapshotHeader)) / cellBytes;
    if (maxCells < 1) { free(cells); return 0; }
    int batches = total > 0 ? (total + maxCells - 1) / maxCells : 1;
    for (int batch = 0; batch < batches; batch++) {
        if (batch != batchIndex) continue;
        uint8_t buf[NET_PACKET_MAX];
        PktFluidSnapshotHeader header = { fluidSnapshotId[clientId], (uint16_t)batch, (uint16_t)batches, 0 };
        int start = batch * maxCells;
        int count = total - start;
        if (count > maxCells) count = maxCells;
        header.count = (uint8_t)count;
        buf[0] = PKT_FLUID_SNAPSHOT;
        memcpy(buf + 1, &header, sizeof(header));
        int pos = 1 + (int)sizeof(header);
        for (int i = 0; i < count; i++) {
            FluidChange *c = &cells[start + i];
            PktFluidCell cell = { c->x, c->y, c->blockType, c->kind, c->level, c->source ? 1 : 0 };
            memcpy(buf + pos, &cell, sizeof(cell));
            pos += sizeof(cell);
        }
        NetSendTo(clientId, buf, pos, false);
    }
    free(cells);
    return batches;
}

static void FlushFluidChanges(void)
{
    if (GetFluidChangeCount() <= 0) return;
    if (NetIsHost()) {
        int index = 0;
        while (index < GetFluidChangeCount()) {
            uint8_t buf[NET_PACKET_MAX];
            buf[0] = PKT_FLUID_DELTA;
            uint8_t count = 0;
            int pos = 2;
            while (index < GetFluidChangeCount() && count < 174 && pos + (int)sizeof(PktFluidCell) <= NET_PACKET_MAX) {
                FluidChange change;
                if (!GetFluidChange(index++, &change)) continue;
                PktFluidCell cell = { change.x, change.y, change.blockType, change.kind, change.level, change.source ? 1 : 0 };
                memcpy(buf + pos, &cell, sizeof(cell));
                pos += sizeof(cell); count++;
            }
            buf[1] = count;
            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                if (players[r].netControlled) NetSendTo(r, buf, pos, false);
            }
        }
    }
    ClearFluidChanges();
}

GameMode gameMode = GAME_SURVIVAL;
int pendingGameMode = 0;
bool creativeOpen = false;
bool achievementsOpen = false;
bool menuPartyMode = false;
int menuTitleClicks = 0;

// Join Game state
static char joinIpBuf[64] = "127.0.0.1";
static int joinIpLen = 8;
static char joinNameBuf[32] = "Player";
static int joinNameLen = 6;
static int joinInputFocus = 0; // 0=name, 1=ip
static bool joinConnecting = false;
static float joinConnectTimer = 0.0f;

// Network state variables (reset on mode switch)
static bool prevJump[MAX_NET_PLAYERS] = {0};
static float attackCooldownNet[MAX_NET_PLAYERS] = {0};
static float lastInputTime[MAX_NET_PLAYERS] = {0};
static float netTickTimer = 0.0f;
static float inputTickTimer = 0.0f;

// Host mode flag - when true, slot select starts hosting after InitGame
static bool pendingHostMode = false;

//----------------------------------------------------------------------------------
// Screen Transition System
//----------------------------------------------------------------------------------
TransitionState transitionState = TRANSITION_NONE;
float transitionAlpha = 0.0f;
GameState transitionTarget = STATE_MENU;
bool g_resetMenuAnim = false;

void StartTransition(GameState target)
{
    if (transitionState != TRANSITION_NONE) return;
    transitionState = TRANSITION_FADE_OUT;
    transitionAlpha = 0.0f;
    transitionTarget = target;
    if (target == STATE_MENU) g_resetMenuAnim = true;
}

void UpdateTransition(float dt)
{
    if (transitionState == TRANSITION_FADE_OUT) {
        transitionAlpha += dt * 3.0f; // 0.33s to fade out
        if (transitionAlpha >= 1.0f) {
            transitionAlpha = 1.0f;
            gameState = transitionTarget;
            transitionState = TRANSITION_FADE_IN;
        }
    } else if (transitionState == TRANSITION_FADE_IN) {
        transitionAlpha -= dt * 3.0f;
        if (transitionAlpha <= 0.0f) {
            transitionAlpha = 0.0f;
            transitionState = TRANSITION_NONE;
        }
    }
}

void DrawTransition(void)
{
    if (transitionAlpha > 0.01f) {
        unsigned char a = (unsigned char)(transitionAlpha * 255);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, a});
    }
}

bool IsTransitioning(void)
{
    return transitionState != TRANSITION_NONE;
}

// Modified block tracking for world sync
ModifiedBlock modifiedBlocks[MAX_MODIFIED_BLOCKS];
int modifiedBlockCount = 0;

static void RecordBlockChange(int x, int y, uint8_t blockType)
{
    // Update existing entry if this block was modified before
    for (int i = 0; i < modifiedBlockCount; i++) {
        if (modifiedBlocks[i].x == x && modifiedBlocks[i].y == y) {
            modifiedBlocks[i].blockType = blockType;
            return;
        }
    }
    // Add new entry
    if (modifiedBlockCount < MAX_MODIFIED_BLOCKS) {
        modifiedBlocks[modifiedBlockCount].x = (uint16_t)x;
        modifiedBlocks[modifiedBlockCount].y = (uint16_t)y;
        modifiedBlocks[modifiedBlockCount].blockType = blockType;
        modifiedBlockCount++;
    }
}

void NetSyncBlockChange(int x, int y, uint8_t blockType)
{
    // Always record block changes for world sync (even in single-player, in case we host later)
    RecordBlockChange(x, y, blockType);
    if (!NetIsConnected()) return;
    uint8_t buf[NET_PACKET_MAX];
    PktBlockChange bc;
    bc.x = (uint16_t)x;
    bc.y = (uint16_t)y;
    bc.blockType = blockType;
    buf[0] = PKT_BLOCK_CHANGE;
    memcpy(buf + 1, &bc, sizeof(bc));
    if (NetIsHost()) {
        NetSendToAll(buf, 1 + sizeof(bc), true);
    } else {
        NetSendToServer(buf, 1 + sizeof(bc), true);
    }
}

// Send modified block deltas to a newly joined client
static void NetSendWorldToClient(int clientId)
{
    if (modifiedBlockCount == 0) return;
    const int batchSize = (NET_PACKET_MAX - (int)sizeof(PacketHeader) - 1) / (int)sizeof(PktBlockChange);
    uint8_t buf[NET_PACKET_MAX];
    int offset = 0;

    for (int i = 0; i < modifiedBlockCount; i++) {
        PktBlockChange *bc = (PktBlockChange *)(buf + 1 + offset * sizeof(PktBlockChange));
        bc->x = modifiedBlocks[i].x;
        bc->y = modifiedBlocks[i].y;
        bc->blockType = modifiedBlocks[i].blockType;
        offset++;

        if (offset >= batchSize) {
            buf[0] = PKT_BLOCK_CHANGE;
            NetSendTo(clientId, buf, 1 + offset * sizeof(PktBlockChange), true);
            offset = 0;
        }
    }
    if (offset > 0) {
        buf[0] = PKT_BLOCK_CHANGE;
        NetSendTo(clientId, buf, 1 + offset * sizeof(PktBlockChange), true);
    }
}

void InitGame(void)
{
    modifiedBlockCount = 0;
    memset(prevJump, 0, sizeof(prevJump));
    memset(attackCooldownNet, 0, sizeof(attackCooldownNet));
    memset(lastInputTime, 0, sizeof(lastInputTime));
    netTickTimer = 0.0f;
    inputTickTimer = 0.0f;
    confirmDialogActive = false;
    gamePaused = false;
    inventoryOpen = false;
    furnaceOpen = false;
    craftingTableOpen = false;
    chestOpen = false;
    srand((unsigned int)time(NULL));
    if (worldSeed == 0) worldSeed = (unsigned int)rand();

    // Atlas and crafting already initialized in main()
    InitMobs();
    InitParticles();
    InitEntities();
    InitProjectiles();
    InitXpOrbs();
    InitLightMap();
    InitSmeltingRecipes();
    InitTrades();
    InitRedstone();
    InitPrimedTnt();
    InitWater();
    InitLava();

    GetSavePath(selectedSaveSlot, currentSavePath, sizeof(currentSavePath));

    gameMode = GAME_SURVIVAL; // LoadWorld overrides this for v15+ saves
    if (SaveExists(currentSavePath) && LoadWorld(currentSavePath)) {
        // Loaded successfully - day/night and weather restored from save
    } else {
        GenerateWorld(worldSeed);
        gameMode = (GameMode)pendingGameMode;
        InitPlayer();
        InitDayNight();
        InitWeather();
    }

    RebuildPressurePlateList();
    RebuildCropList();
    RecalculateAllLight();
    InitCameraSystem();
    InitChunkTable();
    UpdateChunks();
    player.playerDead = false;

    // Scan for placed cauldrons in the world
    {
        cauldronCount = 0;
        for (int x = 0; x < WORLD_WIDTH && cauldronCount < MAX_CAULDRONS; x++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                if (world[x][y] == BLOCK_CAULDRON) {
                    // Only pick up the first few (topmost if stacked)
                    int idx = cauldronCount++;
                    cauldrons[idx].x = x;
                    cauldrons[idx].y = y;
                    cauldrons[idx].fillLevel = 0;
                }
            }
        }
    }

    // Reset achievement tracking for new game
    totalMobsKilled = 0;
    totalBlocksPlaced = 0;

    // Show tutorial hint for new players
    static bool tutorialShown = false;
    if (!tutorialShown) {
        ShowMessage(S(STR_TUTORIAL_CONTROLS), (Color){180, 200, 220, 255});
        tutorialShown = true;
    }
}

static StringId GetAchString(Achievement ach) {
    switch (ach) {
        case ACH_FIRST_STEPS: return STR_ACH_FIRST_STEPS;
        case ACH_DEEP_DIG: return STR_ACH_DEEP_DIG;
        case ACH_MONSTER_HUNTER: return STR_ACH_MONSTER_HUNTER;
        case ACH_ARCHITECT: return STR_ACH_ARCHITECT;
        case ACH_REDSTONE_ENGINEER: return STR_ACH_REDSTONE_ENGINEER;
        case ACH_COLLECTOR: return STR_ACH_COLLECTOR;
        case ACH_ANGLER: return STR_ACH_ANGLER;
        case ACH_BREEDER: return STR_ACH_BREEDER;
        case ACH_ENCHANTER: return STR_ACH_ENCHANTER;
        case ACH_DEMOLITION: return STR_ACH_DEMOLITION;
        default: return STR_NONE;
    }
}

void UnlockAchievement(Achievement ach) {
    if (!achievements[ach]) {
        achievements[ach] = true;
        StringId msgId = GetAchString(ach);
        if (msgId != STR_NONE) {
            ShowMessage(S(msgId), (Color){255, 215, 0, 255});
            PlaySoundXP();
        }
        // Network sync: tell host/host tells clients
        if (NetIsClient()) {
            uint8_t buf[32];
            buf[0] = PKT_ACHIEVEMENT_UNLOCK;
            PktAchievementUnlock au;
            au.achievementId = (uint8_t)ach;
            memcpy(buf + 1, &au, sizeof(PktAchievementUnlock));
            NetSendToServer(buf, 1 + sizeof(PktAchievementUnlock), true);
        } else if (NetIsHost()) {
            uint8_t buf[32];
            buf[0] = PKT_ACHIEVEMENT_UNLOCK;
            PktAchievementUnlock au;
            au.achievementId = (uint8_t)ach;
            memcpy(buf + 1, &au, sizeof(PktAchievementUnlock));
            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                if (players[r].netControlled) {
                    NetSendTo(r, buf, 1 + sizeof(PktAchievementUnlock), true);
                }
            }
        }
    }
}

static void CheckAchievements(void) {
    // First Steps - craft a wooden pickaxe (check if player has one)
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == TOOL_WOOD_PICKAXE) { UnlockAchievement(ACH_FIRST_STEPS); break; }
    }

    // Deep Dig - reach deep underground
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT) / BLOCK_SIZE;
    if (playerBY >= 240) UnlockAchievement(ACH_DEEP_DIG);

    // Monster Hunter - kill 100 mobs
    if (totalMobsKilled >= 100) UnlockAchievement(ACH_MONSTER_HUNTER);

    // Architect - place 1000 blocks
    if (totalBlocksPlaced >= 1000) UnlockAchievement(ACH_ARCHITECT);

    // Collector - count unique item types in inventory
    {
        bool seen[BLOCK_COUNT] = {0};
        int unique = 0;
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (player.inventory[i] != BLOCK_AIR && !seen[player.inventory[i]]) {
                seen[player.inventory[i]] = true;
                unique++;
            }
        }
        if (unique >= 20) UnlockAchievement(ACH_COLLECTOR);
    }

    // Redstone Engineer - check if any redstone lamp is powered (scan nearby)
    if (!achievements[ACH_REDSTONE_ENGINEER]) {
        int pbx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
        int pby = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
        for (int dx = -20; dx <= 20; dx++) {
            for (int dy = -20; dy <= 20; dy++) {
                if (IsRedstoneLampPowered(pbx + dx, pby + dy)) {
                    UnlockAchievement(ACH_REDSTONE_ENGINEER);
                    dx = 21; dy = 21; // break both loops
                }
            }
        }
    }
}

int menuSelection = 0; // 0=New, 1=Load, 2=Settings, 3=Quit

static void StartGameFromSlot(int slot, bool isNew)
{
    selectedSaveSlot = slot;
    if (isNew) {
        DeleteSaveSlot(slot);
        worldSeed = 0; // Reset seed so InitGame generates a new one if no custom seed
        // Parse seed from input buffer
        if (seedInputLen > 0) {
            // Try numeric seed first
            unsigned int parsedSeed = 0;
            bool isNum = true;
            for (int i = 0; i < seedInputLen; i++) {
                if (seedInputBuf[i] < '0' || seedInputBuf[i] > '9') { isNum = false; break; }
                parsedSeed = parsedSeed * 10 + (seedInputBuf[i] - '0');
            }
            if (isNum && parsedSeed > 0) {
                worldSeed = parsedSeed;
            } else {
                // Hash the text string into a seed
                unsigned int h = 0;
                for (int i = 0; i < seedInputLen; i++) {
                    h = h * 31 + (unsigned char)seedInputBuf[i];
                }
                worldSeed = h > 0 ? h : (unsigned int)time(NULL);
            }
        }
        // If seedInputLen == 0, InitGame will generate a random seed
    }
    InitGame();

    if (pendingHostMode) {
        pendingHostMode = false;
        if (NetHostStart(NET_PORT)) {
            localPlayerId = 0;
            StartTransition(STATE_HOST_WAITING);
        } else {
            StartTransition(STATE_MENU);
        }
    } else {
        StartTransition(STATE_PLAYING);
    }
}

// Try to start a new game on slot; show confirm dialog if slot has data
static void TryNewGameOnSlot(int slot)
{
    SaveSlotInfo info;
    GetSlotInfo(slot, &info);
    if (info.exists) {
        if (pendingHostMode) {
            // Hosting: load existing world directly
            StartGameFromSlot(slot, false);
        } else {
            confirmDialogActive = true;
            confirmDialogSlot = slot;
            confirmDialogMode = 0; // overwrite
            PlaySoundUIClick();
        }
    } else {
        StartGameFromSlot(slot, true);
        seedInputLen = 0;
        seedInputBuf[0] = '\0';
    }
}

static void UpdateSlotSelect(float dt)
{
    (void)dt;

    int slotCount = MAX_SAVE_SLOTS;
    int maxScroll = slotCount - SLOT_VISIBLE;
    if (maxScroll < 0) maxScroll = 0;

    // --- Confirmation dialog input (blocks everything else) ---
    if (confirmDialogActive) {
        if (Win32IsKeyPressed(KEY_ESCAPE) || Win32IsKeyPressed(KEY_N)) {
            confirmDialogActive = false;
            PlaySoundUIClick();
            return;
        }
        if (Win32IsKeyPressed(KEY_Y) || Win32IsKeyPressed(KEY_ENTER)) {
            if (confirmDialogMode == 0) {
                // Overwrite: start new game
                confirmDialogActive = false;
                StartGameFromSlot(confirmDialogSlot, true);
                seedInputLen = 0;
                seedInputBuf[0] = '\0';
            } else {
                // Delete save
                DeleteSaveSlot(confirmDialogSlot);
                confirmDialogActive = false;
                PlaySoundUIClick();
            }
            return;
        }
        // Mouse clicks on dialog buttons
        {
            Vector2 mouse = Win32GetMousePosition();
            int dlgW = 340, dlgH = 160;
            int dlgX = (SCREEN_WIDTH - dlgW) / 2;
            int dlgY = (SCREEN_HEIGHT - dlgH) / 2;
            int btnW = 130, btnH = 34;
            int btnY = dlgY + dlgH - 50;
            Rectangle yesBtn = { (float)(dlgX + 30), (float)btnY, (float)btnW, (float)btnH };
            Rectangle noBtn = { (float)(dlgX + dlgW - btnW - 30), (float)btnY, (float)btnW, (float)btnH };

            if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mouse, yesBtn)) {
                    if (confirmDialogMode == 0) {
                        confirmDialogActive = false;
                        StartGameFromSlot(confirmDialogSlot, true);
                        seedInputLen = 0;
                        seedInputBuf[0] = '\0';
                    } else {
                        DeleteSaveSlot(confirmDialogSlot);
                        confirmDialogActive = false;
                        PlaySoundUIClick();
                    }
                    return;
                }
                if (CheckCollisionPointRec(mouse, noBtn)) {
                    confirmDialogActive = false;
                    PlaySoundUIClick();
                    return;
                }
            }
        }
        return; // Block other input while dialog is open
    }

    // Seed input (only in new game mode) - handle before navigation
    if (slotSelectMode == 0) {
        int key = Win32GetCharPressed();
        while (key > 0) {
            if (seedInputLen < 20 && key >= 32 && key < 127) {
                seedInputBuf[seedInputLen++] = (char)key;
                seedInputBuf[seedInputLen] = '\0';
            }
            key = Win32GetCharPressed();
        }
        if (Win32IsKeyPressed(KEY_BACKSPACE) && seedInputLen > 0) {
            seedInputLen--;
            seedInputBuf[seedInputLen] = '\0';
        }
    }

    // Game mode toggle (new game only): Survival / Creative (same row as seed box)
    if (slotSelectMode == 0) {
        int seedBoxW = 240;
        int seedBoxX = (SCREEN_WIDTH - seedBoxW) / 2;
        int modeX = seedBoxX + seedBoxW + 20;
        Rectangle survBtn = { (float)(modeX + 50), 126.0f, 88.0f, 22.0f };
        Rectangle creatBtn = { (float)(modeX + 50 + 88 + 6), 126.0f, 88.0f, 22.0f };
        Vector2 mmouse = Win32GetMousePosition();
        if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mmouse, survBtn)) {
                pendingGameMode = GAME_SURVIVAL;
                PlaySoundUIClick();
            } else if (CheckCollisionPointRec(mmouse, creatBtn)) {
                pendingGameMode = GAME_CREATIVE;
                PlaySoundUIClick();
            }
        }
        if (Win32IsKeyPressed(KEY_LEFT) || Win32IsKeyPressed(KEY_RIGHT)) {
            pendingGameMode = !pendingGameMode;
            PlaySoundUIClick();
        }
    }

    // Keyboard navigation
    if (Win32IsKeyPressed(KEY_DOWN) || Win32IsKeyPressed(KEY_S)) {
        menuSelection++;
        if (menuSelection >= slotCount) menuSelection = 0;
    }
    if (Win32IsKeyPressed(KEY_UP) || Win32IsKeyPressed(KEY_W)) {
        menuSelection--;
        if (menuSelection < 0) menuSelection = slotCount - 1;
    }

    // Keep selection visible
    if (menuSelection < slotScrollOffset) slotScrollOffset = menuSelection;
    if (menuSelection >= slotScrollOffset + SLOT_VISIBLE) slotScrollOffset = menuSelection - SLOT_VISIBLE + 1;
    if (slotScrollOffset < 0) slotScrollOffset = 0;
    if (slotScrollOffset > maxScroll) slotScrollOffset = maxScroll;

    // ESC to go back
    if (Win32IsKeyPressed(KEY_ESCAPE)) {
        pendingHostMode = false;
        pendingGameMode = GAME_SURVIVAL;
        StartTransition(STATE_MENU);
        menuSelection = 0;
        seedInputLen = 0;
        seedInputBuf[0] = '\0';
        PlaySoundUIClick();
        return;
    }

    // Delete key: delete selected slot's save
    if (Win32IsKeyPressed(KEY_DELETE)) {
        SaveSlotInfo info;
        if (GetSlotInfo(menuSelection, &info) && info.exists) {
            confirmDialogActive = true;
            confirmDialogSlot = menuSelection;
            confirmDialogMode = 1; // delete
            PlaySoundUIClick();
        }
        return;
    }

    // Confirm with Enter/Space
    if (Win32IsKeyPressed(KEY_ENTER) || Win32IsKeyPressed(KEY_SPACE)) {
        if (slotSelectMode == 0) {
            TryNewGameOnSlot(menuSelection);
        } else {
            SaveSlotInfo info;
            if (GetSlotInfo(menuSelection, &info) && info.exists) {
                StartGameFromSlot(menuSelection, false);
                return;
            }
        }
    }

    // Mouse hover + click
    {
        Vector2 mouse = Win32GetMousePosition();
        int slotW = 340, slotH = 80;
        int slotX = (SCREEN_WIDTH - slotW) / 2;
        int slotY = (slotSelectMode == 0) ? 185 : 160;
        int spacing = 96;

        // Scroll wheel on the slot area
        Rectangle slotArea = { (float)slotX, (float)slotY, (float)slotW, (float)(SLOT_VISIBLE * spacing) };
        if (CheckCollisionPointRec(mouse, slotArea)) {
            int wheel = (int)Win32GetMouseWheelMove();
            if (wheel != 0) {
                slotScrollOffset -= wheel;
                if (slotScrollOffset < 0) slotScrollOffset = 0;
                if (slotScrollOffset > maxScroll) slotScrollOffset = maxScroll;
            }
        }

        for (int vi = 0; vi < SLOT_VISIBLE; vi++) {
            int i = vi + slotScrollOffset;
            if (i >= slotCount) break;
            Rectangle slotRect = { (float)slotX, (float)(slotY + vi * spacing), (float)slotW, (float)slotH };
            bool hover = CheckCollisionPointRec(mouse, slotRect);

            if (hover) {
                menuSelection = i;
                if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (slotSelectMode == 0) {
                        TryNewGameOnSlot(i);
                    } else {
                        SaveSlotInfo info;
                        if (GetSlotInfo(i, &info) && info.exists) {
                            StartGameFromSlot(i, false);
                            return;
                        }
                    }
                }
            }
        }

        // Back button
        int backBtnW = 160;
        int backBtnH = 38;
        int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
        int backBtnY = slotY + SLOT_VISIBLE * spacing + 10;
        Rectangle backBtn = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
        if (CheckCollisionPointRec(mouse, backBtn) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            StartTransition(STATE_MENU);
            menuSelection = 0;
            pendingGameMode = GAME_SURVIVAL;
            seedInputLen = 0;
            seedInputBuf[0] = '\0';
            PlaySoundUIClick();
        }
    }
}

static void UpdateMainMenu(float dt)
{
    (void)dt;

    int btnCount = 6; // New, Load, Host, Join, Settings, Quit

    // ============================================================
    // Easter eggs
    // ============================================================
    {
        // Konami code: Up Up Down Down Left Right Left Right B A -> party mode
        static int konamiIdx = 0;
        static const int konamiSeq[10] = { KEY_UP, KEY_UP, KEY_DOWN, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_LEFT, KEY_RIGHT, KEY_B, KEY_A };
        int kkey = -1;
        if (Win32IsKeyPressed(KEY_UP)) kkey = KEY_UP;
        else if (Win32IsKeyPressed(KEY_DOWN)) kkey = KEY_DOWN;
        else if (Win32IsKeyPressed(KEY_LEFT)) kkey = KEY_LEFT;
        else if (Win32IsKeyPressed(KEY_RIGHT)) kkey = KEY_RIGHT;
        else if (Win32IsKeyPressed(KEY_B)) kkey = KEY_B;
        else if (Win32IsKeyPressed(KEY_A)) kkey = KEY_A;
        if (kkey >= 0) {
            if (kkey == konamiSeq[konamiIdx]) {
                konamiIdx++;
                if (konamiIdx >= 10) {
                    konamiIdx = 0;
                    menuPartyMode = !menuPartyMode;
                    PlaySoundUIClick();
                    ShowMessage(menuPartyMode ? S(STR_EGG_KONAMI_ON) : S(STR_EGG_KONAMI_OFF),
                                (Color){255, 205, 90, 255});
                }
            } else if (kkey == konamiSeq[0]) {
                konamiIdx = 1;
            } else {
                konamiIdx = 0;
            }
        }

        // P key: random fun fact
        if (Win32IsKeyPressed(KEY_P)) {
            int fact = rand() % 3;
            const char *f = (fact == 0) ? S(STR_EGG_FACT1) : (fact == 1 ? S(STR_EGG_FACT2) : S(STR_EGG_FACT3));
            ShowMessage(f, (Color){120, 200, 235, 255});
            PlaySoundUIClick();
        }
    }

    // Keyboard navigation
    if (Win32IsKeyPressed(KEY_DOWN) || Win32IsKeyPressed(KEY_S)) {
        menuSelection++;
        if (menuSelection >= btnCount) menuSelection = 0;
    }
    if (Win32IsKeyPressed(KEY_UP) || Win32IsKeyPressed(KEY_W)) {
        menuSelection--;
        if (menuSelection < 0) menuSelection = btnCount - 1;
    }

    // Confirm with Enter/Space
    if (Win32IsKeyPressed(KEY_ENTER) || Win32IsKeyPressed(KEY_SPACE)) {
        if (menuSelection == 0) {
            // New Game -> slot select
            slotSelectMode = 0;
            pendingGameMode = GAME_SURVIVAL;
            menuSelection = 0;
            StartTransition(STATE_SLOT_SELECT);
            return;
        } else if (menuSelection == 1) {
            // Load Game -> slot select (load mode)
            slotSelectMode = 1;
            menuSelection = 0;
            bool anySlot = false;
            for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                SaveSlotInfo info;
                if (GetSlotInfo(i, &info) && info.exists) { anySlot = true; break; }
            }
            if (anySlot) {
                StartTransition(STATE_SLOT_SELECT);
            }
            return;
        } else if (menuSelection == 2) {
            // Host Game -> slot select (need world first)
            pendingHostMode = true;
            slotSelectMode = 0;
            pendingGameMode = GAME_SURVIVAL;
            menuSelection = 0;
            StartTransition(STATE_SLOT_SELECT);
            return;
        } else if (menuSelection == 3) {
            // Join Game
            joinConnecting = false;
            joinIpLen = 8;
            memcpy(joinIpBuf, "127.0.0.1", 9);
            StartTransition(STATE_JOIN_GAME);
            return;
        } else if (menuSelection == 4) {
            StartTransition(STATE_SETTINGS);
            return;
        } else if (menuSelection == 5) {
            CloseWindow();
            exit(0);
        }
    }

    // Mouse hover + click
    {
        Vector2 mouse = Win32GetMousePosition();
        int btnW = 280, btnH = 46;
        int btnX = (SCREEN_WIDTH - btnW) / 2;
        int btnY = 225;
        int spacing = 66;

        Rectangle btns[6];
        for (int i = 0; i < 6; i++) {
            btns[i] = (Rectangle){ (float)btnX, (float)(btnY + i * spacing), (float)btnW, (float)btnH };
        }

        // Hover highlight
        for (int i = 0; i < 6; i++) {
            bool hasSave = false;
            for (int s = 0; s < MAX_SAVE_SLOTS; s++) {
                SaveSlotInfo info;
                if (GetSlotInfo(s, &info) && info.exists) { hasSave = true; break; }
            }
            bool enabled = (i == 0) || (i == 1 && hasSave) || (i == 2) || (i == 3) || (i == 4) || (i == 5);
            if (enabled && CheckCollisionPointRec(mouse, btns[i])) {
                menuSelection = i;
                break;
            }
        }

        // Play one click sound for the visible button that was activated.
        if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            bool hasSave = false;
            for (int s = 0; s < MAX_SAVE_SLOTS; s++) {
                SaveSlotInfo info;
                if (GetSlotInfo(s, &info) && info.exists) { hasSave = true; break; }
            }
            for (int i = 0; i < btnCount; i++) {
                bool enabled = (i != 1 || hasSave);
                Rectangle button = btns[i];
                if (enabled && CheckCollisionPointRec(mouse, button)) {
                    PlaySoundUIClick();
                    break;
                }
            }
        }

        // Click
        if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // Easter egg: poke the title 5 times
            if (mouse.y >= 55 && mouse.y <= 140 && mouse.x >= (SCREEN_WIDTH - 400) / 2 && mouse.x <= (SCREEN_WIDTH + 400) / 2) {
                menuTitleClicks++;
                if (menuTitleClicks >= 5) {
                    menuTitleClicks = 0;
                    ShowMessage(S(STR_EGG_TITLE_CLICK), (Color){255, 190, 120, 255});
                    PlaySoundUIClick();
                }
            }
            if (CheckCollisionPointRec(mouse, btns[0])) {
                slotSelectMode = 0;
                pendingGameMode = GAME_SURVIVAL;
                menuSelection = 0;
                StartTransition(STATE_SLOT_SELECT);
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[1])) {
                slotSelectMode = 1;
                menuSelection = 0;
                bool anySlot = false;
                for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                    SaveSlotInfo info;
                    if (GetSlotInfo(i, &info) && info.exists) { anySlot = true; break; }
                }
                if (anySlot) {
                    StartTransition(STATE_SLOT_SELECT);
                }
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[2])) {
                pendingHostMode = true;
                slotSelectMode = 0;
                pendingGameMode = GAME_SURVIVAL;
                menuSelection = 0;
                StartTransition(STATE_SLOT_SELECT);
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[3])) {
                joinConnecting = false;
                joinIpLen = 8;
                memcpy(joinIpBuf, "127.0.0.1", 9);
                StartTransition(STATE_JOIN_GAME);
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[4])) {
                StartTransition(STATE_SETTINGS);
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[5])) {
                CloseWindow();
                exit(0);
            }
        }
    }
}

// Pickup items and sync over network
static void PickupAndSyncItems(float px, float py, int playerId)
{
    bool wasActive[MAX_ENTITIES];
    for (int i = 0; i < MAX_ENTITIES; i++) wasActive[i] = entities[i].active;

    PickupNearbyItems(px, py);

    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (wasActive[i] && !entities[i].active) {
            uint8_t buf[NET_PACKET_MAX];
            PktEntityPickup ep;
            ep.entityIndex = (uint16_t)i;
            ep.playerId = (uint8_t)playerId;
            buf[0] = PKT_ENTITY_PICKUP;
            memcpy(buf + 1, &ep, sizeof(PktEntityPickup));
            if (NetIsHost()) {
                NetSendToAll(buf, 1 + sizeof(PktEntityPickup), false);
            } else if (NetIsClient()) {
                NetSendToServer(buf, 1 + sizeof(PktEntityPickup), false);
            }
        }
    }
}

// Multi-furnace helpers
int FindFurnace(int x, int y)
{
    for (int i = 0; i < furnaceCount; i++) {
        if (furnaces[i].x == x && furnaces[i].y == y) return i;
    }
    return -1;
}

int GetOrCreateFurnace(int x, int y)
{
    int idx = FindFurnace(x, y);
    if (idx >= 0) return idx;
    if (furnaceCount >= MAX_FURNACES) return -1;
    idx = furnaceCount++;
    memset(&furnaces[idx], 0, sizeof(FurnaceData));
    furnaces[idx].x = x;
    furnaces[idx].y = y;
    return idx;
}

void SyncFurnaceToActive(int idx)
{
    if (idx < 0 || idx >= furnaceCount) return;
    FurnaceData *f = &furnaces[idx];
    furnaceBlockX = f->x; furnaceBlockY = f->y;
    furnaceFuel = f->fuel; furnaceFuelCount = f->fuelCount;
    furnaceInput = f->input; furnaceInputCount = f->inputCount;
    furnaceOutput = f->output; furnaceOutputCount = f->outputCount;
    furnaceProgress = f->progress;
    furnaceFuelBurn = f->fuelBurn;
    furnaceFuelBurnMax = f->fuelBurnMax;
}

void SyncActiveToFurnace(int idx)
{
    if (idx < 0 || idx >= furnaceCount) return;
    FurnaceData *f = &furnaces[idx];
    f->fuel = furnaceFuel; f->fuelCount = furnaceFuelCount;
    f->input = furnaceInput; f->inputCount = furnaceInputCount;
    f->output = furnaceOutput; f->outputCount = furnaceOutputCount;
    f->progress = furnaceProgress;
    f->fuelBurn = furnaceFuelBurn;
    f->fuelBurnMax = furnaceFuelBurnMax;
}

// Return furnace items to inventory when closing

//----------------------------------------------------------------------------------
// Chest multiplayer sync helpers
// In multiplayer the host owns chest contents; clients request/open/modify via packets.
//----------------------------------------------------------------------------------
static int ChestIndexAt(int bx, int by)
{
    for (int i = 0; i < chestCount; i++) {
        if (chestData[i].x == bx && chestData[i].y == by) return i;
    }
    return -1;
}

static int GetOrCreateChestIndex(int bx, int by)
{
    int idx = ChestIndexAt(bx, by);
    if (idx >= 0) return idx;
    if (chestCount >= MAX_CHESTS) return -1;
    idx = chestCount++;
    memset(&chestData[idx], 0, sizeof(ChestData));
    chestData[idx].x = bx;
    chestData[idx].y = by;
    for (int s = 0; s < CHEST_SLOTS; s++) {
        chestData[idx].items[s] = BLOCK_AIR;
        chestData[idx].counts[s] = 0;
        chestData[idx].durability[s] = 0;
        chestData[idx].enchantments[s] = 0;
    }
    return idx;
}

static void PackChestSync(PktChestSync *pkt, int idx)
{
    pkt->x = (int16_t)chestData[idx].x;
    pkt->y = (int16_t)chestData[idx].y;
    memcpy(pkt->items, chestData[idx].items, sizeof(pkt->items));
    memcpy(pkt->counts, chestData[idx].counts, sizeof(pkt->counts));
    memcpy(pkt->durability, chestData[idx].durability, sizeof(pkt->durability));
    memcpy(pkt->enchantments, chestData[idx].enchantments, sizeof(pkt->enchantments));
}

static void ApplyChestSync(const PktChestSync *pkt)
{
    int idx = GetOrCreateChestIndex((int)pkt->x, (int)pkt->y);
    if (idx < 0) return;
    memcpy(chestData[idx].items, pkt->items, sizeof(chestData[idx].items));
    memcpy(chestData[idx].counts, pkt->counts, sizeof(chestData[idx].counts));
    memcpy(chestData[idx].durability, pkt->durability, sizeof(chestData[idx].durability));
    memcpy(chestData[idx].enchantments, pkt->enchantments, sizeof(chestData[idx].enchantments));
}

void RequestOpenChest(int bx, int by)
{
    chestBlockX = bx;
    chestBlockY = by;
    if (!NetIsConnected()) {
        // Single-player: open immediately (chest entry is created on demand in UI)
        chestOpen = true;
        inventoryOpen = true;
        gamePaused = false;
        PlaySoundCraft();
        return;
    }
    if (NetIsHost()) {
        // Host already owns the data; make sure the chest entry exists and open.
        GetOrCreateChestIndex(bx, by);
        chestOpen = true;
        inventoryOpen = true;
        gamePaused = false;
        PlaySoundCraft();
        return;
    }
    // Client: ask host for chest contents; UI will open when PKT_CHEST_SYNC arrives.
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_CHEST_OPEN;
    PktChestOpen po;
    po.x = (int16_t)bx;
    po.y = (int16_t)by;
    memcpy(buf + 1, &po, sizeof(po));
    NetSendToServer(buf, 1 + sizeof(po), true);
}

void SyncChestToHost(int chestIdx)
{
    if (chestIdx < 0 || chestIdx >= chestCount) return;
    if (!NetIsClient()) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_CHEST_SYNC;
    PktChestSync pkt;
    PackChestSync(&pkt, chestIdx);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    NetSendToServer(buf, 1 + sizeof(pkt), true);
}

void SyncChestToAll(int chestIdx)
{
    if (chestIdx < 0 || chestIdx >= chestCount) return;
    if (!NetIsHost()) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_CHEST_SYNC;
    PktChestSync pkt;
    PackChestSync(&pkt, chestIdx);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    for (int r = 1; r < NET_MAX_PLAYERS; r++) {
        if (players[r].netControlled) {
            NetSendTo(r, buf, 1 + sizeof(pkt), true);
        }
    }
}

void CloseChestNetwork(void)
{
    if (NetIsClient()) {
        uint8_t buf[NET_PACKET_MAX];
        buf[0] = PKT_CHEST_CLOSE;
        PktChestOpen po;
        po.x = (int16_t)chestBlockX;
        po.y = (int16_t)chestBlockY;
        memcpy(buf + 1, &po, sizeof(po));
        NetSendToServer(buf, 1 + sizeof(po), true);
    }
}

//----------------------------------------------------------------------------------
// Furnace multiplayer sync helpers
//----------------------------------------------------------------------------------
static void PackFurnaceSync(PktFurnaceSync *pkt, int idx)
{
    FurnaceData *f = &furnaces[idx];
    pkt->x = (int16_t)f->x;
    pkt->y = (int16_t)f->y;
    pkt->fuel = f->fuel; pkt->fuelCount = f->fuelCount;
    pkt->input = f->input; pkt->inputCount = f->inputCount;
    pkt->output = f->output; pkt->outputCount = f->outputCount;
    pkt->progress = f->progress;
    pkt->fuelBurn = f->fuelBurn;
    pkt->fuelBurnMax = f->fuelBurnMax;
}

static void ApplyFurnaceSync(const PktFurnaceSync *pkt)
{
    int idx = GetOrCreateFurnace((int)pkt->x, (int)pkt->y);
    if (idx < 0) return;
    FurnaceData *f = &furnaces[idx];
    f->fuel = pkt->fuel; f->fuelCount = pkt->fuelCount;
    f->input = pkt->input; f->inputCount = pkt->inputCount;
    f->output = pkt->output; f->outputCount = pkt->outputCount;
    f->progress = pkt->progress;
    f->fuelBurn = pkt->fuelBurn;
    f->fuelBurnMax = pkt->fuelBurnMax;
    if (activeFurnace == idx) SyncFurnaceToActive(idx);
}

void RequestOpenFurnace(int bx, int by)
{
    furnaceBlockX = bx; furnaceBlockY = by;
    if (!NetIsConnected()) {
        activeFurnace = GetOrCreateFurnace(bx, by);
        if (activeFurnace >= 0) SyncFurnaceToActive(activeFurnace);
        furnaceOpen = true; inventoryOpen = true; gamePaused = false;
        PlaySoundCraft();
        return;
    }
    if (NetIsHost()) {
        activeFurnace = GetOrCreateFurnace(bx, by);
        if (activeFurnace >= 0) SyncFurnaceToActive(activeFurnace);
        furnaceOpen = true; inventoryOpen = true; gamePaused = false;
        PlaySoundCraft();
        return;
    }
    // Client: ask host for furnace contents; UI opens when PKT_FURNACE_SYNC arrives.
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_FURNACE_OPEN;
    PktFurnaceOpen po;
    po.x = (int16_t)bx; po.y = (int16_t)by;
    memcpy(buf + 1, &po, sizeof(po));
    NetSendToServer(buf, 1 + sizeof(po), true);
}

void SyncFurnaceToHost(void)
{
    if (!NetIsClient()) return;
    if (activeFurnace < 0 || activeFurnace >= furnaceCount) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_FURNACE_SYNC;
    PktFurnaceSync pkt;
    PackFurnaceSync(&pkt, activeFurnace);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    NetSendToServer(buf, 1 + sizeof(pkt), true);
}

void SyncFurnaceToAll(void)
{
    if (!NetIsHost()) return;
    if (activeFurnace < 0 || activeFurnace >= furnaceCount) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_FURNACE_SYNC;
    PktFurnaceSync pkt;
    PackFurnaceSync(&pkt, activeFurnace);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    for (int r = 1; r < NET_MAX_PLAYERS; r++) {
        if (players[r].netControlled) {
            NetSendTo(r, buf, 1 + sizeof(pkt), true);
        }
    }
}

void CloseFurnaceNetwork(void)
{
    if (NetIsClient()) {
        uint8_t buf[NET_PACKET_MAX];
        buf[0] = PKT_FURNACE_CLOSE;
        PktFurnaceOpen po;
        po.x = (int16_t)furnaceBlockX; po.y = (int16_t)furnaceBlockY;
        memcpy(buf + 1, &po, sizeof(po));
        NetSendToServer(buf, 1 + sizeof(po), true);
    }
}

//----------------------------------------------------------------------------------
// Inventory multiplayer sync helpers
//----------------------------------------------------------------------------------
static void PackInventorySync(PktInventorySync *pkt, int playerId)
{
    if (playerId < 0 || playerId >= MAX_NET_PLAYERS) return;
    Player *p = &players[playerId];
    pkt->playerId = (uint8_t)playerId;
    memcpy(pkt->inventory, p->inventory, sizeof(pkt->inventory));
    memcpy(pkt->inventoryCount, p->inventoryCount, sizeof(pkt->inventoryCount));
    memcpy(pkt->toolDurability, p->toolDurability, sizeof(pkt->toolDurability));
    memcpy(pkt->itemEnchantments, p->itemEnchantments, sizeof(pkt->itemEnchantments));
    memcpy(pkt->armor, p->armor, sizeof(pkt->armor));
    memcpy(pkt->armorDurability, p->armorDurability, sizeof(pkt->armorDurability));
    memcpy(pkt->armorEnchantments, p->armorEnchantments, sizeof(pkt->armorEnchantments));
}

static void ApplyInventorySync(const PktInventorySync *pkt, int playerId)
{
    if (playerId < 0 || playerId >= MAX_NET_PLAYERS) return;
    Player *p = &players[playerId];
    memcpy(p->inventory, pkt->inventory, sizeof(p->inventory));
    memcpy(p->inventoryCount, pkt->inventoryCount, sizeof(p->inventoryCount));
    memcpy(p->toolDurability, pkt->toolDurability, sizeof(p->toolDurability));
    memcpy(p->itemEnchantments, pkt->itemEnchantments, sizeof(p->itemEnchantments));
    memcpy(p->armor, pkt->armor, sizeof(p->armor));
    memcpy(p->armorDurability, pkt->armorDurability, sizeof(p->armorDurability));
    memcpy(p->armorEnchantments, pkt->armorEnchantments, sizeof(p->armorEnchantments));
}

void SyncInventoryToHost(void)
{
    if (!NetIsClient()) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_INVENTORY_SYNC;
    PktInventorySync pkt;
    PackInventorySync(&pkt, localPlayerId);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    NetSendToServer(buf, 1 + sizeof(pkt), true);
}

void SyncInventoryToAll(void)
{
    if (!NetIsHost()) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_INVENTORY_SYNC;
    PktInventorySync pkt;
    PackInventorySync(&pkt, localPlayerId);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    for (int r = 1; r < NET_MAX_PLAYERS; r++) {
        if (players[r].netControlled) {
            NetSendTo(r, buf, 1 + sizeof(pkt), true);
        }
    }
}

// Authoritative remote block placement. Returns true if the placement was accepted.
static bool TryPlaceBlockRemote(Player *p, int bx, int by)
{
    if (p->playerDead) return false;
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;

    // Range check
    float cx = bx * BLOCK_SIZE + BLOCK_SIZE * 0.5f;
    float cy = by * BLOCK_SIZE + BLOCK_SIZE * 0.5f;
    float px = p->position.x + PLAYER_WIDTH * 0.5f;
    float py = p->position.y + PLAYER_HEIGHT * 0.5f;
    float dx = cx - px, dy = cy - py;
    if (sqrtf(dx * dx + dy * dy) > PLACE_RANGE * BLOCK_SIZE) return false;

    int slot = p->selectedSlot;
    if (slot < 0 || slot >= INVENTORY_SLOTS) return false;

    uint8_t item = p->inventory[slot];
    if (item == BLOCK_AIR) return false;
    // Creative mode: infinite blocks, no need to have a stack in the slot
    if (p->inventoryCount[slot] <= 0 && gameMode != GAME_CREATIVE) return false;

    // Bucket special cases
    if (item == ITEM_WATER_BUCKET) {
        if (world[bx][by] != BLOCK_AIR && world[bx][by] != BLOCK_WATER) return false;
        SetWaterSource(bx, by);
        if (gameMode != GAME_CREATIVE) {
            if (p->inventoryCount[slot] > 1) p->inventoryCount[slot]--;
            else { p->inventory[slot] = ITEM_BUCKET; p->inventoryCount[slot] = 1; }
        }
        NetSyncBlockChange(bx, by, BLOCK_WATER);
        return true;
    }
    if (item == ITEM_LAVA_BUCKET) {
        if (world[bx][by] != BLOCK_AIR && world[bx][by] != BLOCK_WATER) return false;
        if (world[bx][by] == BLOCK_WATER) RemoveWaterAt(bx, by);
        SetLavaSource(bx, by);
        if (gameMode != GAME_CREATIVE) {
            if (p->inventoryCount[slot] > 1) p->inventoryCount[slot]--;
            else { p->inventory[slot] = ITEM_BUCKET; p->inventoryCount[slot] = 1; }
        }
        NetSyncBlockChange(bx, by, BLOCK_LAVA);
        return true;
    }

    // Must be a placeable block
    if (IsTool((BlockType)item) || IsFood((BlockType)item) || IsArmor((BlockType)item)) return false;
    if (item >= BLOCK_COUNT) return false;
    if (!blockInfo[item].breakable) return false;
    if (world[bx][by] != BLOCK_AIR && world[bx][by] != BLOCK_WATER) return false;

    // Player collision check
    float bLeft = bx * BLOCK_SIZE;
    float bRight = bLeft + BLOCK_SIZE;
    float bTop = by * BLOCK_SIZE;
    float bBottom = bTop + BLOCK_SIZE;
    float pLeft = p->position.x;
    float pRight = pLeft + PLAYER_WIDTH;
    float pTop = p->position.y;
    float pBottom = pTop + PLAYER_HEIGHT;
    if (pRight > bLeft && pLeft < bRight && pBottom > bTop && pTop < bBottom)
        return false;

    // Place it
    bool wasWater = (world[bx][by] == BLOCK_WATER);
    if (wasWater) RemoveWaterAt(bx, by);
    world[bx][by] = item;

    // Consume item (creative mode: infinite blocks)
    if (gameMode != GAME_CREATIVE) {
        p->inventoryCount[slot]--;
        if (p->inventoryCount[slot] <= 0) {
            p->inventory[slot] = BLOCK_AIR;
            p->inventoryCount[slot] = 0;
        }
    }

    // Side effects
    if (item == BLOCK_STONE_PRESSURE_PLATE) RegisterPressurePlate(bx, by);
    UpdateLightAt(bx, by);
    InvalidateChunkAt(bx, by);
    if (bx % CHUNK_SIZE == 0) InvalidateChunkAt(bx - 1, by);
    if (bx % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(bx + 1, by);
    UpdateRedstoneAt(bx, by);

    // Falling blocks
    if (IsGravityBlock(item)) {
        world[bx][by] = BLOCK_AIR;
        int landY = by;
        while (landY < WORLD_HEIGHT - 1 &&
               (world[bx][landY + 1] == BLOCK_AIR || world[bx][landY + 1] == BLOCK_WATER))
            landY++;
        world[bx][landY] = item;
        NetSyncBlockChange(bx, by, BLOCK_AIR);
        NetSyncBlockChange(bx, landY, item);
        UpdateLightAt(bx, landY);
        InvalidateChunkAt(bx, landY);
    } else {
        NetSyncBlockChange(bx, by, item);
    }

    totalBlocksPlaced++;
    return true;
}

// Authoritative remote item use (E key / right-click use). Returns true if an item was consumed.
// Handles eating, bucket fill/empty, hoe tilling, seed planting, and mob breeding.
static bool TryUseItemRemote(Player *p, int bx, int by, float cursorX, float cursorY)
{
    if (p->playerDead) return false;
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;

    // Range check against the cursor target
    float px = p->position.x + PLAYER_WIDTH * 0.5f;
    float py = p->position.y + PLAYER_HEIGHT * 0.5f;
    float dx = cursorX - px, dy = cursorY - py;
    if (sqrtf(dx * dx + dy * dy) > BREAK_RANGE * BLOCK_SIZE) return false;

    int slot = p->selectedSlot;
    if (slot < 0 || slot >= INVENTORY_SLOTS) return false;

    uint8_t item = p->inventory[slot];
    if (item == BLOCK_AIR || p->inventoryCount[slot] <= 0) return false;

    // Eat food
    if (IsFood((BlockType)item) && p->hunger < MAX_HUNGER) {
        int foodVal = GetFoodValue((BlockType)item);
        p->hunger += foodVal;
        if (p->hunger > MAX_HUNGER) p->hunger = MAX_HUNGER;
        p->inventoryCount[slot]--;
        if (p->inventoryCount[slot] <= 0) {
            p->inventory[slot] = BLOCK_AIR;
            p->inventoryCount[slot] = 0;
        }
        return true;
    }

    // Bucket use
    if (item == ITEM_WATER_BUCKET) {
        if (world[bx][by] == BLOCK_AIR || world[bx][by] == BLOCK_WATER) {
            SetWaterSource(bx, by);
            if (gameMode != GAME_CREATIVE) {
                if (p->inventoryCount[slot] > 1) p->inventoryCount[slot]--;
                else { p->inventory[slot] = ITEM_BUCKET; p->inventoryCount[slot] = 1; }
            }
            NetSyncBlockChange(bx, by, BLOCK_WATER);
            UpdateLightAt(bx, by);
            InvalidateChunkAt(bx, by);
            return true;
        }
    }
    if (item == ITEM_LAVA_BUCKET) {
        if (world[bx][by] == BLOCK_AIR || world[bx][by] == BLOCK_WATER) {
            if (world[bx][by] == BLOCK_WATER) RemoveWaterAt(bx, by);
            SetLavaSource(bx, by);
            if (gameMode != GAME_CREATIVE) {
                if (p->inventoryCount[slot] > 1) p->inventoryCount[slot]--;
                else { p->inventory[slot] = ITEM_BUCKET; p->inventoryCount[slot] = 1; }
            }
            NetSyncBlockChange(bx, by, BLOCK_LAVA);
            UpdateLightAt(bx, by);
            InvalidateChunkAt(bx, by);
            return true;
        }
    }
    if (item == ITEM_BUCKET) {
        if (world[bx][by] == BLOCK_WATER) {
            RemoveWaterAt(bx, by);
            if (gameMode != GAME_CREATIVE) p->inventory[slot] = ITEM_WATER_BUCKET;
            NetSyncBlockChange(bx, by, BLOCK_AIR);
            UpdateLightAt(bx, by);
            InvalidateChunkAt(bx, by);
            return true;
        }
        if (world[bx][by] == BLOCK_LAVA) {
            RemoveLavaAt(bx, by);
            if (gameMode != GAME_CREATIVE) p->inventory[slot] = ITEM_LAVA_BUCKET;
            NetSyncBlockChange(bx, by, BLOCK_AIR);
            UpdateLightAt(bx, by);
            InvalidateChunkAt(bx, by);
            return true;
        }
    }

    // Hoe: till dirt/grass into farmland
    if (IsHoe((BlockType)item)) {
        if (world[bx][by] == BLOCK_DIRT || world[bx][by] == BLOCK_GRASS) {
            world[bx][by] = BLOCK_FARMLAND;
            NetSyncBlockChange(bx, by, BLOCK_FARMLAND);
            UpdateLightAt(bx, by);
            InvalidateChunkAt(bx, by);
            if (gameMode != GAME_CREATIVE) { // Creative: tools never wear
                p->toolDurability[slot]--;
                if (p->toolDurability[slot] <= 0) {
                    p->inventory[slot] = BLOCK_AIR;
                    p->inventoryCount[slot] = 0;
                    p->toolDurability[slot] = 0;
                    p->itemEnchantments[slot] = 0;
                }
            }
            return true;
        }
    }

    // Seeds: plant on farmland
    if (item == ITEM_WHEAT_SEEDS) {
        if (world[bx][by] == BLOCK_FARMLAND && by > 0 && world[bx][by - 1] == BLOCK_AIR) {
            world[bx][by - 1] = BLOCK_CROPS;
            SetCropGrowth(bx, by - 1, 0);
            RegisterCrop(bx, by - 1);
            NetSyncBlockChange(bx, by - 1, BLOCK_CROPS);
            UpdateLightAt(bx, by - 1);
            InvalidateChunkAt(bx, by - 1);
            // Creative mode: seeds are infinite
            if (gameMode != GAME_CREATIVE) {
                p->inventoryCount[slot]--;
                if (p->inventoryCount[slot] <= 0) {
                    p->inventory[slot] = BLOCK_AIR;
                    p->inventoryCount[slot] = 0;
                }
            }
            return true;
        }
    }

    // Breeding: feed passive mobs with food
    if (IsFood((BlockType)item)) {
        for (int i = 0; i < MAX_MOBS; i++) {
            if (!mobs[i].active || mobs[i].isBaby) continue;
            if (mobs[i].type != MOB_PIG && mobs[i].type != MOB_COW &&
                mobs[i].type != MOB_SHEEP && mobs[i].type != MOB_CHICKEN &&
                mobs[i].type != MOB_HORSE) continue;
            if (mobs[i].loveTimer > 0) continue;
            int mw = GetMobWidth(mobs[i].type);
            int mh = GetMobHeight(mobs[i].type);
            float mx = mobs[i].position.x, my = mobs[i].position.y;
            if (cursorX >= mx && cursorX <= mx + mw && cursorY >= my && cursorY <= my + mh) {
                float mdx = (p->position.x + PLAYER_WIDTH / 2) - (mx + mw / 2);
                float mdy = (p->position.y + PLAYER_HEIGHT / 2) - (my + mh / 2);
                if (mdx * mdx + mdy * mdy < (BREAK_RANGE * BLOCK_SIZE) * (BREAK_RANGE * BLOCK_SIZE)) {
                    mobs[i].loveTimer = 15.0f;
                    p->inventoryCount[slot]--;
                    if (p->inventoryCount[slot] <= 0) {
                        p->inventory[slot] = BLOCK_AIR;
                        p->inventoryCount[slot] = 0;
                    }
                    // Try to find a partner and spawn a baby
                    for (int j = 0; j < MAX_MOBS; j++) {
                        if (j == i || !mobs[j].active) continue;
                        if (mobs[j].type != mobs[i].type || mobs[j].loveTimer <= 0) continue;
                        float bdx = mobs[j].position.x - mobs[i].position.x;
                        float bdy = mobs[j].position.y - mobs[i].position.y;
                        if (bdx * bdx + bdy * bdy < 100 * 100) {
                            float babyX = (mobs[i].position.x + mobs[j].position.x) / 2;
                            float babyY = (mobs[i].position.y + mobs[j].position.y) / 2;
                            Mob *baby = SpawnMob(mobs[i].type, babyX, babyY);
                            if (baby) {
                                baby->isBaby = true;
                                baby->growTimer = 120.0f;
                                baby->health = baby->maxHealth / 2;
                                baby->maxHealth = baby->maxHealth / 2;
                                UnlockAchievement(ACH_BREEDER);
                            }
                            mobs[i].loveTimer = 0;
                            mobs[j].loveTimer = 0;
                            break;
                        }
                    }
                    return true;
                }
            }
        }
    }

    return false;
}

// Add item to a specific player's inventory (host-side helper)
static void AddToInventoryForPlayer(Player *p, BlockType item)
{
    if (IsTool(item)) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (p->inventory[i] == BLOCK_AIR) {
                p->inventory[i] = item;
                p->inventoryCount[i] = 1;
                p->toolDurability[i] = GetToolMaxDurability(item);
                return;
            }
        }
        return;
    }
    if (IsArmor(item)) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (p->inventory[i] == BLOCK_AIR) {
                p->inventory[i] = item;
                p->inventoryCount[i] = 1;
                p->toolDurability[i] = GetArmorMaxDurability(item);
                return;
            }
        }
        return;
    }
    // Stackable items
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (p->inventory[i] == item && p->inventoryCount[i] < 64) {
            p->inventoryCount[i]++;
            return;
        }
    }
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (p->inventory[i] == BLOCK_AIR) {
            p->inventory[i] = item;
            p->inventoryCount[i] = 1;
            return;
        }
    }
}

// Process pending projectile-vs-mob hits (authoritative damage resolution)
static void ProcessPendingProjectileHits(void)
{
    for (int i = 0; i < pendingProjectileHitCount; i++) {
        int mi = pendingProjectileHitIndex[i];
        if (mi < 0 || mi >= MAX_MOBS || !mobs[mi].active) continue;
        if (NetIsHost()) {
            DamageMob(&mobs[mi], pendingProjectileHitDamage[i]);
            // Broadcast to all clients
            uint8_t buf[64];
            buf[0] = PKT_DAMAGE_MOB;
            PktDamageMob dm;
            dm.mobIndex = (uint8_t)mi;
            dm.damage = pendingProjectileHitDamage[i];
            memcpy(buf + 1, &dm, sizeof(PktDamageMob));
            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                if (players[r].netControlled) {
                    NetSendTo(r, buf, 1 + sizeof(PktDamageMob), false);
                }
            }
        } else if (!NetIsClient()) {
            // Single-player: apply damage directly
            DamageMob(&mobs[mi], pendingProjectileHitDamage[i]);
        }
        // Client: don't apply locally, wait for host's PKT_DAMAGE_MOB
    }
    pendingProjectileHitCount = 0;
}

// Broadcast sound event to all clients
static void BroadcastSound(uint8_t soundId, float x, float y)
{
    if (!NetIsHost()) return;
    uint8_t buf[32];
    buf[0] = PKT_SOUND_EVENT;
    PktSoundEvent se;
    se.soundId = soundId;
    se.x = x; se.y = y;
    memcpy(buf + 1, &se, sizeof(PktSoundEvent));
    for (int r = 1; r < NET_MAX_PLAYERS; r++) {
        if (players[r].netControlled) {
            NetSendTo(r, buf, 1 + sizeof(PktSoundEvent), false);
        }
    }
}

// Authoritative remote bow fire. Returns true if the shot was accepted.
static bool TryFireBowRemote(Player *p, const PktBowRequest *req)
{
    if (p->playerDead) return false;

    int slot = p->selectedSlot;
    uint8_t held = p->inventory[slot];

    // Validate: holding a bow with durability
    if (held != ITEM_BOW) return false;
    if (p->toolDurability[slot] <= 0) return false;

    // Find arrows in inventory
    int arrowSlot = -1;
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (p->inventory[i] == ITEM_ARROW && p->inventoryCount[i] > 0) {
            arrowSlot = i;
            break;
        }
    }
    if (arrowSlot < 0) return false;

    // Spawn projectile (authoritative)
    int arrowIdx = SpawnProjectile(req->spawnX, req->spawnY, req->vx, req->vy, true);
    if (arrowIdx >= 0) {
        int baseDmg = PROJECTILE_DAMAGE;
        uint16_t be = p->itemEnchantments[slot];
        if (ENCH_TYPE(be) == ENCH_POWER)
            baseDmg += ENCH_LEVEL(be) * 2;
        // Critical hit: +50% damage at full charge
        if (req->charge >= BOW_CHARGE_MAX * 0.95f) {
            baseDmg = (int)(baseDmg * 1.5f);
        }
        projectiles[arrowIdx].damage = baseDmg;
    }

    // Consume arrow
    p->inventoryCount[arrowSlot]--;
    if (p->inventoryCount[arrowSlot] <= 0)
        p->inventory[arrowSlot] = BLOCK_AIR;

    // Bow durability (Unbreaking check)
    {
        uint16_t bowEnch = p->itemEnchantments[slot];
        bool skipBowDur = false;
        if (ENCH_TYPE(bowEnch) == ENCH_UNBREAKING) {
            skipBowDur = (rand() % (ENCH_LEVEL(bowEnch) + 1)) != 0;
        }
        if (!skipBowDur) p->toolDurability[slot]--;
        if (p->toolDurability[slot] <= 0) {
            p->inventory[slot] = BLOCK_AIR;
            p->inventoryCount[slot] = 0;
            p->toolDurability[slot] = 0;
            p->itemEnchantments[slot] = 0;
        }
    }

    return true;
}

// Authoritative remote ender pearl use. Returns true if teleport was accepted.
static bool TryEnderPearlRemote(Player *p, float targetX, float targetY)
{
    if (p->playerDead) return false;

    int slot = p->selectedSlot;
    uint8_t held = p->inventory[slot];
    if (held != ITEM_ENDER_PEARL) return false;
    if (p->inventoryCount[slot] <= 0) return false;

    // Distance check (max 400 pixels from player center)
    float px = p->position.x + PLAYER_WIDTH / 2.0f;
    float py = p->position.y + PLAYER_HEIGHT / 2.0f;
    float dx = targetX + PLAYER_WIDTH / 2.0f - px;
    float dy = targetY + PLAYER_HEIGHT / 2.0f - py;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist > 400.0f) return false;

    // Check target position is safe (not inside solid blocks)
    int tlx = (int)(targetX) / BLOCK_SIZE;
    int trx = (int)(targetX + PLAYER_WIDTH - 0.01f) / BLOCK_SIZE;
    int tty = (int)(targetY) / BLOCK_SIZE;
    int tby = (int)(targetY + PLAYER_HEIGHT - 0.01f) / BLOCK_SIZE;
    if (tlx < 0) tlx = 0;
    if (trx >= WORLD_WIDTH) trx = WORLD_WIDTH - 1;
    if (tty < 0) tty = 0;
    if (tby >= WORLD_HEIGHT) tby = WORLD_HEIGHT - 1;
    bool safe = true;
    for (int bx = tlx; bx <= trx && safe; bx++) {
        for (int by = tty; by <= tby && safe; by++) {
            if (IsBlockSolid(bx, by)) safe = false;
        }
    }
    if (!safe) {
        // Try to find nearest safe spot by scanning outward
        float bestX = targetX, bestY = targetY;
        float bestDist = 1e9f;
        for (int ox = -2; ox <= 2; ox++) {
            for (int oy = -2; oy <= 2; oy++) {
                int nx = (int)(targetX) / BLOCK_SIZE + ox;
                int ny = (int)(targetY) / BLOCK_SIZE + oy;
                if (nx < 0 || nx >= WORLD_WIDTH - 1 || ny < 0 || ny >= WORLD_HEIGHT - 2) continue;
                float sx = nx * BLOCK_SIZE;
                float sy = ny * BLOCK_SIZE;
                int sminBX = (int)(sx) / BLOCK_SIZE;
                int smaxBX = (int)(sx + PLAYER_WIDTH - 0.01f) / BLOCK_SIZE;
                int sminBY = (int)(sy) / BLOCK_SIZE;
                int smaxBY = (int)(sy + PLAYER_HEIGHT - 0.01f) / BLOCK_SIZE;
                bool free = true;
                for (int bx = sminBX; bx <= smaxBX && free; bx++) {
                    if (bx < 0 || bx >= WORLD_WIDTH) { free = false; break; }
                    for (int by = sminBY; by <= smaxBY && free; by++) {
                        if (IsBlockSolid(bx, by)) free = false;
                    }
                }
                if (free) {
                    float dist2 = (sx - targetX) * (sx - targetX) + (sy - targetY) * (sy - targetY);
                    if (dist2 < bestDist) { bestDist = dist2; bestX = sx; bestY = sy; }
                }
            }
        }
        if (bestDist < 1e8f) {
            targetX = bestX;
            targetY = bestY;
            safe = true;
        }
    }
    if (!safe) return false;

    // Teleport
    p->position.x = targetX;
    p->position.y = targetY;
    p->velocity.x = 0;
    p->velocity.y = 0;

    // Consume pearl
    p->inventoryCount[slot]--;
    if (p->inventoryCount[slot] <= 0) {
        p->inventory[slot] = BLOCK_AIR;
    }
    if (gameMode != GAME_CREATIVE) { // Creative: no pearl fall damage
        p->health -= 2;
        p->damageFlashTimer = 0.3f;
    }

    return true;
}

// Authoritative remote enchanting. Returns true if enchantment was applied.
static bool TryEnchantRemote(Player *p, int blockX, int blockY, int enchantType, int enchantLevel, int xpCost)
{
    if (p->playerDead) return false;

    // Validate enchanting table exists
    if (blockX < 0 || blockX >= WORLD_WIDTH || blockY < 0 || blockY >= WORLD_HEIGHT) return false;
    if (world[blockX][blockY] != BLOCK_ENCHANTING_TABLE) return false;

    int slot = p->selectedSlot;
    uint8_t held = p->inventory[slot];
    if (held == BLOCK_AIR) return false;

    // Only tools and armor can be enchanted
    if (!IsTool((BlockType)held) && !IsArmor((BlockType)held)) return false;

    // Check not already enchanted
    if (ENCH_TYPE(p->itemEnchantments[slot]) != ENCH_NONE) return false;

    // Validate enchantment level range
    if (enchantLevel < 1 || enchantLevel > 3) return false;

    // Validate XP cost range (based on level*2+3 * 10 pattern, range 50-200)
    if (xpCost < 30 || xpCost > 250) return false;
    if (p->xp < xpCost) return false;

    // Deduct XP
    p->xp -= xpCost;

    // Apply enchantment
    p->itemEnchantments[slot] = ENCH_PACK((EnchantmentType)enchantType, enchantLevel);

    // Restore durability
    int maxDur = IsTool((BlockType)held) ? GetToolMaxDurability((BlockType)held) :
                 IsArmor((BlockType)held) ? GetArmorMaxDurability((BlockType)held) : 0;
    if (IsTool((BlockType)held)) {
        p->toolDurability[slot] = maxDur;
    }

    return true;
}

// Authoritative remote fishing. Returns true if accepted.
static bool TryFishingRemote(Player *p, uint8_t action, float vx, float vy)
{
    if (p->playerDead) return false;

    int slot = p->selectedSlot;
    uint8_t held = p->inventory[slot];
    if (held != ITEM_FISHING_ROD) return false;

    if (action == 0) {
        // CAST: spawn fishing projectile
        // Check no active fishing projectile already
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active && projectiles[i].isFishing) return false;
        }
        float px = p->position.x + PLAYER_WIDTH / 2.0f;
        float py = p->position.y + PLAYER_HEIGHT / 2.0f;
        float speed = PROJECTILE_SPEED * 0.8f;
        float aimDist = sqrtf(vx * vx + vy * vy);
        if (aimDist < 0.01f) return false;
        int si = SpawnProjectile(px, py, (vx / aimDist) * speed, (vy / aimDist) * speed, false);
        if (si >= 0) {
            projectiles[si].isFishing = true;
            projectiles[si].fishTimer = 5.0f + (float)(rand() % 25);
            projectiles[si].lifetime = 60.0f;
        }
        return true;
    } else if (action == 1) {
        // RETRACT: check for active fishing projectile
        for (int i = 0; i < MAX_PROJECTILES; i++) {
            if (projectiles[i].active && projectiles[i].isFishing) {
                if (projectiles[i].hasBite) {
                    // Generate catch
                    int roll = rand() % 100;
                    int catchItem;
                    if (roll < 60) {
                        catchItem = ITEM_RAW_FISH;
                    } else if (roll < 85) {
                        int junk = rand() % 3;
                        catchItem = (junk == 0) ? ITEM_STICK : (junk == 1) ? ITEM_BONE : ITEM_RAW_CHICKEN;
                    } else {
                        int treasure = rand() % 4;
                        catchItem = (treasure == 0) ? ITEM_ENDER_PEARL : (treasure == 1) ? ITEM_IRON_INGOT :
                                    (treasure == 2) ? ITEM_BOW : ITEM_STRING;
                    }
                    AddToInventoryForPlayer(p, (BlockType)catchItem);
                }
                projectiles[i].active = false;
                return true;
            }
        }
    }
    return false;
}

// Authoritative remote cauldron interaction. Returns true if accepted.
static bool TryCauldronInteractRemote(Player *p, int bx, int by)
{
    if (p->playerDead) return false;
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;
    if (world[bx][by] != BLOCK_CAULDRON) return false;

    int slot = p->selectedSlot;
    uint8_t held = p->inventory[slot];

    // Find or create cauldron entry
    int cdIdx = -1;
    for (int ci = 0; ci < cauldronCount; ci++) {
        if (cauldrons[ci].x == bx && cauldrons[ci].y == by) {
            cdIdx = ci;
            break;
        }
    }
    if (cdIdx < 0 && cauldronCount < MAX_CAULDRONS) {
        cdIdx = cauldronCount++;
        cauldrons[cdIdx].x = bx;
        cauldrons[cdIdx].y = by;
        cauldrons[cdIdx].fillLevel = 0;
    }
    if (cdIdx < 0) return false;

    if (held == ITEM_WATER_BUCKET && cauldrons[cdIdx].fillLevel < 3) {
        cauldrons[cdIdx].fillLevel = 3;
        p->inventory[slot] = ITEM_BUCKET;
        return true;
    }
    if (held == ITEM_BUCKET && cauldrons[cdIdx].fillLevel > 0) {
        cauldrons[cdIdx].fillLevel = 0;
        p->inventory[slot] = ITEM_WATER_BUCKET;
        return true;
    }
    if (cauldrons[cdIdx].fillLevel >= 2) {
        p->oxygen = MAX_OXYGEN;
        cauldrons[cdIdx].fillLevel = 1;
        return true;
    }
    return false;
}

// Authoritative remote item drop. Returns true if accepted.
static bool TryItemDropRemote(Player *p, int slotIdx)
{
    if (p->playerDead) return false;
    if (slotIdx < 0 || slotIdx >= INVENTORY_SLOTS) return false;

    uint8_t item = p->inventory[slotIdx];
    if (item == BLOCK_AIR) return false;

    int count = p->inventoryCount[slotIdx];
    float px = p->position.x + PLAYER_WIDTH / 2;
    float py = p->position.y + PLAYER_HEIGHT / 2;
    SpawnItemEntity(item, count, px, py);

    p->inventory[slotIdx] = BLOCK_AIR;
    p->inventoryCount[slotIdx] = 0;
    p->toolDurability[slotIdx] = 0;
    p->itemEnchantments[slotIdx] = 0;
    return true;
}

//----------------------------------------------------------------------------------
// Chat helpers
//----------------------------------------------------------------------------------
void AddChatMessage(uint8_t playerId, const char *msg)
{
    if (!msg || !msg[0]) return;
    // Shift history if full
    if (chatHistoryCount >= MAX_CHAT_MESSAGES) {
        for (int i = 1; i < MAX_CHAT_MESSAGES; i++) {
            chatHistory[i - 1] = chatHistory[i];
        }
        chatHistoryCount = MAX_CHAT_MESSAGES - 1;
    }
    ChatMessage *cm = &chatHistory[chatHistoryCount++];
    cm->playerId = playerId;
    snprintf(cm->message, sizeof(cm->message), "%s", msg);
    cm->timer = 12.0f; // visible for 12 seconds
}

void BroadcastChatMessage(uint8_t playerId, const char *msg)
{
    AddChatMessage(playerId, msg);
    if (!NetIsConnected()) return;
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_CHAT;
    PktChat pkt;
    pkt.playerId = playerId;
    snprintf(pkt.message, sizeof(pkt.message), "%s", msg);
    memcpy(buf + 1, &pkt, sizeof(pkt));
    if (NetIsHost()) {
        for (int r = 1; r < NET_MAX_PLAYERS; r++) {
            if (players[r].netControlled) {
                NetSendTo(r, buf, 1 + sizeof(pkt), true);
            }
        }
    } else if (NetIsClient()) {
        NetSendToServer(buf, 1 + sizeof(pkt), true);
    }
}

static bool ProcessChatCommand(const char *msg);

void SendChatMessage(const char *msg)
{
    if (!msg || !msg[0]) return;
    // Process commands locally on host/single-player.
    if (msg[0] == '/' && !NetIsClient()) {
        if (ProcessChatCommand(msg)) return;
    }
    if (NetIsClient()) {
        // Client sends to host; host will relay with proper playerId.
        uint8_t buf[NET_PACKET_MAX];
        buf[0] = PKT_CHAT;
        PktChat pkt;
        pkt.playerId = (uint8_t)localPlayerId;
        snprintf(pkt.message, sizeof(pkt.message), "%s", msg);
        memcpy(buf + 1, &pkt, sizeof(pkt));
        NetSendToServer(buf, 1 + sizeof(pkt), true);
        // Optimistically show locally until host echoes.
        AddChatMessage((uint8_t)localPlayerId, msg);
    } else {
        // Single-player or host: broadcast immediately.
        BroadcastChatMessage((uint8_t)localPlayerId, msg);
    }
}

// Returns true if msg was a recognized command.
static bool ProcessChatCommand(const char *msg)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", msg);
    char *cmd = strtok(buf, " ");
    if (!cmd) return false;

    if (strcmp(cmd, "/help") == 0) {
        AddChatMessage(255, "Commands:");
        AddChatMessage(255, "/tp x y  /give id count  /heal  /list");
        AddChatMessage(255, "/time day|night|noon|midnight|value");
        AddChatMessage(255, "/weather clear|rain|thunder");
        AddChatMessage(255, "/gamemode survival|creative  /mode");
        return true;
    }

    if (strcmp(cmd, "/mode") == 0 || strcmp(cmd, "/gamemode") == 0) {
        char *sarg = strtok(NULL, " ");
        GameMode newMode;
        if (sarg && (strcmp(sarg, "creative") == 0 || strcmp(sarg, "c") == 0 || strcmp(sarg, "1") == 0)) {
            newMode = GAME_CREATIVE;
        } else if (sarg && (strcmp(sarg, "survival") == 0 || strcmp(sarg, "s") == 0 || strcmp(sarg, "0") == 0)) {
            newMode = GAME_SURVIVAL;
        } else {
            AddChatMessage(255, gameMode == GAME_CREATIVE
                           ? "Current mode: Creative"
                           : "Current mode: Survival");
            AddChatMessage(255, "Usage: /gamemode survival|creative");
            return true;
        }
        gameMode = newMode;
        // Broadcast the change to all clients.
        if (NetIsHost()) {
            uint8_t mbuf[NET_PACKET_MAX];
            mbuf[0] = PKT_GAMEMODE_SYNC;
            mbuf[1] = (uint8_t)gameMode;
            NetSendToAll(mbuf, 2, true);
        }
        AddChatMessage(255, gameMode == GAME_CREATIVE
                       ? "Game mode: Creative (E opens creative inventory)"
                       : "Game mode: Survival");
        return true;
    }

    if (strcmp(cmd, "/tp") == 0) {
        char *sx = strtok(NULL, " ");
        char *sy = strtok(NULL, " ");
        if (sx && sy) {
            int tx = atoi(sx);
            int ty = atoi(sy);
            if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                player.position.x = tx * BLOCK_SIZE;
                player.position.y = ty * BLOCK_SIZE;
                AddChatMessage(255, "Teleported.");
            } else {
                AddChatMessage(255, "Invalid coordinates.");
            }
        } else {
            AddChatMessage(255, "Usage: /tp x y");
        }
        return true;
    }

    if (strcmp(cmd, "/give") == 0) {
        char *sid = strtok(NULL, " ");
        char *scnt = strtok(NULL, " ");
        if (sid && scnt) {
            int id = atoi(sid);
            int cnt = atoi(scnt);
            if (id > 0 && id < BLOCK_COUNT && cnt > 0 && cnt <= 64) {
                int added = AddToInventoryCount((BlockType)id, cnt);
                if (added < cnt) {
                    SpawnItemEntity((uint8_t)id, cnt - added,
                                    player.position.x + PLAYER_WIDTH / 2, player.position.y);
                }
                AddChatMessage(255, "Given item.");
            } else {
                AddChatMessage(255, "Invalid item id or count.");
            }
        } else {
            AddChatMessage(255, "Usage: /give id count");
        }
        return true;
    }

    if (strcmp(cmd, "/time") == 0) {
        char *sarg = strtok(NULL, " ");
        if (sarg) {
            if (strcmp(sarg, "day") == 0) dayNight.timeOfDay = 0.25f;
            else if (strcmp(sarg, "noon") == 0) dayNight.timeOfDay = 0.5f;
            else if (strcmp(sarg, "night") == 0) dayNight.timeOfDay = 0.75f;
            else if (strcmp(sarg, "midnight") == 0) dayNight.timeOfDay = 0.0f;
            else dayNight.timeOfDay = fmodf(atof(sarg), 1.0f);
            AddChatMessage(255, "Time set.");
        } else {
            AddChatMessage(255, "Usage: /time day|night|noon|midnight|value");
        }
        return true;
    }

    if (strcmp(cmd, "/weather") == 0) {
        char *sarg = strtok(NULL, " ");
        if (sarg) {
            if (strcmp(sarg, "clear") == 0) { weather.type = WEATHER_CLEAR; weather.duration = 600.0f; }
            else if (strcmp(sarg, "rain") == 0) { weather.type = WEATHER_RAIN; weather.duration = 600.0f; }
            else if (strcmp(sarg, "thunder") == 0) { weather.type = WEATHER_THUNDER; weather.duration = 600.0f; }
            else { AddChatMessage(255, "Unknown weather."); return true; }
            AddChatMessage(255, "Weather set.");
        } else {
            AddChatMessage(255, "Usage: /weather clear|rain|thunder");
        }
        return true;
    }

    if (strcmp(cmd, "/heal") == 0) {
        if (player.playerDead) {
            AddChatMessage(255, "You are dead. Respawn first!");
        } else {
            player.health = MAX_HEALTH;
            player.hunger = MAX_HUNGER;
            player.oxygen = MAX_OXYGEN;
            AddChatMessage(255, "Health and hunger restored.");
        }
        return true;
    }

    if (strcmp(cmd, "/list") == 0) {
        char listBuf[256] = "";
        int count = 0, remaining = (int)sizeof(listBuf) - 1;
        for (int i = 0; i < MAX_NET_PLAYERS; i++) {
            if (i == 0 || players[i].netControlled) {
                const char *name = players[i].playerName[0] ? players[i].playerName : "Host";
                int prefixLen = count > 0 ? 2 : 0; // ", "
                int needed = prefixLen + (int)strlen(name);
                if (needed >= remaining) break;
                if (count > 0) { strcat(listBuf, ", "); remaining -= 2; }
                strcat(listBuf, name);
                remaining -= (int)strlen(name);
                count++;
            }
        }
        if (count == 0) strcpy(listBuf, "No players online.");
        AddChatMessage(255, listBuf);
        return true;
    }

    AddChatMessage(255, "Unknown command. Use /help.");
    return true;
}

void ReturnFurnaceItems(void)
{
    if (furnaceFuel != BLOCK_AIR) {
        int added = AddToInventoryCount((BlockType)furnaceFuel, furnaceFuelCount);
        if (added < furnaceFuelCount) {
            SpawnItemEntity(furnaceFuel, furnaceFuelCount - added,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceFuel = BLOCK_AIR;
        furnaceFuelCount = 0;
    }
    if (furnaceInput != BLOCK_AIR) {
        int added = AddToInventoryCount((BlockType)furnaceInput, furnaceInputCount);
        if (added < furnaceInputCount) {
            SpawnItemEntity(furnaceInput, furnaceInputCount - added,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceInput = BLOCK_AIR;
        furnaceInputCount = 0;
    }
    if (furnaceOutput != BLOCK_AIR) {
        int added = AddToInventoryCount((BlockType)furnaceOutput, furnaceOutputCount);
        if (added < furnaceOutputCount) {
            SpawnItemEntity(furnaceOutput, furnaceOutputCount - added,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceOutput = BLOCK_AIR;
        furnaceOutputCount = 0;
    }
    furnaceProgress = 0.0f;
    furnaceFuelBurn = 0.0f;
    furnaceFuelBurnMax = 0.0f;
    // Sync cleared state back to furnace array
    if (activeFurnace >= 0 && activeFurnace < furnaceCount) {
        SyncActiveToFurnace(activeFurnace);
        if (NetIsClient()) {
            // Notify host that fuel/input/output were returned to the closing player.
            SyncFurnaceToHost();
        } else if (NetIsHost()) {
            // Broadcast the now-empty furnace to other watching clients.
            SyncFurnaceToAll();
        }
    }
    activeFurnace = -1;
}

float GetFuelBurnTime(uint8_t item)
{
    if (item == ITEM_COAL) return 80.0f;        // 8 items per coal
    if (item == ITEM_STICK) return 5.0f;         // 0.5 items per stick
    if (item == BLOCK_WOOD) return 15.0f;        // 1.5 items per log
    if (item == BLOCK_PLANKS) return 15.0f;      // 1.5 items per planks
    return 0.0f;
}

// Furnace smelting tick
static void UpdateFurnaceTick(float dt)
{
    if (furnaceInput == BLOCK_AIR) return;
    if (furnaceOutput != BLOCK_AIR && furnaceOutputCount >= 64) return;

    int recipe = FindSmeltRecipe((BlockType)furnaceInput);
    if (recipe < 0) return;

    // Need fuel
    if (furnaceFuelBurn <= 0.0f) {
        float burnTime = GetFuelBurnTime(furnaceFuel);
        if (burnTime > 0.0f && furnaceFuelCount > 0) {
            furnaceFuelBurn = burnTime;
            furnaceFuelBurnMax = burnTime;
            furnaceFuelCount--;
            if (furnaceFuelCount <= 0) furnaceFuel = BLOCK_AIR;
        } else {
            return;
        }
    }

    furnaceFuelBurn -= dt;

    furnaceProgress += dt / 10.0f;
    if (furnaceProgress >= 1.0f) {
        furnaceProgress = 0.0f;
        furnaceInputCount--;
        if (furnaceInputCount <= 0) furnaceInput = BLOCK_AIR;

        BlockType output = smeltRecipes[recipe].output;
        if (furnaceOutput == BLOCK_AIR) {
            furnaceOutput = (uint8_t)output;
            furnaceOutputCount = 1;
        } else if (furnaceOutput == (uint8_t)output) {
            furnaceOutputCount++;
        }
    }
    // Sync back to furnace array
    if (activeFurnace >= 0 && activeFurnace < furnaceCount) {
        SyncActiveToFurnace(activeFurnace);
    }
}

// Find a safe surface Y (pixel) for spawning at block column bx on the current world.
// Scans from the top for the first non-air/non-water block and returns the pixel Y
// that rests the player's feet just above it. Used so network joiners don't spawn
// inside terrain when the host's reported Y is underground (e.g. host dug a tunnel).
static float FindSpawnSurfaceY(int bx)
{
    if (bx < 0) bx = 0;
    if (bx >= WORLD_WIDTH) bx = WORLD_WIDTH - 1;
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        uint8_t b = world[bx][y];
        if (b != BLOCK_AIR && b != BLOCK_WATER) {
            return (float)(y * BLOCK_SIZE - PLAYER_HEIGHT);
        }
    }
    return (float)(SEA_LEVEL * BLOCK_SIZE - PLAYER_HEIGHT);
}

void UpdateGame(float dt)
{
    // Block input during screen transitions
    if (IsTransitioning()) return;

    if (gameState == STATE_MENU) {
        UpdateMainMenu(dt);
        return;
    }
    if (gameState == STATE_SLOT_SELECT) {
        UpdateSlotSelect(dt);
        return;
    }
    if (gameState == STATE_SETTINGS) {
        // Settings is mostly handled in DrawSettingsScreen (input + render)
        return;
    }
    if (gameState == STATE_HOST_WAITING) {
        // Poll network for incoming clients
        NetPoll();
        // Check for packets from joining clients
        for (int i = 0; i < NetGetReceivedCount(); i++) {
            int size, fromId;
            const void *data = NetGetReceived(i, &size, &fromId);
            if (!data) continue;
            uint8_t type = ((const uint8_t *)data)[0];
            if (type == PKT_JOIN) {
                // Extract the joining player's chosen name (optional).
                const char *joinedName = "Player";
                if (size >= 1 + (int)sizeof(PktJoin)) {
                    const PktJoin *pj = (const PktJoin *)((const uint8_t *)data + 1);
                    if (pj->playerName[0]) joinedName = pj->playerName;
                }

                // Send welcome packet to the new client
                PktWelcome welcome;
                welcome.playerId = (uint8_t)fromId;
                welcome.worldSeed = worldSeed;
                welcome.worldW = WORLD_WIDTH;
                welcome.worldH = WORLD_HEIGHT;
                welcome.timeOfDay = dayNight.timeOfDay;
                welcome.weatherType = (uint8_t)weather.type;
                welcome.weatherDuration = weather.duration;
                welcome.spawnX = player.position.x;
                welcome.spawnY = player.position.y;
                welcome.gameMode = (uint8_t)gameMode;
                uint8_t buf[NET_PACKET_MAX];
                buf[0] = PKT_WELCOME;
                memcpy(buf + 1, &welcome, sizeof(welcome));
                NetSendTo(fromId, buf, 1 + sizeof(welcome), true);
                // Initialize remote player
                memset(&players[fromId], 0, sizeof(Player));
                players[fromId].netControlled = true;
                players[fromId].health = MAX_HEALTH;
                players[fromId].hunger = 20;
                players[fromId].facingRight = true;
                snprintf(players[fromId].playerName, sizeof(players[fromId].playerName), "%s", joinedName);
                // Starter inventory (matches the late-join path during play)
                players[fromId].inventory[0] = TOOL_WOOD_SWORD;   players[fromId].inventoryCount[0] = 1;
                players[fromId].inventory[1] = TOOL_WOOD_PICKAXE; players[fromId].inventoryCount[1] = 1;
                players[fromId].inventory[2] = TOOL_WOOD_AXE;     players[fromId].inventoryCount[2] = 1;
                players[fromId].inventory[3] = TOOL_WOOD_SHOVEL;  players[fromId].inventoryCount[3] = 1;
                players[fromId].inventory[4] = TOOL_WOOD_HOE;     players[fromId].inventoryCount[4] = 1;
                players[fromId].inventory[5] = BLOCK_PLANKS;      players[fromId].inventoryCount[5] = 64;
                players[fromId].inventory[6] = BLOCK_COBBLESTONE; players[fromId].inventoryCount[6] = 64;
                remotePlayers[fromId].active = true;
                remotePlayers[fromId].interpX = players[fromId].position.x;
                remotePlayers[fromId].interpY = players[fromId].position.y;
                // Send authoritative inventory to the new client.
                {
                    uint8_t sbuf[NET_PACKET_MAX];
                    sbuf[0] = PKT_INVENTORY_SYNC;
                    PktInventorySync ipkt;
                    PackInventorySync(&ipkt, fromId);
                    memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                    NetSendTo(fromId, sbuf, 1 + sizeof(ipkt), true);
                }
                // Send modified blocks to new client
                NetSendWorldToClient(fromId);
                fluidSnapshotId[fromId]++;
                fluidSnapshotNext[fromId] = 0;
                fluidSnapshotActive[fromId] = false;
                ShowMessage(S(STR_NET_PLAYER_JOINED), (Color){100, 255, 100, 255});
            }
        }
        // Enter to start game (host can start alone or with players)
        if (Win32IsKeyPressed(KEY_ENTER) || Win32IsKeyPressed(KEY_SPACE) ||
            (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
             CheckCollisionPointRec(Win32GetMousePosition(), (Rectangle){370, 340, 540, 46}))) {
            localPlayerId = 0;
            StartTransition(STATE_PLAYING);
            PlaySoundUIClick();
            return;
        }
        // ESC to cancel hosting
        if (Win32IsKeyPressed(KEY_ESCAPE)) {
            NetHostStop();
            StartTransition(STATE_MENU);
            menuSelection = 0;
            PlaySoundUIClick();
        }
        return;
    }
    if (gameState == STATE_JOIN_GAME) {

        if (!joinConnecting) {
            // Name + IP input mode. Tab switches focus.
            if (Win32IsKeyPressed(KEY_TAB)) joinInputFocus = 1 - joinInputFocus;

            char *activeBuf = (joinInputFocus == 0) ? joinNameBuf : joinIpBuf;
            int *activeLen = (joinInputFocus == 0) ? &joinNameLen : &joinIpLen;
            int activeCap = (joinInputFocus == 0) ? 31 : 60;

            int c;
            while ((c = Win32GetCharPressed()) != 0) {
                if (*activeLen < activeCap && c >= 32 && c < 127) {
                    activeBuf[(*activeLen)++] = (char)c;
                    activeBuf[*activeLen] = '\0';
                }
            }
            // Backspace handling
            if (Win32IsKeyPressed(KEY_BACKSPACE)) {
                if (*activeLen > 0) activeBuf[--(*activeLen)] = '\0';
            }
            // Enter to connect
            if (Win32IsKeyPressed(KEY_ENTER)) {
                joinNameBuf[joinNameLen] = '\0';
                joinIpBuf[joinIpLen] = '\0';
                // Default name if empty
                if (joinNameLen <= 0) { strcpy(joinNameBuf, "Player"); joinNameLen = 6; }
                if (joinIpLen > 0 && NetClientConnect(joinIpBuf, NET_PORT, joinNameBuf)) {
                    snprintf(player.playerName, sizeof(player.playerName), "%s", joinNameBuf);
                    joinConnecting = true;
                    joinConnectTimer = 0.0f;
                }
            }
            if (Win32IsKeyPressed(KEY_ESCAPE)) {
                joinIpLen = 8;
                memcpy(joinIpBuf, "127.0.0.1", 9);
                joinNameLen = 6;
                memcpy(joinNameBuf, "Player", 7);
                joinInputFocus = 0;
                StartTransition(STATE_MENU);
                menuSelection = 0;
                PlaySoundUIClick();
            }
        } else {
            // Waiting for welcome from server
            joinConnectTimer += dt;
            NetPoll();
            for (int i = 0; i < NetGetReceivedCount(); i++) {
                int size, fromId;
                const void *data = NetGetReceived(i, &size, &fromId);
                if (!data) continue;
                uint8_t type = ((const uint8_t *)data)[0];
                if (type == PKT_WELCOME && size >= 1 + (int)sizeof(PktWelcome)) {
                    const PktWelcome *w = (const PktWelcome *)((const uint8_t *)data + 1);
                    localPlayerId = w->playerId;
                    worldSeed = w->worldSeed;
                    // Initialize game systems
                    confirmDialogActive = false;
                    gamePaused = false;
                    inventoryOpen = false;
                    furnaceOpen = false;
                    craftingTableOpen = false;
                    chestOpen = false;
                    InitMobs();
                    InitParticles();
                    InitEntities();
                    InitProjectiles();
                    InitLightMap();
                    InitSmeltingRecipes();
                    GenerateWorld(worldSeed);
                    InitPlayer();
                    // Spawn near the host horizontally, but snap to this client's own
                    // surface so we never spawn inside terrain (host may be underground,
                    // and host's edits arrive after world generation as separate packets).
                    player.position.x = w->spawnX;
                    player.position.y = FindSpawnSurfaceY((int)(w->spawnX / BLOCK_SIZE));
                    dayNight.timeOfDay = w->timeOfDay;
                    weather.type = (WeatherType)w->weatherType;
                    weather.duration = w->weatherDuration;
                    gameMode = (GameMode)w->gameMode;
                    RecalculateAllLight();
                    InitCameraSystem();
                    InitChunkTable();
                    UpdateChunks();
                    player.playerDead = false;
                    joinConnecting = false;
                    joinIpLen = 8;
                    memcpy(joinIpBuf, "127.0.0.1", 9);
                    // Request fluid snapshot after baseline generation.
                    PktFluidRequest fr = { 0, 0, FLUID_REQUEST_BEGIN_SNAPSHOT, 0 };
                    uint8_t fbuf[NET_PACKET_MAX]; fbuf[0] = PKT_FLUID_REQUEST;
                    memcpy(fbuf + 1, &fr, sizeof(fr));
                    NetSendToServer(fbuf, 1 + sizeof(fr), true);
                    // Start playing
                    StartTransition(STATE_PLAYING);
                    return;
                }
            }
            if (joinConnectTimer > 10.0f || Win32IsKeyPressed(KEY_ESCAPE)) {
                NetClientDisconnect();
                joinConnecting = false;
                joinIpLen = 8;
                memcpy(joinIpBuf, "127.0.0.1", 9);
                StartTransition(STATE_MENU);
                menuSelection = 0;
                PlaySoundUIClick();
            }
        }
        return;
    }

    // Sleep fade is a visual/time transition and must not be blocked by UI pause.
    UpdateDayNight(dt);

    // Auto-save every 5 minutes
    {
        static float autoSaveTimer = 0.0f;
        autoSaveTimer += dt;
        if (autoSaveTimer >= 300.0f) {
            autoSaveTimer = 0.0f;
            if (!player.playerDead && currentSavePath[0] && !NetIsClient()) {
                SaveWorld(currentSavePath);
                ShowMessage(S(STR_MSG_GAME_SAVED), (Color){100, 200, 100, 255});
            }
        }
    }

    // Update message timer
    if (messageTimer > 0.0f) messageTimer -= dt;

    // Death respawn input
    if (player.playerDead) {
        // Keep the world/server alive while the local player is dead. Do not
        // process player input, but continue mob simulation and networking.
        if (NetIsClient()) {
            NetPoll();
        } else {
            UpdateMobs(dt);
            UpdateProjectiles(dt);
            UpdateEntities(dt);
            UpdateParticles(dt);
        }
        if (GetDeathFadeTimer() > 1.0f) {
            if (Win32IsKeyPressed(KEY_SPACE)) {
                RespawnPlayer();
            }
            // Mouse click on respawn text
            Vector2 mpos = Win32GetMousePosition();
            const char *rsText = S(STR_PRESS_SPACE_RESPAWN);
            Rectangle rsRect = {
                (float)(SCREEN_WIDTH - DEATH_RESPAWN_BUTTON_W) / 2.0f,
                (float)DEATH_RESPAWN_BUTTON_Y,
                DEATH_RESPAWN_BUTTON_W,
                DEATH_RESPAWN_BUTTON_H
            };
            if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mpos, rsRect)) {
                RespawnPlayer();
            }
        }
        // ESC during death: return to main menu
        if (Win32IsKeyPressed(KEY_ESCAPE)) {
            if (currentSavePath[0]) SaveWorld(currentSavePath);
            StartTransition(STATE_MENU);
            menuSelection = 0;
            PlaySoundUIClick();
        }
        return;
    }

    // Chat input handling
    bool chatJustClosed = false;
    if (chatOpen) {
        int c;
        while ((c = Win32GetCharPressed()) != 0) {
            if (chatInputLen < MAX_CHAT_INPUT - 1 && c >= 32 && c < 127) {
                chatInput[chatInputLen++] = (char)c;
                chatInput[chatInputLen] = '\0';
            }
        }
        if (Win32IsKeyPressed(KEY_BACKSPACE)) {
            if (chatInputLen > 0) chatInput[--chatInputLen] = '\0';
        }
        if (Win32IsKeyPressed(KEY_ENTER)) {
            if (chatInputLen > 0) {
                SendChatMessage(chatInput);
            }
            chatOpen = false;
            chatJustClosed = true;
            chatInput[0] = '\0';
            chatInputLen = 0;
        }
        if (Win32IsKeyPressed(KEY_ESCAPE)) {
            chatOpen = false;
            chatJustClosed = true;
            chatInput[0] = '\0';
            chatInputLen = 0;
        }
    } else {
        // Open chat
        if (Win32IsKeyPressed(KEY_T)) {
            chatOpen = true;
            chatInput[0] = '\0';
            chatInputLen = 0;
        }
    }

    // Toggle inventory
    if (!chatOpen && Win32IsKeyPressed(KEY_E)) {
        if (furnaceOpen) {
            // Close furnace
            CloseFurnaceNetwork();
            ReturnFurnaceItems();
            furnaceOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        } else if (chestOpen) {
            // Close chest
            CloseChestNetwork();
            chestOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        } else if (tradeOpen) {
            // Close trade
            tradeOpen = false;
            inventoryOpen = false;
        } else if (gameMode == GAME_CREATIVE) {
            // Creative: E toggles the block/item palette
            creativeOpen = !creativeOpen;
            inventoryOpen = false;
            gamePaused = false;
        } else {
            inventoryOpen = !inventoryOpen;
            if (inventoryOpen) {
                gamePaused = false;
            } else {
                ReturnHeldItem();
                craftingTableOpen = false;
                craftSearchLen = 0;
                craftSearchBuf[0] = '\0';
            }
        }
        PlaySoundUIClick();
    }

    // I: collection and achievement panel
    if (!chatOpen && !player.playerDead && Win32IsKeyPressed(KEY_I)) {
        achievementsOpen = !achievementsOpen;
        gamePaused = achievementsOpen;
        if (achievementsOpen) { inventoryOpen = false; creativeOpen = false; }
        PlaySoundUIClick();
    }

    // ESC: close enchanting first, then furnace, then chest, then inventory, then large map, then toggle pause
    if (!chatOpen && !chatJustClosed && Win32IsKeyPressed(KEY_ESCAPE)) {
        if (localEnchantSession.open) {
            localEnchantSession.open = false;
            inventoryOpen = false;
            gamePaused = false;
            localEnchantSession.optionCount = 0;
        } else if (furnaceOpen) {
            CloseFurnaceNetwork();
            ReturnFurnaceItems();
            furnaceOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        } else if (chestOpen) {
            CloseChestNetwork();
            chestOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        } else if (tradeOpen) {
            tradeOpen = false;
            inventoryOpen = false;
            gamePaused = false;
        } else if (achievementsOpen) {
            achievementsOpen = false;
            gamePaused = false;
        } else if (creativeOpen) {
            creativeOpen = false;
            gamePaused = false;
        } else if (inventoryOpen) {
            inventoryOpen = false;
            craftingTableOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        } else if (showLargeMap) {
            showLargeMap = false;
            gamePaused = false;
        } else {
            gamePaused = !gamePaused;
        }
        PlaySoundUIClick();
    }

    if (!chatOpen && !chatJustClosed && Win32IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    // M: toggle large map
    if (!chatOpen && !chatJustClosed && Win32IsKeyPressed(KEY_M) && !inventoryOpen && !player.playerDead) {
        showLargeMap = !showLargeMap;
        if (showLargeMap) gamePaused = true;
        else gamePaused = false;
    }

    // F11: toggle fullscreen (any non-windowed mode -> windowed, windowed -> borderless fullscreen)
    // Use borderless instead of exclusive fullscreen to avoid DPI issues with desktop icons
    if (!chatOpen && !chatJustClosed && Win32IsKeyPressed(KEY_F11)) {
        ApplyWindowMode(windowMode != 0 ? 0 : 2);
    }

    // XP healing
    if (!chatOpen && !chatJustClosed && Win32IsKeyPressed(KEY_H) && !inventoryOpen && !gamePaused && !player.playerDead) {
        if (player.xp >= XP_HEAL_COST && player.health < MAX_HEALTH) {
            player.xp -= XP_HEAL_COST;
            player.health += XP_HEAL_AMOUNT;
            if (player.health > MAX_HEALTH) player.health = MAX_HEALTH;
            PlaySoundEat();
            ShowMessage(S(STR_MSG_HEALED_XP), (Color){100, 240, 100, 255});
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT / 2,
                                 (Color){80, 220, 80, 200});
        } else if (player.xp < XP_HEAL_COST) {
            ShowMessage(S(STR_MSG_NOT_ENOUGH_XP), (Color){240, 200, 80, 255});
        }
    }

    // Don't update gameplay when paused, inventory open, or creative palette open
    if (!gamePaused && !inventoryOpen && !creativeOpen && !achievementsOpen) {
        if (!NetIsClient()) UpdateFluidTick(dt);
        if (NetIsHost()) {
            // Host mode: poll client inputs, run authoritative logic, broadcast state
            NetPoll();
            // Process received packets (client inputs, block changes, etc.)
            for (int i = 0; i < NetGetReceivedCount(); i++) {
                int size, fromId;
                const void *data = NetGetReceived(i, &size, &fromId);
                if (!data) continue;
                uint8_t type = ((const uint8_t *)data)[0];
                if (type == PKT_JOIN && fromId > 0 && fromId < MAX_NET_PLAYERS) {
                    // Extract the joining player's chosen name.
                    const char *joinedName = "Player";
                    if (size >= 1 + (int)sizeof(PktJoin)) {
                        const PktJoin *pj = (const PktJoin *)((const uint8_t *)data + 1);
                        if (pj->playerName[0]) joinedName = pj->playerName;
                    }

                    // Late joiner: send welcome + world state
                    PktWelcome welcome;
                    welcome.playerId = (uint8_t)fromId;
                    welcome.worldSeed = worldSeed;
                    welcome.worldW = WORLD_WIDTH;
                    welcome.worldH = WORLD_HEIGHT;
                    welcome.timeOfDay = dayNight.timeOfDay;
                    welcome.weatherType = (uint8_t)weather.type;
                    welcome.weatherDuration = weather.duration;
                    welcome.spawnX = player.position.x;
                    welcome.spawnY = player.position.y;
                    welcome.gameMode = (uint8_t)gameMode;
                    uint8_t wbuf[NET_PACKET_MAX];
                    wbuf[0] = PKT_WELCOME;
                    memcpy(wbuf + 1, &welcome, sizeof(welcome));
                    NetSendTo(fromId, wbuf, 1 + sizeof(welcome), true);
                    // Initialize remote player with starter inventory
                    memset(&players[fromId], 0, sizeof(Player));
                    players[fromId].netControlled = true;
                    players[fromId].health = MAX_HEALTH;
                    players[fromId].hunger = 20;
                    players[fromId].facingRight = true;
                    snprintf(players[fromId].playerName, sizeof(players[fromId].playerName), "%s", joinedName);
                    players[fromId].inventory[0] = TOOL_WOOD_SWORD;
                    players[fromId].inventoryCount[0] = 1;
                    players[fromId].inventory[1] = TOOL_WOOD_PICKAXE;
                    players[fromId].inventoryCount[1] = 1;
                    players[fromId].inventory[2] = TOOL_WOOD_AXE;
                    players[fromId].inventoryCount[2] = 1;
                    players[fromId].inventory[3] = TOOL_WOOD_SHOVEL;
                    players[fromId].inventoryCount[3] = 1;
                    players[fromId].inventory[4] = TOOL_WOOD_HOE;
                    players[fromId].inventoryCount[4] = 1;
                    players[fromId].inventory[5] = BLOCK_PLANKS;
                    players[fromId].inventoryCount[5] = 64;
                    players[fromId].inventory[6] = BLOCK_COBBLESTONE;
                    players[fromId].inventoryCount[6] = 64;
                    remotePlayers[fromId].active = true;
                    remotePlayers[fromId].interpX = players[fromId].position.x;
                    remotePlayers[fromId].interpY = players[fromId].position.y;
                    // Send authoritative inventory to the new client.
                    {
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_INVENTORY_SYNC;
                        PktInventorySync ipkt;
                        PackInventorySync(&ipkt, fromId);
                        memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                        NetSendTo(fromId, sbuf, 1 + sizeof(ipkt), true);
                    }
                    // Send modified blocks to new client
                    NetSendWorldToClient(fromId);
                fluidSnapshotId[fromId]++;
                fluidSnapshotNext[fromId] = 0;
                fluidSnapshotActive[fromId] = false;
                    ShowMessage(S(STR_NET_PLAYER_JOINED), (Color){100, 255, 100, 255});
                } else if (type == PKT_FLUID_REQUEST && size >= 1 + (int)sizeof(PktFluidRequest) && fromId > 0 && fromId < MAX_NET_PLAYERS) {
                    const PktFluidRequest *fr = (const PktFluidRequest *)((const uint8_t *)data + 1);
                    if (fr->action == FLUID_REQUEST_BEGIN_SNAPSHOT) {
                        fluidSnapshotId[fromId]++;
                        fluidSnapshotNext[fromId] = 0;
                        fluidSnapshotActive[fromId] = true;
                        SendFluidSnapshotBatch(fromId, 0);
                        continue;
                    }
                    if (fr->action == FLUID_REQUEST_ACK_SNAPSHOT) {
                        if (fluidSnapshotActive[fromId] && fr->x == fluidSnapshotId[fromId] && fr->y == fluidSnapshotNext[fromId]) {
                            fluidSnapshotNext[fromId]++;
                            int total = SendFluidSnapshotBatch(fromId, fluidSnapshotNext[fromId]);
                            if (fluidSnapshotNext[fromId] >= total) fluidSnapshotActive[fromId] = false;
                        }
                        continue;
                    }
                    Player *fluidPlayer = &players[fromId];
                    if (!fluidPlayer->netControlled || fr->slot >= INVENTORY_SLOTS) continue;
                    fluidPlayer->selectedSlot = fr->slot;
                    int bx = fr->x, by = fr->y;
                    bool accepted = false;
                    if (fr->action == 0) accepted = TryPlaceBlockRemote(fluidPlayer, bx, by);
                    else if (fr->action == 1) accepted = TryUseItemRemote(fluidPlayer, bx, by, bx * BLOCK_SIZE + 8.0f, by * BLOCK_SIZE + 8.0f);
                    if (accepted) {
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_INVENTORY_SYNC;
                        PktInventorySync ipkt;
                        PackInventorySync(&ipkt, fromId);
                        memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                        NetSendTo(fromId, sbuf, 1 + sizeof(ipkt), true);
                    }

                    const PktInput *input = (const PktInput *)((const uint8_t *)data + 1);
                    // Apply remote player input
                    Player *rp = &players[fromId];
                    rp->netControlled = true;
                    rp->moveInput = input->moveX;
                    rp->jumpHeld = input->jump;
                    // Jump: only on rising edge (key was just pressed, not held)
                    if (input->jump && !prevJump[fromId] && rp->onGround) {
                        rp->velocity.y = JUMP_VELOCITY;
                        rp->onGround = false;
                    }
                    prevJump[fromId] = input->jump;
                    rp->sprinting = input->sprint;
                    rp->selectedSlot = input->selectedSlot;
                    rp->facingRight = input->moveX >= 0;
                    remotePlayers[fromId].active = true;
                    // Handle attack (mob damage) from remote player
                    if (input->attack) {
                        float cx = input->cursorX;
                        float cy = input->cursorY;
                        attackCooldownNet[fromId] -= dt;
                        if (attackCooldownNet[fromId] <= 0) {
                            for (int mi = 0; mi < MAX_MOBS; mi++) {
                                if (!mobs[mi].active) continue;
                                int mw = GetMobWidth(mobs[mi].type);
                                int mh = GetMobHeight(mobs[mi].type);
                                if (cx >= mobs[mi].position.x && cx <= mobs[mi].position.x + mw &&
                                    cy >= mobs[mi].position.y && cy <= mobs[mi].position.y + mh) {
                                    int dmg = 1;
                                    BlockType tool = (BlockType)rp->inventory[rp->selectedSlot];
                                    if (IsTool(tool)) {
                                        if (tool == TOOL_WOOD_SWORD) dmg = 3;
                                        else if (tool == TOOL_STONE_SWORD) dmg = 4;
                                        else if (tool == TOOL_IRON_SWORD) dmg = 6;
                                        else if (tool == TOOL_GOLD_SWORD) dmg = 4;
                                        else if (tool == TOOL_DIAMOND_SWORD) dmg = 8;
                                        else dmg = 2;
                                    }
                                    bool crit = rp->velocity.y > CRIT_FALL_THRESHOLD;
                                    if (crit) dmg = (int)(dmg * CRIT_DAMAGE_MULT);
                                    // Fire Aspect: set mob on fire
                                    uint16_t toolEnch = rp->itemEnchantments[rp->selectedSlot];
                                    if (ENCH_TYPE(toolEnch) == ENCH_FIRE_ASPECT) {
                                        mobs[mi].fireTimer = 1.5f * ENCH_LEVEL(toolEnch);
                                    }
                                    DamageMob(&mobs[mi], dmg);
                                    // Knockback: push mob further back
                                    if (ENCH_TYPE(toolEnch) == ENCH_KNOCKBACK) {
                                        float kbDir = (rp->position.x < mobs[mi].position.x) ? 1.0f : -1.0f;
                                        mobs[mi].velocity.x += kbDir * 150.0f * ENCH_LEVEL(toolEnch);
                                    }
                                    attackCooldownNet[fromId] = GetAttackSpeed(tool);
                                    break;
                                }
                            }
                        }
                    }
                    // Handle block place from remote player
                    if (input->place) {
                        int bx = (int)(input->cursorX / BLOCK_SIZE);
                        int by = (int)(input->cursorY / BLOCK_SIZE);
                        uint8_t held = (input->selectedSlot < INVENTORY_SLOTS) ? rp->inventory[input->selectedSlot] : BLOCK_AIR;
                        if (held == ITEM_BUCKET || held == ITEM_WATER_BUCKET || held == ITEM_LAVA_BUCKET) {
                            bool accepted = (held == ITEM_BUCKET)
                                ? TryUseItemRemote(rp, bx, by, input->cursorX, input->cursorY)
                                : TryPlaceBlockRemote(rp, bx, by);
                            if (accepted) {
                                uint8_t sbuf[NET_PACKET_MAX]; sbuf[0] = PKT_INVENTORY_SYNC;
                                PktInventorySync ipkt; PackInventorySync(&ipkt, fromId);
                                memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                                NetSendTo(fromId, sbuf, 1 + sizeof(ipkt), true);
                            }
                        } else {
                            TryPlaceBlockRemote(rp, bx, by);
                        }
                    }
                    // Handle item use from remote player
                    if (input->use) {
                        int bx = (int)(input->cursorX / BLOCK_SIZE);
                        int by = (int)(input->cursorY / BLOCK_SIZE);
                        if (TryUseItemRemote(rp, bx, by, input->cursorX, input->cursorY)) {
                            // Send authoritative inventory back to all clients.
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                        }
                    }
                } else if (type == PKT_BLOCK_CHANGE && size >= 1 + (int)sizeof(PktBlockChange)) {
                    const PktBlockChange *bc = (const PktBlockChange *)((const uint8_t *)data + 1);
                    if (bc->x < WORLD_WIDTH && bc->y < WORLD_HEIGHT) {
                        // Spawn item if block was broken (new type is AIR)
                        if (bc->blockType == BLOCK_AIR) {
                            uint8_t oldBlock = world[bc->x][bc->y];
                            if (oldBlock != BLOCK_AIR && oldBlock != BLOCK_WATER) {
                                uint8_t dropItem = oldBlock;
                                if (oldBlock == BLOCK_STONE) dropItem = BLOCK_COBBLESTONE;
                                else if (oldBlock == BLOCK_COAL_ORE) dropItem = ITEM_COAL;
                                else if (oldBlock == BLOCK_DIAMOND_ORE) dropItem = ITEM_DIAMOND;
                                else if (oldBlock == BLOCK_REDSTONE_ORE) dropItem = ITEM_REDSTONE;
                                else if (oldBlock == BLOCK_LAPIS_ORE) dropItem = ITEM_LAPIS;
                                SpawnItemEntity(dropItem, 1, bc->x * BLOCK_SIZE + 3, bc->y * BLOCK_SIZE + 3);
                            }
                        }
                        world[bc->x][bc->y] = bc->blockType;
                        if (bc->blockType == BLOCK_CROPS) RegisterCrop(bc->x, bc->y); // track network-planted crops
                        RecordBlockChange(bc->x, bc->y, bc->blockType);
                        UpdateLightAt(bc->x, bc->y);
                        InvalidateChunkAt(bc->x, bc->y);
                        // Relay to other clients
                        uint8_t relayBuf[NET_PACKET_MAX];
                        relayBuf[0] = PKT_BLOCK_CHANGE;
                        memcpy(relayBuf + 1, bc, sizeof(PktBlockChange));
                        for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                            if (r != fromId && players[r].netControlled) {
                                NetSendTo(r, relayBuf, 1 + sizeof(PktBlockChange), true);
                            }
                        }
                    }
                } else if (type == PKT_DAMAGE_MOB && size >= 1 + (int)sizeof(PktDamageMob)) {
                    const PktDamageMob *dm = (const PktDamageMob *)((const uint8_t *)data + 1);
                    if (dm->mobIndex < MAX_MOBS && mobs[dm->mobIndex].active && fromId > 0 && fromId < MAX_NET_PLAYERS) {
                        Player *rp = &players[fromId];
                        // Validate: mob must be within melee range
                        float px = rp->position.x + PLAYER_WIDTH / 2.0f;
                        float py = rp->position.y + PLAYER_HEIGHT / 2.0f;
                        float mx = mobs[dm->mobIndex].position.x + GetMobWidth(mobs[dm->mobIndex].type) / 2.0f;
                        float my = mobs[dm->mobIndex].position.y + GetMobHeight(mobs[dm->mobIndex].type) / 2.0f;
                        float dx = mx - px, dy = my - py;
                        float dist = sqrtf(dx * dx + dy * dy);
                        if (dist > BREAK_RANGE * BLOCK_SIZE * 1.5f) continue;
                        // Validate: damage must match expected weapon value
                        int expected = 1;
                        BlockType tool = (BlockType)rp->inventory[rp->selectedSlot];
                        if (IsTool(tool)) {
                            if (tool == TOOL_WOOD_SWORD) expected = 3;
                            else if (tool == TOOL_STONE_SWORD) expected = 4;
                            else if (tool == TOOL_IRON_SWORD) expected = 6;
                            else if (tool == TOOL_GOLD_SWORD) expected = 4;
                            else if (tool == TOOL_DIAMOND_SWORD) expected = 8;
                            else expected = 2;
                            // Critical hit multiplier
                            if (rp->velocity.y > CRIT_FALL_THRESHOLD)
                                expected = (int)(expected * CRIT_DAMAGE_MULT);
                        }
                        // Allow ±2 tolerance (enchantment rounding, sharpness, etc.)
                        int dmg = dm->damage;
                        if (dmg < expected - 2 || dmg > expected + 10) dmg = expected;
                        DamageMob(&mobs[dm->mobIndex], dmg);
                    }
                } else if (type == PKT_ENTITY_PICKUP && size >= 1 + (int)sizeof(PktEntityPickup)) {
                    const PktEntityPickup *ep = (const PktEntityPickup *)((const uint8_t *)data + 1);
                    if (ep->entityIndex < MAX_ENTITIES && entities[ep->entityIndex].active) {
                        entities[ep->entityIndex].active = false;
                        // Relay to other clients
                        uint8_t relayBuf[NET_PACKET_MAX];
                        relayBuf[0] = PKT_ENTITY_PICKUP;
                        memcpy(relayBuf + 1, ep, sizeof(PktEntityPickup));
                        for (int r = 1; r < MAX_NET_PLAYERS; r++) {
                            if (r != fromId && players[r].netControlled) {
                                NetSendTo(r, relayBuf, 1 + sizeof(PktEntityPickup), false);
                            }
                        }
                    }
                } else if (type == PKT_PROJECTILE_SPAWN && size >= 1 + (int)sizeof(PktProjectileSpawn)) {
                    const PktProjectileSpawn *ps = (const PktProjectileSpawn *)((const uint8_t *)data + 1);
                    int si = SpawnProjectile(ps->x, ps->y, ps->vx, ps->vy, ps->fromPlayer);
                    if (si >= 0 && ps->isFishing) projectiles[si].isFishing = true;
                    // Relay to other clients
                    uint8_t relayBuf[NET_PACKET_MAX];
                    relayBuf[0] = PKT_PROJECTILE_SPAWN;
                    memcpy(relayBuf + 1, ps, sizeof(PktProjectileSpawn));
                    for (int r = 1; r < MAX_NET_PLAYERS; r++) {
                        if (r != fromId && players[r].netControlled) {
                            NetSendTo(r, relayBuf, 1 + sizeof(PktProjectileSpawn), false);
                        }
                    }
                } else if (type == PKT_CHEST_OPEN && size >= 1 + (int)sizeof(PktChestOpen)) {
                    const PktChestOpen *po = (const PktChestOpen *)((const uint8_t *)data + 1);
                    int idx = GetOrCreateChestIndex((int)po->x, (int)po->y);
                    if (idx >= 0) {
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_CHEST_SYNC;
                        PktChestSync pkt;
                        PackChestSync(&pkt, idx);
                        memcpy(sbuf + 1, &pkt, sizeof(pkt));
                        NetSendTo(fromId, sbuf, 1 + sizeof(pkt), true);
                    }
                } else if (type == PKT_SOUND_EVENT && size >= 1 + (int)sizeof(PktSoundEvent)) {
                    const PktSoundEvent *se = (const PktSoundEvent *)((const uint8_t *)data + 1);
                    switch (se->soundId) {
                        case 0: PlaySoundBowFire(); break;
                        case 1: PlaySoundHurt(); break;
                        case 2: PlaySoundPickup(); break;
                        case 3: PlaySoundSplash(); break;
                        default: break;
                    }
                } else if (type == PKT_CHEST_SYNC && size >= 1 + (int)sizeof(PktChestSync)) {
                    const PktChestSync *pkt = (const PktChestSync *)((const uint8_t *)data + 1);
                    int idx = GetOrCreateChestIndex((int)pkt->x, (int)pkt->y);
                    if (idx >= 0) {
                        // Authoritative host: apply client's changes, then broadcast to others.
                        ApplyChestSync(pkt);
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_CHEST_SYNC;
                        memcpy(sbuf + 1, pkt, sizeof(PktChestSync));
                        for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                            if (r != fromId && players[r].netControlled) {
                                NetSendTo(r, sbuf, 1 + sizeof(PktChestSync), true);
                            }
                        }
                    }
                } else if (type == PKT_CHEST_CLOSE && size >= 1 + (int)sizeof(PktChestOpen)) {
                    // Host can track per-player open chests here if needed; currently no-op.
                } else if (type == PKT_FURNACE_OPEN && size >= 1 + (int)sizeof(PktFurnaceOpen)) {
                    const PktFurnaceOpen *po = (const PktFurnaceOpen *)((const uint8_t *)data + 1);
                    int idx = GetOrCreateFurnace((int)po->x, (int)po->y);
                    if (idx >= 0) {
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_FURNACE_SYNC;
                        PktFurnaceSync pkt;
                        PackFurnaceSync(&pkt, idx);
                        memcpy(sbuf + 1, &pkt, sizeof(pkt));
                        NetSendTo(fromId, sbuf, 1 + sizeof(pkt), true);
                    }
                } else if (type == PKT_FURNACE_SYNC && size >= 1 + (int)sizeof(PktFurnaceSync)) {
                    const PktFurnaceSync *pkt = (const PktFurnaceSync *)((const uint8_t *)data + 1);
                    int idx = GetOrCreateFurnace((int)pkt->x, (int)pkt->y);
                    if (idx >= 0) {
                        // Authoritative host: apply client's changes, then broadcast to others.
                        ApplyFurnaceSync(pkt);
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_FURNACE_SYNC;
                        memcpy(sbuf + 1, pkt, sizeof(PktFurnaceSync));
                        for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                            if (r != fromId && players[r].netControlled) {
                                NetSendTo(r, sbuf, 1 + sizeof(PktFurnaceSync), true);
                            }
                        }
                    }
                } else if (type == PKT_FURNACE_CLOSE && size >= 1 + (int)sizeof(PktFurnaceOpen)) {
                    // Host can track per-player open furnaces here if needed; currently no-op.
                } else if (type == PKT_INVENTORY_SYNC && size >= 1 + (int)sizeof(PktInventorySync)) {
                    const PktInventorySync *pkt = (const PktInventorySync *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        // Authoritative host: accept client's inventory state and relay to others.
                        ApplyInventorySync(pkt, fromId);
                        uint8_t sbuf[NET_PACKET_MAX];
                        sbuf[0] = PKT_INVENTORY_SYNC;
                        PktInventorySync relay;
                        PackInventorySync(&relay, fromId);
                        memcpy(sbuf + 1, &relay, sizeof(relay));
                        for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                            if (r != fromId && players[r].netControlled) {
                                NetSendTo(r, sbuf, 1 + sizeof(relay), true);
                            }
                        }
                    }
                } else if (type == PKT_CRAFT_REQUEST && size >= 1 + (int)sizeof(PktCraftRequest)) {
                    const PktCraftRequest *req = (const PktCraftRequest *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        int crafted = 0;
                        int limit = req->count > 0 ? req->count : 1;
                        if (limit > 64) limit = 64; // sanity cap
                        while (crafted < limit && CanCraftForPlayer(&players[fromId], req->recipeIndex)) {
                            CraftForPlayer(&players[fromId], req->recipeIndex);
                            crafted++;
                        }
                        if (crafted > 0) {
                            // Send authoritative inventory back to the crafting client and others.
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                        }
                    }
                } else if (type == PKT_BOW_REQUEST && size >= 1 + (int)sizeof(PktBowRequest)) {
                    const PktBowRequest *req = (const PktBowRequest *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        if (TryFireBowRemote(rp, req)) {
                            // Send authoritative inventory back to all clients
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                            // Broadcast projectile spawn to all clients
                            uint8_t psbuf[NET_PACKET_MAX];
                            psbuf[0] = PKT_PROJECTILE_SPAWN;
                            PktProjectileSpawn ps;
                            ps.x = req->spawnX; ps.y = req->spawnY;
                            ps.vx = req->vx; ps.vy = req->vy;
                            ps.fromPlayer = true; ps.playerId = (uint8_t)fromId;
                            ps.isFishing = false;
                            memcpy(psbuf + 1, &ps, sizeof(PktProjectileSpawn));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, psbuf, 1 + sizeof(PktProjectileSpawn), false);
                                }
                            }
                            // Sound: bow fire
                            BroadcastSound(0, rp->position.x, rp->position.y);
                        }
                    }
                } else if (type == PKT_ENDER_PEARL_REQUEST && size >= 1 + (int)sizeof(PktEnderPearlRequest)) {
                    const PktEnderPearlRequest *req = (const PktEnderPearlRequest *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        float saveX = rp->position.x, saveY = rp->position.y;
                        if (TryEnderPearlRemote(rp, req->targetX, req->targetY)) {
                            // Broadcast authoritative inventory
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                            // Broadcast teleport to all clients
                            uint8_t tpbuf[NET_PACKET_MAX];
                            tpbuf[0] = PKT_PLAYER_TELEPORT;
                            PktPlayerTeleport tp;
                            tp.playerId = (uint8_t)fromId;
                            tp.x = rp->position.x; tp.y = rp->position.y;
                            memcpy(tpbuf + 1, &tp, sizeof(PktPlayerTeleport));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, tpbuf, 1 + sizeof(PktPlayerTeleport), false);
                                }
                            }
                            // Apply teleport to host's remote player interpolation
                            remotePlayers[fromId].interpX = rp->position.x;
                            remotePlayers[fromId].interpY = rp->position.y;
                            // Sound: ender pearl teleport
                            BroadcastSound(1, rp->position.x, rp->position.y);
                        }
                    }
                } else if (type == PKT_ENCHANT_REQUEST && size >= 1 + (int)sizeof(PktEnchantRequest)) {
                    const PktEnchantRequest *req = (const PktEnchantRequest *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        if (TryEnchantRemote(rp, req->blockX, req->blockY, req->enchantType, req->enchantLevel, req->xpCost)) {
                            // Broadcast authoritative inventory
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                        }
                    }
                } else if (type == PKT_FISHING_REQUEST && size >= 1 + (int)sizeof(PktFishingRequest)) {
                    const PktFishingRequest *req = (const PktFishingRequest *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        if (TryFishingRemote(rp, req->action, req->vx, req->vy)) {
                            // Broadcast inventory sync
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                            // Broadcast projectile spawn (fishing bobber)
                            if (req->action == 0) {
                                uint8_t psbuf[NET_PACKET_MAX];
                                psbuf[0] = PKT_PROJECTILE_SPAWN;
                                PktProjectileSpawn ps;
                                float px = rp->position.x + PLAYER_WIDTH / 2.0f;
                                float py = rp->position.y + PLAYER_HEIGHT / 2.0f;
                                float aimDist = sqrtf(req->vx * req->vx + req->vy * req->vy);
                                float speed = PROJECTILE_SPEED * 0.8f;
                                ps.x = px; ps.y = py;
                                ps.vx = aimDist > 0.01f ? (req->vx / aimDist) * speed : 0;
                                ps.vy = aimDist > 0.01f ? (req->vy / aimDist) * speed : 0;
                                ps.fromPlayer = false; ps.playerId = 0;
                                ps.isFishing = true;
                                memcpy(psbuf + 1, &ps, sizeof(PktProjectileSpawn));
                                for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                    if (players[r].netControlled) {
                                        NetSendTo(r, psbuf, 1 + sizeof(PktProjectileSpawn), false);
                                    }
                                }
                                // Sound: fishing cast
                                BroadcastSound(3, rp->position.x, rp->position.y);
                            } // else retract handled by TryFishingRemote's catch logic
                        }
                    }
                } else if (type == PKT_CAULDRON_SYNC && size >= 1 + (int)sizeof(PktCauldronSync)) {
                    const PktCauldronSync *req = (const PktCauldronSync *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        if (TryCauldronInteractRemote(rp, (int)req->x, (int)req->y)) {
                            // Broadcast cauldron state to all clients
                            int cdIdx = -1;
                            for (int ci = 0; ci < cauldronCount; ci++) {
                                if (cauldrons[ci].x == (int)req->x && cauldrons[ci].y == (int)req->y) {
                                    cdIdx = ci;
                                    break;
                                }
                            }
                            if (cdIdx >= 0) {
                                uint8_t csbuf[NET_PACKET_MAX];
                                csbuf[0] = PKT_CAULDRON_SYNC;
                                PktCauldronSync cs;
                                cs.x = cauldrons[cdIdx].x; cs.y = cauldrons[cdIdx].y;
                                cs.fillLevel = cauldrons[cdIdx].fillLevel;
                                memcpy(csbuf + 1, &cs, sizeof(PktCauldronSync));
                                for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                    if (players[r].netControlled) {
                                        NetSendTo(r, csbuf, 1 + sizeof(PktCauldronSync), true);
                                    }
                                }
                            }
                            // Broadcast inventory sync
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                        }
                    }
                } else if (type == PKT_ITEM_DROP && size >= 1 + (int)sizeof(PktItemDrop)) {
                    const PktItemDrop *req = (const PktItemDrop *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        Player *rp = &players[fromId];
                        if (TryItemDropRemote(rp, req->slot)) {
                            // Broadcast authoritative inventory
                            uint8_t sbuf[NET_PACKET_MAX];
                            sbuf[0] = PKT_INVENTORY_SYNC;
                            PktInventorySync ipkt;
                            PackInventorySync(&ipkt, fromId);
                            memcpy(sbuf + 1, &ipkt, sizeof(ipkt));
                            for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                                if (players[r].netControlled) {
                                    NetSendTo(r, sbuf, 1 + sizeof(ipkt), true);
                                }
                            }
                        }
                    }
                } else if (type == PKT_ACHIEVEMENT_UNLOCK && size >= 1 + (int)sizeof(PktAchievementUnlock)) {
                    const PktAchievementUnlock *au = (const PktAchievementUnlock *)((const uint8_t *)data + 1);
                    // Relay to all other clients
                    uint8_t relayBuf[NET_PACKET_MAX];
                    relayBuf[0] = PKT_ACHIEVEMENT_UNLOCK;
                    memcpy(relayBuf + 1, au, sizeof(PktAchievementUnlock));
                    for (int r = 1; r < NET_MAX_PLAYERS; r++) {
                        if (r != fromId && players[r].netControlled) {
                            NetSendTo(r, relayBuf, 1 + sizeof(PktAchievementUnlock), true);
                        }
                    }
                } else if (type == PKT_CHAT && size >= 1 + (int)sizeof(PktChat)) {
                    const PktChat *pkt = (const PktChat *)((const uint8_t *)data + 1);
                    if (fromId > 0 && fromId < MAX_NET_PLAYERS && players[fromId].netControlled) {
                        // Host receives chat from client and rebroadcasts to everyone.
                        BroadcastChatMessage((uint8_t)fromId, pkt->message);
                    }
                } else if (type == PKT_PING && size >= 1 + (int)sizeof(PktPing)) {
                    // Respond to ping (keep-alive)
                    uint8_t pongBuf[NET_PACKET_MAX];
                    pongBuf[0] = PKT_PING;
                    memcpy(pongBuf + 1, (const uint8_t *)data + 1, sizeof(PktPing));
                    NetSendTo(fromId, pongBuf, 1 + sizeof(PktPing), false);
                }
            }
            // Timeout check for remote players
            {
                float now = NetGetTime();
                for (int i = 0; i < NetGetReceivedCount(); i++) {
                    int sz, fid;
                    const void *d = NetGetReceived(i, &sz, &fid);
                    if (d && fid > 0 && fid < MAX_NET_PLAYERS) {
                        // Any packet counts as keepalive
                        lastInputTime[fid] = now;
                    }
                }
                for (int i = 1; i < MAX_NET_PLAYERS; i++) {
                    if (remotePlayers[i].active && (now - lastInputTime[i]) > NET_TIMEOUT) {
                        remotePlayers[i].active = false;
                        players[i].netControlled = false;
                        memset(&players[i], 0, sizeof(Player));
                        ShowMessage(S(STR_NET_PLAYER_LEFT), (Color){240, 200, 100, 255});
                    }
                }
            }
            // Run authoritative game logic
            if (!chatOpen) UpdatePlayer(dt);
            // Update remote players' physics
            {
                int savedLocalId = localPlayerId;
                for (int i = 1; i < MAX_NET_PLAYERS; i++) {
                    if (players[i].netControlled) {
                        localPlayerId = i;
                        UpdatePlayer(dt);
                    }
                }
                localPlayerId = savedLocalId;
            }
            UpdateMobs(dt);
            UpdateProjectiles(dt);
            ProcessPendingProjectileHits();
            UpdateXpOrbs(dt);
            UpdateEntities(dt);
            UpdateParticles(dt);
            // Pickup items for host player only (clients pick up on their side)
            PickupAndSyncItems(player.position.x, player.position.y, 0);
            UpdateCameraSystem(dt);
            UpdateWeather(dt);
            UpdateRainAmbient();
            UpdateAmbientSounds();
            if (player.damageFlashTimer > 0.0f) player.damageFlashTimer -= dt;
            // Broadcast state to clients
            {
                netTickTimer += dt;
                if (netTickTimer >= NET_TICK_INTERVAL) {
                    netTickTimer = 0.0f;
                    // Send player state
                    PktPlayerState ps;
                    ps.count = 0;
                    for (int i = 0; i < MAX_NET_PLAYERS; i++) {
                        if (i == 0 || players[i].netControlled) {
                            PktPlayerInfo *pi = &ps.players[ps.count++];
                            pi->playerId = (uint8_t)i;
                            pi->x = players[i].position.x;
                            pi->y = players[i].position.y;
                            pi->vx = players[i].velocity.x;
                            pi->vy = players[i].velocity.y;
                            pi->facingRight = players[i].facingRight;
                            pi->sprinting = players[i].sprinting;
                            pi->onGround = players[i].onGround;
                            pi->selectedSlot = players[i].selectedSlot;
                            pi->health = players[i].health;
                            memcpy(pi->armor, players[i].armor, 4);
                            snprintf(pi->playerName, sizeof(pi->playerName), "%s", players[i].playerName);
                        }
                    }
                    uint8_t buf[NET_PACKET_MAX];
                    buf[0] = PKT_PLAYER_STATE;
                    memcpy(buf + 1, &ps, sizeof(PktPlayerState));
                    NetSendToAll(buf, 1 + sizeof(PktPlayerState), false);
                    // Send mob state
                    PktMobState ms;
                    ms.count = 0;
                    for (int i = 0; i < MAX_MOBS && ms.count < 32; i++) {
                        if (mobs[i].active) {
                            PktMobInfo *mi = &ms.mobs[ms.count++];
                            mi->index = (uint8_t)i;
                            mi->type = (uint8_t)mobs[i].type;
                            mi->x = mobs[i].position.x;
                            mi->y = mobs[i].position.y;
                            mi->vx = mobs[i].velocity.x;
                            mi->vy = mobs[i].velocity.y;
                            mi->health = mobs[i].health;
                            mi->active = true;
                            mi->facingRight = mobs[i].facingRight;
                        }
                    }
                    buf[0] = PKT_MOB_STATE;
                    memcpy(buf + 1, &ms, sizeof(PktMobState));
                    NetSendToAll(buf, 1 + sizeof(PktMobState), false);
                    // Send time sync
                    PktTimeSync ts;
                    ts.timeOfDay = dayNight.timeOfDay;
                    buf[0] = PKT_TIME_SYNC;
                    memcpy(buf + 1, &ts, sizeof(PktTimeSync));
                    NetSendToAll(buf, 1 + sizeof(PktTimeSync), false);
                }
                // Send weather sync less frequently (~every 5s)
                static float weatherSyncTimer = 0.0f;
                weatherSyncTimer += dt;
                if (weatherSyncTimer >= 5.0f) {
                    weatherSyncTimer = 0.0f;
                    uint8_t wbuf[NET_PACKET_MAX];
                    PktWeatherSync ws;
                    ws.weatherType = (uint8_t)weather.type;
                    ws.duration = weather.duration;
                    wbuf[0] = PKT_WEATHER_SYNC;
                    memcpy(wbuf + 1, &ws, sizeof(PktWeatherSync));
                    NetSendToAll(wbuf, 1 + sizeof(PktWeatherSync), false);
                }
            }
        } else if (NetIsClient()) {
            // Client mode: send input, receive state
            NetPoll();
            // Send local input to server (skip while any UI overlay is open so
            // opening inventory/creative palette doesn't also trigger a use action)
            if (!chatOpen && !inventoryOpen && !creativeOpen) {
                inputTickTimer += dt;
                if (inputTickTimer >= NET_TICK_INTERVAL) {
                    inputTickTimer = 0.0f;
                    // Capture raw keyboard state
                    bool left = IsMoveLeftDown();
                    bool right = IsMoveRightDown();
                    float moveX = 0.0f;
                    if (left && !right) moveX = -1.0f;
                    else if (right && !left) moveX = 1.0f;
                    bool jumpKey = IsJumpDown();
                    bool sprintKey = IsSprintDown();
                    PktInput input;
                    input.moveX = moveX;
                    input.jump = jumpKey;
                    input.sprint = sprintKey;
                    input.attack = Win32IsMouseButtonDown(MOUSE_BUTTON_LEFT);
                    input.place = Win32IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
                    input.use = Win32IsKeyPressed(KEY_E);
                    Vector2 mouseWorld = GetScreenToWorld2D(Win32GetMousePosition(), camera);
                    input.cursorX = mouseWorld.x;
                    input.cursorY = mouseWorld.y;
                    input.selectedSlot = player.selectedSlot;
                    uint8_t buf[NET_PACKET_MAX];
                    buf[0] = PKT_INPUT;
                    memcpy(buf + 1, &input, sizeof(PktInput));
                    NetSendToServer(buf, 1 + sizeof(PktInput), false);
                }
            }
            // Process received packets from server
            for (int i = 0; i < NetGetReceivedCount(); i++) {
                int size, fromId;
                const void *data = NetGetReceived(i, &size, &fromId);
                if (!data) continue;
                uint8_t type = ((const uint8_t *)data)[0];
                if (type == PKT_FLUID_DELTA && size >= 2) {
                    const uint8_t *raw = (const uint8_t *)data;
                    int count = raw[1];
                    int pos = 2;
                    for (int fi = 0; fi < count && pos + (int)sizeof(PktFluidCell) <= size; fi++) {
                        PktFluidCell cell;
                        memcpy(&cell, raw + pos, sizeof(cell));
                        RestoreFluidState(cell.x, cell.y, cell.blockType, cell.kind, cell.level, cell.source != 0);
                        InvalidateChunkAt(cell.x, cell.y);
                        pos += sizeof(cell);
                    }
                } else if (type == PKT_FLUID_SNAPSHOT && size >= 1 + (int)sizeof(PktFluidSnapshotHeader)) {
                    const uint8_t *raw = (const uint8_t *)data;
                    PktFluidSnapshotHeader header;
                    memcpy(&header, raw + 1, sizeof(header));
                    int count = header.count;
                    int pos = 1 + (int)sizeof(header);
                    for (int fi = 0; fi < count && pos + (int)sizeof(PktFluidCell) <= size; fi++) {
                        PktFluidCell cell;
                        memcpy(&cell, raw + pos, sizeof(cell));
                        RestoreFluidState(cell.x, cell.y, cell.blockType, cell.kind, cell.level, cell.source != 0);
                        InvalidateChunkAt(cell.x, cell.y);
                        pos += sizeof(cell);
                    }
                    PktFluidRequest ack = { header.snapshotId, header.batchIndex, FLUID_REQUEST_ACK_SNAPSHOT, 0 };
                    uint8_t ackBuf[NET_PACKET_MAX]; ackBuf[0] = PKT_FLUID_REQUEST;
                    memcpy(ackBuf + 1, &ack, sizeof(ack));
                    NetSendToServer(ackBuf, 1 + sizeof(ack), true);
                } else if (type == PKT_PLAYER_STATE && size >= 1 + (int)sizeof(PktPlayerState)) {
                    const PktPlayerState *ps = (const PktPlayerState *)((const uint8_t *)data + 1);
                    for (int j = 0; j < ps->count; j++) {
                        const PktPlayerInfo *pi = &ps->players[j];
                        int pid = pi->playerId;
                        if (pid >= 0 && pid < MAX_NET_PLAYERS && pid != localPlayerId) {
                            if (!remotePlayers[pid].active) {
                                // First update: snap interpolation to avoid teleporting
                                remotePlayers[pid].interpX = pi->x;
                                remotePlayers[pid].interpY = pi->y;
                            }
                            players[pid].position.x = pi->x;
                            players[pid].position.y = pi->y;
                            players[pid].velocity.x = pi->vx;
                            players[pid].velocity.y = pi->vy;
                            players[pid].facingRight = pi->facingRight;
                            players[pid].sprinting = pi->sprinting;
                            players[pid].onGround = pi->onGround;
                            players[pid].selectedSlot = pi->selectedSlot;
                            players[pid].health = pi->health;
                            memcpy(players[pid].armor, pi->armor, 4);
                            snprintf(players[pid].playerName, sizeof(players[pid].playerName), "%s", pi->playerName);
                            remotePlayers[pid].active = true;
                        } else if (pid == localPlayerId) {
                            // Server reconciliation: correct position if diverged
                            float dx = pi->x - player.position.x;
                            float dy = pi->y - player.position.y;
                            float dist2 = dx * dx + dy * dy;
                            if (dist2 > 4.0f) {
                                // Large divergence: snap to server position
                                player.position.x = pi->x;
                                player.position.y = pi->y;
                            } else if (dist2 > 0.5f) {
                                // Small divergence: blend toward server position
                                player.position.x += dx * 0.3f;
                                player.position.y += dy * 0.3f;
                            }
                            player.health = pi->health;
                        }
                    }
                } else if (type == PKT_MOB_STATE && size >= 1 + (int)sizeof(PktMobState)) {
                    const PktMobState *ms = (const PktMobState *)((const uint8_t *)data + 1);
                    // First deactivate all mobs, then activate those in the packet
                    for (int j = 0; j < MAX_MOBS; j++) mobs[j].active = false;
                    for (int j = 0; j < ms->count && j < 32; j++) {
                        const PktMobInfo *mi = &ms->mobs[j];
                        int idx = mi->index;
                        if (idx >= 0 && idx < MAX_MOBS) {
                            mobs[idx].type = (MobType)mi->type;
                            mobs[idx].position.x = mi->x;
                            mobs[idx].position.y = mi->y;
                            mobs[idx].velocity.x = mi->vx;
                            mobs[idx].velocity.y = mi->vy;
                            mobs[idx].health = mi->health;
                            mobs[idx].active = mi->active;
                            mobs[idx].facingRight = mi->facingRight;
                        }
                    }
                } else if (type == PKT_BLOCK_CHANGE) {
                    // Support batch block changes (multiple PktBlockChange per packet)
                    int count = (size - 1) / (int)sizeof(PktBlockChange);
                    for (int j = 0; j < count; j++) {
                        const PktBlockChange *bc = (const PktBlockChange *)((const uint8_t *)data + 1 + j * sizeof(PktBlockChange));
                        if (bc->x < WORLD_WIDTH && bc->y < WORLD_HEIGHT) {
                            world[bc->x][bc->y] = bc->blockType;
                            if (bc->blockType == BLOCK_CROPS) RegisterCrop(bc->x, bc->y);
                            UpdateLightAt(bc->x, bc->y);
                            InvalidateChunkAt(bc->x, bc->y);
                        }
                    }
                } else if (type == PKT_ENTITY_SPAWN && size >= 1 + (int)sizeof(PktEntitySpawn)) {
                    const PktEntitySpawn *es = (const PktEntitySpawn *)((const uint8_t *)data + 1);
                    // Dedup: skip if an entity of same type already exists nearby (can happen when client
                    // already spawned the item from PKT_BLOCK_CHANGE processing)
                    bool found = false;
                    for (int ei = 0; ei < MAX_ENTITIES; ei++) {
                        if (!entities[ei].active) continue;
                        if (entities[ei].itemType != es->itemType) continue;
                        float dx = entities[ei].position.x - es->x;
                        float dy = entities[ei].position.y - es->y;
                        if (dx * dx + dy * dy < 64.0f) { found = true; break; }
                    }
                    if (!found) SpawnItemEntity((BlockType)es->itemType, es->count, es->x, es->y);
                } else if (type == PKT_ENTITY_PICKUP && size >= 1 + (int)sizeof(PktEntityPickup)) {
                    const PktEntityPickup *ep = (const PktEntityPickup *)((const uint8_t *)data + 1);
                    if (ep->entityIndex < MAX_ENTITIES) {
                        entities[ep->entityIndex].active = false;
                    }
                } else if (type == PKT_PROJECTILE_SPAWN && size >= 1 + (int)sizeof(PktProjectileSpawn)) {
                    const PktProjectileSpawn *ps = (const PktProjectileSpawn *)((const uint8_t *)data + 1);
                    int si = SpawnProjectile(ps->x, ps->y, ps->vx, ps->vy, ps->fromPlayer);
                    if (si >= 0 && ps->isFishing) projectiles[si].isFishing = true;
                } else if (type == PKT_TIME_SYNC && size >= 1 + (int)sizeof(PktTimeSync)) {
                    const PktTimeSync *ts = (const PktTimeSync *)((const uint8_t *)data + 1);
                    dayNight.timeOfDay = ts->timeOfDay;
                } else if (type == PKT_WEATHER_SYNC && size >= 1 + (int)sizeof(PktWeatherSync)) {
                    const PktWeatherSync *ws = (const PktWeatherSync *)((const uint8_t *)data + 1);
                    weather.type = (WeatherType)ws->weatherType;
                    weather.duration = ws->duration;
                    weather.transitionTimer = 0;
                    weather.rainAlpha = (weather.type == WEATHER_CLEAR) ? 0 : 1;
                } else if (type == PKT_CAULDRON_SYNC && size >= 1 + (int)sizeof(PktCauldronSync)) {
                    const PktCauldronSync *cs = (const PktCauldronSync *)((const uint8_t *)data + 1);
                    int cdIdx = -1;
                    for (int ci = 0; ci < cauldronCount; ci++) {
                        if (cauldrons[ci].x == (int)cs->x && cauldrons[ci].y == (int)cs->y) {
                            cdIdx = ci;
                            break;
                        }
                    }
                    if (cdIdx < 0 && cauldronCount < MAX_CAULDRONS) {
                        cdIdx = cauldronCount++;
                        cauldrons[cdIdx].x = (int)cs->x;
                        cauldrons[cdIdx].y = (int)cs->y;
                    }
                    if (cdIdx >= 0) {
                        cauldrons[cdIdx].fillLevel = cs->fillLevel;
                    }
                } else if (type == PKT_DAMAGE_PLAYER && size >= 1 + (int)sizeof(PktDamagePlayer)) {
                    const PktDamagePlayer *dp = (const PktDamagePlayer *)((const uint8_t *)data + 1);
                    if (dp->playerId == localPlayerId && gameMode != GAME_CREATIVE) {
                        player.health -= dp->damage;
                        player.velocity.x += dp->knockbackX;
                        player.velocity.y += dp->knockbackY;
                        player.damageFlashTimer = 0.3f;
                    }
                } else if (type == PKT_PLAYER_TELEPORT && size >= 1 + (int)sizeof(PktPlayerTeleport)) {
                    const PktPlayerTeleport *tp = (const PktPlayerTeleport *)((const uint8_t *)data + 1);
                    if (tp->playerId < MAX_NET_PLAYERS) {
                        if (tp->playerId == localPlayerId) {
                            // Host teleported us; apply to local player
                            player.position.x = tp->x;
                            player.position.y = tp->y;
                            player.velocity.x = 0;
                            player.velocity.y = 0;
                        } else if (players[tp->playerId].netControlled) {
                            // Remote player teleported; update interpolation
                            players[tp->playerId].position.x = tp->x;
                            players[tp->playerId].position.y = tp->y;
                            remotePlayers[tp->playerId].interpX = tp->x;
                            remotePlayers[tp->playerId].interpY = tp->y;
                        }
                    }
                } else if (type == PKT_ACHIEVEMENT_UNLOCK && size >= 1 + (int)sizeof(PktAchievementUnlock)) {
                    const PktAchievementUnlock *au = (const PktAchievementUnlock *)((const uint8_t *)data + 1);
                    if (au->achievementId < ACH_COUNT) {
                        if (!achievements[au->achievementId]) {
                            achievements[au->achievementId] = true;
                            ShowMessage(S(GetAchString((Achievement)au->achievementId)),
                                       (Color){255, 215, 0, 255});
                            PlaySoundXP();
                        }
                    }
                } else if (type == PKT_DISCONNECT && size >= 2) {
                    // Host disconnected, return to menu
                    NetClientDisconnect();
                    ShowMessage(S(STR_NET_HOST_DISCONNECTED), (Color){240, 100, 100, 255});
                    StartTransition(STATE_MENU);
                    menuSelection = 0;
                    return;
                } else if (type == PKT_GAMEMODE_SYNC && size >= 2) {
                    // Host changed the game mode
                    uint8_t gm = ((const uint8_t *)data)[1];
                    if (gm < GAME_MODE_COUNT) {
                        gameMode = (GameMode)gm;
                        AddChatMessage(255, gameMode == GAME_CREATIVE
                                       ? "Game mode: Creative (E opens creative inventory)"
                                       : "Game mode: Survival");
                        // Creative palette state is per-client UI; close it if we left creative.
                        if (gameMode != GAME_CREATIVE) {
                            creativeOpen = false;
                            gamePaused = false;
                        }
                    }
                } else if (type == PKT_CHEST_SYNC && size >= 1 + (int)sizeof(PktChestSync)) {
                    const PktChestSync *pkt = (const PktChestSync *)((const uint8_t *)data + 1);
                    ApplyChestSync(pkt);
                    // If we have a pending open request for this chest, open the UI now.
                    if (!chestOpen && pkt->x == chestBlockX && pkt->y == chestBlockY) {
                        chestOpen = true;
                        inventoryOpen = true;
                        gamePaused = false;
                        PlaySoundCraft();
                    }
                } else if (type == PKT_FURNACE_SYNC && size >= 1 + (int)sizeof(PktFurnaceSync)) {
                    const PktFurnaceSync *pkt = (const PktFurnaceSync *)((const uint8_t *)data + 1);
                    ApplyFurnaceSync(pkt);
                    // If we have a pending open request for this furnace, open the UI now.
                    if (!furnaceOpen && pkt->x == furnaceBlockX && pkt->y == furnaceBlockY) {
                        activeFurnace = FindFurnace((int)pkt->x, (int)pkt->y);
                        if (activeFurnace >= 0) {
                            SyncFurnaceToActive(activeFurnace);
                            furnaceOpen = true;
                            inventoryOpen = true;
                            gamePaused = false;
                            PlaySoundCraft();
                        }
                    }
                } else if (type == PKT_INVENTORY_SYNC && size >= 1 + (int)sizeof(PktInventorySync)) {
                    const PktInventorySync *pkt = (const PktInventorySync *)((const uint8_t *)data + 1);
                    int pid = pkt->playerId;
                    if (pid == localPlayerId && pid >= 0 && pid < MAX_NET_PLAYERS) {
                        // Host is the authority: overwrite local inventory with host's view.
                        ApplyInventorySync(pkt, pid);
                    }
                } else if (type == PKT_CHAT && size >= 1 + (int)sizeof(PktChat)) {
                    const PktChat *pkt = (const PktChat *)((const uint8_t *)data + 1);
                    AddChatMessage(pkt->playerId, pkt->message);
                } else if (type == PKT_PING && size >= 1 + (int)sizeof(PktPing)) {
                    // Server ping response — just counts as keepalive
                }
            }
            // Client-side keep-alive: send ping every 3s, detect host timeout
            {
                static float clientPingTimer = 0.0f;
                static float lastServerPacket = 0.0f;
                clientPingTimer += dt;
                if (clientPingTimer >= 3.0f) {
                    clientPingTimer = 0.0f;
                    PktPing pp;
                    pp.timestamp = (uint32_t)(NetGetTime() * 1000.0f);
                    uint8_t pbuf[NET_PACKET_MAX];
                    pbuf[0] = PKT_PING;
                    memcpy(pbuf + 1, &pp, sizeof(PktPing));
                    NetSendToServer(pbuf, 1 + sizeof(PktPing), false);
                }
                // Periodic full-inventory sync safety net (mining drops, pickups, etc.)
                {
                    static float clientInvSyncTimer = 0.0f;
                    clientInvSyncTimer += dt;
                    if (clientInvSyncTimer >= 2.0f) {
                        clientInvSyncTimer = 0.0f;
                        SyncInventoryToHost();
                    }
                }
                // Any received packet keeps connection alive
                if (NetGetReceivedCount() > 0) lastServerPacket = NetGetTime();
                // Timeout: no packets from server for 15s
                if (lastServerPacket > 0.1f && NetGetTime() - lastServerPacket > NET_TIMEOUT) {
                    NetClientDisconnect();
                    ShowMessage(S(STR_NET_HOST_DISCONNECTED), (Color){240, 100, 100, 255});
                    StartTransition(STATE_MENU);
                    menuSelection = 0;
                    return;
                }
            }
            // Client-side: update local player, entities, particles, camera
            if (!chatOpen) UpdatePlayer(dt);
            UpdateEntities(dt);
            UpdateParticles(dt);
            PickupAndSyncItems(player.position.x, player.position.y, localPlayerId);
            UpdateCameraSystem(dt);
            if (player.damageFlashTimer > 0.0f) player.damageFlashTimer -= dt;
        } else {
            // Single-player mode: original logic
            if (!chatOpen) UpdatePlayer(dt);
            UpdateMobs(dt);
            UpdateProjectiles(dt);
            ProcessPendingProjectileHits();
            UpdateXpOrbs(dt);
            UpdateEntities(dt);
            UpdateParticles(dt);
            PickupNearbyItems(player.position.x, player.position.y);
            UpdateCameraSystem(dt);
            UpdateWeather(dt);
            UpdateRainAmbient();
            UpdateAmbientSounds();
            if (player.damageFlashTimer > 0.0f) player.damageFlashTimer -= dt;
        }
        FlushFluidChanges();
    }
    // Status effects (drowning, hunger) apply even with inventory open
    if (!gamePaused) {
        UpdatePlayerStatus(dt);
        UpdateFurnaceTick(dt);
    }
    UpdateChunks();
    if (!chatOpen) UpdateHotbar();
    UpdateRedstoneTick();
    UpdateCrops(dt);
    UpdatePrimedTnt(dt);
    CheckAchievements();

    // Distance checks: auto-close crafting table/furnace if player moves away
    if (craftingTableOpen || furnaceOpen || chestOpen) {
        int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
        int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
        bool nearBlock = false;
        for (int dx = -1; dx <= 1 && !nearBlock; dx++) {
            for (int dy = -1; dy <= 1 && !nearBlock; dy++) {
                int tx = bx + dx, ty = by + dy;
                if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                    if (craftingTableOpen && world[tx][ty] == BLOCK_CRAFTING_TABLE) nearBlock = true;
                    if (furnaceOpen && world[tx][ty] == BLOCK_FURNACE) nearBlock = true;
                    if (chestOpen && world[tx][ty] == BLOCK_CHEST) nearBlock = true;
                }
            }
        }
        if (!nearBlock) {
            if (furnaceOpen) {
                CloseFurnaceNetwork();
                ReturnFurnaceItems();
                furnaceOpen = false;
            }
            if (chestOpen) CloseChestNetwork();
            chestOpen = false;
            craftingTableOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        }
    }

    // Update chat message timers
    for (int i = 0; i < chatHistoryCount; i++) {
        chatHistory[i].timer -= dt;
    }
}

void DrawGame(void)
{
    BeginDrawing();
    if (logicalCanvasReady) {
        BeginTextureMode(logicalCanvas);
    }
    ClearBackground(GetSkyColor());

    if (gameState == STATE_MENU) {
        DrawMainMenu();
        if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);
        DrawTransition();
        goto draw_finish;
    }

    if (gameState == STATE_SLOT_SELECT) {
        DrawSlotSelectScreen();
        if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);
        DrawTransition();
        goto draw_finish;
    }

    if (gameState == STATE_SETTINGS) {
        DrawSettingsScreen();
        if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);
        DrawTransition();
        goto draw_finish;
    }

    if (gameState == STATE_HOST_WAITING) {
        DrawBackground();
        // Dark overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});
        // Centered host card and explicit start affordance.
        DrawRectangle(300, 140, 680, 390, (Color){10, 16, 28, 225});
        DrawRectangleLines(300, 140, 680, 390, (Color){88, 112, 145, 220});
        DrawRectangle(350, 142, 580, 2, (Color){255, 185, 90, 180});
        // Title
        const char *title = S(STR_HOST_WAITING);
        int titleW = MeasureGameTextWidth(title, 32);
        DrawGameText(title, (SCREEN_WIDTH - titleW) / 2, 200, 32, (Color){200, 220, 255, 255});
        // Show local IP
        char localIp[64];
        NetGetLocalIP(localIp, sizeof(localIp));
        char ipMsg[128];
        snprintf(ipMsg, sizeof(ipMsg), S(STR_HOST_IP_HINT), localIp, NET_PORT);
        int ipW = MeasureGameTextWidth(ipMsg, 20);
        DrawGameText(ipMsg, (SCREEN_WIDTH - ipW) / 2, 260, 20, (Color){180, 180, 200, 200});
        // Connected players count
        char countMsg[64];
        snprintf(countMsg, sizeof(countMsg), S(STR_HOST_PLAYERS_COUNT), NetGetPlayerCount(), NET_MAX_PLAYERS);
        int countW = MeasureGameTextWidth(countMsg, 20);
        const char *startHint = S(STR_HOST_START_HINT);
        DrawUiButton((SCREEN_WIDTH - 300) / 2, 340, 300, 46, startHint, 16,
                     false, true, true, 1.0f);
        // ESC hint
        const char *hint = S(STR_HOST_CANCEL_HINT);
        int hintW = MeasureGameTextWidth(hint, 18);
        DrawGameText(hint, (SCREEN_WIDTH - hintW) / 2, 400, 18, (Color){150, 150, 170, 180});
        if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);
        DrawTransition();
        goto draw_finish;
    }

    if (gameState == STATE_JOIN_GAME) {
        DrawBackground();
        // Dark overlay
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 160});
        if (!joinConnecting) {
            // Modern dark-flat panel behind the join form
            int jPanelW = 440, jPanelH = 250;
            int jPanelX = (SCREEN_WIDTH - jPanelW) / 2;
            int jPanelY = 175;
            DrawRectangle(jPanelX + 3, jPanelY + 3, jPanelW, jPanelH, (Color){0, 0, 0, 60});
            DrawRectangle(jPanelX, jPanelY, jPanelW, jPanelH, (Color){31, 39, 52, 240});
            DrawRectangleLines(jPanelX, jPanelY, jPanelW, jPanelH, (Color){58, 71, 92, 255});
            DrawRectangle(jPanelX + 12, jPanelY, jPanelW - 24, 1, (Color){255, 255, 255, 16});

            // Title
            const char *title = S(STR_JOIN_TITLE);
            int titleW = MeasureGameTextWidth(title, 32);
            DrawGameText(title, (SCREEN_WIDTH - titleW) / 2, 200, 32, (Color){235, 235, 235, 255});
            // IP hint
            const char *hint = S(STR_JOIN_IP_HINT);
            int hintW = MeasureGameTextWidth(hint, 20);
            DrawGameText(hint, (SCREEN_WIDTH - hintW) / 2, 260, 20, (Color){180, 180, 200, 200});
            // Name input box
            int boxW = 300, boxH = 36;
            int boxX = (SCREEN_WIDTH - boxW) / 2;
            int boxY = 250; // above IP
            bool nameFocused = (joinInputFocus == 0);
            DrawRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (Color){24, 29, 38, 230});
            DrawRectangleLinesEx((Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH}, 2,
                nameFocused ? (Color){56, 217, 169, 255} : (Color){58, 71, 92, 220});
            DrawGameText(joinNameBuf, boxX + 10, boxY + 8, 20, (Color){220, 230, 255, 255});
            if (joinInputFocus == 0) {
                float blink = sinf((float)GetTime() * 4.0f) * 0.5f + 0.5f;
                int cursorX = boxX + 10 + MeasureGameTextWidth(joinNameBuf, 20);
                DrawRectangle(cursorX, boxY + 6, 2, boxH - 12, (Color){220, 230, 255, (unsigned char)(blink * 255)});
            }
            // IP input box
            boxY = 300;
            bool ipFocused = (joinInputFocus == 1);
            DrawRectangle(boxX + 1, boxY + 1, boxW - 2, boxH - 2, (Color){24, 29, 38, 230});
            DrawRectangleLinesEx((Rectangle){(float)boxX, (float)boxY, (float)boxW, (float)boxH}, 2,
                ipFocused ? (Color){56, 217, 169, 255} : (Color){58, 71, 92, 220});
            DrawGameText(joinIpBuf, boxX + 10, boxY + 8, 20, (Color){220, 230, 255, 255});
            if (joinInputFocus == 1) {
                float blink = sinf((float)GetTime() * 4.0f) * 0.5f + 0.5f;
                int cursorX = boxX + 10 + MeasureGameTextWidth(joinIpBuf, 20);
                DrawRectangle(cursorX, boxY + 6, 2, boxH - 12, (Color){220, 230, 255, (unsigned char)(blink * 255)});
            }
            // ESC hint
            const char *escHint = S(STR_JOIN_HELP_HINT);
            int escW = MeasureGameTextWidth(escHint, 16);
            DrawGameText(escHint, (SCREEN_WIDTH - escW) / 2, 360, 16, (Color){150, 150, 170, 180});
            // Tab hint
            const char *tabHint = S(STR_JOIN_TAB_HINT);
            int tabW = MeasureGameTextWidth(tabHint, 14);
            DrawGameText(tabHint, (SCREEN_WIDTH - tabW) / 2, 380, 14, (Color){130, 140, 150, 160});
        } else {
            // Connecting screen
            const char *title = S(STR_JOIN_CONNECTING);
            int titleW = MeasureGameTextWidth(title, 32);
            DrawGameText(title, (SCREEN_WIDTH - titleW) / 2, 260, 32, (Color){200, 220, 255, 255});
            // Animated dots
            int dots = ((int)(GetTime() * 3.0f)) % 4;
            char dotsBuf[8];
            for (int d = 0; d < dots; d++) dotsBuf[d] = '.';
            dotsBuf[dots] = '\0';
            DrawGameText(dotsBuf, (SCREEN_WIDTH + titleW) / 2 + 4, 260, 32, (Color){200, 220, 255, 200});
        }
        if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);
        DrawTransition();
        goto draw_finish;
    }

    DrawBackground();

    BeginMode2D(camera);
    DrawWorld();
    DrawFireEffects(GetFrameTime());
    DrawMiningCrack();
    DrawWater();
    DrawMobs();
    DrawProjectiles();
    DrawXpOrbs();
    DrawEntities();
    DrawParticles();
    DrawRemotePlayers();
    if (!inventoryOpen && !gamePaused && !player.playerDead) DrawCrosshair();
    DrawPlayerSprite();
    EndMode2D();

    // Weather effects (rain, lightning)
    DrawWeather();

    if (dayNight.lightLevel < 1.0f) {
        unsigned char alpha = (unsigned char)(120 * (1.0f - dayNight.lightLevel));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){5, 5, 25, alpha});
    }

    // Underwater blue overlay
    if (IsPlayerUnderwater()) {
        float pulse = sinf((float)GetTime() * 1.5f) * 0.05f + 0.95f;
        unsigned char ua = (unsigned char)(100 * pulse);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){20, 60, 140, ua});
        // Darker edges vignette
        DrawRectangle(0, 0, SCREEN_WIDTH, 40, (Color){10, 30, 80, (unsigned char)(60 * pulse)});
        DrawRectangle(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, (Color){10, 30, 80, (unsigned char)(60 * pulse)});
    }

    // Damage flash overlay
    if (player.damageFlashTimer > 0.0f) {
        float a = 160.0f * (player.damageFlashTimer / 0.3f);
        if (a > 160.0f) a = 160.0f;
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){200, 30, 30, (unsigned char)a});
    }

    // Low health vignette (pulsing red edges when health <= 6)
    if (player.health > 0 && player.health <= 6) {
        float pulse = sinf((float)GetTime() * 3.0f) * 0.3f + 0.7f;
        float healthFactor = 1.0f - (float)player.health / 6.0f;
        unsigned char vignetteA = (unsigned char)(80 * healthFactor * pulse);
        DrawRectangle(0, 0, SCREEN_WIDTH, 40, (Color){150, 0, 0, vignetteA});
        DrawRectangle(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, (Color){150, 0, 0, vignetteA});
        DrawRectangle(0, 0, 30, SCREEN_HEIGHT, (Color){150, 0, 0, (unsigned char)(vignetteA * 0.6f)});
        DrawRectangle(SCREEN_WIDTH - 30, 0, 30, SCREEN_HEIGHT, (Color){150, 0, 0, (unsigned char)(vignetteA * 0.6f)});
    }

    bool deadModal = player.playerDead;
    if (!deadModal) {
        DrawHotbar();
        DrawPlayerStatus();
        DrawDebugInfo();
        DrawMinimap();
        DrawMessage();
        DrawChatUI();

        DrawInventoryScreen();
        DrawCreativeScreen();
        DrawTradeUI();
        DrawEnchantingTableUI();
        if (!achievementsOpen) DrawPauseMenu();
    }
    DrawAchievementsUI();
    DrawDeathScreen(GetFrameTime());

    // Sleep transition overlay (fade to black while sleeping)
    if (sleepFade > 0.01f) {
        unsigned char sleepA = (unsigned char)(sleepFade * 255);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, sleepA});
    }

    // Large map overlay (draws on top of everything)
    if (!player.playerDead && showLargeMap) DrawLargeMap();

    if (showDebug) DrawFPS(SCREEN_WIDTH - 80, 10);

    DrawTransition();
draw_finish:
    if (logicalCanvasReady) {
        EndTextureMode();
        int outputW = GetScreenWidth();
        int outputH = GetScreenHeight();
        UpdateLogicalViewport();
        ClearBackground((Color){ 8, 10, 18, 255 });
        Rectangle source = { 0, 0, (float)logicalCanvas.texture.width, -(float)logicalCanvas.texture.height };
        Rectangle dest = logicalViewport;
        DrawTexturePro(logicalCanvas.texture, source, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
    }

    // TEMP capture hook
    {
        static int inited = 0, frame = 0;
        static const char *name = NULL;
        static int at = 60;
        if (!inited) {
            inited = 1;
            name = getenv("MYWORLD_SHOT");
            if (name) { const char *q = strchr(name, '?'); if (q) at = atoi(q + 1); }
            const char *ws = getenv("MYWORLD_WORLD");
            if (ws) StartGameFromSlot(atoi(ws), false);
            const char *st = getenv("MYWORLD_STATE");
            if (st && !strcmp(st, "slotsel")) {
                slotSelectMode = getenv("MYWORLD_MODE") ? atoi(getenv("MYWORLD_MODE")) : 1;
                gameState = STATE_SLOT_SELECT;
            } else if (st && !strcmp(st, "settings")) {
                gameState = STATE_SETTINGS;   // forced each frame below
            }
        }
        // Keep the forced state stable (menu logic overwrites it otherwise)
        {
            const char *st = getenv("MYWORLD_STATE");
            static int forced = 0;
            if (st && !strcmp(st, "settings")) {
                if (!forced) { forced = 1; }
                else { gameState = STATE_SETTINGS; transitionAlpha = 0.0f; }
            }
        }
        const char *open = getenv("MYWORLD_OPEN");
        if (open && gameState == STATE_PLAYING) {
            if (!strcmp(open, "inv")) inventoryOpen = true;
            else if (!strcmp(open, "creative")) creativeOpen = true;
            else if (!strcmp(open, "pause")) gamePaused = true;
        }
        if (name && ++frame == at) {
            char path[256];
            const char *q = strchr(name, '?');
            int n = q ? (int)(q - name) : (int)strlen(name);
            snprintf(path, sizeof(path), "shots/%.*s.png", n, name);
            Image img = LoadImageFromTexture(logicalCanvas.texture);
            ImageFlipVertical(&img);
            ExportImage(img, path);
            UnloadImage(img);
        }
    }

    EndDrawing();
}

void UnloadGame(void)
{
    if (gameState == STATE_PLAYING && currentSavePath[0]) {
        SaveWorld(currentSavePath);
    }

    // Save settings
    {
        FILE *f = fopen("settings.cfg", "w");
        if (f) {
            fprintf(f, "language=%d\n", (int)language);
            fprintf(f, "bgm_volume=%.2f\n", bgmVolumeSlider);
            fprintf(f, "sfx_volume=%.2f\n", sfxVolumeSlider);
            fprintf(f, "window_mode=%d\n", windowMode);
            fprintf(f, "resolution=%d\n", resolutionPreset);
            fprintf(f, "difficulty=%d\n", (int)gameDifficulty);
            fprintf(f, "font_custom=%d\n", useCustomFont ? 1 : 0);
            if (customFontPath[0]) fprintf(f, "font_path=%s\n", customFontPath);
            fclose(f);
        }
    }

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX != CHUNK_EMPTY && loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
    }
    // Notify peers before shutting down
    if (NetIsHost()) NetHostStop();
    else if (NetIsClient()) NetClientDisconnect();
    NetShutdown();
    UnloadTexture(blockAtlas);
    UnloadSounds();
    UnloadGameFont();
}

void LoadSettings(void)
{
    FILE *f = fopen("settings.cfg", "r");
    if (!f) return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Strip newline
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;

        if (strncmp(line, "language=", 9) == 0) {
            int v = atoi(line + 9);
            if (v >= 0 && v < LANG_COUNT) language = (Language)v;
        } else if (strncmp(line, "bgm_volume=", 11) == 0) {
            float v = (float)atof(line + 11);
            if (v >= 0.0f && v <= 1.0f) bgmVolumeSlider = v;
        } else if (strncmp(line, "sfx_volume=", 11) == 0) {
            float v = (float)atof(line + 11);
            if (v >= 0.0f && v <= 1.0f) sfxVolumeSlider = v;
        } else if (strncmp(line, "difficulty=", 11) == 0) {
            int v = atoi(line + 11);
            if (v >= 0 && v <= DIFFICULTY_HARD) gameDifficulty = (Difficulty)v;
        } else if (strncmp(line, "window_mode=", 12) == 0) {
            int v = atoi(line + 12);
            if (v >= 0 && v <= 2) windowMode = v;
        } else if (strncmp(line, "resolution=", 11) == 0) {
            int v = atoi(line + 11);
            if (v >= 0 && v < 3) resolutionPreset = v;
        } else if (strncmp(line, "font_custom=", 12) == 0) {
            // Will be applied after font loading
        } else if (strncmp(line, "font_path=", 10) == 0) {
            strncpy(customFontPath, line + 10, sizeof(customFontPath) - 1);
            customFontPath[sizeof(customFontPath) - 1] = 0;
        }
    }
    fclose(f);
}

void UpdateDrawFrame(void)
{
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    simulationAccumulator += dt;
    if (simulationAccumulator > 0.25f) simulationAccumulator = 0.25f;
    while (simulationAccumulator >= SIM_TICK_DT) {
        simulationAccumulator -= SIM_TICK_DT;
        simulationTick++;
    }
    UpdateBGM();
    UpdateTransition(dt);
    UpdateGame(dt);
    DrawGame();
}
