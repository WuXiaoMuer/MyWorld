#include "sound.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/stat.h>

// Sound globals defined in main.c

static unsigned int soundSeed = 12345;
static float bgmVolume = 0.3f;
static float sfxVolume = 0.7f;

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

// Footstep - soft thud
static float footstepGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 18.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 100.0f) * 0.4f;
    return (noise * 0.25f + tone) * env;
}

// Zombie groan - low rumble with modulation
static float zombieGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = 1.0f - t * 2.0f;
    if (env < 0) env = 0;
    env *= env; // ease-in
    float pitch = 80.0f + fast_sine(t * 3.0f) * 15.0f;
    float s = fast_sine(t * pitch) * 0.5f;
    float s2 = fast_sine(t * pitch * 1.01f) * 0.3f; // slight detune for roughness
    return (s + s2) * env;
}

// Pig oink - short nasal sound
static float pigGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 10.0f);
    float pitch = 400.0f + fast_sine(t * 40.0f) * 50.0f;
    float s = fast_sine(t * pitch) * 0.4f;
    float nasal = fast_sine(t * pitch * 2.0f) * 0.2f;
    return (s + nasal) * env;
}

// Water splash - noise burst with filter
static float splashGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 5.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 200.0f) * 0.3f;
    float bubble = fast_sine(t * 600.0f * (1.0f - t)) * 0.2f;
    return (noise * 0.4f + tone + bubble) * env;
}

//----------------------------------------------------------------------------------
// Ambient BGM Generator
//----------------------------------------------------------------------------------
// Chord frequencies (Am, F, C, G) - each root + fifth + octave
static const float chordFreqs[4][3] = {
    { 220.00f, 329.63f, 440.00f },  // Am: A3, E4, A4
    { 174.61f, 261.63f, 349.23f },  // F:  F3, C4, F4
    { 261.63f, 392.00f, 523.25f },  // C:  C4, G4, C5
    { 196.00f, 293.66f, 392.00f },  // G:  G3, D4, G4
};

static void GenerateBGMWave(short *samples, int totalFrames, int sampleRate)
{
    float chordDuration = 7.5f; // seconds per chord
    int chordFrames = (int)(chordDuration * sampleRate);

    for (int i = 0; i < totalFrames; i++) {
        float t = (float)i / (float)sampleRate;
        int chordIdx = (i / chordFrames) % 4;
        float chordT = (float)(i % chordFrames) / (float)chordFrames; // 0..1 within chord

        float sample = 0.0f;

        // Pad: three sine tones per chord with slow crossfade
        for (int n = 0; n < 3; n++) {
            float freq = chordFreqs[chordIdx][n];
            float phase = t * freq;
            float sine = fast_sine(phase);

            // Slow amplitude modulation for organic feel
            float mod = 0.7f + 0.3f * fast_sine(t * 0.3f + n * 0.7f);

            // Crossfade at chord boundaries
            float fade = 1.0f;
            if (chordT < 0.08f) fade = chordT / 0.08f;
            else if (chordT > 0.92f) fade = (1.0f - chordT) / 0.08f;

            sample += sine * mod * fade * 0.12f;
        }

        // Low sub-bass drone (constant, very quiet)
        sample += fast_sine(t * 55.0f) * 0.04f;

        // Soft high shimmer (filtered noise-like)
        float shimmer = fast_sine(t * 1760.0f) * fast_sine(t * 0.5f) * 0.015f;
        sample += shimmer;

        // Gentle wind-like noise (very subtle)
        unsigned int rng = (unsigned int)(i * 1103515245 + 12345);
        float noise = (float)(rng % 1000) / 500.0f - 1.0f;
        float windEnv = 0.5f + 0.5f * fast_sine(t * 0.15f);
        sample += noise * windEnv * 0.01f;

        // Clamp and convert
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        samples[i] = (short)(sample * 32000);
    }
}

static void InitBGM(void)
{
    if (!IsAudioDeviceReady()) return;

    int sr = 22050;
    float duration = 15.0f;
    int frames = (int)(duration * sr);
    int dataSize = frames * 2;

    short *samples = (short *)malloc(dataSize);
    if (!samples) return;

    GenerateBGMWave(samples, frames, sr);

    // Ensure saves directory exists
    mkdir("saves");

    // Save WAV to file
    const char *bgmPath = "saves/bgm.wav";
    FILE *f = fopen(bgmPath, "wb");
    if (!f) { free(samples); return; }

    // WAV header
    int wavSize = 44 + dataSize;
    int chunkSize = wavSize - 8;
    int fmtSize = 16;
    short audioFmt = 1, numChannels = 1, blockAlign = 2, bitsPerSample = 16;
    int byteRate = sr * 2;

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunkSize, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    fwrite(&fmtSize, 4, 1, f);
    fwrite(&audioFmt, 2, 1, f);
    fwrite(&numChannels, 2, 1, f);
    fwrite(&sr, 4, 1, f);
    fwrite(&byteRate, 4, 1, f);
    fwrite(&blockAlign, 2, 1, f);
    fwrite(&bitsPerSample, 2, 1, f);
    fwrite("data", 1, 4, f);
    fwrite(&dataSize, 4, 1, f);
    fwrite(samples, 1, dataSize, f);

    fclose(f);
    free(samples);

    bgm = LoadMusicStream(bgmPath);
    if (bgm.stream.buffer) {
        SetMusicVolume(bgm, bgmVolume);
        PlayMusicStream(bgm);
    }
}

