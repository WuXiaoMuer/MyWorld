#include "types.h"
#include <stdlib.h>
#include <math.h>

// Mob globals defined in main.c

// Mob properties per type
static const int mobMaxHealth[] = { 0, 10, 20 };       // NONE, PIG, ZOMBIE
static const float mobSpeed[] = { 0, 40.0f, 60.0f };
static const int mobWidth[] = { 0, 16, 12 };
static const int mobHeight[] = { 0, 12, 28 };
static const int mobDamage[] = { 0, 0, 2 };            // contact damage

void InitMobs(void)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = false;
        mobs[i].type = MOB_NONE;
    }
    mobSpawnTimer = 0.0f;
}

Mob* SpawnMob(MobType type, float x, float y)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) {
            mobs[i].type = type;
            mobs[i].position = (Vector2){ x, y };
            mobs[i].velocity = (Vector2){ 0, 0 };
            mobs[i].health = mobMaxHealth[type];
            mobs[i].maxHealth = mobMaxHealth[type];
            mobs[i].facingRight = rand() % 2;
            mobs[i].onGround = false;
            mobs[i].aiTimer = (float)(rand() % 100) / 100.0f * MOB_AI_INTERVAL;
            mobs[i].aiState = 0;
            mobs[i].contactCooldown = 0.0f;
            mobs[i].deathTimer = 0.0f;
            mobs[i].active = true;
            return &mobs[i];
        }
    }
    return NULL;
}

static bool IsMobOnGround(Mob *mob)
{
    int bx1 = (int)(mob->position.x) / BLOCK_SIZE;
    int bx2 = (int)(mob->position.x + mobWidth[mob->type] - 1) / BLOCK_SIZE;
    int by = (int)(mob->position.y + mobHeight[mob->type]) / BLOCK_SIZE;

    for (int bx = bx1; bx <= bx2; bx++) {
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
            if (IsBlockSolid(bx, by)) return true;
        }
    }
    return false;
}

static bool CanMobSeePlayer(Mob *mob)
{
    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + mobWidth[mob->type] / 2);
    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + mobHeight[mob->type] / 2);
    float dist = sqrtf(dx * dx + dy * dy);
    return dist < 400.0f;
}

static void UpdateMobPhysics(Mob *mob, float dt)
{
    int w = mobWidth[mob->type];
    int h = mobHeight[mob->type];

    // Apply gravity
    mob->velocity.y += MOB_GRAVITY * dt;
    if (mob->velocity.y > 800.0f) mob->velocity.y = 800.0f;

    // Horizontal movement with collision
    float newX = mob->position.x + mob->velocity.x * dt;
    int minBX = (int)(newX) / BLOCK_SIZE;
    int maxBX = (int)(newX + w - 1) / BLOCK_SIZE;
    int minBY = (int)(mob->position.y) / BLOCK_SIZE;
    int maxBY = (int)(mob->position.y + h - 1) / BLOCK_SIZE;

    bool hBlocked = false;
    for (int bx = minBX; bx <= maxBX && !hBlocked; bx++) {
        for (int by = minBY; by <= maxBY && !hBlocked; by++) {
            if (IsBlockSolid(bx, by)) hBlocked = true;
        }
    }

    if (hBlocked) {
        mob->velocity.x = 0;
        // Try to jump over obstacle
        if (mob->onGround && mob->type == MOB_ZOMBIE) {
            mob->velocity.y = -300.0f;
            mob->onGround = false;
        }
    } else {
        mob->position.x = newX;
    }

    // Vertical movement with collision
    float newY = mob->position.y + mob->velocity.y * dt;
    minBX = (int)(mob->position.x) / BLOCK_SIZE;
    maxBX = (int)(mob->position.x + w - 1) / BLOCK_SIZE;
    minBY = (int)(newY) / BLOCK_SIZE;
    maxBY = (int)(newY + h - 1) / BLOCK_SIZE;

    bool vBlocked = false;
    for (int bx = minBX; bx <= maxBX && !vBlocked; bx++) {
        for (int by = minBY; by <= maxBY && !vBlocked; by++) {
            if (IsBlockSolid(bx, by)) vBlocked = true;
        }
    }

    mob->onGround = false;
    if (vBlocked) {
        if (mob->velocity.y > 0) {
            newY = (int)((newY + h) / BLOCK_SIZE) * BLOCK_SIZE - h;
            mob->onGround = true;
        } else {
            newY = (int)(newY / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        mob->velocity.y = 0;
    }
    mob->position.y = newY;

    // Despawn if fallen out of world
    if (mob->position.y > DEATH_Y) {
        mob->active = false;
    }
}

static void UpdateZombieAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    if (CanMobSeePlayer(mob) && dist < 350.0f) {
        // Chase player
        mob->aiState = 1;
        mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_ZOMBIE];
        mob->facingRight = dx > 0;
    } else {
        // Wander
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3; // 0=stop, 1=left, 2=right
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_ZOMBIE] * 0.5f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_ZOMBIE] * 0.5f; mob->facingRight = true; break;
        }
    }
}

