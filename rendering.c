#include "types.h"
#include "net.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#include <string.h>

static const char* GetEnchantName(EnchantmentType type);

// Forward declarations (drawn helpers used before their definition in this file)
static void DrawUiBox(int x, int y, int w, int h, float radius, Color color);
static void DrawCompetitionIcon(int x, int y, int size, unsigned char alpha, bool unlocked);
static void DrawCollectibleIcon(int x, int y, int size, int variant, unsigned char alpha, bool unlocked);

//----------------------------------------------------------------------------------
// HUD vertical layout — single source of truth
// The hotbar, selected-item name and player-status bars all stack above the
// hotbar; each gets its own reserved band so nothing overlaps. Bands are laid
// out bottom-up from the screen edge (720-high screen assumed).
//----------------------------------------------------------------------------------
#define HUD_SLOT_SIZE     44
#define HUD_SLOT_PAD      4
#define HUD_HOTBAR_Y      (SCREEN_HEIGHT - HUD_SLOT_SIZE - 12)          // 664
#define HUD_XP_BAR_H      4
#define HUD_XP_BAR_Y      (HUD_HOTBAR_Y - HUD_XP_BAR_H - 4)             // 656
#define HUD_XP_LABEL_Y    (HUD_XP_BAR_Y + HUD_XP_BAR_H + 2)             // 662
#define HUD_SEL_NAME_Y    (HUD_XP_BAR_Y - 34)                           // 622 name box 620-652 (enchant)
#define HUD_STATUS_Y      (HUD_SEL_NAME_Y - 34)                         // 588 status icons
#define HUD_BOSS_BAR_Y    (HUD_STATUS_Y - 30)                           // 558 boss health band

//----------------------------------------------------------------------------------
// Minecraft-style UI palette — authentic colors from the vanilla GUI.
// Inventory panels are LIGHT gray (#C6C6C6) with white top-left bevel and
// #555555 bottom-right shadow; slots are #8B8B8B recessed with #373737
// top-left and white bottom-right inner edges. Buttons are stone gray with a
// black outline. Text carries a hard 1px offset shadow like MC.
//----------------------------------------------------------------------------------
#define UI_PANEL      (Color){198, 198, 198, 255}   // light GUI panel body (#C6C6C6)
#define UI_PANEL_DK   (Color){ 85,  85,  85, 255}   // panel shadow edge (#555)
#define UI_PANEL_LT   (Color){255, 255, 255, 255}   // panel highlight edge
#define UI_SLOT       (Color){139, 139, 139, 255}   // slot body (#8B8B8B)
#define UI_SLOT_DK    (Color){ 55,  55,  55, 255}   // slot inner shadow (#373737)
#define UI_SLOT_HOVER (Color){160, 160, 160, 255}
#define UI_BTN        (Color){108, 108, 108, 255}   // stone button face (#6C6C6C)
#define UI_BTN_TOP    (Color){160, 160, 160, 255}   // raised top/left highlight
#define UI_BTN_BOT    (Color){ 70,  70,  70, 255}   // raised bottom/right shadow
#define UI_BTN_HOVER  (Color){126, 126, 136, 255}   // hovered face (slight cool tint)
#define UI_BTN_OFF    (Color){ 64,  64,  64, 255}   // disabled face
#define UI_TEXT_ON_LT (Color){ 62,  62,  62, 255}   // dark text on light panels (#3E)
#define UI_TEXT       (Color){255, 255, 255, 255}   // white text (buttons/tooltips)
#define UI_TEXT_DIM   (Color){160, 160, 160, 255}
#define UI_TEXT_OFF   (Color){160, 160, 160, 255}
#define UI_TTIP_BG    (Color){ 16,   0,  16, 240}   // tooltip near-black purple
#define UI_TTIP_EDGE  (Color){ 80,   0, 255, 160}   // tooltip purple border
#define UI_DANGER     (Color){255,  85,  85, 255}
#define UI_GOLD       (Color){255, 205,  90, 255}

// Draw a Minecraft-style beveled box. `raised` = panel/button sticking out
// (light top-left, dark bottom-right); flat/recessed = the inverse. Edges are
// 2px for the chunky MC look.
static void DrawBevelBox(int x, int y, int w, int h, Color fill, bool raised)
{
    if (w <= 0 || h <= 0) return;
    DrawRectangle(x, y, w, h, fill);
    Color lt = raised ? UI_PANEL_LT : UI_PANEL_DK;
    Color dk = raised ? UI_PANEL_DK : UI_PANEL_LT;
    DrawRectangle(x, y, w, 2, lt);                 // top
    DrawRectangle(x, y, 2, h, lt);                 // left
    DrawRectangle(x, y + h - 2, w, 2, dk);         // bottom
    DrawRectangle(x + w - 2, y, 2, h, dk);         // right
}

// White text with a hard dark offset shadow — the MC text treatment. Use for
// any text sitting directly on the game world or on gray slots.
static void DrawTextMC(const char *text, int x, int y, int fontSize, Color color)
{
    // Hard 1px black shadow (MC style). The shadow must be much darker than the
    // glyph or the text looks doubled instead of outlined.
    DrawGameText(text, x + 1, y + 1, fontSize, (Color){0, 0, 0, color.a});
    DrawGameText(text, x, y, fontSize, color);
}

// Title-screen background: vanilla-style dirt. Each 16px tile has a few
// chunky grains (matching MC's 16x16 tile scaled up) so it reads as soil
// rather than a flat checkerboard. A dark overlay keeps UI text legible.
static void DrawDirtBackground(unsigned char overlayAlpha)
{
    const int tile = 16;
    // Base dirt tones (warm browns)
    static const Color base[4] = {
        {134,  96,  67, 255},
        {124,  88,  60, 255},
        {143, 103,  72, 255},
        {117,  82,  56, 255}
    };
    static const Color grain[2] = {
        { 96,  67,  45, 255},   // dark grain
        {160, 118,  84, 255}    // light grain
    };

    for (int ty = 0; ty < SCREEN_HEIGHT; ty += tile) {
        for (int tx = 0; tx < SCREEN_WIDTH; tx += tile) {
            unsigned int h0 = (unsigned int)((tx / tile) * 73856093u ^ (ty / tile) * 19349663u);
            DrawRectangle(tx, ty, tile, tile, base[h0 % 4]);

            // Exactly two chunky 4x4 grains per tile, placed deterministically
            for (int g = 0; g < 2; g++) {
                unsigned int h = h0 ^ (unsigned int)(g * 2654435761u);
                int gx = (int)((h >> 4) % 3) * 4;   // 0, 4, 8
                int gy = (int)((h >> 8) % 3) * 4;
                DrawRectangle(tx + gx, ty + gy, 4, 4, grain[(h >> 12) & 1]);
            }
        }
    }
    if (overlayAlpha > 0) DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, overlayAlpha});
}

//----------------------------------------------------------------------------------
// Main-menu background - a parallax dusk scene.
//
// The menu previously sat on the same tiled dirt as the in-game pause screen,
// which read as flat and unfinished. This paints a sky instead: a vertical
// gradient, two layers of hills scrolling at different rates, slow clouds, and
// a warm horizon glow, so the title has something to sit against.
//
// Parallax uses a slow time value rather than the camera (there is none here);
// each layer gets a different speed so the scene has depth.
//----------------------------------------------------------------------------------
static void DrawMenuBackground(float time)
{
    // --- Sky gradient: deep blue up top easing into warm dusk at the horizon ---
    const Color skyTop    = { 38,  52,  92, 255};
    const Color skyMid    = { 92, 106, 148, 255};
    const Color skyLow    = {196, 150, 118, 255};
    const Color skyHorizon= {226, 172, 120, 255};
    const int horizonY = SCREEN_HEIGHT - 150;

    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        float t = (float)y / (float)horizonY;
        if (t > 1.0f) t = 1.0f;
        Color c;
        if (t < 0.62f) {
            float u = t / 0.62f;
            c = (Color){
                (unsigned char)(skyTop.r + (skyMid.r - skyTop.r) * u),
                (unsigned char)(skyTop.g + (skyMid.g - skyTop.g) * u),
                (unsigned char)(skyTop.b + (skyMid.b - skyTop.b) * u), 255};
        } else {
            float u = (t - 0.62f) / 0.38f;
            if (u > 1.0f) u = 1.0f;
            if (u < 0.5f) {
                float v = u / 0.5f;
                c = (Color){
                    (unsigned char)(skyMid.r + (skyLow.r - skyMid.r) * v),
                    (unsigned char)(skyMid.g + (skyLow.g - skyMid.g) * v),
                    (unsigned char)(skyMid.b + (skyLow.b - skyMid.b) * v), 255};
            } else {
                float v = (u - 0.5f) / 0.5f;
                c = (Color){
                    (unsigned char)(skyLow.r + (skyHorizon.r - skyLow.r) * v),
                    (unsigned char)(skyLow.g + (skyHorizon.g - skyLow.g) * v),
                    (unsigned char)(skyLow.b + (skyHorizon.b - skyLow.b) * v), 255};
            }
        }
        DrawRectangle(0, y, SCREEN_WIDTH, 1, c);
    }

    // --- Stars in the upper sky, fading toward the horizon ---
    for (int i = 0; i < 70; i++) {
        unsigned int h = (unsigned int)(i * 2654435761u);
        int sx = (int)(h % SCREEN_WIDTH);
        int sy = (int)((h >> 9) % (unsigned)(horizonY - 120));
        float tw = sinf(time * 1.4f + (float)i) * 0.5f + 0.5f;
        float fade = 1.0f - (float)sy / (float)(horizonY - 120);
        unsigned char a = (unsigned char)(tw * fade * 190.0f);
        DrawRectangle(sx, sy, 1, 1, (Color){255, 255, 240, a});
    }

    // --- Sun low on the horizon with a soft halo ---
    {
        int sunX = SCREEN_WIDTH / 2 + 220;
        int sunY = horizonY - 34;
        for (int r = 90; r > 0; r -= 6) {
            unsigned char a = (unsigned char)(10 + (90 - r) * 0.5f);
            DrawCircle(sunX, sunY, (float)r, (Color){255, 214, 150, a});
        }
        DrawCircle(sunX, sunY, 22.0f, (Color){255, 238, 200, 255});
        DrawCircle(sunX, sunY, 18.0f, (Color){255, 248, 224, 255});
    }

    // --- Clouds: chunky MC-style slabs drifting at two speeds ---
    for (int layer = 0; layer < 2; layer++) {
        float speed = (layer == 0) ? 5.0f : 11.0f;
        int baseY = (layer == 0) ? 66 : 128;
        unsigned char calpha = (layer == 0) ? 190 : 150;
        Color cloud = (Color){238, 236, 244, calpha};
        for (int i = 0; i < 4; i++) {
            unsigned int h = (unsigned int)((i + layer * 7) * 40503u);
            float w = 90.0f + (float)(h % 130);
            float span = SCREEN_WIDTH + 260.0f;
            float cx = fmodf((float)(h % 1000) + time * speed, span) - 130.0f;
            int cy = baseY + (int)(h % 40);
            DrawRectangle((int)cx, cy, (int)w, 13, cloud);
            DrawRectangle((int)(cx + 22), cy - 9, (int)(w * 0.55f), 12, cloud);
            DrawRectangle((int)(cx + 46), cy - 15, (int)(w * 0.28f), 10, cloud);
        }
    }

    // --- Hills: two silhouettes, the far one lighter and slower ---
    {
        const Color farHill  = { 78, 96, 120, 255};
        const Color nearHill = { 52, 70,  86, 255};
        for (int layer = 0; layer < 2; layer++) {
            Color c = (layer == 0) ? farHill : nearHill;
            float speed = (layer == 0) ? 3.0f : 7.0f;
            float amp   = (layer == 0) ? 42.0f : 64.0f;
            float base  = (layer == 0) ? horizonY - 6.0f : horizonY + 16.0f;
            float off   = time * speed;
            for (int x = 0; x < SCREEN_WIDTH; x += 2) {
                float u = (float)x + off;
                float y = base
                        - sinf(u * 0.0042f) * amp
                        - sinf(u * 0.0113f + 1.7f) * amp * 0.35f;
                DrawRectangle(x, (int)y, 2, SCREEN_HEIGHT - (int)y, c);
            }
        }
    }

    // --- Horizon glow: warm wash where the sky meets the hills ---
    for (int i = 0; i < 26; i++) {
        unsigned char a = (unsigned char)(16 - i * 0.6f);
        DrawRectangle(0, horizonY - 30 + i, SCREEN_WIDTH, 1, (Color){255, 198, 140, a});
    }
}

//----------------------------------------------------------------------------------
// Smooth hover animation system
//----------------------------------------------------------------------------------
#define MAX_HOVER_SLOTS 768
static float hoverAlphas[MAX_HOVER_SLOTS] = {0};

float GetHoverAlpha(int slotId, bool hovered, float dt)
{
    if (slotId < 0 || slotId >= MAX_HOVER_SLOTS) return hovered ? 1.0f : 0.0f;
    float target = hovered ? 1.0f : 0.0f;
    float speed = 8.0f; // reaches target in ~0.125s
    hoverAlphas[slotId] += (target - hoverAlphas[slotId]) * speed * dt;
    if (hoverAlphas[slotId] < 0.01f) hoverAlphas[slotId] = 0.0f;
    if (hoverAlphas[slotId] > 0.99f) hoverAlphas[slotId] = 1.0f;
    return hoverAlphas[slotId];
}

//----------------------------------------------------------------------------------
// MC stone button: gray face, black outline, light top edge. Hover brightens
// with a white inset frame; label carries the hard MC text shadow.
//----------------------------------------------------------------------------------
void DrawUiButton(float x, float y, float w, float h, const char *label,
                  int fontSize, bool hover, bool selected, bool enabled, float alpha)
{
    if (alpha <= 0.01f || w <= 0 || h <= 0) return;
    int ix = (int)x, iy = (int)y, iw = (int)w, ih = (int)h;

    Color fill = UI_BTN;
    Color textCol = UI_TEXT;
    if (!enabled) {
        fill = UI_BTN_OFF;
        textCol = (Color){160, 160, 160, 255};
    } else if (hover || selected) {
        fill = UI_BTN_HOVER;
    }

    fill.a = (unsigned char)(fill.a * alpha);
    textCol.a = (unsigned char)(textCol.a * alpha);

    // MC stone button: black outline, then a raised bevel (light top-left, dark
    // bottom-right) around a flat gray face. Small buttons get a 2px bevel so
    // the text still has room; large ones get 3px.
    int bev = (ih >= 26) ? 3 : 2;
    DrawRectangle(ix - 1, iy - 1, iw + 2, ih + 2, (Color){0, 0, 0, (unsigned char)(255 * alpha)});
    DrawRectangle(ix, iy, iw, ih, fill);
    Color top = {UI_BTN_TOP.r, UI_BTN_TOP.g, UI_BTN_TOP.b, (unsigned char)(255 * alpha)};
    Color bot = {UI_BTN_BOT.r, UI_BTN_BOT.g, UI_BTN_BOT.b, (unsigned char)(255 * alpha)};
    DrawRectangle(ix, iy, iw, bev, top);
    DrawRectangle(ix, iy, bev, ih, top);
    DrawRectangle(ix, iy + ih - bev, iw, bev, bot);
    DrawRectangle(ix + iw - bev, iy, bev, ih, bot);

    // Hover/selected: white inset frame (MC highlight)
    if (enabled && (hover || selected)) {
        DrawRectangleLines(ix + 1, iy + 1, iw - 2, ih - 2,
                           (Color){255, 255, 255, (unsigned char)(220 * alpha)});
    }

    // Label with hard shadow
    if (label && label[0]) {
        int tw = MeasureGameTextWidth(label, fontSize);
        int tx = ix + (iw - tw) / 2;
        int ty = iy + (ih - fontSize) / 2;
        DrawTextMC(label, tx, ty, fontSize, textCol);
    }
}

//----------------------------------------------------------------------------------
// Shared UI panel: authentic light-gray MC inventory panel (#C6C6C6) with
// white/#555 2px bevel. Text drawn on it must be dark (UI_TEXT_ON_LT).
//----------------------------------------------------------------------------------
void DrawUiPanel(int x, int y, int w, int h, unsigned char alpha)
{
    if (w <= 0 || h <= 0) return;
    Color body = UI_PANEL;
    body.a = alpha;
    DrawBevelBox(x, y, w, h, body, true);
    // Thin inner shadow line under the top edge for depth
    DrawRectangle(x + 2, y + 2, w - 4, 1, (Color){0, 0, 0, (unsigned char)(alpha * 0.18f)});
}

// Minecraft-style slot: dark #8B8B8B recessed square. Vanilla draws a 1px
// #373737 line along the top/left and a 1px #FFFFFF line along the bottom/right
// of the *inner* area, which reads as a sunken well.
void DrawUiSlot(int x, int y, int size, bool hover, bool selected, float alpha)
{
    int a = (int)(255 * alpha);
    Color fill = hover ? UI_SLOT_HOVER : UI_SLOT;
    fill.a = (unsigned char)a;
    Color dark = {UI_SLOT_DK.r, UI_SLOT_DK.g, UI_SLOT_DK.b, (unsigned char)a};
    Color lite = {255, 255, 255, (unsigned char)a};

    DrawRectangle(x, y, size, size, fill);
    // Sunken inner bevel: dark on top/left, white on bottom/right
    DrawRectangle(x, y, size, 1, dark);
    DrawRectangle(x, y, 1, size, dark);
    DrawRectangle(x, y + size - 1, size, 1, lite);
    DrawRectangle(x + size - 1, y, 1, size, lite);

    if (selected) {
        // MC selected-slot: white frame drawn just outside the slot
        DrawRectangleLinesEx((Rectangle){(float)x - 1, (float)y - 1, (float)size + 2, (float)size + 2},
                             2, (Color){255, 255, 255, (unsigned char)(235 * alpha)});
    }
}

//----------------------------------------------------------------------------------
// MC-style slider: recessed dark groove with a raised stone knob.
// Shared by the pause menu and the settings screen so both look identical.
//----------------------------------------------------------------------------------
void DrawUiSlider(int x, int y, int w, float value, bool hot, float alpha)
{
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    int a = (int)(255 * alpha);
    const int trackH = 8;
    int ty = y - trackH / 2;

    // Recessed groove: dark top/left inner edge, light bottom/right
    DrawRectangle(x, ty, w, trackH, (Color){48, 48, 48, (unsigned char)a});
    DrawRectangle(x, ty, w, 1, (Color){28, 28, 28, (unsigned char)a});
    DrawRectangle(x, ty, 1, trackH, (Color){28, 28, 28, (unsigned char)a});
    DrawRectangle(x, ty + trackH - 1, w, 1, (Color){120, 120, 120, (unsigned char)a});

    // Filled portion
    int fillW = (int)(value * w);
    if (fillW > 0) {
        DrawRectangle(x + 1, ty + 1, fillW > 2 ? fillW - 1 : fillW, trackH - 2,
                      (Color){178, 178, 178, (unsigned char)a});
    }

    // Raised stone knob (9x18) with a 1px black outline
    int kx = x + fillW - 4;
    if (kx < x - 4) kx = x - 4;
    if (kx > x + w - 5) kx = x + w - 5;
    int ky = y - 9;
    DrawRectangle(kx - 1, ky - 1, 11, 20, (Color){0, 0, 0, (unsigned char)a});
    Color face = hot ? (Color){225, 225, 225, (unsigned char)a} : (Color){190, 190, 190, (unsigned char)a};
    DrawRectangle(kx, ky, 9, 18, face);
    DrawRectangle(kx, ky, 9, 2, (Color){240, 240, 240, (unsigned char)a});
    DrawRectangle(kx, ky, 2, 18, (Color){240, 240, 240, (unsigned char)a});
    DrawRectangle(kx, ky + 16, 9, 2, (Color){130, 130, 130, (unsigned char)a});
    DrawRectangle(kx + 7, ky, 2, 18, (Color){130, 130, 130, (unsigned char)a});
}

//----------------------------------------------------------------------------------
// Tooltip placement — clamped to the screen and flipped at edges.
// Every tooltip (hotbar, inventory, creative, chest, armor) uses this so the
// panel never runs off-screen and behaves consistently near the right/bottom.
//----------------------------------------------------------------------------------
void GetUiTooltipPos(int mouseX, int mouseY, int w, int h, int *outX, int *outY)
{
    const int gap = 14;
    const int margin = 4;
    int tx = mouseX + gap;
    int ty = mouseY - h / 2;               // vertically centered on the cursor
    if (tx + w + margin > SCREEN_WIDTH) tx = mouseX - w - gap;   // flip left
    if (ty + h + margin > SCREEN_HEIGHT) ty = SCREEN_HEIGHT - h - margin; // clamp bottom
    if (ty < margin) ty = margin;                                  // clamp top
    if (tx < margin) tx = margin;                                  // clamp left
    *outX = tx;
    *outY = ty;
}
// Held item state for inventory drag-and-drop
uint8_t heldItem = BLOCK_AIR;
int heldCount = 0;
int heldDurability = 0;
uint16_t heldItemEnchant = 0;

void ReturnHeldItem(void)
{
    if (heldItem == BLOCK_AIR) return;
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            player.inventory[i] = heldItem;
            player.inventoryCount[i] = heldCount;
            player.toolDurability[i] = heldDurability;
            player.itemEnchantments[i] = heldItemEnchant;
            heldItem = BLOCK_AIR;
            heldCount = 0;
            heldDurability = 0;
            heldItemEnchant = 0;
            return;
        }
    }
}

//----------------------------------------------------------------------------------
// Menu UI Helper Functions
//----------------------------------------------------------------------------------

// Draw a rounded rectangle (simulated with overlapping rectangles + circles)
// Flat UI box (MC style — no rounded corners). Corner radius is intentionally
// ignored; the panel helpers handle beveled edges where needed.
static void DrawUiBox(int x, int y, int w, int h, float radius, Color color)
{
    (void)radius;
    if (w <= 0 || h <= 0) return;
    DrawRectangle(x, y, w, h, color);
}

void ClearHeldItem(void)
{
    heldItem = BLOCK_AIR;
    heldCount = 0;
    heldDurability = 0;
    heldItemEnchant = 0;
}

// Crafting scroll state
int craftScrollOffset = 0;

// Death state
static float deathFadeTimer = 0.0f;
static StringId lastDeathCause = STR_YOU_DIED;

void SetDeathCause(StringId cause) { lastDeathCause = cause; }

// Subtle animated purple shimmer drawn over enchanted item icons (UI polish).
static void DrawEnchantGlint(int x, int y, int size)
{
    float t = (float)GetTime();
    unsigned char a = (unsigned char)(55 + 40 * (0.5f + 0.5f * sinf(t * 3.0f)));
    DrawRectangleLines(x, y, size, size, (Color){180, 110, 255, a});
    for (int s = 0; s < 2; s++) {
        float ph = t * 2.0f + s * 3.14159f;
        int sx = x + (int)(size * (0.3f + 0.4f * (0.5f + 0.5f * sinf(ph))));
        int sy = y + (int)(size * (0.25f + 0.5f * (0.5f + 0.5f * cosf(ph * 1.3f))));
        unsigned char sa = (unsigned char)(120 + 100 * (0.5f + 0.5f * sinf(ph * 2.0f)));
        DrawRectangle(sx, sy, 2, 2, (Color){235, 205, 255, sa});
    }
}

//----------------------------------------------------------------------------------
// Inventory Sort
//----------------------------------------------------------------------------------
void SortInventory(void)
{
    // Collect all non-empty items with their counts and durability
    typedef struct { uint8_t item; int count; int durability; uint16_t ench; } SortEntry;
    SortEntry entries[INVENTORY_SLOTS];
    int entryCount = 0;

    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] != BLOCK_AIR) {
            entries[entryCount].item = player.inventory[i];
            entries[entryCount].count = player.inventoryCount[i];
            entries[entryCount].durability = player.toolDurability[i];
            entries[entryCount].ench = player.itemEnchantments[i];
            entryCount++;
        }
    }

    // Sort by type (tools first, then blocks, then food), then by item ID
    for (int i = 0; i < entryCount - 1; i++) {
        for (int j = i + 1; j < entryCount; j++) {
            bool swap = false;
            int ti = (IsTool((BlockType)entries[i].item) || IsArmor((BlockType)entries[i].item)) ? 0 : (entries[i].item >= FOOD_RAW_PORK ? 2 : 1);
            int tj = (IsTool((BlockType)entries[j].item) || IsArmor((BlockType)entries[j].item)) ? 0 : (entries[j].item >= FOOD_RAW_PORK ? 2 : 1);
            if (tj < ti) swap = true;
            else if (tj == ti && entries[j].item < entries[i].item) swap = true;
            if (swap) {
                SortEntry tmp = entries[i];
                entries[i] = entries[j];
                entries[j] = tmp;
            }
        }
    }

    // Try to merge stacks of the same item (non-tools, non-armor)
    for (int i = 0; i < entryCount; i++) {
        if (IsTool((BlockType)entries[i].item) || IsArmor((BlockType)entries[i].item)) continue;
        for (int j = i + 1; j < entryCount; j++) {
            if (entries[j].item == entries[i].item && !IsTool((BlockType)entries[j].item) && !IsArmor((BlockType)entries[j].item)) {
                int space = 64 - entries[i].count;
                int toAdd = entries[j].count > space ? space : entries[j].count;
                entries[i].count += toAdd;
                entries[j].count -= toAdd;
                if (entries[j].count <= 0) {
                    // Remove entry j
                    for (int k = j; k < entryCount - 1; k++) entries[k] = entries[k + 1];
                    entryCount--;
                    j--;
                }
            }
        }
    }

    // Write back to inventory
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (i < entryCount) {
            player.inventory[i] = entries[i].item;
            player.inventoryCount[i] = entries[i].count;
            player.toolDurability[i] = entries[i].durability;
            player.itemEnchantments[i] = entries[i].ench;
        } else {
            player.inventory[i] = BLOCK_AIR;
            player.inventoryCount[i] = 0;
            player.toolDurability[i] = 0;
            player.itemEnchantments[i] = 0;
        }
    }
}

