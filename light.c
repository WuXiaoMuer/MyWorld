#include "types.h"
#include <string.h>

// BFS queue for light propagation (circular buffer)
#define LIGHT_QUEUE_SIZE (WORLD_WIDTH * 4)
static int queueX[LIGHT_QUEUE_SIZE];
static int queueY[LIGHT_QUEUE_SIZE];
static int queueLevel[LIGHT_QUEUE_SIZE];

static bool IsTransparent(int bx, int by)
{
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return true;
    uint8_t block = GetBlock(bx, by);
    return block == BLOCK_AIR || block == BLOCK_WATER || block == BLOCK_LAVA || block == BLOCK_TORCH ||
           block == BLOCK_FLOWER || block == BLOCK_TALL_GRASS || block == BLOCK_GLASS ||
           block == BLOCK_LANTERN || block == BLOCK_REDSTONE_WIRE ||
           block == BLOCK_STONE_PRESSURE_PLATE || block == BLOCK_LEVER;
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

static void PushLightQueue(int x, int y, int level, int *tail)
{
    int next = (*tail + 1) % LIGHT_QUEUE_SIZE;
    if (next == *tail % LIGHT_QUEUE_SIZE) return; // queue full
    queueX[*tail % LIGHT_QUEUE_SIZE] = x;
    queueY[*tail % LIGHT_QUEUE_SIZE] = y;
    queueLevel[*tail % LIGHT_QUEUE_SIZE] = level;
    (*tail)++;
}

void PropagateLight(int startX, int startY, int level)
{
    if (level <= 0) return;

    int qHead = 0, qTail = 0;

    queueX[qTail % LIGHT_QUEUE_SIZE] = startX;
    queueY[qTail % LIGHT_QUEUE_SIZE] = startY;
    queueLevel[qTail % LIGHT_QUEUE_SIZE] = level;
    qTail++;

    while (qHead < qTail) {
        int idx = qHead % LIGHT_QUEUE_SIZE;
        int cx = queueX[idx];
        int cy = queueY[idx];
        int cl = queueLevel[idx];
        qHead++;

        if (cl <= 0) continue;
        if (cx < 0 || cx >= WORLD_WIDTH || cy < 0 || cy >= WORLD_HEIGHT) continue;
        if (lightMap[cx][cy] >= cl) continue;
        // Don't propagate into solid blocks (except the source itself)
        if (qHead > 1 && !IsTransparent(cx, cy)) continue;

        lightMap[cx][cy] = (uint8_t)cl;

        int nextL = cl - 1;
        if (nextL > 0) {
            PushLightQueue(cx - 1, cy, nextL, &qTail);
            PushLightQueue(cx + 1, cy, nextL, &qTail);
            PushLightQueue(cx, cy - 1, nextL, &qTail);
            PushLightQueue(cx, cy + 1, nextL, &qTail);
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

    // Re-propagate all torches and powered lamps in the area
    for (int x = startX - radius; x <= startX + radius; x++) {
        for (int y = startY - radius; y <= startY + radius; y++) {
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                if (GetBlock(x, y) == BLOCK_TORCH || GetBlock(x, y) == BLOCK_LANTERN) {
                    PropagateLight(x, y, TORCH_LIGHT);
                } else if (GetBlock(x, y) == BLOCK_REDSTONE_LAMP && IsRedstoneLampPowered(x, y)) {
                    PropagateLight(x, y, 12);
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

    // Second pass: propagate torch light and powered redstone lamps
    for (int x = 0; x < WORLD_WIDTH; x++) {
        for (int y = 0; y < WORLD_HEIGHT; y++) {
            if (GetBlock(x, y) == BLOCK_TORCH || GetBlock(x, y) == BLOCK_LANTERN) {
                PropagateLight(x, y, TORCH_LIGHT);
            } else if (GetBlock(x, y) == BLOCK_REDSTONE_LAMP && IsRedstoneLampPowered(x, y)) {
                PropagateLight(x, y, 12);
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