static void UpdatePigAI(Mob *mob, float dt)
{
    mob->aiTimer -= dt;
    if (mob->aiTimer <= 0) {
        mob->aiTimer = MOB_AI_INTERVAL * 2.0f + (float)(rand() % 100) / 100.0f * 2.0f;
        mob->aiState = rand() % 3; // 0=idle, 1=left, 2=right
    }
    switch (mob->aiState) {
        case 0: mob->velocity.x = 0; break;
        case 1: mob->velocity.x = -mobSpeed[MOB_PIG]; mob->facingRight = false; break;
        case 2: mob->velocity.x = mobSpeed[MOB_PIG]; mob->facingRight = true; break;
    }
}

static void UpdateMobContactDamage(Mob *mob, float dt)
{
    if (mob->contactCooldown > 0) {
        mob->contactCooldown -= dt;
        return;
    }
    if (mobDamage[mob->type] <= 0) return;

    int w = mobWidth[mob->type];
    int h = mobHeight[mob->type];

    float pLeft = player.position.x;
    float pRight = pLeft + PLAYER_WIDTH;
    float pTop = player.position.y;
    float pBottom = pTop + PLAYER_HEIGHT;

    float mLeft = mob->position.x;
    float mRight = mLeft + w;
    float mTop = mob->position.y;
    float mBottom = mTop + h;

    if (pRight > mLeft && pLeft < mRight && pBottom > mTop && pTop < mBottom) {
        player.health -= mobDamage[mob->type];
        if (player.health < 0) player.health = 0;
        mob->contactCooldown = MOB_CONTACT_COOLDOWN;

        // Knockback
        float kbDir = (player.position.x < mob->position.x) ? -1.0f : 1.0f;
        player.velocity.x = kbDir * 200.0f;
        player.velocity.y = -150.0f;

        PlaySoundHurt();
    }
}

void DamageMob(Mob *mob, int damage)
{
    mob->health -= damage;
    if (mob->health <= 0) {
        mob->deathTimer = MOB_DEATH_TIME;
        mob->velocity.x = 0;

        // Drop items
        if (mob->type == MOB_PIG) {
            AddToInventory(FOOD_RAW_PORK);
        } else if (mob->type == MOB_ZOMBIE) {
            // Zombies have a small chance to drop nothing or an apple
            if (rand() % 4 == 0) AddToInventory(FOOD_APPLE);
        }
        PlaySoundDeath();
        player.xp += (mob->type == MOB_ZOMBIE) ? 5 : 3;
        if (player.xp > MAX_XP) player.xp = MAX_XP;
    }
}

bool IsPlayerNearMob(Mob *mob, float range)
{
    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + mobWidth[mob->type] / 2);
    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + mobHeight[mob->type] / 2);
    return (dx * dx + dy * dy) < (range * range);
}

static void TrySpawnMobs(float dt)
{
    mobSpawnTimer -= dt;
    if (mobSpawnTimer > 0) return;
    mobSpawnTimer = MOB_SPAWN_INTERVAL;

    // Count active mobs
    int count = 0;
    for (int i = 0; i < MAX_MOBS; i++) {
        if (mobs[i].active) count++;
    }
    if (count >= MAX_MOBS / 2) return;

    float playerCX = player.position.x + PLAYER_WIDTH / 2;
    float playerCY = player.position.y + PLAYER_HEIGHT / 2;

    // Zombie spawning: at night
    if (dayNight.lightLevel < 0.3f && count < 8) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;
        float spawnY = playerCY + sinf(angle) * dist * 0.5f;

        int bx = (int)(spawnX / BLOCK_SIZE);
        int by = (int)(spawnY / BLOCK_SIZE);

        // Find ground below spawn point
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = by; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    SpawnMob(MOB_ZOMBIE, spawnX, (y - 2) * BLOCK_SIZE);
                    break;
                }
            }
        }
    }

    // Pig spawning: during day on grass
    if (dayNight.lightLevel > 0.5f && count < 6) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (world[bx][y] == BLOCK_GRASS && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    SpawnMob(MOB_PIG, spawnX, (y - 2) * BLOCK_SIZE);
                    break;
                }
            }
        }
    }
}

