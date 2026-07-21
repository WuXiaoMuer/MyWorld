#include "types.h"
#include "net.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

// Mob globals defined in main.c

// Mob properties per type
// NONE, PIG, ZOMBIE, SKELETON, CREEPER, SPIDER, SLIME, ENDERMAN, COW, SHEEP, CHICKEN, VILLAGER
static const int mobMaxHealth[] = { 0, 10, 20, 15, 20, 16, 8, 40, 10, 8, 4, 20 };
static const float mobSpeed[] = { 0, 40.0f, 30.0f, 40.0f, 25.0f, 50.0f, 35.0f, 60.0f, 30.0f, 25.0f, 35.0f, 20.0f };
static const int mobWidth[] = { 0, 16, 12, 12, 12, 20, 16, 10, 20, 16, 8, 12 };
static const int mobHeight[] = { 0, 12, 28, 24, 24, 16, 12, 32, 16, 14, 10, 28 };
static const int mobDamage[] = { 0, 0, 4, 2, 0, 3, 2, 5, 0, 0, 0, 0 };

int GetMobWidth(MobType type) { return mobWidth[type]; }
int GetMobHeight(MobType type) { return mobHeight[type]; }

// Per-instance size. Small slimes (split children, slimeType==1) are half-size in
// both sprite and hitbox; every other mob returns its plain per-type dimensions.
// Used wherever a specific mob's box matters (collision, hit tests, drawing) so
// the visual and the hitbox stay in agreement.
static int GetMobW(const Mob *m) {
    int w = mobWidth[m->type];
    if (m->type == MOB_SLIME && m->slimeType == 1) w = (w + 1) / 2;
    return w;
}
static int GetMobH(const Mob *m) {
    int h = mobHeight[m->type];
    if (m->type == MOB_SLIME && m->slimeType == 1) h = (h + 1) / 2;
    return h;
}

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

int SpawnProjectile(float x, float y, float vx, float vy, bool fromPlayer)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) {
            projectiles[i].position = (Vector2){ x, y };
            projectiles[i].velocity = (Vector2){ vx, vy };
            projectiles[i].lifetime = PROJECTILE_LIFETIME;
            projectiles[i].active = true;
            projectiles[i].fromPlayer = fromPlayer;
            // Reset transient fields so a reused slot doesn't inherit stale state
            // (e.g. a previous fishing bobber) and arrows get a clean base damage.
            projectiles[i].isFishing = false;
            projectiles[i].hasBite = false;
            projectiles[i].fishTimer = 0.0f;
            projectiles[i].catchValue = 0;
            projectiles[i].damage = PROJECTILE_DAMAGE;
            return i;
        }
    }
    return -1;
}

void UpdateProjectiles(float dt)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;
        Projectile *p = &projectiles[i];

        if (p->isFishing) {
            // Fishing bobber physics
            p->velocity.x *= 0.9f;
            p->velocity.y *= 0.9f;

            p->position.x += p->velocity.x * dt;
            p->position.y += p->velocity.y * dt;
            p->lifetime -= dt;

            if (p->lifetime <= 0) {
                p->active = false;
                continue;
            }

            // Check collision with world (ground)
            int bx = (int)(p->position.x) / BLOCK_SIZE;
            int by = (int)(p->position.y) / BLOCK_SIZE;
            if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                if (IsBlockSolid(bx, by)) {
                    p->position.y = (float)(by * BLOCK_SIZE);
                    p->velocity.x = 0;
                    p->velocity.y = 0;
                }
            }

            // Bob up and down gently once landed
            if (fabsf(p->velocity.x) < 5.0f && fabsf(p->velocity.y) < 5.0f) {
                p->position.y += sinf((float)GetTime() * 2.0f) * 0.3f;

                // Countdown to bite
                p->fishTimer -= dt;
                if (p->fishTimer <= 0 && !p->hasBite) {
                    p->hasBite = true;
                    if (p->fromPlayer) {
                        ShowMessage(S(STR_FISH_BITE), (Color){255, 100, 100, 255});
                    }
                    TriggerCameraShake(2.0f, 0.5f);
                }
            }

            continue; // Don't check mob/player collision for fishing bobber
        }

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
        int bbx = (int)(p->position.x) / BLOCK_SIZE;
        int bby = (int)(p->position.y) / BLOCK_SIZE;
        if (bbx >= 0 && bbx < WORLD_WIDTH && bby >= 0 && bby < WORLD_HEIGHT) {
            if (IsBlockSolid(bbx, bby)) {
                p->active = false;
                continue;
            }
        }

        if (p->fromPlayer) {
            // Player projectile: check collision with mobs
            for (int m = 0; m < MAX_MOBS; m++) {
                if (!mobs[m].active || mobs[m].deathTimer > 0) continue;
                int mw = GetMobW(&mobs[m]);
                int mh = GetMobH(&mobs[m]);
                if (p->position.x >= mobs[m].position.x && p->position.x <= mobs[m].position.x + mw &&
                    p->position.y >= mobs[m].position.y && p->position.y <= mobs[m].position.y + mh) {
                    DamageMob(&mobs[m], p->damage);
                    SpawnDamageParticles(p->position.x, p->position.y, (Color){200, 50, 50, 255});
                    p->active = false;
                    break;
                }
            }
        } else {
            // Mob projectile: check collision with player
            float px = player.position.x;
            float py = player.position.y;
            if (p->position.x >= px && p->position.x <= px + PLAYER_WIDTH &&
                p->position.y >= py && p->position.y <= py + PLAYER_HEIGHT) {
                float reduction = GetArmorDamageReduction();
                int arrowDamage = PROJECTILE_DAMAGE;
                if (gameDifficulty == DIFFICULTY_EASY) arrowDamage = arrowDamage * 3 / 4;
                else if (gameDifficulty == DIFFICULTY_HARD) arrowDamage = arrowDamage * 3 / 2;
                int finalDamage = (int)(arrowDamage * (1.0f - reduction));
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
}

void DrawProjectiles(void)
{
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!projectiles[i].active) continue;
        Projectile *p = &projectiles[i];

        if (p->isFishing) {
            // Draw fishing line (thin line from player to bobber)
            float plx = player.position.x + PLAYER_WIDTH / 2;
            float ply = player.position.y + PLAYER_HEIGHT / 2;
            DrawLine((int)plx, (int)ply, (int)p->position.x, (int)p->position.y, (Color){180, 180, 180, 80});
            // Draw bobber
            Color bobberColor = p->hasBite ? (Color){255, 50, 50, 255} : (Color){200, 100, 50, 255};
            DrawCircle((int)p->position.x, (int)p->position.y, 4, bobberColor);
            // Bite indicator: pulsing red ring
            if (p->hasBite) {
                float pulse = sinf((float)GetTime() * 6.0f) * 0.5f + 0.5f;
                DrawCircle((int)p->position.x, (int)p->position.y, 6 + pulse * 4,
                           (Color){255, 80, 80, (unsigned char)(100 * pulse)});
            }
            continue;
        }

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

//----------------------------------------------------------------------------------
// XP Orb System
//----------------------------------------------------------------------------------
XpOrb xpOrbs[MAX_XP_ORBS];

void InitXpOrbs(void)
{
    for (int i = 0; i < MAX_XP_ORBS; i++) {
        xpOrbs[i].active = false;
    }
}

void SpawnXpOrb(float x, float y, int value)
{
    if (value <= 0) return;
    for (int i = 0; i < MAX_XP_ORBS; i++) {
        if (!xpOrbs[i].active) {
            xpOrbs[i].position = (Vector2){ x, y };
            xpOrbs[i].velocity = (Vector2){ (float)(rand() % 40 - 20), -(float)(rand() % 80 + 40) };
            xpOrbs[i].lifetime = 60.0f;
            xpOrbs[i].active = true;
            xpOrbs[i].xpValue = value;
            xpOrbs[i].bobPhase = (float)(rand() % 100) / 100.0f * 6.28f;
            xpOrbs[i].attractTimer = 2.0f;
            return;
        }
    }
}

void UpdateXpOrbs(float dt)
{
    for (int i = 0; i < MAX_XP_ORBS; i++) {
        XpOrb *o = &xpOrbs[i];
        if (!o->active) continue;

        o->lifetime -= dt;
        if (o->lifetime <= 0) { o->active = false; continue; }

        // Gravity (lighter than regular items)
        o->velocity.y += PARTICLE_GRAVITY * 0.3f * dt;

        o->position.x += o->velocity.x * dt;
        o->position.y += o->velocity.y * dt;
        o->velocity.x *= 0.95f; // friction

        // Bobbing once settled
        o->bobPhase += dt * 2.0f;

        // Magnetic attraction after delay
        o->attractTimer -= dt;
        if (o->attractTimer <= 0.0f) {
            float dx = (player.position.x + PLAYER_WIDTH / 2) - o->position.x;
            float dy = (player.position.y + PLAYER_HEIGHT / 2) - o->position.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < XP_ORB_ATTRACT_DIST && dist > 1.0f) {
                float speed = 200.0f;
                float nx = dx / dist, ny = dy / dist;
                o->velocity.x = o->velocity.x * 0.8f + nx * speed * 0.2f;
                o->velocity.y = o->velocity.y * 0.8f + ny * speed * 0.2f - 20.0f;
            }
        }

        // Ground collision
        int bx = (int)(o->position.x) / BLOCK_SIZE;
        int by = (int)(o->position.y) / BLOCK_SIZE;
        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
            if (IsBlockSolid(bx, by)) {
                o->position.y = (float)(by * BLOCK_SIZE);
                o->velocity.y = 0;
                o->velocity.x *= 0.9f;
            }
        }

        // Pickup by player
        {
            float dx = (player.position.x + PLAYER_WIDTH / 2) - o->position.x;
            float dy = (player.position.y + PLAYER_HEIGHT / 2) - o->position.y;
            if (dx * dx + dy * dy < 256.0f) { // 16px radius
                player.xp += o->xpValue;
                if (player.xp > MAX_XP) player.xp = MAX_XP;
                PlaySoundXP();
                o->active = false;
            }
        }
    }
}