//----------------------------------------------------------------------------------
// Inventory Screen
//----------------------------------------------------------------------------------
void DrawInventoryScreen(void)
{
    // Panel open/close animation
    static float panelAnim = 0.0f;
    float dt = GetFrameTime();
    float target = inventoryOpen ? 1.0f : 0.0f;
    panelAnim += (target - panelAnim) * 25.0f * dt;
    if (panelAnim < 0.01f && !inventoryOpen) return;
    if (panelAnim > 0.99f) panelAnim = 1.0f;

    // Reset scroll when inventory first opens
    static bool prevInvOpen = false;
    if (inventoryOpen && !prevInvOpen) craftScrollOffset = 0;
    prevInvOpen = inventoryOpen;

    // Snapshot inventory + armor at the start of the frame so we can detect
    // modifications and sync them in multiplayer.
    bool doInvSync = (inventoryOpen && NetIsConnected());
    uint8_t  snapInv[INVENTORY_SLOTS];
    int      snapCount[INVENTORY_SLOTS];
    int      snapDur[INVENTORY_SLOTS];
    uint16_t snapEnch[INVENTORY_SLOTS];
    uint8_t  snapArmor[4];
    int      snapArmorDur[4];
    uint16_t snapArmorEnch[4];
    if (doInvSync) {
        memcpy(snapInv,   player.inventory,        sizeof(snapInv));
        memcpy(snapCount, player.inventoryCount,   sizeof(snapCount));
        memcpy(snapDur,   player.toolDurability,   sizeof(snapDur));
        memcpy(snapEnch,  player.itemEnchantments, sizeof(snapEnch));
        memcpy(snapArmor, player.armor,            sizeof(snapArmor));
        memcpy(snapArmorDur,  player.armorDurability,  sizeof(snapArmorDur));
        memcpy(snapArmorEnch, player.armorEnchantments, sizeof(snapArmorEnch));
    }

    int slotSize = 40;

    // --- Minecraft-style combined furnace + inventory screen ---
    if (brewingOpen && !furnaceOpen) {
        // Snapshot active furnace state at the start of the frame so we can detect
        // modifications and sync them to the host in multiplayer.

        int padding = 3;
        int armorSlotSize = 40;
        int armorPad = 3;
        int armorColW = armorSlotSize + armorPad;
        int previewW = 60;
        int previewPad = 6;
        int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;

        int contW = 14 + previewW + previewPad + armorColW + gridW + 14;
        int brewH = 175;
        int dividerH = 2;
        int invTitleH = 20;
        int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
        int contH = brewH + dividerH + invTitleH + gridH + 14;

        int contX = (SCREEN_WIDTH - contW) / 2;
        int contY = (SCREEN_HEIGHT - contH) / 2;
        contY += (int)((1.0f - panelAnim) * 40.0f);
        int panelPad = 14;

        Vector2 mouse = Win32GetMousePosition();

        // Light overlay
        unsigned char furnaceOA = (unsigned char)(100 * panelAnim);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, furnaceOA});

        // Container
        DrawUiPanel(contX, contY, contW, contH, 255);

        // --- Furnace section ---
        int fuSlotSize = 40;
        int fuY = contY + panelPad;
        DrawGameText(S(STR_BREWING), contX + contW / 2 - MeasureGameTextWidth(S(STR_BREWING), 16) / 2, fuY, 16, (Color){62, 62, 62, 255});
        fuY += 28;

        int fuSlotY = fuY;
        int fuCenterX = contX + contW / 2;
        int fuelX = fuCenterX - 120;
        int inputX = fuCenterX - fuSlotSize / 2;
        int outputX = fuCenterX + 80;

        // Fuel slot
        Rectangle fuelRect = { (float)fuelX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool fuelHover = CheckCollisionPointRec(mouse, fuelRect);
        DrawUiSlot(fuelX, fuSlotY, fuSlotSize, fuelHover, false, 1.0f);
        DrawGameText(S(STR_BOTTLE), fuelX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_FUEL), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Input slot
        Rectangle inputRect = { (float)inputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool inputHover = CheckCollisionPointRec(mouse, inputRect);
        DrawUiSlot(inputX, fuSlotY, fuSlotSize, inputHover, false, 1.0f);
        DrawGameText(S(STR_INPUT), inputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_INPUT), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Output slot
        Rectangle outputRect = { (float)outputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool outputHover = CheckCollisionPointRec(mouse, outputRect);
        DrawUiSlot(outputX, fuSlotY, fuSlotSize, outputHover, false, 1.0f);
        DrawGameText(S(STR_OUTPUT), outputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_OUTPUT), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Progress arrow between input and output
        int arrowX = inputX + fuSlotSize + 6;
        int arrowW = outputX - inputX - fuSlotSize - 12;
        int arrowY = fuSlotY + fuSlotSize / 2 - 4;
        DrawRectangle(arrowX, arrowY, arrowW, 8, (Color){36, 36, 36, 200});
        if (brewProgress > 0.0f) {
            DrawRectangle(arrowX, arrowY, (int)(arrowW * brewProgress), 8, (Color){200, 200, 200, 255});
        }
        DrawTriangle(
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY - 4)},
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY + 12)},
            (Vector2){(float)(arrowX + arrowW + 8), (float)(arrowY + 4)},
            (Color){200, 200, 200, 255});

        // Draw items in furnace slots
        if (brewBottle != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(brewBottle * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(fuelX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (brewBottleCount > 1) {
                DrawGameText(TextFormat("%d", brewBottleCount), fuelX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", brewBottleCount), fuelX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (brewIngredient != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(brewIngredient * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(inputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (brewIngredientCount > 1) {
                DrawGameText(TextFormat("%d", brewIngredientCount), inputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", brewIngredientCount), inputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (brewOutput != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(brewOutput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(outputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (brewOutputCount > 1) {
                DrawGameText(TextFormat("%d", brewOutputCount), outputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", brewOutputCount), outputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }

        // Brewing info
        if (brewIngredient != BLOCK_AIR) {
            BlockType bout = FindBrewOutput((BlockType)brewIngredient);
            if (bout != BLOCK_AIR) {
                DrawGameText(Sf(STR_MSG_BREWING, GetBlockName(bout)), contX + panelPad, contY + brewH - 18, 10, (Color){150, 145, 160, 200});
            } else {
                DrawGameText(S(STR_MSG_CANNOT_SMELT), contX + panelPad, contY + brewH - 18, 14, (Color){232, 87, 92, 200});
            }
        }

        // --- Divider ---
        int divY = contY + brewH;
        DrawRectangle(contX + 8, divY, contW - 16, dividerH, (Color){90, 90, 90, 200});

        // --- Inventory section ---
        int invX = contX + panelPad + previewW + previewPad + armorColW;
        int invY = divY + dividerH + 4;
        DrawGameText(S(STR_INVENTORY), invX, invY, 14, (Color){62, 62, 62, 255});
        invY += 20;

        // Player preview (compact in furnace view)
        {
            int prevX = contX + panelPad;
            int prevY = invY;
            int prevH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
            DrawUiBox(prevX, prevY, previewW, prevH, 0.06f, (Color){36, 36, 36, 220});
            DrawRectangleLines(prevX, prevY, previewW, prevH, (Color){90, 90, 90, 180});
            int sc = 2;
            int charW = 12 * sc, charH = 28 * sc;
            int cx = prevX + (previewW - charW) / 2;
            int cy = prevY + (prevH - charH) / 2;
            Color skin = {220,180,140,255}, hair = {80,50,30,255}, shirt = {0,100,200,255}, pants_ = {60,40,20,255};
            Color hc={0,0,0,0},cc={0,0,0,0},lc={0,0,0,0},bc={0,0,0,0};
            for (int i = 0; i < 4; i++) { if (player.armor[i] != BLOCK_AIR) { BlockType a=(BlockType)player.armor[i]; Color ac; if(a>=ARMOR_DIAMOND_HELMET) ac=(Color){80,220,230,255}; else if(a>=ARMOR_GOLD_HELMET) ac=(Color){220,180,50,255}; else if(a>=ARMOR_IRON_HELMET) ac=(Color){200,210,220,255}; else if(a>=ARMOR_STONE_HELMET) ac=(Color){140,140,140,255}; else if(a>=ARMOR_WOOD_HELMET) ac=(Color){160,120,60,255}; else ac=(Color){160,120,60,255}; if(i==0)hc=ac;else if(i==1)cc=ac;else if(i==2)lc=ac;else bc=ac; } }
            DrawRectangle(cx+2*sc,cy,8*sc,8*sc,skin); DrawRectangle(cx+2*sc,cy,8*sc,3*sc,hair);
            if(hc.a>0){DrawRectangle(cx+1*sc,cy-1*sc,10*sc,5*sc,hc);DrawRectangle(cx+2*sc,cy+4*sc,8*sc,2*sc,hc);}
            DrawRectangle(cx+3*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255}); DrawRectangle(cx+7*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255});
            DrawRectangle(cx+1*sc,cy+8*sc,10*sc,10*sc,shirt);
            if(cc.a>0){DrawRectangle(cx,cy+7*sc,12*sc,11*sc,cc);DrawRectangle(cx+1*sc,cy+8*sc,10*sc,9*sc,(Color){(unsigned char)(cc.r*0.8f),(unsigned char)(cc.g*0.8f),(unsigned char)(cc.b*0.8f),255});}
            DrawRectangle(cx-2*sc,cy+8*sc,3*sc,10*sc,skin); DrawRectangle(cx+11*sc,cy+8*sc,3*sc,10*sc,skin);
            DrawRectangle(cx+1*sc,cy+18*sc,4*sc,10*sc,pants_); DrawRectangle(cx+7*sc,cy+18*sc,4*sc,10*sc,pants_);
            if(lc.a>0){DrawRectangle(cx,cy+18*sc,5*sc,10*sc,lc);DrawRectangle(cx+6*sc,cy+18*sc,5*sc,10*sc,lc);}
            if(bc.a>0){DrawRectangle(cx,cy+25*sc,5*sc,3*sc,bc);DrawRectangle(cx+6*sc,cy+25*sc,5*sc,3*sc,bc);}
            if(dayNight.lightLevel<1.0f){unsigned char a=(unsigned char)(120*(1.0f-dayNight.lightLevel));DrawRectangle(cx-2*sc,cy-1*sc,16*sc,29*sc,(Color){5,5,25,a});}
        }

        // Armor slots (compact)
        {
            const char *armorLabels[] = {"H", "C", "L", "B"};
            int armorX = contX + panelPad + previewW + previewPad;
            int armorY = invY;
            for (int i = 0; i < 4; i++) {
                int ay = armorY + i * (armorSlotSize + armorPad);
                Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                DrawUiSlot(armorX, ay, armorSlotSize, hover, false, 1.0f);
                if (player.armor[i] != BLOCK_AIR && blockAtlas.id > 0) {
                    int item = player.armor[i];
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(armorX + 4), (float)(ay + 4), (float)(armorSlotSize - 8), (float)(armorSlotSize - 8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
                    if (ENCH_TYPE(player.armorEnchantments[i]) != ENCH_NONE)
                        DrawEnchantGlint(armorX + 4, ay + 4, armorSlotSize - 8);
                } else {
                    DrawGameText(armorLabels[i], armorX + armorSlotSize / 2 - 4, ay + armorSlotSize / 2 - 6, 14, (Color){62, 62, 62, 150});
                }
                // Armor click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (heldItem == BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                        heldItem = player.armor[i]; heldCount = 1; heldDurability = player.armorDurability[i]; heldItemEnchant = player.armorEnchantments[i];
                        player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; player.armorEnchantments[i] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.armor[i] == BLOCK_AIR && IsArmor((BlockType)heldItem)) {
                        int slotType = GetArmorSlot((BlockType)heldItem);
                        if (slotType == i) { player.armor[i] = heldItem; player.armorDurability[i] = heldDurability; player.armorEnchantments[i] = heldItemEnchant; ClearHeldItem(); PlaySoundUIClick(); }
                    }
                }
            }
        }

        // Inventory grid
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int idx = row * INVENTORY_COLS + col;
                int x = invX + col * (slotSize + padding);
                int y = invY + row * (slotSize + padding);
                Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                bool selected = (row == 0 && col == player.selectedSlot);
                DrawUiSlot(x, y, slotSize, hover, selected, 1.0f);

                int item = player.inventory[idx];
                if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(x+4), (float)(y+4), (float)(slotSize-8), (float)(slotSize-8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
                    if (ENCH_TYPE(player.itemEnchantments[idx]) != ENCH_NONE)
                        DrawEnchantGlint(x+4, y+4, slotSize-8);
                    if (player.inventoryCount[idx] > 1) {
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-19, y+slotSize-13, 14, (Color){0,0,0,150});
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-20, y+slotSize-14, 13, WHITE);
                    }
                    if (IsTool((BlockType)item)) {
                        int maxDur = GetToolMaxDurability((BlockType)item);
                        if (maxDur > 0) {
                            float pct = (float)player.toolDurability[idx] / maxDur;
                            int barW = slotSize-8, barX = x+4, barY = y+slotSize-5;
                            Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                            DrawRectangle(barX,barY,barW,3,(Color){0,0,0,180});
                            DrawRectangle(barX,barY,(int)(barW*pct),3,barColor);
                        }
                    }
                }
                if (row == 0) DrawGameText(TextFormat("%d",col+1), x+2, y+1, 14, (Color){170,170,170,120});

                // Click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                        if (player.inventory[idx] != BLOCK_AIR) {
                            bool transferred = false;
                            uint8_t itm = player.inventory[idx]; int cnt = player.inventoryCount[idx];
                            if (itm == ITEM_POTION_WATER) {
                                if (brewBottle == BLOCK_AIR) { brewBottle = itm; brewBottleCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (brewBottle == itm) { int sp = 64 - brewBottleCount; int ta = cnt > sp ? sp : cnt; brewBottleCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            } else if (FindBrewOutput((BlockType)itm) != BLOCK_AIR) {
                                if (brewIngredient == BLOCK_AIR) { brewIngredient = itm; brewIngredientCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (brewIngredient == itm) { int sp = 64 - brewIngredientCount; int ta = cnt > sp ? sp : cnt; brewIngredientCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            }
                            if (!transferred) {
                                // Auto-equip armor when shift-clicking
                                if (IsArmor((BlockType)player.inventory[idx])) {
                                    int slot = GetArmorSlot((BlockType)player.inventory[idx]);
                                    if (slot >= 0 && player.armor[slot] == BLOCK_AIR) {
                                        player.armor[slot] = player.inventory[idx];
                                        player.armorDurability[slot] = player.toolDurability[idx];
                                        player.armorEnchantments[slot] = player.itemEnchantments[idx];
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                        player.toolDurability[idx] = 0;
                                        player.itemEnchantments[idx] = 0;
                                        transferred = true;
                                        PlaySoundUIClick();
                                    }
                                }
                            }
                            if (!transferred) {
                                int startDest = (row == 0) ? HOTBAR_SLOTS : 0;
                                int endDest = (row == 0) ? INVENTORY_SLOTS : HOTBAR_SLOTS;
                                bool moved = false;
                                for (int d = startDest; d < endDest; d++) {
                                    if (player.inventory[d] == player.inventory[idx] && player.inventoryCount[d] < 64) {
                                        int sp = 64 - player.inventoryCount[d]; int ta = player.inventoryCount[idx] > sp ? sp : player.inventoryCount[idx];
                                        player.inventoryCount[d] += ta; player.inventoryCount[idx] -= ta;
                                        if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; }
                                        moved = true; break;
                                    }
                                }
                                if (!moved) { for (int d = startDest; d < endDest; d++) { if (player.inventory[d] == BLOCK_AIR) { player.inventory[d] = player.inventory[idx]; player.inventoryCount[d] = player.inventoryCount[idx]; player.toolDurability[d] = player.toolDurability[idx]; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; break; } } }
                            }
                            PlaySoundUIClick();
                        }
                    } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        heldItem = player.inventory[idx]; heldCount = player.inventoryCount[idx]; heldDurability = player.toolDurability[idx]; heldItemEnchant = player.itemEnchantments[idx];
                        player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; player.itemEnchantments[idx] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability; player.itemEnchantments[idx] = heldItemEnchant;
                        ClearHeldItem(); PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                        int sp = 64 - player.inventoryCount[idx]; int ta = heldCount > sp ? sp : heldCount;
                        player.inventoryCount[idx] += ta; heldCount -= ta;
                        if (heldCount <= 0) { ClearHeldItem(); } PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        uint8_t ti = player.inventory[idx]; int tc = player.inventoryCount[idx]; int td = player.toolDurability[idx]; uint16_t te = player.itemEnchantments[idx];
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability; player.itemEnchantments[idx] = heldItemEnchant;
                        heldItem = ti; heldCount = tc; heldDurability = td; heldItemEnchant = te; PlaySoundUIClick();
                    }
                }
                if (hover && Win32IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                    PlaySoundDrop(); player.inventoryCount[idx]--;
                    if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; player.itemEnchantments[idx] = 0; }
                }
            }
        }

        // Furnace slot click handling
        if (CheckCollisionPointRec(mouse, fuelRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && brewBottle != BLOCK_AIR) {
                heldItem = brewBottle; heldCount = brewBottleCount; heldDurability = 0;
                brewBottle = BLOCK_AIR; brewBottleCount = 0;
            } else if (heldItem != BLOCK_AIR && brewBottle == BLOCK_AIR && heldItem == ITEM_POTION_WATER) {
                brewBottle = heldItem; brewBottleCount = heldCount;
                ClearHeldItem();
            } else if (heldItem != BLOCK_AIR && brewBottle == heldItem) {
                int sp = 64 - brewBottleCount; int ta = heldCount > sp ? sp : heldCount;
                brewBottleCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, inputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && brewIngredient != BLOCK_AIR) {
                heldItem = brewIngredient; heldCount = brewIngredientCount; heldDurability = 0;
                brewIngredient = BLOCK_AIR; brewIngredientCount = 0; brewProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && brewIngredient == BLOCK_AIR && FindBrewOutput((BlockType)heldItem) != BLOCK_AIR) {
                brewIngredient = heldItem; brewIngredientCount = heldCount;
                ClearHeldItem(); brewProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && brewIngredient == heldItem) {
                int sp = 64 - brewIngredientCount; int ta = heldCount > sp ? sp : heldCount;
                brewIngredientCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, outputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && brewOutput != BLOCK_AIR) {
                heldItem = brewOutput; heldCount = brewOutputCount; heldDurability = 0;
                brewOutput = BLOCK_AIR; brewOutputCount = 0;
            } else if (heldItem == brewOutput && brewOutput != BLOCK_AIR) {
                int sp = 64 - heldCount; int ta = brewOutputCount > sp ? sp : brewOutputCount;
                heldCount += ta; brewOutputCount -= ta;
                if (brewOutputCount <= 0) { brewOutput = BLOCK_AIR; brewOutputCount = 0; }
            }
        }

        // Drop held item outside container
        if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle container = { (float)contX, (float)contY, (float)contW, (float)contH };
            if (!CheckCollisionPointRec(mouse, container)) {
                PlaySoundDrop();
                SpawnItemEntity(heldItem, heldCount, player.position.x + PLAYER_WIDTH / 2, player.position.y);
                ClearHeldItem();
            }
        }

        // Tooltip for inventory slots
        if (heldItem == BLOCK_AIR) {
            for (int row = 0; row < INVENTORY_ROWS; row++) {
                for (int col = 0; col < INVENTORY_COLS; col++) {
                    int idx = row * INVENTORY_COLS + col;
                    int x = invX + col * (slotSize + padding);
                    int y = invY + row * (slotSize + padding);
                    Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                    if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                        BlockType bt = (BlockType)player.inventory[idx];
                        const char *name = GetBlockName(bt);
                        int tw = MeasureGameTextWidth(name, 14);
                        const char *typeLabel = NULL; Color typeColor = {180,180,190,200};
                        if (IsTool(bt)) { typeLabel = S(STR_TYPE_TOOL); typeColor = (Color){100,160,220,255}; }
                        else if (IsArmor(bt)) { typeLabel = S(STR_TYPE_ARMOR); typeColor = (Color){180,100,220,255}; }
                        else if (IsFood(bt)) { typeLabel = S(STR_TYPE_FOOD); typeColor = (Color){100,200,100,255}; }
                        else { typeLabel = S(STR_TYPE_BLOCK); typeColor = (Color){180,175,190,200}; }
                        int typeW = MeasureGameTextWidth(typeLabel, 11); int maxW = tw > typeW ? tw : typeW;
                        char info[64] = {0};
                        if (IsFood(bt)) snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                        else if (IsArmor(bt)) { int av = GetArmorValue(bt); int md = GetArmorMaxDurability(bt); int pct = md > 0 ? player.toolDurability[idx]*100/md : 0; snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), av, pct); }
                        else if (IsTool(bt)) { int md = GetToolMaxDurability(bt); if (md > 0) { int pct = player.toolDurability[idx]*100/md; snprintf(info, sizeof(info), S(STR_TOOLTIP_DURABILITY), pct); } }
                        int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0; if (infoW > maxW) maxW = infoW;
                        int ttH = 36 + (info[0] ? 16 : 0);
                        int boxW = maxW + 10, boxH = ttH + 2;
                        int tx, ty;
                        GetUiTooltipPos((int)mouse.x, (int)mouse.y, boxW, boxH, &tx, &ty);
                        DrawRectangle(tx+1, ty+1, boxW, boxH, (Color){0,0,0,60});
                        DrawRectangle(tx, ty, boxW, boxH, (Color){55,55,55,240});
                        DrawRectangleLines(tx, ty, boxW, boxH, (Color){90,90,90,220});
                        DrawGameText(typeLabel, tx+4, ty+2, 11, typeColor);
                        DrawGameText(name, tx+4, ty+16, 14, (Color){230,225,240,255});
                        if (info[0]) DrawGameText(info, tx+4, ty+32, 14, (Color){180,200,180,255});
                    }
                }
            }
        }

        // Close hint
        DrawGameText(S(STR_PRESS_E_ESC_CLOSE),
            contX + contW / 2 - MeasureGameTextWidth(S(STR_PRESS_E_ESC_CLOSE), 10) / 2,
            contY + contH - 14, 10, (Color){62, 62, 62, 180});

        // Held item follows mouse
        if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
            int mx = (int)mouse.x - slotSize / 2; int my = (int)mouse.y - slotSize / 2;
            Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(mx+4), (float)(my+4), (float)(slotSize-8), (float)(slotSize-8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
            if (heldCount > 1) DrawGameText(TextFormat("%d", heldCount), mx+slotSize-20, my+slotSize-16, 14, WHITE);
        }

        // Sync slot edits back to the stand array (per-machine data)
        if (activeBrewing >= 0 && activeBrewing < brewingCount) {
            SyncActiveToBrewing(activeBrewing);
        }
        // If inventory/armor changed inside the furnace UI, sync that too.
        if (doInvSync) {
            if (memcmp(snapInv,   player.inventory,        sizeof(snapInv))   != 0 ||
                memcmp(snapCount, player.inventoryCount,   sizeof(snapCount)) != 0 ||
                memcmp(snapDur,   player.toolDurability,   sizeof(snapDur))   != 0 ||
                memcmp(snapEnch,  player.itemEnchantments, sizeof(snapEnch))  != 0 ||
                memcmp(snapArmor, player.armor,            sizeof(snapArmor)) != 0 ||
                memcmp(snapArmorDur,  player.armorDurability,  sizeof(snapArmorDur))  != 0 ||
                memcmp(snapArmorEnch, player.armorEnchantments, sizeof(snapArmorEnch)) != 0) {
                if (NetIsClient()) SyncInventoryToHost();
                else if (NetIsHost()) SyncInventoryToAll();
            }
        }

        return;
    }



    if (furnaceOpen) {
        // Snapshot active furnace state at the start of the frame so we can detect
        // modifications and sync them to the host in multiplayer.
        uint8_t snapFuel = furnaceFuel; int snapFuelCount = furnaceFuelCount;
        uint8_t snapInput = furnaceInput; int snapInputCount = furnaceInputCount;
        float snapProgress = furnaceProgress;
        float snapFuelBurn = furnaceFuelBurn;
        float snapFuelBurnMax = furnaceFuelBurnMax;

        int padding = 3;
        int armorSlotSize = 40;
        int armorPad = 3;
        int armorColW = armorSlotSize + armorPad;
        int previewW = 60;
        int previewPad = 6;
        int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;

        int contW = 14 + previewW + previewPad + armorColW + gridW + 14;
        int furnaceH = 175;
        int dividerH = 2;
        int invTitleH = 20;
        int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
        int contH = furnaceH + dividerH + invTitleH + gridH + 14;

        int contX = (SCREEN_WIDTH - contW) / 2;
        int contY = (SCREEN_HEIGHT - contH) / 2;
        contY += (int)((1.0f - panelAnim) * 40.0f);
        int panelPad = 14;

        Vector2 mouse = Win32GetMousePosition();

        // Light overlay
        unsigned char furnaceOA = (unsigned char)(100 * panelAnim);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, furnaceOA});

        // Container
        DrawUiPanel(contX, contY, contW, contH, 255);

        // --- Furnace section ---
        int fuSlotSize = 40;
        int fuY = contY + panelPad;
        DrawGameText(S(STR_FURNACE), contX + contW / 2 - MeasureGameTextWidth(S(STR_FURNACE), 16) / 2, fuY, 16, (Color){62, 62, 62, 255});
        fuY += 28;

        int fuSlotY = fuY;
        int fuCenterX = contX + contW / 2;
        int fuelX = fuCenterX - 120;
        int inputX = fuCenterX - fuSlotSize / 2;
        int outputX = fuCenterX + 80;

        // Fuel slot
        Rectangle fuelRect = { (float)fuelX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool fuelHover = CheckCollisionPointRec(mouse, fuelRect);
        DrawUiSlot(fuelX, fuSlotY, fuSlotSize, fuelHover, false, 1.0f);
        DrawGameText(S(STR_FUEL), fuelX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_FUEL), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Input slot
        Rectangle inputRect = { (float)inputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool inputHover = CheckCollisionPointRec(mouse, inputRect);
        DrawUiSlot(inputX, fuSlotY, fuSlotSize, inputHover, false, 1.0f);
        DrawGameText(S(STR_INPUT), inputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_INPUT), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Output slot
        Rectangle outputRect = { (float)outputX, (float)fuSlotY, (float)fuSlotSize, (float)fuSlotSize };
        bool outputHover = CheckCollisionPointRec(mouse, outputRect);
        DrawUiSlot(outputX, fuSlotY, fuSlotSize, outputHover, false, 1.0f);
        DrawGameText(S(STR_OUTPUT), outputX + fuSlotSize / 2 - MeasureGameTextWidth(S(STR_OUTPUT), 10) / 2, fuSlotY - 13, 10, (Color){62, 62, 62, 200});

        // Progress arrow between input and output
        int arrowX = inputX + fuSlotSize + 6;
        int arrowW = outputX - inputX - fuSlotSize - 12;
        int arrowY = fuSlotY + fuSlotSize / 2 - 4;
        DrawRectangle(arrowX, arrowY, arrowW, 8, (Color){36, 36, 36, 200});
        if (furnaceProgress > 0.0f) {
            DrawRectangle(arrowX, arrowY, (int)(arrowW * furnaceProgress), 8, (Color){200, 200, 200, 255});
        }
        DrawTriangle(
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY - 4)},
            (Vector2){(float)(arrowX + arrowW), (float)(arrowY + 12)},
            (Vector2){(float)(arrowX + arrowW + 8), (float)(arrowY + 4)},
            (Color){200, 200, 200, 255});

        // Draw items in furnace slots
        if (furnaceFuel != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceFuel * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(fuelX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceFuelCount > 1) {
                DrawGameText(TextFormat("%d", furnaceFuelCount), fuelX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceFuelCount), fuelX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (furnaceInput != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceInput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(inputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceInputCount > 1) {
                DrawGameText(TextFormat("%d", furnaceInputCount), inputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceInputCount), inputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }
        if (furnaceOutput != BLOCK_AIR && blockAtlas.id > 0) {
            Rectangle src = { (float)(furnaceOutput * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(outputX + 4), (float)(fuSlotY + 4), (float)(fuSlotSize - 8), (float)(fuSlotSize - 8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (furnaceOutputCount > 1) {
                DrawGameText(TextFormat("%d", furnaceOutputCount), outputX + fuSlotSize - 19, fuSlotY + fuSlotSize - 13, 14, (Color){0, 0, 0, 150});
                DrawGameText(TextFormat("%d", furnaceOutputCount), outputX + fuSlotSize - 20, fuSlotY + fuSlotSize - 14, 13, WHITE);
            }
        }

        // Fuel bar
        int flameY = fuSlotY + fuSlotSize + 8;
        DrawGameText(TextFormat("%s:", S(STR_FUEL)), fuelX, flameY, 10, (Color){62, 62, 62, 200});
        if (furnaceFuelBurn > 0.0f) {
            int flameW = 60;
            float fuelPct = furnaceFuelBurnMax > 0 ? furnaceFuelBurn / furnaceFuelBurnMax : 0;
            DrawRectangle(fuelX, flameY + 14, flameW, 8, (Color){36, 36, 36, 200});
            DrawRectangle(fuelX, flameY + 14, (int)(flameW * fuelPct), 8, (Color){200, 200, 200, 255});
        } else {
            DrawGameText(S(STR_MSG_NO_FUEL), fuelX, flameY + 14, 14, (Color){150, 80, 80, 200});
        }

        // Smelting info
        if (furnaceInput != BLOCK_AIR) {
            int ri = FindSmeltRecipe((BlockType)furnaceInput);
            if (ri >= 0) {
                DrawGameText(Sf(STR_MSG_SMELTING, S(smeltRecipes[ri].nameId)), contX + panelPad, contY + furnaceH - 18, 10, (Color){150, 145, 160, 200});
            } else {
                DrawGameText(S(STR_MSG_CANNOT_SMELT), contX + panelPad, contY + furnaceH - 18, 14, (Color){232, 87, 92, 200});
            }
        }

        // --- Divider ---
        int divY = contY + furnaceH;
        DrawRectangle(contX + 8, divY, contW - 16, dividerH, (Color){90, 90, 90, 200});

        // --- Inventory section ---
        int invX = contX + panelPad + previewW + previewPad + armorColW;
        int invY = divY + dividerH + 4;
        DrawGameText(S(STR_INVENTORY), invX, invY, 14, (Color){62, 62, 62, 255});
        invY += 20;

        // Player preview (compact in furnace view)
        {
            int prevX = contX + panelPad;
            int prevY = invY;
            int prevH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;
            DrawUiBox(prevX, prevY, previewW, prevH, 0.06f, (Color){36, 36, 36, 220});
            DrawRectangleLines(prevX, prevY, previewW, prevH, (Color){90, 90, 90, 180});
            int sc = 2;
            int charW = 12 * sc, charH = 28 * sc;
            int cx = prevX + (previewW - charW) / 2;
            int cy = prevY + (prevH - charH) / 2;
            Color skin = {220,180,140,255}, hair = {80,50,30,255}, shirt = {0,100,200,255}, pants_ = {60,40,20,255};
            Color hc={0,0,0,0},cc={0,0,0,0},lc={0,0,0,0},bc={0,0,0,0};
            for (int i = 0; i < 4; i++) { if (player.armor[i] != BLOCK_AIR) { BlockType a=(BlockType)player.armor[i]; Color ac; if(a>=ARMOR_DIAMOND_HELMET) ac=(Color){80,220,230,255}; else if(a>=ARMOR_GOLD_HELMET) ac=(Color){220,180,50,255}; else if(a>=ARMOR_IRON_HELMET) ac=(Color){200,210,220,255}; else if(a>=ARMOR_STONE_HELMET) ac=(Color){140,140,140,255}; else if(a>=ARMOR_WOOD_HELMET) ac=(Color){160,120,60,255}; else ac=(Color){160,120,60,255}; if(i==0)hc=ac;else if(i==1)cc=ac;else if(i==2)lc=ac;else bc=ac; } }
            DrawRectangle(cx+2*sc,cy,8*sc,8*sc,skin); DrawRectangle(cx+2*sc,cy,8*sc,3*sc,hair);
            if(hc.a>0){DrawRectangle(cx+1*sc,cy-1*sc,10*sc,5*sc,hc);DrawRectangle(cx+2*sc,cy+4*sc,8*sc,2*sc,hc);}
            DrawRectangle(cx+3*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255}); DrawRectangle(cx+7*sc,cy+4*sc,2*sc,2*sc,(Color){40,40,40,255});
            DrawRectangle(cx+1*sc,cy+8*sc,10*sc,10*sc,shirt);
            if(cc.a>0){DrawRectangle(cx,cy+7*sc,12*sc,11*sc,cc);DrawRectangle(cx+1*sc,cy+8*sc,10*sc,9*sc,(Color){(unsigned char)(cc.r*0.8f),(unsigned char)(cc.g*0.8f),(unsigned char)(cc.b*0.8f),255});}
            DrawRectangle(cx-2*sc,cy+8*sc,3*sc,10*sc,skin); DrawRectangle(cx+11*sc,cy+8*sc,3*sc,10*sc,skin);
            DrawRectangle(cx+1*sc,cy+18*sc,4*sc,10*sc,pants_); DrawRectangle(cx+7*sc,cy+18*sc,4*sc,10*sc,pants_);
            if(lc.a>0){DrawRectangle(cx,cy+18*sc,5*sc,10*sc,lc);DrawRectangle(cx+6*sc,cy+18*sc,5*sc,10*sc,lc);}
            if(bc.a>0){DrawRectangle(cx,cy+25*sc,5*sc,3*sc,bc);DrawRectangle(cx+6*sc,cy+25*sc,5*sc,3*sc,bc);}
            if(dayNight.lightLevel<1.0f){unsigned char a=(unsigned char)(120*(1.0f-dayNight.lightLevel));DrawRectangle(cx-2*sc,cy-1*sc,16*sc,29*sc,(Color){5,5,25,a});}
        }

        // Armor slots (compact)
        {
            const char *armorLabels[] = {"H", "C", "L", "B"};
            int armorX = contX + panelPad + previewW + previewPad;
            int armorY = invY;
            for (int i = 0; i < 4; i++) {
                int ay = armorY + i * (armorSlotSize + armorPad);
                Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                DrawUiSlot(armorX, ay, armorSlotSize, hover, false, 1.0f);
                if (player.armor[i] != BLOCK_AIR && blockAtlas.id > 0) {
                    int item = player.armor[i];
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(armorX + 4), (float)(ay + 4), (float)(armorSlotSize - 8), (float)(armorSlotSize - 8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
                    if (ENCH_TYPE(player.armorEnchantments[i]) != ENCH_NONE)
                        DrawEnchantGlint(armorX + 4, ay + 4, armorSlotSize - 8);
                } else {
                    DrawGameText(armorLabels[i], armorX + armorSlotSize / 2 - 4, ay + armorSlotSize / 2 - 6, 14, (Color){62, 62, 62, 150});
                }
                // Armor click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (heldItem == BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                        heldItem = player.armor[i]; heldCount = 1; heldDurability = player.armorDurability[i]; heldItemEnchant = player.armorEnchantments[i];
                        player.armor[i] = BLOCK_AIR; player.armorDurability[i] = 0; player.armorEnchantments[i] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.armor[i] == BLOCK_AIR && IsArmor((BlockType)heldItem)) {
                        int slotType = GetArmorSlot((BlockType)heldItem);
                        if (slotType == i) { player.armor[i] = heldItem; player.armorDurability[i] = heldDurability; player.armorEnchantments[i] = heldItemEnchant; ClearHeldItem(); PlaySoundUIClick(); }
                    }
                }
            }
        }

        // Inventory grid
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int idx = row * INVENTORY_COLS + col;
                int x = invX + col * (slotSize + padding);
                int y = invY + row * (slotSize + padding);
                Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                bool selected = (row == 0 && col == player.selectedSlot);
                DrawUiSlot(x, y, slotSize, hover, selected, 1.0f);

                int item = player.inventory[idx];
                if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    Rectangle dst = { (float)(x+4), (float)(y+4), (float)(slotSize-8), (float)(slotSize-8) };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
                    if (ENCH_TYPE(player.itemEnchantments[idx]) != ENCH_NONE)
                        DrawEnchantGlint(x+4, y+4, slotSize-8);
                    if (player.inventoryCount[idx] > 1) {
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-19, y+slotSize-13, 14, (Color){0,0,0,150});
                        DrawGameText(TextFormat("%d",player.inventoryCount[idx]), x+slotSize-20, y+slotSize-14, 13, WHITE);
                    }
                    if (IsTool((BlockType)item)) {
                        int maxDur = GetToolMaxDurability((BlockType)item);
                        if (maxDur > 0) {
                            float pct = (float)player.toolDurability[idx] / maxDur;
                            int barW = slotSize-8, barX = x+4, barY = y+slotSize-5;
                            Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                            DrawRectangle(barX,barY,barW,3,(Color){0,0,0,180});
                            DrawRectangle(barX,barY,(int)(barW*pct),3,barColor);
                        }
                    }
                }
                if (row == 0) DrawGameText(TextFormat("%d",col+1), x+2, y+1, 14, (Color){170,170,170,120});

                // Click handling
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                        if (player.inventory[idx] != BLOCK_AIR) {
                            bool transferred = false;
                            uint8_t itm = player.inventory[idx]; int cnt = player.inventoryCount[idx];
                            if (GetFuelBurnTime(itm) > 0.0f) {
                                if (furnaceFuel == BLOCK_AIR) { furnaceFuel = itm; furnaceFuelCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (furnaceFuel == itm) { int sp = 64 - furnaceFuelCount; int ta = cnt > sp ? sp : cnt; furnaceFuelCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            } else if (FindSmeltRecipe((BlockType)itm) >= 0) {
                                if (furnaceInput == BLOCK_AIR) { furnaceInput = itm; furnaceInputCount = cnt; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; transferred = true; }
                                else if (furnaceInput == itm) { int sp = 64 - furnaceInputCount; int ta = cnt > sp ? sp : cnt; furnaceInputCount += ta; player.inventoryCount[idx] -= ta; if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; } transferred = true; }
                            }
                            if (!transferred) {
                                // Auto-equip armor when shift-clicking
                                if (IsArmor((BlockType)player.inventory[idx])) {
                                    int slot = GetArmorSlot((BlockType)player.inventory[idx]);
                                    if (slot >= 0 && player.armor[slot] == BLOCK_AIR) {
                                        player.armor[slot] = player.inventory[idx];
                                        player.armorDurability[slot] = player.toolDurability[idx];
                                        player.armorEnchantments[slot] = player.itemEnchantments[idx];
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                        player.toolDurability[idx] = 0;
                                        player.itemEnchantments[idx] = 0;
                                        transferred = true;
                                        PlaySoundUIClick();
                                    }
                                }
                            }
                            if (!transferred) {
                                int startDest = (row == 0) ? HOTBAR_SLOTS : 0;
                                int endDest = (row == 0) ? INVENTORY_SLOTS : HOTBAR_SLOTS;
                                bool moved = false;
                                for (int d = startDest; d < endDest; d++) {
                                    if (player.inventory[d] == player.inventory[idx] && player.inventoryCount[d] < 64) {
                                        int sp = 64 - player.inventoryCount[d]; int ta = player.inventoryCount[idx] > sp ? sp : player.inventoryCount[idx];
                                        player.inventoryCount[d] += ta; player.inventoryCount[idx] -= ta;
                                        if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; }
                                        moved = true; break;
                                    }
                                }
                                if (!moved) { for (int d = startDest; d < endDest; d++) { if (player.inventory[d] == BLOCK_AIR) { player.inventory[d] = player.inventory[idx]; player.inventoryCount[d] = player.inventoryCount[idx]; player.toolDurability[d] = player.toolDurability[idx]; player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; break; } } }
                            }
                            PlaySoundUIClick();
                        }
                    } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        heldItem = player.inventory[idx]; heldCount = player.inventoryCount[idx]; heldDurability = player.toolDurability[idx]; heldItemEnchant = player.itemEnchantments[idx];
                        player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; player.itemEnchantments[idx] = 0; PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability; player.itemEnchantments[idx] = heldItemEnchant;
                        ClearHeldItem(); PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                        int sp = 64 - player.inventoryCount[idx]; int ta = heldCount > sp ? sp : heldCount;
                        player.inventoryCount[idx] += ta; heldCount -= ta;
                        if (heldCount <= 0) { ClearHeldItem(); } PlaySoundUIClick();
                    } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                        uint8_t ti = player.inventory[idx]; int tc = player.inventoryCount[idx]; int td = player.toolDurability[idx]; uint16_t te = player.itemEnchantments[idx];
                        player.inventory[idx] = heldItem; player.inventoryCount[idx] = heldCount; player.toolDurability[idx] = heldDurability; player.itemEnchantments[idx] = heldItemEnchant;
                        heldItem = ti; heldCount = tc; heldDurability = td; heldItemEnchant = te; PlaySoundUIClick();
                    }
                }
                if (hover && Win32IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                    PlaySoundDrop(); player.inventoryCount[idx]--;
                    if (player.inventoryCount[idx] <= 0) { player.inventory[idx] = BLOCK_AIR; player.inventoryCount[idx] = 0; player.toolDurability[idx] = 0; player.itemEnchantments[idx] = 0; }
                }
            }
        }

        // Furnace slot click handling
        if (CheckCollisionPointRec(mouse, fuelRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceFuel != BLOCK_AIR) {
                heldItem = furnaceFuel; heldCount = furnaceFuelCount; heldDurability = 0;
                furnaceFuel = BLOCK_AIR; furnaceFuelCount = 0;
            } else if (heldItem != BLOCK_AIR && furnaceFuel == BLOCK_AIR && GetFuelBurnTime(heldItem) > 0.0f) {
                furnaceFuel = heldItem; furnaceFuelCount = heldCount;
                ClearHeldItem();
            } else if (heldItem != BLOCK_AIR && furnaceFuel == heldItem && GetFuelBurnTime(heldItem) > 0.0f) {
                int sp = 64 - furnaceFuelCount; int ta = heldCount > sp ? sp : heldCount;
                furnaceFuelCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, inputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceInput != BLOCK_AIR) {
                heldItem = furnaceInput; heldCount = furnaceInputCount; heldDurability = 0;
                furnaceInput = BLOCK_AIR; furnaceInputCount = 0; furnaceProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && furnaceInput == BLOCK_AIR && FindSmeltRecipe((BlockType)heldItem) >= 0) {
                furnaceInput = heldItem; furnaceInputCount = heldCount;
                ClearHeldItem(); furnaceProgress = 0.0f;
            } else if (heldItem != BLOCK_AIR && furnaceInput == heldItem) {
                int sp = 64 - furnaceInputCount; int ta = heldCount > sp ? sp : heldCount;
                furnaceInputCount += ta; heldCount -= ta;
                if (heldCount <= 0) { heldItem = BLOCK_AIR; heldCount = 0; }
            }
        }
        if (CheckCollisionPointRec(mouse, outputRect) && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (heldItem == BLOCK_AIR && furnaceOutput != BLOCK_AIR) {
                heldItem = furnaceOutput; heldCount = furnaceOutputCount; heldDurability = 0;
                furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0;
            } else if (heldItem == furnaceOutput && furnaceOutput != BLOCK_AIR) {
                int sp = 64 - heldCount; int ta = furnaceOutputCount > sp ? sp : furnaceOutputCount;
                heldCount += ta; furnaceOutputCount -= ta;
                if (furnaceOutputCount <= 0) { furnaceOutput = BLOCK_AIR; furnaceOutputCount = 0; }
            }
        }

        // Drop held item outside container
        if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle container = { (float)contX, (float)contY, (float)contW, (float)contH };
            if (!CheckCollisionPointRec(mouse, container)) {
                PlaySoundDrop();
                SpawnItemEntity(heldItem, heldCount, player.position.x + PLAYER_WIDTH / 2, player.position.y);
                ClearHeldItem();
            }
        }

        // Tooltip for inventory slots
        if (heldItem == BLOCK_AIR) {
            for (int row = 0; row < INVENTORY_ROWS; row++) {
                for (int col = 0; col < INVENTORY_COLS; col++) {
                    int idx = row * INVENTORY_COLS + col;
                    int x = invX + col * (slotSize + padding);
                    int y = invY + row * (slotSize + padding);
                    Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                    if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                        BlockType bt = (BlockType)player.inventory[idx];
                        const char *name = GetBlockName(bt);
                        int tw = MeasureGameTextWidth(name, 14);
                        const char *typeLabel = NULL; Color typeColor = {180,180,190,200};
                        if (IsTool(bt)) { typeLabel = S(STR_TYPE_TOOL); typeColor = (Color){100,160,220,255}; }
                        else if (IsArmor(bt)) { typeLabel = S(STR_TYPE_ARMOR); typeColor = (Color){180,100,220,255}; }
                        else if (IsFood(bt)) { typeLabel = S(STR_TYPE_FOOD); typeColor = (Color){100,200,100,255}; }
                        else { typeLabel = S(STR_TYPE_BLOCK); typeColor = (Color){180,175,190,200}; }
                        int typeW = MeasureGameTextWidth(typeLabel, 11); int maxW = tw > typeW ? tw : typeW;
                        char info[64] = {0};
                        if (IsFood(bt)) snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                        else if (IsArmor(bt)) { int av = GetArmorValue(bt); int md = GetArmorMaxDurability(bt); int pct = md > 0 ? player.toolDurability[idx]*100/md : 0; snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), av, pct); }
                        else if (IsTool(bt)) { int md = GetToolMaxDurability(bt); if (md > 0) { int pct = player.toolDurability[idx]*100/md; snprintf(info, sizeof(info), S(STR_TOOLTIP_DURABILITY), pct); } }
                        int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0; if (infoW > maxW) maxW = infoW;
                        int ttH = 36 + (info[0] ? 16 : 0);
                        int boxW = maxW + 10, boxH = ttH + 2;
                        int tx, ty;
                        GetUiTooltipPos((int)mouse.x, (int)mouse.y, boxW, boxH, &tx, &ty);
                        DrawRectangle(tx+1, ty+1, boxW, boxH, (Color){0,0,0,60});
                        DrawRectangle(tx, ty, boxW, boxH, (Color){55,55,55,240});
                        DrawRectangleLines(tx, ty, boxW, boxH, (Color){90,90,90,220});
                        DrawGameText(typeLabel, tx+4, ty+2, 11, typeColor);
                        DrawGameText(name, tx+4, ty+16, 14, (Color){230,225,240,255});
                        if (info[0]) DrawGameText(info, tx+4, ty+32, 14, (Color){180,200,180,255});
                    }
                }
            }
        }

        // Close hint
        DrawGameText(S(STR_PRESS_E_ESC_CLOSE),
            contX + contW / 2 - MeasureGameTextWidth(S(STR_PRESS_E_ESC_CLOSE), 10) / 2,
            contY + contH - 14, 10, (Color){62, 62, 62, 180});

        // Held item follows mouse
        if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
            int mx = (int)mouse.x - slotSize / 2; int my = (int)mouse.y - slotSize / 2;
            Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(mx+4), (float)(my+4), (float)(slotSize-8), (float)(slotSize-8) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0,0}, 0, WHITE);
            if (heldCount > 1) DrawGameText(TextFormat("%d", heldCount), mx+slotSize-20, my+slotSize-16, 14, WHITE);
        }

        // If this client modified the furnace this frame, push changes to the host.
        // If we are the host, broadcast changes to watching clients.
        if (NetIsConnected() && activeFurnace >= 0 && activeFurnace < furnaceCount) {
            if (snapFuel != furnaceFuel || snapFuelCount != furnaceFuelCount ||
                snapInput != furnaceInput || snapInputCount != furnaceInputCount ||
                snapProgress != furnaceProgress || snapFuelBurn != furnaceFuelBurn ||
                snapFuelBurnMax != furnaceFuelBurnMax) {
                SyncActiveToFurnace(activeFurnace);
                if (NetIsClient()) SyncFurnaceToHost();
                else if (NetIsHost()) SyncFurnaceToAll();
            }
        }

        // If inventory/armor changed inside the furnace UI, sync that too.
        if (doInvSync) {
            if (memcmp(snapInv,   player.inventory,        sizeof(snapInv))   != 0 ||
                memcmp(snapCount, player.inventoryCount,   sizeof(snapCount)) != 0 ||
                memcmp(snapDur,   player.toolDurability,   sizeof(snapDur))   != 0 ||
                memcmp(snapEnch,  player.itemEnchantments, sizeof(snapEnch))  != 0 ||
                memcmp(snapArmor, player.armor,            sizeof(snapArmor)) != 0 ||
                memcmp(snapArmorDur,  player.armorDurability,  sizeof(snapArmorDur))  != 0 ||
                memcmp(snapArmorEnch, player.armorEnchantments, sizeof(snapArmorEnch)) != 0) {
                if (NetIsClient()) SyncInventoryToHost();
                else if (NetIsHost()) SyncInventoryToAll();
            }
        }

        return;
    }

    // --- Minecraft-style combined chest + inventory screen ---
    if (chestOpen) {
        int padding = 3;
        int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;

        // Find chest data index
        int chestIdx = -1;
        for (int i = 0; i < chestCount; i++) {
            if (chestData[i].x == chestBlockX && chestData[i].y == chestBlockY) {
                chestIdx = i;
                break;
            }
        }
        // Create chest entry if not found
        if (chestIdx == -1 && chestCount < MAX_CHESTS) {
            chestIdx = chestCount++;
            chestData[chestIdx].x = chestBlockX;
            chestData[chestIdx].y = chestBlockY;
            for (int i = 0; i < CHEST_SLOTS; i++) {
                chestData[chestIdx].items[i] = BLOCK_AIR;
                chestData[chestIdx].counts[i] = 0;
                chestData[chestIdx].durability[i] = 0;
                chestData[chestIdx].enchantments[i] = 0;
            }
        }

        // Snapshot chest contents at the start of the frame so we can detect
        // modifications and sync them to the host in multiplayer.
        uint8_t chestSnapItems[CHEST_SLOTS];
        int chestSnapCounts[CHEST_SLOTS];
        int chestSnapDur[CHEST_SLOTS];
        uint16_t chestSnapEnch[CHEST_SLOTS];
        if (chestIdx >= 0) {
            memcpy(chestSnapItems, chestData[chestIdx].items, sizeof(chestSnapItems));
            memcpy(chestSnapCounts, chestData[chestIdx].counts, sizeof(chestSnapCounts));
            memcpy(chestSnapDur, chestData[chestIdx].durability, sizeof(chestSnapDur));
            memcpy(chestSnapEnch, chestData[chestIdx].enchantments, sizeof(chestSnapEnch));
        }

        int chestRows = 3;
        int chestGridH = chestRows * slotSize + (chestRows - 1) * padding;
        int chestTitleH = 24;
        int dividerH = 2;
        int invTitleH = 20;
        int invGridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;

        int contW = gridW + 28;
        int contH = chestTitleH + chestGridH + dividerH + invTitleH + invGridH + 14;
        int contX = (SCREEN_WIDTH - contW) / 2;
        int contY = (SCREEN_HEIGHT - contH) / 2;
        contY += (int)((1.0f - panelAnim) * 40.0f);

        Vector2 mouse = Win32GetMousePosition();

        // Light overlay
        unsigned char chestOA = (unsigned char)(100 * panelAnim);
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, chestOA});
        // Container
        DrawUiPanel(contX, contY, contW, contH, 255);

        // Chest title
        int cy = contY + 8;
        DrawGameText(S(STR_BLOCK_CHEST), contX + contW / 2 - MeasureGameTextWidth(S(STR_BLOCK_CHEST), 14) / 2, cy, 14, (Color){62, 62, 62, 255});
        cy += chestTitleH;

        // Chest grid (3 rows x 9 cols)
        int chestGridX = contX + 14;
        const char *tooltipText = NULL;
        uint16_t tooltipEnchant = 0;
        int tooltipX = 0, tooltipY = 0;

        for (int row = 0; row < chestRows; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int si = row * INVENTORY_COLS + col;
                int sx = chestGridX + col * (slotSize + padding);
                int sy = cy + row * (slotSize + padding);
                Rectangle slotRect = { (float)sx, (float)sy, (float)slotSize, (float)slotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                DrawUiSlot(sx, sy, slotSize, hover, false, 1.0f);

                if (chestIdx >= 0 && chestData[chestIdx].items[si] != BLOCK_AIR) {
                    uint8_t item = chestData[chestIdx].items[si];
                    int cnt = chestData[chestIdx].counts[si];
                    Rectangle dst = { (float)(sx + 4), (float)(sy + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
                    if (ENCH_TYPE(chestData[chestIdx].enchantments[si]) != ENCH_NONE)
                        DrawEnchantGlint(sx + 4, sy + 4, slotSize - 8);
                    if (cnt > 1) DrawGameText(TextFormat("%d", cnt), sx + slotSize - 20, sy + slotSize - 16, 12, WHITE);
                    if (hover && heldItem == BLOCK_AIR) {
                        BlockType bt = (BlockType)item;
                        const char *name = GetBlockName(bt);
                        const char *typeLabel = NULL; Color typeColor = {180,180,190,200};
                        if (IsTool(bt)) { typeLabel = S(STR_TYPE_TOOL); typeColor = (Color){100,160,220,255}; }
                        else if (IsArmor(bt)) { typeLabel = S(STR_TYPE_ARMOR); typeColor = (Color){180,100,220,255}; }
                        else if (IsFood(bt)) { typeLabel = S(STR_TYPE_FOOD); typeColor = (Color){100,200,100,255}; }
                        else { typeLabel = S(STR_TYPE_BLOCK); typeColor = (Color){180,175,190,200}; }
                        int tw = MeasureGameTextWidth(name, 14);
                        int typeW = MeasureGameTextWidth(typeLabel, 11);
                        int maxW = tw > typeW ? tw : typeW;
                        char info[64] = {0};
                        if (IsFood(bt)) snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                        else if (IsArmor(bt)) snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), GetArmorValue(bt), 100);
                        int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0;
                        if (infoW > maxW) maxW = infoW;
                        int ttH = 36 + (info[0] ? 16 : 0);
                        int boxW = maxW + 10, boxH = ttH + 2;
                        int ttx, tty;
                        GetUiTooltipPos((int)mouse.x, (int)mouse.y, boxW, boxH, &ttx, &tty);
                        DrawRectangle(ttx, tty, boxW, boxH, (Color){55,55,55,240});
                        DrawRectangleLines(ttx, tty, boxW, boxH, (Color){90,90,90,220});
                        DrawGameText(typeLabel, ttx+4, tty+2, 11, typeColor);
                        DrawGameText(name, ttx+4, tty+16, 14, (Color){230,225,240,255});
                        if (info[0]) DrawGameText(info, ttx+4, tty+32, 14, (Color){180,200,180,255});
                    }
                }

                // Shift-click: transfer chest -> inventory
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Win32IsKeyDown(KEY_LEFT_SHIFT) && chestIdx >= 0 && chestData[chestIdx].items[si] != BLOCK_AIR) {
                    uint8_t item = chestData[chestIdx].items[si];
                    int cnt = chestData[chestIdx].counts[si];
                    // Try to merge into existing inventory stacks
                    for (int d = 0; d < INVENTORY_SLOTS && cnt > 0; d++) {
                        if (player.inventory[d] == item && player.inventoryCount[d] < 64) {
                            int space = 64 - player.inventoryCount[d];
                            int add = (cnt < space) ? cnt : space;
                            player.inventoryCount[d] += add;
                            cnt -= add;
                        }
                    }
                    chestData[chestIdx].counts[si] = cnt;
                    if (cnt <= 0) { chestData[chestIdx].items[si] = BLOCK_AIR; chestData[chestIdx].durability[si] = 0; chestData[chestIdx].enchantments[si] = 0; }
                    if (cnt > 0) {
                        for (int d = 0; d < INVENTORY_SLOTS; d++) {
                            if (player.inventory[d] == BLOCK_AIR) {
                                player.inventory[d] = item;
                                player.inventoryCount[d] = cnt;
                                player.toolDurability[d] = chestData[chestIdx].durability[si];
                                player.itemEnchantments[d] = chestData[chestIdx].enchantments[si];
                                chestData[chestIdx].items[si] = BLOCK_AIR;
                                chestData[chestIdx].counts[si] = 0;
                                chestData[chestIdx].durability[si] = 0;
                                chestData[chestIdx].enchantments[si] = 0;
                                break;
                            }
                        }
                    }
                }

                // Click handling for chest slots
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && chestIdx >= 0) {
                    if (heldItem == BLOCK_AIR && chestData[chestIdx].items[si] != BLOCK_AIR) {
                        heldItem = chestData[chestIdx].items[si];
                        heldCount = chestData[chestIdx].counts[si];
                        heldDurability = chestData[chestIdx].durability[si];
                        heldItemEnchant = chestData[chestIdx].enchantments[si];
                        chestData[chestIdx].items[si] = BLOCK_AIR;
                        chestData[chestIdx].counts[si] = 0;
                        chestData[chestIdx].durability[si] = 0;
                        chestData[chestIdx].enchantments[si] = 0;
                    } else if (heldItem != BLOCK_AIR && chestData[chestIdx].items[si] == BLOCK_AIR) {
                        chestData[chestIdx].items[si] = heldItem;
                        chestData[chestIdx].counts[si] = heldCount;
                        chestData[chestIdx].durability[si] = heldDurability;
                        chestData[chestIdx].enchantments[si] = heldItemEnchant;
                        heldItem = BLOCK_AIR;
                        heldCount = 0;
                        heldDurability = 0;
                        heldItemEnchant = 0;
                    } else if (heldItem != BLOCK_AIR && chestData[chestIdx].items[si] == heldItem && chestData[chestIdx].counts[si] < 64) {
                        int space = 64 - chestData[chestIdx].counts[si];
                        int add = (heldCount < space) ? heldCount : space;
                        chestData[chestIdx].counts[si] += add;
                        heldCount -= add;
                        if (heldCount <= 0) { heldItem = BLOCK_AIR; heldDurability = 0; heldItemEnchant = 0; }
                    }
                }
            }
        }

        cy += chestGridH + 4;
        DrawRectangle(contX + 10, cy, contW - 20, dividerH, (Color){90, 90, 90, 200});
        cy += dividerH + 4;

        // Inventory title
        DrawGameText(S(STR_INVENTORY), contX + contW / 2 - MeasureGameTextWidth(S(STR_INVENTORY), 12) / 2, cy, 12, (Color){62, 62, 62, 200});
        cy += invTitleH;

        // Player inventory grid (4 rows x 9 cols)
        int invGridX = contX + 14;
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int si = row * INVENTORY_COLS + col;
                int sx = invGridX + col * (slotSize + padding);
                int sy = cy + row * (slotSize + padding);
                Rectangle slotRect = { (float)sx, (float)sy, (float)slotSize, (float)slotSize };
                bool hover = CheckCollisionPointRec(mouse, slotRect);
                DrawUiSlot(sx, sy, slotSize, hover, false, 1.0f);

                if (player.inventory[si] != BLOCK_AIR) {
                    uint8_t item = player.inventory[si];
                    int cnt = player.inventoryCount[si];
                    Rectangle dst = { (float)(sx + 4), (float)(sy + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                    Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                    DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
                    if (ENCH_TYPE(player.itemEnchantments[si]) != ENCH_NONE)
                        DrawEnchantGlint(sx + 4, sy + 4, slotSize - 8);
                    if (cnt > 1) DrawGameText(TextFormat("%d", cnt), sx + slotSize - 20, sy + slotSize - 16, 12, WHITE);
                    if (hover && heldItem == BLOCK_AIR) {
                        tooltipText = GetBlockName((BlockType)item);
                        tooltipEnchant = player.itemEnchantments[si];
                        tooltipX = (int)mouse.x;
                        tooltipY = (int)mouse.y - 20;
                    }
                }

                // Click handling for inventory slots
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (heldItem == BLOCK_AIR && player.inventory[si] != BLOCK_AIR) {
                        heldItem = player.inventory[si];
                        heldCount = player.inventoryCount[si];
                        heldDurability = player.toolDurability[si];
                        heldItemEnchant = player.itemEnchantments[si];
                        player.inventory[si] = BLOCK_AIR;
                        player.inventoryCount[si] = 0;
                        player.toolDurability[si] = 0;
                        player.itemEnchantments[si] = 0;
                    } else if (heldItem != BLOCK_AIR && player.inventory[si] == BLOCK_AIR) {
                        player.inventory[si] = heldItem;
                        player.inventoryCount[si] = heldCount;
                        player.toolDurability[si] = heldDurability;
                        player.itemEnchantments[si] = heldItemEnchant;
                        heldItem = BLOCK_AIR;
                        heldCount = 0;
                        heldDurability = 0;
                        heldItemEnchant = 0;
                    } else if (heldItem != BLOCK_AIR && player.inventory[si] == heldItem && player.inventoryCount[si] < 64) {
                        int space = 64 - player.inventoryCount[si];
                        int add = (heldCount < space) ? heldCount : space;
                        player.inventoryCount[si] += add;
                        heldCount -= add;
                        if (heldCount <= 0) { heldItem = BLOCK_AIR; heldDurability = 0; heldItemEnchant = 0; }
                    }
                }

                // Shift-click: transfer inventory -> chest
                if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && Win32IsKeyDown(KEY_LEFT_SHIFT) && player.inventory[si] != BLOCK_AIR && chestIdx >= 0) {
                    uint8_t item = player.inventory[si];
                    int cnt = player.inventoryCount[si];
                    // Try to merge into existing chest slot
                    bool merged = false;
                    for (int ci = 0; ci < CHEST_SLOTS; ci++) {
                        if (chestData[chestIdx].items[ci] == item && chestData[chestIdx].counts[ci] < 64) {
                            int space = 64 - chestData[chestIdx].counts[ci];
                            int add = (cnt < space) ? cnt : space;
                            chestData[chestIdx].counts[ci] += add;
                            cnt -= add;
                            if (cnt <= 0) break;
                        }
                    }
                    if (cnt <= 0) { player.inventory[si] = BLOCK_AIR; player.inventoryCount[si] = 0; player.toolDurability[si] = 0; player.itemEnchantments[si] = 0; merged = true; }
                    // Place remaining in first empty slot
                    if (!merged) {
                        for (int ci = 0; ci < CHEST_SLOTS; ci++) {
                            if (chestData[chestIdx].items[ci] == BLOCK_AIR) {
                                chestData[chestIdx].items[ci] = item;
                                chestData[chestIdx].counts[ci] = cnt;
                                chestData[chestIdx].durability[ci] = player.toolDurability[si];
                                chestData[chestIdx].enchantments[ci] = player.itemEnchantments[si];
                                player.inventory[si] = BLOCK_AIR;
                                player.inventoryCount[si] = 0;
                                player.toolDurability[si] = 0;
                                player.itemEnchantments[si] = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Tooltip
        if (tooltipText) {
            int tw = MeasureGameTextWidth(tooltipText, 14) + 10;
            int th = 20;
            if (ENCH_TYPE(tooltipEnchant) != ENCH_NONE) th = 36;
            int tx, ty;
            GetUiTooltipPos(tooltipX, tooltipY, tw, th, &tx, &ty);
            DrawRectangle(tx, ty, tw, th, (Color){16, 0, 16, 235});
            DrawGameText(tooltipText, tx + 5, ty + 2, 14, WHITE);
            if (ENCH_TYPE(tooltipEnchant) != ENCH_NONE) {
                int enchType = ENCH_TYPE(tooltipEnchant);
                int enchLvl = ENCH_LEVEL(tooltipEnchant);
                StringId enchStr = STR_NONE;
                if (enchType == ENCH_SHARPNESS) enchStr = STR_ENCH_SHARPNESS;
                else if (enchType == ENCH_EFFICIENCY) enchStr = STR_ENCH_EFFICIENCY;
                else if (enchType == ENCH_PROTECTION) enchStr = STR_ENCH_PROTECTION;
                else if (enchType == ENCH_FORTUNE) enchStr = STR_ENCH_FORTUNE;
                else if (enchType == ENCH_UNBREAKING) enchStr = STR_ENCH_UNBREAKING;
                else if (enchType == ENCH_SILK_TOUCH) enchStr = STR_ENCH_SILK_TOUCH;
                else if (enchType == ENCH_POWER) enchStr = STR_ENCH_POWER;
                const char *enchName = S(enchStr);
                int ew = MeasureGameTextWidth(TextFormat("%s %d", enchName, enchLvl), 12) + 10;
                if (ew + 10 > tw) tw = ew + 10;
                GetUiTooltipPos(tooltipX, tooltipY, tw, th, &tx, &ty);
                DrawRectangle(tx, ty, tw, th, (Color){16, 0, 16, 235});
                DrawRectangleLines(tx, ty, tw, th, (Color){80, 0, 255, 170});
                DrawGameText(TextFormat("%s %d", enchName, enchLvl), tx + 5, ty + 18, 14, (Color){180, 120, 255, 255});
            }
        }

        // Draw held item on cursor
        if (heldItem != BLOCK_AIR) {
            int mx = (int)mouse.x, my = (int)mouse.y;
            Rectangle dst = { (float)(mx - 12), (float)(my - 12), 24, 24 };
            Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (heldCount > 1) DrawGameText(TextFormat("%d", heldCount), mx + 8, my + 8, 12, WHITE);
        }

        // If this client modified the chest this frame, push changes to the host.
        // If we are the host, broadcast changes to watching clients.
        if (chestIdx >= 0 && NetIsConnected()) {
            if (memcmp(chestSnapItems, chestData[chestIdx].items, sizeof(chestSnapItems)) != 0 ||
                memcmp(chestSnapCounts, chestData[chestIdx].counts, sizeof(chestSnapCounts)) != 0 ||
                memcmp(chestSnapDur, chestData[chestIdx].durability, sizeof(chestSnapDur)) != 0 ||
                memcmp(chestSnapEnch, chestData[chestIdx].enchantments, sizeof(chestSnapEnch)) != 0) {
                if (NetIsClient()) SyncChestToHost(chestIdx);
                else if (NetIsHost()) SyncChestToAll(chestIdx);
            }
        }

        // If inventory/armor changed inside the chest UI, sync that too.
        if (doInvSync) {
            if (memcmp(snapInv,   player.inventory,        sizeof(snapInv))   != 0 ||
                memcmp(snapCount, player.inventoryCount,   sizeof(snapCount)) != 0 ||
                memcmp(snapDur,   player.toolDurability,   sizeof(snapDur))   != 0 ||
                memcmp(snapEnch,  player.itemEnchantments, sizeof(snapEnch))  != 0 ||
                memcmp(snapArmor, player.armor,            sizeof(snapArmor)) != 0 ||
                memcmp(snapArmorDur,  player.armorDurability,  sizeof(snapArmorDur))  != 0 ||
                memcmp(snapArmorEnch, player.armorEnchantments, sizeof(snapArmorEnch)) != 0) {
                if (NetIsClient()) SyncInventoryToHost();
                else if (NetIsHost()) SyncInventoryToAll();
            }
        }

        return;
    }

    // --- Normal inventory screen (when furnace is NOT open) ---
    int padding = 3;
    int gridW = INVENTORY_COLS * slotSize + (INVENTORY_COLS - 1) * padding;
    int gridH = INVENTORY_ROWS * slotSize + (INVENTORY_ROWS - 1) * padding;

    // Player preview dimensions
    int previewW = 86;
    int previewPad = 6;

    // Armor slots: vertical column to the left of inventory
    int armorSlotSize = 40;
    int armorPad = 3;
    int armorColW = armorSlotSize + armorPad;

    // Crafting panel dimensions (kept short so the panel hugs the inventory)
    int craftSlotH = 44;
    int craftPad = 2;
    int craftPanelW = 360;
    // Show as many recipes as fit the panel height budget rather than deriving it
    // from the inventory grid: tying it to the grid left only 3 visible rows and
    // wasted most of the crafting column.
    int visibleRecipes = 7;
    {
        int maxPanelH = SCREEN_HEIGHT - 80;
        int maxRows = (maxPanelH - 28 - 32) / (craftSlotH + craftPad);
        if (maxRows < 4) maxRows = 4;
        if (visibleRecipes > maxRows) visibleRecipes = maxRows;
    }
    int craftVisibleH = visibleRecipes * (craftSlotH + craftPad);
    int craftPanelH = craftVisibleH + 32; // title + padding

    // Unified container
    int panelPad = 14;
    int dividerW = 2;
    int totalW = panelPad + previewW + previewPad + armorColW + gridW + panelPad + dividerW + panelPad + craftPanelW + panelPad;
    int totalH = panelPad + 24 + gridH + panelPad; // 24 for title
    if (craftPanelH + panelPad > totalH - panelPad) totalH = craftPanelH + panelPad * 2;

    int containerX = (SCREEN_WIDTH - totalW) / 2;
    int containerY = (SCREEN_HEIGHT - totalH) / 2;
    containerY += (int)((1.0f - panelAnim) * 40.0f);

    // Background overlay
    unsigned char invOA = (unsigned char)(160 * panelAnim);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, invOA});

    // Container background with subtle gradient feel
    DrawUiPanel(containerX, containerY, totalW, totalH, 255);

    // Inventory title
    int invX = containerX + panelPad + previewW + previewPad + armorColW;
    // Centre the left column vertically. The panel height is driven by the recipe
    // list, so anchoring the inventory to the top left dead space underneath it.
    int leftContentH = 24 + gridH;
    int invY = containerY + (totalH - leftContentH) / 2;
    if (invY < containerY + panelPad) invY = containerY + panelPad;
    Vector2 mouse = Win32GetMousePosition();
    DrawGameText(S(STR_INVENTORY), invX, invY, 14, (Color){62, 62, 62, 255});
    invY += 24;

    // --- Player Preview ---
    {
        int prevX = containerX + panelPad;
        int prevY = invY;
        int prevH = gridH;

        // Preview background with subtle pattern
        DrawUiBox(prevX, prevY, previewW, prevH, 0.06f, (Color){36, 36, 36, 220});
        DrawRectangleLines(prevX, prevY, previewW, prevH, (Color){90, 90, 90, 180});
        // Inner shadow (top darker, bottom lighter)
        DrawRectangle(prevX + 1, prevY + 1, previewW - 2, 3, (Color){20, 18, 28, 100});
        DrawRectangle(prevX + 1, prevY + prevH - 4, previewW - 2, 3, (Color){50, 47, 60, 80});

        // Draw scaled player character (3x scale)
        int scale = 3;
        int charW = 12 * scale;
        int charH = 28 * scale;
        int cx = prevX + (previewW - charW) / 2;
        int cy = prevY + (prevH - charH) / 2;

        // Player colors
        Color skin = (Color){220, 180, 140, 255};
        Color hair = (Color){80, 50, 30, 255};
        Color shirt = (Color){0, 100, 200, 255};
        Color pants = (Color){60, 40, 20, 255};

        // Armor colors
        Color hc = {0,0,0,0}, cc = {0,0,0,0}, lc = {0,0,0,0}, bc = {0,0,0,0};
        for (int i = 0; i < 4; i++) {
            if (player.armor[i] != BLOCK_AIR) {
                BlockType a = (BlockType)player.armor[i];
                Color ac;
                if (a >= ARMOR_DIAMOND_HELMET) ac = (Color){80, 220, 230, 255};
                else if (a >= ARMOR_GOLD_HELMET) ac = (Color){220, 180, 50, 255};
                else if (a >= ARMOR_IRON_HELMET) ac = (Color){200, 210, 220, 255};
                else if (a >= ARMOR_STONE_HELMET) ac = (Color){140, 140, 140, 255};
                else if (a >= ARMOR_WOOD_HELMET) ac = (Color){160, 120, 60, 255};
                else ac = (Color){160, 120, 60, 255};
                if (i == 0) hc = ac; else if (i == 1) cc = ac; else if (i == 2) lc = ac; else bc = ac;
            }
        }

        // Head
        DrawRectangle(cx + 2*scale, cy, 8*scale, 8*scale, skin);
        DrawRectangle(cx + 2*scale, cy, 8*scale, 3*scale, hair);
        // Helmet overlay
        if (hc.a > 0) {
            DrawRectangle(cx + 1*scale, cy - 1*scale, 10*scale, 5*scale, hc);
            DrawRectangle(cx + 2*scale, cy + 4*scale, 8*scale, 2*scale, hc);
        }
        // Eyes
        DrawRectangle(cx + 3*scale, cy + 4*scale, 2*scale, 2*scale, (Color){40, 40, 40, 255});
        DrawRectangle(cx + 7*scale, cy + 4*scale, 2*scale, 2*scale, (Color){40, 40, 40, 255});

        // Body
        DrawRectangle(cx + 1*scale, cy + 8*scale, 10*scale, 10*scale, shirt);
        // Chestplate overlay
        if (cc.a > 0) {
            DrawRectangle(cx, cy + 7*scale, 12*scale, 11*scale, cc);
            DrawRectangle(cx + 1*scale, cy + 8*scale, 10*scale, 9*scale, (Color){
                (unsigned char)(cc.r * 0.8f), (unsigned char)(cc.g * 0.8f), (unsigned char)(cc.b * 0.8f), 255});
        }

        // Arms
        DrawRectangle(cx - 2*scale, cy + 8*scale, 3*scale, 10*scale, skin);
        DrawRectangle(cx + 11*scale, cy + 8*scale, 3*scale, 10*scale, skin);

        // Legs
        DrawRectangle(cx + 1*scale, cy + 18*scale, 4*scale, 10*scale, pants);
        DrawRectangle(cx + 7*scale, cy + 18*scale, 4*scale, 10*scale, pants);
        // Leggings overlay
        if (lc.a > 0) {
            DrawRectangle(cx, cy + 18*scale, 5*scale, 10*scale, lc);
            DrawRectangle(cx + 6*scale, cy + 18*scale, 5*scale, 10*scale, lc);
        }
        // Boots overlay
        if (bc.a > 0) {
            DrawRectangle(cx, cy + 25*scale, 5*scale, 3*scale, bc);
            DrawRectangle(cx + 6*scale, cy + 25*scale, 5*scale, 3*scale, bc);
        }

        // Day/night tint to match in-game lighting
        if (dayNight.lightLevel < 1.0f) {
            unsigned char a = (unsigned char)(120 * (1.0f - dayNight.lightLevel));
            DrawRectangle(cx - 2*scale, cy - 1*scale, 16*scale, 29*scale, (Color){5, 5, 25, a});
        }

        // Health bar under preview
        int hpBarX = prevX + 4;
        int hpBarY = prevY + prevH - 14;
        int hpBarW = previewW - 8;
        int hpBarH = 4;
        float hpPct = (float)player.health / MAX_HEALTH;
        DrawRectangle(hpBarX, hpBarY, hpBarW, hpBarH, (Color){60, 20, 20, 200});
        DrawRectangle(hpBarX, hpBarY, (int)(hpBarW * hpPct), hpBarH, hpPct > 0.5f ? (Color){200, 50, 50, 255} : (Color){255, 80, 80, 255});

        // Hunger bar
        int hungerBarY = hpBarY + hpBarH + 2;
        float hungerPct = (float)player.hunger / MAX_HUNGER;
        DrawRectangle(hpBarX, hungerBarY, hpBarW, hpBarH, (Color){60, 50, 10, 200});
        DrawRectangle(hpBarX, hungerBarY, (int)(hpBarW * hungerPct), hpBarH, (Color){180, 140, 40, 255});

        // Armor value display
        int totalArmor = 0;
        for (int i = 0; i < 4; i++) {
            if (player.armor[i] != BLOCK_AIR) totalArmor += GetArmorValue((BlockType)player.armor[i]);
        }
        if (totalArmor > 0) {
            int armorIconX = prevX + 4;
            int armorIconY = prevY + 4;
            // Shield icon (small)
            DrawRectangle(armorIconX, armorIconY, 8, 10, (Color){100, 100, 120, 200});
            DrawRectangle(armorIconX + 1, armorIconY + 1, 6, 4, (Color){140, 140, 160, 200});
            DrawGameText(TextFormat("%d", totalArmor), armorIconX + 10, armorIconY, 14, (Color){180, 200, 220, 255});
        }
    }

    // Armor slots (vertical, to the left of inventory)
    {
        int armorX = containerX + panelPad + previewW + previewPad;
        int armorY = invY;
        for (int i = 0; i < 4; i++) {
            int ay = armorY + i * (armorSlotSize + armorPad);
            Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect) && !furnaceOpen && !chestOpen;

            DrawUiSlot(armorX, ay, armorSlotSize, hover, false, 1.0f);

            // Draw equipped armor item
            if (player.armor[i] != BLOCK_AIR && blockAtlas.id > 0) {
                int item = player.armor[i];
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(armorX + 4), (float)(ay + 4), (float)(armorSlotSize - 8), (float)(armorSlotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                // Durability bar
                int maxDur = GetArmorMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    float pct = (float)player.armorDurability[i] / maxDur;
                    int barW = armorSlotSize - 8;
                    int barX = armorX + 4;
                    int barY = ay + armorSlotSize - 5;
                    Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                    DrawRectangle(barX, barY, barW, 3, (Color){0, 0, 0, 180});
                    DrawRectangle(barX, barY, (int)(barW * pct), 3, barColor);
                }
            } else {
                // Empty armor slot: faint MC-style silhouette icon
                Color ghost = (Color){90, 90, 90, 90};
                int icx = armorX + armorSlotSize / 2;
                int icy = ay + armorSlotSize / 2;
                if (i == 0) {          // helmet
                    DrawRectangle(icx - 8, icy - 7, 16, 8, ghost);
                    DrawRectangle(icx - 6, icy + 1, 12, 3, ghost);
                } else if (i == 1) {   // chestplate
                    DrawRectangle(icx - 8, icy - 7, 16, 14, ghost);
                    DrawRectangle(icx - 11, icy - 6, 3, 9, ghost);
                    DrawRectangle(icx + 8, icy - 6, 3, 9, ghost);
                } else if (i == 2) {   // leggings
                    DrawRectangle(icx - 7, icy - 6, 6, 13, ghost);
                    DrawRectangle(icx + 1, icy - 6, 6, 13, ghost);
                } else {               // boots
                    DrawRectangle(icx - 7, icy - 2, 6, 8, ghost);
                    DrawRectangle(icx + 1, icy - 2, 6, 8, ghost);
                }
            }

            // Click handling for armor slots
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                    // Shift-click: auto-equip from inventory
                    if (player.armor[i] == BLOCK_AIR) {
                        for (int s = 0; s < INVENTORY_SLOTS; s++) {
                            if (IsArmor((BlockType)player.inventory[s])) {
                                int slotType = GetArmorSlot((BlockType)player.inventory[s]);
                                if (slotType == i) {
                                    player.armor[i] = player.inventory[s];
                                    player.armorDurability[i] = player.toolDurability[s];
                                    player.armorEnchantments[i] = player.itemEnchantments[s];
                                    player.inventory[s] = BLOCK_AIR;
                                    player.inventoryCount[s] = 0;
                                    player.toolDurability[s] = 0;
                                    player.itemEnchantments[s] = 0;
                                    PlaySoundUIClick();
                                    break;
                                }
                            }
                        }
                    }
                } else if (heldItem == BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                    // Pick up equipped armor
                    heldItem = player.armor[i];
                    heldCount = 1;
                    heldDurability = player.armorDurability[i];
                    heldItemEnchant = player.armorEnchantments[i];
                    player.armor[i] = BLOCK_AIR;
                    player.armorDurability[i] = 0;
                    player.armorEnchantments[i] = 0;
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.armor[i] == BLOCK_AIR) {
                    // Equip held armor
                    if (IsArmor((BlockType)heldItem)) {
                        int slotType = GetArmorSlot((BlockType)heldItem);
                        if (slotType == i) {
                            player.armor[i] = heldItem;
                            player.armorDurability[i] = heldDurability;
                            player.armorEnchantments[i] = heldItemEnchant;
                            ClearHeldItem();
                            PlaySoundUIClick();
                        }
                    }
                } else if (heldItem != BLOCK_AIR && player.armor[i] != BLOCK_AIR) {
                    // Swap held armor with equipped
                    if (IsArmor((BlockType)heldItem)) {
                        int slotType = GetArmorSlot((BlockType)heldItem);
                        if (slotType == i) {
                            uint8_t tmpItem = player.armor[i];
                            int tmpDur = player.armorDurability[i];
                            uint16_t tmpEnch = player.armorEnchantments[i];
                            player.armor[i] = heldItem;
                            player.armorDurability[i] = heldDurability;
                            player.armorEnchantments[i] = heldItemEnchant;
                            heldItem = tmpItem;
                            heldCount = 1;
                            heldDurability = tmpDur;
                            heldItemEnchant = tmpEnch;
                            PlaySoundUIClick();
                        }
                    }
                }
            }
        }

        // Armor tooltip
        if (heldItem == BLOCK_AIR) {
            for (int i = 0; i < 4; i++) {
                int ay = armorY + i * (armorSlotSize + armorPad);
                Rectangle slotRect = { (float)armorX, (float)ay, (float)armorSlotSize, (float)armorSlotSize };
                if (CheckCollisionPointRec(mouse, slotRect) && player.armor[i] != BLOCK_AIR) {
                    BlockType bt = (BlockType)player.armor[i];
                    const char *name = GetBlockName(bt);
                    int tw = MeasureGameTextWidth(name,14);

                    char info[64] = { 0 };
                    int armorVal = GetArmorValue(bt);
                    int maxDur = GetArmorMaxDurability(bt);
                    int pct = maxDur > 0 ? player.armorDurability[i] * 100 / maxDur : 0;
                    snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), armorVal, pct);
                    int iw = MeasureGameTextWidth(info,13);
                    if (iw > tw) tw = iw;

                    uint16_t aEnch = player.armorEnchantments[i];
                    char enchBuf[32] = {0};
                    if (ENCH_TYPE(aEnch) != ENCH_NONE) {
                        snprintf(enchBuf, sizeof(enchBuf), "%s %d",
                            GetEnchantName((EnchantmentType)ENCH_TYPE(aEnch)),
                            ENCH_LEVEL(aEnch));
                        int ew = MeasureGameTextWidth(enchBuf, 12);
                        if (ew > tw) tw = ew;
                    }

                    // Reserve one row for name + one per info/enchant line.
                    int rows = 1 + 1 + (enchBuf[0] ? 1 : 0);
                    int boxW = tw + 8, boxH = rows * 16;
                    int tx, ty;
                    GetUiTooltipPos((int)mouse.x, (int)mouse.y, boxW, boxH, &tx, &ty);

                    DrawRectangle(tx + 1, ty + 1, boxW, boxH, (Color){0, 0, 0, 60});
                    DrawRectangle(tx - 4, ty - 2, boxW, boxH, (Color){16, 0, 16, 235});
                    DrawRectangleLines(tx - 4, ty - 2, boxW, boxH, (Color){80, 0, 255, 170});
                    DrawGameText(name, tx, ty, 14, (Color){230, 225, 240, 255});

                    ty += 16;
                    DrawRectangle(tx - 4, ty - 2, boxW, 15, (Color){36, 36, 36, 230});
                    DrawGameText(info, tx, ty,14, (Color){180, 200, 180, 255});

                    if (enchBuf[0]) {
                        ty += 16;
                        DrawRectangle(tx - 4, ty - 2, boxW, 15, (Color){16, 0, 16, 235});
                        DrawRectangleLines(tx - 4, ty - 2, boxW, 15, (Color){150, 150, 150, 210});
                        DrawGameText(enchBuf, tx, ty, 14, (Color){180, 120, 255, 255});
                    }
                }
            }
        }
    }

    // Divider line
    int divX = containerX + panelPad + previewW + previewPad + armorColW + gridW + panelPad;
    DrawRectangle(divX, containerY + 8, dividerW, totalH - 16, (Color){90, 90, 90, 200});


    // Sort button
    {
        int sortBtnX = invX + 84;
        int sortBtnY = invY - 26;
        int sortBtnW = 40;
        int sortBtnH = 16;
        Rectangle sortBtn = { (float)sortBtnX, (float)sortBtnY, (float)sortBtnW, (float)sortBtnH };
        bool sortHover = CheckCollisionPointRec(mouse, sortBtn);
        DrawUiButton(sortBtnX, sortBtnY, sortBtnW, sortBtnH, S(STR_SORT), 13, sortHover, false, true, 1.0f);
        if (sortHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            SortInventory();
            PlaySoundUIClick();
        }
    }

    // Inventory grid
    for (int row = 0; row < INVENTORY_ROWS; row++) {
        for (int col = 0; col < INVENTORY_COLS; col++) {
            int idx = row * INVENTORY_COLS + col;
            int x = invX + col * (slotSize + padding);
            int y = invY + row * (slotSize + padding);

            Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);
            bool selected = (row == 0 && col == player.selectedSlot);

            // Slot background (modern dark-flat)
            DrawUiSlot(x, y, slotSize, hover, selected, 1.0f);

            int item = player.inventory[idx];
            // Item type color indicator (small bar at bottom)
            if (item != BLOCK_AIR) {
                Color typeColor = {0, 0, 0, 0};
                if (IsTool((BlockType)item)) typeColor = (Color){80, 140, 200, 180};
                else if (IsArmor((BlockType)item)) typeColor = (Color){160, 80, 200, 180};
                else if (IsFood((BlockType)item)) typeColor = (Color){80, 180, 80, 180};
                if (typeColor.a > 0) {
                    DrawRectangle(x + 3, y + slotSize - 4, slotSize - 6, 2, typeColor);
                }
            }
            if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                if (player.inventoryCount[idx] > 1) {
                    // Shadow for readability
                    DrawGameText(TextFormat("%d", player.inventoryCount[idx]),
                             x + slotSize - 19, y + slotSize - 13, 14, (Color){0, 0, 0, 150});
                    DrawGameText(TextFormat("%d", player.inventoryCount[idx]),
                             x + slotSize - 20, y + slotSize - 14, 13, WHITE);
                }

                if (IsTool((BlockType)item)) {
                    int maxDur = GetToolMaxDurability((BlockType)player.inventory[idx]);
                    if (maxDur > 0) {
                        float pct = (float)player.toolDurability[idx] / maxDur;
                        int barW = slotSize - 8;
                        int barH = 3;
                        int barX = x + 4;
                        int barY = y + slotSize - 5;
                        Color barColor = pct > 0.5f ? GREEN : (pct > 0.25f ? YELLOW : RED);
                        DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 180});
                        DrawRectangle(barX, barY, (int)(barW * pct), barH, barColor);
                    }
                }
            }

            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT)) {
                    if (player.inventory[idx] != BLOCK_AIR) {
                        bool transferred = false;
                        if (furnaceOpen) {
                            uint8_t slotItem = player.inventory[idx];
                            int count = player.inventoryCount[idx];
                            if (GetFuelBurnTime(slotItem) > 0.0f) {
                                // Transfer fuel to fuel slot
                                if (furnaceFuel == BLOCK_AIR) {
                                    furnaceFuel = slotItem;
                                    furnaceFuelCount = count;
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    transferred = true;
                                } else if (furnaceFuel == slotItem) {
                                    int space = 64 - furnaceFuelCount;
                                    int toAdd = count > space ? space : count;
                                    furnaceFuelCount += toAdd;
                                    player.inventoryCount[idx] -= toAdd;
                                    if (player.inventoryCount[idx] <= 0) {
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                    }
                                    transferred = true;
                                }
                            } else if (FindSmeltRecipe((BlockType)slotItem) >= 0) {
                                // Transfer smeltable to input slot
                                if (furnaceInput == BLOCK_AIR) {
                                    furnaceInput = slotItem;
                                    furnaceInputCount = count;
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    transferred = true;
                                } else if (furnaceInput == slotItem) {
                                    int space = 64 - furnaceInputCount;
                                    int toAdd = count > space ? space : count;
                                    furnaceInputCount += toAdd;
                                    player.inventoryCount[idx] -= toAdd;
                                    if (player.inventoryCount[idx] <= 0) {
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                    }
                                    transferred = true;
                                }
                            }
                        }
                        if (!transferred) {
                            // Auto-equip armor when shift-clicking
                            if (IsArmor((BlockType)player.inventory[idx])) {
                                int slot = GetArmorSlot((BlockType)player.inventory[idx]);
                                if (slot >= 0 && player.armor[slot] == BLOCK_AIR) {
                                    player.armor[slot] = player.inventory[idx];
                                    player.armorDurability[slot] = player.toolDurability[idx];
                                    player.armorEnchantments[slot] = player.itemEnchantments[idx];
                                    player.inventory[idx] = BLOCK_AIR;
                                    player.inventoryCount[idx] = 0;
                                    player.toolDurability[idx] = 0;
                                    player.itemEnchantments[idx] = 0;
                                    transferred = true;
                                    PlaySoundUIClick();
                                }
                            }
                        }
                        if (!transferred) {
                            int startDest = (row == 0) ? HOTBAR_SLOTS : 0;
                            int endDest = (row == 0) ? INVENTORY_SLOTS : HOTBAR_SLOTS;
                            bool moved = false;
                            for (int d = startDest; d < endDest; d++) {
                                if (player.inventory[d] == player.inventory[idx] && player.inventoryCount[d] < 64) {
                                    int space = 64 - player.inventoryCount[d];
                                    int toAdd = player.inventoryCount[idx] > space ? space : player.inventoryCount[idx];
                                    player.inventoryCount[d] += toAdd;
                                    player.inventoryCount[idx] -= toAdd;
                                    if (player.inventoryCount[idx] <= 0) {
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                    }
                                    moved = true;
                                    break;
                                }
                            }
                            if (!moved) {
                                for (int d = startDest; d < endDest; d++) {
                                    if (player.inventory[d] == BLOCK_AIR) {
                                        player.inventory[d] = player.inventory[idx];
                                        player.inventoryCount[d] = player.inventoryCount[idx];
                                        player.toolDurability[d] = player.toolDurability[idx];
                                        player.inventory[idx] = BLOCK_AIR;
                                        player.inventoryCount[idx] = 0;
                                        player.toolDurability[idx] = 0;
                                        break;
                                    }
                                }
                            }
                        }
                        PlaySoundUIClick();
                    }
                } else if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    heldItem = player.inventory[idx];
                    heldCount = player.inventoryCount[idx];
                    heldDurability = player.toolDurability[idx];
                    heldItemEnchant = player.itemEnchantments[idx];
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                    player.itemEnchantments[idx] = 0;
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    player.toolDurability[idx] = heldDurability;
                    player.itemEnchantments[idx] = heldItemEnchant;
                    ClearHeldItem();
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem) {
                    int space = 64 - player.inventoryCount[idx];
                    int toAdd = heldCount > space ? space : heldCount;
                    player.inventoryCount[idx] += toAdd;
                    heldCount -= toAdd;
                    if (heldCount <= 0) { ClearHeldItem(); }
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    uint8_t tmpItem = player.inventory[idx];
                    int tmpCount = player.inventoryCount[idx];
                    int tmpDur = player.toolDurability[idx];
                    uint16_t tmpEnch = player.itemEnchantments[idx];
                    player.inventory[idx] = heldItem;
                    player.inventoryCount[idx] = heldCount;
                    player.toolDurability[idx] = heldDurability;
                    player.itemEnchantments[idx] = heldItemEnchant;
                    heldItem = tmpItem;
                    heldCount = tmpCount;
                    heldDurability = tmpDur;
                    heldItemEnchant = tmpEnch;
                    PlaySoundUIClick();
                }
            }

            if (hover && Win32IsKeyPressed(KEY_Q) && player.inventory[idx] != BLOCK_AIR) {
                PlaySoundDrop();
                player.inventoryCount[idx]--;
                if (player.inventoryCount[idx] <= 0) {
                    player.inventory[idx] = BLOCK_AIR;
                    player.inventoryCount[idx] = 0;
                    player.toolDurability[idx] = 0;
                }
            }

            // Right-click: split stack OR place one item into matching slot
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                if (heldItem == BLOCK_AIR && player.inventory[idx] != BLOCK_AIR) {
                    // Split stack
                    heldItem = player.inventory[idx];
                    heldCount = (player.inventoryCount[idx] + 1) / 2;
                    player.inventoryCount[idx] -= heldCount;
                    if (player.inventoryCount[idx] <= 0) {
                        player.inventory[idx] = BLOCK_AIR;
                        player.inventoryCount[idx] = 0;
                    }
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == heldItem && player.inventoryCount[idx] < 64) {
                    // Place one item into matching stack
                    player.inventoryCount[idx]++;
                    heldCount--;
                    if (heldCount <= 0) { ClearHeldItem(); }
                    PlaySoundUIClick();
                } else if (heldItem != BLOCK_AIR && player.inventory[idx] == BLOCK_AIR) {
                    // Place one item into empty slot
                    player.inventory[idx] = (uint8_t)heldItem;
                    player.inventoryCount[idx] = 1;
                    player.toolDurability[idx] = (IsTool((BlockType)heldItem) && heldDurability > 0) ? heldDurability : 0;
                    heldCount--;
                    if (heldCount <= 0) { ClearHeldItem(); }
                    PlaySoundUIClick();
                }
            }
        }
    }

    // Crafting panel
    int craftX = divX + dividerW + panelPad;
    int craftY = containerY + panelPad;
    DrawCraftingPanel(craftX, craftY, craftPanelW, visibleRecipes, craftSlotH, craftPad, craftingTableOpen);

    // Drop held item by clicking outside the container (skip when furnace is open)
    if (heldItem != BLOCK_AIR && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !furnaceOpen && !chestOpen) {
        Rectangle container = { (float)containerX, (float)containerY, (float)totalW, (float)totalH };
        if (!CheckCollisionPointRec(mouse, container)) {
            PlaySoundDrop();
            SpawnItemEntity(heldItem, heldCount,
                            player.position.x + PLAYER_WIDTH / 2,
                            player.position.y);
            heldItem = BLOCK_AIR;
            heldCount = 0;
            heldDurability = 0;
        }
    }

    // Tooltip for hovered slot (skip when furnace is open)
    if (heldItem == BLOCK_AIR && !furnaceOpen && !chestOpen) {
        for (int row = 0; row < INVENTORY_ROWS; row++) {
            for (int col = 0; col < INVENTORY_COLS; col++) {
                int idx = row * INVENTORY_COLS + col;
                int x = invX + col * (slotSize + padding);
                int y = invY + row * (slotSize + padding);
                Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
                if (CheckCollisionPointRec(mouse, slotRect) && player.inventory[idx] != BLOCK_AIR) {
                    BlockType bt = (BlockType)player.inventory[idx];
                    const char *name = GetBlockName(bt);
                    int tw = MeasureGameTextWidth(name,14);

                    // Item type label
                    const char *typeLabel = NULL;
                    Color typeColor = {180, 180, 190, 200};
                    if (IsTool(bt)) { typeLabel = S(STR_TYPE_TOOL); typeColor = (Color){100, 160, 220, 255}; }
                    else if (IsArmor(bt)) { typeLabel = S(STR_TYPE_ARMOR); typeColor = (Color){180, 100, 220, 255}; }
                    else if (IsFood(bt)) { typeLabel = S(STR_TYPE_FOOD); typeColor = (Color){100, 200, 100, 255}; }
                    else { typeLabel = S(STR_TYPE_BLOCK); typeColor = (Color){180, 175, 190, 200}; }

                    int typeW = MeasureGameTextWidth(typeLabel, 11);
                    int maxW = tw > typeW ? tw : typeW;

                    // Extra info line
                    char info[64] = { 0 };
                    if (IsFood(bt)) {
                        snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue(bt));
                    } else if (IsArmor(bt)) {
                        int armorVal = GetArmorValue(bt);
                        int maxDur = GetArmorMaxDurability(bt);
                        int pct = maxDur > 0 ? player.toolDurability[idx] * 100 / maxDur : 0;
                        snprintf(info, sizeof(info), S(STR_TOOLTIP_ARMOR), armorVal, pct);
                    } else if (IsTool(bt)) {
                        int maxDur = GetToolMaxDurability(bt);
                        if (maxDur > 0) {
                            int pct = player.toolDurability[idx] * 100 / maxDur;
                            snprintf(info, sizeof(info), S(STR_TOOLTIP_DURABILITY), pct);
                        }
                    }
                    int infoW = info[0] ? MeasureGameTextWidth(info, 13) : 0;
                    if (infoW > maxW) maxW = infoW;

                    // Enchantment info
                    uint16_t ench = player.itemEnchantments[idx];
                    char enchBuf[32] = {0};
                    if (ENCH_TYPE(ench) != ENCH_NONE) {
                        snprintf(enchBuf, sizeof(enchBuf), "%s %d",
                            GetEnchantName((EnchantmentType)ENCH_TYPE(ench)),
                            ENCH_LEVEL(ench));
                        int enchW = MeasureGameTextWidth(enchBuf, 12);
                        if (enchW > maxW) maxW = enchW;
                    }

                    // Tooltip background (multi-line)
                    int ttH = 36 + (info[0] ? 16 : 0) + (enchBuf[0] ? 16 : 0);
                    int boxW = maxW + 10, boxH = ttH + 2;
                    int tx, ty;
                    GetUiTooltipPos((int)mouse.x, (int)mouse.y, boxW, boxH, &tx, &ty);
                    DrawRectangle(tx + 1, ty + 1, boxW, boxH, (Color){0, 0, 0, 60});
                    DrawRectangle(tx, ty, boxW, boxH, (Color){16, 0, 16, 235});
                    DrawRectangleLines(tx, ty, boxW, boxH, (Color){80, 0, 255, 170});

                    // Type label (small, colored)
                    DrawGameText(typeLabel, tx + 4, ty + 2, 11, typeColor);
                    // Item name
                    DrawGameText(name, tx + 4, ty + 16, 14, (Color){230, 225, 240, 255});

                    // Info line
                    if (info[0]) {
                        DrawGameText(info, tx + 4, ty + 32, 14, (Color){180, 200, 180, 255});
                    }

                    // Enchantment line
                    if (enchBuf[0]) {
                        int ety = ty + 32 + (info[0] ? 16 : 0);
                        DrawGameText(enchBuf, tx + 4, ety, 14, (Color){180, 120, 255, 255});
                    }
                }
            }
        }
    }

    // Held item follows mouse
    if (heldItem != BLOCK_AIR && heldItem < BLOCK_COUNT && blockAtlas.id > 0) {
        int mx = (int)mouse.x - slotSize / 2;
        int my = (int)mouse.y - slotSize / 2;
        Rectangle src = { (float)(heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(mx + 4), (float)(my + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
        if (heldCount > 1) {
            DrawGameText(TextFormat("%d", heldCount), mx + slotSize - 20, my + slotSize - 16,14, WHITE);
        }
    }

    // If inventory/armor changed inside the normal inventory UI, sync it.
    if (doInvSync) {
        if (memcmp(snapInv,   player.inventory,        sizeof(snapInv))   != 0 ||
            memcmp(snapCount, player.inventoryCount,   sizeof(snapCount)) != 0 ||
            memcmp(snapDur,   player.toolDurability,   sizeof(snapDur))   != 0 ||
            memcmp(snapEnch,  player.itemEnchantments, sizeof(snapEnch))  != 0 ||
            memcmp(snapArmor, player.armor,            sizeof(snapArmor)) != 0 ||
            memcmp(snapArmorDur,  player.armorDurability,  sizeof(snapArmorDur))  != 0 ||
            memcmp(snapArmorEnch, player.armorEnchantments, sizeof(snapArmorEnch)) != 0) {
            if (NetIsClient()) SyncInventoryToHost();
            else if (NetIsHost()) SyncInventoryToAll();
        }
    }
}

//----------------------------------------------------------------------------------
// Creative Mode Inventory Palette
//----------------------------------------------------------------------------------
// Case-insensitive substring match for the palette search box.
static bool NameContains(const char *hay, const char *needle)
{
    if (!needle || !needle[0]) return true;
    if (!hay) return false;
    for (size_t i = 0; hay[i]; i++) {
        size_t j = 0;
        while (needle[j] && hay[i + j]) {
            char a = hay[i + j], b = needle[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a + 32);
            if (b >= 'A' && b <= 'Z') b = (char)(b + 32);
            if (a != b) break;
            j++;
        }
        if (needle[j] == 0) return true;
    }
    return false;
}

void DrawCreativeScreen(void)
{
    static float panelAnim = 0.0f;
    float dt = GetFrameTime();
    float target = creativeOpen ? 1.0f : 0.0f;
    panelAnim += (target - panelAnim) * 25.0f * dt;
    if (panelAnim < 0.01f && !creativeOpen) return;
    if (panelAnim > 0.99f) panelAnim = 1.0f;

    // Palette contents: every breakable block plus the liquid buckets
    static int palette[BLOCK_COUNT + 2];
    static int paletteCount = -1;
    if (paletteCount < 0) {
        paletteCount = 0;
        for (int i = 1; i < BLOCK_COUNT; i++) {
            if (blockInfo[i].breakable) palette[paletteCount++] = i;
        }
        palette[paletteCount++] = ITEM_WATER_BUCKET;
        palette[paletteCount++] = ITEM_LAVA_BUCKET;
    }

    int slotSize = 40, padding = 4, cols = 10, rows = 5;
    int gridW = cols * slotSize + (cols - 1) * padding;
    int gridH = rows * slotSize + (rows - 1) * padding;
    int titleH = 26;
    int searchH = 24;
    int footerH = 26;
    int panelW = gridW + 24;
    int panelH = titleH + searchH + gridH + footerH;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = (SCREEN_HEIGHT - panelH) / 2;
    panelY += (int)((1.0f - panelAnim) * 40.0f);

    Vector2 mouse = Win32GetMousePosition();

    // Dim overlay
    unsigned char ovA = (unsigned char)(90 * panelAnim);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, ovA});

    // Panel frame
    DrawUiPanel(panelX, panelY, panelW, panelH, 255);

    // Title
    const char *title = S(STR_CREATIVE_TITLE);
    int titleW = MeasureGameTextWidth(title, 16);
    DrawGameText(title, panelX + panelW / 2 - titleW / 2, panelY + 7, 14, (Color){62, 62, 62, 255});

    // Backpack button: open the normal 36-slot inventory
    Rectangle backpackBtn = { (float)(panelX + panelW - 82), (float)(panelY + 4), 70.0f, 20.0f };
    bool bpHover = CheckCollisionPointRec(mouse, backpackBtn);
    DrawUiButton(backpackBtn.x, backpackBtn.y, backpackBtn.width, backpackBtn.height,
                 S(STR_CREATIVE_BACKPACK), 13, bpHover, false, true, 1.0f);
    if (bpHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        creativeOpen = false;
        inventoryOpen = true;
        gamePaused = false;
    }

    int gridX = panelX + 12;
    int gridY = panelY + titleH + searchH;

    // Search box
    static char creativeSearch[24] = "";
    static bool creativeSearchFocused = false;
    Rectangle searchBox = { (float)gridX, (float)(panelY + titleH + 3), (float)(panelW - 24), 18.0f };
    bool searchHover = CheckCollisionPointRec(mouse, searchBox);
    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        creativeSearchFocused = CheckCollisionPointRec(mouse, searchBox);
    }
    if (creativeSearchFocused) {
        int ch;
        while ((ch = Win32GetCharPressed()) != 0) {
            if (ch >= 32 && ch < 127 && (int)strlen(creativeSearch) < 23) {
                int len = (int)strlen(creativeSearch);
                creativeSearch[len] = (char)ch;
                creativeSearch[len + 1] = 0;
            }
        }
        if (Win32IsKeyPressed(KEY_BACKSPACE)) {
            int len = (int)strlen(creativeSearch);
            if (len > 0) creativeSearch[len - 1] = 0;
        }
        if (Win32IsKeyPressed(KEY_ESCAPE)) {
            creativeSearchFocused = false;
        }
    }
    // Search field — recessed MC text field matching the crafting panel
    DrawRectangle((int)searchBox.x, (int)searchBox.y, (int)searchBox.width, (int)searchBox.height,
                  (Color){70, 70, 70, 255});
    DrawRectangle((int)searchBox.x, (int)searchBox.y, (int)searchBox.width, 1, (Color){40, 40, 40, 255});
    DrawRectangle((int)searchBox.x, (int)searchBox.y, 1, (int)searchBox.height, (Color){40, 40, 40, 255});
    DrawRectangleLines(searchBox.x, searchBox.y, searchBox.width, searchBox.height,
        creativeSearchFocused ? (Color){255, 255, 255, 255} : (Color){110, 110, 110, 255});
    if (creativeSearch[0]) {
        DrawGameText(creativeSearch, gridX + 6, panelY + titleH + 5, 14, (Color){255, 255, 255, 255});
    } else {
        DrawGameText(S(STR_CREATIVE_SEARCH), gridX + 6, panelY + titleH + 5, 14, (Color){170, 170, 170, 255});
    }

    static int scrollOffset = 0;

    // Filter palette by search text
    static int visIdx[BLOCK_COUNT + 2];
    int visCount = 0;
    {
        bool empty = (creativeSearch[0] == 0);
        for (int pi = 0; pi < paletteCount; pi++) {
            int it = palette[pi];
            if (empty || NameContains(GetBlockName((BlockType)it), creativeSearch)) {
                visIdx[visCount++] = it;
            }
        }
    }
    // Reset scroll when the search text changes
    static char lastSearch[24] = "";
    if (strcmp(lastSearch, creativeSearch) != 0) {
        snprintf(lastSearch, sizeof(lastSearch), "%s", creativeSearch);
        scrollOffset = 0;
    }

    if (visCount == 0) {
        const char *none = S(STR_CREATIVE_NO_MATCH);
        int nw = MeasureGameTextWidth(none, 14);
        DrawGameText(none, panelX + panelW / 2 - nw / 2, gridY + gridH / 2 - 8, 14, (Color){62, 62, 62, 200});
    }

    // Scroll
    int totalRows = (visCount + cols - 1) / cols;
    int maxScroll = totalRows - rows;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    Rectangle gridRect = { (float)gridX, (float)gridY, (float)gridW, (float)gridH };
    if (CheckCollisionPointRec(mouse, gridRect) && !searchHover) {
        int wheel = (int)Win32GetMouseWheelMove();
        if (wheel != 0) {
            scrollOffset -= wheel;
            if (scrollOffset < 0) scrollOffset = 0;
            if (scrollOffset > maxScroll) scrollOffset = maxScroll;
            PlaySoundUIClick();
        }
    }

    int hoveredIdx = -1;
    int selectedItem = (player.selectedSlot >= 0 && player.selectedSlot < INVENTORY_SLOTS)
        ? player.inventory[player.selectedSlot] : BLOCK_AIR;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = (r + scrollOffset) * cols + c;
            if (idx >= visCount) break;
            int item = visIdx[idx];
            int x = gridX + c * (slotSize + padding);
            int y = gridY + r * (slotSize + padding);
            Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
            bool hover = CheckCollisionPointRec(mouse, slotRect);
            bool sel = (item == selectedItem && item != BLOCK_AIR);
            if (hover) hoveredIdx = idx;

            // Slot background (modern dark-flat)
            DrawUiSlot(x, y, slotSize, hover, sel, 1.0f);

            // Item icon
            if (item > 0 && item < BLOCK_COUNT && blockAtlas.id > 0) {
                Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(x + 4), (float)(y + 4), (float)(slotSize - 8), (float)(slotSize - 8) };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);

                bool isTool = IsTool((BlockType)item);
                // MC creative shows no stack counts — just the icon. Keep a
                // durability bar for tools so damaged gear reads correctly.
                if (isTool) {
                    int maxDur = GetToolMaxDurability((BlockType)item);
                    if (maxDur > 0) {
                        int barW = slotSize - 8;
                        int barH = 3;
                        int barX = x + 4;
                        int barY = y + slotSize - 6;
                        DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 120});
                        DrawRectangle(barX, barY, barW, barH, (Color){80, 200, 100, 220});
                        DrawRectangle(barX, barY, barW, 1, (Color){255, 255, 255, 40});
                    }
                }
            }

            // Click: pick into selected hotbar slot (infinite stack)
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int slot = player.selectedSlot;
                player.inventory[slot] = (uint8_t)item;
                player.inventoryCount[slot] = IsTool((BlockType)item) ? 1 : 64;
                player.toolDurability[slot] = GetToolMaxDurability((BlockType)item);
                player.itemEnchantments[slot] = 0;
                PlaySoundUIClick();
                if (NetIsClient()) SyncInventoryToHost();
                else if (NetIsHost()) SyncInventoryToAll();
            }
        }
    }

    // Footer: selected item + hint
    int footerY = gridY + gridH + 7;
    char footerBuf[128];
    const char *footer;
    if (selectedItem != BLOCK_AIR && selectedItem < BLOCK_COUNT) {
        footer = Sf(STR_CREATIVE_SELECTED, GetBlockName((BlockType)selectedItem));
    } else {
        snprintf(footerBuf, sizeof(footerBuf), "%s  (%s)", S(STR_CREATIVE_SELECT_HINT), S(STR_HINT_SCROLL));
        footer = footerBuf;
    }
    int footerW = MeasureGameTextWidth(footer, 13);
    DrawGameText(footer, panelX + panelW / 2 - footerW / 2, footerY, 14, (Color){78, 78, 78, 240});

    // Scroll indicator
    if (maxScroll > 0) {
        int indX = panelX + panelW - 10;
        int indH = gridH;
        int indY = gridY;
        float frac = (float)scrollOffset / maxScroll;
        int knobH = indH / (maxScroll + 1);
        if (knobH < 6) knobH = 6;
        int knobY = indY + (int)((indH - knobH) * frac);
        DrawRectangle(indX, indY, 4, indH, (Color){36, 36, 36, 160});
        DrawRectangle(indX, knobY, 4, knobH, (Color){200, 200, 200, 235});
    }

    // Tooltip
    if (hoveredIdx >= 0) {
        int item = visIdx[hoveredIdx];
        const char *name = GetBlockName((BlockType)item);
        int tw = MeasureGameTextWidth(name, 14) + 12;
        int tx, ty;
        GetUiTooltipPos((int)mouse.x, (int)mouse.y, tw, 18, &tx, &ty);
        DrawRectangle(tx + 2, ty + 2, tw, 18, (Color){0, 0, 0, 50});
        DrawRectangle(tx, ty, tw, 18, (Color){16, 0, 16, 235});
        DrawRectangleLines(tx, ty, tw, 18, (Color){80, 0, 255, 170});
        DrawGameText(name, tx + 6, ty + 3, 14, (Color){255, 255, 255, 255});
    }
}


