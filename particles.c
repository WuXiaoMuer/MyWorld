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

void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &particles[i];
        if (!p->active) continue;
        p->lifetime -= dt;
        if (p->lifetime <= 0.0f) { p->active = false; continue; }
        p->velocity.y += PARTICLE_GRAVITY * dt;
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
        c.a = (unsigned char)(alpha * 255);
        DrawRectangle(
            (int)(p->position.x - p->size / 2),
            (int)(p->position.y - p->size / 2),
            (int)p->size, (int)p->size, c
        );
    }
}