void DrawXpOrbs(void)
{
    for (int i = 0; i < MAX_XP_ORBS; i++) {
        XpOrb *o = &xpOrbs[i];
        if (!o->active) continue;

        float bob = sinf(o->bobPhase) * 1.5f;
        int drawX = (int)o->position.x;
        int drawY = (int)(o->position.y + bob);

        // Fade in last 5 seconds
        unsigned char alpha = 255;
        if (o->lifetime < 5.0f) {
            float blink = sinf(o->lifetime * 6.0f) * 0.5f + 0.5f;
            alpha = (unsigned char)(blink * 255);
        }

        int radius = (o->xpValue == 7) ? 5 : (o->xpValue == 3) ? 4 : 3;
        DrawCircle(drawX, drawY, radius, (Color){60, 220, 80, alpha});
        DrawCircle(drawX - 1, drawY - 1, radius - 1, (Color){100, 255, 120, alpha});
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
            mobs[i].fuseTimer = 0.0f;
            mobs[i].burnTimer = 0.0f;
            mobs[i].fireTimer = 0.0f;
            mobs[i].despawnTimer = MOB_DESPAWN_TIME;
            mobs[i].active = true;
            return &mobs[i];
        }
    }
    return NULL;
}

static bool CanMobSeePlayer(Mob *mob)
{
    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + GetMobW(mob) / 2);
    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + GetMobH(mob) / 2);
    return dx * dx + dy * dy < 160000.0f; // 400^2
}

static void UpdateMobPhysics(Mob *mob, float dt)
{
    int w = GetMobW(mob);
    int h = GetMobH(mob);

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
        // Try to jump over obstacle (zombies, creepers)
        if (mob->onGround && (mob->type == MOB_ZOMBIE || mob->type == MOB_CREEPER)) {
            mob->velocity.y = -300.0f;
            mob->onGround = false;
        }
        // Spiders climb walls
        if (mob->type == MOB_SPIDER) {
            mob->velocity.y = SPIDER_WALL_CLIMB_VEL;
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
        case 1: mob->velocity.x = -mobSpeed[MOB_PIG] * 0.5f; mob->facingRight = false; break;
        case 2: mob->velocity.x = mobSpeed[MOB_PIG] * 0.5f; mob->facingRight = true; break;
    }
}