//----------------------------------------------------------------------------------
// World Rendering (hash table iteration)
//----------------------------------------------------------------------------------
void DrawWorld(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (loadedChunks[i].chunkX < minCX || loadedChunks[i].chunkX > maxCX) continue;
        if (!loadedChunks[i].textureValid) continue;
        float x = (float)(loadedChunks[i].chunkX * CHUNK_SIZE * BLOCK_SIZE);
        DrawTexture(loadedChunks[i].texture, (int)x, 0, WHITE);
    }

    // World edge indicators
    int worldPixelW = WORLD_WIDTH * BLOCK_SIZE;
    int edgeW = 4;
    float viewTop = camera.target.y - (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    if (viewLeft < edgeW) {
        DrawRectangle(0, (int)viewTop, edgeW, (int)(viewBottom - viewTop), (Color){180, 40, 40, 150});
    }
    if (viewRight > worldPixelW - edgeW) {
        DrawRectangle(worldPixelW - edgeW, (int)viewTop, edgeW, (int)(viewBottom - viewTop), (Color){180, 40, 40, 150});
    }
}

// Fire glow and particle effects for torches, lanterns, lava
static float fireTimer = 0.0f;
void DrawFireEffects(float dt)
{
    fireTimer += dt;
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewTop = camera.target.y - (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;

    int minBX = (int)(viewLeft / BLOCK_SIZE) - 1;
    int maxBX = (int)(viewRight / BLOCK_SIZE) + 1;
    int minBY = (int)(viewTop / BLOCK_SIZE) - 1;
    int maxBY = (int)(viewBottom / BLOCK_SIZE) + 1;
    if (minBX < 0) minBX = 0;
    if (maxBX >= WORLD_WIDTH) maxBX = WORLD_WIDTH - 1;
    if (minBY < 0) minBY = 0;
    if (maxBY >= WORLD_HEIGHT) maxBY = WORLD_HEIGHT - 1;

    float time = (float)GetTime();

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            uint8_t block = GetBlock(bx, by);
            bool isFire = (block == BLOCK_TORCH || block == BLOCK_LANTERN);
            bool isLava = (block == BLOCK_LAVA);
            if (!isFire && !isLava) continue;

            float cx = bx * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
            float cy = by * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
            uint8_t light = GetLightLevel(bx, by);
            if (light < 4) continue;

            // Glow halo
            float glowRadius = isLava ? 20.0f : 30.0f;
            float flicker = 0.8f + sinf(time * 8.0f + bx * 3.7f + by * 5.3f) * 0.15f
                                   + sinf(time * 13.0f + bx * 7.1f) * 0.05f;
            unsigned char glowA = (unsigned char)(25 * flicker * (light / 15.0f));
            Color glowColor = isLava
                ? (Color){255, 120, 20, glowA}
                : (Color){255, 180, 50, glowA};
            // Multi-layer glow
            for (int r = 3; r > 0; r--) {
                float radius = glowRadius * r / 3.0f;
                unsigned char a = glowA / r;
                Color gc = {glowColor.r, glowColor.g, glowColor.b, a};
                DrawCircleV((Vector2){cx, cy}, radius, gc);
            }

            // Spawn fire particles (throttled)
            if (isFire && fireTimer > 0.08f) {
                SpawnFireParticle(cx, cy - 4);
            }
            if (isLava && fireTimer > 0.2f) {
                SpawnFireParticle(cx + (float)(rand() % 12 - 6), cy - 2);
            }
        }
    }
    if (fireTimer > 0.2f) fireTimer = 0.0f;
}

