#include "types.h"
#include <stdlib.h>
#include <math.h>

// Mob globals defined in main.c

// Mob properties per type
static const int mobMaxHealth[] = { 0, 10, 20, 15 };       // NONE, PIG, ZOMBIE, SKELETON
static const float mobSpeed[] = { 0, 40.0f, 30.0f, 40.0f };
static const int mobWidth[] = { 0, 16, 12, 12 };
static const int mobHeight[] = { 0, 12, 28, 28 };
static const int mobDamage[] = { 0, 0, 4, 2 };            // contact damage

void InitMobs(void)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        mobs[i].active = false;
        mobs[i].type = MOB_NONE;
    }
    mobSpawnTimer = 0.0f;
}

//----------------------------------------------------------------------------------
// Projectile System
//----------------------------------------------------------------------------------
void InitProjectiles(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        projectiles[i].active = false;
    }
}

void SpawnProjectile(float x, float y, float vx, float vy)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].position = (Vector2){ x, y };
            projectiles[i].velocity = (Vector2){ vx, vy };
            projectiles[i].lifetime = PROJECTILE_LIFETIME;
            projectiles[i].active = true;
            return;
        }
    }
}

void UpdateProjectiles(float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;
        Projectile *p = &projectiles[i];

        // Apply gravity (arrows arc)
        p->velocity.y += ARROW_GRAVITY * dt;

        p->position.x += p->velocity.x * dt;
        p->position.y += p->velocity.y * dt;
        p->lifetime -= dt;

        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }

        // Check collision with world
        int bx = (int)(p->position.x) / BLOCK_SIZE;
        int by = (int)(p->position.y) / BLOCK_SIZE;
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
            if (IsBlockSolid(bx, by)) {
                p->active = false;
                continue;
            }
        }

        // Check collision with player
        float px = player.position.x;
        float py = player.position.y;
        if (p->position.x >= px && p->position.x <= px + PLAYER_WIDTH &&
            p->position.y >= py && p->position.y <= py + PLAYER_HEIGHT) {
            float reduction = GetArmorDamageReduction();
            int finalDamage = (int)(PROJECTILE_DAMAGE * (1.0f - reduction));
            if (finalDamage < 1) finalDamage = 1;
            player.health -= finalDamage;
            if (player.health < 0) player.health = 0;
            if (player.health <= 0) SetDeathCause(STR_DEATH_MOB_SKELETON);
            DamageArmor();
            player.damageFlashTimer = 0.3f;
            SpawnDamageParticles(p->position.x, p->position.y, (Color){200, 50, 50, 255});
            PlaySoundHurt();
            p->active = false;
        }
    }
}

void DrawProjectiles(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;
        Projectile *p = &projectiles[i];
        // Draw arrow as a small line
        float angle = atan2f(p->velocity.y, p->velocity.x);
        float len = 6.0f;
        float ex = p->position.x - cosf(angle) * len;
        float ey = p->position.y - sinf(angle) * len;
        DrawLine((int)p->position.x, (int)p->position.y, (int)ex, (int)ey, (Color){180, 160, 120, 255});
        // Arrowhead
        DrawRectangle((int)p->position.x - 1, (int)p->position.y - 1, 3, 3, (Color){200, 200, 200, 255});
    }
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
            mobs[i].attackTimer = 1.0f + (float)(rand() % 100) / 100.0f;
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

static void UpdateSkeletonAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    if (CanMobSeePlayer(mob) && dist < 350.0f) {
        // Face player
        mob->facingRight = dx > 0;

        // Maintain distance (~150-200 pixels)
        if (dist < 150.0f) {
            // Too close, back away
            mob->velocity.x = (dx > 0 ? -1 : 1) * mobSpeed[MOB_SKELETON];
        } else if (dist > 220.0f) {
            // Too far, approach
            mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_SKELETON] * 0.6f;
        } else {
            // Good range, strafe
            mob->aiTimer -= dt;
            if (mob->aiTimer <= 0) {
                mob->aiTimer = 1.0f + (float)(rand() % 100) / 100.0f;
                mob->aiState = (rand() % 2) ? 1 : -1;
            }
            mob->velocity.x = mob->aiState * mobSpeed[MOB_SKELETON] * 0.4f;
        }

        // Shoot arrows
        mob->attackTimer -= dt;
        if (mob->attackTimer <= 0) {
            mob->attackTimer = 1.5f + (float)(rand() % 100) / 100.0f;
            float arrowX = mob->position.x + mobWidth[MOB_SKELETON] / 2;
            float arrowY = mob->position.y + mobHeight[MOB_SKELETON] / 3;
            float targetX = player.position.x + PLAYER_WIDTH / 2;
            float targetY = player.position.y + PLAYER_HEIGHT / 3;
            float adx = targetX - arrowX;
            float ady = targetY - arrowY;
            float adist = sqrtf(adx * adx + ady * ady);
            if (adist > 0) {
                float speed = PROJECTILE_SPEED;
                // Aim with slight arc compensation
                float flightTime = adist / speed;
                float arcComp = 0.5f * ARROW_GRAVITY * flightTime * flightTime;
                float vx = (adx / adist) * speed;
                float vy = (ady / adist) * speed - arcComp / flightTime;
                SpawnProjectile(arrowX, arrowY, vx, vy);
            }
        }
    } else {
        // Wander
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3;
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_SKELETON] * 0.5f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_SKELETON] * 0.5f; mob->facingRight = true; break;
        }
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
        int rawDamage = mobDamage[mob->type];
        float reduction = GetArmorDamageReduction();
        int finalDamage = (int)(rawDamage * (1.0f - reduction));
        if (finalDamage < 1) finalDamage = 1;
        player.health -= finalDamage;
        if (player.health < 0) player.health = 0;
        if (player.health <= 0) SetDeathCause(STR_DEATH_MOB_ZOMBIE);
        DamageArmor();
        mob->contactCooldown = MOB_CONTACT_COOLDOWN;

        // Knockback
        float kbDir = (player.position.x < mob->position.x) ? -1.0f : 1.0f;
        player.velocity.x = kbDir * 200.0f;
        player.velocity.y = -150.0f;
        player.knockbackTimer = 0.2f;

        player.damageFlashTimer = 0.3f;
        SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2.0f,
                             player.position.y + PLAYER_HEIGHT / 2.0f,
                             (Color){200, 50, 50, 255});
        PlaySoundHurt();
    }
}

