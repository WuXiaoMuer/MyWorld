#include "types.h"
#include <stdlib.h>
#include <time.h>

void InitGame(void)
{
    srand((unsigned int)time(NULL));
    worldSeed = (unsigned int)rand();

    GenerateBlockAtlas();
    InitSounds();
    InitCraftingRecipes();
    InitMobs();

    if (SaveExists(SAVE_PATH)) {
        LoadWorld(SAVE_PATH);
    } else {
        GenerateWorld(worldSeed);
        InitPlayer();
    }

    InitCameraSystem();
    InitDayNight();
    UpdateChunks();
}

void UpdateGame(float dt)
{
    // Update message timer
    if (messageTimer > 0.0f) messageTimer -= dt;

    // Toggle inventory
    if (IsKeyPressed(KEY_E)) {
        inventoryOpen = !inventoryOpen;
        if (inventoryOpen) gamePaused = false;
    }

    // Toggle pause (only when inventory is closed)
    if (IsKeyPressed(KEY_ESCAPE) && !inventoryOpen) {
        gamePaused = !gamePaused;
    }

    if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    // Don't update gameplay when paused or inventory open
    if (!gamePaused && !inventoryOpen) {
        UpdatePlayer(dt);
        UpdatePlayerStatus(dt);
        UpdateMobs(dt);
        UpdateCameraSystem(dt);
    }
    UpdateChunks();
    UpdateHotbar();
    UpdateDayNight(dt);
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(GetSkyColor());

    DrawBackground();

    BeginMode2D(camera);
    DrawWorld();
    DrawWater();
    DrawMobs();
    if (!inventoryOpen && !gamePaused) DrawCrosshair();
    DrawPlayerSprite();
    EndMode2D();

    if (dayNight.lightLevel < 1.0f) {
        unsigned char alpha = (unsigned char)(120 * (1.0f - dayNight.lightLevel));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){5, 5, 25, alpha});
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
    SaveWorld(SAVE_PATH);

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
    UpdateGame(dt);
    DrawGame();
}