static void UpdatePassiveAI(Mob *mob, float dt, MobType type)
{
    mob->aiTimer -= dt;
    if (mob->aiTimer <= 0) {
        mob->aiTimer = MOB_AI_INTERVAL * 2.0f + (float)(rand() % 100) / 100.0f * 2.0f;
        mob->aiState = rand() % 3;
    }
    switch (mob->aiState) {
        case 0: mob->velocity.x = 0; break;
        case 1: mob->velocity.x = -mobSpeed[type] * 0.5f; mob->facingRight = false; break;
        case 2: mob->velocity.x = mobSpeed[type] * 0.5f; mob->facingRight = true; break;
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
                SpawnProjectile(arrowX, arrowY, vx, vy, false);
                // Sync skeleton arrow to clients
                if (NetIsHost()) {
                    uint8_t buf[64];
                    PktProjectileSpawn ps;
                    ps.x = arrowX; ps.y = arrowY;
                    ps.vx = vx; ps.vy = vy;
                    ps.fromPlayer = false; ps.playerId = 0;
                    buf[0] = PKT_PROJECTILE_SPAWN;
                    memcpy(buf + 1, &ps, sizeof(PktProjectileSpawn));
                    NetSendToAll(buf, 1 + sizeof(PktProjectileSpawn), false);
                }
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

static void UpdateCreeperAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    if (CanMobSeePlayer(mob) && dist < 300.0f) {
        // Chase player
        mob->aiState = 1;
        mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_CREEPER];
        mob->facingRight = dx > 0;

        // Start fuse when close enough
        if (dist < CREEPER_EXPLODE_DIST) {
            mob->fuseTimer += dt;
            // Play fuse hiss at start and periodically
            if (mob->fuseTimer <= dt || fmodf(mob->fuseTimer, 1.0f) < dt) {
                PlaySoundCreeperFuse();
            }
            // Explode!
            if (mob->fuseTimer >= CREEPER_FUSE_TIME) {
                // Damage player
                float pdx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + mobWidth[MOB_CREEPER] / 2);
                float pdy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + mobHeight[MOB_CREEPER] / 2);
                float pdistSq = pdx * pdx + pdy * pdy;
                if (pdistSq < CREEPER_EXPLODE_DIST * 1.5f * CREEPER_EXPLODE_DIST * 1.5f) {
                    float reduction = GetArmorDamageReduction();
                    int creeperDmg = CREEPER_DAMAGE;
                    if (gameDifficulty == DIFFICULTY_EASY) creeperDmg = creeperDmg * 3 / 4;
                    else if (gameDifficulty == DIFFICULTY_HARD) creeperDmg = creeperDmg * 3 / 2;
                    int finalDamage = (int)(creeperDmg * (1.0f - reduction));
                    if (finalDamage < 1) finalDamage = 1;
                    player.health -= finalDamage;
                    if (player.health < 0) player.health = 0;
                    if (player.health <= 0) SetDeathCause(STR_DEATH_MOB_CREEPER);
                    DamageArmor();
                    player.damageFlashTimer = 0.5f;
                    player.knockbackTimer = 0.3f;
                    player.velocity.x = (pdx > 0 ? 1 : -1) * 300.0f;
                    player.velocity.y = -250.0f;
                    PlaySoundHurt();
                }

                // Destroy nearby blocks
                int cx = (int)(mob->position.x + mobWidth[MOB_CREEPER] / 2) / BLOCK_SIZE;
                int cy = (int)(mob->position.y + mobHeight[MOB_CREEPER] / 2) / BLOCK_SIZE;
                for (int bx = cx - CREEPER_EXPLODE_RADIUS; bx <= cx + CREEPER_EXPLODE_RADIUS; bx++) {
                    for (int by = cy - CREEPER_EXPLODE_RADIUS; by <= cy + CREEPER_EXPLODE_RADIUS; by++) {
                        if (bx >= 0 && bx < WORLD_WIDTH && by >= 0 && by < WORLD_HEIGHT) {
                            float bdx = (bx - cx) * BLOCK_SIZE;
                            float bdy = (by - cy) * BLOCK_SIZE;
                            float explodeR = CREEPER_EXPLODE_RADIUS * BLOCK_SIZE;
                            if (bdx * bdx + bdy * bdy <= explodeR * explodeR) {
                                BlockType bt = (BlockType)world[bx][by];
                                if (bt != BLOCK_AIR && bt != BLOCK_BEDROCK) {
                                    SpawnBlockParticles(bx, by, bt);
                                    world[bx][by] = BLOCK_AIR;
                                    NetSyncBlockChange(bx, by, BLOCK_AIR);
                                    InvalidateChunkAt(bx, by);
                                    UpdateLightAt(bx, by);
                                }
                            }
                        }
                    }
                }
                // Trigger gravity on affected columns
                for (int bx = cx - CREEPER_EXPLODE_RADIUS; bx <= cx + CREEPER_EXPLODE_RADIUS; bx++) {
                    if (bx >= 0 && bx < WORLD_WIDTH) {
                        ApplyGravityAt(bx, cy + CREEPER_EXPLODE_RADIUS);
                    }
                }

                // Explosion particles (radial spread)
                float ecx = mob->position.x + mobWidth[MOB_CREEPER] / 2;
                float ecy = mob->position.y + mobHeight[MOB_CREEPER] / 2;
                for (int p = 0; p < 20; p++) {
                    float angle = (float)(rand() % 628) / 100.0f;
                    float pDist = 5.0f + (float)(rand() % 20);
                    SpawnDamageParticles(ecx + cosf(angle) * pDist,
                                         ecy + sinf(angle) * pDist,
                                         (Color){255, 150, 50, 255});
                }

                TriggerCameraShake(8.0f, 0.5f);

                // Kill the creeper
                mob->health = 0;
                mob->deathTimer = MOB_DEATH_TIME;
                mob->velocity.x = 0;
                PlaySoundDeath();
                SpawnXpOrb(mob->position.x + mobWidth[MOB_CREEPER] / 2, mob->position.y, 3);
                if (player.xp > MAX_XP) player.xp = MAX_XP;
            }
        } else {
            mob->fuseTimer = 0.0f; // Reset fuse if player moves away
        }
    } else {
        // Wander
        mob->fuseTimer = 0.0f;
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3;
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_CREEPER] * 0.5f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_CREEPER] * 0.5f; mob->facingRight = true; break;
        }
    }
}

static void UpdateSpiderAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    // Spiders are hostile at night, neutral during day
    bool hostile = dayNight.lightLevel < 0.5f;

    if (hostile && CanMobSeePlayer(mob) && dist < 400.0f) {
        // Chase player
        mob->aiState = 1;
        mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_SPIDER];
        mob->facingRight = dx > 0;

        // Wall climbing - if hitting a wall, climb up
        if (!mob->onGround && mob->velocity.x != 0) {
            int checkX = (int)((mob->position.x + (dx > 0 ? mobWidth[MOB_SPIDER] + 2 : -2)) / BLOCK_SIZE);
            int checkY = (int)(mob->position.y + mobHeight[MOB_SPIDER] / 2) / BLOCK_SIZE;
            if (checkX >= 0 && checkX < WORLD_WIDTH && checkY >= 0 && checkY < WORLD_HEIGHT) {
                if (IsBlockSolid(checkX, checkY)) {
                    mob->velocity.y = SPIDER_WALL_CLIMB_VEL;
                }
            }
        }
    } else {
        // Wander
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL * 1.5f + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3;
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_SPIDER] * 0.4f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_SPIDER] * 0.4f; mob->facingRight = true; break;
        }
    }
}

// Get biome at a world X position (must match GenerateWorld logic)
static int GetBiomeAtX(int worldX)
{
    float biomeNoise = fbm(worldX * 0.008f, 0.0f, 2, 0.5f, worldSeed + 8000);
    if (biomeNoise > 0.55f) return 1;       // desert
    else if (biomeNoise > 0.35f) return 6;   // taiga
    else if (biomeNoise > 0.15f) return 0;   // plains
    else if (biomeNoise > -0.05f) return 4;  // swamp
    else if (biomeNoise > -0.25f) return 2;  // forest
    else if (biomeNoise > -0.45f) return 5;  // jungle
    else return 3;                            // tundra
}

static void UpdateSlimeAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    if (CanMobSeePlayer(mob) && dist < 300.0f) {
        // Hop toward player
        mob->aiState = 1;
        mob->facingRight = dx > 0;
        mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_SLIME];

        // Periodic hopping
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = 0.8f + (float)(rand() % 100) / 200.0f;
            if (mob->onGround) {
                mob->velocity.y = -220.0f;
            }
        }
    } else {
        // Wander with hops
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL * 2.0f + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3;
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_SLIME] * 0.5f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_SLIME] * 0.5f; mob->facingRight = true; break;
        }
        // Random hops while wandering (framerate-independent: ~2% chance per frame at 60fps)
        if (mob->onGround && (rand() % 10000) < (int)(200.0f * dt)) {
            mob->velocity.y = -180.0f;
        }
    }
}