void DamageMob(Mob *mob, int damage)
{
    mob->health -= damage;
    SpawnDamageParticles(mob->position.x + mobWidth[mob->type] / 2.0f,
                         mob->position.y + mobHeight[mob->type] / 2.0f,
                         (Color){180, 30, 30, 255});
    if (mob->health <= 0) {
        mob->deathTimer = MOB_DEATH_TIME;
        mob->velocity.x = 0;

        // Death particles
        Color deathColor;
        switch (mob->type) {
            case MOB_PIG: deathColor = (Color){230, 160, 150, 255}; break;
            case MOB_ZOMBIE: deathColor = (Color){80, 140, 60, 255}; break;
            case MOB_SKELETON: deathColor = (Color){220, 210, 190, 255}; break;
            default: deathColor = (Color){180, 30, 30, 255}; break;
        }
        SpawnDamageParticles(mob->position.x + mobWidth[mob->type] / 2.0f,
                             mob->position.y + mobHeight[mob->type] / 2.0f,
                             deathColor);

        // Drop items
        float dropX = mob->position.x + mobWidth[mob->type] / 2;
        float dropY = mob->position.y;
        if (mob->type == MOB_PIG) {
            SpawnItemEntity(FOOD_RAW_PORK, 1, dropX, dropY);
        } else if (mob->type == MOB_ZOMBIE) {
            if (rand() % 4 == 0) SpawnItemEntity(FOOD_APPLE, 1, dropX, dropY);
            if (rand() % 3 == 0) SpawnItemEntity(ITEM_COAL, 1 + rand() % 2, dropX, dropY);
        } else if (mob->type == MOB_SKELETON) {
            SpawnItemEntity(ITEM_STICK, 1 + rand() % 3, dropX, dropY); // bones
            if (rand() % 3 == 0) SpawnItemEntity(ITEM_COAL, 1, dropX, dropY);
        }
        PlaySoundDeath();
        player.xp += (mob->type == MOB_PIG) ? 3 : 5;
        if (player.xp > MAX_XP) player.xp = MAX_XP;
    }
}

bool IsPlayerNearMob(Mob *mob, float range)
{
    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + mobWidth[mob->type] / 2);
    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + mobHeight[mob->type] / 2);
    return (dx * dx + dy * dy) < (range * range);
}

