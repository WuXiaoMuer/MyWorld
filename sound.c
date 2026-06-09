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
    if (!samples) return (Wave){0};

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

// Stone/metallic break - muted thud
static float stoneBreakGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 10.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 400.0f) * 0.4f;
    return (noise * 0.35f + tone) * env;
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

// Hurt - softer impact
static float hurtGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 8.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float tone = fast_sine(t * 200.0f) * 0.35f;
    return (noise * 0.3f + tone) * env;
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

// Drinking - bubbling/gurgling
static float drinkGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 6.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float bubble1 = fast_sine(t * 400.0f + fast_sine(t * 50.0f) * 0.3f);
    float bubble2 = fast_sine(t * 650.0f + fast_sine(t * 70.0f) * 0.2f) * 0.5f;
    float mod = fast_sine(t * 15.0f) * 0.5f + 0.5f;
    return (bubble1 * 0.4f + bubble2 * 0.3f + noise * 0.3f) * env * mod;
}

// UI click - soft
static float clickGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 25.0f);
    return fast_sine(t * 800.0f) * env * 0.3f;
}

// Craft - pleasant ding
static float craftGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 4.0f);
    float s = fast_sine(t * 800.0f) * 0.4f;
    float s2 = fast_sine(t * 1200.0f) * 0.2f;
    return (s + s2) * env;
}

// XP orb pickup - gentle chime
static float xpGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 6.0f);
    float pitch = 600.0f + t * 200.0f;
    return fast_sine(t * pitch) * env * 0.25f;
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

// Villager "hmm" - low hum with vibrato
static float villagerGen(float t, float freq, unsigned int *rng) {
    (void)freq; (void)rng;
    float env = expf(-t * 4.0f) * (1.0f - expf(-t * 50.0f));
    float pitch = 180.0f + fast_sine(t * 8.0f) * 15.0f;
    float s = fast_sine(t * pitch) * 0.3f;
    float harmonic = fast_sine(t * pitch * 2.0f) * 0.15f;
    float nasal = fast_sine(t * pitch * 3.0f) * 0.08f;
    return (s + harmonic + nasal) * env;
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

// Skeleton bone rattle - soft dry clicks
static float skeletonGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 5.0f);
    // Multiple short click bursts
    float click1 = 0.0f, click2 = 0.0f, click3 = 0.0f;
    if (t < 0.06f) {
        float ce = expf(-t * 60.0f);
        *rng = *rng * 1103515245 + 12345;
        click1 = ((float)(*rng % 1000) / 500.0f - 1.0f) * ce * 0.35f;
    }
    if (t > 0.08f && t < 0.14f) {
        float ce = expf(-(t - 0.08f) * 70.0f);
        *rng = *rng * 1103515245 + 12345;
        click2 = ((float)(*rng % 1000) / 500.0f - 1.0f) * ce * 0.3f;
    }
    if (t > 0.16f && t < 0.21f) {
        float ce = expf(-(t - 0.16f) * 80.0f);
        *rng = *rng * 1103515245 + 12345;
        click3 = ((float)(*rng % 1000) / 500.0f - 1.0f) * ce * 0.25f;
    }
    // Soft rattle undertone
    float rattle = fast_sine(t * 900.0f) * fast_sine(t * 3.0f) * 0.1f;
    return (click1 + click2 + click3 + rattle) * env;
}

// Creeper hiss - soft sustained noise with tremolo
static float creeperHissGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = 1.0f - t * 2.0f;
    if (env < 0) env = 0;
    env *= env;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float hiss = fast_sine(t * 1200.0f) * 0.2f;
    float tremolo = 0.6f + 0.4f * fast_sine(t * 4.0f);
    return (noise * 0.2f + hiss) * env * tremolo;
}

// Spider chittering - rapid high-freq pulses with noise
static float spiderGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 5.0f);
    float pulse = fast_sine(t * 800.0f) * 0.3f;
    float rapid = 0.5f + 0.5f * fast_sine(t * 40.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    return (pulse * rapid + noise * 0.25f) * env;
}

// Slime squelch - low tone with noise and AM modulation
static float slimeGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 6.0f);
    float tone = fast_sine(t * 150.0f) * 0.4f;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float am = 0.5f + 0.5f * fast_sine(t * 10.0f);
    float squelch = fast_sine(t * 250.0f * (1.0f - t * 0.3f)) * 0.2f;
    return (tone + noise * 0.2f + squelch) * env * am;
}