static void UpdateEndermanAI(Mob *mob, float dt)
{
    float dx = player.position.x - mob->position.x;
    float dist = fabsf(dx);

    // Check if player is looking at enderman (crosshair near mob center)
    Vector2 mouseScreen = Win32GetMousePosition();
    Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, camera);
    float mobCenterX = mob->position.x + mobWidth[MOB_ENDERMAN] / 2.0f;
    float mobCenterY = mob->position.y + mobHeight[MOB_ENDERMAN] / 2.0f;
    float lookDx = mouseWorld.x - mobCenterX, lookDy = mouseWorld.y - mobCenterY;
    bool playerLooking = (lookDx * lookDx + lookDy * lookDy < 10000.0f) && dist < 400.0f; // 100^2

    if (playerLooking || (mob->aiState == 1 && dist < 500.0f)) {
        // Provoked: chase + teleport
        mob->aiState = 1;
        if (!playerLooking && dist > 400.0f) mob->aiState = 0; // Reset if player not looking and far
        mob->facingRight = dx > 0;
        mob->velocity.x = (dx > 0 ? 1 : -1) * mobSpeed[MOB_ENDERMAN];

        // Teleport logic
        mob->attackTimer -= dt;
        if (mob->attackTimer <= 0 && dist > 150.0f) {
            mob->attackTimer = 1.0f + (float)(rand() % 100) / 100.0f;
            // Teleport to a random position near player
            float offset = (float)(rand() % 200 - 100);
            float newX = player.position.x + offset;
            // Find ground at new position
            int bx = (int)(newX / BLOCK_SIZE);
            if (bx >= 0 && bx < WORLD_WIDTH) {
                float oldX = mob->position.x;
                float oldY = mob->position.y;
                for (int by = 0; by < WORLD_HEIGHT - 2; by++) {
                    if (IsBlockSolid(bx, by) && !IsBlockSolid(bx, by - 1) && !IsBlockSolid(bx, by - 2)) {
                        mob->position.x = newX;
                        mob->position.y = (by - 2) * BLOCK_SIZE;
                        mob->velocity = (Vector2){0, 0};
                        // Spawn particles at old and new position
                        SpawnDamageParticles(oldX + 5, oldY + 16, (Color){120, 80, 200, 255});
                        SpawnDamageParticles(mob->position.x + 5, mob->position.y + 16, (Color){120, 80, 200, 255});
                        break;
                    }
                }
            }
        }
    } else {
        // Neutral wander
        mob->aiTimer -= dt;
        if (mob->aiTimer <= 0) {
            mob->aiTimer = MOB_AI_INTERVAL * 2.0f + (float)(rand() % 100) / 100.0f;
            mob->aiState = rand() % 3;
        }
        switch (mob->aiState) {
            case 0: mob->velocity.x = 0; break;
            case 1: mob->velocity.x = -mobSpeed[MOB_ENDERMAN] * 0.3f; mob->facingRight = false; break;
            case 2: mob->velocity.x = mobSpeed[MOB_ENDERMAN] * 0.3f; mob->facingRight = true; break;
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

    int w = GetMobW(mob);
    int h = GetMobH(mob);

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
        // Difficulty multiplier
        if (gameDifficulty == DIFFICULTY_EASY) rawDamage = rawDamage * 3 / 4;
        else if (gameDifficulty == DIFFICULTY_HARD) rawDamage = rawDamage * 3 / 2;
        float reduction = GetArmorDamageReduction();
        int finalDamage = (int)(rawDamage * (1.0f - reduction));
        if (finalDamage < 1) finalDamage = 1;
        player.health -= finalDamage;
        if (player.health < 0) player.health = 0;
        if (player.health <= 0) {
            switch (mob->type) {
                case MOB_ZOMBIE: SetDeathCause(STR_DEATH_MOB_ZOMBIE); break;
                case MOB_SKELETON: SetDeathCause(STR_DEATH_MOB_SKELETON); break;
                case MOB_CREEPER: SetDeathCause(STR_DEATH_MOB_CREEPER); break;
                case MOB_SPIDER: SetDeathCause(STR_DEATH_MOB_SPIDER); break;
                case MOB_SLIME: SetDeathCause(STR_DEATH_MOB_SLIME); break;
                case MOB_ENDERMAN: SetDeathCause(STR_DEATH_MOB_ENDERMAN); break;
                default: SetDeathCause(STR_DEATH_MOB_ZOMBIE); break;
            }
        }
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
    if (mob->deathTimer > 0) return; // Already dying, don't drop again
    mob->health -= damage;
    mob->despawnTimer = MOB_DESPAWN_TIME; // Reset timer on engagement
    SpawnDamageParticles(mob->position.x + GetMobW(mob) / 2.0f,
                         mob->position.y + GetMobH(mob) / 2.0f,
                         (Color){180, 30, 30, 255});
    if (mob->health <= 0) {
        mob->deathTimer = MOB_DEATH_TIME;
        mob->velocity.x = 0;
        totalMobsKilled++;

        // Death particles
        Color deathColor;
        switch (mob->type) {
            case MOB_PIG: deathColor = (Color){230, 160, 150, 255}; break;
            case MOB_ZOMBIE: deathColor = (Color){80, 140, 60, 255}; break;
            case MOB_SKELETON: deathColor = (Color){220, 210, 190, 255}; break;
            case MOB_CREEPER: deathColor = (Color){60, 140, 50, 255}; break;
            case MOB_SPIDER: deathColor = (Color){60, 40, 30, 255}; break;
            case MOB_SLIME: deathColor = (Color){80, 200, 60, 255}; break;
            case MOB_ENDERMAN: deathColor = (Color){120, 80, 200, 255}; break;
            case MOB_COW: deathColor = (Color){100, 60, 30, 255}; break;
            case MOB_SHEEP: deathColor = (Color){230, 230, 230, 255}; break;
            case MOB_CHICKEN: deathColor = (Color){240, 230, 220, 255}; break;
            case MOB_VILLAGER: deathColor = (Color){120, 80, 50, 255}; break;
            default: deathColor = (Color){180, 30, 30, 255}; break;
        }
        SpawnDamageParticles(mob->position.x + GetMobW(mob) / 2.0f,
                             mob->position.y + GetMobH(mob) / 2.0f,
                             deathColor);

        // Drop items with staggered positions
        float baseDropX = mob->position.x + GetMobW(mob) / 2;
        float baseDropY = mob->position.y;
        if (mob->type == MOB_PIG) {
            SpawnItemEntity(FOOD_RAW_PORK, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_ZOMBIE) {
            if (rand() % 4 == 0) SpawnItemEntity(FOOD_APPLE, 1, baseDropX + (rand() % 10 - 5), baseDropY);
            if (rand() % 3 == 0) SpawnItemEntity(ITEM_COAL, 1 + rand() % 2, baseDropX + (rand() % 10 - 5), baseDropY);
            if (rand() % 20 == 0) SpawnItemEntity(ITEM_IRON_INGOT, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_SKELETON) {
            SpawnItemEntity(ITEM_BONE, 1 + rand() % 3, baseDropX + (rand() % 10 - 5), baseDropY);
            if (rand() % 3 == 0) SpawnItemEntity(ITEM_COAL, 1, baseDropX + (rand() % 10 - 5), baseDropY);
            if (rand() % 20 == 0) SpawnItemEntity(TOOL_IRON_SWORD, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_CREEPER) {
            SpawnItemEntity(ITEM_GUNPOWDER, 1 + rand() % 2, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_SPIDER) {
            SpawnItemEntity(ITEM_STRING, 1 + rand() % 2, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_SLIME) {
            SpawnItemEntity(ITEM_SLIMEBALL, 1 + rand() % 3, baseDropX + (rand() % 10 - 5), baseDropY);
            // Large slimes (slimeType==0) split into 2 small slimes
            if (mob->slimeType == 0) {
                Mob *s1 = SpawnMob(MOB_SLIME, mob->position.x - 10, mob->position.y);
                Mob *s2 = SpawnMob(MOB_SLIME, mob->position.x + 10, mob->position.y);
                if (s1) { s1->slimeType = 1; s1->health = 4; s1->maxHealth = 4; }
                if (s2) { s2->slimeType = 1; s2->health = 4; s2->maxHealth = 4; }
            }
        } else if (mob->type == MOB_ENDERMAN) {
            SpawnItemEntity(ITEM_ENDER_PEARL, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_COW) {
            SpawnItemEntity(ITEM_RAW_BEEF, 1 + rand() % 2, baseDropX + (rand() % 10 - 5), baseDropY);
            SpawnItemEntity(ITEM_LEATHER, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_SHEEP) {
            SpawnItemEntity(ITEM_RAW_MUTTON, 1, baseDropX + (rand() % 10 - 5), baseDropY);
            SpawnItemEntity(ITEM_WOOL, 1, baseDropX + (rand() % 10 - 5), baseDropY);
        } else if (mob->type == MOB_CHICKEN) {
            SpawnItemEntity(ITEM_RAW_CHICKEN, 1, baseDropX + (rand() % 10 - 5), baseDropY);
            SpawnItemEntity(ITEM_FEATHER, 1 + rand() % 2, baseDropX + (rand() % 10 - 5), baseDropY);
        }
        PlaySoundDeath();
        // Spawn XP orb
        int passive = (mob->type == MOB_PIG || mob->type == MOB_COW ||
                       mob->type == MOB_SHEEP || mob->type == MOB_CHICKEN);
        int orbValue = passive ? 1 : 3;
        SpawnXpOrb(baseDropX, baseDropY, orbValue);
        if (player.xp > MAX_XP) player.xp = MAX_XP;
    }
}

bool IsPlayerNearMob(Mob *mob, float range)
{
    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mob->position.x + GetMobW(mob) / 2);
    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (mob->position.y + GetMobH(mob) / 2);
    return (dx * dx + dy * dy) < (range * range);
}

#define MOB_HOSTILE_LIGHT_MAX 6

static void TrySpawnMobs(float dt)
{
    mobSpawnTimer -= dt;
    if (mobSpawnTimer > 0) return;
    // Difficulty affects spawn rate
    float spawnInterval = MOB_SPAWN_INTERVAL;
    if (gameDifficulty == DIFFICULTY_PEACEFUL) spawnInterval = 999.0f; // Effectively no hostile spawns
    else if (gameDifficulty == DIFFICULTY_EASY) spawnInterval = MOB_SPAWN_INTERVAL * 1.5f;
    else if (gameDifficulty == DIFFICULTY_HARD) spawnInterval = MOB_SPAWN_INTERVAL * 0.7f;
    mobSpawnTimer = spawnInterval;

    // Count active mobs by category
    int hostileCount = 0, passiveCount = 0;
    for (int i = 0; i < MAX_MOBS; i++) {
        if (!mobs[i].active) continue;
        if (mobs[i].type == MOB_PIG || mobs[i].type == MOB_COW ||
            mobs[i].type == MOB_SHEEP || mobs[i].type == MOB_CHICKEN ||
            mobs[i].type == MOB_VILLAGER) passiveCount++;
        else hostileCount++;
    }
    if (hostileCount + passiveCount >= MAX_MOBS - 4) return; // Leave room for spawns

    // Peaceful: only passive mobs (pigs)
    bool peaceful = (gameDifficulty == DIFFICULTY_PEACEFUL);

    float playerCX = player.position.x + PLAYER_WIDTH / 2;
    float playerCY = player.position.y + PLAYER_HEIGHT / 2;

    // Zombie spawning: dark areas (night surface or underground)
    if (!peaceful && hostileCount < 12) {
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
    if (!peaceful && hostileCount < 12 && (rand() % 3 == 0)) {
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
                        SpawnMob(MOB_SKELETON, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Pig spawning: during day on grass, bright areas
    if (dayNight.lightLevel > 0.5f && passiveCount < 8) {
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

    // Creeper spawning: dark areas like zombie
    if (!peaceful && hostileCount < 12 && (rand() % 4 == 0)) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light <= MOB_HOSTILE_LIGHT_MAX) {
                        SpawnMob(MOB_CREEPER, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Spider spawning: dark areas
    if (!peaceful && hostileCount < 12 && (rand() % 4 == 0)) {
        float angle = (float)(rand() % 628) / 100.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light <= MOB_HOSTILE_LIGHT_MAX) {
                        SpawnMob(MOB_SPIDER, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Slime: swamp/jungle biome, daytime, bright areas
    if (!peaceful && hostileCount + passiveCount < MAX_MOBS - 4) {
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            int biome = GetBiomeAtX(bx);
            if (biome == 4 || biome == 5) { // swamp or jungle
                for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                    if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                        uint8_t light = GetLightLevel(bx, y - 1);
                        if (light >= 8 && dayNight.lightLevel > 0.5f) {
                            SpawnMob(MOB_SLIME, spawnX, (y - 2) * BLOCK_SIZE);
                        }
                        break;
                    }
                }
            }
        }
    }

    // Enderman: night only (or very dark areas), rare
    if (!peaceful && hostileCount + passiveCount < MAX_MOBS - 4 && (rand() % 4 == 0) && dayNight.lightLevel < 0.5f) {
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (IsBlockSolid(bx, y) && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    uint8_t light = GetLightLevel(bx, y - 1);
                    if (light <= MOB_HOSTILE_LIGHT_MAX) {
                        SpawnMob(MOB_ENDERMAN, spawnX, (y - 2) * BLOCK_SIZE);
                    }
                    break;
                }
            }
        }
    }

    // Passive mobs: spawn on grass in daylight, biome-specific weights
    if (passiveCount < 8 && dayNight.lightLevel > 0.5f && (rand() % 3 == 0)) {
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float dist = MOB_SPAWN_DIST_MIN + (float)(rand() % (int)(MOB_SPAWN_DIST_MAX - MOB_SPAWN_DIST_MIN));
        float spawnX = playerCX + cosf(angle) * dist;

        int bx = (int)(spawnX / BLOCK_SIZE);
        if (bx >= 0 && bx < WORLD_WIDTH) {
            for (int y = 0; y < WORLD_HEIGHT - 2; y++) {
                if (world[bx][y] == BLOCK_GRASS && !IsBlockSolid(bx, y - 1) && !IsBlockSolid(bx, y - 2)) {
                    int biome = GetBiomeAtX(bx);
                    int roll = rand() % 100;
                    MobType spawnType;
                    bool doSpawn = true;
                    switch (biome) {
                        case 0: // plains - balanced
                            if (roll < 35) spawnType = MOB_PIG;
                            else if (roll < 65) spawnType = MOB_COW;
                            else if (roll < 85) spawnType = MOB_SHEEP;
                            else if (roll < 95) spawnType = MOB_CHICKEN;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 1: // desert - sparse, heat-tolerant
                            if (roll < 60) doSpawn = false;
                            else if (roll < 80) spawnType = MOB_CHICKEN;
                            else if (roll < 95) spawnType = MOB_PIG;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 2: // forest - more cows and sheep
                            if (roll < 35) spawnType = MOB_COW;
                            else if (roll < 65) spawnType = MOB_SHEEP;
                            else if (roll < 85) spawnType = MOB_PIG;
                            else if (roll < 95) spawnType = MOB_CHICKEN;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 3: // tundra - hardy herds
                            if (roll < 40) spawnType = MOB_COW;
                            else if (roll < 75) spawnType = MOB_SHEEP;
                            else if (roll < 90) spawnType = MOB_CHICKEN;
                            else if (roll < 95) spawnType = MOB_PIG;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 4: // swamp - pigs common
                            if (roll < 40) spawnType = MOB_PIG;
                            else if (roll < 65) spawnType = MOB_COW;
                            else if (roll < 80) spawnType = MOB_SHEEP;
                            else if (roll < 90) spawnType = MOB_CHICKEN;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 5: // jungle - chickens and pigs
                            if (roll < 35) spawnType = MOB_CHICKEN;
                            else if (roll < 65) spawnType = MOB_PIG;
                            else if (roll < 85) spawnType = MOB_COW;
                            else if (roll < 95) spawnType = MOB_SHEEP;
                            else spawnType = MOB_VILLAGER;
                            break;
                        case 6: // taiga - sheep and cows
                            if (roll < 40) spawnType = MOB_SHEEP;
                            else if (roll < 70) spawnType = MOB_COW;
                            else if (roll < 85) spawnType = MOB_PIG;
                            else if (roll < 95) spawnType = MOB_CHICKEN;
                            else spawnType = MOB_VILLAGER;
                            break;
                        default: // fallback to plains
                            if (roll < 35) spawnType = MOB_PIG;
                            else if (roll < 65) spawnType = MOB_COW;
                            else if (roll < 85) spawnType = MOB_SHEEP;
                            else if (roll < 95) spawnType = MOB_CHICKEN;
                            else spawnType = MOB_VILLAGER;
                            break;
                    }
                    if (doSpawn) SpawnMob(spawnType, spawnX, (y - 2) * BLOCK_SIZE);
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

        // Time-based despawn: reset when player is nearby (engaged)
        {
            float dist = fabsf(dx);
            if (dist < MOB_DESPAWN_ENGAGE) {
                mob->despawnTimer = MOB_DESPAWN_TIME;
            } else {
                mob->despawnTimer -= dt;
                if (mob->despawnTimer <= 0.0f) {
                    mob->active = false;
                    continue;
                }
            }
        }

        // Hostile mobs burn and despawn in sunlight
        if (mob->type == MOB_ZOMBIE || mob->type == MOB_SKELETON || mob->type == MOB_ENDERMAN) {
            int mbx = (int)(mob->position.x / BLOCK_SIZE);
            int mby = (int)(mob->position.y / BLOCK_SIZE);
            uint8_t light = GetLightLevel(mbx, mby);
            if (light >= 12 && dayNight.lightLevel > 0.6f) {
                mob->burnTimer += dt * 5.0f; // ~5 damage/sec
                while (mob->burnTimer >= 1.0f) {
                    DamageMob(mob, 1);
                    mob->burnTimer -= 1.0f;
                    if (mob->deathTimer > 0) break; // mob is dying
                }
            } else {
                mob->burnTimer = 0.0f;
            }
        }

        // Fire Aspect damage
        if (mob->fireTimer > 0.0f) {
            mob->fireTimer -= dt;
            // Deal fire damage every 0.5s
            static float fireDmgAccum = 0.0f;
            fireDmgAccum += dt;
            if (fireDmgAccum >= 0.5f) {
                fireDmgAccum -= 0.5f;
                DamageMob(mob, 1);
            }
        }

        // Ambient mob sounds (positional, closer = more likely)
        {
            float dist = fabsf(dx);
            if (dist < 500.0f && mob->deathTimer <= 0) {
                float soundChance = (1.0f - dist / 500.0f) * 0.0008f;
                if ((float)rand() / RAND_MAX < soundChance) {
                    PlaySoundMobAt(mob->type,
                                   mob->position.x + GetMobW(mob) / 2,
                                   mob->position.y + GetMobH(mob) / 2);
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

        // AI
        if (mob->type == MOB_ZOMBIE) UpdateZombieAI(mob, dt);
        else if (mob->type == MOB_PIG) UpdatePigAI(mob, dt);
        else if (mob->type == MOB_SKELETON) UpdateSkeletonAI(mob, dt);
        else if (mob->type == MOB_CREEPER) UpdateCreeperAI(mob, dt);
        else if (mob->type == MOB_SPIDER) UpdateSpiderAI(mob, dt);
        else if (mob->type == MOB_SLIME) UpdateSlimeAI(mob, dt);
        else if (mob->type == MOB_ENDERMAN) UpdateEndermanAI(mob, dt);
        else if (mob->type == MOB_COW || mob->type == MOB_SHEEP || mob->type == MOB_CHICKEN)
            UpdatePassiveAI(mob, dt, mob->type);
        else if (mob->type == MOB_VILLAGER) UpdatePassiveAI(mob, dt, MOB_VILLAGER);

        // Love timer countdown
        if (mob->loveTimer > 0) mob->loveTimer -= dt;

        // Baby growth
        if (mob->isBaby) {
            mob->growTimer -= dt;
            if (mob->growTimer <= 0) {
                mob->isBaby = false;
                mob->maxHealth = mobMaxHealth[mob->type];
                mob->health = mob->maxHealth;
            }
        }

        // Chicken egg drops (every ~30 seconds)
        if (mob->type == MOB_CHICKEN && !mob->isBaby) {
            mob->attackTimer -= dt;
            if (mob->attackTimer <= 0) {
                mob->attackTimer = 25.0f + (float)(rand() % 15);
                SpawnItemEntity(ITEM_EGG, 1, mob->position.x + 4, mob->position.y);
            }
        }

        // Physics
        UpdateMobPhysics(mob, dt);

        // Contact damage
        UpdateMobContactDamage(mob, dt);
    }
}

//----------------------------------------------------------------------------------
// Mob Rendering
//----------------------------------------------------------------------------------
// Baby animals are drawn at half size, anchored at the bottom center so their
// feet stay on the ground and collision box alignment is preserved.
#define BABY_SCALE 0.5f
static float MobScale(const Mob *m) { return m->isBaby ? BABY_SCALE : 1.0f; }
#define SRECT(m, _x, _y, _w, _h) \
    (int)(m->position.x + ((m->isBaby ? (mobWidth[m->type] * (1.0f - BABY_SCALE) * 0.5f) : 0.0f) + ((_x) - m->position.x) * MobScale(m))), \
    (int)(m->position.y + ((m->isBaby ? (mobHeight[m->type] * (1.0f - BABY_SCALE)) : 0.0f) + ((_y) - m->position.y) * MobScale(m))), \
    (int)((_w) * MobScale(m)), (int)((_h) * MobScale(m))

static void DrawZombieSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float armSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;
    float legSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;

    // Death fade with white flash
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
        // White flash at start of death (first 30% of death time)
        float flashIntensity = 1.0f - deathProgress;
        if (flashIntensity < 0.7f) flashIntensity = 0.0f;
        else flashIntensity = (flashIntensity - 0.7f) / 0.3f;
        if (flashIntensity > 0.01f) {
            unsigned char flashA = (unsigned char)(flashIntensity * 180);
            DrawRectangle((int)(x - 1), (int)(y - 1), (int)(mobWidth[MOB_ZOMBIE] + 2), (int)(mobHeight[MOB_ZOMBIE] + 2), (Color){255, 255, 255, flashA});
        }
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
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
        float flashIntensity = 1.0f - deathProgress;
        if (flashIntensity < 0.7f) flashIntensity = 0.0f;
        else flashIntensity = (flashIntensity - 0.7f) / 0.3f;
        if (flashIntensity > 0.01f) {
            unsigned char flashA = (unsigned char)(flashIntensity * 180);
            DrawRectangle(SRECT(mob, x - 1, y - 1, mobWidth[MOB_PIG] + 2, mobHeight[MOB_PIG] + 2), (Color){255, 255, 255, flashA});
        }
    }

    // Body (pink)
    DrawRectangle(SRECT(mob, x, y + 2, 16, 8), (Color){220, 150, 140, alpha});
    // Head
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? 12 : -4), y, 8, 8), (Color){230, 160, 150, alpha});
    // Snout
    float snoutX = mob->facingRight ? (x + 18) : (x - 4);
    DrawRectangle(SRECT(mob, snoutX, y + 3, 4, 4), (Color){200, 130, 120, alpha});
    // Eye
    float eyeX = mob->facingRight ? (x + 17) : (x + 1);
    DrawRectangle(SRECT(mob, eyeX, y + 2, 2, 2), (Color){40, 40, 40, alpha});
    // Legs
    DrawRectangle(SRECT(mob, x + 1, y + 10 + legSwing, 3, 4), (Color){200, 130, 120, alpha});
    DrawRectangle(SRECT(mob, x + 5, y + 10 - legSwing, 3, 4), (Color){200, 130, 120, alpha});
    DrawRectangle(SRECT(mob, x + 10, y + 10 + legSwing, 3, 4), (Color){200, 130, 120, alpha});
}

static void DrawCowSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 10.0f) * 2.0f : 0;
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float dp = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * dp);
    }
    // Body (brown)
    DrawRectangle(SRECT(mob, x, y + 3, 20, 10), (Color){100, 60, 30, alpha});
    // Head
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? 16 : -6), y, 10, 10), (Color){110, 70, 35, alpha});
    // Eye
    float eyeX = mob->facingRight ? (x + 23) : (x + 1);
    DrawRectangle(SRECT(mob, eyeX, y + 3, 2, 2), (Color){30, 30, 30, alpha});
    // Horns
    float hornX = mob->facingRight ? (x + 18) : (x + 2);
    DrawRectangle(SRECT(mob, hornX, y - 2, 2, 3), (Color){200, 190, 170, alpha});
    DrawRectangle(SRECT(mob, hornX + 4, y - 2, 2, 3), (Color){200, 190, 170, alpha});
    // Legs
    DrawRectangle(SRECT(mob, x + 2, y + 13 + legSwing, 3, 5), (Color){90, 55, 25, alpha});
    DrawRectangle(SRECT(mob, x + 7, y + 13 - legSwing, 3, 5), (Color){90, 55, 25, alpha});
    DrawRectangle(SRECT(mob, x + 12, y + 13 + legSwing, 3, 5), (Color){90, 55, 25, alpha});
    DrawRectangle(SRECT(mob, x + 17, y + 13 - legSwing, 3, 5), (Color){90, 55, 25, alpha});
}

static void DrawSheepSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 10.0f) * 2.0f : 0;
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float dp = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * dp);
    }
    // Wool body (white fluffy)
    DrawRectangle(SRECT(mob, x, y + 2, 16, 10), (Color){240, 240, 240, alpha});
    DrawRectangle(SRECT(mob, x + 1, y + 1, 14, 12), (Color){230, 230, 230, alpha});
    // Head (dark)
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? 12 : -4), y, 8, 8), (Color){60, 60, 60, alpha});
    // Eye
    float eyeX = mob->facingRight ? (x + 17) : (x + 1);
    DrawRectangle(SRECT(mob, eyeX, y + 2, 2, 2), (Color){200, 200, 200, alpha});
    // Legs
    DrawRectangle(SRECT(mob, x + 2, y + 12 + legSwing, 3, 4), (Color){50, 50, 50, alpha});
    DrawRectangle(SRECT(mob, x + 6, y + 12 - legSwing, 3, 4), (Color){50, 50, 50, alpha});
    DrawRectangle(SRECT(mob, x + 10, y + 12 + legSwing, 3, 4), (Color){50, 50, 50, alpha});
    DrawRectangle(SRECT(mob, x + 14, y + 12 - legSwing, 3, 4), (Color){50, 50, 50, alpha});
}

