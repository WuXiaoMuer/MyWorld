#include "types.h"
#include <math.h>
#include <stdio.h>

//----------------------------------------------------------------------------------
// Find safe spawn point on land (not in water)
//----------------------------------------------------------------------------------
static void FindSpawnPoint(int *outX, int *outY)
{
    int centerX = WORLD_WIDTH / 2;
    // Search outward from center for a land column
    for (int offset = 0; offset < WORLD_WIDTH / 2; offset += 4) {
        for (int dir = -1; dir <= 1; dir += 2) {
            int x = centerX + offset * dir;
            if (x < 0 || x >= WORLD_WIDTH) continue;
            // Check surface block - must be grass (land), not water
            for (int y = 0; y < WORLD_HEIGHT - 5; y++) {
                if (world[x][y] == BLOCK_GRASS) {
                    *outX = x;
                    *outY = y - 3;
                    return;
                }
                if (world[x][y] == BLOCK_WATER) break; // skip water columns
            }
        }
    }
    // Fallback: find any grass block
    for (int x = 0; x < WORLD_WIDTH; x += 16) {
        for (int y = 0; y < WORLD_HEIGHT - 5; y++) {
            if (world[x][y] == BLOCK_GRASS) {
                *outX = x;
                *outY = y - 3;
                return;
            }
        }
    }
    // Last resort: use center
    *outX = centerX;
    *outY = 60;
}

//----------------------------------------------------------------------------------
// Player Init
//----------------------------------------------------------------------------------
void InitPlayer(void)
{
    int spawnX, spawnY;
    FindSpawnPoint(&spawnX, &spawnY);
    player.position = (Vector2){ spawnX * BLOCK_SIZE, spawnY * BLOCK_SIZE };
    player.velocity = (Vector2){ 0, 0 };
    player.onGround = false;
    player.selectedSlot = 0;

    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        player.inventory[i] = BLOCK_AIR;
        player.inventoryCount[i] = 0;
        player.toolDurability[i] = 0;
    }
    player.inventory[0] = BLOCK_PLANKS;
    player.inventoryCount[0] = 64;
    player.inventory[1] = BLOCK_DIRT;
    player.inventoryCount[1] = 32;
    player.inventory[2] = BLOCK_COBBLESTONE;
    player.inventoryCount[2] = 32;

    // Status
    player.health = MAX_HEALTH;
    player.hunger = MAX_HUNGER;
    player.oxygen = MAX_OXYGEN;
    player.xp = 0;
    player.oxygenTimer = 0.0f;
    player.hungerTimer = 0.0f;
    player.regenTimer = 0.0f;
    player.drownTimer = 0.0f;
    player.hungerDamageTimer = 0.0f;
}

//----------------------------------------------------------------------------------
// Inventory
//----------------------------------------------------------------------------------
bool AddToInventory(BlockType item)
{
    // Tools don't stack, always use new slot
    if (IsTool(item)) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (player.inventory[i] == BLOCK_AIR) {
                player.inventory[i] = item;
                player.inventoryCount[i] = 1;
                player.toolDurability[i] = GetToolMaxDurability(item);
                return true;
            }
        }
        snprintf(messageText, sizeof(messageText), "Inventory full!");
        messageTimer = MESSAGE_DURATION;
        return false;
    }

    // Try to stack in existing slots
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == item && player.inventoryCount[i] < 64) {
            player.inventoryCount[i]++;
            return true;
        }
    }
    // Find empty slot
    for (int i = 0; i < INVENTORY_SLOTS; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            player.inventory[i] = item;
            player.inventoryCount[i] = 1;
            return true;
        }
    }
    // Inventory full
    snprintf(messageText, sizeof(messageText), "Inventory full!");
    messageTimer = MESSAGE_DURATION;
    return false;
}

//----------------------------------------------------------------------------------
// Tool System
//----------------------------------------------------------------------------------
bool IsTool(BlockType item)
{
    return item >= TOOL_WOOD_PICKAXE && item <= TOOL_IRON_SHOVEL;
}