void DrawMiningCrack(void)
{
    float progress = GetMiningProgress();
    int bx = GetMiningBlockX();
    int by = GetMiningBlockY();
    if (progress <= 0.0f || bx < 0 || by < 0) return;

    int stage = (int)(progress * (CRACK_STAGES - 1));
    if (stage >= CRACK_STAGES) stage = CRACK_STAGES - 1;
    if (stage < 0) return;

    // Draw crack overlay on the block
    Rectangle src = { 0, 0, BLOCK_SIZE, BLOCK_SIZE };
    Rectangle dst = { (float)(bx * BLOCK_SIZE), (float)(by * BLOCK_SIZE), BLOCK_SIZE, BLOCK_SIZE };
    DrawTexturePro(crackTextures[stage], src, dst, (Vector2){0, 0}, 0, WHITE);
}

//----------------------------------------------------------------------------------
// Water Rendering (uses cached waterTopY per chunk)
//----------------------------------------------------------------------------------
void DrawWater(void)
{
    float viewLeft = camera.target.x - (SCREEN_WIDTH / 2.0f) / camera.zoom;
    float viewRight = camera.target.x + (SCREEN_WIDTH / 2.0f) / camera.zoom;

    int minCX = (int)(viewLeft) / (CHUNK_SIZE * BLOCK_SIZE) - 1;
    int maxCX = (int)(viewRight) / (CHUNK_SIZE * BLOCK_SIZE) + 1;

    float viewBottom = camera.target.y + (SCREEN_HEIGHT / 2.0f) / camera.zoom;
    int maxBY = (int)(viewBottom / BLOCK_SIZE) + 1;
    if (maxBY >= WORLD_HEIGHT) maxBY = WORLD_HEIGHT - 1;

    float time = (float)GetTime();

    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX == CHUNK_EMPTY) continue;
        if (loadedChunks[i].chunkX < minCX || loadedChunks[i].chunkX > maxCX) continue;

        Chunk *c = &loadedChunks[i];
        int startX = c->chunkX * CHUNK_SIZE;

        for (int bx = 0; bx < CHUNK_SIZE; bx++) {
            int wx = startX + bx;
            if (wx < 0 || wx >= WORLD_WIDTH) continue;
            int waterTop = c->waterTopY[bx];
            if (waterTop < 0) continue;

            for (int by = waterTop; by <= maxBY; by++) {
                if (GetBlock(wx, by) != BLOCK_WATER) break;
                float wave = sinf(wx * 0.5f + time * 2.0f) * 1.5f;
                Color wc = ApplyLighting(blockInfo[BLOCK_WATER].baseColor, wx, by);
                DrawRectangle(wx * BLOCK_SIZE, (int)(by * BLOCK_SIZE + wave), BLOCK_SIZE, BLOCK_SIZE, wc);
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Remote Players (multiplayer)
//----------------------------------------------------------------------------------
static void DrawLimb(int jointX, int jointY, int w, int h, float angleDeg, Color color);
void DrawRemotePlayers(void)
{
    if (!NetIsConnected()) return;

    float dt = GetFrameTime();
    float lerpSpeed = 12.0f; // Smooth interpolation speed

    for (int i = 0; i < MAX_NET_PLAYERS; i++) {
        if (i == localPlayerId) continue;
        if (!remotePlayers[i].active) continue;
        if (players[i].health <= 0) continue;

        // Smooth interpolation toward target position
        float targetX = players[i].position.x;
        float targetY = players[i].position.y;
        remotePlayers[i].interpX += (targetX - remotePlayers[i].interpX) * lerpSpeed * dt;
        remotePlayers[i].interpY += (targetY - remotePlayers[i].interpY) * lerpSpeed * dt;
        float px = remotePlayers[i].interpX;
        float py = remotePlayers[i].interpY;

        // Per-player animation — simple linear swing
        float walkT = players[i].walkTimer;
        bool moving = fabsf(players[i].velocity.x) > 10.0f;
        bool facing = players[i].facingRight;
        bool sprinting = players[i].sprinting && moving;
        float armDeg = moving ? sinf(walkT) * (sprinting ? 40.0f : 30.0f) : 4.0f;
        float legDeg = moving ? sinf(walkT + 3.141592653f) * (sprinting ? 32.0f : 24.0f) : 0.0f;
        float dirSign = facing ? 1.0f : -1.0f;
        float sneakShrink = players[i].sneaking ? 4.0f : 0;
        float bobY = py - sneakShrink;

        // Different shirt colors per player
        Color skin = (Color){220, 180, 140, 255};
        Color hair = (Color){80, 50, 30, 255};
        Color shirt, pants;
        switch (i) {
            case 1: shirt = (Color){200, 50, 50, 255}; pants = (Color){40, 40, 60, 255}; break;
            case 2: shirt = (Color){50, 200, 50, 255}; pants = (Color){40, 60, 40, 255}; break;
            case 3: shirt = (Color){200, 200, 50, 255}; pants = (Color){60, 60, 40, 255}; break;
            default: shirt = (Color){0, 100, 200, 255}; pants = (Color){60, 40, 20, 255}; break;
        }

        // Damage flash
        if (players[i].damageFlashTimer > 0.0f) {
            skin = (Color){255, 150, 150, 255};
            shirt = (Color){100, 50, 50, 255};
        }

        float centerX = px + PLAYER_WIDTH / 2.0f;
        float footY = bobY + PLAYER_HEIGHT;

        // Shadow
        DrawEllipse((int)centerX, (int)footY, 8, 3, (Color){0, 0, 0, 50});

        // Legs — linear swing
        DrawLimb((int)centerX - 2, (int)(bobY + 16), 4, 12, dirSign * legDeg, pants);
        DrawLimb((int)centerX + 3, (int)(bobY + 16), 4, 12, dirSign * (-legDeg), pants);

        // Body
        DrawRectangle((int)(centerX - 5), (int)(bobY + 10), 11, 14, shirt);

        // Arms — linear swing
        DrawLimb((int)centerX - 5, (int)(bobY + 11), 3, 11, dirSign * (-armDeg), skin);
        DrawLimb((int)centerX + 7, (int)(bobY + 11), 3, 11, dirSign * armDeg, skin);

        // Head
        DrawRectangle((int)(centerX - 4), (int)(bobY + 2), 9, 8, skin);
        // Hair
        DrawRectangle((int)(centerX - 4), (int)(bobY + 2), 9, 3, hair);
        // Eyes
        int eyeX = facing ? (int)(centerX + 1) : (int)(centerX - 3);
        DrawRectangle(eyeX, (int)(bobY + 6), 2, 2, (Color){40, 40, 40, 255});

        // Name tag
        const char *nameTag = players[i].playerName[0] ? players[i].playerName : "Player";
        int nameW = MeasureGameTextWidth(nameTag, 12);
        DrawGameText(nameTag, (int)(centerX - nameW / 2), (int)(bobY - 6), 14,
                     (Color){255, 255, 255, 200});
    }
}

//----------------------------------------------------------------------------------
// Limb drawing — limbs hang from a joint and rotate around it.
//
// A limb is drawn as a rectangle whose pivot (origin) sits at the top-centre, so
// rotating it swings the far end like a real arm or leg instead of sliding the
// whole rectangle up and down. angleDeg is measured from straight-down: positive
// swings the free end forward (the direction the character faces).
//
// The body is mirrored when facing left, which flips the visual direction of a
// rotation, so callers pass the already-signed angle and this just draws.
//----------------------------------------------------------------------------------
static void DrawLimb(int jointX, int jointY, int w, int h, float angleDeg, Color color)
{
    Rectangle rec = { (float)jointX - w * 0.5f, (float)jointY, (float)w, (float)h };
    Vector2 origin = { w * 0.5f, 0.0f };   // pivot at the joint (top centre)
    DrawRectanglePro(rec, origin, angleDeg, color);
}

//----------------------------------------------------------------------------------
// Player Sprite
//----------------------------------------------------------------------------------
void DrawPlayerSprite(void)
{
    float px = player.position.x;
    float py = player.position.y;
    float walkT = player.walkTimer;
    bool moving = fabsf(player.velocity.x) > 10.0f;
    bool facing = player.facingRight;
    bool sprinting = player.sprinting && moving;
    bool airborne = !player.onGround && !player.flying;

    // --- Animation drivers -------------------------------------------------
    // Walk cycle: forward arm goes UP, back arm goes DOWN (2D side view)

    // Idle breathing: gentle torso rise/fall when standing still on the ground
    float breath = (!moving && player.onGround) ? sinf((float)GetTime() * 2.0f) * 0.8f : 0.0f;

    // Jump/fall pose: rising stretches the body and lifts the arm; falling
    // compresses slightly. velN is -1 (rising) .. +1 (falling fast).
    float velN = 0.0f;
    if (airborne) {
        velN = player.velocity.y / 400.0f;
        if (velN > 1.0f) velN = 1.0f;
        if (velN < -1.0f) velN = -1.0f;
    }

    // Landing squash: compress the torso briefly on impact.
    float landSquash = 0.0f;
    if (player.landSquashTimer > 0.0f) {
        float t = player.landSquashTimer / 0.20f; // 1 → 0
        if (t > 1.0f) t = 1.0f;
        landSquash = t * 3.0f;
    }

    // Attack swing: 0 → 1 over the cooldown, peaks mid-swing.

    // Total vertical offset: crouch + breathing + landing squash
    float sneakShrink = player.sneaking ? 4.0f : 0;
    float bobY = py - sneakShrink + breath + landSquash;

    // Held item wobble + rotation
    float itemAngle = moving ? sinf(walkT) * (sprinting ? 10.0f : 8.0f) : 0;

    // Sprint dust is spawned by PlayerPhysics, not here (avoid duplicates)

    // Damage flash tint
    Color skin = (Color){220, 180, 140, 255};
    Color hair = (Color){80, 50, 30, 255};
    Color shirt = (Color){0, 100, 200, 255};
    Color pants = (Color){60, 40, 20, 255};
    if (player.damageFlashTimer > 0.0f) {
        skin = (Color){255, 150, 150, 255};
        shirt = (Color){100, 50, 50, 255};
    }

    // Armor colors based on tier
    Color helmetColor = {0, 0, 0, 0}, chestColor = {0, 0, 0, 0}, legColor = {0, 0, 0, 0}, bootColor = {0, 0, 0, 0};
    if (player.armor[0] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[0];
        if (a <= ARMOR_WOOD_BOOTS) helmetColor = (Color){160, 120, 60, 255};
        else if (a <= ARMOR_STONE_BOOTS) helmetColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) helmetColor = (Color){200, 210, 220, 255};
        else if (a <= ARMOR_GOLD_BOOTS) helmetColor = (Color){220, 180, 50, 255};
        else helmetColor = (Color){80, 220, 230, 255};
    }
    if (player.armor[1] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[1];
        if (a <= ARMOR_WOOD_BOOTS) chestColor = (Color){160, 120, 60, 255};
        else if (a <= ARMOR_STONE_BOOTS) chestColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) chestColor = (Color){200, 210, 220, 255};
        else if (a <= ARMOR_GOLD_BOOTS) chestColor = (Color){220, 180, 50, 255};
        else chestColor = (Color){80, 220, 230, 255};
    }
    if (player.armor[2] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[2];
        if (a <= ARMOR_WOOD_BOOTS) legColor = (Color){160, 120, 60, 255};
        else if (a <= ARMOR_STONE_BOOTS) legColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) legColor = (Color){200, 210, 220, 255};
        else if (a <= ARMOR_GOLD_BOOTS) legColor = (Color){220, 180, 50, 255};
        else legColor = (Color){80, 220, 230, 255};
    }
    if (player.armor[3] != BLOCK_AIR) {
        BlockType a = (BlockType)player.armor[3];
        if (a <= ARMOR_WOOD_BOOTS) bootColor = (Color){160, 120, 60, 255};
        else if (a <= ARMOR_STONE_BOOTS) bootColor = (Color){140, 140, 140, 255};
        else if (a <= ARMOR_IRON_BOOTS) bootColor = (Color){200, 210, 220, 255};
        else if (a <= ARMOR_GOLD_BOOTS) bootColor = (Color){220, 180, 50, 255};
        else bootColor = (Color){80, 220, 230, 255};
    }

    // Single body draw, mirrored horizontally when facing left. `mx` maps a
    // right-facing local X (relative to px) to the mirrored position.
    #define MX(off, w) (int)(facing ? (px + (off)) : (px + (12 - (off) - (w))))

    // Head + hair
    DrawRectangle(MX(2, 8), (int)bobY, 8, 8, skin);
    DrawRectangle(MX(2, 8), (int)bobY, 8, 3, hair);
    if (helmetColor.a > 0) {
        DrawRectangle(MX(1, 10), (int)(bobY - 1), 10, 5, helmetColor);
        DrawRectangle(MX(2, 8), (int)(bobY + 4), 8, 2, helmetColor);
    }
    // Eyes (shift slightly inward when facing)
    DrawRectangle(MX(3, 2), (int)(bobY + 4), 2, 2, (Color){40, 40, 40, 255});
    DrawRectangle(MX(7, 2), (int)(bobY + 4), 2, 2, (Color){40, 40, 40, 255});

    // Torso
    DrawRectangle(MX(1, 10), (int)(bobY + 7), 10, 10, shirt);
    if (chestColor.a > 0) {
        DrawRectangle(MX(0, 12), (int)(bobY + 6), 12, 11, chestColor);
        DrawRectangle(MX(1, 10), (int)(bobY + 7), 10, 9, (Color){
            (unsigned char)(chestColor.r * 0.8f), (unsigned char)(chestColor.g * 0.8f), (unsigned char)(chestColor.b * 0.8f), 255
        });
    }

    // Arms hang from the shoulder and swing around it. The free end travels in an
    // arc, which is what makes a walk cycle read as a walk rather than the whole
    // arm sliding up and down.
    //   walk phase : ±26° (sprinting widens it)
    //   airborne   : arms trail up while rising, back a little while falling
    //   attack     : the front arm drives forward through the swing
    {
        float swingDeg = moving ? sinf(walkT) * (sprinting ? 40.0f : 30.0f) : 0.0f;
        if (!moving && !airborne) {
            // Idle: arms rest almost straight down with a touch of outward drift
            swingDeg = 4.0f;
        }
        float airDeg = airborne ? (player.velocity.y < 0.0f ? -55.0f : 18.0f) : 0.0f;
        float attackDeg = 0.0f;
        if (player.attackCooldown > 0.0f && player.attackAnim > 0.0f) {
            float p = 1.0f - (player.attackCooldown / player.attackAnim);
            if (p < 0.0f) p = 0.0f;
            if (p > 1.0f) p = 1.0f;
            attackDeg = sinf(p * 3.141592653f) * 95.0f;   // big forward arc
        }
        // Sign flips with facing so "forward" always means the way we look.
        float dir = facing ? 1.0f : -1.0f;
        int shoulderY = (int)(bobY + 8);

        // Back arm leads the walk cycle in the opposite phase
        int backJointX = MX(-2, 3) + 2;
        DrawLimb(backJointX, shoulderY, 3, 11, dir * (-swingDeg), skin);
        // Front arm carries the swing/air/attack motion
        int frontJointX = MX(11, 3) + 2;
        DrawLimb(frontJointX, shoulderY, 3, 11, dir * (swingDeg + airDeg + attackDeg), skin);
    }

    // Legs pivot at the hip, opposite phase to the arms. In the air they tuck.
    {
        float legDeg = moving ? sinf(walkT + 3.141592653f) * (sprinting ? 32.0f : 24.0f) : 0.0f;
        if (airborne) legDeg = 14.0f;
        float dir = facing ? 1.0f : -1.0f;
        int hipY = (int)(bobY + 17);

        int backHipX = MX(1, 4) + 2;
        int frontHipX = MX(7, 4) + 2;
        DrawLimb(backHipX, hipY, 4, 11, dir * legDeg, pants);
        DrawLimb(frontHipX, hipY, 4, 11, dir * (-legDeg), pants);
        // Armor overlays follow the same pivot so the greaves stay on the leg
        if (legColor.a > 0) {
            DrawLimb(backHipX, hipY, 5, 11, dir * legDeg, legColor);
            DrawLimb(frontHipX, hipY, 5, 11, dir * (-legDeg), legColor);
        }
        if (bootColor.a > 0) {
            // Boots sit at the far end of the leg; rotating the leg carries them
            for (int k = 0; k < 2; k++) {
                float a = dir * (k == 0 ? legDeg : -legDeg);
                int jx = (k == 0) ? backHipX : frontHipX;
                float rad = a * 3.141592653f / 180.0f;
                int bx2 = jx + (int)(sinf(rad) * 11.0f);
                int by2 = hipY + (int)(cosf(rad) * 11.0f);
                DrawRectangle(bx2 - 2, by2 - 3, 5, 3, bootColor);
            }
        }
    }
    #undef MX

    // Draw held item - hangs off the front hand, so it inherits the arm's angle
    // and rotates with it instead of sliding alongside.
    int slotItem = player.inventory[player.selectedSlot];
    if (slotItem != BLOCK_AIR && slotItem < BLOCK_COUNT && blockAtlas.id > 0) {
        int itemSize = 13;
        // Same angle the front arm used above, recomputed so the item tracks it
        float swingDeg = moving ? sinf(walkT) * (sprinting ? 40.0f : 30.0f) : 0.0f;
        if (!moving && !airborne) swingDeg = 4.0f;
        float airDeg = airborne ? (player.velocity.y < 0.0f ? -55.0f : 18.0f) : 0.0f;
        float attackDeg = 0.0f;
        if (player.attackCooldown > 0.0f && player.attackAnim > 0.0f) {
            float p = 1.0f - (player.attackCooldown / player.attackAnim);
            if (p < 0.0f) p = 0.0f;
            if (p > 1.0f) p = 1.0f;
            attackDeg = sinf(p * 3.141592653f) * 95.0f;
        }
        float armDeg = (facing ? 1.0f : -1.0f) * (swingDeg + airDeg + attackDeg);

        // Hand sits at the rotated far end of the arm
        float rad = armDeg * 3.141592653f / 180.0f;
        float shoulderYf = bobY + 8;
        float shoulderX = px + (facing ? 13.0f : 0.0f);   // matches frontJointX = MX(11,3)+2
        float hx = shoulderX + sinf(rad) * 14.0f;   // a little past the hand so the block reads as held
        float hy = shoulderYf + cosf(rad) * 13.0f;
        float rot = itemAngle + armDeg * 0.6f;
        Rectangle src = { (float)(slotItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { hx, hy, (float)itemSize, (float)itemSize };
        Vector2 origin = { itemSize * 0.5f, itemSize * 0.5f };
        DrawTexturePro(blockAtlas, src, dst, origin, rot, WHITE);
    }
}
//----------------------------------------------------------------------------------
// Hotbar
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
// Active status-effect readout, right of the hotbar. MC shows icons top-right;
// here the right edge of the hotbar is free and keeps the info near survival UI.
// Each entry: colour chip + abbreviation + seconds left.
//----------------------------------------------------------------------------------
void DrawActiveEffects(void)
{
    int x = (SCREEN_WIDTH + HOTBAR_SLOTS * HUD_SLOT_SIZE + (HOTBAR_SLOTS - 1) * HUD_SLOT_PAD) / 2 + 12;
    int y = HUD_HOTBAR_Y + 2;
    int shown = 0;
    for (int i = 0; i < MAX_PLAYER_EFFECTS; i++) {
        if (player.effects[i].time <= 0.0f) continue;
        EffectType type = (EffectType)player.effects[i].type;
        Color chip;
        const char *label;
        switch (type) {
            case EFFECT_SPEED:           chip = (Color){120, 190, 250, 255}; label = "SPD"; break;
            case EFFECT_STRENGTH:        chip = (Color){215, 100,  90, 255}; label = "STR"; break;
            case EFFECT_REGEN:           chip = (Color){235, 130, 170, 255}; label = "REG"; break;
            case EFFECT_FIRE_RESISTANCE: chip = (Color){240, 160,  60, 255}; label = "FIR"; break;
            case EFFECT_WATER_BREATHING: chip = (Color){ 80, 170, 220, 255}; label = "WTR"; break;
            case EFFECT_POISON:          chip = (Color){110, 180,  70, 255}; label = "PSN"; break;
            default:                     chip = (Color){160, 160, 160, 255}; label = "?";   break;
        }
        DrawRectangle(x, y, 4, 14, chip);
        char buf[24];
        if (player.effects[i].level > 1)
            snprintf(buf, sizeof(buf), "%s %d %ds", label, player.effects[i].level, (int)(player.effects[i].time + 0.9f));
        else
            snprintf(buf, sizeof(buf), "%s %ds", label, (int)(player.effects[i].time + 0.9f));
        DrawGameText(buf, x + 8, y + 2, 14, (Color){230, 230, 230, 255});
        y += 18;
        shown++;
        if (shown >= 6) break;   // keep the column on screen
    }
}


void DrawHotbar(void)
{
    int slotSize = HUD_SLOT_SIZE;
    int padding = HUD_SLOT_PAD;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    int startY = HUD_HOTBAR_Y;

    // --- Boss bar (Abyss Warden): MC dragon-style bar above the status band ---
    // Scanned (not via bossMobIndex) so it also works on network clients whose
    // mob list is a host-replicated mirror.
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active || mobs[i].type != MOB_ABYSS_WARDEN) continue;
        const Mob *boss = &mobs[i];
        int barW = totalW;
        int barH = 8;
        int barX = startX;
        int barY = HUD_BOSS_BAR_Y;
        float pct = (boss->maxHealth > 0) ? (float)boss->health / boss->maxHealth : 0.0f;
        if (pct < 0.0f) pct = 0.0f;
        if (pct > 1.0f) pct = 1.0f;
        // Boss name centered above the bar, hard black shadow like MC
        const char *bossName = S(STR_MOB_ABYSS_WARDEN);
        int nameW = MeasureGameTextWidth(bossName, 14);
        DrawGameText(bossName, barX + barW / 2 - nameW / 2 + 1, barY - 15, 14, (Color){0, 0, 0, 210});
        DrawGameText(bossName, barX + barW / 2 - nameW / 2, barY - 16, 14, (Color){205, 140, 255, 255});
        // Backing and purple fill with a bright top edge
        DrawRectangle(barX - 1, barY - 1, barW + 2, barH + 2, (Color){0, 0, 0, 160});
        DrawRectangle(barX, barY, barW, barH, (Color){25, 18, 35, 230});
        int fillW = (int)(barW * pct);
        if (fillW > 0) {
            DrawRectangle(barX, barY, fillW, barH, (Color){170, 60, 220, 255});
            DrawRectangle(barX, barY, fillW, 2, (Color){215, 125, 255, 220});
        }
        break;
    }

    // --- XP Bar above hotbar (the single XP bar; status bar no longer draws one) ---
    int xpBarW = totalW;
    int xpBarH = HUD_XP_BAR_H;
    int xpBarX = startX;
    int xpBarY = HUD_XP_BAR_Y;
    float xpPct = (float)player.xp / MAX_XP;
    if (xpPct > 1.0f) xpPct = 1.0f;
    DrawRectangle(xpBarX - 1, xpBarY - 1, xpBarW + 2, xpBarH + 2, (Color){0, 0, 0, 140});
    DrawRectangle(xpBarX, xpBarY, xpBarW, xpBarH, (Color){20, 20, 20, 230});
    // Flat green XP fill (MC experience bar), with a bright top line.
    int fillW = (int)(xpBarW * xpPct);
    if (fillW > 0) {
        DrawRectangle(xpBarX, xpBarY, fillW, xpBarH, (Color){80, 200, 60, 255});
        DrawRectangle(xpBarX, xpBarY, fillW, 1, (Color){150, 255, 130, 220});
    }
    // XP level number, to the left of the bar (keeps the band above the bar free)
    if (player.xp > 0) {
        const char *lvlStr = TextFormat("%d", player.xp / 10);
        int lvlW = MeasureGameTextWidth(lvlStr, 11);
        DrawGameText(lvlStr, xpBarX - lvlW - 6, xpBarY - 3, 14, (Color){120, 235, 130, 230});
    }
    // XP text
    char xpText[32];
    snprintf(xpText, sizeof(xpText), "%d XP", player.xp);
    int xpTextW = MeasureGameTextWidth(xpText, 10);

    // Inventory count on left, XP on right — both below the bar
    int labelY = xpBarY + xpBarH + 2;
    {
        int cnt = 0;
        for (int s = 0; s < INVENTORY_SLOTS; s++) {
            if (player.inventory[s] != BLOCK_AIR) cnt++;
        }
        char invCount[24];
        snprintf(invCount, sizeof(invCount), "%d/%d", cnt, INVENTORY_SLOTS);
        DrawGameText(invCount, xpBarX, labelY, 14, (Color){140, 135, 160, 170});
    }
    DrawGameText(xpText, xpBarX + xpBarW - xpTextW, labelY, 14, (Color){170, 220, 170, 190});

    Vector2 mouse = Win32GetMousePosition();
    int hoveredSlot = -1;

    // Slot selection bounce animation
    static float slotBounce = 0.0f;
    static int lastSlot = -1;
    if (lastSlot != player.selectedSlot) { slotBounce = 1.0f; lastSlot = player.selectedSlot; }
    slotBounce *= powf(0.05f, GetFrameTime());
    if (slotBounce < 0.01f) slotBounce = 0.0f;

    // MC hotbar backing: hard dark strip (no transparency/rounding)
    // MC hotbar: dark translucent strip with a light gray frame
    DrawRectangle(startX - 6, startY - 6, totalW + 12, slotSize + 12, (Color){0, 0, 0, 150});
    DrawRectangleLines(startX - 6, startY - 6, totalW + 12, slotSize + 12, (Color){160, 160, 160, 220});

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = startX + i * (slotSize + padding);
        int y = startY;
        Rectangle slotRect = { (float)x, (float)y, (float)slotSize, (float)slotSize };
        bool hover = CheckCollisionPointRec(mouse, slotRect);
        bool selected = (i == player.selectedSlot);

        if (hover) hoveredSlot = i;

        int drawY = selected ? y - (int)(slotBounce * 4.0f) : y;

        // Modern dark-flat slot
        DrawUiSlot(x, drawY, slotSize, hover, selected, 1.0f);

        int item = player.inventory[i];
        if (item != BLOCK_AIR && item < BLOCK_COUNT && blockAtlas.id > 0) {
            Rectangle src = { (float)(item * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { (float)(x + 5), (float)(drawY + 5), (float)(slotSize - 10), (float)(slotSize - 10) };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            if (ENCH_TYPE(player.itemEnchantments[i]) != ENCH_NONE)
                DrawEnchantGlint(x + 5, drawY + 5, slotSize - 10);

            if (player.inventoryCount[i] > 1) {
                // MC style: white count with a hard black shadow, bottom-right
                const char *countStr = TextFormat("%d", player.inventoryCount[i]);
                int ctw = MeasureGameTextWidth(countStr, 12);
                int badgeX = x + slotSize - ctw - 4;
                int badgeY = drawY + slotSize - 15;
                DrawGameText(countStr, badgeX + 1, badgeY + 1, 14, (Color){0, 0, 0, 220});
                DrawGameText(countStr, badgeX, badgeY, 14, (Color){255, 255, 255, 255});
            }

            if (IsTool((BlockType)item)) {
                int maxDur = GetToolMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    float pct = (float)player.toolDurability[i] / maxDur;
                    int barW = slotSize - 10;
                    int barH = 3;
                    int barX = x + 5;
                    int barY = drawY + slotSize - 6;
                    // Smooth color gradient: green → yellow → orange → red
                    Color barColor;
                    if (pct > 0.6f) barColor = (Color){80, 200, 100, 220};
                    else if (pct > 0.3f) barColor = (Color){220, 180, 60, 220};
                    else barColor = (Color){220, 80, 60, 220};
                    DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 120});
                    DrawRectangle(barX, barY, (int)(barW * pct), barH, barColor);
                    // Highlight on top of bar
                    DrawRectangle(barX, barY, (int)(barW * pct), 1, (Color){255, 255, 255, 40});
                }
            }
        }

        // Slot number — subtle, top-left (white with MC hard shadow)
        DrawTextMC(TextFormat("%d", i + 1), x + 3, drawY + 2, 11,
                   selected ? (Color){255, 255, 255, 255} : (Color){225, 225, 225, 215});
    }

    // Smooth sliding selection indicator bar
    {
        static float selBarX = -1;
        if (selBarX < 0) selBarX = (float)(startX + player.selectedSlot * (slotSize + padding));
        float targetX = (float)(startX + player.selectedSlot * (slotSize + padding));
        selBarX += (targetX - selBarX) * 12.0f * GetFrameTime();
        int barH = 3;
        int barY = startY + slotSize + 2;
        int bx = (int)selBarX;
        DrawRectangle(bx, barY + 1, slotSize, barH, (Color){0, 0, 0, 120});
        // Flat white selection marker (MC selected-slot frame)
        DrawRectangle(bx, barY, slotSize, barH, (Color){235, 235, 235, 255});
    }

    // Tooltip for hovered slot
    if (hoveredSlot >= 0) {
        int item = player.inventory[hoveredSlot];
        if (item != BLOCK_AIR && item < BLOCK_COUNT) {
            const char *name = GetBlockName(item);
            int tw = MeasureGameTextWidth(name,14) + 12;
            int tx, ty;
            GetUiTooltipPos((int)mouse.x, (int)mouse.y, tw, 18, &tx, &ty);
            // Tooltip shadow
            DrawRectangle(tx + 2, ty + 2, tw, 18, (Color){0, 0, 0, 50});
            // Tooltip background
            DrawRectangle(tx, ty, tw, 18, (Color){16, 0, 16, 235});
            DrawRectangleLines(tx, ty, tw, 18, (Color){80, 0, 255, 170});
            DrawGameText(name, tx + 6, ty + 3,14, (Color){255, 255, 255, 255});

            // Extra info for tools/food
            char info[32] = { 0 };
            if (IsFood((BlockType)item)) {
                snprintf(info, sizeof(info), S(STR_TOOLTIP_HUNGER), GetFoodValue((BlockType)item));
            } else if (IsTool((BlockType)item)) {
                int maxDur = GetToolMaxDurability((BlockType)item);
                if (maxDur > 0) {
                    snprintf(info, sizeof(info), "%d/%d", player.toolDurability[hoveredSlot], maxDur);
                }
            }
            if (info[0]) {
                int iw = MeasureGameTextWidth(info,13) + 12;
                if (iw > tw) tw = iw;
                ty += 18;
                DrawRectangle(tx + 2, ty + 2, tw, 15, (Color){0, 0, 0, 50});
                DrawRectangle(tx, ty, tw, 15, (Color){16, 0, 16, 235});
                DrawRectangleLines(tx, ty, tw, 15, (Color){80, 0, 255, 170});
                DrawGameText(info, tx + 6, ty + 2,14, (Color){180, 200, 180, 255});
            }
        }
    }

    // Selected item name (always visible above hotbar center)
    int selItem = player.inventory[player.selectedSlot];
    if (selItem != BLOCK_AIR && selItem < BLOCK_COUNT) {
        const char *selName = GetBlockName((BlockType)selItem);
        uint16_t selEnch = player.itemEnchantments[player.selectedSlot];
        bool hasEnchant = ENCH_TYPE(selEnch) != ENCH_NONE;
        char enchBuf[32] = {0};
        int enchW = 0;
        if (hasEnchant) {
            int etype = ENCH_TYPE(selEnch);
            int elvl = ENCH_LEVEL(selEnch);
            snprintf(enchBuf, sizeof(enchBuf), "%s %d", GetEnchantName((EnchantmentType)etype), elvl);
            enchW = MeasureGameTextWidth(enchBuf, 11);
        }
        int selW = MeasureGameTextWidth(selName, 13);
        int maxW = (enchW > selW) ? enchW : selW;
        int selX = (SCREEN_WIDTH - maxW) / 2;
        int selY = HUD_SEL_NAME_Y;
        int h = hasEnchant ? 32 : 16;
        DrawRectangle(selX - 4, selY - 2, maxW + 8, h, (Color){0, 0, 0, 170});
        DrawGameText(selName, selX, selY, 14, (Color){255, 255, 255, 230});
        if (hasEnchant) {
            DrawGameText(enchBuf, selX, selY + 16, 14, (Color){180, 120, 255, 220});
        }
    }
}

