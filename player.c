#include "types.h"
#include <math.h>

//----------------------------------------------------------------------------------
// Player Init
//----------------------------------------------------------------------------------
void InitPlayer(void)
{
    int spawnX = WORLD_WIDTH / 2;
    int spawnY = 0;
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        if (world[spawnX][y] != BLOCK_AIR && world[spawnX][y] != BLOCK_WATER) {
            spawnY = y - 3;
            break;
        }
    }
    player.position = (Vector2){ spawnX * BLOCK_SIZE, spawnY * BLOCK_SIZE };
    player.velocity = (Vector2){ 0, 0 };
    player.onGround = false;
    player.selectedSlot = 0;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        player.inventory[i] = BLOCK_AIR;
        player.inventoryCount[i] = 0;
    }
    player.inventory[0] = BLOCK_PLANKS;
    player.inventoryCount[0] = 64;
    player.inventory[1] = BLOCK_DIRT;
    player.inventoryCount[1] = 32;
    player.inventory[2] = BLOCK_COBBLESTONE;
    player.inventoryCount[2] = 32;
}

//----------------------------------------------------------------------------------
// Inventory
//----------------------------------------------------------------------------------
bool AddToInventory(BlockType item)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (player.inventory[i] == item && player.inventoryCount[i] < 64) {
            player.inventoryCount[i]++;
            return true;
        }
    }
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            player.inventory[i] = item;
            player.inventoryCount[i] = 1;
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------------
// Player Physics
//----------------------------------------------------------------------------------
void PlayerPhysics(float dt)
{
    player.velocity.x = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) player.velocity.x = -MOVE_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) player.velocity.x = MOVE_SPEED;

    if ((IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_SPACE)) && player.onGround) {
        player.velocity.y = JUMP_VELOCITY;
        player.onGround = false;
    }

    player.velocity.y += GRAVITY * dt;
    if (player.velocity.y > 800.0f) player.velocity.y = 800.0f;

    // Horizontal collision
    float newX = player.position.x + player.velocity.x * dt;
    float left = newX;
    float right = newX + PLAYER_WIDTH;
    float top = player.position.y;
    float bottom = player.position.y + PLAYER_HEIGHT;

    bool blocked = false;
    int minBX = (int)(left) / BLOCK_SIZE;
    int maxBX = (int)(right - 0.01f) / BLOCK_SIZE;
    int minBY = (int)(top) / BLOCK_SIZE;
    int maxBY = (int)(bottom - 0.01f) / BLOCK_SIZE;

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    if (blocked) {
        if (player.velocity.x > 0) {
            newX = (int)(right / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_WIDTH - 0.01f;
        } else if (player.velocity.x < 0) {
            newX = (int)(left / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.x = 0;
    }
    player.position.x = newX;

    // Vertical collision
    float newY = player.position.y + player.velocity.y * dt;
    left = player.position.x;
    right = player.position.x + PLAYER_WIDTH;
    top = newY;
    bottom = newY + PLAYER_HEIGHT;

    blocked = false;
    minBX = (int)(left) / BLOCK_SIZE;
    maxBX = (int)(right - 0.01f) / BLOCK_SIZE;
    minBY = (int)(top) / BLOCK_SIZE;
    maxBY = (int)(bottom - 0.01f) / BLOCK_SIZE;

    for (int bx = minBX; bx <= maxBX; bx++) {
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    player.onGround = false;
    if (blocked) {
        if (player.velocity.y > 0) {
            newY = (int)(bottom / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_HEIGHT;
            player.onGround = true;
        } else if (player.velocity.y < 0) {
            newY = (int)(top / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.y = 0;
    }
    player.position.y = newY;

    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > (WORLD_WIDTH - 1) * BLOCK_SIZE)
        player.position.x = (WORLD_WIDTH - 1) * BLOCK_SIZE;
}

//----------------------------------------------------------------------------------
// Block Interaction
//----------------------------------------------------------------------------------
void PlayerBlockInteraction(void)
{
    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX < 0 || blockX >= WORLD_WIDTH || blockY < 0 || blockY >= WORLD_HEIGHT) return;

    float playerCenterX = player.position.x + PLAYER_WIDTH / 2.0f;
    float playerCenterY = player.position.y + PLAYER_HEIGHT / 2.0f;
    float distX = fabsf((blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterX) / BLOCK_SIZE;
    float distY = fabsf((blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterY) / BLOCK_SIZE;
    float dist = sqrtf(distX * distX + distY * distY);

    if (dist > BREAK_RANGE) return;

    // Break block
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        BlockType bt = (BlockType)world[blockX][blockY];
        if (bt != BLOCK_AIR && bt != BLOCK_WATER && blockInfo[bt].breakable) {
            world[blockX][blockY] = BLOCK_AIR;
            AddToInventory(bt);
            InvalidateChunkAt(blockX, blockY);
            if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
            if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
        }
    }

    // Place block
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        BlockType selected = (BlockType)player.inventory[player.selectedSlot];
        if (selected != BLOCK_AIR && player.inventoryCount[player.selectedSlot] > 0) {
            if (world[blockX][blockY] == BLOCK_AIR || world[blockX][blockY] == BLOCK_WATER) {
                float bLeft = blockX * BLOCK_SIZE;
                float bRight = bLeft + BLOCK_SIZE;
                float bTop = blockY * BLOCK_SIZE;
                float bBottom = bTop + BLOCK_SIZE;

                float pLeft = player.position.x;
                float pRight = pLeft + PLAYER_WIDTH;
                float pTop = player.position.y;
                float pBottom = pTop + PLAYER_HEIGHT;

                if (!(pRight > bLeft && pLeft < bRight && pBottom > bTop && pTop < bBottom)) {
                    world[blockX][blockY] = selected;
                    player.inventoryCount[player.selectedSlot]--;
                    if (player.inventoryCount[player.selectedSlot] <= 0) {
                        player.inventory[player.selectedSlot] = BLOCK_AIR;
                    }
                    InvalidateChunkAt(blockX, blockY);
                    if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                    if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------
// Player Update
//----------------------------------------------------------------------------------
void UpdatePlayer(float dt)
{
    PlayerPhysics(dt);
    PlayerBlockInteraction();
}

//----------------------------------------------------------------------------------
// Camera System
//----------------------------------------------------------------------------------
void InitCameraSystem(void)
{
    camera.offset = (Vector2){ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    camera.target = (Vector2){ player.position.x + PLAYER_WIDTH / 2, player.position.y + PLAYER_HEIGHT / 2 };
    camera.rotation = 0.0f;
    camera.zoom = 2.0f;
}

void UpdateCameraSystem(float dt)
{
    Vector2 playerCenter = {
        player.position.x + PLAYER_WIDTH / 2.0f,
        player.position.y + PLAYER_HEIGHT / 2.0f
    };
    camera.target = Vector2Lerp(camera.target, playerCenter, 8.0f * dt);
}

//----------------------------------------------------------------------------------
// Hotbar Input
//----------------------------------------------------------------------------------
void UpdateHotbar(void)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (IsKeyPressed(KEY_ONE + i)) player.selectedSlot = i;
    }

    float wheel = GetMouseWheelMove();
    if (wheel < 0) player.selectedSlot = (player.selectedSlot + 1) % HOTBAR_SLOTS;
    if (wheel > 0) player.selectedSlot = (player.selectedSlot - 1 + HOTBAR_SLOTS) % HOTBAR_SLOTS;
}