void InitSounds(void)
{
    // InitAudioDevice() is called in background thread before this
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

    w = GenerateWave(0.08f, sr, footstepGen);
    sndFootstep = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.5f, sr, zombieGen);
    sndZombie = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.2f, sr, pigGen);
    sndPig = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.4f, sr, splashGen);
    sndSplash = LoadSoundFromWave(w);
    UnloadWave(w);

    InitBGM();
}

void UpdateBGM(void)
{
    if (!IsAudioDeviceReady()) return;
    if (!bgm.stream.buffer) return;
    UpdateMusicStream(bgm);
}

void SetBGMVolume(float volume)
{
    bgmVolume = volume;
    if (bgm.stream.buffer) {
        SetMusicVolume(bgm, bgmVolume);
    }
}

void UnloadSounds(void)
{
    if (!audioReady) return;
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
    UnloadSound(sndFootstep);
    UnloadSound(sndZombie);
    UnloadSound(sndPig);
    UnloadSound(sndSplash);
    if (bgm.stream.buffer) UnloadMusicStream(bgm);
    CloseAudioDevice();
}

static bool IsStoneBlock(BlockType bt) {
    return bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE || bt == BLOCK_BRICK ||
           bt == BLOCK_COAL_ORE || bt == BLOCK_IRON_ORE || bt == BLOCK_SANDSTONE ||
           bt == BLOCK_GLASS || bt == BLOCK_FURNACE;
}

void SetSFXVolume(float volume) {
    sfxVolume = volume;
    if (sfxVolume < 0.0f) sfxVolume = 0.0f;
    if (sfxVolume > 1.0f) sfxVolume = 1.0f;
}

void PlaySoundBreak(BlockType block) {
    if (!IsAudioDeviceReady()) return;
    Sound s = IsStoneBlock(block) ? sndBreakStone : sndBreak;
    SetSoundVolume(s, sfxVolume);
    PlaySound(s);
}

void PlaySoundPlace(BlockType block) {
    (void)block;
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndPlace, sfxVolume);
    PlaySound(sndPlace);
}

void PlaySoundJump(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndJump, sfxVolume);
    PlaySound(sndJump);
}

void PlaySoundLand(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndLand, sfxVolume);
    PlaySound(sndLand);
}

void PlaySoundHurt(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndHurt, sfxVolume);
    PlaySound(sndHurt);
}

void PlaySoundDeath(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndDeath, sfxVolume);
    PlaySound(sndDeath);
}

void PlaySoundEat(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndEat, sfxVolume);
    PlaySound(sndEat);
}

void PlaySoundUIClick(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndClick, sfxVolume);
    PlaySound(sndClick);
}

void PlaySoundCraft(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndCraft, sfxVolume);
    PlaySound(sndCraft);
}

void PlaySoundXP(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndXP, sfxVolume);
    PlaySound(sndXP);
}

void PlaySoundDrop(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndDrop, sfxVolume);
    PlaySound(sndDrop);
}

void PlaySoundFootstep(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndFootstep, sfxVolume);
    PlaySound(sndFootstep);
}

void PlaySoundMob(MobType type) {
    if (!IsAudioDeviceReady()) return;
    if (type == MOB_ZOMBIE) { SetSoundVolume(sndZombie, sfxVolume); PlaySound(sndZombie); }
    else if (type == MOB_PIG) { SetSoundVolume(sndPig, sfxVolume); PlaySound(sndPig); }
    else if (type == MOB_SKELETON) { SetSoundVolume(sndZombie, sfxVolume * 0.8f); PlaySound(sndZombie); }
    else if (type == MOB_CREEPER) { SetSoundVolume(sndZombie, sfxVolume * 0.4f); PlaySound(sndZombie); }
    else if (type == MOB_SPIDER) { SetSoundVolume(sndPig, sfxVolume * 0.6f); PlaySound(sndPig); }
}

void PlaySoundSplash(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndSplash, sfxVolume);
    PlaySound(sndSplash);
}