//----------------------------------------------------------------------------------
// Player Status Bars (armor, health, hunger, oxygen)
// The XP bar lives with the hotbar (see DrawHotbar) so the two never overlap.
//----------------------------------------------------------------------------------
void DrawPlayerStatus(void)
{
    int slotSize = HUD_SLOT_SIZE;
    int padding = HUD_SLOT_PAD;
    int totalW = HOTBAR_SLOTS * slotSize + (HOTBAR_SLOTS - 1) * padding;
    int startX = (SCREEN_WIDTH - totalW) / 2;
    // Dedicated status band above the selected-item name (see HUD_* layout).
    int barY = HUD_STATUS_Y;

    int iconSize = 16;
    int iconPad = 2;
    int barX = startX;

    // ---- Helper: draw a chunky MC heart at (cx, cy) using a pixel mask ----
    // 9x8 mask: two lobes on top, tapering to a point at the bottom.
    static const char *HEART_MASK[8] = {
        " XX XX ",
        "XXXXXXX",
        "XXXXXXX",
        "XXXXXXX",
        " XXXXX ",
        "  XXX  ",
        "   X   ",
        "       "
    };
    #define HEART_BLIT(cx, cy, px, col) do { \
        for (int _r = 0; _r < 8; _r++) { \
            const char *_row = HEART_MASK[_r]; \
            for (int _c = 0; _c < 7; _c++) { \
                if (_row[_c] == 'X') \
                    DrawRectangle((cx) - 4 * (px) + _c * (px), (cy) - 4 * (px) + _r * (px), (px), (px), col); \
            } \
        } \
    } while(0)
    #define DRAW_HEART(cx, cy, col, outline) do { \
        int hx = (cx), hy = (cy); \
        if (outline.a > 0) HEART_BLIT(hx, hy, 3, outline); \
        HEART_BLIT(hx, hy, 2, col); \
        DrawRectangle(hx - 4, hy - 3, 2, 2, (Color){255, 255, 255, 95}); \
    } while(0)

    float time = (float)GetTime();

    // ---- Armor bar (Minecraft-style, above hearts) ----
    int armorVal = GetTotalArmorPoints();
    if (armorVal > 0) {
        int armorY = barY - iconSize - 4;
        for (int i = 0; i < 10; i++) {
            int ax = barX + i * (iconSize + iconPad);
            bool filled = armorVal >= (i + 1) * 2;
            bool half = !filled && armorVal >= i * 2 + 1;
            Color ac = filled ? (Color){160, 165, 175, 240} : (half ? (Color){100, 105, 115, 180} : (Color){40, 42, 48, 100});
            // Simple chestplate icon (rectangle + lines)
            DrawRectangle(ax + 2, armorY + 1, 8, 9, ac);
            DrawRectangle(ax + 3, armorY, 6, 3, ac);
            DrawRectangleLines(ax + 3, armorY, 6, 3, (Color){(unsigned char)(ac.r+30), (unsigned char)(ac.g+30), (unsigned char)(ac.b+30), ac.a});
            DrawRectangleLines(ax + 2, armorY + 1, 8, 9, (Color){(unsigned char)(ac.r+25), (unsigned char)(ac.g+25), (unsigned char)(ac.b+25), ac.a});
        }
    }

    // Health hearts with damage pulse and low-health flash
    static float heartPulse = 0.0f;
    static int lastHealth = -1;
    if (lastHealth < 0) lastHealth = player.health;
    if (player.health < lastHealth) heartPulse = 1.0f;
    lastHealth = player.health;
    heartPulse *= powf(0.1f, GetFrameTime());
    if (heartPulse < 0.01f) heartPulse = 0.0f;

    bool healthLow = player.health <= 6;
    float healthFlash = healthLow ? (sinf(time * 4.0f) * 0.3f + 0.7f) : 1.0f;

    for (int i = 0; i < MAX_HEALTH / 2; i++) {
        float cx = barX + i * (iconSize + iconPad) + iconSize / 2.0f;
        float cy = barY + iconSize / 2.0f + 2;
        bool filled = player.health >= (i + 1) * 2;
        bool half = !filled && player.health >= i * 2 + 1;
        Color c = filled ? (Color){220, 60, 60, 255} : (half ? (Color){170, 50, 50, 230} : (Color){55, 25, 25, 160});
        Color outline = filled ? (Color){140, 30, 30, 200} : (Color){80, 20, 20, 120};
        if (filled) {
            float flash = 1.0f + heartPulse * 0.5f;
            float brightness = flash * healthFlash;
            c.r = (unsigned char)(c.r * brightness > 255 ? 255 : c.r * brightness);
            c.g = (unsigned char)(c.g * brightness > 255 ? 255 : c.g * brightness);
            c.b = (unsigned char)(c.b * brightness > 255 ? 255 : c.b * brightness);
        }
        DRAW_HEART(cx, cy, c, outline);
    }

    // Hunger drumsticks (MC style: beige bone handle, brown meat body)
    int hungerX = barX + (MAX_HEALTH / 2) * (iconSize + iconPad) + 16;
    bool hungerLow = player.hunger <= 6;
    float hungerFlash = hungerLow ? (sinf(time * 4.0f) * 0.3f + 0.7f) : 1.0f;
    for (int i = 0; i < MAX_HUNGER / 2; i++) {
        int dx = hungerX + i * (iconSize + iconPad);
        int dy = barY;
        bool filled = player.hunger >= (i + 1) * 2;
        bool half = !filled && player.hunger >= i * 2 + 1;
        Color meat = filled ? (Color){190, 120, 45, 255} : (half ? (Color){130, 85, 32, 235} : (Color){55, 38, 20, 190});
        Color dark = (Color){90, 55, 20, 235};
        if (hungerLow && filled) {
            meat.r = (unsigned char)(meat.r * hungerFlash);
            meat.g = (unsigned char)(meat.g * hungerFlash);
            meat.b = (unsigned char)(meat.b * hungerFlash);
        }
        // Bone handle (upper-left) + rounded meat body (lower-right)
        DrawRectangle(dx + 1, dy + 1, 3, 7, (Color){225, 215, 190, 235});
        DrawRectangle(dx + 4, dy + 4, 4, 3, dark);
        DrawRectangle(dx + 3, dy + 5, 10, 8, meat);
        DrawRectangle(dx + 4, dy + 4, 8, 2, meat);
        DrawRectangle(dx + 11, dy + 7, 3, 5, dark);
        DrawRectangle(dx + 5, dy + 7, 3, 3, (Color){230, 190, 140, 150});
    }

    // Oxygen bubbles (only show when underwater or not full)
    if (player.oxygen < MAX_OXYGEN) {
        int oxyX = hungerX + (MAX_HUNGER / 2) * (iconSize + iconPad) + 16;
        for (int i = 0; i < MAX_OXYGEN / 2; i++) {
            int bx = oxyX + i * (iconSize + iconPad) + iconSize / 2;
            int by = barY + iconSize / 2 + 1;
            bool filled = player.oxygen >= (i + 1) * 2;
            bool half = !filled && player.oxygen >= i * 2 + 1;
            Color c = filled ? (Color){80, 180, 240, 255} : (half ? (Color){55, 130, 200, 230} : (Color){28, 55, 95, 160});
            Color outline = filled ? (Color){40, 100, 160, 200} : (Color){20, 40, 70, 120};
            // Bubble: filled circle + highlight
            DrawCircle(bx, by, 4.5f, c);
            DrawCircleLines(bx, by, 4.5f, outline);
            if (filled) {
                DrawCircle(bx - 1, by - 2, 1.5f, (Color){160, 220, 255, 180}); // highlight
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Crosshair
//----------------------------------------------------------------------------------
void DrawCrosshair(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(Win32GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX >= 0 && blockX < WORLD_WIDTH && blockY >= 0 && blockY < WORLD_HEIGHT) {
        float px = (float)(blockX * BLOCK_SIZE);
        float py = (float)(blockY * BLOCK_SIZE);

        // Mining progress (crack overlay handled by DrawMiningCrack)
        float progress = GetMiningProgress();
        int mBlockX = GetMiningBlockX();
        int mBlockY = GetMiningBlockY();

        // Block placement preview (ghost block)
        int selItem = player.inventory[player.selectedSlot];
        BlockType cursorBlock = (BlockType)GetBlock(blockX, blockY);
        float playerCX = player.position.x + PLAYER_WIDTH / 2.0f;
        float playerCY = player.position.y + PLAYER_HEIGHT / 2.0f;
        float distBlocks = sqrtf(powf((blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCX, 2) +
                                 powf((blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCY, 2)) / BLOCK_SIZE;
        bool inRange = distBlocks <= PLACE_RANGE + 0.5f;
        if (selItem != BLOCK_AIR && selItem < BLOCK_COUNT && !IsTool((BlockType)selItem) && !IsFood((BlockType)selItem)
            && (cursorBlock == BLOCK_AIR || cursorBlock == BLOCK_WATER) && blockAtlas.id > 0 && inRange) {
            Rectangle src = { (float)(selItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
            Rectangle dst = { px, py, (float)BLOCK_SIZE, (float)BLOCK_SIZE };
            DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, (Color){255, 255, 255, 100});
        }

        // Crosshair outline
        DrawRectangleLines((int)px - 1, (int)py - 1, BLOCK_SIZE + 2, BLOCK_SIZE + 2, (Color){0, 0, 0, 120});
        DrawRectangleLines((int)px, (int)py, BLOCK_SIZE, BLOCK_SIZE, (Color){255, 255, 255, 200});

        // Mining progress arc (circular ring around crosshair)
        if (progress > 0.0f) {
            Vector2 center = { px + BLOCK_SIZE / 2.0f, py + BLOCK_SIZE / 2.0f };
            float innerR = BLOCK_SIZE * 0.38f;
            float outerR = BLOCK_SIZE * 0.48f;
            int segs = 24;
            Color bgRing = { 0, 0, 0, 100 };
            DrawRing(center, innerR, outerR, 0, 360, segs, bgRing);
            BlockType heldTool = (BlockType)player.inventory[player.selectedSlot];
            BlockType minedBlock = (BlockType)GetBlock(mBlockX, mBlockY);
            float speed = GetToolMiningSpeed(heldTool, minedBlock);
            Color arcColor;
            if (speed >= 3.0f) arcColor = (Color){60, 220, 60, 240};
            else if (speed >= 2.0f) arcColor = (Color){200, 200, 60, 240};
            else arcColor = (Color){200, 80, 60, 240};
            float startAngle = -90.0f;
            float endAngle = startAngle + 360.0f * progress;
            DrawRing(center, innerR, outerR, startAngle, endAngle, segs, arcColor);
            // Thin white outline on leading edge
            float rad = (innerR + outerR) * 0.5f;
            float radA = endAngle * DEG2RAD;
            Vector2 tip = { center.x + cosf(radA) * rad, center.y + sinf(radA) * rad };
            DrawCircleV(tip, 2.0f, (Color){255, 255, 255, 200});
        }
    }
}

//----------------------------------------------------------------------------------
// Debug Info
//----------------------------------------------------------------------------------
void DrawDebugInfo(void)
{
    if (!showDebug) return;
    int y = 10;
    int lineH = 16;
    Color c = (Color){255, 255, 0, 200};

    int chunkCount = 0;
    for (int i = 0; i < MAX_CHUNKS; i++) {
        if (loadedChunks[i].chunkX != CHUNK_EMPTY) chunkCount++;
    }

    DrawGameText(TextFormat(S(STR_DBG_FPS), GetFPS()), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_POS), player.position.x, player.position.y), 10, y,16, c); y += lineH;
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    DrawGameText(TextFormat(S(STR_DBG_BLOCK), bx, by), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_CHUNKS), chunkCount), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_TIME), dayNight.timeOfDay), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_LIGHT), dayNight.lightLevel), 10, y,16, c); y += lineH;
    const char *weatherStr = weather.type == WEATHER_CLEAR ? S(STR_WEATHER_CLEAR) : (weather.type == WEATHER_RAIN ? S(STR_WEATHER_RAIN) : S(STR_WEATHER_THUNDER));
    DrawGameText(TextFormat(S(STR_DBG_WEATHER), weatherStr), 10, y, 16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_MODE), gameMode == GAME_CREATIVE ? S(STR_MODE_CREATIVE) : S(STR_MODE_SURVIVAL)), 10, y, 16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_GROUND), player.onGround ? S(STR_YES) : S(STR_NO)), 10, y,16, c); y += lineH;
    // Player status
    DrawGameText(TextFormat(S(STR_DBG_HP), player.health, MAX_HEALTH, player.hunger, MAX_HUNGER), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_OXYGEN), player.oxygen, MAX_OXYGEN, player.xp, MAX_XP), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_UNDERWATER), IsPlayerUnderwater() ? S(STR_YES) : S(STR_NO)), 10, y,16, c); y += lineH;
    DrawGameText(TextFormat(S(STR_DBG_SEED), worldSeed), 10, y,16, c);
}

