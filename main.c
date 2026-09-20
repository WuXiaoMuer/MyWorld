/*******************************************************************************************
*
*   MyWorld - A 2D sandbox game inspired by Minecraft, built with raylib 5.5
*
********************************************************************************************/

#include "types.h"
#include "net.h"
#include <process.h>
#include <string.h>

//----------------------------------------------------------------------------------
// Win32 Input Override Implementation
// Uses GetForegroundWindow() to completely isolate input between multiple instances.
// When this window is NOT the foreground window, ALL input reads return zero/false.
//----------------------------------------------------------------------------------
int win32MouseX = 0, win32MouseY = 0;
int win32MousePrevX = 0, win32MousePrevY = 0;
bool win32LMB = false, win32LMBPrev = false;
bool win32RMB = false, win32RMBPrev = false;

// Set once per frame in UpdateWin32Input; all other input functions read this.
static bool g_windowForeground = false;

// Per-frame "pressed this frame" tracking for keys
static bool keyPrev[256] = {0};

static int MapRaylibToVK(int key)
{
    if (key >= KEY_A && key <= KEY_Z) return 0x41 + (key - KEY_A);
    if (key >= KEY_ZERO && key <= KEY_NINE) return 0x30 + (key - KEY_ZERO);
    if (key >= KEY_ONE && key <= KEY_NINE) return 0x31 + (key - KEY_ONE);
    if (key == KEY_SPACE) return VK_SPACE;
    if (key == KEY_ESCAPE) return VK_ESCAPE;
    if (key == KEY_ENTER) return VK_RETURN;
    if (key == KEY_BACKSPACE) return VK_BACK;
    if (key == KEY_TAB) return VK_TAB;
    if (key == KEY_DELETE) return VK_DELETE;
    if (key == KEY_LEFT) return VK_LEFT;
    if (key == KEY_RIGHT) return VK_RIGHT;
    if (key == KEY_UP) return VK_UP;
    if (key == KEY_DOWN) return VK_DOWN;
    if (key == KEY_LEFT_SHIFT) return VK_SHIFT;
    if (key == KEY_RIGHT_SHIFT) return VK_SHIFT;
    if (key == KEY_LEFT_CONTROL) return VK_CONTROL;
    if (key == KEY_RIGHT_CONTROL) return VK_CONTROL;
    if (key == KEY_LEFT_ALT) return VK_MENU;
    if (key == KEY_RIGHT_ALT) return VK_MENU;
    if (key == KEY_F3) return VK_F3;
    if (key == KEY_F11) return VK_F11;
    if (key == VK_OEM_PERIOD) return VK_OEM_PERIOD;
    if (key == VK_OEM_2) return VK_OEM_2;
    return 0;
}

// Reset all input tracking when losing foreground (avoids stale "pressed" on refocus)
static void ResetAllInputState(void)
{
    win32LMB = false; win32LMBPrev = false;
    win32RMB = false; win32RMBPrev = false;
    memset(keyPrev, 0, sizeof(keyPrev));
}

void UpdateLogicalViewport(void)
{
    int outputW = GetScreenWidth();
    int outputH = GetScreenHeight();
    if (outputW <= 0 || outputH <= 0) return;
    logicalScale = fminf((float)outputW / SCREEN_WIDTH, (float)outputH / SCREEN_HEIGHT);
    if (logicalScale <= 0.0f) logicalScale = 1.0f;
    logicalViewport.width = SCREEN_WIDTH * logicalScale;
    logicalViewport.height = SCREEN_HEIGHT * logicalScale;
    logicalViewport.x = (outputW - logicalViewport.width) * 0.5f;
    logicalViewport.y = (outputH - logicalViewport.height) * 0.5f;
}

