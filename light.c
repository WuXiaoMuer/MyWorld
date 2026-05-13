#include "types.h"
#include <string.h>

// BFS queue for light propagation
static int queueX[WORLD_WIDTH * 4];
static int queueY[WORLD_WIDTH * 4];
static int queueLevel[WORLD_WIDTH * 4];

static bool IsTransparent(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return true;
    uint8_t block = world[bx][by];
    return block == BLOCK_AIR || block == BLOCK_WATER || block == BLOCK_TORCH ||
           block == BLOCK_FLOWER || block == BLOCK_TALL_GRASS || block == BLOCK_GLASS;
}

void InitLightMap(void)
{
    memset(lightMap, 0, sizeof(uint8_t) * WORLD_WIDTH * WORLD_HEIGHT);
}

void CalculateSunlight(void)
{
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (IsTransparent(x, y)) {
                lightMap[x][y] = SUNLIGHT_LEVEL;
            } else {
                // Sunlight blocked by solid block, propagate into cave
                if (y > 0 && lightMap[x][y - 1] > 0) {
                    // Light enters from above, propagate sideways
                    PropagateLight(x, y - 1, lightMap[x][y - 1]);
                }
                break;
            }
        }
    }
}

void PropagateLight(int startX, int startY, int level)
{
    if (level <= 0) return;

    int qHead = 0, qTail = 0;

    queueX[qTail] = startX;
    queueY[qTail] = startY;
    queueLevel[qTail] = level;
    qTail++;

    while (qHead < qTail) {
        int cx = queueX[qHead];
        int cy = queueY[qHead];
        int cl = queueLevel[qHead];
        qHead++;

        if (cl <= 0) continue;
        if (cx < 0 || cx >= WORLD_WIDTH || cy < 0 || cy >= WORLD_HEIGHT) continue;

        if (lightMap[cx][cy] >= cl) continue;

        lightMap[cx][cy] = cl;

        int nextL = cl - 1;
        if (nextL > 0) {
            queueX[qTail] = cx - 1; queueY[qTail] = cy; queueLevel[qTail] = nextL; qTail++;
            queueX[qTail] = cx + 1; queueY[qTail] = cy; queueLevel[qTail] = nextL; qTail++;
            queueX[qTail] = cx; queueY[qTail] = cy - 1; queueLevel[qTail] = nextL; qTail++;
            queueX[qTail] = cx; queueY[qTail] = cy + 1; queueLevel[qTail] = nextL; qTail++;
        }
    }
}

void RemoveLight(int startX, int startY)
{
    // Reset light around the source and recalculate
    int radius = MAX_LIGHT_LEVEL + 2;
    for (int x = startX - radius; x <= startX + radius; x++) {
        for (int y = startY - radius; y <= startY + radius; y++) {
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                lightMap[x][y] = 0;
            }
        }
    }

    // Recalculate sunlight in the area
    for (int x = startX - radius; x <= startX + radius; x++) {
        if (x < 0 || x >= WORLD_WIDTH) continue;
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (IsTransparent(x, y)) {
                lightMap[x][y] = SUNLIGHT_LEVEL;
            } else {
                break;
            }
        }
    }

    // Re-propagate all torches in the area
    for (int x = startX - radius; x <= startX + radius; x++) {
        for (int y = startY - radius; y <= startY + radius; y++) {
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                if (world[x][y] == BLOCK_TORCH) {
                    PropagateLight(x, y, TORCH_LIGHT);
                }
            }
        }
    }
}

void UpdateLightAt(int bx, int by)
{
    // Remove old light in area, then recalculate
    RemoveLight(bx, by);
}

void RecalculateAllLight(void)
{
    InitLightMap();

    // First pass: sunlight from top
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (IsTransparent(x, y)) {
                lightMap[x][y] = SUNLIGHT_LEVEL;
            } else {
                break;
            }
        }
    }

    // Second pass: propagate torch light
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (world[x][y] == BLOCK_TORCH) {
                PropagateLight(x, y, TORCH_LIGHT);
            }
        }
    }
}

uint8_t GetLightLevel(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return MAX_LIGHT_LEVEL;
    return lightMap[bx][by];
}

Color ApplyLighting(Color base, int bx, int by)
{
    uint8_t light = GetLightLevel(bx, by);
    float brightness = (float)light / MAX_LIGHT_LEVEL;

    // Minimum brightness so blocks are never completely black
    if (brightness < 0.05f) brightness = 0.05f;

    return (Color){
        (unsigned char)(base.r * brightness),
        (unsigned char)(base.g * brightness),
        (unsigned char)(base.b * brightness),
        base.a
    };
}