static void DrawChickenSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 15.0f) * 2.0f : 0;
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float dp = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * dp);
    }
    // Body (white)
    DrawRectangle(SRECT(mob, x, y + 2, 8, 6), (Color){240, 230, 220, alpha});
    // Head
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? 6 : -2), y, 5, 5), (Color){240, 230, 220, alpha});
    // Beak
    float beakX = mob->facingRight ? (x + 10) : (x - 2);
    DrawRectangle(SRECT(mob, beakX, y + 2, 3, 2), (Color){230, 180, 50, alpha});
    // Comb (red)
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? 7 : 0), y - 1, 3, 2), (Color){200, 50, 50, alpha});
    // Eye
    float eyeX = mob->facingRight ? (x + 9) : (x + 1);
    DrawRectangle(SRECT(mob, eyeX, y + 1, 1, 1), (Color){30, 30, 30, alpha});
    // Legs
    DrawRectangle(SRECT(mob, x + 2, y + 8 + legSwing, 2, 3), (Color){200, 150, 50, alpha});
    DrawRectangle(SRECT(mob, x + 5, y + 8 - legSwing, 2, 3), (Color){200, 150, 50, alpha});
    // Tail
    DrawRectangle(SRECT(mob, x + (mob->facingRight ? -1 : 7), y + 1, 2, 4), (Color){220, 210, 200, alpha});
}