// Enderman warble - gentle vibrato with soft whoosh
static float endermanGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = 1.0f - t * 1.8f;
    if (env < 0) env = 0;
    float vibrato = fast_sine(t * 12.0f) * 120.0f;
    float tone = fast_sine(t * (400.0f + vibrato)) * 0.25f;
    // Soft whoosh effect
    float whoosh = 0.0f;
    if (t > 0.1f && t < 0.35f) {
        float wt = (t - 0.1f) / 0.25f;
        float wEnv = fast_sine(wt * 3.14159f);
        *rng = *rng * 1103515245 + 12345;
        whoosh = ((float)(*rng % 1000) / 500.0f - 1.0f) * wEnv * 0.15f;
    }
    return (tone + whoosh) * env;
}

// Creeper fuse hiss - softer
static float creeperFuseGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 3.0f);
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float hiss = fast_sine(t * 1500.0f) * 0.2f;
    float sizzle = fast_sine(t * 2500.0f * (1.0f - t * 0.5f)) * 0.12f;
    return (noise * 0.25f + hiss + sizzle) * env;
}

// Rain ambient - continuous filtered noise
static float rainAmbientGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float mod = 0.5f + 0.5f * fast_sine(t * 0.8f);
    float drip = fast_sine(t * 400.0f) * 0.1f * fast_sine(t * 2.0f);
    return (noise * 0.15f + drip) * mod;
}

// Cave drip - short water drop with reverb
static float caveDripGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 8.0f);
    float drop = fast_sine(t * 1800.0f) * env;
    float echo = fast_sine(t * 900.0f) * expf(-t * 4.0f) * 0.3f;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    return (drop + echo + noise * env * 0.1f) * 0.5f;
}

// Cave ambience - low rumble with subtle drips
static float caveAmbientGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    // Low rumble
    float rumble = fast_sine(t * 30.0f) * 0.3f + fast_sine(t * 47.0f) * 0.2f;
    // Occasional drip pattern
    float dripMod = 0.5f + 0.5f * fast_sine(t * 0.3f);
    float drip = fast_sine(t * 1200.0f * dripMod) * 0.05f * fast_sine(t * 1.5f);
    return (noise * 0.08f + rumble + drip) * 0.6f;
}

// Wind - whooshing surface ambience
static float windGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float mod = 0.5f + 0.5f * fast_sine(t * 0.5f);
    float gust = fast_sine(t * 0.2f) * 0.5f + 0.5f;
    return noise * 0.12f * mod * gust;
}

// Thunder - deep rumble with initial crack
static float thunderGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    // Initial crack (first 0.1s)
    float crackEnv = expf(-t * 20.0f);
    *rng = *rng * 1103515245 + 12345;
    float crackNoise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float crack = crackNoise * crackEnv * 0.6f;

    // Low rumble (sustained)
    float rumbleEnv = 1.0f - t * 0.4f;
    if (rumbleEnv < 0) rumbleEnv = 0;
    rumbleEnv *= rumbleEnv;
    *rng = *rng * 1103515245 + 12345;
    float rumbleNoise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float rumbleTone = fast_sine(t * 40.0f) * 0.3f;
    float rumble = (rumbleNoise * 0.3f + rumbleTone) * rumbleEnv * 0.4f;

    return crack + rumble;
}

// Item pickup - soft chime
static float pickupGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 10.0f);
    float tone = fast_sine(t * 800.0f) * 0.4f + fast_sine(t * 1200.0f) * 0.15f;
    *rng = *rng * 1103515245 + 12345;
    float click = (float)(*rng % 1000) / 500.0f - 1.0f;
    return (tone + click * 0.05f) * env * 0.5f;
}

