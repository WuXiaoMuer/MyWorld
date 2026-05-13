#include "types.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

void InitGame(void)
{
    srand((unsigned int)time(NULL));
    worldSeed = (unsigned int)rand();

    // Atlas and crafting already initialized in main()
    InitMobs();
    InitParticles();
    InitEntities();
    InitLightMap();

    if (SaveExists(SAVE_PATH) && LoadWorld(SAVE_PATH)) {
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

int menuSelection = 0; // 0=New, 1=Load, 2=Quit

static void UpdateMainMenu(float dt)
{
    (void)dt;

    // Keyboard navigation
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        menuSelection++;
        if (menuSelection > 2) menuSelection = 0;
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        menuSelection--;
        if (menuSelection < 0) menuSelection = 2;
    }

    // Confirm with Enter/Space
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        if (menuSelection == 0) {
            remove(SAVE_PATH);
            InitGame();
            gameState = STATE_PLAYING;
            return;
        } else if (menuSelection == 1 && SaveExists(SAVE_PATH)) {
            InitGame();
            gameState = STATE_PLAYING;
            return;
        } else if (menuSelection == 2) {
            CloseWindow();
            exit(0);
        }
    }

    // Mouse hover + click
    {
        Vector2 mouse = GetMousePosition();
        Vector2 delta = GetMouseDelta();
        int btnW = 260, btnH = 44;
        int btnX = (SCREEN_WIDTH - btnW) / 2;
        int btnY = 250;
        int spacing = 58;

        Rectangle btnNew = { (float)btnX, (float)btnY, (float)btnW, (float)btnH };
        Rectangle btnLoad = { (float)btnX, (float)(btnY + spacing), (float)btnW, (float)btnH };
        Rectangle btnQuit = { (float)btnX, (float)(btnY + spacing * 2), (float)btnW, (float)btnH };

        // Hover highlight only when mouse moves
        if (fabsf(delta.x) > 0.5f || fabsf(delta.y) > 0.5f) {
            if (CheckCollisionPointRec(mouse, btnNew)) menuSelection = 0;
            else if (CheckCollisionPointRec(mouse, btnLoad) && SaveExists(SAVE_PATH)) menuSelection = 1;
            else if (CheckCollisionPointRec(mouse, btnQuit)) menuSelection = 2;
        }

        // Click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, btnNew)) {
                remove(SAVE_PATH);
                InitGame();
                gameState = STATE_PLAYING;
                return;
            }
            if (CheckCollisionPointRec(mouse, btnLoad) && SaveExists(SAVE_PATH)) {
                InitGame();
                gameState = STATE_PLAYING;
                return;
            }
            if (CheckCollisionPointRec(mouse, btnQuit)) {
                CloseWindow();
                exit(0);
            }
        }
    }
}

void UpdateGame(float dt)
{
    if (gameState == STATE_MENU) {
        UpdateMainMenu(dt);
        return;
    }

    // Auto-save every 5 minutes
    {
        static float autoSaveTimer = 0.0f;
        autoSaveTimer += dt;
        if (autoSaveTimer >= 300.0f) {
            autoSaveTimer = 0.0f;
            if (!player.playerDead) {
                SaveWorld(SAVE_PATH);
            }
        }
    }

    // Update message timer
    if (messageTimer > 0.0f) messageTimer -= dt;

    // Death respawn input
    if (player.playerDead) {
        if (GetDeathFadeTimer() > 1.0f && IsKeyPressed(KEY_SPACE)) {
            RespawnPlayer();
        }
        return;
    }

    // Toggle inventory
    if (IsKeyPressed(KEY_E)) {
        inventoryOpen = !inventoryOpen;
        if (inventoryOpen) {
            gamePaused = false;
        } else {
            ReturnHeldItem();
        }
        PlaySoundUIClick();
    }

    // ESC: close inventory first, then toggle pause
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (inventoryOpen) {
            inventoryOpen = false;
            ReturnHeldItem();
        } else {
            gamePaused = !gamePaused;
        }
        PlaySoundUIClick();
    }

    if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    // XP healing
    if (IsKeyPressed(KEY_H) && !inventoryOpen && !gamePaused && !player.playerDead) {
        if (player.xp >= XP_HEAL_COST && player.health < MAX_HEALTH) {
            player.xp -= XP_HEAL_COST;
            player.health += XP_HEAL_AMOUNT;
            if (player.health > MAX_HEALTH) player.health = MAX_HEALTH;
            PlaySoundEat();
            ShowMessage("Healed with XP!", (Color){100, 240, 100, 255});
        } else if (player.xp < XP_HEAL_COST) {
            ShowMessage("Not enough XP!", (Color){240, 200, 80, 255});
        }
    }

    // Don't update gameplay when paused or inventory open
    if (!gamePaused && !inventoryOpen) {
        UpdatePlayer(dt);
        UpdatePlayerStatus(dt);
        UpdateMobs(dt);
        UpdateParticles(dt);
        UpdateEntities(dt);
        PickupNearbyItems(player.position.x, player.position.y);
        UpdateCameraSystem(dt);
        UpdateDayNight(dt);
        if (player.damageFlashTimer > 0.0f) player.damageFlashTimer -= dt;
    }
    UpdateChunks();
    UpdateHotbar();
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

    DrawBackground();

    BeginMode2D(camera);
    DrawWorld();
    DrawWater();
    DrawMobs();
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

    DrawHotbar();
    DrawPlayerStatus();
    DrawDebugInfo();
    DrawMessage();

    DrawInventoryScreen();
    DrawPauseMenu();
    DrawDeathScreen(GetFrameTime());

    DrawFPS(SCREEN_WIDTH - 80, 10);

    EndDrawing();
}

void UnloadGame(void)
{
    if (gameState == STATE_PLAYING) {
        SaveWorld(SAVE_PATH);
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
