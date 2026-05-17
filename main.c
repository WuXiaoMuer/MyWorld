/*******************************************************************************************
*
*   MyWorld - A 2D sandbox game inspired by Minecraft, built with raylib 5.5
*
********************************************************************************************/

#include "types.h"
#include <process.h>

//----------------------------------------------------------------------------------
// Win32 Input Override Implementation
//----------------------------------------------------------------------------------
int win32MouseX = 0, win32MouseY = 0;
int win32MousePrevX = 0, win32MousePrevY = 0;
bool win32LMB = false, win32LMBPrev = false;
bool win32RMB = false, win32RMBPrev = false;

// Per-frame "pressed this frame" tracking for keys
static bool keyPrev[256] = {0};

void UpdateWin32Input(void)
{
    // Save previous mouse state
    win32MousePrevX = win32MouseX;
    win32MousePrevY = win32MouseY;
    win32LMBPrev = win32LMB;
    win32RMBPrev = win32RMB;

    // Get raw screen cursor position
    long pt[2];
    GetCursorPos(pt);

    // Convert to window client coordinates
    void *hwnd = GetWindowHandle();
    if (hwnd) ScreenToClient(hwnd, pt);
    win32MouseX = (int)pt[0];
    win32MouseY = (int)pt[1];

    // Mouse buttons
    win32LMB = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    win32RMB = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
}

Vector2 Win32GetMousePosition(void)
{
    return (Vector2){ (float)win32MouseX, (float)win32MouseY };
}

Vector2 Win32GetMouseDelta(void)
{
    return (Vector2){ (float)(win32MouseX - win32MousePrevX), (float)(win32MouseY - win32MousePrevY) };
}

bool Win32IsMouseButtonPressed(int button)
{
    if (button == 0) return win32LMB && !win32LMBPrev;  // LEFT
    if (button == 1) return win32RMB && !win32RMBPrev;  // RIGHT
    return false;
}

bool Win32IsMouseButtonReleased(int button)
{
    if (button == 0) return !win32LMB && win32LMBPrev;
    if (button == 1) return !win32RMB && win32RMBPrev;
    return false;
}