// Bow fire - twang with snap
static float bowFireGen(float t, float freq, unsigned int *rng) {
    (void)freq;
    float env = expf(-t * 6.0f);
    float twang = fast_sine(t * 400.0f) * 0.4f + fast_sine(t * 600.0f) * 0.3f;
    *rng = *rng * 1103515245 + 12345;
    float noise = (float)(*rng % 1000) / 500.0f - 1.0f;
    float snap = noise * expf(-t * 30.0f) * 0.5f;
    return (twang * env + snap) * 0.7f;
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

    w = GenerateWave(0.35f, sr, drinkGen);
    sndDrink = LoadSoundFromWave(w);
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

    w = GenerateWave(0.4f, sr, villagerGen);
    sndVillager = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.3f, sr, skeletonGen);
    sndSkeleton = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.5f, sr, creeperHissGen);
    sndCreeperHiss = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.15f, sr, spiderGen);
    sndSpider = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.3f, sr, slimeGen);
    sndSlime = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.4f, sr, endermanGen);
    sndEnderman = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.4f, sr, splashGen);
    sndSplash = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.5f, sr, creeperFuseGen);
    sndCreeperFuse = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(2.0f, sr, rainAmbientGen);
    sndRain = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(2.5f, sr, thunderGen);
    sndThunder = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.8f, sr, caveDripGen);
    sndCaveDrip = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(3.0f, sr, caveAmbientGen);
    sndCaveAmbient = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(2.0f, sr, windGen);
    sndWind = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.15f, sr, pickupGen);
    sndPickup = LoadSoundFromWave(w);
    UnloadWave(w);

    w = GenerateWave(0.3f, sr, bowFireGen);
    sndBowFire = LoadSoundFromWave(w);
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
    UnloadSound(sndDrink);
    UnloadSound(sndClick);
    UnloadSound(sndCraft);
    UnloadSound(sndXP);
    UnloadSound(sndDrop);
    UnloadSound(sndFootstep);
    UnloadSound(sndZombie);
    UnloadSound(sndPig);
    UnloadSound(sndSkeleton);
    UnloadSound(sndCreeperHiss);
    UnloadSound(sndSpider);
    UnloadSound(sndSlime);
    UnloadSound(sndEnderman);
    UnloadSound(sndSplash);
    UnloadSound(sndCreeperFuse);
    UnloadSound(sndRain);
    UnloadSound(sndThunder);
    UnloadSound(sndCaveDrip);
    UnloadSound(sndCaveAmbient);
    UnloadSound(sndWind);
    UnloadSound(sndPickup);
    UnloadSound(sndBowFire);
    if (bgm.stream.buffer) UnloadMusicStream(bgm);
    CloseAudioDevice();
}

static bool IsStoneBlock(BlockType bt) {
    return bt == BLOCK_STONE || bt == BLOCK_COBBLESTONE || bt == BLOCK_BRICK ||
           bt == BLOCK_COAL_ORE || bt == BLOCK_IRON_ORE || bt == BLOCK_SANDSTONE ||
           bt == BLOCK_GLASS || bt == BLOCK_FURNACE ||
           bt == BLOCK_GOLD_ORE || bt == BLOCK_DIAMOND_ORE ||
           bt == BLOCK_REDSTONE_ORE || bt == BLOCK_LAPIS_ORE ||
           bt == BLOCK_MOSSY_COBBLESTONE || bt == BLOCK_BONE_BLOCK;
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

void PlaySoundDrink(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndDrink, sfxVolume);
    PlaySound(sndDrink);
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

void PlaySoundPickup(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndPickup, sfxVolume * 0.5f);
    PlaySound(sndPickup);
}

void PlaySoundBowFire(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndBowFire, sfxVolume);
    PlaySound(sndBowFire);
}

void PlaySoundMob(MobType type) {
    if (!IsAudioDeviceReady()) return;
    if (type == MOB_ZOMBIE) { SetSoundVolume(sndZombie, sfxVolume); PlaySound(sndZombie); }
    else if (type == MOB_PIG) { SetSoundVolume(sndPig, sfxVolume); PlaySound(sndPig); }
    else if (type == MOB_SKELETON) { SetSoundVolume(sndSkeleton, sfxVolume * 0.8f); PlaySound(sndSkeleton); }
    else if (type == MOB_CREEPER) { SetSoundVolume(sndCreeperHiss, sfxVolume * 0.5f); PlaySound(sndCreeperHiss); }
    else if (type == MOB_SPIDER) { SetSoundVolume(sndSpider, sfxVolume * 0.7f); PlaySound(sndSpider); }
    else if (type == MOB_SLIME) { SetSoundVolume(sndSlime, sfxVolume * 0.6f); PlaySound(sndSlime); }
    else if (type == MOB_ENDERMAN) { SetSoundVolume(sndEnderman, sfxVolume * 0.5f); PlaySound(sndEnderman); }
    else if (type == MOB_VILLAGER) { SetSoundVolume(sndVillager, sfxVolume * 0.6f); PlaySound(sndVillager); }
}