void UpdateWin32Input(void)
{
    UpdateLogicalViewport();
    // Save previous mouse state
    win32MousePrevX = win32MouseX;
    win32MousePrevY = win32MouseY;
    win32LMBPrev = win32LMB;
    win32RMBPrev = win32RMB;

    // Check if THIS window is the foreground window
    void *hwnd = GetWindowHandle();
    void *fgWnd = GetForegroundWindow();
    bool wasForeground = g_windowForeground;
    g_windowForeground = (hwnd != NULL && hwnd == fgWnd);

    // If we just lost foreground, reset all input to zero
    if (wasForeground && !g_windowForeground) {
        ResetAllInputState();
        return;
    }

    // If not foreground, keep everything at zero
    if (!g_windowForeground) {
        win32LMB = false;
        win32RMB = false;
        return;
    }

    // Get raw screen cursor position
    long pt[2];
    GetCursorPos(pt);

    // Convert to window client coordinates
    if (hwnd) ScreenToClient(hwnd, pt);
    win32MouseX = (int)pt[0];
    win32MouseY = (int)pt[1];

    // Mouse buttons (only when foreground)
    win32LMB = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    win32RMB = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

bool IsMoveLeftDown(void)
{
    return Win32IsKeyDown(KEY_A) || Win32IsKeyDown(KEY_LEFT);
}

bool IsMoveRightDown(void)
{
    return Win32IsKeyDown(KEY_D) || Win32IsKeyDown(KEY_RIGHT);
}

bool IsJumpDown(void)
{
    return Win32IsKeyDown(KEY_W) || Win32IsKeyDown(KEY_UP) || Win32IsKeyDown(KEY_SPACE);
}

bool IsJumpPressed(void)
{
    return Win32IsKeyPressed(KEY_W) || Win32IsKeyPressed(KEY_UP) || Win32IsKeyPressed(KEY_SPACE);
}

bool IsSprintDown(void)
{
    return Win32IsKeyDown(KEY_LEFT_CONTROL) || Win32IsKeyDown(KEY_RIGHT_CONTROL);
}

bool IsSneakDown(void)
{
    return Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT);
}

static Vector2 GetRawWin32MousePosition(void)
{
    return (Vector2){ (float)win32MouseX, (float)win32MouseY };
}

Vector2 Win32GetMousePosition(void)
{
    return Win32GetLogicalMousePosition();
}

Vector2 Win32GetLogicalMousePosition(void)
{
    Vector2 mouse = GetRawWin32MousePosition();
    if (logicalScale <= 0.0f) return mouse;
    mouse.x = (mouse.x - logicalViewport.x) / logicalScale;
    mouse.y = (mouse.y - logicalViewport.y) / logicalScale;
    return mouse;
}

Vector2 Win32GetMouseDelta(void)
{
    return (Vector2){ (float)(win32MouseX - win32MousePrevX), (float)(win32MouseY - win32MousePrevY) };
}

bool Win32IsMouseButtonPressed(int button)
{
    if (!g_windowForeground) return false;
    if (button == 0) return win32LMB && !win32LMBPrev;
    if (button == 1) return win32RMB && !win32RMBPrev;
    return false;
}

bool Win32IsMouseButtonReleased(int button)
{
    if (!g_windowForeground) return false;
    if (button == 0) return !win32LMB && win32LMBPrev;
    if (button == 1) return !win32RMB && win32RMBPrev;
    return false;
}

bool Win32IsMouseButtonDown(int button)
{
    if (!g_windowForeground) return false;
    if (button == 0) return win32LMB;
    if (button == 1) return win32RMB;
    return false;
}

bool Win32IsKeyDown(int key)
{
    if (!g_windowForeground) return false;
    int vk = MapRaylibToVK(key);
    if (vk == 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool Win32IsKeyPressed(int key)
{
    if (!g_windowForeground) return false;
    int vk = MapRaylibToVK(key);
    if (vk == 0) return false;
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool wasDown = keyPrev[vk & 0xFF];
    keyPrev[vk & 0xFF] = down;
    return down && !wasDown;
}

// Character input queue for Win32
#define CHAR_QUEUE_SIZE 32
static int charQueue[CHAR_QUEUE_SIZE];
static int charQueueHead = 0;
static int charQueueTail = 0;
static bool charKeyPrev[256] = {0};

int Win32GetCharPressed(void)
{
    if (!g_windowForeground) return 0;

    // Drain: return queued character if available
    if (charQueueTail != charQueueHead) {
        int c = charQueue[charQueueTail];
        charQueueTail = (charQueueTail + 1) % CHAR_QUEUE_SIZE;
        return c;
    }

    // Generate: check for newly pressed character keys
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

    // Letters A-Z
    for (int vk = 0x41; vk <= 0x5A; vk++) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (down && !charKeyPrev[vk]) {
            int c = shift ? vk : (vk + 32);
            charKeyPrev[vk] = true;
            return c;
        }
        charKeyPrev[vk] = down;
    }

    // Digits 0-9
    for (int vk = 0x30; vk <= 0x39; vk++) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (down && !charKeyPrev[vk]) {
            charKeyPrev[vk] = true;
            if (shift) {
                static const char shiftDigits[] = ")!@#$%^&*(";
                return shiftDigits[vk - 0x30];
            }
            return vk;
        }
        charKeyPrev[vk] = down;
    }

    // Space
    {
        bool down = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
        if (down && !charKeyPrev[VK_SPACE]) {
            charKeyPrev[VK_SPACE] = true;
            return ' ';
        }
        charKeyPrev[VK_SPACE] = down;
    }

    // Period (for IP input)
    {
        bool down = (GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000) != 0;
        if (down && !charKeyPrev[VK_OEM_PERIOD]) {
            charKeyPrev[VK_OEM_PERIOD] = true;
            return shift ? '>' : '.';
        }
        charKeyPrev[VK_OEM_PERIOD] = down;
    }
    // Slash (for IP input)
    {
        bool down = (GetAsyncKeyState(VK_OEM_2) & 0x8000) != 0;
        if (down && !charKeyPrev[VK_OEM_2]) {
            charKeyPrev[VK_OEM_2] = true;
            return shift ? '?' : '/';
        }
        charKeyPrev[VK_OEM_2] = down;
    }

    return 0;
}