int GetToolMaxDurability(BlockType tool)
{
    if (tool == TOOL_WOOD_PICKAXE || tool == TOOL_WOOD_AXE || tool == TOOL_WOOD_SWORD || tool == TOOL_WOOD_SHOVEL) return DURABILITY_WOOD;
    if (tool == TOOL_STONE_PICKAXE || tool == TOOL_STONE_AXE || tool == TOOL_STONE_SWORD || tool == TOOL_STONE_SHOVEL) return DURABILITY_STONE;
    if (tool == TOOL_IRON_PICKAXE || tool == TOOL_IRON_AXE || tool == TOOL_IRON_SWORD || tool == TOOL_IRON_SHOVEL) return DURABILITY_IRON;
    return 0;
}

bool IsFood(BlockType item)
{
    return item >= FOOD_RAW_PORK && item <= FOOD_BREAD;
}

int GetFoodValue(BlockType item)
{
    switch (item) {
    case FOOD_RAW_PORK: return FOOD_RAW_PORK_VALUE;
    case FOOD_COOKED_PORK: return FOOD_COOKED_PORK_VALUE;
    case FOOD_APPLE: return FOOD_APPLE_VALUE;
    case FOOD_BREAD: return FOOD_BREAD_VALUE;
    default: return 0;
    }
}

