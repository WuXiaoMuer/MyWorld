#include "sound.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Sound globals defined in main.c

static unsigned int soundSeed = 12345;

static float fast_sine(float t) {
    t = t - (int)t;
    if (t < 0) t += 1.0f;
    float x = 4.0f * t * (1.0f - t);
    return x * (1.0f - 2.0f * x * x + (2.0f / 3.0f) * x * x * x * x);
}

static Wave GenerateWave(float duration, int sampleRate, float (*generator)(float t, float freq, unsigned int *rng))
{
    int frames = (int)(duration * sampleRate);
    short *samples = (short *)malloc(frames * sizeof(short));

    unsigned int rng = soundSeed;
    for (int i = 0; i < frames; i++) {
        float t = (float)i / (float)sampleRate;
        float sample = generator(t, 0.0f, &rng);
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        samples[i] = (short)(sample * 32000);
    }
    soundSeed = rng;

    Wave wave = { 0 };
    wave.frameCount = frames;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples;
    return wave;
}

// Noise burst with exponential decay
static float noiseBurstGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 12.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    return noise * env;
}

// Stone/metallic break - higher pitched
static float stoneBreakGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 8.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 800.0f) * 0.4f;
    return (noise * 0.6f + tone) * env;
}

// Place sound - short thud
static float placeGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 15.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 150.0f) * 0.5f;
    return (noise * 0.3f + tone) * env;
}

// Jump - rising pitch
static float jumpGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = 1.0f - t * 3.0f;
    if (env < 0) env = 0;
    float pitch = 300.0f + t * 400.0f;
    return fast_sine(t * pitch) * env * 0.5f;
}

// Land - thud
static float landGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 10.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 80.0f) * 0.6f;
    return (noise * 0.3f + tone) * env;
}

// Hurt - noise with tone
static float hurtGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 6.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 250.0f) * 0.4f;
    return (noise * 0.5f + tone) * env;
}

// Death - descending
static float deathGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = 1.0f - t * 1.5f;
    if (env < 0) env = 0;
    float pitch = 400.0f - t * 200.0f;
    if (pitch < 50) pitch = 50;
    float s = fast_sine(t * pitch);
    float s2 = fast_sine(t * pitch * 1.5f) * 0.3f;
    return (s + s2) * env * 0.5f;
}

// Eat - crunch
static float eatGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 8.0f);
    float phase1 = fast_sine(t * 600.0f) * 0.3f;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float mod = fast_sine(t * 20.0f);
    return (phase1 + noise * 0.4f) * env * (0.5f + 0.5f * mod);
}

// UI click
static float clickGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 30.0f);
    return fast_sine(t * 1200.0f) * env * 0.4f;
}

// Craft - pleasant ding
static float craftGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 4.0f);
    float s = fast_sine(t * 800.0f) * 0.4f;
    float s2 = fast_sine(t * 1200.0f) * 0.2f;
    return (s + s2) * env;
}

// XP orb pickup
static float xpGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 5.0f);
    float pitch = 1000.0f + t * 500.0f;
    return fast_sine(t * pitch) * env * 0.35f;
}

// Drop item
static float dropGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 12.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    return noise * env * 0.3f;
}

void InitSounds(void)
{
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;

    int sr = 22050;
    Wave w;

    w = GenerateWave(0.15f, sr, noiseBurstGen);
    sndBreak = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.2f, sr, stoneBreakGen);
    sndBreakStone = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.1f, sr, placeGen);
    sndPlace = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.2f, sr, jumpGen);
    sndJump = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.12f, sr, landGen);
    sndLand = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.25f, sr, hurtGen);
    sndHurt = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.8f, sr, deathGen);
    sndDeath = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.2f, sr, eatGen);
    sndEat = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.05f, sr, clickGen);
    sndClick = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.25f, sr, craftGen);
    sndCraft = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.15f, sr, xpGen);
    sndXP = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.1f, sr, dropGen);
    sndDrop = LoadSoundFromWave(w);
    UnloadWave(w);
}

void UnloadSounds(void)
{
    UnloadSound(sndBreak);
    UnloadSound(sndBreakStone);
    UnloadSound(sndPlace);
    UnloadSound(sndJump);
    UnloadSound(sndLand);
    UnloadSound(sndHurt);
    UnloadSound(sndDeath);
    UnloadSound(sndEat);
    UnloadSound(sndClick);
    UnloadSound(sndCraft);
    UnloadSound(sndXP);
    UnloadSound(sndDrop);
    CloseAudioDevice();
}

static bool IsStoneBlock(BlockType bt) {
    return bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE || bt == BLOCK_BRICK ||
           bt == BLOCK_COAL_ORE || bt == BLOCK_IRON_ORE || bt == BLOCK_SANDSTONE ||
           bt == BLOCK_GLASS || bt == BLOCK_FURNACE;
}

void PlaySoundBreak(BlockType block) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(IsStoneBlock(block) ? sndBreakStone : sndBreak);
}

void PlaySoundPlace(BlockType block) {
    (void)block;
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndPlace);
}

void PlaySoundJump(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndJump);
}

void PlaySoundLand(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndLand);
}

void PlaySoundHurt(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndHurt);
}

void PlaySoundDeath(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndDeath);
}

void PlaySoundEat(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndEat);
}

void PlaySoundUIClick(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndClick);
}

void PlaySoundCraft(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndCraft);
}

void PlaySoundXP(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndXP);
}

void PlaySoundDrop(void) {
    if (!IsAudioDeviceReady()) return;
    PlaySound(sndDrop);
}
