#include "types.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

//----------------------------------------------------------------------------------
// Window Mode Management
//----------------------------------------------------------------------------------
static int savedWinX = 0, savedWinY = 0, savedWinW = SCREEN_WIDTH, savedWinH = SCREEN_HEIGHT;

void ApplyWindowMode(int mode)
{
    if (mode == windowMode) return;

    int monW = GetMonitorWidth(GetCurrentMonitor());
    int monH = GetMonitorHeight(GetCurrentMonitor());

    if (mode == 0) {
        // Windowed: restore previous size
        if (IsWindowFullscreen()) ToggleFullscreen();
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
        SetWindowSize(savedWinW, savedWinH);
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

void InitGame(void)
{
    confirmDialogActive = false;
    gamePaused = false;
    inventoryOpen = false;
    furnaceOpen = false;
    craftingTableOpen = false;
    srand((unsigned int)time(NULL));
    if (worldSeed == 0) worldSeed = (unsigned int)rand();

    // Atlas and crafting already initialized in main()
    InitMobs();
    InitParticles();
    InitEntities();
    InitProjectiles();
    InitLightMap();
    InitSmeltingRecipes();

    GetSavePath(selectedSaveSlot, currentSavePath, sizeof(currentSavePath));

    if (SaveExists(currentSavePath) && LoadWorld(currentSavePath)) {
        // Loaded successfully
    } else {
        GenerateWorld(worldSeed);
        InitPlayer();
    }

    RecalculateAllLight();
    InitCameraSystem();
    InitDayNight();
    UpdateChunks();
    player.playerDead = false;
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
    gameState = STATE_PLAYING;
}

// Try to start a new game on slot; show confirm dialog if slot has data
static void TryNewGameOnSlot(int slot)
{
    SaveSlotInfo info;
    GetSlotInfo(slot, &info);
    if (info.exists) {
        confirmDialogActive = true;
        confirmDialogSlot = slot;
        confirmDialogMode = 0; // overwrite
        PlaySoundUIClick();
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
        gameState = STATE_MENU;
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
        int slotY = 160;
        int spacing = 96;

        // Scroll wheel on the slot area
        Rectangle slotArea = { (float)slotX, (float)slotY, (float)slotW, (float)(SLOT_VISIBLE * spacing) };
        if (CheckCollisionPointRec(mouse, slotArea)) {
            int wheel = (int)GetMouseWheelMove();
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
            gameState = STATE_MENU;
            menuSelection = 0;
            seedInputLen = 0;
            seedInputBuf[0] = '\0';
            PlaySoundUIClick();
        }
    }
}

static void UpdateMainMenu(float dt)
{
    (void)dt;

    int btnCount = 4; // New, Load, Settings, Quit

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
            menuSelection = 0;
            gameState = STATE_SLOT_SELECT;
            return;
        } else if (menuSelection == 1) {
            // Load Game -> slot select (load mode)
            slotSelectMode = 1;
            menuSelection = 0;
            // Check if any slot exists
            bool anySlot = false;
            for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
                SaveSlotInfo info;
                if (GetSlotInfo(i, &info) && info.exists) { anySlot = true; break; }
            }
            if (anySlot) {
                gameState = STATE_SLOT_SELECT;
            }
            return;
        } else if (menuSelection == 2) {
            gameState = STATE_SETTINGS;
            return;
        } else if (menuSelection == 3) {
            CloseWindow();
            exit(0);
        }
    }

    // Mouse hover + click
    {
        Vector2 mouse = Win32GetMousePosition();
        Vector2 delta = Win32GetMouseDelta();
        int btnW = 260, btnH = 44;
        int btnX = (SCREEN_WIDTH - btnW) / 2;
        int btnY = 220;
        int spacing = 54;

        Rectangle btns[4];
        for (int i = 0; i < 4; i++) {
            btns[i] = (Rectangle){ (float)btnX, (float)(btnY + i * spacing), (float)btnW, (float)btnH };
        }

        // Hover highlight only when mouse moves
        if (fabsf(delta.x) > 0.5f || fabsf(delta.y) > 0.5f) {
            for (int i = 0; i < 4; i++) {
                bool hasSave = false;
                for (int s = 0; s < MAX_SAVE_SLOTS; s++) {
                    SaveSlotInfo info;
                    if (GetSlotInfo(s, &info) && info.exists) { hasSave = true; break; }
                }
                bool enabled = (i == 0) || (i == 1 && hasSave) || (i == 2) || (i == 3);
                if (enabled && CheckCollisionPointRec(mouse, btns[i])) {
                    menuSelection = i;
                    break;
                }
            }
        }

        // Click
        if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, btns[0])) {
                slotSelectMode = 0;
                menuSelection = 0;
                gameState = STATE_SLOT_SELECT;
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
                    gameState = STATE_SLOT_SELECT;
                }
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[2])) {
                gameState = STATE_SETTINGS;
                return;
            }
            if (CheckCollisionPointRec(mouse, btns[3])) {
                CloseWindow();
                exit(0);
            }
        }
    }
}