static void DrawVillagerSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 8.0f) * 2.0f : 0;
    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float dp = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * dp);
    }
    // Body (brown robe)
    DrawRectangle((int)x, (int)(y + 8), 12, 16, (Color){120, 80, 50, alpha});
    // Head (skin)
    DrawRectangle((int)(x + 2), (int)y, 8, 10, (Color){200, 160, 120, alpha});
    // Eyes
    DrawRectangle((int)(x + 4), (int)(y + 3), 2, 2, (Color){40, 40, 40, alpha});
    DrawRectangle((int)(x + 8), (int)(y + 3), 2, 2, (Color){40, 40, 40, alpha});
    // Nose
    DrawRectangle((int)(x + 6), (int)(y + 5), 2, 2, (Color){180, 140, 100, alpha});
    // Arms
    DrawRectangle((int)(x - 2), (int)(y + 9 + legSwing), 3, 10, (Color){110, 75, 45, alpha});
    DrawRectangle((int)(x + 13), (int)(y + 9 - legSwing), 3, 10, (Color){110, 75, 45, alpha});
    // Legs
    DrawRectangle((int)(x + 2), (int)(y + 24 + legSwing), 3, 6, (Color){100, 70, 40, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 24 - legSwing), 3, 6, (Color){100, 70, 40, alpha});
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
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
        float flashIntensity = 1.0f - deathProgress;
        if (flashIntensity < 0.7f) flashIntensity = 0.0f;
        else flashIntensity = (flashIntensity - 0.7f) / 0.3f;
        if (flashIntensity > 0.01f) {
            unsigned char flashA = (unsigned char)(flashIntensity * 180);
            DrawRectangle((int)(x - 1), (int)(y - 1), (int)(mobWidth[MOB_SKELETON] + 2), (int)(mobHeight[MOB_SKELETON] + 2), (Color){255, 255, 255, flashA});
        }
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