float GetToolMiningSpeed(BlockType tool, BlockType block)
{
    if (!IsTool(tool)) return 1.0f; // bare hands

    bool isPickaxe = (tool == TOOL_WOOD_PICKAXE || tool == TOOL_STONE_PICKAXE || tool == TOOL_IRON_PICKAXE);
    bool isAxe = (tool == TOOL_WOOD_AXE || tool == TOOL_STONE_AXE || tool == TOOL_IRON_AXE);
    bool isSword = (tool == TOOL_WOOD_SWORD || tool == TOOL_STONE_SWORD || tool == TOOL_IRON_SWORD);
    bool isShovel = (tool == TOOL_WOOD_SHOVEL || tool == TOOL_STONE_SHOVEL || tool == TOOL_IRON_SHOVEL);

    // Tier multiplier: wood=1.5, stone=2.5, iron=4.0
    float tier = 1.0f;
    if (tool == TOOL_WOOD_PICKAXE || tool == TOOL_WOOD_AXE || tool == TOOL_WOOD_SWORD || tool == TOOL_WOOD_SHOVEL) tier = 1.5f;
    if (tool == TOOL_STONE_PICKAXE || tool == TOOL_STONE_AXE || tool == TOOL_STONE_SWORD || tool == TOOL_STONE_SHOVEL) tier = 2.5f;
    if (tool == TOOL_IRON_PICKAXE || tool == TOOL_IRON_AXE || tool == TOOL_IRON_SWORD || tool == TOOL_IRON_SHOVEL) tier = 4.0f;

    // Correct tool bonus
    if (isPickaxe && (block == BLOCK_STONE || block == BLOCK_COBBLESTONE || block == BLOCK_COAL_ORE || block == BLOCK_IRON_ORE || block == BLOCK_FURNACE || block == BLOCK_SANDSTONE)) {
        return tier * 1.5f;
    }
    if (isAxe && (block == BLOCK_WOOD || block == BLOCK_PLANKS)) {
        return tier * 1.5f;
    }
    if (isSword && (block == BLOCK_LEAVES || block == BLOCK_TALL_GRASS)) {
        return tier * 1.5f;
    }
    if (isShovel && (block == BLOCK_DIRT || block == BLOCK_SAND || block == BLOCK_GRAVEL || block == BLOCK_CLAY)) {
        return tier * 1.5f;
    }

    return tier;
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
        PlaySoundJump();
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

    bool wasOnGround = player.onGround;
    player.onGround = false;
    if (blocked) {
        if (player.velocity.y > 0) {
            newY = (int)(bottom / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_HEIGHT;
            player.onGround = true;
            if (!wasOnGround && player.velocity.y > 100.0f) PlaySoundLand();
        } else if (player.velocity.y < 0) {
            newY = (int)(top / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.y = 0;
    }
    player.position.y = newY;

    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > (WORLD_WIDTH - 1) * BLOCK_SIZE)
        player.position.x = (WORLD_WIDTH - 1) * BLOCK_SIZE;

    // Death from falling out of world
    if (player.position.y > DEATH_Y) {
        player.health = 0;
    }
}

//----------------------------------------------------------------------------------
// Block Interaction (hold-to-mine with tool speed)
//----------------------------------------------------------------------------------
static int miningBlockX = -1, miningBlockY = -1;
static float miningProgress = 0.0f;

void PlayerBlockInteraction(void)
{
    if (inventoryOpen || gamePaused) return;

    Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
    int blockX = (int)(mouseWorld.x / BLOCK_SIZE);
    int blockY = (int)(mouseWorld.y / BLOCK_SIZE);

    if (blockX < 0 || blockX >= WORLD_WIDTH || blockY < 0 || blockY >= WORLD_HEIGHT) {
        miningProgress = 0.0f;
        return;
    }

    float playerCenterX = player.position.x + PLAYER_WIDTH / 2.0f;
    float playerCenterY = player.position.y + PLAYER_HEIGHT / 2.0f;
    float distX = fabsf((blockX * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterX) / BLOCK_SIZE;
    float distY = fabsf((blockY * BLOCK_SIZE + BLOCK_SIZE / 2.0f) - playerCenterY) / BLOCK_SIZE;
    float dist = sqrtf(distX * distX + distY * distY);

    if (dist > BREAK_RANGE) {
        miningProgress = 0.0f;
        return;
    }

    BlockType selectedTool = (BlockType)player.inventory[player.selectedSlot];
    float toolSpeed = GetToolMiningSpeed(selectedTool, (BlockType)world[blockX][blockY]);

    // Hold left to mine / attack mobs
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Check mob hit first
        for (int i = 0; i < MAX_MOBS; i++) {
            if (!mobs[i].active || mobs[i].deathTimer > 0) continue;
            if (IsPlayerNearMob(&mobs[i], BREAK_RANGE * BLOCK_SIZE)) {
                int mw = (mobs[i].type == MOB_ZOMBIE) ? 12 : 16;
                int mh = (mobs[i].type == MOB_ZOMBIE) ? 28 : 12;
                float mLeft = mobs[i].position.x;
                float mRight = mLeft + mw;
                float mTop = mobs[i].position.y;
                float mBottom = mTop + mh;

                if (mouseWorld.x >= mLeft && mouseWorld.x <= mRight &&
                    mouseWorld.y >= mTop && mouseWorld.y <= mBottom) {
                    // Attack mob
                    int damage = 1;
                    if (IsTool(selectedTool)) {
                        // Swords deal more damage
                        if (selectedTool == TOOL_WOOD_SWORD) damage = 3;
                        else if (selectedTool == TOOL_STONE_SWORD) damage = 4;
                        else if (selectedTool == TOOL_IRON_SWORD) damage = 6;
                        else damage = 2; // Other tools
                    }
                    static float attackCooldown = 0.0f;
                    attackCooldown -= GetFrameTime();
                    if (attackCooldown <= 0) {
                        DamageMob(&mobs[i], damage);
                        attackCooldown = 0.4f;
                        // Consume durability
                        if (IsTool(selectedTool)) {
                            int slot = player.selectedSlot;
                            player.toolDurability[slot]--;
                            if (player.toolDurability[slot] <= 0) {
                                player.inventory[slot] = BLOCK_AIR;
                                player.inventoryCount[slot] = 0;
                                player.toolDurability[slot] = 0;
                                snprintf(messageText, sizeof(messageText), "Tool broke!");
                                messageTimer = MESSAGE_DURATION;
                            }
                        }
                    }
                    return; // Don't mine block if we hit a mob
                }
            }
        }

        BlockType bt = (BlockType)world[blockX][blockY];
        if (bt != BLOCK_AIR && bt != BLOCK_WATER && blockInfo[bt].breakable) {
            // Reset progress if target changed
            if (blockX != miningBlockX || blockY != miningBlockY) {
                miningBlockX = blockX;
                miningBlockY = blockY;
                miningProgress = 0.0f;
            }
            float dt = GetFrameTime();
            float baseMineTime = 0.4f; // seconds for bare hands
            miningProgress += (dt * toolSpeed) / baseMineTime;

            if (miningProgress >= 1.0f) {
                world[blockX][blockY] = BLOCK_AIR;
                AddToInventory(bt);
                PlaySoundBreak(bt);
                InvalidateChunkAt(blockX, blockY);
                if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                miningProgress = 0.0f;
                miningBlockX = -1;

                // Leaves have a chance to drop apples
                if (bt == BLOCK_LEAVES && (hash2D(blockX, blockY, 12345) % 10) == 0) {
                    AddToInventory(FOOD_APPLE);
                }

                // Consume tool durability
                if (IsTool(selectedTool)) {
                    int slot = player.selectedSlot;
                    player.toolDurability[slot]--;
                    if (player.toolDurability[slot] <= 0) {
                        // Tool breaks
                        player.inventory[slot] = BLOCK_AIR;
                        player.inventoryCount[slot] = 0;
                        player.toolDurability[slot] = 0;
                        snprintf(messageText, sizeof(messageText), "Tool broke!");
                        messageTimer = MESSAGE_DURATION;
                    }
                }
            }
        }
    } else {
        miningProgress = 0.0f;
    }

    // Place block or eat food (right click, instant)
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        // Eat food
        if (IsFood(selectedTool) && player.hunger < MAX_HUNGER) {
            int foodVal = GetFoodValue(selectedTool);
            player.hunger += foodVal;
            if (player.hunger > MAX_HUNGER) player.hunger = MAX_HUNGER;
            player.inventoryCount[player.selectedSlot]--;
            if (player.inventoryCount[player.selectedSlot] <= 0) {
                player.inventory[player.selectedSlot] = BLOCK_AIR;
            }
            PlaySoundEat();
            snprintf(messageText, sizeof(messageText), "Ate %s (+%d hunger)", blockInfo[selectedTool].name, foodVal);
            messageTimer = MESSAGE_DURATION;
            return;
        }

        if (IsTool(selectedTool) || IsFood(selectedTool)) return; // Can't place tools or food
        if (selectedTool != BLOCK_AIR && player.inventoryCount[player.selectedSlot] > 0) {
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
                    world[blockX][blockY] = selectedTool;
                    player.inventoryCount[player.selectedSlot]--;
                    if (player.inventoryCount[player.selectedSlot] <= 0) {
                        player.inventory[player.selectedSlot] = BLOCK_AIR;
                    }
                    PlaySoundPlace(selectedTool);
                    InvalidateChunkAt(blockX, blockY);
                    if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                    if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                }
            }
        }
    }
}