// Return furnace items to inventory when closing
static void ReturnFurnaceItems(void)
{
    if (furnaceFuel != BLOCK_AIR) {
        if (!AddToInventory((BlockType)furnaceFuel)) {
            SpawnItemEntity(furnaceFuel, furnaceFuelCount,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceFuel = BLOCK_AIR;
        furnaceFuelCount = 0;
    }
    if (furnaceInput != BLOCK_AIR) {
        if (!AddToInventory((BlockType)furnaceInput)) {
            SpawnItemEntity(furnaceInput, furnaceInputCount,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceInput = BLOCK_AIR;
        furnaceInputCount = 0;
    }
    if (furnaceOutput != BLOCK_AIR) {
        if (!AddToInventory((BlockType)furnaceOutput)) {
            SpawnItemEntity(furnaceOutput, furnaceOutputCount,
                            player.position.x + PLAYER_WIDTH / 2, player.position.y);
        }
        furnaceOutput = BLOCK_AIR;
        furnaceOutputCount = 0;
    }
    furnaceProgress = 0.0f;
    furnaceFuelBurn = 0.0f;
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
        if (furnaceFuel == ITEM_COAL && furnaceFuelCount > 0) {
            furnaceFuelBurn = 8.0f;
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
}

void UpdateGame(float dt)
{
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

    // Auto-save every 5 minutes
    {
        static float autoSaveTimer = 0.0f;
        autoSaveTimer += dt;
        if (autoSaveTimer >= 300.0f) {
            autoSaveTimer = 0.0f;
            if (!player.playerDead && currentSavePath[0]) {
                SaveWorld(currentSavePath);
                ShowMessage("Game Saved", (Color){100, 200, 100, 255});
            }
        }
    }

    // Update message timer
    if (messageTimer > 0.0f) messageTimer -= dt;

    // Death respawn input
    if (player.playerDead) {
        if (GetDeathFadeTimer() > 1.0f && Win32IsKeyPressed(KEY_SPACE)) {
            RespawnPlayer();
        }
        // ESC during death: return to main menu
        if (Win32IsKeyPressed(KEY_ESCAPE)) {
            if (currentSavePath[0]) SaveWorld(currentSavePath);
            gameState = STATE_MENU;
            menuSelection = 0;
            PlaySoundUIClick();
        }
        return;
    }

    // Toggle inventory
    if (Win32IsKeyPressed(KEY_E)) {
        if (furnaceOpen) {
            // Close furnace
            ReturnFurnaceItems();
            furnaceOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
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

    // ESC: close furnace first, then inventory, then large map, then toggle pause
    if (Win32IsKeyPressed(KEY_ESCAPE)) {
        if (furnaceOpen) {
            ReturnFurnaceItems();
            furnaceOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
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

    if (Win32IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    // M: toggle large map
    if (Win32IsKeyPressed(KEY_M) && !inventoryOpen && !player.playerDead) {
        showLargeMap = !showLargeMap;
        if (showLargeMap) gamePaused = true;
        else gamePaused = false;
    }

    // F11: toggle fullscreen (any non-windowed mode -> windowed, windowed -> fullscreen)
    if (Win32IsKeyPressed(KEY_F11)) {
        ApplyWindowMode(windowMode != 0 ? 0 : 1);
    }

    // XP healing
    if (Win32IsKeyPressed(KEY_H) && !inventoryOpen && !gamePaused && !player.playerDead) {
        if (player.xp >= XP_HEAL_COST && player.health < MAX_HEALTH) {
            player.xp -= XP_HEAL_COST;
            player.health += XP_HEAL_AMOUNT;
            if (player.health > MAX_HEALTH) player.health = MAX_HEALTH;
            PlaySoundEat();
            ShowMessage("Healed with XP!", (Color){100, 240, 100, 255});
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT / 2,
                                 (Color){80, 220, 80, 200});
        } else if (player.xp < XP_HEAL_COST) {
            ShowMessage("Not enough XP!", (Color){240, 200, 80, 255});
        }
    }

    // Don't update gameplay when paused or inventory open (but allow furnace to tick)
    if (!gamePaused && !inventoryOpen) {
        UpdatePlayer(dt);
        UpdateMobs(dt);
        UpdateProjectiles(dt);
        UpdateParticles(dt);
        UpdateEntities(dt);
        PickupNearbyItems(player.position.x, player.position.y);
        UpdateCameraSystem(dt);
        UpdateDayNight(dt);
        if (player.damageFlashTimer > 0.0f) player.damageFlashTimer -= dt;
    }
    // Status effects (drowning, hunger) apply even with inventory open
    if (!gamePaused) {
        UpdatePlayerStatus(dt);
        UpdateFurnaceTick(dt);
    }
    UpdateChunks();
    UpdateHotbar();

    // Distance checks: auto-close crafting table/furnace if player moves away
    if (craftingTableOpen || furnaceOpen) {
        int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
        int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
        bool nearBlock = false;
        for (int dx = -1; dx <= 1 && !nearBlock; dx++) {
            for (int dy = -1; dy <= 1 && !nearBlock; dy++) {
                int tx = bx + dx, ty = by + dy;
                if (tx >= 0 && tx < WORLD_WIDTH && ty >= 0 && ty < WORLD_HEIGHT) {
                    if (craftingTableOpen && world[tx][ty] == BLOCK_CRAFTING_TABLE) nearBlock = true;
                    if (furnaceOpen && world[tx][ty] == BLOCK_FURNACE) nearBlock = true;
                }
            }
        }
        if (!nearBlock) {
            if (furnaceOpen) {
                ReturnFurnaceItems();
                furnaceOpen = false;
            }
            craftingTableOpen = false;
            inventoryOpen = false;
            ReturnHeldItem();
            craftSearchLen = 0;
            craftSearchBuf[0] = '\0';
        }
    }
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(GetSkyColor());

    if (gameState == STATE_MENU) {
        DrawBackground();
        DrawMainMenu();
        DrawFPS(SCREEN_WIDTH - 80, 10);
        EndDrawing();
        return;
    }

    if (gameState == STATE_SLOT_SELECT) {
        DrawSlotSelectScreen();
        DrawFPS(SCREEN_WIDTH - 80, 10);
        EndDrawing();
        return;
    }

    if (gameState == STATE_SETTINGS) {
        DrawSettingsScreen();
        DrawFPS(SCREEN_WIDTH - 80, 10);
        EndDrawing();
        return;
    }

    DrawBackground();

    BeginMode2D(camera);
    DrawWorld();
    DrawWater();
    DrawMobs();
    DrawProjectiles();
    DrawEntities();
    DrawParticles();
    if (!inventoryOpen && !gamePaused && !player.playerDead) DrawCrosshair();
    DrawPlayerSprite();
    EndMode2D();

    if (dayNight.lightLevel < 1.0f) {
        unsigned char alpha = (unsigned char)(120 * (1.0f - dayNight.lightLevel));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){5, 5, 25, alpha});
    }

    // Damage flash overlay
    if (player.damageFlashTimer > 0.0f) {
        unsigned char alpha = (unsigned char)(160 * (player.damageFlashTimer / 0.3f));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){200, 30, 30, alpha});
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

    DrawHotbar();
    DrawPlayerStatus();
    DrawDebugInfo();
    DrawMinimap();
    DrawMessage();

    DrawInventoryScreen();
    if (furnaceOpen) DrawFurnaceUI();
    DrawPauseMenu();
    DrawDeathScreen(GetFrameTime());

    // Large map overlay (draws on top of everything)
    if (showLargeMap) DrawLargeMap();

    DrawFPS(SCREEN_WIDTH - 80, 10);

    EndDrawing();
}

void UnloadGame(void)
{
    if (gameState == STATE_PLAYING && currentSavePath[0]) {
        SaveWorld(currentSavePath);
    }

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX != CHUNK_EMPTY && loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
    }
    UnloadTexture(blockAtlas);
    UnloadSounds();
}

void UpdateDrawFrame(void)
{
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    UpdateBGM();
    UpdateGame(dt);
    DrawGame();
}