void PlaySoundSplash(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndSplash, sfxVolume);
    PlaySound(sndSplash);
}

void PlaySoundCreeperFuse(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndCreeperFuse, sfxVolume * 0.6f);
    PlaySound(sndCreeperFuse);
}

void PlaySoundThunder(void) {
    if (!IsAudioDeviceReady()) return;
    SetSoundVolume(sndThunder, sfxVolume * 0.8f);
    PlaySound(sndThunder);
}

static bool rainPlaying = false;

void UpdateRainAmbient(void) {
    if (!IsAudioDeviceReady()) return;
    bool shouldPlay = (weather.rainAlpha > 0.1f);
    if (shouldPlay) {
        if (!IsSoundPlaying(sndRain)) {
            SetSoundVolume(sndRain, sfxVolume * weather.rainAlpha * 0.4f);
            PlaySound(sndRain);
        } else {
            SetSoundVolume(sndRain, sfxVolume * weather.rainAlpha * 0.4f);
        }
        rainPlaying = true;
    } else if (rainPlaying) {
        StopSound(sndRain);
        rainPlaying = false;
    }
}

void PlaySoundMobAt(MobType type, float mobX, float mobY) {
    if (!IsAudioDeviceReady()) return;
    float dx = mobX - (player.position.x + PLAYER_WIDTH / 2.0f);
    float dy = mobY - (player.position.y + PLAYER_HEIGHT / 2.0f);
    float dist = sqrtf(dx * dx + dy * dy);
    float maxDist = 500.0f;
    if (dist > maxDist) return;
    float attenuation = 1.0f - (dist / maxDist);
    attenuation *= attenuation;
    float vol = sfxVolume * attenuation;

    Sound s;
    if (type == MOB_ZOMBIE) s = sndZombie;
    else if (type == MOB_PIG) s = sndPig;
    else if (type == MOB_SKELETON) s = sndSkeleton;
    else if (type == MOB_CREEPER) s = sndCreeperHiss;
    else if (type == MOB_SPIDER) s = sndSpider;
    else if (type == MOB_SLIME) s = sndSlime;
    else if (type == MOB_ENDERMAN) s = sndEnderman;
    else if (type == MOB_VILLAGER) s = sndVillager;
    else return;

    SetSoundVolume(s, vol);
    PlaySound(s);
}

// Ambient sound state
static bool cavePlaying = false;
static bool windPlaying = false;
static float dripTimer = 0.0f;

void UpdateAmbientSounds(void) {
    if (!IsAudioDeviceReady()) return;

    int playerBX = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int playerBY = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    uint8_t light = GetLightLevel(playerBX, playerBY);
    bool underwater = IsPlayerUnderwater();

    // Cave ambient: play when underground (low light) and not underwater
    bool inCave = (light < 8) && !underwater;
    if (inCave) {
        // Cave rumble
        if (!IsSoundPlaying(sndCaveAmbient)) {
            SetSoundVolume(sndCaveAmbient, sfxVolume * 0.15f);
            PlaySound(sndCaveAmbient);
        }
        // Periodic cave drips
        dripTimer -= GetFrameTime();
        if (dripTimer <= 0.0f) {
            dripTimer = 3.0f + (float)(rand() % 500) / 100.0f; // 3-8 seconds
            SetSoundVolume(sndCaveDrip, sfxVolume * 0.25f);
            PlaySound(sndCaveDrip);
        }
        cavePlaying = true;
    } else if (cavePlaying) {
        StopSound(sndCaveAmbient);
        cavePlaying = false;
        dripTimer = 0.0f;
    }

    // Wind: play when on surface and exposed to sky
    bool exposed = true;
    if (playerBY >= 0 && playerBY < WORLD_HEIGHT) {
        for (int y = 0; y < playerBY; y++) {
            if (IsBlockSolid(playerBX, y)) { exposed = false; break; }
        }
    }
    bool onSurface = exposed && !underwater && light >= 10;
    if (onSurface) {
        if (!IsSoundPlaying(sndWind)) {
            SetSoundVolume(sndWind, sfxVolume * 0.08f);
            PlaySound(sndWind);
        }
        windPlaying = true;
    } else if (windPlaying) {
        StopSound(sndWind);
        windPlaying = false;
    }
}