//----------------------------------------------------------------------------------
// Message (inventory full, etc.)
//----------------------------------------------------------------------------------
void ShowMessage(const char *msg, Color color)
{
    snprintf(messageText, sizeof(messageText), "%s", msg);
    messageTimer = MESSAGE_DURATION;
    messageColor = color;
    messageSlide = 1.0f;
}

void DrawMessage(void)
{
    if (messageTimer <= 0.0f) return;

    // Slide-in animation
    messageSlide *= powf(0.01f, GetFrameTime());
    if (messageSlide < 0.01f) messageSlide = 0.0f;

    int fontSize = 16;
    int textW = MeasureGameTextWidth(messageText, fontSize);
    int padX = 20, padY = 10;
    int pillW = textW + padX * 2;
    int pillH = fontSize + padY * 2;
    int x = (SCREEN_WIDTH - pillW) / 2;
    int slideOffset = (int)((1.0f - messageSlide) * -25.0f);
    int y = SCREEN_HEIGHT / 2 + 90 + slideOffset;

    float alpha = messageTimer > 0.5f ? 1.0f : messageTimer * 2.0f;
    unsigned char a = (unsigned char)(alpha * 255);

    // Glass-morphism pill background
    DrawRectangle(x, y, pillW, pillH, (Color){15, 14, 22, (unsigned char)(a * 0.9f)});
    // Top accent line (colored based on message type)
    DrawRectangle(x, y, pillW, 2, (Color){messageColor.r, messageColor.g, messageColor.b, (unsigned char)(a * 0.8f)});
    // Subtle inner highlight
    DrawRectangle(x + 1, y + 1, pillW - 2, pillH / 3, (Color){30, 28, 40, (unsigned char)(a * 0.4f)});
    // Border
    DrawRectangleLines(x, y, pillW, pillH, (Color){60, 55, 75, (unsigned char)(a * 0.5f)});
    // Text
    int textX = (SCREEN_WIDTH - textW) / 2;
    DrawGameText(messageText, textX, y + padY, fontSize, (Color){messageColor.r, messageColor.g, messageColor.b, a});
}

//----------------------------------------------------------------------------------
// Pause Menu
//----------------------------------------------------------------------------------
void DrawPauseMenu(void)
{
    static float pauseAnim = 0.0f;
    float dt = GetFrameTime();
    float target = gamePaused ? 1.0f : 0.0f;
    pauseAnim += (target - pauseAnim) * 25.0f * dt;
    if (pauseAnim < 0.01f && !gamePaused) return;
    if (pauseAnim > 0.99f) pauseAnim = 1.0f;

    // Dark overlay with scan lines
    unsigned char pauseOA = (unsigned char)(170 * pauseAnim);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, pauseOA});
    for (int y = 0; y < SCREEN_HEIGHT; y += 3) {
        DrawRectangle(0, y, SCREEN_WIDTH, 1, (Color){0, 0, 0, (unsigned char)(8 * pauseAnim)});
    }

    int boxW = 440;
    int boxH = 452;
    int boxX = (SCREEN_WIDTH - boxW) / 2;
    int boxY = (SCREEN_HEIGHT - boxH) / 2;
    boxY += (int)((1.0f - pauseAnim) * 40.0f);

    // Container — modern dark flat panel
    DrawUiBox(boxX, boxY, boxW, boxH, 0.05f, (Color){55, 55, 55, 245});
    DrawRectangleLines(boxX, boxY, boxW, boxH, (Color){90, 90, 90, 255});
    // Subtle inner top highlight
    DrawRectangle(boxX + 8, boxY, boxW - 16, 1, (Color){255, 255, 255, 18});

    // Title
    const char *title = S(STR_PAUSED);
    int titleW = MeasureGameTextWidth(title, 28);
    int titleX = boxX + (boxW - titleW) / 2;
    DrawGameText(title, titleX + 1, boxY + 15, 28, (Color){0, 0, 0, 100});
    DrawGameText(title, titleX, boxY + 14, 28, (Color){255, 255, 255, 255});
    DrawRectangle(boxX + 24, boxY + 46, boxW - 48, 1, (Color){90, 90, 90, 120});

    Vector2 mouse = Win32GetMousePosition();

    // --- Volume Sliders ---
    int sliderX = boxX + 30;
    int sliderW = boxW - 90;
    int sliderY = boxY + 62;

    static int activeSlider = -1;

    // BGM Volume
    DrawGameText(S(STR_MUSIC_VOLUME), sliderX, sliderY, 14, (Color){235, 235, 235, 255});
    sliderY += 20;
    Rectangle bgmArea = { (float)(sliderX - 10), (float)(sliderY - 12), (float)(sliderW + 20), 26.0f };
    bool bgmHover = CheckCollisionPointRec(mouse, bgmArea);

    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && bgmHover) activeSlider = 0;
    if (Win32IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) activeSlider = -1;
    if (activeSlider == 0) {
        bgmVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
        if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
        SetBGMVolume(bgmVolumeSlider);
    }
    DrawUiSlider(sliderX, sliderY, sliderW, bgmVolumeSlider, activeSlider == 0 || bgmHover, 1.0f);
    {
        char bgmText[16];
        snprintf(bgmText, sizeof(bgmText), "%d%%", (int)(bgmVolumeSlider * 100));
        DrawGameText(bgmText, sliderX + sliderW + 8, sliderY - 7, 14, (Color){235, 235, 235, 255});
    }

    // SFX Volume
    sliderY += 36;
    DrawGameText(S(STR_SFX_VOLUME), sliderX, sliderY, 14, (Color){235, 235, 235, 255});
    sliderY += 20;
    Rectangle sfxArea = { (float)(sliderX - 10), (float)(sliderY - 8), (float)(sliderW + 20), 24.0f };
    bool sfxHover = CheckCollisionPointRec(mouse, sfxArea);

    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && sfxHover) activeSlider = 1;
    if (Win32IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) activeSlider = -1;
    if (activeSlider == 1) {
        sfxVolumeSlider = (mouse.x - sliderX) / (float)sliderW;
        if (sfxVolumeSlider < 0.0f) sfxVolumeSlider = 0.0f;
        if (sfxVolumeSlider > 1.0f) sfxVolumeSlider = 1.0f;
        SetSFXVolume(sfxVolumeSlider);
    }
    DrawUiSlider(sliderX, sliderY, sliderW, sfxVolumeSlider, activeSlider == 1 || sfxHover, 1.0f);
    char sfxText[16];
    snprintf(sfxText, sizeof(sfxText), "%d%%", (int)(sfxVolumeSlider * 100));
    DrawGameText(sfxText, sliderX + sliderW + 8, sliderY - 7, 14, (Color){235, 235, 235, 255});

    // --- Game Mode Toggle (Survival / Creative) ---
    sliderY += 40;
    Rectangle modeBtn = { (float)(boxX + 30), (float)sliderY, (float)(boxW - 60), 28.0f };
    bool modeHover = CheckCollisionPointRec(mouse, modeBtn);
    bool modeIsCreative = (gameMode == GAME_CREATIVE);
    char modeLbl[64];
    snprintf(modeLbl, sizeof(modeLbl), "%s: %s", S(STR_GAMEMODE), modeIsCreative ? S(STR_MODE_CREATIVE) : S(STR_MODE_SURVIVAL));
    DrawUiButton(modeBtn.x, modeBtn.y, modeBtn.width, modeBtn.height, modeLbl, 15, modeHover, false, true, pauseAnim);
    if (modeHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySoundUIClick();
        if (NetIsClient()) {
            ShowMessage(S(STR_MSG_HOST_ONLY), (Color){240, 200, 80, 255});
        } else {
            gameMode = modeIsCreative ? GAME_SURVIVAL : GAME_CREATIVE;
            if (gameMode != GAME_CREATIVE) { creativeOpen = false; }
            if (NetIsHost()) {
                uint8_t mbuf[NET_PACKET_MAX];
                mbuf[0] = PKT_GAMEMODE_SYNC;
                mbuf[1] = (uint8_t)gameMode;
                NetSendToAll(mbuf, 2, true);
            }
            ShowMessage(Sf(STR_MODE_SET, gameMode == GAME_CREATIVE ? S(STR_MODE_CREATIVE) : S(STR_MODE_SURVIVAL)),
                        (Color){200, 220, 255, 255});
        }
    }
    sliderY += 28 + 8;

    // --- Controls ---
    int ctrlY = sliderY + 30;
    const char *ctrlTitle = S(STR_CONTROLS_TITLE);
    DrawGameText(ctrlTitle, boxX + (boxW - MeasureGameTextWidth(ctrlTitle, 16)) / 2, ctrlY, 16, (Color){235, 235, 235, 255});
    ctrlY += 8;
    DrawRectangle(boxX + 24, ctrlY, boxW - 48, 1, (Color){90, 90, 90, 90});
    ctrlY += 14;

    const char *keys[] = { S(STR_KEY_WASD), S(STR_KEY_SPACE), S(STR_KEY_CTRL), S(STR_KEY_SHIFT), S(STR_KEY_LCLICK), S(STR_KEY_RCLICK), S(STR_KEY_E), S(STR_KEY_H), S(STR_KEY_F3), S(STR_KEY_ESC), S(STR_KEY_19) };
    const char *acts[] = { S(STR_ACT_MOVE), S(STR_ACT_JUMP), S(STR_ACT_SPRINT), S(STR_ACT_SNEAK), S(STR_ACT_BREAK), S(STR_ACT_PLACE), S(STR_ACT_INVENTORY), S(STR_ACT_HEAL), S(STR_ACT_DEBUG), S(STR_ACT_PAUSE), S(STR_ACT_HOTBAR) };
    int numControls = sizeof(keys) / sizeof(keys[0]);
    int halfControls = (numControls + 1) / 2;
    int ctrlColW = (boxW - 60) / 2;
    for (int i = 0; i < numControls; i++) {
        int col = (i < halfControls) ? 0 : 1;
        int row = (i < halfControls) ? i : i - halfControls;
        int kx = boxX + 30 + col * ctrlColW;
        int yy = ctrlY + row * 15;
        DrawGameText(keys[i], kx, yy, 14, (Color){190, 190, 190, 235});
        DrawGameText(acts[i], kx + 68, yy, 14, (Color){225, 225, 225, 255});
    }
    ctrlY += halfControls * 15;

    // --- Open to LAN (contextual; hidden for clients who can't host) ---
    if (!NetIsClient()) {
        int lanW = boxW - 60;
        int lanX = boxX + 30;
        int lanH = 30;
        int lanY = ctrlY + 16;
        char lanIp[64];
        NetGetLocalIP(lanIp, sizeof(lanIp));
        if (NetIsHost()) {
            // Already open — show a status line instead of a button.
            char st[128];
            snprintf(st, sizeof(st), S(STR_LAN_STATUS), lanIp, NET_PORT, NetGetPlayerCount(), NET_MAX_PLAYERS);
            DrawUiBox(lanX, lanY, lanW, lanH, 0.06f, (Color){55, 55, 55, (unsigned char)(200 * pauseAnim)});
            DrawRectangleLinesEx((Rectangle){(float)lanX, (float)lanY, (float)lanW, (float)lanH}, 1, (Color){200, 200, 200, (unsigned char)(150 * pauseAnim)});
            int tw = MeasureGameTextWidth(st, 13);
            DrawGameText(st, lanX + (lanW - tw) / 2, lanY + 9, 14, (Color){190, 220, 200, (unsigned char)(230 * pauseAnim)});
        } else {
            Rectangle r = {(float)lanX, (float)lanY, (float)lanW, (float)lanH};
            bool hover = CheckCollisionPointRec(mouse, r);
            DrawUiButton(lanX, lanY, lanW, lanH, S(STR_OPEN_TO_LAN), 16, hover, false, true, pauseAnim);
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySoundUIClick();
                if (NetHostStart(NET_PORT)) {
                    localPlayerId = 0;
                    ShowMessage(Sf(STR_LAN_OPENED, lanIp, NET_PORT), (Color){200, 200, 200, 255});
                    gamePaused = false;
                } else {
                    ShowMessage(S(STR_LAN_FAILED), (Color){232, 87, 92, 255});
                }
            }
        }
    }

    // --- Buttons (styled like main menu) ---
    {
        int btnW = 110;
        int btnH = 34;
        int btnY = ctrlY + (NetIsClient() ? 20 : 60);
        int btnGap = 12;
        int totalBtnW = btnW * 3 + btnGap * 2;
        int btnStartX = boxX + (boxW - totalBtnW) / 2;

        const char *pauseLabels[] = { S(STR_CONTINUE), S(STR_MAIN_MENU), S(STR_BTN_QUIT) };
        for (int pi = 0; pi < 3; pi++) {
            int bx = btnStartX + pi * (btnW + btnGap);
            Rectangle r = {(float)bx, (float)btnY, (float)btnW, (float)btnH};
            bool hover = CheckCollisionPointRec(mouse, r);

            DrawUiButton(bx, btnY, btnW, btnH, pauseLabels[pi], 16, hover, false, true, pauseAnim);

            // Click actions
            if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                PlaySoundUIClick();
                if (pi == 0) {
                    gamePaused = false;
                } else if (pi == 1) {
                    if (currentSavePath[0]) SaveWorld(currentSavePath);
                    if (NetIsHost()) NetHostStop();
                    else if (NetIsClient()) NetClientDisconnect();
                    gamePaused = false; inventoryOpen = false;
                    StartTransition(STATE_MENU); menuSelection = 0;
                } else {
                    if (currentSavePath[0]) SaveWorld(currentSavePath);
                    CloseWindow(); exit(0);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Death Screen
//----------------------------------------------------------------------------------
float GetDeathFadeTimer(void) { return deathFadeTimer; }

static void DrawCompetitionIcon(int x, int y, int size, unsigned char alpha, bool unlocked)
{
    Color gold = {255, 205, 90, alpha};
    Color teal = {56, 217, 169, alpha};
    DrawUiSlot(x, y, size, false, unlocked, alpha / 255.0f);
    int cx = x + size / 2, top = y + size / 5;
    DrawRectangle(cx - size / 5, top, size * 2 / 5, size / 3, (Color){34, 42, 56, alpha});
    DrawRectangleLines(cx - size / 5, top, size * 2 / 5, size / 3, gold);
    DrawRectangle(x + size / 8, top + 3, size / 8, size / 5, teal);
    DrawRectangle(x + size * 3 / 4, top + 3, size / 8, size / 5, teal);
    DrawRectangle(cx - size / 16, top + size / 3, size / 8, size / 5, gold);
    DrawRectangle(cx - size / 4, y + size / 2, size / 2, size / 10, gold);
    DrawLine(x + size / 8, y + size * 3 / 4, x + size / 3, y + size * 3 / 5, teal);
    DrawLine(x + size * 7 / 8, y + size * 3 / 4, x + size * 2 / 3, y + size * 3 / 5, teal);
}

static void DrawCollectibleIcon(int x, int y, int size, int variant, unsigned char alpha, bool unlocked)
{
    Color frame = unlocked ? (Color){255, 205, 90, alpha} : (Color){74, 88, 112, alpha};
    Color gem = unlocked ? (Color){74, 210, 235, alpha} : (Color){70, 80, 100, alpha};
    DrawUiSlot(x, y, size, false, unlocked, alpha / 255.0f);
    int cx = x + size / 2, cy = y + size / 2;
    if (variant == 1) {
        DrawRectangle(x + size / 5, y + size / 4, size * 3 / 5, size / 2, (Color){130, 90, 55, alpha});
        DrawRectangleLines(x + size / 5, y + size / 4, size * 3 / 5, size / 2, frame);
        DrawLine(cx, y + size / 4, cx, y + size * 3 / 4, frame);
    } else {
        DrawCircle(cx, cy, size / 4, gem);
        DrawCircleLines(cx, cy, size / 4, frame);
        DrawTriangle((Vector2){(float)cx, (float)(y + size / 8)}, (Vector2){(float)(x + size * 7 / 8), (float)cy}, (Vector2){(float)cx, (float)(y + size * 7 / 8)}, frame);
    }
    DrawRectangle(cx - size / 2, cy - 1, size / 4, 2, frame);
    DrawRectangle(cx + size / 4, cy - 1, size / 4, 2, frame);
}

void DrawAchievementsUI(void)
{
    if (!achievementsOpen) return;
    int panelW = 760, panelH = 570;
    int panelX = (SCREEN_WIDTH - panelW) / 2, panelY = 75;
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 185});
    DrawUiPanel(panelX, panelY, panelW, panelH, 255);
    DrawCompetitionIcon(panelX + 24, panelY + 22, 56, 255, true);
    DrawGameText(S(STR_COLLECTION_TITLE), panelX + 96, panelY + 28, 28, (Color){62, 62, 62, 255});
    DrawGameText(S(STR_COLLECTION_CHALLENGES), panelX + 98, panelY + 64, 14, (Color){78, 78, 78, 240});
    static const StringId achNames[ACH_COUNT] = {
        STR_ACH_FIRST_STEPS, STR_ACH_DEEP_DIG, STR_ACH_MONSTER_HUNTER, STR_ACH_ARCHITECT,
        STR_ACH_REDSTONE_ENGINEER, STR_ACH_COLLECTOR, STR_ACH_ANGLER, STR_ACH_BREEDER,
        STR_ACH_ENCHANTER, STR_ACH_DEMOLITION
    };
    for (int i = 0; i < ACH_COUNT; i++) {
        int col = i % 2, row = i / 2;
        int x = panelX + 28 + col * 366, y = panelY + 112 + row * 72;
        bool unlocked = achievements[i];
        DrawUiBox(x, y, 340, 60, 0.0f, unlocked ? (Color){150, 150, 150, 245} : (Color){120, 120, 120, 245});
        DrawRectangleLines(x, y, 340, 60, unlocked ? (Color){170, 130, 20, 255} : (Color){85, 85, 85, 255});
        DrawCollectibleIcon(x + 8, y + 8, 44, i % 3, 255, unlocked);
        DrawGameText(S(achNames[i]), x + 62, y + 11, 14, (Color){62, 62, 62, unlocked ? (unsigned char)255 : (unsigned char)205});
        DrawGameText(S(unlocked ? STR_COLLECTION_UNLOCKED : STR_COLLECTION_LOCKED), x + 62, y + 36, 11, unlocked ? (Color){150, 112, 12, 255} : (Color){95, 95, 95, 255});
    }
    DrawGameText(S(STR_COLLECTION_CLOSE), panelX + 250, panelY + panelH - 30, 14, (Color){72, 72, 72, 245});
}

void DrawDeathScreen(float dt)
{
    if (!player.playerDead) {
        deathFadeTimer = 0.0f;
        return;
    }

    deathFadeTimer += dt;
    float alpha = deathFadeTimer < 1.0f ? deathFadeTimer : 1.0f;
    float time = (float)GetTime();

    // Dramatic dark overlay with red vignette
    unsigned char overlayA = (unsigned char)(alpha * 180);
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){60, 0, 0, overlayA});

    // Radial vignette effect (darker edges)
    for (int edge = 0; edge < 80; edge += 4) {
        unsigned char edgeA = (unsigned char)(alpha * (80 - edge));
        DrawRectangle(0, edge, SCREEN_WIDTH, 4, (Color){0, 0, 0, edgeA});
        DrawRectangle(0, SCREEN_HEIGHT - edge - 4, SCREEN_WIDTH, 4, (Color){0, 0, 0, edgeA});
        DrawRectangle(edge, 0, 4, SCREEN_HEIGHT, (Color){0, 0, 0, (unsigned char)(edgeA * 0.7f)});
        DrawRectangle(SCREEN_WIDTH - edge - 4, 0, 4, SCREEN_HEIGHT, (Color){0, 0, 0, (unsigned char)(edgeA * 0.7f)});
    }

    // Animated blood drip lines
    for (int i = 0; i < 5; i++) {
        int dripX = (SCREEN_WIDTH / 6) * (i + 1);
        float dripProgress = (alpha - 0.2f - i * 0.05f);
        if (dripProgress > 0.0f) {
            int dripH = (int)(dripProgress * SCREEN_HEIGHT * 0.4f);
            if (dripH > SCREEN_HEIGHT / 2) dripH = SCREEN_HEIGHT / 2;
            unsigned char dripA = (unsigned char)(40 * alpha);
            DrawRectangle(dripX, 0, 2, dripH, (Color){120, 0, 0, dripA});
        }
    }

    if (alpha > 0.4f) {
        float textFade = (alpha - 0.4f) / 0.6f;
        unsigned char textA = (unsigned char)(textFade * 255);

        // "YOU DIED" with dramatic glow
        const char *text = S(STR_YOU_DIED);
        int fontSize = 56;
        int textW = MeasureGameTextWidth(text, fontSize);
        int tx = (SCREEN_WIDTH - textW) / 2;
        int ty = SCREEN_HEIGHT / 2 - 70;

        // Pulsing red glow
        float pulse = sinf(time * 2.0f) * 0.3f + 0.7f;
        unsigned char glowA = (unsigned char)(30 * pulse * textFade);
        for (int r = 5; r > 0; r--) {
            DrawGameText(text, tx - r, ty, fontSize, (Color){180, 0, 0, (unsigned char)(glowA / r)});
            DrawGameText(text, tx + r, ty, fontSize, (Color){180, 0, 0, (unsigned char)(glowA / r)});
            DrawGameText(text, tx, ty - r, fontSize, (Color){180, 0, 0, (unsigned char)(glowA / r)});
            DrawGameText(text, tx, ty + r, fontSize, (Color){180, 0, 0, (unsigned char)(glowA / r)});
        }
        // Shadow
        DrawGameText(text, tx + 3, ty + 3, fontSize, (Color){0, 0, 0, (unsigned char)(textA * 0.6f)});
        // Main text
        DrawGameText(text, tx, ty, fontSize, (Color){230, 50, 50, textA});

        // Decorative line under title
        int lineW = 200;
        int lineX = (SCREEN_WIDTH - lineW) / 2;
        DrawRectangle(lineX, ty + 65, lineW, 1, (Color){150, 50, 50, (unsigned char)(textA * 0.6f)});

        // Death cause
        const char *cause = S(lastDeathCause);
        int causeW = MeasureGameTextWidth(cause, 18);
        DrawGameText(cause, (SCREEN_WIDTH - causeW) / 2, ty + 75, 14, (Color){200, 180, 170, textA});

        // Score (XP)
        char scoreBuf[64];
        snprintf(scoreBuf, sizeof(scoreBuf), S(STR_DEATH_SCORE), player.xp);
        int scoreW = MeasureGameTextWidth(scoreBuf, 16);
        DrawGameText(scoreBuf, (SCREEN_WIDTH - scoreW) / 2, ty + 92, 14, (Color){220, 200, 120, textA});
    }

    if (deathFadeTimer > DEATH_RESPAWN_READY_TIME) {
        float subFade = (alpha - 0.8f) * 5.0f;
        unsigned char subA = (unsigned char)(subFade * 255);

        // Fixed logical button geometry keeps localization and scaling stable.
        const char *sub = S(STR_PRESS_SPACE_RESPAWN);
        float bounce = sinf(time * 3.0f) * 0.08f + 0.92f;
        Vector2 dmouse = Win32GetMousePosition();
        Rectangle respawnBtn = {
            (float)(SCREEN_WIDTH - DEATH_RESPAWN_BUTTON_W) / 2.0f,
            (float)DEATH_RESPAWN_BUTTON_Y,
            DEATH_RESPAWN_BUTTON_W,
            DEATH_RESPAWN_BUTTON_H
        };
        bool respawnHover = CheckCollisionPointRec(dmouse, respawnBtn);
        DrawUiButton(respawnBtn.x, respawnBtn.y, respawnBtn.width, respawnBtn.height,
                     sub, 18, respawnHover, false, true, subA * bounce);

        const char *escHint = S(STR_PRESS_ESC_MENU);
        int escW = MeasureGameTextWidth(escHint, 14);
        DrawGameText(escHint, (SCREEN_WIDTH - escW) / 2, SCREEN_HEIGHT / 2 + 82, 14, (Color){160, 155, 170, (unsigned char)(subA * 0.6f)});
    }
}