static void DrawCreeperSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legSwing = moving ? sinf(time * 8.0f) * 2.0f : 0;

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
        float flashIntensity = 1.0f - deathProgress;
        if (flashIntensity < 0.7f) flashIntensity = 0.0f;
        else flashIntensity = (flashIntensity - 0.7f) / 0.3f;
        if (flashIntensity > 0.01f) {
            unsigned char flashA = (unsigned char)(flashIntensity * 180);
            DrawRectangle((int)(x - 1), (int)(y - 1), (int)(mobWidth[MOB_CREEPER] + 2), (int)(mobHeight[MOB_CREEPER] + 2), (Color){255, 255, 255, flashA});
        }
    }

    // Flashing when about to explode
    if (mob->fuseTimer > 0.0f && mob->fuseTimer < 1.5f) {
        if ((int)(time * 8) % 2 == 0) alpha = (unsigned char)(alpha * 0.5f);
    }

    // Head (green, slightly taller)
    DrawRectangle((int)(x + 1), (int)y, 10, 10, (Color){60, 140, 50, alpha});
    // Face (darker green pattern - creeper face)
    DrawRectangle((int)(x + 2), (int)(y + 2), 2, 2, (Color){30, 80, 25, alpha});
    DrawRectangle((int)(x + 7), (int)(y + 2), 2, 2, (Color){30, 80, 25, alpha});
    DrawRectangle((int)(x + 4), (int)(y + 5), 3, 2, (Color){30, 80, 25, alpha});
    // Body (green)
    DrawRectangle((int)(x + 2), (int)(y + 10), 8, 10, (Color){50, 120, 40, alpha});
    // Legs (4 short legs)
    DrawRectangle((int)(x + 1), (int)(y + 20 + legSwing), 3, 4, (Color){50, 120, 40, alpha});
    DrawRectangle((int)(x + 8), (int)(y + 20 - legSwing), 3, 4, (Color){50, 120, 40, alpha});
}

