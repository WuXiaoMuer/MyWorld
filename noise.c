#include "types.h"
#include <math.h>

unsigned int hash2D(int x, int y, unsigned int seed)
{
    unsigned int h = seed;
    h ^= (unsigned int)x * 374761393u;
    h ^= (unsigned int)y * 668265263u;
    h = (h ^ (h >> 13u)) * 1274126177u;
    return h;
}

float valueNoise(float x, float y, unsigned int seed)
{
    int ix = (int)floorf(x);
    int iy = (int)floorf(y);
    float fx = x - ix;
    float fy = y - iy;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    float v00 = (float)(hash2D(ix, iy, seed) & 0xFFFF) / 65535.0f;
    float v10 = (float)(hash2D(ix + 1, iy, seed) & 0xFFFF) / 65535.0f;
    float v01 = (float)(hash2D(ix, iy + 1, seed) & 0xFFFF) / 65535.0f;
    float v11 = (float)(hash2D(ix + 1, iy + 1, seed) & 0xFFFF) / 65535.0f;

    float top = v00 + fx * (v10 - v00);
    float bot = v01 + fx * (v11 - v01);
    return top + fy * (bot - top);
}

float fbm(float x, float y, int octaves, float persistence, unsigned int seed)
{
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxVal = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += valueNoise(x * frequency, y * frequency, seed + i * 1000) * amplitude;
        maxVal += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
    return total / maxVal;
}