float GetMiningProgress(void)
{
    return miningProgress;
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
// Player Status System
//----------------------------------------------------------------------------------
bool IsPlayerUnderwater(void)
{
    int bx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
    int by = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
    if (bx < 0 || bx >= WORLD_WIDTH || by < 0 || by >= WORLD_HEIGHT) return false;
    return world[bx][by] == BLOCK_WATER;
}

void UpdatePlayerStatus(float dt)
{
    if (gamePaused || inventoryOpen) return;

    bool underwater = IsPlayerUnderwater();

    // --- Oxygen ---
    if (underwater) {
        player.oxygenTimer += dt;
        if (player.oxygenTimer >= 1.0f / OXYGEN_DRAIN_RATE) {
            player.oxygenTimer -= 1.0f / OXYGEN_DRAIN_RATE;
            if (player.oxygen > 0) player.oxygen--;
        }
        // Drowning damage when oxygen depleted
        if (player.oxygen <= 0) {
            player.drownTimer += dt;
            if (player.drownTimer >= 1.0f / DROWN_DAMAGE_RATE) {
                player.drownTimer -= 1.0f / DROWN_DAMAGE_RATE;
                player.health -= 2;
                if (player.health < 0) player.health = 0;
                PlaySoundHurt();
            }
        }
    } else {
        // Recover oxygen when out of water
        player.oxygenTimer = 0.0f;
        player.drownTimer = 0.0f;
        if (player.oxygen < MAX_OXYGEN) {
            player.oxygenTimer += dt;
            if (player.oxygenTimer >= 1.0f) {
                player.oxygenTimer -= 1.0f;
                player.oxygen++;
                if (player.oxygen > MAX_OXYGEN) player.oxygen = MAX_OXYGEN;
            }
        }
    }

    // --- Hunger ---
    float hungerRate = HUNGER_DRAIN_RATE;
    // TODO: multiply by HUNGER_SPRINT_MULT when sprinting
    player.hungerTimer += dt;
    if (player.hungerTimer >= 1.0f / hungerRate) {
        player.hungerTimer -= 1.0f / hungerRate;
        if (player.hunger > 0) player.hunger--;
    }

    // Hunger damage at 0 hunger
    if (player.hunger <= 0) {
        player.hungerDamageTimer += dt;
        if (player.hungerDamageTimer >= 1.0f / HUNGER_DAMAGE_RATE) {
            player.hungerDamageTimer -= 1.0f / HUNGER_DAMAGE_RATE;
            player.health--;
            if (player.health < 0) player.health = 0;
            PlaySoundHurt();
        }
    } else {
        player.hungerDamageTimer = 0.0f;
    }

    // --- Health Regen ---
    if (player.health < MAX_HEALTH && player.hunger >= HEALTH_REGEN_HUNGER) {
        player.regenTimer += dt;
        if (player.regenTimer >= 1.0f / HEALTH_REGEN_RATE) {
            player.regenTimer -= 1.0f / HEALTH_REGEN_RATE;
            player.health++;
        }
    } else {
        player.regenTimer = 0.0f;
    }

    // --- Death ---
    if (player.health <= 0) {
        PlaySoundDeath();
        // Lose all items, respawn
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            player.inventory[i] = BLOCK_AIR;
            player.inventoryCount[i] = 0;
            player.toolDurability[i] = 0;
        }
        int spawnX, spawnY;
        FindSpawnPoint(&spawnX, &spawnY);
        player.position = (Vector2){ spawnX * BLOCK_SIZE, spawnY * BLOCK_SIZE };
        player.velocity = (Vector2){ 0, 0 };
        player.onGround = false;
        player.selectedSlot = 0;
        player.health = MAX_HEALTH;
        player.hunger = MAX_HUNGER;
        player.oxygen = MAX_OXYGEN;
        player.oxygenTimer = 0.0f;
        player.hungerTimer = 0.0f;
        player.regenTimer = 0.0f;
        player.drownTimer = 0.0f;
        player.hungerDamageTimer = 0.0f;
        InitCameraSystem();
    }
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
    if (inventoryOpen) return;

    int oldSlot = player.selectedSlot;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (IsKeyPressed(KEY_ONE + i)) player.selectedSlot = i;
    }

    float wheel = GetMouseWheelMove();
    if (wheel < 0) player.selectedSlot = (player.selectedSlot + 1) % HOTBAR_SLOTS;
    if (wheel > 0) player.selectedSlot = (player.selectedSlot - 1 + HOTBAR_SLOTS) % HOTBAR_SLOTS;

    // Show item name when switching slots
    if (player.selectedSlot != oldSlot) {
        BlockType item = (BlockType)player.inventory[player.selectedSlot];
        if (item != BLOCK_AIR) {
            snprintf(messageText, sizeof(messageText), "%s", blockInfo[item].name);
            messageTimer = 0.8f;
        }
    }
}
