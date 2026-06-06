#include "types.h"
#include <stdlib.h>

Particle particles[MAX_PARTICLES];

void InitParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
}

static Particle* FindInactive(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) return &particles[i];
    }
    return NULL;
}

void SpawnBlockParticles(int blockX, int blockY, BlockType block)
{
    Color c = blockInfo[block].baseColor;
    float cx = blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
    float cy = blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f;

    for (int i = 0; i < 8; i++) {
        Particle *p = FindInactive();
        if (!p) break;
        p->position = (Vector2){ cx + (rand() % 8 - 4), cy + (rand() % 8 - 4) };
        p->velocity = (Vector2){
            (float)(rand() % 200 - 100),
            -(float)(rand() % 150 + 50)
        };
        p->color = c;
        p->color.r = (unsigned char)(c.r * (0.7f + (rand() % 60) / 100.0f));
        p->color.g = (unsigned char)(c.g * (0.7f + (rand() % 60) / 100.0f));
        p->color.b = (unsigned char)(c.b * (0.7f + (rand() % 60) / 100.0f));
        p->lifetime = 0.4f + (rand() % 100) / 200.0f;
        p->maxLifetime = p->lifetime;
        p->size = 2.0f + (rand() % 3);
        p->active = true;
    }
}

void SpawnDamageParticles(float x, float y, Color color)
{
    for (int i = 0; i < 6; i++) {
        Particle *p = FindInactive();
        if (!p) break;
        p->position = (Vector2){ x + (rand() % 10 - 5), y + (rand() % 10 - 5) };
        p->velocity = (Vector2){
            (float)(rand() % 160 - 80),
            -(float)(rand() % 120 + 40)
        };
        p->color = color;
        p->lifetime = 0.3f + (rand() % 100) / 300.0f;
        p->maxLifetime = p->lifetime;
        p->size = 2.0f + (rand() % 2);
        p->active = true;
    }
}

void SpawnSprintDust(float x, float y)
{
    for (int i = 0; i < 2; i++) {
        Particle *p = FindInactive();
        if (!p) break;
        p->position = (Vector2){ x + (rand() % 8 - 4), y + (rand() % 4 - 2) };
        p->velocity = (Vector2){
            (float)(rand() % 40 - 20),
            -(float)(rand() % 20 + 10)
        };
        p->color = (Color){180, 170, 150, 180};
        p->lifetime = 0.3f + (rand() % 100) / 300.0f;
        p->maxLifetime = p->lifetime;
        p->size = 2.0f + (rand() % 2);
        p->active = true;
    }
}

void SpawnLandingDust(float x, float y, float intensity)
{
    if (intensity < 0.2f) return;
    int count = (int)(intensity * 8);
    if (count > 10) count = 10;
    for (int i = 0; i < count; i++) {
        Particle *p = FindInactive();
        if (!p) break;
        p->position = (Vector2){ x + (rand() % 10 - 5), y };
        p->velocity = (Vector2){
            (float)(rand() % 80 - 40) * intensity,
            -(float)(rand() % 30 + 5)
        };
        p->color = (Color){160, 150, 130, 150};
        p->lifetime = 0.2f + (rand() % 100) / 400.0f;
        p->maxLifetime = p->lifetime;
        p->size = 1.5f + (rand() % 2) * intensity;
        p->active = true;
    }
}

void SpawnBubble(float x, float y)
{
    Particle *p = FindInactive();
    if (!p) return;
    p->position = (Vector2){ x + (rand() % 10 - 5), y + (rand() % 6 - 3) };
    p->velocity = (Vector2){
        (float)(rand() % 20 - 10),
        -(float)(rand() % 30 + 20)
    };
    p->color = (Color){180, 220, 255, 120};
    p->lifetime = 0.5f + (rand() % 100) / 200.0f;
    p->maxLifetime = p->lifetime;
    p->size = 2.0f + (rand() % 3);
    p->active = true;
}

void SpawnFireParticle(float x, float y)
{
    Particle *p = FindInactive();
    if (!p) return;
    p->position = (Vector2){ x + (float)(rand() % 8 - 4), y };
    p->velocity = (Vector2){
        (float)(rand() % 16 - 8),
        -(float)(rand() % 40 + 20)
    };
    // Fire colors: yellow → orange → red
    int r = rand() % 3;
    if (r == 0) p->color = (Color){255, 200, 50, 200};
    else if (r == 1) p->color = (Color){255, 140, 30, 180};
    else p->color = (Color){220, 80, 20, 160};
    p->lifetime = 0.3f + (rand() % 100) / 300.0f;
    p->maxLifetime = p->lifetime;
    p->size = 1.5f + (float)(rand() % 2);
    p->active = true;
}

void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        if (!p->active) continue;
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) { p->active = false; continue; }
        // Bubbles (light blue, alpha 120) float up, no gravity
        if (p->color.b > 200 && p->color.a <= 120) {
            p->velocity.y -= PARTICLE_GRAVITY * 0.3f * dt; // float up
            p->velocity.x += (float)(rand() % 10 - 5) * dt; // drift
        } else {
            p->velocity.y += PARTICLE_GRAVITY * dt;
        }
        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
    }
}

void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        if (!p->active) continue;
        float alpha = p->lifetime / p->maxLifetime;
        Color c = p->color;
        c.a = (unsigned char)(alpha * p->color.a);
        if (p->size < 3.0f && p->color.a < 200) {
            DrawCircleV(p->position, p->size / 2.0f, c);
        } else {
            DrawRectangle(
                (int)(p->position.x - p->size / 2),
                (int)(p->position.y - p->size / 2),
                (int)p->size, (int)p->size, c
            );
        }
    }
}