//----------------------------------------------------------------------------------
// Minimap
//----------------------------------------------------------------------------------
void DrawMinimap(void)
{
    int mapSize = MINIMAP_SIZE;
    int mapX = SCREEN_WIDTH - mapSize - 10;
    int mapY = 30; // below FPS counter

    // Minecraft-style frame: beveled gray border around a dark bezel
    DrawRectangle(mapX - 3, mapY - 3, mapSize + 6, mapSize + 6, (Color){25, 25, 25, 230});
    DrawRectangle(mapX - 2, mapY - 2, mapSize + 4, mapSize + 4, (Color){110, 110, 110, 230});
    DrawRectangle(mapX - 1, mapY - 1, mapSize + 2, mapSize + 2, (Color){16, 0, 16, 235});
    DrawRectangle(mapX, mapY, mapSize, mapSize, (Color){35, 35, 35, 255});

    // Sky tint inside the map area (follows day/night light level)
    float lvl = dayNight.lightLevel;
    Color skyCol = { (unsigned char)(36 + lvl * 24), (unsigned char)(52 + lvl * 48), (unsigned char)(86 + lvl * 92), 255 };
    DrawRectangle(mapX, mapY, mapSize, mapSize, skyCol);

    // Calculate player's block position
    int playerBX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;

    int range = MINIMAP_RANGE;

    // Draw terrain - scan columns for first non-air block (surface or water)
    for (int px = 0; px < mapSize; px++) {
        int worldBX = playerBX + (px - mapSize / 2) * range / (mapSize / 2);
        if (worldBX < 0 || worldBX >= WORLD_WIDTH) continue;

        // Find first non-air block
        int surfaceY = -1;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (GetBlock(worldBX, y) != BLOCK_AIR) {
                surfaceY = y;
                break;
            }
        }
        if (surfaceY < 0) continue;

        BlockType bt = (BlockType)GetBlock(worldBX, surfaceY);
        Color c = {100, 100, 100, 255};
        if (bt == BLOCK_GRASS) c = (Color){92, 165, 64, 255};
        else if (bt == BLOCK_DIRT) c = (Color){146, 104, 61, 255};
        else if (bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE) c = (Color){132, 132, 132, 255};
        else if (bt == BLOCK_SAND) c = (Color){216, 202, 132, 255};
        else if (bt == BLOCK_WATER) c = (Color){46, 92, 176, 255};
        else if (bt == BLOCK_WOOD || bt == BLOCK_PLANKS) c = (Color){150, 106, 54, 255};
        else if (bt == BLOCK_LEAVES) c = (Color){54, 134, 42, 255};
        else if (bt == BLOCK_COAL_ORE) c = (Color){66, 66, 66, 255};
        else if (bt == BLOCK_IRON_ORE) c = (Color){168, 146, 132, 255};
        else if (bt == BLOCK_SANDSTONE) c = (Color){196, 174, 122, 255};
        else if (bt == BLOCK_GOLD_ORE) c = (Color){206, 182, 58, 255};
        else if (bt == BLOCK_DIAMOND_ORE) c = (Color){92, 216, 216, 255};
        else if (bt == BLOCK_REDSTONE_ORE) c = (Color){186, 44, 44, 255};
        else if (bt == BLOCK_LAPIS_ORE) c = (Color){48, 66, 190, 255};
        else if (bt == BLOCK_SNOW || bt == BLOCK_ICE || bt == BLOCK_PACKED_ICE) c = (Color){210, 224, 238, 255};
        else if (bt == BLOCK_GRAVEL) c = (Color){108, 102, 96, 255};
        else if (bt == BLOCK_CLAY) c = (Color){168, 156, 146, 255};
        else if (bt == BLOCK_BRICK) c = (Color){156, 86, 76, 255};
        else if (bt == BLOCK_BEDROCK) c = (Color){52, 44, 58, 255};
        else if (bt == BLOCK_OBSIDIAN) c = (Color){42, 30, 54, 255};
        else if (bt == BLOCK_TORCH || bt == BLOCK_LANTERN) c = (Color){244, 204, 84, 255};

        int mapPY = mapY + (surfaceY * mapSize / WORLD_HEIGHT);
        if (mapPY < mapY) mapPY = mapY;
        if (mapPY >= mapY + mapSize) mapPY = mapY + mapSize - 1;

        // Column height from surface to map bottom
        int colH = mapY + mapSize - mapPY;

        if (bt == BLOCK_WATER) {
            // Water column with a dirt/terrain bed below (approx surface depth)
            DrawRectangle(mapX + px, mapPY, 1, colH, c);
            int underY = mapPY + colH / 2;
            DrawRectangle(mapX + px, underY, 1, mapY + mapSize - underY, (Color){120, 88, 52, 255});
        } else {
            // Terrain body with a subtle underground shading near the surface
            DrawRectangle(mapX + px, mapPY, 1, colH, c);
            int shadeH = colH / 4;
            if (shadeH < 1) shadeH = 1;
            if (shadeH > 8) shadeH = 8;
            Color shade = { (unsigned char)(c.r * 0.72f), (unsigned char)(c.g * 0.72f), (unsigned char)(c.b * 0.72f), 255 };
            DrawRectangle(mapX + px, mapPY + colH - shadeH, 1, shadeH, shade);
        }
    }

    // Draw mobs as dots
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        int mobBX = (int)(mobs[i].position.x + 8) / BLOCK_SIZE;
        int mobBY = (int)(mobs[i].position.y + 8) / BLOCK_SIZE;
        int dx = (mobBX - playerBX) * (mapSize / 2) / range + mapSize / 2;
        int dy = mobBY * mapSize / WORLD_HEIGHT;
        int dotX = mapX + dx;
        int dotY = mapY + dy;
        if (dotX >= mapX && dotX < mapX + mapSize && dotY >= mapY && dotY < mapY + mapSize) {
            Color dotColor;
            if (mobs[i].type == MOB_ZOMBIE) dotColor = (Color){86, 190, 64, 255};
            else if (mobs[i].type == MOB_SKELETON) dotColor = (Color){208, 208, 196, 255};
            else dotColor = (Color){226, 154, 144, 255}; // pig & other passives
            DrawRectangle(dotX - 1, dotY - 1, 3, 3, dotColor);
        }
    }

    // Player marker: white arrow pointing up (chunky pixel arrow)
    int pdx = mapX + mapSize / 2;
    int pdy = mapY + playerBY * mapSize / WORLD_HEIGHT;
    if (pdy < mapY) pdy = mapY;
    if (pdy > mapY + mapSize - 1) pdy = mapY + mapSize - 1;
    DrawRectangle(pdx - 1, pdy - 3, 3, 5, WHITE);       // vertical bar
    DrawRectangle(pdx - 2, pdy - 3, 5, 3, WHITE);       // arrow head
    DrawRectangle(pdx - 2, pdy + 2, 5, 1, WHITE);       // base bar
    DrawRectangle(pdx - 2, pdy - 4, 1, 1, (Color){200, 220, 255, 255}); // head highlight

    // Inner bevel highlight (bottom-right dark, top-left light) for pixel frame look
    DrawRectangle(mapX, mapY, mapSize, 1, (Color){160, 160, 160, 130});
    DrawRectangle(mapX, mapY, 1, mapSize, (Color){160, 160, 160, 130});
    DrawRectangle(mapX, mapY + mapSize - 1, mapSize, 1, (Color){0, 0, 0, 130});
    DrawRectangle(mapX + mapSize - 1, mapY, 1, mapSize, (Color){0, 0, 0, 130});
}

//----------------------------------------------------------------------------------
// Large Map (M key)
//----------------------------------------------------------------------------------
void DrawLargeMap(void)
{
    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 200});

    int margin = 40;
    int mapX = margin;
    int mapY = margin;
    int mapW = SCREEN_WIDTH - margin * 2;
    int mapH = SCREEN_HEIGHT - margin * 2 - 30; // leave room for hint text

    // Background
    DrawRectangle(mapX - 2, mapY - 2, mapW + 4, mapH + 4, (Color){20, 18, 25, 240});
    DrawRectangleLines(mapX - 2, mapY - 2, mapW + 4, mapH + 4, (Color){80, 75, 95, 220});

    int playerBX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;

    // Show a wider range than minimap
    int rangeX = 300; // blocks visible horizontally
    int rangeY = rangeX * mapH / mapW; // maintain aspect ratio

    // Draw terrain columns (2x2 pixel blocks for performance)
    int step = 2;
    for (int px = 0; px < mapW; px += step) {
        int worldBX = playerBX + (px - mapW / 2) * rangeX / (mapW / 2);
        if (worldBX < 0 || worldBX >= WORLD_WIDTH) continue;

        for (int py = 0; py < mapH; py += step) {
            int worldBY = playerBY + (py - mapH / 2) * rangeY / (mapH / 2);
            if (worldBY < 0 || worldBY >= WORLD_HEIGHT) continue;

            uint8_t bt = GetBlock(worldBX, worldBY);
            if (bt == BLOCK_AIR) continue;

            Color c = {0, 0, 0, 0};
            switch (bt) {
                case BLOCK_GRASS: c = (Color){80, 160, 60, 255}; break;
                case BLOCK_DIRT: c = (Color){130, 90, 50, 255}; break;
                case BLOCK_STONE: case BLOCK_COBBLESTONE: c = (Color){120, 120, 120, 255}; break;
                case BLOCK_SAND: c = (Color){210, 200, 140, 255}; break;
                case BLOCK_WATER: c = (Color){40, 80, 160, 180}; break;
                case BLOCK_WOOD: c = (Color){140, 100, 50, 255}; break;
                case BLOCK_LEAVES: c = (Color){50, 130, 40, 200}; break;
                case BLOCK_COAL_ORE: c = (Color){60, 60, 60, 255}; break;
                case BLOCK_IRON_ORE: c = (Color){160, 140, 130, 255}; break;
                case BLOCK_GOLD_ORE: c = (Color){200, 180, 60, 255}; break;
                case BLOCK_DIAMOND_ORE: c = (Color){80, 220, 220, 255}; break;
                case BLOCK_REDSTONE_ORE: c = (Color){180, 40, 40, 255}; break;
                case BLOCK_LAPIS_ORE: c = (Color){40, 60, 180, 255}; break;
                case BLOCK_SANDSTONE: c = (Color){190, 170, 120, 255}; break;
                case BLOCK_BEDROCK: c = (Color){40, 35, 45, 255}; break;
                case BLOCK_GRAVEL: c = (Color){100, 95, 90, 255}; break;
                case BLOCK_CLAY: c = (Color){160, 150, 140, 255}; break;
                case BLOCK_BRICK: c = (Color){150, 80, 70, 255}; break;
                case BLOCK_TORCH: c = (Color){240, 200, 80, 255}; break;
                case BLOCK_FLOWER: c = (Color){220, 80, 120, 255}; break;
                case BLOCK_TALL_GRASS: c = (Color){60, 140, 50, 255}; break;
                case BLOCK_PLANKS: c = (Color){180, 140, 70, 255}; break;
                case BLOCK_FURNACE: c = (Color){100, 80, 70, 255}; break;
                case BLOCK_CRAFTING_TABLE: c = (Color){160, 120, 60, 255}; break;
                case BLOCK_CHEST: c = (Color){180, 140, 60, 255}; break;
                case BLOCK_BED: c = (Color){180, 60, 60, 255}; break;
                case BLOCK_GLASS: c = (Color){180, 200, 220, 100}; break;
                default: c = (Color){100, 100, 100, 255}; break;
            }
            if (c.a > 0) {
                DrawRectangle(mapX + px, mapY + py, step, step, c);
            }
        }
    }

    // Draw mobs as dots
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        int mobBX = (int)(mobs[i].position.x + 8) / BLOCK_SIZE;
        int mobBY = (int)(mobs[i].position.y + 8) / BLOCK_SIZE;
        int dx = (mobBX - playerBX) * (mapW / 2) / rangeX + mapW / 2;
        int dy = (mobBY - playerBY) * (mapH / 2) / rangeY + mapH / 2;
        int dotX = mapX + dx;
        int dotY = mapY + dy;
        if (dotX >= mapX && dotX < mapX + mapW && dotY >= mapY && dotY < mapY + mapH) {
            Color dotColor;
            if (mobs[i].type == MOB_ZOMBIE) dotColor = (Color){80, 180, 60, 255};
            else if (mobs[i].type == MOB_SKELETON) dotColor = (Color){220, 220, 200, 255};
            else dotColor = (Color){230, 160, 150, 255};
            DrawRectangle(dotX - 1, dotY - 1, 3, 3, dotColor);
        }
    }

    // Draw player as bright yellow dot
    int pdx = mapX + mapW / 2;
    int pdy = mapY + mapH / 2;
    DrawRectangle(pdx - 2, pdy - 2, 5, 5, (Color){255, 255, 100, 255});
    DrawRectangle(pdx - 1, pdy - 1, 3, 3, (Color){255, 255, 200, 255});

    // Title and hints
    DrawGameText(S(STR_WORLD_MAP), mapX, mapY - 22, 14, (Color){200, 195, 210, 255});
    DrawGameText(S(STR_PRESS_M_CLOSE), SCREEN_WIDTH / 2 - 70, SCREEN_HEIGHT - margin + 5,14, (Color){150, 145, 165, 200});

    // Player coordinates
    DrawGameText(TextFormat(S(STR_PLAYER_COORD), playerBX, playerBY), mapX + mapW - 150, mapY - 22,16, (Color){180, 175, 195, 200});
}

//----------------------------------------------------------------------------------
// Main Menu — Redesigned: deep night, gold accent, premium card buttons
//----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------
// World loading screen.
//
// Generating a world is a single blocking call, so without this the window just
// froze on whatever was last drawn. This paints a progress screen first, which
// the main loop flushes to the screen before it starts the expensive work.
//
// progress is 0..1; the caller drives it (the generator itself is not split
// into steps, so it currently sits at a fixed early value and completes when
// the world is ready).
//----------------------------------------------------------------------------------
void DrawLoadingScreen(const char *message, float progress)
{
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    // Backdrop matching the menu dusk gradient, so the transition is not jarring
    const Color top = { 24,  32,  56, 255};
    const Color bot = { 48,  60,  86, 255};
    int h = SCREEN_HEIGHT;
    for (int y = 0; y < h; y++) {
        float t = (float)y / (float)h;
        DrawRectangle(0, y, SCREEN_WIDTH, 1, (Color){
            (unsigned char)(top.r + (bot.r - top.r) * t),
            (unsigned char)(top.g + (bot.g - top.g) * t),
            (unsigned char)(top.b + (bot.b - top.b) * t), 255});
    }

    // Panel
    int pw = 420, ph = 116;
    int px = (SCREEN_WIDTH - pw) / 2;
    int py = (SCREEN_HEIGHT - ph) / 2;
    DrawUiPanel(px, py, pw, ph, 255);

    // Message
    int mw = MeasureGameTextWidth(message, 28);
    DrawGameText(message, px + (pw - mw) / 2, py + 22, 28, UI_TEXT_ON_LT);

    // Progress bar: recessed groove with a filled portion
    int bx = px + 30, by = py + 62, bw = pw - 60, bh = 18;
    DrawRectangle(bx, by, bw, bh, (Color){70, 70, 70, 255});
    DrawRectangle(bx, by, bw, 1, (Color){40, 40, 40, 255});
    DrawRectangle(bx, by, 1, bh, (Color){40, 40, 40, 255});
    int fill = (int)(progress * (bw - 4));
    if (fill > 0) DrawRectangle(bx + 2, by + 2, fill, bh - 4, (Color){96, 168, 86, 255});
    DrawRectangleLinesEx((Rectangle){(float)bx, (float)by, (float)bw, (float)bh}, 1, (Color){140, 140, 140, 255});

    // Percentage
    char pct[16];
    snprintf(pct, sizeof(pct), "%d%%", (int)(progress * 100));
    int pctW = MeasureGameTextWidth(pct, 14);
    DrawGameText(pct, px + (pw - pctW) / 2, by + bh + 6, 14, UI_TEXT_ON_LT);
}
void DrawMainMenu(void)
{
    float time = (float)GetTime();
    float dt = GetFrameTime();

    // Menu entrance animation state
    static float menuEnterTime = 0.0f;
    static bool menuInited = false;
    extern bool g_resetMenuAnim;
    if (g_resetMenuAnim) {
        menuInited = false;
        g_resetMenuAnim = false;
    }
    if (!menuInited) {
        menuEnterTime = time;
        menuInited = true;
    }
    float elapsed = time - menuEnterTime;

    // ================================================================
    // Background: parallax dusk scene (sky, hills, clouds, sun)
    // ================================================================
    DrawMenuBackground(time);

    // Soft veil behind the bottom hints so they stay readable on the hills
    DrawRectangle(0, SCREEN_HEIGHT - 40, SCREEN_WIDTH, 40, (Color){10, 12, 18, 150});

    // Focused menu card: keeps the title and actions readable over the animated sky.
    // The menu keeps the sky open; the controls themselves provide the focus.


    // ================================================================
    // Title — block-themed with decorative icons and sparkles
    // ================================================================
    const char *title = S(STR_TITLE);
    int titleSize = 84;
    int titleW = MeasureGameTextWidth(title, titleSize);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    int titleY = 62;
    int titleLen = (int)strlen(title);

    float titleTotalDur = 0.4f + titleLen * 0.055f;
    int charWidths[128];
    int charPositions[128];
    int runX = titleX;
    for (int k = 0; k < titleLen && k < 128; k++) {
        char tmp[2] = { title[k], 0 };
        charWidths[k] = MeasureGameTextWidth(tmp, titleSize);
        charPositions[k] = runX;
        runX += charWidths[k];
    }

    // Background glow aura (removed for clean modern look)
    if (elapsed < titleTotalDur) {

    // Bounce-in animation
    for (int ci = 0; ci < titleLen; ci++) {
        float letterStart = ci * 0.055f;
        float lt = elapsed - letterStart;
        if (lt < 0.0f) continue;
        float progress = lt / 0.38f;
        if (progress > 1.0f) progress = 1.0f;
        float bounce;
        if (progress < 0.55f) { bounce = progress / 0.55f; bounce = 1.2f * bounce; }
        else if (progress < 0.78f) { bounce = 1.2f - (progress - 0.55f) / 0.23f * 0.22f; }
        else { bounce = 0.96f + (progress - 0.78f) / 0.22f * 0.04f; }
        float alpha = progress < 0.3f ? progress / 0.3f : 1.0f;
        float offsetY = (1.0f - bounce) * -28.0f;
        char chBuf[2] = { title[ci], 0 };
        int cx = charPositions[ci];
        int cy = titleY + (int)offsetY;
        unsigned char ca = (unsigned char)(alpha * 255);
        DrawGameText(chBuf, cx + 2, cy + 3, titleSize, (Color){0, 0, 0, (unsigned char)(120 * alpha)});
        DrawGameText(chBuf, cx, cy, titleSize, (Color){235, 215, 165, ca});
    }
    }

    // Final title — MC gold with a full dark outline + hard drop shadow
    if (elapsed > titleTotalDur) {
        Color outline = (Color){52, 36, 12, 255};
        DrawGameText(title, titleX + 3, titleY + 4, titleSize, (Color){0, 0, 0, 180});
        for (int dy = -3; dy <= 3; dy += 3)
            for (int dx = -3; dx <= 3; dx += 3)
                if (dx || dy) DrawGameText(title, titleX + dx, titleY + dy, titleSize, outline);
        DrawGameText(title, titleX, titleY, titleSize, (Color){252, 226, 130, 255});
    }

    // ================================================================
    // Subtitle — refined
    // ================================================================
    float subFade = (elapsed - titleTotalDur) / 0.45f;
    if (subFade > 0.0f) {
        if (subFade > 1.0f) subFade = 1.0f;
        const char *sub = S(STR_SUBTITLE);
        int subW = MeasureGameTextWidth(sub, 17);
        unsigned char subA = (unsigned char)(subFade * 170);
        float subY = 155.0f + (1.0f - subFade) * 8.0f;
        DrawGameText(sub, (SCREEN_WIDTH - subW) / 2, (int)subY, 14, (Color){145, 145, 165, subA});
    }

    // ================================================================
    // Separator line — subtle, Minecraft-style
    // ================================================================
    float sepFade = (elapsed - titleTotalDur - 0.25f) / 0.35f;
    if (sepFade > 0.0f) {
        if (sepFade > 1.0f) sepFade = 1.0f;
        unsigned char la = (unsigned char)(sepFade * 120);
        int lineY = 188;
        int lineHalf = 200;
        // Dark line with thin white highlight below (chunky pixel look)
        DrawRectangle(SCREEN_WIDTH / 2 - lineHalf, lineY, lineHalf * 2, 1, (Color){0, 0, 0, (unsigned char)(la)});
        DrawRectangle(SCREEN_WIDTH / 2 - lineHalf, lineY + 1, lineHalf * 2, 1, (Color){90, 90, 100, (unsigned char)(la * 0.6f)});
    }

    // ================================================================
    // Buttons — Minecraft-style chunky gray buttons
    // ================================================================
    // These MUST match game.c UpdateMainMenu exactly
    int btnW = 260, btnH = 42;
    int btnX = (SCREEN_WIDTH - btnW) / 2;
    int btnY = 196;
    int spacing = 58;
    float btnDelay0 = titleTotalDur + 0.15f;

    Vector2 mouse = Win32GetMousePosition();

    const char *btnLabels[] = { S(STR_BTN_NEW_GAME), S(STR_BTN_LOAD_GAME), S(STR_BTN_HOST_GAME), S(STR_BTN_JOIN_GAME), S(STR_BTN_SETTINGS), S(STR_BTN_QUIT) };
    int btnCount = 6;
    bool hasAnySave = false;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        SaveSlotInfo info;
        if (GetSlotInfo(i, &info) && info.exists) { hasAnySave = true; break; }
    }
    bool btnEnabled[] = { true, hasAnySave, true, true, true, true };

    for (int i = 0; i < btnCount; i++) {
        // Staggered entrance
        float btnDelay = btnDelay0 + i * 0.08f;
        float btnProgress = (elapsed - btnDelay) / 0.28f;
        if (btnProgress < 0.0f) continue;
        if (btnProgress > 1.0f) btnProgress = 1.0f;
        // Ease out quad
        float bp = 1.0f - (1.0f - btnProgress) * (1.0f - btnProgress);
        float slideX = 0.0f;
        float btnAlpha = bp;

        int by = btnY + i * spacing;
        int drawX = btnX + (int)slideX;

        bool hover = CheckCollisionPointRec(mouse, (Rectangle){(float)drawX, (float)by, (float)btnW, (float)btnH});
        bool sel = (menuSelection == i) && btnEnabled[i];

        DrawUiButton(drawX, by, btnW, btnH, btnLabels[i], 19,
                     hover && btnEnabled[i], sel, btnEnabled[i], btnAlpha);
    }

    // ================================================================
    // Party mode confetti (Konami code easter egg)
    // ================================================================
    if (menuPartyMode) {
        static float confettiX[70], confettiY[70], confettiS[70], confettiHue[70];
        static bool confettiInit = false;
        if (!confettiInit) {
            for (int i = 0; i < 70; i++) {
                confettiX[i] = (float)(rand() % SCREEN_WIDTH);
                confettiY[i] = (float)(rand() % SCREEN_HEIGHT);
                confettiS[i] = 60.0f + (float)(rand() % 90);   // fall speed
                confettiHue[i] = (float)(rand() % 360);        // 0-360 hue seed
            }
            confettiInit = true;
        }
        for (int i = 0; i < 70; i++) {
            confettiY[i] += confettiS[i] * dt;
            confettiX[i] += sinf(time * 1.5f + i) * 24.0f * dt;
            if (confettiY[i] > SCREEN_HEIGHT + 4) {
                confettiY[i] = -6.0f;
                confettiX[i] = (float)(rand() % SCREEN_WIDTH);
            }
            // Cycle through a colorful palette by hue seed
            unsigned char cr, cg, cb;
            float h = confettiHue[i] + time * 40.0f;
            if (h >= 360.0f) h -= 360.0f;
            if (h < 60.0f)      { cr = 255; cg = (unsigned char)(h * 4.25f);     cb = 40; }
            else if (h < 120.0f){ cr = (unsigned char)(255 - (h - 60.0f) * 4.25f); cg = 255; cb = 40; }
            else if (h < 180.0f){ cr = 40; cg = 255; cb = (unsigned char)((h - 120.0f) * 4.25f); }
            else if (h < 240.0f){ cr = 40; cg = (unsigned char)(255 - (h - 180.0f) * 4.25f); cb = 255; }
            else if (h < 300.0f){ cr = (unsigned char)((h - 240.0f) * 4.25f); cg = 40; cb = 255; }
            else                { cr = 255; cg = 40; cb = (unsigned char)(255 - (h - 300.0f) * 4.25f); }
            int cs = 3 + (i % 3);
            DrawRectangle((int)confettiX[i], (int)confettiY[i], cs, cs, (Color){cr, cg, cb, 200});
        }
    }

    // ================================================================
    // Bottom hints — refined
    // ================================================================
    float hintFade = (elapsed - btnDelay0 - btnCount * 0.08f - 0.25f) / 0.4f;
    if (hintFade > 0.0f) {
        if (hintFade > 1.0f) hintFade = 1.0f;
        unsigned char a = (unsigned char)(hintFade * 235);
        const char *hint1 = S(STR_HINT_NAVIGATE);
        int hint1W = MeasureGameTextWidth(hint1, 12);
        int h1x = (SCREEN_WIDTH - hint1W) / 2;
        DrawGameText(hint1, h1x + 1, SCREEN_HEIGHT - 53, 14, (Color){0, 0, 0, a});
        DrawGameText(hint1, h1x, SCREEN_HEIGHT - 54, 14, (Color){235, 235, 235, a});
        const char *hint2 = S(STR_HINT_CONTROLS);
        int h2x = (SCREEN_WIDTH - MeasureGameTextWidth(hint2, 12)) / 2;
        DrawGameText(hint2, h2x + 1, SCREEN_HEIGHT - 37, 14, (Color){0, 0, 0, a});
        DrawGameText(hint2, h2x, SCREEN_HEIGHT - 38, 14, (Color){225, 225, 225, a});
    }

    // Version text intentionally omitted from the playable UI; keep the footer quiet.

}

