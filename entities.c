#include "types.h"
#include <stdlib.h>
#include <math.h>

void InitEntities(void)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        entities[i].active = false;
    }
}

void SpawnItemEntity(uint8_t itemType, int count, float x, float y)
{
    if (itemType == BLOCK_AIR || count <= 0) return;

    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_ENTITIES; i++) {
        if (!entities[i].active) { slot = i; break; }
    }
    if (slot < 0) return; // No free slots

    ItemEntity *e = &entities[slot];
    e->itemType = itemType;
    e->count = count > 64 ? 64 : count;
    e->position.x = x;
    e->position.y = y;
    // Random upward/sideways velocity
    e->velocity.x = (float)(rand() % 120 - 60);
    e->velocity.y = -(float)(rand() % 150 + 80);
    e->lifetime = ENTITY_LIFETIME;
    e->pickupDelay = ENTITY_PICKUP_DELAY;
    e->active = true;
}

void UpdateEntities(float dt)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        ItemEntity *e = &entities[i];
        if (!e->active) continue;

        e->lifetime -= dt;
        if (e->lifetime <= 0.0f) {
            e->active = false;
            continue;
        }

        if (e->pickupDelay > 0.0f) e->pickupDelay -= dt;

        // Gravity
        e->velocity.y += ENTITY_GRAVITY * dt;
        if (e->velocity.y > 600.0f) e->velocity.y = 600.0f;

        // Friction
        e->velocity.x *= 0.95f;

        // Move horizontally
        float newX = e->position.x + e->velocity.x * dt;
        int bx = (int)(newX + 4) / BLOCK_SIZE;
        int by = (int)(e->position.y + 4) / BLOCK_SIZE;
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT && IsBlockSolid(bx, by)) {
            e->velocity.x *= -ENTITY_BOUNCE;
        } else {
            e->position.x = newX;
        }

        // Move vertically
        float newY = e->position.y + e->velocity.y * dt;
        bx = (int)(e->position.x + 4) / BLOCK_SIZE;
        by = (int)(newY + 8) / BLOCK_SIZE;
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT && IsBlockSolid(bx, by)) {
            if (e->velocity.y > 0) {
                // Land on block
                e->position.y = (float)(by * BLOCK_SIZE - 8);
                e->velocity.y *= -ENTITY_BOUNCE;
                if (fabsf(e->velocity.y) < 30.0f) e->velocity.y = 0;
            } else {
                e->velocity.y *= -ENTITY_BOUNCE;
            }
        } else {
            e->position.y = newY;
        }

        // Fall into void
        if (e->position.y > WORLD_HEIGHT * BLOCK_SIZE + 200) {
            e->active = false;
        }
    }
}

void DrawEntities(void)
{
    for (int i = 0; i < MAX_ENTITIES; i++) {
        ItemEntity *e = &entities[i];
        if (!e->active) continue;
        if (e->itemType >= BLOCK_COUNT || blockAtlas.id == 0) continue;

        // Bobbing animation
        float bob = sinf((float)GetTime() * 3.0f + i * 1.7f) * 2.0f;
        int drawX = (int)e->position.x;
        int drawY = (int)(e->position.y + bob);

        // Fade out in last 5 seconds
        unsigned char alpha = 255;
        if (e->lifetime < 5.0f) {
            float blink = sinf(e->lifetime * 6.0f) * 0.5f + 0.5f;
            alpha = (unsigned char)(blink * 255);
        }

        // Draw item sprite (smaller than block)
        Rectangle src = { (float)(e->itemType * BLOCK_SIZE), 0, BLOCK_SIZE, BLOCK_SIZE };
        Rectangle dst = { (float)drawX, (float)drawY, 10, 10 };
        DrawTexturePro(blockAtlas, src, dst, (Vector2){0, 0}, 0, (Color){255, 255, 255, alpha});

        // Stack count
        if (e->count > 1) {
            DrawGameText(TextFormat("%d", e->count), drawX + 8, drawY + 6, 8, (Color){255, 255, 255, alpha});
        }
    }
}

void PickupNearbyItems(float px, float py)
{
    float pcx = px + PLAYER_WIDTH / 2;
    float pcy = py + PLAYER_HEIGHT / 2;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        ItemEntity *e = &entities[i];
        if (!e->active || e->pickupDelay > 0.0f) continue;

        float dx = pcx - (e->position.x + 5);
        float dy = pcy - (e->position.y + 5);
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < ENTITY_PICKUP_DIST) {
            // Try to add to inventory
            if (AddToInventory((BlockType)e->itemType)) {
                e->active = false;
                PlaySoundXP();
            }
        } else if (dist < ENTITY_PICKUP_DIST * 4 && dist > 1.0f) {
            // Magnetic pull: set velocity toward player, damped by distance
            float speed = 150.0f;
            float nx = dx / dist;
            float ny = dy / dist;
            // Blend toward pull direction rather than accumulating
            e->velocity.x = e->velocity.x * 0.8f + nx * speed * 0.2f;
            e->velocity.y = e->velocity.y * 0.8f + ny * speed * 0.2f - 30.0f; // slight upward bias
        }
    }
}
