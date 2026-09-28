#include "types.h"
#include "net.h"
#include <stdlib.h>
#include <string.h>
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

    // Broadcast to clients if host
    if (NetIsHost()) {
        uint8_t buf[NET_PACKET_MAX];
        PktEntitySpawn es;
        es.itemType = itemType;
        es.count = count;
        es.x = x;
        es.y = y;
        buf[0] = PKT_ENTITY_SPAWN;
        memcpy(buf + 1, &es, sizeof(PktEntitySpawn));
        NetSendToAll(buf, 1 + sizeof(PktEntitySpawn), false);
    }
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
{    float pcx = px + PLAYER_WIDTH / 2;
    float pcy = py + PLAYER_HEIGHT / 2;

    for (int i = 0; i < MAX_ENTITIES; i++) {
        ItemEntity *e = &entities[i];
        if (!e->active || e->pickupDelay > 0.0f) continue;

        float dx = pcx - (e->position.x + 5);
        float dy = pcy - (e->position.y + 5);
        float dist = sqrtf(dx * dx + dy * dy);

        if (dist < ENTITY_PICKUP_DIST) {
            // Try to add to inventory
            int added = AddToInventoryCount((BlockType)e->itemType, e->count);
            if (added > 0) {
                e->count -= added;
                if (e->count <= 0) e->active = false;
                PlaySoundPickup();
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

//----------------------------------------------------------------------------------
// Minecarts: rideable carts that run along rail networks.
//
// Movement is segment-based: the cart lives in a rail cell and travels from
// cell centre to cell centre along (dirX, dirY). At each cell centre the next
// direction is picked from the neighbouring rails (straight first, then slope
// and turn connections, reverse only as a last resort). Uphill travel sheds
// speed, downhill gains it; powered rails push the cart back up to boost
// speed while their cell carries redstone power.
//
// Multiplayer: carts are host/single-player only for now (not replicated to
// clients), so network clients never see or mount them.
//----------------------------------------------------------------------------------
Minecart carts[MAX_CARTS];

void InitCarts(void)
{
    for (int i = 0; i < MAX_CARTS; i++) {
        carts[i].active = false;
        carts[i].passengerId = -1;
    }
}

bool SpawnMinecartAt(int bx, int by)
{
    if (!IsRailBlockAt(bx, by)) return false;
    for (int i = 0; i < MAX_CARTS; i++) {
        if (carts[i].active) continue;
        Minecart *c = &carts[i];
        memset(c, 0, sizeof(Minecart));
        c->railX = bx;
        c->railY = by;
        c->position.x = (float)bx * BLOCK_SIZE + (BLOCK_SIZE - CART_BOX_W) / 2.0f;
        c->position.y = (float)by * BLOCK_SIZE + (BLOCK_SIZE - CART_BOX_H) / 2.0f;
        c->dirX = 1;
        c->dirY = 0;
        c->speed = 0.0f;
        c->passengerId = -1;
        c->active = true;
        return true;
    }
    return false;
}

void RemoveCartDrop(int idx)
{
    if (idx < 0 || idx >= MAX_CARTS || !carts[idx].active) return;
    int pid = carts[idx].passengerId;
    if (pid >= 0 && pid < MAX_NET_PLAYERS) players[pid].ridingCart = -1;
    SpawnItemEntity(ITEM_MINECART, 1,
                    carts[idx].position.x + CART_BOX_W / 2.0f - 5.0f,
                    carts[idx].position.y);
    carts[idx].active = false;
}

int FindCartNear(float wx, float wy, float radius)
{
    int best = -1;
    float bestDist = radius * radius;
    for (int i = 0; i < MAX_CARTS; i++) {
        if (!carts[i].active) continue;
        if (carts[i].passengerId >= 0) continue;   // occupied carts are not mountable
        float cx = carts[i].position.x + CART_BOX_W / 2.0f;
        float cy = carts[i].position.y + CART_BOX_H / 2.0f;
        float dx = wx - cx, dy = wy - cy;
        float d2 = dx * dx + dy * dy;
        if (d2 < bestDist) { bestDist = d2; best = i; }
    }
    return best;
}

// Choose the next direction when the cart reaches a cell centre.
static bool PickNextCartDir(Minecart *c)
{
    int bx = c->railX, by = c->railY;
    int cands[8][2];
    int n = 0;
    if (c->dirY == 0) {
        int sx = (c->dirX >= 0) ? 1 : -1;
        cands[n][0]=sx;  cands[n][1]=0;  n++;   // straight
        cands[n][0]=sx;  cands[n][1]=-1; n++;   // uphill slope
        cands[n][0]=sx;  cands[n][1]=1;  n++;   // downhill slope
        cands[n][0]=0;   cands[n][1]=-1; n++;   // vertical turns
        cands[n][0]=0;   cands[n][1]=1;  n++;
        cands[n][0]=-sx; cands[n][1]=0;  n++;   // reverse (last resort)
        cands[n][0]=-sx; cands[n][1]=-1; n++;
        cands[n][0]=-sx; cands[n][1]=1;  n++;
    } else {
        int sy = (c->dirY >= 0) ? 1 : -1;
        cands[n][0]=0;  cands[n][1]=sy;  n++;
        cands[n][0]=-1; cands[n][1]=sy;  n++;
        cands[n][0]=1;  cands[n][1]=sy;  n++;
        cands[n][0]=-1; cands[n][1]=0;   n++;
        cands[n][0]=1;  cands[n][1]=0;   n++;
        cands[n][0]=0;  cands[n][1]=-sy; n++;
        cands[n][0]=-1; cands[n][1]=-sy; n++;
        cands[n][0]=1;  cands[n][1]=-sy; n++;
    }
    for (int k = 0; k < n; k++) {
        int dx = cands[k][0], dy = cands[k][1];
        if (IsRailBlockAt(bx + dx, by + dy)) {
            c->dirX = dx;
            c->dirY = dy;
            return true;
        }
    }
    return false;
}

void UpdateCarts(float dt)
{
    for (int i = 0; i < MAX_CARTS; i++) {
        Minecart *c = &carts[i];
        if (!c->active) continue;

        // Rail removed under the cart: drop it as an item
        if (!IsRailBlockAt(c->railX, c->railY)) {
            RemoveCartDrop(i);
            continue;
        }

        // Rider input: A/D accelerates forward or brakes
        if (c->passengerId >= 0 && c->passengerId < MAX_NET_PLAYERS) {
            Player *p = &players[c->passengerId];
            if (p->playerDead) {
                p->ridingCart = -1;
                c->passengerId = -1;
            } else {
                float move = p->netControlled
                    ? p->moveInput
                    : (IsMoveLeftDown() ? -1.0f : (IsMoveRightDown() ? 1.0f : 0.0f));
                if (c->speed <= 0.1f && move != 0.0f) {
                    // Start moving in the input direction if track leads that way
                    int sx = (move > 0.0f) ? 1 : -1;
                    if (IsRailBlockAt(c->railX + sx, c->railY)) {
                        c->dirX = sx; c->dirY = 0;
                        c->speed = CART_SPEED_MIN;
                    } else if (IsRailBlockAt(c->railX + sx, c->railY - 1)) {
                        c->dirX = sx; c->dirY = -1;
                        c->speed = CART_SPEED_MIN;
                    } else if (IsRailBlockAt(c->railX + sx, c->railY + 1)) {
                        c->dirX = sx; c->dirY = 1;
                        c->speed = CART_SPEED_MIN;
                    }
                } else if (move != 0.0f && c->dirY == 0) {
                    float along = move * c->dirX;
                    if (along > 0.0f) {
                        c->speed += CART_SLOPE_ACCEL * dt;
                    } else {
                        c->speed -= CART_SLOPE_DECEL * dt;
                        if (c->speed < 0.0f) c->speed = 0.0f;
                    }
                }
            }
        }

        // Powered rail: restore boost speed while the cell is energised
        if (GetBlock(c->railX, c->railY) == BLOCK_POWERED_RAIL &&
            GetRedstonePowerAt(c->railX, c->railY) > 0 &&
            c->speed < CART_BOOST_SPEED) {
            c->speed = CART_BOOST_SPEED;
        }

        // Slopes and friction
        if (c->dirY < 0) {
            c->speed -= CART_SLOPE_DECEL * dt;
            if (c->speed <= 5.0f) {
                // Stalled climbing: roll back down
                c->dirX = -c->dirX;
                c->dirY = -c->dirY;
                c->speed = CART_SPEED_MIN;
            }
        } else if (c->dirY > 0) {
            c->speed += CART_SLOPE_ACCEL * dt;
        } else {
            c->speed -= CART_FRICTION * dt;
            if (c->speed < 0.0f) c->speed = 0.0f;
        }
        if (c->speed > CART_SPEED_MAX) c->speed = CART_SPEED_MAX;
        if (c->speed <= 0.0f) continue;   // parked

        // Advance along the rail, cell by cell
        float remaining = c->speed * dt;
        for (int step = 0; step < 8 && remaining > 0.0f; step++) {
            float dirLen = sqrtf((float)(c->dirX * c->dirX + c->dirY * c->dirY));
            float normX = (dirLen > 0.0f) ? c->dirX / dirLen : 0.0f;
            float normY = (dirLen > 0.0f) ? c->dirY / dirLen : 0.0f;
            float ccx = c->position.x + CART_BOX_W / 2.0f;
            float ccy = c->position.y + CART_BOX_H / 2.0f;
            float nextCX = (c->railX + c->dirX) * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
            float nextCY = (c->railY + c->dirY) * BLOCK_SIZE + BLOCK_SIZE / 2.0f;
            float distToNext = fabsf((nextCX - ccx) * normX) + fabsf((nextCY - ccy) * normY);
            if (distToNext <= 0.01f) {
                // Already at the next centre: switch cells
                c->railX += c->dirX;
                c->railY += c->dirY;
                if (!IsRailBlockAt(c->railX, c->railY) || !PickNextCartDir(c)) {
                    c->speed = 0.0f;
                    break;
                }
                continue;
            }
            if (remaining < distToNext) {
                c->position.x += normX * remaining;
                c->position.y += normY * remaining;
                remaining = 0.0f;
            } else {
                c->position.x = nextCX - CART_BOX_W / 2.0f;
                c->position.y = nextCY - CART_BOX_H / 2.0f;
                remaining -= distToNext;
                c->railX += c->dirX;
                c->railY += c->dirY;
                if (!IsRailBlockAt(c->railX, c->railY) || !PickNextCartDir(c)) {
                    c->speed = 0.0f;
                    break;
                }
            }
        }

        // Keep the rider attached
        if (c->passengerId >= 0 && c->passengerId < MAX_NET_PLAYERS) {
            Player *p = &players[c->passengerId];
            p->position.x = c->position.x + CART_BOX_W / 2.0f - PLAYER_WIDTH / 2.0f;
            p->position.y = c->position.y - PLAYER_HEIGHT + 4.0f;
            p->velocity.x = 0.0f;
            p->velocity.y = 0.0f;
            p->fallDistance = 0.0f;
            p->fallPeakVel = 0.0f;
            p->onGround = false;
        }
    }
}

void DrawCarts(void)
{
    for (int i = 0; i < MAX_CARTS; i++) {
        Minecart *c = &carts[i];
        if (!c->active) continue;
        int x = (int)c->position.x;
        int y = (int)c->position.y;
        // Body
        DrawRectangle(x + 1, y + 3, CART_BOX_W - 2, CART_BOX_H - 6, (Color){120, 120, 130, 255});
        DrawRectangle(x + 1, y + 3, CART_BOX_W - 2, 2, (Color){165, 165, 175, 255});
        // Wheels
        DrawRectangle(x + 3, y + CART_BOX_H - 3, 4, 3, (Color){40, 40, 44, 255});
        DrawRectangle(x + CART_BOX_W - 7, y + CART_BOX_H - 3, 4, 3, (Color){40, 40, 44, 255});
    }
}