// Mouse wheel via thread-specific WH_GETMESSAGE hook on GLFW's thread
static volatile float wheelAccum = 0.0f;
static void *wheelHookHandle = NULL;

__declspec(dllimport) void __stdcall Sleep(unsigned long);
__declspec(dllimport) unsigned long __stdcall GetWindowThreadProcessId(void*, unsigned long*);
__declspec(dllimport) void* __stdcall SetWindowsHookExA(int, void*, void*, unsigned long);
__declspec(dllimport) int __stdcall UnhookWindowsHookEx(void*);
__declspec(dllimport) long long __stdcall CallNextHookEx(void*, int, long long, long long);
#define WH_GETMESSAGE 3
#define WM_MOUSEWHEEL 0x020A
#define HC_ACTION 0

static long long __stdcall WheelGetMsgProc(int nCode, long long wParam, long long lParam)
{
    if (nCode >= 0 && wParam == 1) {
        unsigned int *raw = (unsigned int *)lParam;
        unsigned int msgType = raw[2];
        if (msgType == WM_MOUSEWHEEL) {
            size_t *fields = (size_t *)lParam;
            size_t wp = fields[2];
            short delta = (short)(wp >> 16);
            wheelAccum += (float)delta / 120.0f;
        }
    }
    return CallNextHookEx(wheelHookHandle, nCode, wParam, lParam);
}

void InitWin32WheelHook(void)
{
    void *hwnd = GetWindowHandle();
    if (hwnd) {
        unsigned long glfwTid = GetWindowThreadProcessId(hwnd, NULL);
        wheelHookHandle = SetWindowsHookExA(WH_GETMESSAGE, (void*)WheelGetMsgProc, NULL, glfwTid);
    }
}

float Win32GetMouseWheelMove(void)
{
    if (!g_windowForeground) {
        wheelAccum = 0.0f;
        return 0.0f;
    }
    float v = wheelAccum;
    wheelAccum = 0.0f;
    return v;
}

//----------------------------------------------------------------------------------
// Global Definitions
//----------------------------------------------------------------------------------
uint8_t world[WORLD_WIDTH][WORLD_HEIGHT];
uint8_t lightMap[WORLD_WIDTH][WORLD_HEIGHT];
Chunk loadedChunks[MAX_CHUNKS];

Player players[MAX_NET_PLAYERS] = { 0 };
int localPlayerId = 0;
RemotePlayer remotePlayers[MAX_NET_PLAYERS] = { 0 };
Camera2D camera = { 0 };
DayNightCycle dayNight = { 0 };

Texture2D blockAtlas = { 0 };
Texture2D crackTextures[CRACK_STAGES] = { 0 };
bool showDebug = false;
bool showLargeMap = false;
bool inventoryOpen = false;
EnchantSession localEnchantSession = {0};
bool gamePaused = false;
unsigned int worldSeed = 0;

char messageText[128] = { 0 };
float messageTimer = 0.0f;
float messageSlide = 0.0f;
Color messageColor = { 240, 100, 100, 255 };

Mob mobs[MAX_MOBS];
float mobSpawnTimer = 0.0f;

ItemEntity entities[MAX_ENTITIES];
Projectile projectiles[MAX_PROJECTILES];
int pendingProjectileHitCount = 0;
int pendingProjectileHitIndex[MAX_PROJECTILES];
int pendingProjectileHitDamage[MAX_PROJECTILES];

CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
int craftRecipeCount = 0;

// Sound globals
Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
Sound sndHurt, sndDeath, sndEat, sndDrink, sndClick, sndCraft, sndXP, sndDrop;
Sound sndFootstep, sndZombie, sndPig, sndSplash, sndVillager;
Sound sndSkeleton, sndCreeperHiss, sndSpider, sndSlime, sndEnderman;
Sound sndHorse, sndWolf, sndWitch, sndBat;
Sound sndRain, sndCreeperFuse, sndThunder;
Sound sndCaveDrip, sndCaveAmbient, sndWind;
Sound sndPickup, sndBowFire;
Music bgm = { 0 };

bool audioReady = false;
GameState gameState = STATE_MENU;
Difficulty gameDifficulty = DIFFICULTY_NORMAL;

// Achievement tracking
bool achievements[ACH_COUNT] = {0};
int totalMobsKilled = 0;
int totalBlocksPlaced = 0;
float bgmVolumeSlider = 0.3f;
float sfxVolumeSlider = 0.7f;
int selectedSaveSlot = -1;
int slotSelectMode = 0; // 0=new game, 1=load game
int slotScrollOffset = 0;
int windowMode = 0; // 0=windowed, 1=fullscreen, 2=borderless
int resolutionPreset = 1;
RenderTexture2D logicalCanvas = { 0 };
bool logicalCanvasReady = false;
Rectangle logicalViewport = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
float logicalScale = 1.0f;
char seedInputBuf[32] = { 0 };
int seedInputLen = 0;
bool seedFieldFocused = false;   // true while the seed text field owns keyboard input

// Crafting search
char craftSearchBuf[32] = { 0 };
int craftSearchLen = 0;

// Confirmation dialog
bool confirmDialogActive = false;
int confirmDialogSlot = -1;
int confirmDialogMode = 0; // 0=overwrite, 1=delete

// Furnace UI state
bool furnaceOpen = false;
int furnaceBlockX = -1, furnaceBlockY = -1;
uint8_t furnaceFuel = 0;
int furnaceFuelCount = 0;
uint8_t furnaceInput = 0;
int furnaceInputCount = 0;
uint8_t furnaceOutput = 0;
int furnaceOutputCount = 0;
float furnaceProgress = 0.0f;
float furnaceFuelBurn = 0.0f;
float furnaceFuelBurnMax = 0.0f;

// Multi-furnace support
FurnaceData furnaces[MAX_FURNACES];
int furnaceCount = 0;
int activeFurnace = -1;

// Crafting table state
bool craftingTableOpen = false;

// Chest state
bool chestOpen = false;
int chestBlockX = -1, chestBlockY = -1;
ChestData chestData[MAX_CHESTS];
int chestCount = 0;

// Trading state
Trade trades[MAX_TRADES];
int tradeCount = 0;
bool tradeOpen = false;
int villagerTradeIndex = -1;

// Cauldron state
CauldronData cauldrons[MAX_CAULDRONS];
int cauldronCount = 0;

// Chat state
bool chatOpen = false;
char chatInput[MAX_CHAT_INPUT] = {0};
int chatInputLen = 0;
ChatMessage chatHistory[MAX_CHAT_MESSAGES];
int chatHistoryCount = 0;

// Smelting recipes
SmeltRecipe smeltRecipes[MAX_SMELT_RECIPES];
int smeltRecipeCount = 0;

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
    UpdateLogicalViewport();
    logicalCanvas = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
    logicalCanvasReady = logicalCanvas.texture.id != 0;
    SetTargetFPS(60);
    SetExitKey(0);
    SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
    InitWin32WheelHook();

    // Start audio loading in background thread
    _beginthread(AudioLoadThread, 0, NULL);

    // Pre-init: load systems needed for menu rendering
    LoadSettings();
    ApplyResolution(resolutionPreset);
    if (windowMode != 0) ApplyWindowMode(windowMode);
    LoadGameFont();
    GenerateBlockAtlas();
    InitCraftingRecipes();
    InitCameraSystem();
    InitDayNight();
    NetInit();

    gameState = STATE_MENU;


    while (!WindowShouldClose())
    {
        UpdateWin32Input();
        UpdateDrawFrame();
    }

    UnloadGame();
    if (logicalCanvasReady) UnloadRenderTexture(logicalCanvas);
    CloseWindow();
    return 0;
}