bool Win32IsKeyDown(int key)
{
    int vk = 0;
    // Map raylib KEY_ codes to VK_ codes
    if (key >= KEY_A && key <= KEY_Z) vk = 0x41 + (key - KEY_A);
    else if (key >= KEY_ZERO && key <= KEY_NINE) vk = 0x30 + (key - KEY_ZERO);
    else if (key >= KEY_ONE && key <= KEY_NINE) vk = 0x31 + (key - KEY_ONE);
    else if (key == KEY_SPACE) vk = VK_SPACE;
    else if (key == KEY_ESCAPE) vk = VK_ESCAPE;
    else if (key == KEY_ENTER) vk = VK_RETURN;
    else if (key == KEY_BACKSPACE) vk = VK_BACK;
    else if (key == KEY_TAB) vk = VK_TAB;
    else if (key == KEY_DELETE) vk = VK_DELETE;
    else if (key == KEY_LEFT) vk = VK_LEFT;
    else if (key == KEY_RIGHT) vk = VK_RIGHT;
    else if (key == KEY_UP) vk = VK_UP;
    else if (key == KEY_DOWN) vk = VK_DOWN;
    else if (key == KEY_LEFT_SHIFT) vk = VK_SHIFT;
    else if (key == KEY_RIGHT_SHIFT) vk = VK_SHIFT;
    else if (key == KEY_LEFT_CONTROL) vk = VK_CONTROL;
    else if (key == KEY_RIGHT_CONTROL) vk = VK_CONTROL;
    else if (key == KEY_LEFT_ALT) vk = VK_MENU;
    else if (key == KEY_RIGHT_ALT) vk = VK_MENU;
    else if (key == KEY_F3) vk = VK_F3;
    else if (key == KEY_F11) vk = VK_F11;
    if (vk == 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

bool Win32IsKeyPressed(int key)
{
    int vk = 0;
    if (key >= KEY_A && key <= KEY_Z) vk = 0x41 + (key - KEY_A);
    else if (key >= KEY_ZERO && key <= KEY_NINE) vk = 0x30 + (key - KEY_ZERO);
    else if (key >= KEY_ONE && key <= KEY_NINE) vk = 0x31 + (key - KEY_ONE);
    else if (key == KEY_SPACE) vk = VK_SPACE;
    else if (key == KEY_ESCAPE) vk = VK_ESCAPE;
    else if (key == KEY_ENTER) vk = VK_RETURN;
    else if (key == KEY_BACKSPACE) vk = VK_BACK;
    else if (key == KEY_TAB) vk = VK_TAB;
    else if (key == KEY_DELETE) vk = VK_DELETE;
    else if (key == KEY_LEFT) vk = VK_LEFT;
    else if (key == KEY_RIGHT) vk = VK_RIGHT;
    else if (key == KEY_UP) vk = VK_UP;
    else if (key == KEY_DOWN) vk = VK_DOWN;
    else if (key == KEY_LEFT_SHIFT) vk = VK_SHIFT;
    else if (key == KEY_RIGHT_SHIFT) vk = VK_SHIFT;
    else if (key == KEY_LEFT_CONTROL) vk = VK_CONTROL;
    else if (key == KEY_RIGHT_CONTROL) vk = VK_CONTROL;
    else if (key == KEY_LEFT_ALT) vk = VK_MENU;
    else if (key == KEY_RIGHT_ALT) vk = VK_MENU;
    else if (key == KEY_F3) vk = VK_F3;
    else if (key == KEY_F11) vk = VK_F11;
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

static void PushChar(int c)
{
    int next = (charQueueHead + 1) % CHAR_QUEUE_SIZE;
    if (next != charQueueTail) {
        charQueue[charQueueHead] = c;
        charQueueHead = next;
    }
}

int Win32GetCharPressed(void)
{
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
            int c = shift ? vk : (vk + 32); // uppercase or lowercase
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
                // Shift+digit symbols on US keyboard
                static const char shiftDigits[] = ")!@#$%^&*(";
                return shiftDigits[vk - 0x30];
            }
            return vk; // '0'-'9'
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

// Thread-specific hook: intercepts WM_MOUSEWHEEL from GLFW's message loop.
// lParam points to MSG struct when nCode >= 0 and wParam == PM_REMOVE (1).
// MSG layout (64-bit): hwnd(8) + message(4) + pad(4) + wP(8) + lP(8) + time(4) + pad(4) + ptX(4) + ptY(4)
// message at byte 8, wParam at byte 16
static long long __stdcall WheelGetMsgProc(int nCode, long long wParam, long long lParam)
{
    if (nCode >= 0 && wParam == 1) { // HC_ACTION, PM_REMOVE
        unsigned int *raw = (unsigned int *)lParam;
        unsigned int msgType = raw[2]; // byte 8 / sizeof(UINT)=4 → index 2 (on both 32/64 with padding)
        if (msgType == WM_MOUSEWHEEL) {
            // On 64-bit: wParam field at byte 16 → size_t index = 16/8 = 2
            // On 32-bit: wParam field at byte 8 → size_t index = 8/4 = 2
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
    // Get GLFW window's thread ID and install hook on that thread
    void *hwnd = GetWindowHandle();
    if (hwnd) {
        unsigned long glfwTid = GetWindowThreadProcessId(hwnd, NULL);
        wheelHookHandle = SetWindowsHookExA(WH_GETMESSAGE, (void*)WheelGetMsgProc, NULL, glfwTid);
    }
}

float Win32GetMouseWheelMove(void)
{
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

Player player = { 0 };
Camera2D camera = { 0 };
DayNightCycle dayNight = { 0 };

Texture2D blockAtlas = { 0 };
Texture2D crackTextures[CRACK_STAGES] = { 0 };
bool showDebug = false;
bool showLargeMap = false;
bool inventoryOpen = false;
bool gamePaused = false;
unsigned int worldSeed = 0;

char messageText[128] = { 0 };
float messageTimer = 0.0f;
Color messageColor = { 240, 100, 100, 255 };

Mob mobs[MAX_MOBS];
float mobSpawnTimer = 0.0f;

ItemEntity entities[MAX_ENTITIES];
Projectile projectiles[MAX_PROJECTILES];

CraftingRecipe craftRecipes[MAX_CRAFT_RECIPES];
int craftRecipeCount = 0;

// Sound globals
Sound sndBreak, sndBreakStone, sndPlace, sndJump, sndLand;
Sound sndHurt, sndDeath, sndEat, sndClick, sndCraft, sndXP, sndDrop;
Sound sndFootstep, sndZombie, sndPig, sndSplash;
Music bgm = { 0 };

bool audioReady = false;
GameState gameState = STATE_MENU;
float bgmVolumeSlider = 0.3f;
float sfxVolumeSlider = 0.7f;
int selectedSaveSlot = -1;
int slotSelectMode = 0; // 0=new game, 1=load game
int slotScrollOffset = 0;
int windowMode = 0; // 0=windowed, 1=fullscreen, 2=borderless
char seedInputBuf[32] = { 0 };
int seedInputLen = 0;

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

// Crafting table state
bool craftingTableOpen = false;

// Chest state
bool chestOpen = false;
int chestBlockX = -1, chestBlockY = -1;
ChestData chestData[MAX_CHESTS];
int chestCount = 0;

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
    SetTargetFPS(60);
    SetExitKey(0);
    SetWindowState(FLAG_WINDOW_ALWAYS_RUN);
    InitWin32WheelHook();

    // Start audio loading in background thread
    _beginthread(AudioLoadThread, 0, NULL);

    // Pre-init: load systems needed for menu rendering
    LoadSettings();
    if (windowMode != 0) ApplyWindowMode(windowMode);
    LoadGameFont();
    GenerateBlockAtlas();
    InitCraftingRecipes();
    InitCameraSystem();
    InitDayNight();

    gameState = STATE_MENU;

    while (!WindowShouldClose())
    {
        UpdateWin32Input();
        UpdateDrawFrame();
    }

    UnloadGame();
    CloseWindow();
    return 0;
}
