/*******************************************************************************************
*
*   MyWorld - A 2D sandbox game inspired by Minecraft, built with raylib 5.5
*
********************************************************************************************/

#include "types.h"

//----------------------------------------------------------------------------------
// Global Definitions
//----------------------------------------------------------------------------------
uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
Chunk loadedChunks[MAX_CHUNKS];
int loadedChunkCount = 0;

Player player = { 0 };
Camera2D camera = { 0 };
DayNightCycle dayNight = { 0 };

Texture2D blockAtlas = { 0 };
bool showDebug = false;
bool inventoryOpen = false;
unsigned int worldSeed = 0;

CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
int craftRecipeCount = 0;

//----------------------------------------------------------------------------------
// Program Entry Point
//----------------------------------------------------------------------------------
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "MyWorld");
    SetTargetFPS(60);
    SetExitKey(0);

    InitGame();

    while (!WindowShouldClose())
    {
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}