//----------------------------------------------------------------------------------
// Save Slot Selection Screen
//----------------------------------------------------------------------------------
void DrawSlotSelectScreen(void)
{

    // Background: flat MC dark backdrop
    DrawDirtBackground(40);

    // ---- Layout: size the panel to its content and center it ----
    int slotW = 360, slotH = 78;
    int spacing = 88;
    int isNew = (slotSelectMode == 0);

    int titleH   = 92;                       // title + subtitle block
    int seedH    = isNew ? 46 : 0;           // seed + game-mode row
    int listH    = SLOT_VISIBLE * spacing;
    int panelW   = 520;
    int panelH   = titleH + seedH + listH + 56;
    int panelX   = (SCREEN_WIDTH - panelW) / 2;
    int panelY   = (SCREEN_HEIGHT - panelH) / 2;
    DrawUiPanel(panelX, panelY, panelW, panelH, 255);

    int contentX = panelX + 28;
    int slotX    = panelX + (panelW - slotW) / 2;

    // Title — dark text with a light outline: the panel behind it is light gray
    const char *title = isNew ? S(STR_NEW_GAME_TITLE) : S(STR_LOAD_GAME_TITLE);
    int titleSize = 42;
    int titleW = MeasureGameTextWidth(title, titleSize);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    int titleY = panelY + 18;
    {
        Color ol = {255, 255, 255, 200};
        for (int dy = -2; dy <= 2; dy += 2)
            for (int dx = -2; dx <= 2; dx += 2)
                if (dx || dy) DrawGameText(title, titleX + dx, titleY + dy, titleSize, ol);
        DrawGameText(title, titleX, titleY, titleSize, (Color){58, 58, 58, 255});
    }

    // Subtitle
    const char *sub = isNew ? S(STR_NEW_GAME_SUB) : S(STR_LOAD_GAME_SUB);
    int subW = MeasureGameTextWidth(sub, 15);
    DrawGameText(sub, (SCREEN_WIDTH - subW) / 2, titleY + 50, 14, (Color){110, 110, 110, 220});

    Vector2 mouse = Win32GetMousePosition();

    int cursorY = panelY + titleH;

    // Seed input + game mode (new game mode only)
    if (isNew) {
        int seedBoxW = 200;   // matches the hit box in the input handler
        int seedBoxH = 24;
        int seedBoxX = contentX;
        int seedBoxY = cursorY;
        Rectangle seedBox = { (float)seedBoxX, (float)seedBoxY, (float)seedBoxW, (float)seedBoxH };
        bool seedHover = CheckCollisionPointRec(mouse, seedBox);

        DrawGameText(S(STR_SEED), seedBoxX, seedBoxY - 16, 14, (Color){90, 90, 90, 220});
        // Recessed MC text field
        DrawRectangle(seedBoxX, seedBoxY, seedBoxW, seedBoxH, (Color){60, 60, 60, 255});
        DrawRectangle(seedBoxX, seedBoxY, seedBoxW, 1, (Color){30, 30, 30, 255});
        DrawRectangle(seedBoxX, seedBoxY, 1, seedBoxH, (Color){30, 30, 30, 255});
        DrawRectangleLinesEx(seedBox, 1, seedFieldFocused ? (Color){255, 255, 255, 255} : (Color){120, 120, 120, 255});
        // Click to focus, click away to release. Focus is what routes typing
        // to this field (see the input handler); without it the field looked
        // interactive but was not.
        if (seedHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySoundUIClick();
            seedFieldFocused = true;
        } else if (!seedHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            seedFieldFocused = false;
        }

        if (seedInputLen > 0) {
            DrawGameText(seedInputBuf, seedBoxX + 6, seedBoxY + 5, 14, (Color){255, 255, 255, 255});
            // Blinking caret after the last character
            if (seedFieldFocused && fmodf((float)GetTime(), 1.0f) < 0.5f) {
                int caretX = seedBoxX + 6 + MeasureGameTextWidth(seedInputBuf, 14) + 1;
                DrawRectangle(caretX, seedBoxY + 5, 2, 14, (Color){235, 235, 235, 255});
            }
        } else {
            DrawGameText(S(STR_RANDOM), seedBoxX + 6, seedBoxY + 5, 14, (Color){150, 150, 150, 255});
            if (seedFieldFocused && fmodf((float)GetTime(), 1.0f) < 0.5f) {
                DrawRectangle(seedBoxX + 6, seedBoxY + 5, 2, 14, (Color){235, 235, 235, 255});
            }
        }
        // Game mode toggle to the right of the seed box
        int lblW = 40, survW = 72, creatW = 72, gap = 6;
        int modeX = seedBoxX + seedBoxW + 6 + 56 + 16;   // clear of the random button
        // Random-seed button sits just right of the field
        int rndBtnW = 56;
        int rndBtnX = seedBoxX + seedBoxW + 6;
        Rectangle rndBtn = { (float)rndBtnX, (float)seedBoxY, (float)rndBtnW, (float)seedBoxH };
        bool rndHover = CheckCollisionPointRec(mouse, rndBtn);
        DrawUiButton(rndBtn.x, rndBtn.y, rndBtn.width, rndBtn.height, S(STR_RANDOM), 13, rndHover, false, true, 1.0f);
        if (rndHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            PlaySoundUIClick();
            unsigned int rs = (unsigned int)(GetTime() * 100000.0) ^ (unsigned int)rand();
            if (rs == 0) rs = 1;
            snprintf(seedInputBuf, sizeof(seedInputBuf), "%u", rs);
            seedInputLen = (int)strlen(seedInputBuf);
        }
        DrawGameText(S(STR_GAMEMODE), modeX, seedBoxY + 6, 14, (Color){90, 90, 90, 220});
        Rectangle survRect = { (float)(modeX + lblW), (float)seedBoxY, (float)survW, (float)seedBoxH };
        Rectangle creatRect = { (float)(modeX + lblW + survW + gap), (float)seedBoxY, (float)creatW, (float)seedBoxH };
        bool survHover = CheckCollisionPointRec(mouse, survRect);
        bool creatHover = CheckCollisionPointRec(mouse, creatRect);
        DrawUiButton(survRect.x, survRect.y, survRect.width, survRect.height,
                     S(STR_MODE_SURVIVAL), 13, survHover, (pendingGameMode == GAME_SURVIVAL), true, 1.0f);
        DrawUiButton(creatRect.x, creatRect.y, creatRect.width, creatRect.height,
                     S(STR_MODE_CREATIVE), 13, creatHover, (pendingGameMode == GAME_CREATIVE), true, 1.0f);

        cursorY += seedH;
    }

    int slotY = cursorY;
    int listRight = slotX + slotW;

    // Draw visible slots
    for (int vi = 0; vi < SLOT_VISIBLE; vi++) {
        int i = vi + slotScrollOffset;
        if (i >= MAX_SAVE_SLOTS) break;

        SaveSlotInfo info;
        GetSlotInfo(i, &info);

        int sy = slotY + vi * spacing;
        Rectangle slotRect = { (float)slotX, (float)sy, (float)slotW, (float)slotH };
        bool hover = CheckCollisionPointRec(mouse, slotRect);
        bool sel = (menuSelection == i);
        float hA = GetHoverAlpha(60 + i, hover && !sel, GetFrameTime());

        // Determine if this slot is usable
        bool usable = isNew || info.exists;

        // Slot card: MC stone gray, brighter when selected/hovered
        Color fill;
        Color border = {85, 85, 85, 255};
        if (!usable) {
            fill = (Color){150, 150, 150, 160};
            border = (Color){110, 110, 110, 150};
        } else if (sel) {
            fill = (Color){205, 205, 205, 255};
            border = (Color){255, 255, 255, 255};
        } else if (hA > 0.01f) {
            fill = (Color){180, 180, 180, 255};
        } else {
            fill = (Color){160, 160, 160, 255};
        }

        // Beveled MC-style card
        DrawRectangle(slotX, sy, slotW, slotH, fill);
        DrawRectangle(slotX, sy, slotW, 2, (Color){215, 215, 215, fill.a});
        DrawRectangle(slotX, sy, 2, slotH, (Color){215, 215, 215, fill.a});
        DrawRectangle(slotX, sy + slotH - 2, slotW, 2, (Color){95, 95, 95, fill.a});
        DrawRectangle(slotX + slotW - 2, sy, 2, slotH, (Color){95, 95, 95, fill.a});
        DrawRectangleLines(slotX, sy, slotW, slotH, border);

        // Slot number
        char slotLabel[24];
        snprintf(slotLabel, sizeof(slotLabel), S(STR_SLOT), i + 1);
        Color labelColor = usable ? (Color){48, 48, 48, 255} : (Color){130, 130, 130, 190};
        DrawGameText(slotLabel, slotX + 14, sy + 12, 22, labelColor);

        if (info.exists) {
            char seedText[40];
            snprintf(seedText, sizeof(seedText), S(STR_SEED_DISPLAY), info.seed);
            DrawGameText(seedText, slotX + 14, sy + 40, 14, (Color){70, 70, 70, 235});

            char sizeText[32];
            snprintf(sizeText, sizeof(sizeText), "%dx%d", info.worldW, info.worldH);
            int sizeW = MeasureGameTextWidth(sizeText, 13);
            DrawGameText(sizeText, slotX + slotW - sizeW - 14, sy + 40, 14, (Color){70, 70, 70, 235});

            // "已占用" badge with a small grass icon, top-right
            const char *status = S(STR_OCCUPIED);
            int statusW = MeasureGameTextWidth(status, 13);
            int badgeW = statusW + 34;
            int badgeX = slotX + slotW - badgeW - 12;
            DrawRectangle(badgeX, sy + 12, badgeW, 20, (Color){0, 0, 0, 45});
            DrawRectangleLines(badgeX, sy + 12, badgeW, 20, (Color){60, 130, 60, 200});
            if (blockAtlas.id > 0) {
                Rectangle src = { (float)(BLOCK_GRASS * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
                Rectangle dst = { (float)(badgeX + 5), (float)(sy + 15), 14, 14 };
                DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
            }
            DrawGameText(status, badgeX + 24, sy + 16, 14, (Color){40, 120, 40, 255});
        } else {
            const char *emptyText = isNew ? S(STR_EMPTY_NEW) : S(STR_EMPTY_LOAD);
            Color emptyColor = isNew ? (Color){75, 75, 75, 220} : (Color){120, 120, 120, 190};
            DrawGameText(emptyText, slotX + 14, sy + 36, 16, emptyColor);
        }
    }

    // Scrollbar (drawn inside the panel, right of the list)
    if (MAX_SAVE_SLOTS > SLOT_VISIBLE) {
        int trackX = listRight + 10;
        int trackY = slotY + 2;
        int trackH = listH - 12;
        DrawRectangle(trackX, trackY, 6, trackH, (Color){70, 70, 70, 255});
        float viewRatio = (float)SLOT_VISIBLE / MAX_SAVE_SLOTS;
        int maxScroll = MAX_SAVE_SLOTS - SLOT_VISIBLE;
        float scrollRatio = maxScroll > 0 ? (float)slotScrollOffset / maxScroll : 0;
        int thumbH = (int)(trackH * viewRatio);
        if (thumbH < 20) thumbH = 20;
        int thumbY = trackY + (int)((trackH - thumbH) * scrollRatio);
        DrawRectangle(trackX, thumbY, 6, thumbH, (Color){170, 170, 170, 255});
    }

    // Footer: back button right-aligned; hint centered below the panel
    int backW = 100, backH2 = 28;
    int backX = panelX + panelW - backW - 28;
    int backY = panelY + panelH - 40;
    Rectangle backRect = { (float)backX, (float)backY, (float)backW, (float)backH2 };
    bool backHover = CheckCollisionPointRec(mouse, backRect);
    DrawUiButton(backRect.x, backRect.y, backRect.width, backRect.height,
                 S(STR_BACK), 14, backHover, false, true, 1.0f);

    // Hint line below the panel (white with MC shadow so it reads on dirt)
    {
        const char *hint = S(STR_HINT_SLOT);
        int hw = MeasureGameTextWidth(hint, 12);
        int hx = (SCREEN_WIDTH - hw) / 2;
        int hy = panelY + panelH + 8;
        DrawGameText(hint, hx + 1, hy + 1, 14, (Color){0, 0, 0, 200});
        DrawGameText(hint, hx, hy, 14, (Color){235, 235, 235, 235});
    }
}

//----------------------------------------------------------------------------------
// Confirmation Dialog (overwrite / delete save)
//----------------------------------------------------------------------------------
void DrawConfirmDialog(void)
{
    // Dark overlay
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, (Color){0, 0, 0, 180});

    int dlgW = 340, dlgH = 160;
    int dlgX = (SCREEN_WIDTH - dlgW) / 2;
    int dlgY = (SCREEN_HEIGHT - dlgH) / 2;

    bool isDelete = (confirmDialogMode == 1);

    // Dialog box — modern dark flat panel
    DrawUiPanel(dlgX, dlgY, dlgW, dlgH, 255);
    // Top accent (danger/amber by mode)
    Color accentBar = isDelete ? (Color){232, 87, 92, 220} : (Color){240, 190, 90, 220};
    DrawRectangle(dlgX + 10, dlgY, dlgW - 20, 2, accentBar);

    // Title
    const char *title = isDelete ? S(STR_DELETE_SAVE) : S(STR_OVERWRITE_SAVE);
    Color titleColor = isDelete ? (Color){240, 100, 90, 255} : (Color){240, 190, 90, 255};
    int titleW = MeasureGameTextWidth(title,24);
    DrawGameText(title, dlgX + (dlgW - titleW) / 2, dlgY + 18,24, titleColor);

    // Message
    char msg[64];
    snprintf(msg, sizeof(msg), S(STR_SLOT_HAS_DATA), confirmDialogSlot + 1);
    int msgW = MeasureGameTextWidth(msg,16);
    DrawGameText(msg, dlgX + (dlgW - msgW) / 2, dlgY + 52,14, (Color){62, 62, 62, 255});

    const char *warn = S(STR_CANNOT_UNDO);
    int warnW = MeasureGameTextWidth(warn,14);
    DrawGameText(warn, dlgX + (dlgW - warnW) / 2, dlgY + 74,14, (Color){210, 130, 110, 220});

    // Buttons
    Vector2 mouse = Win32GetMousePosition();
    int btnW = 130, btnH = 34;
    int btnY = dlgY + dlgH - 50;

    // Yes button (MC-style, same rect as hit-testing)
    {
        Rectangle r = { (float)(dlgX + 30), (float)btnY, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, r);
        const char *t = isDelete ? S(STR_YES_DELETE) : S(STR_YES_OVERWRITE);
        DrawUiButton(r.x, r.y, r.width, r.height, t, 16, hover, false, true, 1.0f);
    }

    // No button (MC-style, same rect as hit-testing)
    {
        Rectangle r = { (float)(dlgX + dlgW - btnW - 30), (float)btnY, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, r);
        DrawUiButton(r.x, r.y, r.width, r.height, S(STR_CANCEL), 16, hover, false, true, 1.0f);
    }

    // Key hints
    const char *keyHint = S(STR_CONFIRM_KEYS);
    int keyHintW = MeasureGameTextWidth(keyHint,13);
    DrawGameText(keyHint, dlgX + (dlgW - keyHintW) / 2, dlgY + dlgH - 14, 14, (Color){95, 95, 95, 235});
}

//----------------------------------------------------------------------------------
// Settings Screen
//----------------------------------------------------------------------------------
void DrawSettingsScreen(void)
{
    // Background: flat MC dark backdrop
    DrawDirtBackground(40);

    // Title — white with a full dark outline so it reads on the dirt backdrop
    const char *title = S(STR_SETTINGS);
    int titleW = MeasureGameTextWidth(title, 42);
    int titleX = (SCREEN_WIDTH - titleW) / 2;
    {
        Color ol = {40, 40, 40, 255};
        for (int dy = -2; dy <= 2; dy += 2)
            for (int dx = -2; dx <= 2; dx += 2)
                if (dx || dy) DrawGameText(title, titleX + dx, 20 + dy, 42, ol);
        DrawGameText(title, titleX, 20, 42, (Color){240, 240, 240, 255});
    }
    // Hard separator under the title
    DrawRectangle((SCREEN_WIDTH - 160) / 2, 66, 160, 2, (Color){120, 120, 120, 180});

    Vector2 mouse = Win32GetMousePosition();

    // Settings panel
    int panelW = 520;
    int panelH = 644;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = 84;

    // Panel shadow
    DrawUiBox(panelX + 3, panelY + 4, panelW, panelH, 0.03f, (Color){0, 0, 0, 45});
    // Modern panel: rounded corners, dark fill, subtle border
    DrawUiPanel(panelX, panelY, panelW, panelH, 255);
    DrawRectangle(panelX + 12, panelY, panelW - 24, 1, (Color){255, 255, 255, 16});
    int leftX = panelX + 30;
    int rightX = panelX + panelW / 2 + 10;

    // ============================================================
    // Section: Audio
    // ============================================================
    int sectionY = panelY + 14;
    // Section header
    DrawGameText(S(STR_SECTION_AUDIO), leftX, sectionY, 14, (Color){70, 70, 70, 220});
    DrawRectangle(leftX, sectionY + 18, panelW - 60, 1, (Color){90, 90, 90, 100});
    sectionY += 32;

    // Music Volume
    DrawGameText(S(STR_MUSIC_VOLUME), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    int musicSliderX = leftX + MeasureGameTextWidth(S(STR_MUSIC_VOLUME), 13) + 12;
    int musicSliderW = panelX + panelW - 30 - musicSliderX - 45;
    {
        Rectangle bgmTrack = { (float)(musicSliderX - 10), (float)(sectionY - 4), (float)(musicSliderW + 20), 26 };
        bool bgmHover = CheckCollisionPointRec(mouse, bgmTrack);
        extern float bgmVolumeSlider;
        if (win32LMB && bgmHover) {
            bgmVolumeSlider = (mouse.x - musicSliderX) / (float)musicSliderW;
            if (bgmVolumeSlider < 0.0f) bgmVolumeSlider = 0.0f;
            if (bgmVolumeSlider > 1.0f) bgmVolumeSlider = 1.0f;
            SetBGMVolume(bgmVolumeSlider);
        }
        DrawUiSlider(musicSliderX, sectionY + 9, musicSliderW, bgmVolumeSlider, bgmHover, 1.0f);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)(bgmVolumeSlider * 100));
        DrawGameText(buf, musicSliderX + musicSliderW + 10, sectionY + 2, 14, (Color){70, 70, 70, 240});
    }
    sectionY += 26;
    // SFX Volume
    DrawGameText(S(STR_SOUND_EFFECTS), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    int sfxSliderX = leftX + MeasureGameTextWidth(S(STR_SOUND_EFFECTS), 13) + 12;
    int sfxSliderW = panelX + panelW - 30 - sfxSliderX - 45;
    {
        Rectangle sfxTrack = { (float)(sfxSliderX - 10), (float)(sectionY - 4), (float)(sfxSliderW + 20), 26 };
        bool sfxHover = CheckCollisionPointRec(mouse, sfxTrack);
        extern float sfxVolumeSlider;
        if (win32LMB && sfxHover) {
            sfxVolumeSlider = (mouse.x - sfxSliderX) / (float)sfxSliderW;
            if (sfxVolumeSlider < 0.0f) sfxVolumeSlider = 0.0f;
            if (sfxVolumeSlider > 1.0f) sfxVolumeSlider = 1.0f;
            SetSFXVolume(sfxVolumeSlider);
        }
        DrawUiSlider(sfxSliderX, sectionY + 9, sfxSliderW, sfxVolumeSlider, sfxHover, 1.0f);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%%", (int)(sfxVolumeSlider * 100));
        DrawGameText(buf, sfxSliderX + sfxSliderW + 10, sectionY + 2, 14, (Color){70, 70, 70, 240});
    }
    sectionY += 26;

    // ============================================================
    // Section: Display
    // ============================================================
    DrawGameText(S(STR_SECTION_DISPLAY), leftX, sectionY, 14, (Color){70, 70, 70, 220});
    DrawRectangle(leftX, sectionY + 18, panelW - 60, 1, (Color){90, 90, 90, 100});
    sectionY += 28;
    DrawGameText(S(STR_WINDOW_MODE), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    sectionY += 20;

    int btnW = 100;
    int btnH = 28;
    int btnGap = 6;
    const char *modeNames[] = { S(STR_WINDOWED), S(STR_FULLSCREEN), S(STR_BORDERLESS) };

    for (int i = 0; i < 3; i++) {
        int bx = leftX + i * (btnW + btnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)btnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (windowMode == i);

        DrawUiButton(bx, sectionY, btnW, btnH, modeNames[i], 13, hover, sel, true, 1.0f);

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            ApplyWindowMode(i);
            PlaySoundUIClick();
        }
    }
    sectionY += btnH + 10;

    // Resolution presets keep the 1280x720 logical canvas intact.
    DrawGameText(S(STR_RESOLUTION), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    sectionY += 20;
    const char *resolutionNames[] = { S(STR_RES_960), S(STR_RES_1280), S(STR_RES_1600) };
    int resolutionBtnW = 145;
    for (int i = 0; i < 3; i++) {
        int bx = leftX + i * (resolutionBtnW + btnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)resolutionBtnW, (float)btnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (resolutionPreset == i);
        DrawUiButton(bx, sectionY, resolutionBtnW, btnH, resolutionNames[i], 13, hover, sel, true, 1.0f);
        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            ApplyResolution(i);
            PlaySoundUIClick();
        }
    }
    sectionY += btnH + 10;
    Vector2 dpiScale = GetWindowScaleDPI();
    int mon = GetCurrentMonitor();
    int monW = GetMonitorWidth(mon);
    int monH = GetMonitorHeight(mon);
    char dpiInfo[64];
    snprintf(dpiInfo, sizeof(dpiInfo), "Monitor: %dx%d  DPI: %.0f%%", monW, monH, dpiScale.x * 100);
    DrawGameText(dpiInfo, leftX, sectionY, 14, (Color){95, 95, 95, 180});

    char resInfo[64];
    snprintf(resInfo, sizeof(resInfo), "Window: %dx%d", GetScreenWidth(), GetScreenHeight());
    DrawGameText(resInfo, rightX, sectionY, 14, (Color){95, 95, 95, 180});
    sectionY += 26;

    // ============================================================
    // Section: Language & Font
    // ============================================================
    DrawGameText(S(STR_SECTION_LANGUAGE), leftX, sectionY, 14, (Color){70, 70, 70, 220});
    DrawRectangle(leftX, sectionY + 18, panelW - 60, 1, (Color){90, 90, 90, 100});
    sectionY += 32;

    // Language buttons
    DrawGameText(S(STR_LANGUAGE), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    sectionY += 18;

    const char *langNames[] = { S(STR_LANG_NAME_EN), S(STR_LANG_NAME_ZH), S(STR_LANG_NAME_JA) };
    int langBtnW = 100;
    int langBtnH = 26;
    int langBtnGap = 6;

    for (int i = 0; i < LANG_COUNT; i++) {
        int bx = leftX + i * (langBtnW + langBtnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)langBtnW, (float)langBtnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (language == (Language)i);

        DrawUiButton(bx, sectionY, langBtnW, langBtnH, langNames[i], 13, hover, sel, true, 1.0f);

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            language = (Language)i;
            PlaySoundUIClick();
        }
    }
    sectionY += langBtnH + 14;

    // Font buttons
    DrawGameText(S(STR_FONT), leftX, sectionY, 14, (Color){70, 70, 70, 230});
    sectionY += 24;

    const char *fontNames[] = { S(STR_FONT_NAME_BUILTIN), S(STR_FONT_NAME_LXGW) };
    int fontBtnW = 130;
    int fontBtnH = 26;
    int fontBtnGap = 6;

    for (int i = 0; i < 2; i++) {
        int bx = leftX + i * (fontBtnW + fontBtnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)fontBtnW, (float)fontBtnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (i == 0 && !useCustomFont) || (i == 1 && useCustomFont);

        DrawUiButton(bx, sectionY, fontBtnW, fontBtnH, fontNames[i], 13, hover, sel, true, 1.0f);

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            if (i == 0) {
                UnloadGameFont();
            } else {
                ReloadGameFont("assets/fonts/LXGWWenKaiLite-Regular.ttf");
            }
            PlaySoundUIClick();
        }
    }
    sectionY += fontBtnH + 22;

    // ============================================================
    // Section: Difficulty
    // ============================================================
    DrawGameText(S(STR_DIFFICULTY), leftX, sectionY, 14, (Color){70, 70, 70, 220});
    DrawRectangle(leftX, sectionY + 18, panelW - 60, 1, (Color){90, 90, 90, 100});
    sectionY += 32;

    const char *diffNames[] = {
        S(STR_DIFFICULTY_PEACEFUL), S(STR_DIFFICULTY_EASY),
        S(STR_DIFFICULTY_NORMAL), S(STR_DIFFICULTY_HARD)
    };
    int diffBtnW = 110;
    int diffBtnH = 26;
    int diffBtnGap = 6;

    for (int i = 0; i < DIFFICULTY_COUNT; i++) {
        int bx = leftX + i * (diffBtnW + diffBtnGap);
        Rectangle btn = { (float)bx, (float)sectionY, (float)diffBtnW, (float)diffBtnH };
        bool hover = CheckCollisionPointRec(mouse, btn);
        bool sel = (gameDifficulty == (Difficulty)i);

        DrawUiButton(bx, sectionY, diffBtnW, diffBtnH, diffNames[i], 13, hover, sel, true, 1.0f);

        if (hover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !sel) {
            gameDifficulty = (Difficulty)i;
            PlaySoundUIClick();
        }
    }
    sectionY += diffBtnH + 22;

    // ============================================================
    // Section: Controls
    // ============================================================
    DrawGameText(S(STR_CONTROLS_TITLE), leftX, sectionY, 14, (Color){70, 70, 70, 220});
    DrawRectangle(leftX, sectionY + 18, panelW - 60, 1, (Color){90, 90, 90, 100});
    sectionY += 26;

    const char *controls[] = {
        S(STR_KEY_WASD),    S(STR_ACT_MOVE),
        S(STR_KEY_SPACE),   S(STR_ACT_JUMP),
        S(STR_KEY_CTRL),    S(STR_ACT_SPRINT),
        S(STR_KEY_SHIFT),   S(STR_ACT_SNEAK),
        S(STR_KEY_LCLICK),  S(STR_ACT_BREAK),
        S(STR_KEY_RCLICK),  S(STR_ACT_PLACE),
        S(STR_KEY_E),       S(STR_ACT_INVENTORY),
        S(STR_KEY_H),       S(STR_ACT_HEAL),
        S(STR_KEY_F3),      S(STR_ACT_DEBUG),
        S(STR_KEY_F11),     S(STR_ACT_FULLSCREEN),
        S(STR_KEY_ESC),     S(STR_ACT_PAUSE),
        S(STR_KEY_19),      S(STR_ACT_HOTBAR)
    };
    int numControls = sizeof(controls) / sizeof(controls[0]) / 2;
    // Three columns keep the list short enough to clear the Back button.
    const int ctrlCols = 3;
    int ctrlRows = (numControls + ctrlCols - 1) / ctrlCols;
    int ctrlColW = (panelW - 60) / ctrlCols;
    for (int i = 0; i < numControls; i++) {
        int col = i / ctrlRows;
        int row = i % ctrlRows;
        int kx = leftX + col * ctrlColW;
        int yy = sectionY + row * 15;
        DrawGameText(controls[i * 2], kx, yy, 14, (Color){70, 70, 70, 255});
        DrawGameText(controls[i * 2 + 1], kx + 62, yy, 14, (Color){95, 95, 95, 255});
    }
    sectionY += ctrlRows * 15;

    // Back button (MC-style)
    int backBtnW = 130;
    int backBtnH = 30;
    int backBtnX = (SCREEN_WIDTH - backBtnW) / 2;
    int backBtnY = sectionY + 12;
    Rectangle backBtn = { (float)backBtnX, (float)backBtnY, (float)backBtnW, (float)backBtnH };
    bool backHover = CheckCollisionPointRec(mouse, backBtn);
    DrawUiButton(backBtnX, backBtnY, backBtnW, backBtnH, S(STR_BACK), 15, backHover, false, true, 1.0f);

    // ESC to go back
    if (Win32IsKeyPressed(KEY_ESCAPE) || (backHover && Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        StartTransition(STATE_MENU);
        PlaySoundUIClick();
    }
}

//----------------------------------------------------------------------------------
// Enchanting Table UI
//----------------------------------------------------------------------------------
static const char* GetEnchantName(EnchantmentType type) {
    switch (type) {
        case ENCH_SHARPNESS:    return S(STR_ENCH_SHARPNESS);
        case ENCH_EFFICIENCY:   return S(STR_ENCH_EFFICIENCY);
        case ENCH_PROTECTION:   return S(STR_ENCH_PROTECTION);
        case ENCH_FORTUNE:      return S(STR_ENCH_FORTUNE);
        case ENCH_UNBREAKING:   return S(STR_ENCH_UNBREAKING);
        case ENCH_SILK_TOUCH:   return S(STR_ENCH_SILK_TOUCH);
        case ENCH_POWER:        return S(STR_ENCH_POWER);
        default:                return "Unknown";
    }
}

void DrawEnchantingTableUI(void)
{
    if (!localEnchantSession.open || localEnchantSession.optionCount == 0) return;

    int panelW = 500, panelH = 320;
    int panelX = (SCREEN_WIDTH - panelW) / 2;
    int panelY = (SCREEN_HEIGHT - panelH) / 2;

    // Panel background
    DrawRectangle(panelX, panelY, panelW, panelH, (Color){25, 20, 40, 245});
    DrawRectangleLines(panelX, panelY, panelW, panelH, (Color){100, 60, 180, 200});

    // Title
    DrawGameText(S(STR_BLOCK_ENCHANTING_TABLE), panelX + panelW / 2 - 60, panelY + 10, 14, (Color){180, 120, 255, 255});

    // Item preview slot (left side)
    int slotX = panelX + 20;
    int slotY = panelY + 60;
    int slotSize = 48;
    DrawRectangle(slotX, slotY, slotSize, slotSize, (Color){40, 35, 55, 255});
    DrawRectangleLines(slotX, slotY, slotSize, slotSize, (Color){120, 100, 180, 200});
    // Draw the held item in the slot
    if (localEnchantSession.heldItem != BLOCK_AIR && blockAtlas.id > 0) {
        Rectangle src = { (float)(localEnchantSession.heldItem * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)(slotX + 8), (float)(slotY + 8), (float)(32), (float)(32) };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, WHITE);
    }
    DrawGameText(GetBlockName((BlockType)localEnchantSession.heldItem), slotX + slotSize + 8, slotY + 18, 11, (Color){200, 200, 200, 255});

    // XP available
    DrawGameText(TextFormat("XP: %d", player.xp), panelX + panelW / 2 - 30, panelY + 270, 14, (Color){80, 220, 80, 200});

    // 3 enchantment options (right side, stacked vertically)
    int optX = panelX + 190;
    int optY = panelY + 55;
    int optH = 70;
    int optW = panelW - 210;
    int optPad = 6;

    for (int i = 0; i < localEnchantSession.optionCount; i++) {
        EnchantOption *eo = &localEnchantSession.options[i];
        if (eo->type == ENCH_NONE) continue;

        int oy = optY + i * (optH + optPad);
        int canAfford = (player.xp >= eo->xpCost);

        // Option box
        Color bgColor = canAfford ? (Color){45, 35, 70, 255} : (Color){35, 30, 40, 255};
        DrawRectangle(optX, oy, optW, optH, bgColor);
        DrawRectangleLines(optX, oy, optW, optH,
            canAfford ? (Color){140, 80, 220, 220} : (Color){80, 60, 80, 160});

        // Purple glow border for available
        if (canAfford) {
            DrawRectangleLines(optX - 1, oy - 1, optW + 2, optH + 2, (Color){160, 100, 240, 80});
        }

        // Enchantment name
        const char *enchName = GetEnchantName(eo->type);
        DrawGameText(enchName, optX + 10, oy + 8, 14, canAfford ? (Color){200, 160, 255, 255} : (Color){120, 100, 140, 200});

        // Level
        DrawGameText(TextFormat("Lv.%d", eo->level), optX + 10, oy + 26, 12, canAfford ? (Color){180, 140, 240, 220} : (Color){100, 80, 120, 180});

        // XP cost
        if (canAfford) {
            DrawGameText(TextFormat("%d XP", eo->xpCost), optX + optW - 80, oy + 26, 14, (Color){80, 220, 80, 220});
        } else {
            DrawGameText(TextFormat("Need %d XP", eo->xpCost), optX + optW - 100, oy + 26, 14, (Color){220, 80, 80, 200});
        }

        // Click detection
        bool hover = Win32IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                     Win32GetMousePosition().x >= optX && Win32GetMousePosition().x <= optX + optW &&
                     Win32GetMousePosition().y >= oy && Win32GetMousePosition().y <= oy + optH;

        if (hover && canAfford) {
            if (NetIsClient()) {
                // Send enchant request to host
                uint8_t buf[64];
                PktEnchantRequest ereq;
                ereq.optionIndex = i;
                ereq.blockX = localEnchantSession.blockX;
                ereq.blockY = localEnchantSession.blockY;
                ereq.enchantType = eo->type;
                ereq.enchantLevel = eo->level;
                ereq.xpCost = eo->xpCost;
                buf[0] = PKT_ENCHANT_REQUEST;
                memcpy(buf + 1, &ereq, sizeof(PktEnchantRequest));
                NetSendToServer(buf, 1 + sizeof(PktEnchantRequest), true);
                // Close UI immediately (host response will sync inventory)
                localEnchantSession.open = false;
                inventoryOpen = false;
                localEnchantSession.optionCount = 0;
            } else {
            // Apply enchantment locally
            player.xp -= eo->xpCost;
            player.itemEnchantments[localEnchantSession.heldItemSlot] = ENCH_PACK(eo->type, eo->level);
            UnlockAchievement(ACH_ENCHANTER);

            // Restore durability
            int maxDur = IsTool((BlockType)localEnchantSession.heldItem) ? GetToolMaxDurability((BlockType)localEnchantSession.heldItem) :
                         IsArmor((BlockType)localEnchantSession.heldItem) ? GetArmorMaxDurability((BlockType)localEnchantSession.heldItem) : 0;
            if (IsTool((BlockType)localEnchantSession.heldItem)) {
                player.toolDurability[localEnchantSession.heldItemSlot] = maxDur;
            }

            PlaySoundCraft();
            ShowMessage(S(STR_MSG_ENCHANTED), (Color){180, 120, 255, 255});

            // Close UI
            localEnchantSession.open = false;
            inventoryOpen = false;
            localEnchantSession.optionCount = 0;
            }
        }
    }
}

void DrawChatUI(void)
{
    int x = 10;
    int y = SCREEN_HEIGHT - 220;
    int w = 500;
    int lineH = 18;
    int maxVisible = 10;
    int maxW = w - 8;

    // Draw recent messages (wrapped onto multiple lines so long text never overflows)
    int shown = 0;
    for (int i = chatHistoryCount - 1; i >= 0 && shown < maxVisible; i--) {
        ChatMessage *cm = &chatHistory[i];
        if (!chatOpen && cm->timer <= 0.0f) continue;
        const char *name = "System";
        bool isPlayer = cm->playerId < MAX_NET_PLAYERS;
        Color nameColor = (Color){170, 170, 170, 255};   // system messages
        if (isPlayer) {
            name = players[cm->playerId].playerName;
            // Own messages green, other players blue (MC chat convention)
            if (cm->playerId == localPlayerId) nameColor = (Color){100, 220, 100, 255};
            else nameColor = (Color){100, 180, 220, 255};
        }

        // Compose full line text
        char full[160];
        if (isPlayer) snprintf(full, sizeof(full), "%s: %s", name, cm->message);
        else snprintf(full, sizeof(full), "%s", cm->message);

        // Wrap into lines that fit the chat box width
        char lines[10][160];
        int lineCount = 0;
        {
            char buf[160];
            int blen = 0;
            size_t flen = strlen(full);
            for (size_t k = 0; k < flen && lineCount < 10; k++) {
                // Determine UTF-8 char length
                unsigned char c = (unsigned char)full[k];
                int clen = 1;
                if ((c & 0xE0) == 0xC0) clen = 2;
                else if ((c & 0xF0) == 0xE0) clen = 3;
                else if ((c & 0xF8) == 0xF0) clen = 4;
                if (k + clen > flen) clen = 1;
                // Test if adding this char overflows
                char test[160];
                memcpy(test, buf, blen);
                test[blen] = 0;
                memcpy(test + blen, full + k, clen);
                test[blen + clen] = 0;
                if (blen > 0 && MeasureGameTextWidth(test, 12) > maxW) {
                    memcpy(lines[lineCount], buf, blen);
                    lines[lineCount][blen] = 0;
                    lineCount++;
                    blen = 0;
                }
                memcpy(buf + blen, full + k, clen);
                blen += clen;
            }
            if (lineCount < 10) {
                memcpy(lines[lineCount], buf, blen);
                lines[lineCount][blen] = 0;
                lineCount++;
            }
        }

        // Draw lines bottom-up (last continuation line lowest)
        int alpha = 255;
        if (!chatOpen && cm->timer < 3.0f) alpha = (int)(255 * cm->timer / 3.0f);
        if (alpha < 0) alpha = 0;
        for (int li = lineCount - 1; li >= 0 && shown < maxVisible; li--, shown++) {
            int ly = y - shown * lineH;
            DrawRectangle(x, ly, w, lineH, (Color){0, 0, 0, (unsigned char)(140 * alpha / 255)});
            // First line carries the coloured "<name>: " prefix, the rest is body text
            if (li == 0 && isPlayer) {
                int nameW = MeasureGameTextWidth(name, 14);
                DrawGameText(name, x + 4, ly + 2, 14, (Color){nameColor.r, nameColor.g, nameColor.b, (unsigned char)alpha});
                DrawGameText(":", x + 4 + nameW, ly + 2, 14, (Color){200, 200, 200, (unsigned char)alpha});
                // Body starts after the "name: " prefix produced above
                const char *body = lines[li];
                int prefixLen = (int)strlen(name) + 2;   // name + ": "
                if ((int)strlen(body) >= prefixLen) body += prefixLen;
                DrawGameText(body, x + 10 + nameW, ly + 2, 14, (Color){230, 230, 230, (unsigned char)alpha});
            } else {
                DrawGameText(lines[li], x + 4, ly + 2, 14, (Color){230, 230, 230, (unsigned char)alpha});
            }
        }
    }

    // Draw chat input bar when open
    if (chatOpen) {
        int inputY = SCREEN_HEIGHT - 40;
        DrawRectangle(x, inputY, w, 28, (Color){0, 0, 0, 200});
        DrawRectangleLines(x, inputY, w, 28, (Color){120, 120, 140, 255});
        DrawGameText(TextFormat("%s%s", chatInput, ((int)(GetTime() * 2) % 2 == 0) ? "_" : ""),
                     x + 6, inputY + 6, 14, WHITE);
    }
}

//----------------------------------------------------------------------------------
// Background (parallax clouds + mountains)
//----------------------------------------------------------------------------------
void DrawBackground(void)
{
    float camX = camera.target.x;
    float lerp = dayNight.lightLevel;

    // Sky gradient (top to bottom)
    {
        unsigned char tr = (unsigned char)(60 + lerp * 80);
        unsigned char tg = (unsigned char)(80 + lerp * 120);
        unsigned char tb = (unsigned char)(140 + lerp * 100);
        unsigned char br = (unsigned char)(100 + lerp * 100);
        unsigned char bg = (unsigned char)(130 + lerp * 90);
        unsigned char bb = (unsigned char)(180 + lerp * 60);
        for (int y = 0; y < 200; y += 4) {
            float t = y / 200.0f;
            unsigned char r = (unsigned char)(tr + (br - tr) * t);
            unsigned char g = (unsigned char)(tg + (bg - tg) * t);
            unsigned char b = (unsigned char)(tb + (bb - tb) * t);
            DrawRectangle(0, y, SCREEN_WIDTH, 4, (Color){r, g, b, 255});
        }
    }

    // Stars at night
    if (lerp < 0.5f) {
        unsigned char starA = (unsigned char)((0.5f - lerp) * 2.0f * 180);
        for (int i = 0; i < 20; i++) {
            int sx = (i * 137 + 50) % SCREEN_WIDTH;
            int sy = (i * 89 + 20) % 140;
            int sz = 1 + (i % 2);
            float twinkle = sinf((float)GetTime() * 2.0f + i * 1.7f) * 0.3f + 0.7f;
            unsigned char a = (unsigned char)(starA * twinkle);
            DrawRectangle(sx, sy, sz, sz, (Color){255, 255, 220, a});
        }
    }

    // Sun/Moon celestial body
    {
        float t = dayNight.timeOfDay;
        float celestialAngle;
        Color bodyColor;
        int bodySize;

        if (t >= 0.20f && t < 0.80f) {
            // Daytime: sun
            celestialAngle = (t - 0.20f) / 0.60f;
            unsigned char sa = (unsigned char)(200 * lerp);
            bodyColor = (Color){255, 240, 100, sa};
            bodySize = 8;
        } else {
            // Nighttime: moon
            if (t >= 0.80f) celestialAngle = (t - 0.80f) / 0.60f;
            else celestialAngle = (t + 0.20f) / 0.60f;
            float nightness = (0.5f - lerp) * 2.0f;
            if (nightness < 0.0f) nightness = 0.0f;
            unsigned char ma = (unsigned char)(160 * nightness);
            bodyColor = (Color){220, 220, 240, ma};
            bodySize = 6;
        }

        float cx = celestialAngle * SCREEN_WIDTH;
        float cy = 60.0f - sinf(celestialAngle * 3.14159f) * 50.0f;

        if (bodyColor.a > 5) {
            DrawCircleV((Vector2){cx, cy}, (float)(bodySize + 4),
                        (Color){bodyColor.r, bodyColor.g, bodyColor.b, (unsigned char)(bodyColor.a / 4)});
            DrawCircleV((Vector2){cx, cy}, (float)bodySize, bodyColor);
        }
    }

    // Distant mountain silhouettes (parallax 0.1)
    {
        float parallax = 0.1f;
        float offsetX = camX * parallax;
        int baseY = 180;
        unsigned char mr = (unsigned char)(80 + lerp * 60);
        unsigned char mg = (unsigned char)(90 + lerp * 50);
        unsigned char mb = (unsigned char)(110 + lerp * 50);
        unsigned char ma = (unsigned char)(50 + lerp * 50);
        Color mountainColor = { mr, mg, mb, ma };

        for (int x = -100; x < SCREEN_WIDTH + 100; x += 3) {
            float wx = (float)x + offsetX;
            float h = 30.0f + 20.0f * sinf(wx * 0.003f) + 15.0f * sinf(wx * 0.007f + 1.5f);
            DrawRectangle(x, baseY - (int)h, 3, (int)h, mountainColor);
        }
    }

    // Nearer hills (parallax 0.2)
    {
        float parallax = 0.2f;
        float offsetX = camX * parallax;
        int baseY = 200;
        unsigned char hr = (unsigned char)(50 + lerp * 40);
        unsigned char hg = (unsigned char)(70 + lerp * 50);
        unsigned char hb = (unsigned char)(50 + lerp * 40);
        unsigned char ha = (unsigned char)(40 + lerp * 30);
        Color hillColor = { hr, hg, hb, ha };

        for (int x = -100; x < SCREEN_WIDTH + 100; x += 3) {
            float wx = (float)x + offsetX;
            float h = 15.0f + 12.0f * sinf(wx * 0.005f + 3.0f) + 8.0f * sinf(wx * 0.011f);
            DrawRectangle(x, baseY - (int)h, 3, (int)h, hillColor);
        }
    }

    // Clouds (parallax 0.05, slowly drifting)
    {
        float parallax = 0.05f;
        float offsetX = camX * parallax + (float)GetTime() * 8.0f;
        Color cloudColor = { 255, 255, 255, (unsigned char)(25 + lerp * 55) };

        for (int i = 0; i < 8; i++) {
            float cx = fmodf(i * 200.0f + offsetX, (float)(SCREEN_WIDTH + 400)) - 200;
            float cy = 30 + (i * 37) % 70;
            float w = 80 + (i * 53) % 60;
            float h = 10 + (i * 23) % 10;
            DrawRectangle((int)cx, (int)cy, (int)w, (int)h, cloudColor);
            DrawRectangle((int)(cx + w * 0.15f), (int)(cy - h * 0.4f), (int)(w * 0.7f), (int)(h * 0.8f), cloudColor);
            DrawRectangle((int)(cx + w * 0.4f), (int)(cy - h * 0.2f), (int)(w * 0.3f), (int)(h * 0.5f), cloudColor);
        }
    }

    // Rain (heavier at night, occasional during day)
    {
        static float bgRainDrops[60][2]; // x, y positions
        static bool rainInit = false;
        static float rainIntensity = 0.0f;
        float time = (float)GetTime();

        // Rain intensity: higher at night, varies with a slow sine wave
        float nightFactor = 1.0f - lerp; // 1 at night, 0 at day
        float waveFactor = sinf(time * 0.05f) * 0.3f + 0.5f; // slow variation
        float targetIntensity = nightFactor * waveFactor * 0.8f;
        // Occasional light rain during day
        if (lerp > 0.5f && sinf(time * 0.02f) > 0.7f) {
            targetIntensity = 0.15f;
        }
        rainIntensity += (targetIntensity - rainIntensity) * 0.01f;

        if (!rainInit) {
            for (int i = 0; i < 60; i++) {
                bgRainDrops[i][0] = (float)(rand() % SCREEN_WIDTH);
                bgRainDrops[i][1] = (float)(rand() % SCREEN_HEIGHT);
            }
            rainInit = true;
        }

        if (rainIntensity > 0.05f) {
            int dropCount = (int)(rainIntensity * 60);
            unsigned char rainA = (unsigned char)(rainIntensity * 120);
            for (int i = 0; i < dropCount; i++) {
                bgRainDrops[i][1] += 8.0f + rainIntensity * 4.0f; // fall speed
                bgRainDrops[i][0] += sinf(time * 3.0f + i) * 0.5f; // slight wind
                if (bgRainDrops[i][1] > SCREEN_HEIGHT) {
                    bgRainDrops[i][0] = (float)(rand() % SCREEN_WIDTH);
                    bgRainDrops[i][1] = -5.0f;
                }
                DrawLine((int)bgRainDrops[i][0], (int)bgRainDrops[i][1],
                         (int)(bgRainDrops[i][0] - 1), (int)(bgRainDrops[i][1] + 6),
                         (Color){150, 170, 220, rainA});
            }
        }
    }
}