#define MOB_HOSTILE_LIGHT_MAX 6

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

    // Zombie spawning: dark areas (night surface or underground)
    if (count < 8) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;
        float spawnY = playerCY + sinf(angle) * dist * 0.5f;

        int bx = (int)(spawnX / BLOCK_SIZE);
        int by = (int)(spawnY / BLOCK_SIZE);

        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = by; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light <= MOB_HOSTILE_LIGHT_MAX) {
                        SpawnMob(MOB_ZOMBIE, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Skeleton spawning: dark areas, less frequent
    if (count < 6 && (rand() % 3 == 0)) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;
        float spawnY = playerCY + sinf(angle) * dist * 0.5f;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light <= MOB_HOSTILE_LIGHT_MAX) {
                        SpawnMob(MOB_SKELETON, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Pig spawning: during day on grass, bright areas
    if (dayNight.lightLevel > 0.5f && count < 6) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (world[bx][y] == BLOCK_GRASS && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light >= 8) {
                        SpawnMob(MOB_PIG, spawnX, (y - 2) * BLOCK_SIZE);
                    }
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

        // Hostile mobs burn and despawn in sunlight
        if (mob->type == MOB_ZOMBIE || mob->type == MOB_SKELETON) {
            int mbx = (int)(mob->position.x / BLOCK_SIZE);
            int mby = (int)(mob->position.y / BLOCK_SIZE);
            uint8_t light = GetLightLevel(mbx, mby);
            if (light >= 12 && dayNight.lightLevel > 0.6f) {
                mob->health -= (int)(20.0f * dt);
                if (mob->health <= 0) {
                    mob->deathTimer = MOB_DEATH_TIME;
                }
            }
        }

        // Ambient mob sounds (closer = more likely)
        {
            float dist = fabsf(dx);
            if (dist < 300.0f && mob->deathTimer <= 0) {
                float soundChance = (1.0f - dist / 300.0f) * 0.003f;
                if ((float)rand() / RAND_MAX < soundChance) {
                    PlaySoundMob(mob->type);
                }
            }
        }

        // Death animation
        if (mob->deathTimer > 0) {
            mob->deathTimer -= dt;
            if (mob->deathTimer <= 0) {
                mob->active = false;
            }
            continue;
        }

        // Undead mobs burn in sunlight
        if ((mob->type == MOB_ZOMBIE || mob->type == MOB_SKELETON) && dayNight.lightLevel > 0.8f) {
            // Check if exposed to sky
            int bx = (int)(mob->position.x + mobWidth[mob->type] / 2) / BLOCK_SIZE;
            int by = (int)(mob->position.y) / BLOCK_SIZE;
            bool exposed = true;
            for (int y = 0; y < by; y++) {
                if (IsBlockSolid(bx, y)) { exposed = false; break; }
            }
            if (exposed) {
                mob->burnTimer += dt * 5.0f; // ~5 damage/sec
                while (mob->burnTimer >= 1.0f) {
                    mob->health--;
                    mob->burnTimer -= 1.0f;
                }
                if (mob->health <= 0) {
                    mob->deathTimer = MOB_DEATH_TIME;
                    mob->velocity.x = 0;
                    SpawnDamageParticles(mob->position.x + mobWidth[mob->type] / 2.0f,
                                         mob->position.y + mobHeight[mob->type] / 2.0f,
                                         (Color){200, 160, 60, 255});
                    // Drop items like normal death
                    float dropX = mob->position.x + mobWidth[mob->type] / 2;
                    float dropY = mob->position.y;
                    if (rand() % 4 == 0) SpawnItemEntity(FOOD_APPLE, 1, dropX, dropY);
                    if (rand() % 3 == 0) SpawnItemEntity(ITEM_COAL, 1 + rand() % 2, dropX, dropY);
                    PlaySoundDeath();
                    player.xp += 5;
                    if (player.xp > MAX_XP) player.xp = MAX_XP;
                }
            } else {
                mob->burnTimer = 0.0f;
            }
        } else if (mob->type == MOB_ZOMBIE) {
            mob->burnTimer = 0.0f;
        }

        // AI
        if (mob->type == MOB_ZOMBIE) UpdateZombieAI(mob, dt);
        else if (mob->type == MOB_PIG) UpdatePigAI(mob, dt);
        else if (mob->type == MOB_SKELETON) UpdateSkeletonAI(mob, dt);

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

static void DrawSkeletonSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float armSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;
    float legSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        alpha = (unsigned char)(255 * (mob->deathTimer / MOB_DEATH_TIME));
    }

    // Head (bone white)
    DrawRectangle((int)(x + 1), (int)y, 10, 10, (Color){230, 220, 200, alpha});
    // Eyes (dark hollow)
    DrawRectangle((int)(x + 3), (int)(y + 4), 2, 2, (Color){40, 30, 30, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 4), 2, 2, (Color){40, 30, 30, alpha});
    // Ribs (bone white with gaps)
    DrawRectangle((int)(x + 2), (int)(y + 10), 8, 2, (Color){220, 210, 190, alpha});
    DrawRectangle((int)(x + 2), (int)(y + 13), 8, 2, (Color){220, 210, 190, alpha});
    DrawRectangle((int)(x + 2), (int)(y + 16), 8, 2, (Color){220, 210, 190, alpha});
    // Arms (thin bone)
    DrawRectangle((int)(x - 1), (int)(y + 10 + armSwing), 2, 10, (Color){220, 210, 190, alpha});
    DrawRectangle((int)(x + 11), (int)(y + 10 - armSwing), 2, 10, (Color){220, 210, 190, alpha});
    // Bow in right hand (when not attacking)
    if (mob->attackTimer > 0.5f) {
        DrawRectangle((int)(x + 13), (int)(y + 8 - armSwing), 2, 8, (Color){140, 100, 50, alpha});
    }
    // Legs (thin bone)
    DrawRectangle((int)(x + 2), (int)(y + 20 + legSwing), 3, 8, (Color){220, 210, 190, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 20 - legSwing), 3, 8, (Color){220, 210, 190, alpha});
}

void DrawMobs(void)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *mob = &mobs[i];
        if (!mob->active) continue;

        switch (mob->type) {
            case MOB_ZOMBIE: DrawZombieSprite(mob); break;
            case MOB_PIG: DrawPigSprite(mob); break;
            case MOB_SKELETON: DrawSkeletonSprite(mob); break;
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
