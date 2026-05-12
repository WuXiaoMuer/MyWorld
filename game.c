#include "types.h"
#include <stdlib.h>
#include <time.h>

void InitGame(void)
{
    srand((unsigned int)time(NULL));
    unsigned int seed = (unsigned int)rand();

    GenerateBlockAtlas();

    if (SaveExists(SAVE_PATH)) {
        LoadWorld(SAVE_PATH);
    } else {
        GenerateWorld(seed);
        InitPlayer();
    }

    InitCameraSystem();
    InitDayNight();
    UpdateChunks();
}

void UpdateGame(float dt)
{
    if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

    UpdatePlayer(dt);
    UpdateCameraSystem(dt);
    UpdateChunks();
    UpdateHotbar();
    UpdateDayNight(dt);
}

void DrawGame(void)
{
    BeginDrawing();
    ClearBackground(GetSkyColor());

    BeginMode2D(camera);
    DrawWorld();
    DrawWater();
    DrawCrosshair();
    DrawPlayerSprite();
    EndMode2D();

    // Dark overlay for night
    if (dayNight.lightLevel < 1.0f) {
        unsigned char alpha = (unsigned char)(200 * (1.0f - dayNight.lightLevel));
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 20, alpha});
    }

    DrawHotbar();
    DrawDebugInfo();
    DrawFPS(SCREEN_WIDTH - 80, 10);

    EndDrawing();
}

void UnloadGame(void)
{
    SaveWorld(SAVE_PATH);

    for (int i = 0; i < loadedChunkCount; i++) {
        if (loadedChunks[i].textureValid) {
            UnloadTexture(loadedChunks[i].texture);
        }
    }
    UnloadTexture(blockAtlas);
}

void UpdateDrawFrame(void)
{
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    UpdateGame(dt);
    DrawGame();
}
