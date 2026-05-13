/*******************************************************************************************
*
*   MyWorld - A 2D sandbox game inspired by Minecraft, built with raylib 5.5
*
********************************************************************************************/

#include "types.h"
#include <process.h>

//----------------------------------------------------------------------------------
// Global Definitions
//----------------------------------------------------------------------------------
uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
uint8_t lightMap[WORLD_WIDTH][WORLD_HEIGHT];
Chunk loadedChunks[MAX_CHUNKS];

Player player = { 0 };
Camera2D camera = { 0 };
DayNightCycle dayNight = { 0 };

Texture2D blockAtlas = { 0 };
bool showDebug = false;
bool inventoryOpen = false;
bool gamePaused = false;
unsigned int worldSeed = 0;

char messageText[128] = { 0 };
float messageTimer = 0.0f;
Color messageColor = { 240, 100, 100, 255 };

Mob mobs[MAX_MOBS];
float mobSpawnTimer = 0.0f;

ItemEntity entities[MAX_ENTITIES];

CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
int craftRecipeCount = 0;

// Sound globals
Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
Sound sndHurt, sndDeath, sndEat, sndClick, sndCraft, sndXP, sndDrop;
Sound sndFootstep, sndZombie, sndPig, sndSplash;
Music bgm = { 0 };

bool audioReady = false;
GameState gameState = STATE_MENU;

//----------------------------------------------------------------------------------
// Background audio loading thread
//----------------------------------------------------------------------------------
static void AudioLoadThread(void *arg)
{
    (void)arg;
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        InitSounds();
    }
    audioReady = true;
}

//----------------------------------------------------------------------------------
// Program Entry Point
//----------------------------------------------------------------------------------
int main(void)
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MyWorld");
    SetTargetFPS(60);
    SetExitKey(0);
    SetWindowState(FLAG_WINDOW_ALWAYS_RUN);

    // Start audio loading in background thread
    _beginthread(AudioLoadThread, 0, NULL);

    // Pre-init: load systems needed for menu rendering
    GenerateBlockAtlas();
    InitCraftingRecipes();
    InitCameraSystem();
    InitDayNight();

    gameState = STATE_MENU;

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}