static void DrawSpiderSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float legWave = moving ? sinf(time * 12.0f) * 3.0f : 0;

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
        float flashIntensity = 1.0f - deathProgress;
        if (flashIntensity < 0.7f) flashIntensity = 0.0f;
        else flashIntensity = (flashIntensity - 0.7f) / 0.3f;
        if (flashIntensity > 0.01f) {
            unsigned char flashA = (unsigned char)(flashIntensity * 180);
            DrawRectangle((int)(x - 1), (int)(y - 1), (int)(mobWidth[MOB_SPIDER] + 2), (int)(mobHeight[MOB_SPIDER] + 2), (Color){255, 255, 255, flashA});
        }
    }

    // Body (dark brown/red)
    DrawRectangle((int)(x + 4), (int)(y + 4), 12, 8, (Color){60, 40, 30, alpha});
    // Head (lighter)
    DrawRectangle((int)(x + 2), (int)(y + 3), 6, 6, (Color){80, 55, 40, alpha});
    // Eyes (red, multiple)
    DrawRectangle((int)(x + 3), (int)(y + 4), 2, 2, (Color){200, 50, 30, alpha});
    DrawRectangle((int)(x + 6), (int)(y + 4), 2, 2, (Color){200, 50, 30, alpha});
    DrawRectangle((int)(x + 4), (int)(y + 6), 1, 1, (Color){200, 50, 30, alpha});
    // Legs (4 pairs, angled)
    for (int i = 0; i < 4; i++) {
        int lx = (int)(x + 5 + i * 3);
        float offset = (i % 2 == 0) ? legWave : -legWave;
        DrawRectangle(lx, (int)(y + 12 + offset), 2, 6, (Color){50, 35, 25, alpha});
        DrawRectangle(lx - 2, (int)(y + 14 + offset), 2, 4, (Color){50, 35, 25, alpha});
    }
}

static void DrawSlimeSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    int w = GetMobW(mob);
    int h = GetMobH(mob);

    // Squash/stretch animation
    float bounce = mob->onGround ? 1.0f : 0.8f;
    int drawH = (int)(h * bounce);
    int drawW = (int)(w / bounce);

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
    }

    // Body (green blob)
    DrawRectangle((int)x, (int)(y + h - drawH), drawW, drawH, (Color){80, 200, 60, alpha});
    // Darker spots
    DrawRectangle((int)(x + 3), (int)(y + 3), 3, 3, (Color){60, 170, 40, alpha});
    DrawRectangle((int)(x + 9), (int)(y + 5), 2, 2, (Color){60, 170, 40, alpha});
    // Eyes (white with dark pupil)
    DrawRectangle((int)(x + 4), (int)(y + 3), 3, 3, (Color){255, 255, 255, alpha});
    DrawRectangle((int)(x + 9), (int)(y + 3), 3, 3, (Color){255, 255, 255, alpha});
    DrawRectangle((int)(x + 5), (int)(y + 4), 2, 2, (Color){30, 30, 30, alpha});
    DrawRectangle((int)(x + 10), (int)(y + 4), 2, 2, (Color){30, 30, 30, alpha});
    // Highlight
    DrawRectangle((int)(x + 2), (int)(y + 1), 2, 2, (Color){120, 230, 100, alpha});
}

static void DrawEndermanSprite(Mob *mob)
{
    float x = mob->position.x;
    float y = mob->position.y;
    float time = (float)GetTime();
    bool moving = fabsf(mob->velocity.x) > 5.0f;
    float armSwing = moving ? sinf(time * 6.0f) * 4.0f : 0;
    float bob = sinf(time * 3.0f) * 1.5f;

    unsigned char alpha = 255;
    if (mob->deathTimer > 0) {
        float deathProgress = mob->deathTimer / MOB_DEATH_TIME;
        alpha = (unsigned char)(255 * deathProgress);
    }

    // Body (tall, dark)
    DrawRectangle((int)(x + 2), (int)(y + 8 + bob), 6, 16, (Color){20, 20, 25, alpha});
    // Head
    DrawRectangle((int)(x + 1), (int)(y + bob), 8, 8, (Color){25, 25, 30, alpha});
    // Eyes (bright purple, glowing)
    DrawRectangle((int)(x + 3), (int)(y + 3 + bob), 2, 2, (Color){140, 80, 220, alpha});
    DrawRectangle((int)(x + 6), (int)(y + 3 + bob), 2, 2, (Color){140, 80, 220, alpha});
    // Arms (long, thin)
    DrawRectangle((int)(x - 1), (int)(y + 10 + armSwing + bob), 2, 14, (Color){20, 20, 25, alpha});
    DrawRectangle((int)(x + 9), (int)(y + 10 - armSwing + bob), 2, 14, (Color){20, 20, 25, alpha});
    // Legs (long, thin)
    float legSwing = moving ? sinf(time * 8.0f) * 3.0f : 0;
    DrawRectangle((int)(x + 2), (int)(y + 24 + legSwing + bob), 2, 8, (Color){20, 20, 25, alpha});
    DrawRectangle((int)(x + 6), (int)(y + 24 - legSwing + bob), 2, 8, (Color){20, 20, 25, alpha});

    // Purple particle trail when provoked
    if (mob->aiState == 1) {
        float particleTime = time * 5.0f;
        for (int i = 0; i < 3; i++) {
            float px = x + 5 + sinf(particleTime + i * 2.0f) * 8;
            float py = y + 10 + cosf(particleTime + i * 1.5f) * 6 + bob;
            DrawRectangle((int)px, (int)py, 2, 2, (Color){120, 60, 200, (unsigned char)(alpha / 2)});
        }
    }
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
            case MOB_CREEPER: DrawCreeperSprite(mob); break;
            case MOB_SPIDER: DrawSpiderSprite(mob); break;
            case MOB_SLIME: DrawSlimeSprite(mob); break;
            case MOB_ENDERMAN: DrawEndermanSprite(mob); break;
            case MOB_COW: DrawCowSprite(mob); break;
            case MOB_SHEEP: DrawSheepSprite(mob); break;
            case MOB_CHICKEN: DrawChickenSprite(mob); break;
            case MOB_VILLAGER: DrawVillagerSprite(mob); break;
            default: break;
        }

        // Health bar (when damaged, fades during death)
        if (mob->health < mob->maxHealth) {
            unsigned char barAlpha = 180;
            if (mob->deathTimer > 0) {
                barAlpha = (unsigned char)(180 * (mob->deathTimer / MOB_DEATH_TIME));
            }
            int w = GetMobW(mob);
            int barW = w;
            int barH = 3;
            int barX = (int)mob->position.x;
            int barY = (int)mob->position.y - 8;
            float pct = (float)mob->health / mob->maxHealth;
            DrawRectangle(barX, barY, barW, barH, (Color){0, 0, 0, barAlpha});
            DrawRectangle(barX, barY, (int)(barW * pct), barH, (Color){200, 30, 30, barAlpha});
        }

        // Love mode hearts
        if (mob->loveTimer > 0) {
            float heartTime = (float)GetTime() * 2.5f;
            for (int h = 0; h < 5; h++) {
                float hx = mob->position.x + GetMobW(mob) / 2 + sinf(heartTime + h * 1.3f) * 12;
                float hy = mob->position.y - 8 + cosf(heartTime * 0.6f + h * 1.1f) * 5;
                DrawCircle((int)hx, (int)hy, 3, (Color){255, 80, 120, 200});
            }
        }

        // Baby growth timer indicator
        if (mob->isBaby && mob->growTimer > 0) {
            char timerStr[16];
            snprintf(timerStr, sizeof(timerStr), "%ds", (int)mob->growTimer);
            DrawGameText(timerStr, (int)mob->position.x, (int)mob->position.y - 18, 10, (Color){255, 255, 100, 180});
        }
    }
}