void UpdateMobs(float dt)
{
    TrySpawnMobs(dt);

    float playerCX = player.position.x + PLAYER_WIDTH / 2;

    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *mob = &mobs[i];
        if (!mob->active) continue;

        // Despawn if too far from player
        float dx = mob->position.x - playerCX;
        if (dx > MOB_DESPAWN_DIST || dx < -MOB_DESPAWN_DIST) {
            mob->active = false;
            continue;
        }

        // Death animation
        if (mob->deathTimer > 0) {
            mob->deathTimer -= dt;
            if (mob->deathTimer <= 0) {
                mob->active = false;
            }
            continue;
        }

        // Zombie burns in sunlight
        if (mob->type == MOB_ZOMBIE && dayNight.lightLevel > 0.8f) {
            // Check if exposed to sky
            int bx = (int)(mob->position.x + mobWidth[mob->type] / 2) / BLOCK_SIZE;
            int by = (int)(mob->position.y) / BLOCK_SIZE;
            bool exposed = true;
            for (int y = 0; y < by; y++) {
                if (IsBlockSolid(bx, y)) { exposed = false; break; }
            }
            if (exposed) {
                mob->health -= (int)(dt * 5.0f); // ~5 damage/sec
                if (mob->health <= 0) {
                    mob->deathTimer = MOB_DEATH_TIME;
                    mob->velocity.x = 0;
                    PlaySoundDeath();
                    player.xp += 5;
                    if (player.xp > MAX_XP) player.xp = MAX_XP;
                }
            }
        }

        // AI
        if (mob->type == MOB_ZOMBIE) UpdateZombieAI(mob, dt);
        else if (mob->type == MOB_PIG) UpdatePigAI(mob, dt);

        // Physics
        UpdateMobPhysics(mob, dt);

        // Contact damage
        UpdateMobContactDamage(mob, dt);
    }
}

//----------------------------------------------------------------------------------
// Mob Rendering
//----------------------------------------------------------------------------------
static void DrawZombieSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float armSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;
    float legSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;

    // Death fade
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        alpha = (unsigned char)(255 * (mob->deathTimer / MOB_DEATH_TIME));
    }

    // Head (green-ish)
    DrawRectangle((int)(x + 1), (int)y, 10, 10, (Color){80, 140, 60, alpha});
    // Eyes
    DrawRectangle((int)(x + 3), (int)(y + 4), 2, 2, (Color){200, 50, 50, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 4), 2, 2, (Color){200, 50, 50, alpha});
    // Body (dark shirt)
    DrawRectangle((int)(x + 1), (int)(y + 10), 10, 10, (Color){60, 80, 50, alpha});
    // Arms
    DrawRectangle((int)(x - 2), (int)(y + 10 + armSwing), 3, 10, (Color){80, 140, 60, alpha});
    DrawRectangle((int)(x + 11), (int)(y + 10 - armSwing), 3, 10, (Color){80, 140, 60, alpha});
    // Legs
    DrawRectangle((int)(x + 1), (int)(y + 20 + legSwing), 4, 8, (Color){50, 60, 40, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 20 - legSwing), 4, 8, (Color){50, 60, 40, alpha});
}

static void DrawPigSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 12.0f) * 2.0f : 0;

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        alpha = (unsigned char)(255 * (mob->deathTimer / MOB_DEATH_TIME));
    }

    // Body (pink)
    DrawRectangle((int)x, (int)(y + 2), 16, 8, (Color){220, 150, 140, alpha});
    // Head
    DrawRectangle((int)(x + (mob->facingRight ? 12 : -4)), (int)y, 8, 8, (Color){230, 160, 150, alpha});
    // Snout
    int snoutX = mob->facingRight ? (int)(x + 18) : (int)(x - 4);
    DrawRectangle(snoutX, (int)(y + 3), 4, 4, (Color){200, 130, 120, alpha});
    // Eye
    int eyeX = mob->facingRight ? (int)(x + 17) : (int)(x + 1);
    DrawRectangle(eyeX, (int)(y + 2), 2, 2, (Color){40, 40, 40, alpha});
    // Legs
    DrawRectangle((int)(x + 1), (int)(y + 10 + legSwing), 3, 4, (Color){200, 130, 120, alpha});
    DrawRectangle((int)(x + 5), (int)(y + 10 - legSwing), 3, 4, (Color){200, 130, 120, alpha});
    DrawRectangle((int)(x + 10), (int)(y + 10 + legSwing), 3, 4, (Color){200, 130, 120, alpha});
}

void DrawMobs(void)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *mob = &mobs[i];
        if (!mob->active) continue;

        switch (mob->type) {
            case MOB_ZOMBIE: DrawZombieSprite(mob); break;
            case MOB_PIG: DrawPigSprite(mob); break;
            default: break;
        }

        // Health bar (when damaged)
        if (mob->health < mob->maxHealth && mob->deathTimer <= 0) {
            int w = mobWidth[mob->type];
            int barW = w;
            int barH = 3;
            int barX = (int)mob->position.x;
            int barY = (int)mob->position.y - 8;
            float pct = (float)mob->health / mob->maxHealth;
            DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, 180});
            DrawRectangle(barX, barY, (int)(barW * pct), barH, RED);
        }
    }
}
