#include "types.h"
#include "net.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static StringId pendingDeathCause = STR_DEATH_FALL;

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
        player.itemEnchantments[i] = 0;
    }
    player.inventory[0] = BLOCK_PLANKS;
    player.inventoryCount[0] = 16;

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
    player.facingRight = true;
    player.wasInWater = false;
    player.footstepTimer = 0.0f;
    player.sprinting = false;
    player.knockbackTimer = 0.0f;
    player.damageFlashTimer = 0.0f;
    player.playerDead = false;
    player.fallPeakVel = 0.0f;
    player.coyoteTimer = 0.0f;
    player.jumpBufferTimer = 0.0f;
    player.cameraShakeIntensity = 0.0f;
    player.cameraShakeTimer = 0.0f;
    player.attackCooldown = 0.0f;
    player.spawnX = -1;
    player.spawnY = -1;
    player.netControlled = false;
    player.moveInput = 0.0f;

    // Preserve player name across respawns/init if already set; default otherwise.
    if (player.playerName[0] == '\0') {
        strcpy(player.playerName, "Player");
    }

    for (int i = 0; i < 4; i++) {
        player.armor[i] = BLOCK_AIR;
        player.armorDurability[i] = 0;
        player.armorEnchantments[i] = 0;
    }
}

//----------------------------------------------------------------------------------
// Inventory
//----------------------------------------------------------------------------------
bool AddToInventory(BlockType item)
{
    // Tools and armor don't stack, always use new slot
    if (IsTool(item)) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (player.inventory[i] == BLOCK_AIR) {
                player.inventory[i] = item;
                player.inventoryCount[i] = 1;
                player.toolDurability[i] = GetToolMaxDurability(item);
                return true;
            }
        }
        ShowMessage(S(STR_MSG_INVENTORY_FULL), (Color){240, 80, 80, 255});
        return false;
    }
    if (IsArmor(item)) {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (player.inventory[i] == BLOCK_AIR) {
                player.inventory[i] = item;
                player.inventoryCount[i] = 1;
                player.toolDurability[i] = GetArmorMaxDurability(item);
                return true;
            }
        }
        ShowMessage(S(STR_MSG_INVENTORY_FULL), (Color){240, 80, 80, 255});
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
    ShowMessage(S(STR_MSG_INVENTORY_FULL), (Color){240, 80, 80, 255});
    return false;
}

int AddToInventoryCount(BlockType item, int count)
{
    if (count <= 0) return 0;
    int remaining = count;
    bool nonStackable = IsTool(item) || IsArmor(item);
    int maxStack = nonStackable ? 1 : 64;
    // Try to stack in existing slots (only for stackable items)
    if (!nonStackable) {
        for (int i = 0; i < INVENTORY_SLOTS && remaining > 0; i++) {
            if (player.inventory[i] == item && player.inventoryCount[i] < maxStack) {
                int space = maxStack - player.inventoryCount[i];
                int toAdd = remaining > space ? space : remaining;
                player.inventoryCount[i] += toAdd;
                remaining -= toAdd;
            }
        }
    }
    // Find empty slots
    for (int i = 0; i < INVENTORY_SLOTS && remaining > 0; i++) {
        if (player.inventory[i] == BLOCK_AIR) {
            int toAdd = remaining > maxStack ? maxStack : remaining;
            player.inventory[i] = item;
            player.inventoryCount[i] = toAdd;
            player.toolDurability[i] = IsTool(item) ? GetToolMaxDurability(item) : 0;
            remaining -= toAdd;
            if (nonStackable) break; // one per slot
        }
    }
    if (remaining > 0) {
        ShowMessage(S(STR_MSG_INVENTORY_FULL), (Color){240, 80, 80, 255});
    }
    return count - remaining; // number actually added
}

//----------------------------------------------------------------------------------
// Tool System
//----------------------------------------------------------------------------------
bool IsTool(BlockType item)
{
    return (item >= TOOL_WOOD_PICKAXE && item <= TOOL_WOOD_HOE) ||
           (item >= TOOL_STONE_PICKAXE && item <= TOOL_STONE_HOE) ||
           (item >= TOOL_IRON_PICKAXE && item <= TOOL_IRON_HOE) ||
           (item >= TOOL_GOLD_PICKAXE && item <= TOOL_GOLD_HOE) ||
           (item >= TOOL_DIAMOND_PICKAXE && item <= TOOL_DIAMOND_HOE) ||
           item == ITEM_BOW;
}

int GetToolMaxDurability(BlockType tool)
{
    if (tool == TOOL_WOOD_PICKAXE || tool == TOOL_WOOD_AXE || tool == TOOL_WOOD_SWORD || tool == TOOL_WOOD_SHOVEL || tool == TOOL_WOOD_HOE) return DURABILITY_WOOD;
    if (tool == TOOL_STONE_PICKAXE || tool == TOOL_STONE_AXE || tool == TOOL_STONE_SWORD || tool == TOOL_STONE_SHOVEL || tool == TOOL_STONE_HOE) return DURABILITY_STONE;
    if (tool == TOOL_IRON_PICKAXE || tool == TOOL_IRON_AXE || tool == TOOL_IRON_SWORD || tool == TOOL_IRON_SHOVEL || tool == TOOL_IRON_HOE) return DURABILITY_IRON;
    if (tool == TOOL_GOLD_PICKAXE || tool == TOOL_GOLD_AXE || tool == TOOL_GOLD_SWORD || tool == TOOL_GOLD_SHOVEL || tool == TOOL_GOLD_HOE) return DURABILITY_GOLD;
    if (tool == TOOL_DIAMOND_PICKAXE || tool == TOOL_DIAMOND_AXE || tool == TOOL_DIAMOND_SWORD || tool == TOOL_DIAMOND_SHOVEL || tool == TOOL_DIAMOND_HOE) return DURABILITY_DIAMOND;
    if (tool == ITEM_BOW) return DURABILITY_BOW;
    return 0;
}

bool IsFood(BlockType item)
{
    return (item >= FOOD_RAW_PORK && item <= FOOD_BREAD) ||
           item == ITEM_RAW_BEEF || item == ITEM_COOKED_BEEF ||
           item == ITEM_RAW_MUTTON || item == ITEM_COOKED_MUTTON ||
           item == ITEM_RAW_CHICKEN || item == ITEM_COOKED_CHICKEN ||
           item == ITEM_RAW_FISH || item == ITEM_COOKED_FISH;
}

bool IsSword(BlockType tool)
{
    return tool == TOOL_WOOD_SWORD || tool == TOOL_STONE_SWORD || tool == TOOL_IRON_SWORD ||
           tool == TOOL_GOLD_SWORD || tool == TOOL_DIAMOND_SWORD;
}

bool IsPickaxe(BlockType tool)
{
    return tool == TOOL_WOOD_PICKAXE || tool == TOOL_STONE_PICKAXE || tool == TOOL_IRON_PICKAXE ||
           tool == TOOL_GOLD_PICKAXE || tool == TOOL_DIAMOND_PICKAXE;
}

bool IsHoe(BlockType tool)
{
    return tool == TOOL_WOOD_HOE || tool == TOOL_STONE_HOE || tool == TOOL_IRON_HOE ||
           tool == TOOL_GOLD_HOE || tool == TOOL_DIAMOND_HOE;
}

float GetToolTier(BlockType tool)
{
    if (tool == TOOL_WOOD_PICKAXE || tool == TOOL_WOOD_AXE || tool == TOOL_WOOD_SWORD || tool == TOOL_WOOD_SHOVEL) return 1.5f;
    if (tool == TOOL_STONE_PICKAXE || tool == TOOL_STONE_AXE || tool == TOOL_STONE_SWORD || tool == TOOL_STONE_SHOVEL) return 2.5f;
    if (tool == TOOL_IRON_PICKAXE || tool == TOOL_IRON_AXE || tool == TOOL_IRON_SWORD || tool == TOOL_IRON_SHOVEL) return 4.0f;
    if (tool == TOOL_GOLD_PICKAXE || tool == TOOL_GOLD_AXE || tool == TOOL_GOLD_SWORD || tool == TOOL_GOLD_SHOVEL) return 6.0f;
    if (tool == TOOL_DIAMOND_PICKAXE || tool == TOOL_DIAMOND_AXE || tool == TOOL_DIAMOND_SWORD || tool == TOOL_DIAMOND_SHOVEL) return 5.0f;
    return 1.0f;
}

int GetSwordDamage(BlockType tool)
{
    switch (tool) {
        case TOOL_WOOD_SWORD: return 3;
        case TOOL_STONE_SWORD: return 4;
        case TOOL_IRON_SWORD: return 6;
        case TOOL_GOLD_SWORD: return 4;
        case TOOL_DIAMOND_SWORD: return 8;
        default: return 1;
    }
}

int GetFoodValue(BlockType item)
{
    switch (item) {
    case FOOD_RAW_PORK: return FOOD_RAW_PORK_VALUE;
    case FOOD_COOKED_PORK: return FOOD_COOKED_PORK_VALUE;
    case FOOD_APPLE: return FOOD_APPLE_VALUE;
    case FOOD_BREAD: return FOOD_BREAD_VALUE;
    case ITEM_RAW_BEEF: return FOOD_RAW_BEEF_VALUE;
    case ITEM_COOKED_BEEF: return FOOD_COOKED_BEEF_VALUE;
    case ITEM_RAW_MUTTON: return FOOD_RAW_MUTTON_VALUE;
    case ITEM_COOKED_MUTTON: return FOOD_COOKED_MUTTON_VALUE;
    case ITEM_RAW_CHICKEN: return FOOD_RAW_CHICKEN_VALUE;
    case ITEM_COOKED_CHICKEN: return FOOD_COOKED_CHICKEN_VALUE;
    case ITEM_RAW_FISH: return FOOD_RAW_FISH_VALUE;
    case ITEM_COOKED_FISH: return FOOD_COOKED_FISH_VALUE;
    default: return 0;
    }
}

//----------------------------------------------------------------------------------
// Armor System
//----------------------------------------------------------------------------------
bool IsArmor(BlockType item)
{
    return (item >= ARMOR_WOOD_HELMET && item <= ARMOR_WOOD_BOOTS) ||
           (item >= ARMOR_STONE_HELMET && item <= ARMOR_STONE_BOOTS) ||
           (item >= ARMOR_IRON_HELMET && item <= ARMOR_IRON_BOOTS) ||
           (item >= ARMOR_GOLD_HELMET && item <= ARMOR_GOLD_BOOTS) ||
           (item >= ARMOR_DIAMOND_HELMET && item <= ARMOR_DIAMOND_BOOTS);
}

int GetArmorSlot(BlockType item)
{
    switch (item) {
    case ARMOR_WOOD_HELMET: case ARMOR_STONE_HELMET: case ARMOR_IRON_HELMET:
    case ARMOR_GOLD_HELMET: case ARMOR_DIAMOND_HELMET: return 0;
    case ARMOR_WOOD_CHESTPLATE: case ARMOR_STONE_CHESTPLATE: case ARMOR_IRON_CHESTPLATE:
    case ARMOR_GOLD_CHESTPLATE: case ARMOR_DIAMOND_CHESTPLATE: return 1;
    case ARMOR_WOOD_LEGGINGS: case ARMOR_STONE_LEGGINGS: case ARMOR_IRON_LEGGINGS:
    case ARMOR_GOLD_LEGGINGS: case ARMOR_DIAMOND_LEGGINGS: return 2;
    case ARMOR_WOOD_BOOTS: case ARMOR_STONE_BOOTS: case ARMOR_IRON_BOOTS:
    case ARMOR_GOLD_BOOTS: case ARMOR_DIAMOND_BOOTS: return 3;
    default: return -1;
    }
}

int GetArmorValue(BlockType item)
{
    switch (item) {
    case ARMOR_WOOD_HELMET:     return 1;
    case ARMOR_WOOD_CHESTPLATE: return 3;
    case ARMOR_WOOD_LEGGINGS:   return 2;
    case ARMOR_WOOD_BOOTS:      return 1;
    case ARMOR_STONE_HELMET:    return 2;
    case ARMOR_STONE_CHESTPLATE:return 5;
    case ARMOR_STONE_LEGGINGS:  return 3;
    case ARMOR_STONE_BOOTS:     return 2;
    case ARMOR_IRON_HELMET:     return 3;
    case ARMOR_IRON_CHESTPLATE: return 8;
    case ARMOR_IRON_LEGGINGS:   return 6;
    case ARMOR_IRON_BOOTS:      return 3;
    case ARMOR_GOLD_HELMET:     return 2;
    case ARMOR_GOLD_CHESTPLATE: return 6;
    case ARMOR_GOLD_LEGGINGS:   return 4;
    case ARMOR_GOLD_BOOTS:      return 2;
    case ARMOR_DIAMOND_HELMET:     return 4;
    case ARMOR_DIAMOND_CHESTPLATE: return 10;
    case ARMOR_DIAMOND_LEGGINGS:   return 7;
    case ARMOR_DIAMOND_BOOTS:      return 4;
    default: return 0;
    }
}

int GetArmorMaxDurability(BlockType item)
{
    if (item >= ARMOR_WOOD_HELMET && item <= ARMOR_WOOD_BOOTS) return ARMOR_DURABILITY_WOOD;
    if (item >= ARMOR_STONE_HELMET && item <= ARMOR_STONE_BOOTS) return ARMOR_DURABILITY_STONE;
    if (item >= ARMOR_IRON_HELMET && item <= ARMOR_IRON_BOOTS) return ARMOR_DURABILITY_IRON;
    if (item >= ARMOR_GOLD_HELMET && item <= ARMOR_GOLD_BOOTS) return ARMOR_DURABILITY_GOLD;
    if (item >= ARMOR_DIAMOND_HELMET && item <= ARMOR_DIAMOND_BOOTS) return ARMOR_DURABILITY_DIAMOND;
    return 0;
}

int GetTotalArmorPoints(void)
{
    int total = 0;
    for (int i = 0; i < 4; i++) {
        if (player.armor[i] != BLOCK_AIR) {
            total += GetArmorValue((BlockType)player.armor[i]);
        }
    }
    return total;
}

float GetArmorDamageReduction(void)
{
    int points = GetTotalArmorPoints();
    float reduction = points * 0.04f;
    // Protection enchantment bonus from all armor pieces
    for (int i = 0; i < 4; i++) {
        if (player.armor[i] != BLOCK_AIR && ENCH_TYPE(player.armorEnchantments[i]) == ENCH_PROTECTION) {
            reduction += ENCH_LEVEL(player.armorEnchantments[i]) * 0.02f;
        }
    }
    if (reduction > 0.80f) reduction = 0.80f;
    return reduction;
}

void DamageArmor(void)
{
    for (int i = 0; i < 4; i++) {
        if (player.armor[i] != BLOCK_AIR) {
            // Unbreaking check
            bool skipDur = false;
            if (ENCH_TYPE(player.armorEnchantments[i]) == ENCH_UNBREAKING) {
                skipDur = (rand() % (ENCH_LEVEL(player.armorEnchantments[i]) + 1)) != 0;
            }
            if (!skipDur) {
                player.armorDurability[i]--;
                if (player.armorDurability[i] <= 0) {
                    ShowMessage(Sf(STR_MSG_BROKE, GetBlockName(player.armor[i])),
                               (Color){240, 80, 80, 255});
                    player.armor[i] = BLOCK_AIR;
                    player.armorDurability[i] = 0;
                    player.armorEnchantments[i] = 0;
                }
            }
        }
    }
}

float GetAttackSpeed(BlockType tool)
{
    if (!IsTool(tool)) return ATTACK_SPEED_BARE;
    if (IsSword(tool)) return ATTACK_SPEED_SWORD;
    bool isAxe = (tool == TOOL_WOOD_AXE || tool == TOOL_STONE_AXE || tool == TOOL_IRON_AXE || tool == TOOL_GOLD_AXE || tool == TOOL_DIAMOND_AXE);
    bool isPick = (tool == TOOL_WOOD_PICKAXE || tool == TOOL_STONE_PICKAXE || tool == TOOL_IRON_PICKAXE || tool == TOOL_GOLD_PICKAXE || tool == TOOL_DIAMOND_PICKAXE);
    bool isShovel = (tool == TOOL_WOOD_SHOVEL || tool == TOOL_STONE_SHOVEL || tool == TOOL_IRON_SHOVEL || tool == TOOL_GOLD_SHOVEL || tool == TOOL_DIAMOND_SHOVEL);
    if (isAxe) return ATTACK_SPEED_AXE;
    if (isPick) return ATTACK_SPEED_PICK;
    if (isShovel) return ATTACK_SPEED_SHOVEL;
    return ATTACK_SPEED_BARE;
}

float GetToolMiningSpeed(BlockType tool, BlockType block)
{
    if (!IsTool(tool)) return 1.0f; // bare hands

    bool isPickaxe = (tool == TOOL_WOOD_PICKAXE || tool == TOOL_STONE_PICKAXE || tool == TOOL_IRON_PICKAXE || tool == TOOL_GOLD_PICKAXE || tool == TOOL_DIAMOND_PICKAXE);
    bool isAxe = (tool == TOOL_WOOD_AXE || tool == TOOL_STONE_AXE || tool == TOOL_IRON_AXE || tool == TOOL_GOLD_AXE || tool == TOOL_DIAMOND_AXE);
    bool isShovel = (tool == TOOL_WOOD_SHOVEL || tool == TOOL_STONE_SHOVEL || tool == TOOL_IRON_SHOVEL || tool == TOOL_GOLD_SHOVEL || tool == TOOL_DIAMOND_SHOVEL);

    float tier = GetToolTier(tool);
    float speed = tier;

    // Correct tool bonus
    if (isPickaxe && (block == BLOCK_STONE || block == BLOCK_COBBLESTONE || block == BLOCK_COAL_ORE || block == BLOCK_IRON_ORE || block == BLOCK_GOLD_ORE || block == BLOCK_DIAMOND_ORE || block == BLOCK_REDSTONE_ORE || block == BLOCK_LAPIS_ORE || block == BLOCK_FURNACE || block == BLOCK_SANDSTONE || block == BLOCK_CRAFTING_TABLE || block == BLOCK_CHEST || block == BLOCK_OBSIDIAN)) {
        speed = tier * 1.5f;
    } else if (isAxe && (block == BLOCK_WOOD || block == BLOCK_PLANKS)) {
        speed = tier * 1.5f;
    } else if (IsSword(tool) && (block == BLOCK_LEAVES || block == BLOCK_TALL_GRASS)) {
        speed = tier * 1.5f;
    } else if (isShovel && (block == BLOCK_DIRT || block == BLOCK_SAND || block == BLOCK_GRAVEL || block == BLOCK_CLAY)) {
        speed = tier * 1.5f;
    }

    // Efficiency enchantment
    uint16_t ench = player.itemEnchantments[player.selectedSlot];
    if (ENCH_TYPE(ench) == ENCH_EFFICIENCY) {
        speed *= (1.0f + ENCH_LEVEL(ench) * 0.5f);
    }

    return speed;
}

// Check if tool tier is sufficient to get drops from a block
bool CanToolMineBlock(BlockType tool, BlockType block)
{
    float tier = GetToolTier(tool);
    // Diamond-tier blocks require iron+ pickaxe
    if ((block == BLOCK_DIAMOND_ORE || block == BLOCK_OBSIDIAN) && tier < 4.0f) return false;
    // Gold/redstone/lapis require stone+ pickaxe
    if ((block == BLOCK_GOLD_ORE || block == BLOCK_REDSTONE_ORE || block == BLOCK_LAPIS_ORE) && tier < 2.5f) return false;
    // Iron ore requires stone+ pickaxe
    if (block == BLOCK_IRON_ORE && tier < 2.5f) return false;
    return true;
}

//----------------------------------------------------------------------------------
// Player Physics
//----------------------------------------------------------------------------------
void PlayerPhysics(float dt)
{
    // Knockback preserves horizontal velocity
    if (player.knockbackTimer > 0.0f) {
        player.knockbackTimer -= dt;
    } else {
        // Sprint determination (before movement so we can use it as target)
        bool wantsSprint;
        float targetSpeed = 0.0f;
        if (player.netControlled) {
            // Network-controlled: use moveInput from server
            wantsSprint = player.sprinting;
            targetSpeed = player.moveInput * MOVE_SPEED;
        } else {
            // Local keyboard input
            wantsSprint = Win32IsKeyDown(KEY_LEFT_SHIFT) || Win32IsKeyDown(KEY_RIGHT_SHIFT);
            bool left = Win32IsKeyDown(KEY_A) || Win32IsKeyDown(KEY_LEFT);
            bool right = Win32IsKeyDown(KEY_D) || Win32IsKeyDown(KEY_RIGHT);
            if (left && !right) targetSpeed = -MOVE_SPEED;
            else if (right && !left) targetSpeed = MOVE_SPEED;
        }
        player.sprinting = wantsSprint && player.onGround && player.hunger > 0;

        // Apply sprint multiplier to target, not to velocity
        if (player.sprinting && targetSpeed != 0.0f) {
            targetSpeed *= SPRINT_SPEED_MULT;
        }

        float accel = (targetSpeed != 0.0f) ? MOVE_ACCEL : MOVE_DECEL;
        float diff = targetSpeed - player.velocity.x;
        if (fabsf(diff) < accel * dt) {
            player.velocity.x = targetSpeed;
        } else {
            player.velocity.x += (diff > 0 ? 1.0f : -1.0f) * accel * dt;
        }

        // Sprint dust (local player only)
        if (!player.netControlled && player.sprinting && targetSpeed != 0.0f) {
            static float dustTimer = 0.0f;
            dustTimer -= dt;
            if (dustTimer <= 0.0f) {
                dustTimer = 0.08f;
                SpawnSprintDust(player.position.x + PLAYER_WIDTH / 2.0f,
                                player.position.y + PLAYER_HEIGHT);
            }
        }
    }

    // Update facing direction
    if (player.velocity.x > 0.1f) player.facingRight = true;
    else if (player.velocity.x < -0.1f) player.facingRight = false;

    // Footstep sounds
    bool isMoving = fabsf(player.velocity.x) > 10.0f && player.onGround;
    if (isMoving) {
        float stepInterval = player.sprinting ? 0.35f : 0.5f;
        player.footstepTimer -= dt;
        if (player.footstepTimer <= 0.0f) {
            player.footstepTimer = stepInterval;
            PlaySoundFootstep();
        }
    } else {
        player.footstepTimer = 0.0f;
    }

    // Water physics
    bool inWater = IsPlayerUnderwater();
    // Water splash on entry
    if (inWater && !player.wasInWater) {
        if (!player.netControlled) {
            PlaySoundSplash();
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT / 2,
                                 (Color){100, 160, 220, 200});
        }
    }
    // Water splash on exit
    if (!inWater && player.wasInWater) {
        if (!player.netControlled) {
            PlaySoundSplash();
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT,
                                 (Color){120, 180, 230, 200});
        }
    }
    player.wasInWater = inWater;

    // Coyote time: track time since leaving ground
    if (player.onGround) {
        player.coyoteTimer = COYOTE_TIME;
    } else {
        player.coyoteTimer -= dt;
    }

    // Jump buffer: remember jump presses
    bool jumpPressed, jumpHeld;
    if (player.netControlled) {
        jumpPressed = false; // jump is applied directly from network
        jumpHeld = player.jumpHeld;
    } else {
        jumpPressed = Win32IsKeyPressed(KEY_W) || Win32IsKeyPressed(KEY_UP) || Win32IsKeyPressed(KEY_SPACE);
        jumpHeld = Win32IsKeyDown(KEY_W) || Win32IsKeyDown(KEY_UP) || Win32IsKeyDown(KEY_SPACE);
    }
    if (jumpPressed) {
        player.jumpBufferTimer = JUMP_BUFFER_TIME;
    }
    player.jumpBufferTimer -= dt;

    if (inWater) {
        player.velocity.x *= WATER_SPEED_MULT;
        if (jumpHeld) {
            player.velocity.y = WATER_SWIM_VEL;
        }
    } else {
        // Jump with coyote time + buffering
        if (player.jumpBufferTimer > 0.0f && player.coyoteTimer > 0.0f) {
            player.velocity.y = JUMP_VELOCITY;
            player.onGround = false;
            player.coyoteTimer = 0.0f;
            player.jumpBufferTimer = 0.0f;
            PlaySoundJump();
        }

        // Variable jump height: cut velocity when jump released early
        if (!jumpHeld && player.velocity.y < JUMP_VELOCITY * JUMP_CUT_MULT) {
            player.velocity.y = JUMP_VELOCITY * JUMP_CUT_MULT;
        }
    }

    float gravityScale = inWater ? WATER_GRAVITY_MULT : 1.0f;
    player.velocity.y += GRAVITY * gravityScale * dt;
    float maxFall = inWater ? WATER_MAX_FALL : 800.0f;
    if (player.velocity.y > maxFall) player.velocity.y = maxFall;

    // Horizontal collision with step-up
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
        if (bx < 0 || bx >= WORLD_WIDTH) { blocked = true; break; }
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    if (blocked) {
        // Step-up: try climbing a 1-block step when on ground
        if (player.onGround && player.velocity.x != 0.0f) {
            float stepY = player.position.y - (BLOCK_SIZE + 4);
            int stepMinBY = (int)(stepY) / BLOCK_SIZE;
            int stepMaxBY = (int)(stepY + PLAYER_HEIGHT - 0.01f) / BLOCK_SIZE;
            bool stepBlocked = false;
            for (int bx = minBX; bx <= maxBX; bx++) {
                if (bx < 0 || bx >= WORLD_WIDTH) { stepBlocked = true; break; }
                for (int by = stepMinBY; by <= stepMaxBY; by++) {
                    if (IsBlockSolid(bx, by)) {
                        stepBlocked = true;
                        break;
                    }
                }
                if (stepBlocked) break;
            }
            if (!stepBlocked) {
                player.position.y = stepY;
                player.onGround = false;
                player.coyoteTimer = COYOTE_TIME; // grace time after step
                // proceed with horizontal movement below
            } else {
                blocked = true; // keep wall snap
            }
        }

        if (blocked) {
            if (player.velocity.x > 0) {
                newX = (int)(right / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_WIDTH - 0.01f;
            } else if (player.velocity.x < 0) {
                newX = (int)(left / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
            }
            player.velocity.x = 0;
        }
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
        if (bx < 0 || bx >= WORLD_WIDTH) { blocked = true; break; }
        for (int by = minBY; by <= maxBY; by++) {
            if (IsBlockSolid(bx, by)) {
                blocked = true;
                break;
            }
        }
        if (blocked) break;
    }

    // Track peak fall velocity
    if (player.velocity.y > 0) {
        if (player.velocity.y > player.fallPeakVel) player.fallPeakVel = player.velocity.y;
    }

    bool wasOnGround = player.onGround;
    player.onGround = false;
    if (blocked) {
        if (player.velocity.y > 0) {
            newY = (int)(bottom / BLOCK_SIZE) * BLOCK_SIZE - PLAYER_HEIGHT;
            player.onGround = true;
            if (!wasOnGround && player.velocity.y > 200.0f) {
                if (!player.netControlled) {
                    PlaySoundLand();
                    SpawnLandingDust(player.position.x + PLAYER_WIDTH / 2.0f,
                                     player.position.y + PLAYER_HEIGHT,
                                     player.velocity.y / 400.0f);
                }
            }
            // Fall damage
            if (player.fallPeakVel > 300.0f) {
                int damage = (int)((player.fallPeakVel - 300.0f) / 100.0f);
                if (damage > 0) {
                    float reduction = GetArmorDamageReduction();
                    damage = (int)(damage * (1.0f - reduction));
                    if (damage < 1) damage = 1;
                    player.health -= damage;
                    if (player.health < 0) player.health = 0;
                    DamageArmor();
                    pendingDeathCause = STR_DEATH_FALL;
                    player.damageFlashTimer = 0.3f;
                    if (!player.netControlled) {
                        TriggerCameraShake(3.0f, 0.2f);
                        SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                             player.position.y + PLAYER_HEIGHT,
                                             (Color){200, 50, 50, 255});
                    }
                    PlaySoundHurt();
                    ShowMessage(S(STR_MSG_FALL_DAMAGE), (Color){240, 100, 100, 255});
                }
            }
            player.fallPeakVel = 0.0f;
        } else if (player.velocity.y < 0) {
            newY = (int)(top / BLOCK_SIZE) * BLOCK_SIZE + BLOCK_SIZE;
        }
        player.velocity.y = 0;
    }
    // Reset peak vel when not falling (e.g., in water, on ground)
    if (player.onGround || player.velocity.y <= 0) player.fallPeakVel = 0.0f;
    player.position.y = newY;

    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > (WORLD_WIDTH - 1) * BLOCK_SIZE)
        player.position.x = (WORLD_WIDTH - 1) * BLOCK_SIZE;

    // Death from falling out of world
    if (player.position.y > DEATH_Y) {
        player.health = 0;
        pendingDeathCause = STR_DEATH_VOID;
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
    if (player.netControlled) return; // Skip block interaction for remote players

    Vector2 mouseWorld = GetScreenToWorld2D(Win32GetMousePosition(), camera);
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
    if (win32LMB) {
        // Decrement attack cooldown every frame
        if (player.attackCooldown > 0) player.attackCooldown -= GetFrameTime();
        // Check mob hit first
        for (int i = 0; i < MAX_MOBS; i++) {
            if (!mobs[i].active || mobs[i].deathTimer > 0) continue;
            if (IsPlayerNearMob(&mobs[i], BREAK_RANGE * BLOCK_SIZE)) {
                int mw = GetMobWidth(mobs[i].type);
                int mh = GetMobHeight(mobs[i].type);
                float mLeft = mobs[i].position.x;
                float mRight = mLeft + mw;
                float mTop = mobs[i].position.y;
                float mBottom = mTop + mh;

                if (mouseWorld.x >= mLeft && mouseWorld.x <= mRight &&
                    mouseWorld.y >= mTop && mouseWorld.y <= mBottom) {
                    // Attack mob
                    int damage = IsTool(selectedTool) ? (IsSword(selectedTool) ? GetSwordDamage(selectedTool) : 2) : 1;
                    // Sharpness enchantment bonus
                    uint16_t toolEnch = player.itemEnchantments[player.selectedSlot];
                    if (ENCH_TYPE(toolEnch) == ENCH_SHARPNESS) {
                        damage += (int)(ENCH_LEVEL(toolEnch) * 1.5f);
                    }
                    if (player.attackCooldown <= 0) {
                        // Critical hit: falling fast enough
                        bool crit = player.velocity.y > CRIT_FALL_THRESHOLD;
                        if (crit) {
                            damage = (int)(damage * CRIT_DAMAGE_MULT);
                            ShowMessage(S(STR_MSG_CRIT_HIT), (Color){255, 215, 0, 255});
                            TriggerCameraShake(4.0f, 0.2f);
                            SpawnDamageParticles(mobs[i].position.x + mw / 2.0f,
                                                 mobs[i].position.y + mh / 2.0f,
                                                 (Color){255, 215, 0, 255});
                        }
                        DamageMob(&mobs[i], damage);
                        player.attackCooldown = GetAttackSpeed(selectedTool);
                        // Consume durability (Unbreaking check)
                        if (IsTool(selectedTool)) {
                            int slot = player.selectedSlot;
                            uint16_t ench = player.itemEnchantments[slot];
                            bool skipDur = false;
                            if (ENCH_TYPE(ench) == ENCH_UNBREAKING) {
                                skipDur = (rand() % (ENCH_LEVEL(ench) + 1)) != 0;
                            }
                            if (!skipDur) player.toolDurability[slot]--;
                            if (player.toolDurability[slot] <= 0) {
                                player.inventory[slot] = BLOCK_AIR;
                                player.inventoryCount[slot] = 0;
                                player.toolDurability[slot] = 0;
                                player.itemEnchantments[slot] = 0;
                                ShowMessage(S(STR_MSG_TOOL_BROKE), (Color){240, 80, 80, 255});
                            } else {
                                int maxDur = GetToolMaxDurability(selectedTool);
                                if (maxDur > 0 && player.toolDurability[slot] == maxDur / 5) {
                                    ShowMessage(S(STR_MSG_TOOL_WEARING), (Color){240, 200, 80, 255});
                                }
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
            static float lastParticleThreshold = 0.0f;
            if (blockX != miningBlockX || blockY != miningBlockY) {
                miningBlockX = blockX;
                miningBlockY = blockY;
                miningProgress = 0.0f;
                lastParticleThreshold = 0.0f;
            }
            float dt = GetFrameTime();
            float baseMineTime = 0.4f; // seconds for bare hands
            miningProgress += (dt * toolSpeed) / baseMineTime;

            if (miningProgress - lastParticleThreshold >= 0.15f) {
                lastParticleThreshold += 0.15f;
                SpawnMiningParticles(blockX, blockY, bt);
            }

            if (miningProgress >= 1.0f) {
                // Return furnace items before destroying
                if (bt == BLOCK_FURNACE) {
                    ReturnFurnaceItems();
                }
                // Drop chest items before destroying
                if (bt == BLOCK_CHEST) {
                    for (int ci = 0; ci < chestCount; ci++) {
                        if (chestData[ci].x == blockX && chestData[ci].y == blockY) {
                            for (int si = 0; si < CHEST_SLOTS; si++) {
                                if (chestData[ci].items[si] != BLOCK_AIR && chestData[ci].counts[si] > 0) {
                                    SpawnItemEntity(chestData[ci].items[si], chestData[ci].counts[si],
                                        blockX * BLOCK_SIZE + 3, blockY * BLOCK_SIZE + 3);
                                }
                            }
                            chestData[ci] = chestData[--chestCount];
                            break;
                        }
                    }
                }
                world[blockX][blockY] = BLOCK_AIR;
                NetSyncBlockChange(blockX, blockY, BLOCK_AIR);
                if (bt == BLOCK_STONE_PRESSURE_PLATE) UnregisterPressurePlate(blockX, blockY);
                SpawnBlockParticles(blockX, blockY, bt);
                // Ore drop special cases (only if tool tier is sufficient)
                if (CanToolMineBlock(selectedTool, bt) || bt == BLOCK_CROPS || bt == BLOCK_FARMLAND) {
                    // Check for Silk Touch on the mining tool
                    uint16_t silkEnch = player.itemEnchantments[player.selectedSlot];
                    bool hasSilkTouch = (ENCH_TYPE(silkEnch) == ENCH_SILK_TOUCH);
                    uint8_t dropItem = bt;
                    bool useSilkTouch = false;
                    if (hasSilkTouch) {
                        // Silk Touch: drop block itself for specific block types
                        if (bt == BLOCK_GLASS || bt == BLOCK_SAND || bt == BLOCK_GRAVEL ||
                            bt == BLOCK_COAL_ORE || bt == BLOCK_IRON_ORE || bt == BLOCK_GOLD_ORE ||
                            bt == BLOCK_DIAMOND_ORE || bt == BLOCK_REDSTONE_ORE || bt == BLOCK_LAPIS_ORE ||
                            bt == BLOCK_OBSIDIAN || bt == BLOCK_LEAVES || bt == BLOCK_ICE ||
                            bt == BLOCK_PACKED_ICE || bt == BLOCK_SANDSTONE || bt == BLOCK_SNOW ||
                            bt == BLOCK_BOOKSHELF || bt == BLOCK_BONE_BLOCK || bt == BLOCK_MOSSY_COBBLESTONE) {
                            useSilkTouch = true;
                            dropItem = bt;
                        }
                    }
                    if (!useSilkTouch) {
                        // Normal drop behavior
                        if (bt == BLOCK_STONE) dropItem = BLOCK_COBBLESTONE;
                        else if (bt == BLOCK_COAL_ORE) dropItem = ITEM_COAL;
                        else if (bt == BLOCK_DIAMOND_ORE) dropItem = ITEM_DIAMOND;
                        else if (bt == BLOCK_REDSTONE_ORE) dropItem = ITEM_REDSTONE;
                        else if (bt == BLOCK_LAPIS_ORE) dropItem = ITEM_LAPIS;
                        else if (bt == BLOCK_CROPS) { dropItem = ITEM_WHEAT; }
                        else if (bt == BLOCK_FARMLAND) { dropItem = BLOCK_DIRT; }
                    }
                    int dropCount = 1;
                    // Fortune enchantment: extra drops for ores (not on Silk Touch blocks)
                    if (!useSilkTouch) {
                        uint16_t ench = player.itemEnchantments[player.selectedSlot];
                        if (ENCH_TYPE(ench) == ENCH_FORTUNE && dropItem != bt) {
                            if (rand() % 100 < ENCH_LEVEL(ench) * 15) dropCount++;
                        }
                    }
                    // Crop drops scale with growth stage
                    if (bt == BLOCK_CROPS) {
                        int growth = GetCropGrowth(blockX, blockY);
                        if (growth < 4) {
                            dropCount = 0; // Too young, no wheat
                        } else if (growth < 7) {
                            dropCount = 1;
                        } else {
                            dropCount = 1 + rand() % 2; // 1-2 wheat when mature
                        }
                    }
                    if (dropCount > 0) SpawnItemEntity(dropItem, dropCount, blockX * BLOCK_SIZE + 3, blockY * BLOCK_SIZE + 3);
                    // Crops drop seeds based on growth
                    if (bt == BLOCK_CROPS) {
                        int growth = GetCropGrowth(blockX, blockY);
                        int seedDrop = 1;
                        if (growth >= 7) seedDrop = 1 + rand() % 3; // 1-3 seeds when mature
                        SpawnItemEntity(ITEM_WHEAT_SEEDS, seedDrop, blockX * BLOCK_SIZE + 3, blockY * BLOCK_SIZE + 3);
                    }
                }
                PlaySoundBreak(bt);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                UpdateRedstoneAt(blockX, blockY);
                miningProgress = 0.0f;
                miningBlockX = -1;

                // Sand/gravel gravity: make blocks above fall
                ApplyGravityAt(blockX, blockY);

                // Leaves have a chance to drop apples
                if (bt == BLOCK_LEAVES && (hash2D(blockX, blockY, 12345) % 20) == 0) {
                    AddToInventory(FOOD_APPLE);
                }

                // Consume tool durability (Unbreaking check)
                if (IsTool(selectedTool)) {
                    int slot = player.selectedSlot;
                    uint16_t ench = player.itemEnchantments[slot];
                    bool skipDur = false;
                    if (ENCH_TYPE(ench) == ENCH_UNBREAKING) {
                        skipDur = (rand() % (ENCH_LEVEL(ench) + 1)) != 0;
                    }
                    if (!skipDur) player.toolDurability[slot]--;
                    if (player.toolDurability[slot] <= 0) {
                        // Tool breaks
                        player.inventory[slot] = BLOCK_AIR;
                        player.inventoryCount[slot] = 0;
                        player.toolDurability[slot] = 0;
                        player.itemEnchantments[slot] = 0;
                        ShowMessage(S(STR_MSG_TOOL_BROKE), (Color){240, 80, 80, 255});
                    } else {
                        int maxDur = GetToolMaxDurability(selectedTool);
                        if (maxDur > 0 && player.toolDurability[slot] == maxDur / 5) {
                            ShowMessage(S(STR_MSG_TOOL_WEARING), (Color){240, 200, 80, 255});
                        }
                    }
                }
            }
        }
    } else {
        miningProgress = 0.0f;
    }

    // Place block or eat food (right click, instant)
    if (Win32IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        // Interact with bed (set spawn point or sleep)
        if (blockX >= 0 && blockX < WORLD_WIDTH && blockY >= 0 && blockY < WORLD_HEIGHT) {
            if (world[blockX][blockY] == BLOCK_BED) {
                player.spawnX = blockX;
                player.spawnY = blockY - 1;
                if (TrySleep()) {
                    ShowMessage(S(STR_MSG_SLEEP), (Color){200, 150, 255, 255});
                } else {
                    ShowMessage(S(STR_MSG_SLEEP_ONLY_NIGHT), (Color){255, 200, 100, 255});
                    ShowMessage(S(STR_MSG_SPAWN_SET), (Color){100, 220, 100, 255});
                }
                PlaySoundCraft();
                return;
            }
            // Interact with crafting table
            if (world[blockX][blockY] == BLOCK_CRAFTING_TABLE) {
                inventoryOpen = true;
                craftingTableOpen = true;
                gamePaused = false;
                PlaySoundCraft();
                return;
            }
            // Interact with furnace
            if (world[blockX][blockY] == BLOCK_FURNACE) {
                RequestOpenFurnace(blockX, blockY);
                return;
            }
            // Interact with chest
            if (world[blockX][blockY] == BLOCK_CHEST) {
                RequestOpenChest(blockX, blockY);
                return;
            }
            // Toggle lever
            if (world[blockX][blockY] == BLOCK_LEVER) {
                ToggleLever(blockX, blockY);
                PlaySoundUIClick();
                return;
            }
            // Ignite TNT (right-click lights the fuse)
            if (world[blockX][blockY] == BLOCK_TNT) {
                PrimeTnt(blockX, blockY);
                ShowMessage(S(STR_BLOCK_TNT), (Color){255, 120, 80, 255});
                return;
            }
            // Enchanting table interaction
            if (world[blockX][blockY] == BLOCK_ENCHANTING_TABLE) {
                if (IsTool(selectedTool) || IsArmor(selectedTool)) {
                    int slot = player.selectedSlot;
                    // Check if already enchanted
                    if (ENCH_TYPE(player.itemEnchantments[slot]) != ENCH_NONE) {
                        ShowMessage(S(STR_MSG_ALREADY_ENCHANTED), (Color){240, 200, 80, 255});
                        return;
                    }
                    // Count nearby bookshelves for power
                    int bookshelfCount = 0;
                    for (int dx = -2; dx <= 2; dx++) {
                        for (int dy = -2; dy <= 2; dy++) {
                            if (dx == 0 && dy == 0) continue;
                            int nx = blockX + dx, ny = blockY + dy;
                            if (nx >= 0 && nx < WORLD_WIDTH && ny >= 0 && ny < WORLD_HEIGHT) {
                                if (world[nx][ny] == BLOCK_BOOKSHELF) bookshelfCount++;
                            }
                        }
                    }
                    // Determine enchantment level (1-5) based on bookshelves
                    int maxLevel = 1 + bookshelfCount / 5;
                    if (maxLevel > 5) maxLevel = 5;
                    if (maxLevel < 1) maxLevel = 1;

                    // Generate 3 random enchantment options
                    enchantOptionCount = 0;
                    for (int opt = 0; opt < MAX_ENCHANT_OPTIONS && enchantOptionCount < MAX_ENCHANT_OPTIONS; opt++) {
                        EnchantOption *eo = &enchantOptions[opt];
                        eo->type = ENCH_NONE;

                        bool hasSilkTouch = (ENCH_TYPE(player.itemEnchantments[slot]) == ENCH_SILK_TOUCH);
                        bool hasFortune = (ENCH_TYPE(player.itemEnchantments[slot]) == ENCH_FORTUNE);

                        // Choose enchantment type based on item
                        if (IsArmor(selectedTool)) {
                            int pool[] = { ENCH_PROTECTION, ENCH_PROTECTION, ENCH_UNBREAKING };
                            eo->type = (EnchantmentType)pool[rand() % 3];
                        } else if (IsSword(selectedTool)) {
                            // Silk Touch excluded on swords (it's for mining)
                            int pool[] = { ENCH_SHARPNESS, ENCH_SHARPNESS, ENCH_UNBREAKING };
                            eo->type = (EnchantmentType)pool[rand() % 3];
                        } else if (IsPickaxe(selectedTool)) {
                            if (hasSilkTouch) {
                                int pool[] = { ENCH_EFFICIENCY, ENCH_EFFICIENCY, ENCH_UNBREAKING };
                                eo->type = (EnchantmentType)pool[rand() % 3];
                            } else if (hasFortune) {
                                int pool[] = { ENCH_EFFICIENCY, ENCH_EFFICIENCY, ENCH_UNBREAKING };
                                eo->type = (EnchantmentType)pool[rand() % 3];
                            } else {
                                int pool[] = { ENCH_EFFICIENCY, ENCH_FORTUNE, ENCH_UNBREAKING, ENCH_EFFICIENCY };
                                eo->type = (EnchantmentType)pool[rand() % 4];
                            }
                        } else if (selectedTool == ITEM_BOW) {
                            int pool[] = { ENCH_POWER, ENCH_POWER, ENCH_UNBREAKING };
                            eo->type = (EnchantmentType)pool[rand() % 3];
                        } else {
                            // Axe, shovel, hoe
                            int pool[] = { ENCH_EFFICIENCY, ENCH_UNBREAKING, ENCH_EFFICIENCY };
                            eo->type = (EnchantmentType)pool[rand() % 3];
                        }

                        if (eo->type == ENCH_NONE) continue;

                        // Level: 1 to min(3, maxLevel)
                        int lvl = 1 + rand() % maxLevel;
                        if (lvl > 3) lvl = 3;
                        eo->level = lvl;
                        eo->xpCost = (lvl * 2 + 3) * 10;
                        enchantOptionCount++;
                    }

                    enchantHeldItem = selectedTool;
                    enchantHeldItemSlot = slot;
                    enchantTableBlockX = blockX;
                    enchantTableBlockY = blockY;
                    enchantOpen = true;
                    inventoryOpen = true;
                    gamePaused = true;
                    PlaySoundCraft();
                } else {
                    inventoryOpen = true;
                    craftingTableOpen = true;
                    gamePaused = false;
                    PlaySoundCraft();
                }
                return;
            }
        }

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
            ShowMessage(Sf(STR_MSG_ATE, GetBlockName(selectedTool), foodVal), (Color){80, 220, 80, 255});
            return;
        }

        // Use ender pearl
        if (selectedTool == ITEM_ENDER_PEARL) {
            float px = player.position.x + PLAYER_WIDTH / 2.0f;
            float py = player.position.y + PLAYER_HEIGHT / 2.0f;
            float dx = mouseWorld.x - px;
            float dy = mouseWorld.y - py;
            float tpDist = sqrtf(dx * dx + dy * dy);
            if (tpDist > 1.0f) {
                // Limit teleport distance
                float maxDist = 400.0f;
                if (tpDist > maxDist) { dx = dx / tpDist * maxDist; dy = dy / tpDist * maxDist; }
                // Check if target position is safe (not inside solid blocks)
                float targetX = player.position.x + dx - PLAYER_WIDTH / 2.0f;
                float targetY = player.position.y + dy - PLAYER_HEIGHT / 2.0f;
                // Check AABB collision at target with bounds clamp
                int tlx = (int)(targetX) / BLOCK_SIZE;
                int trx = (int)(targetX + PLAYER_WIDTH - 0.01f) / BLOCK_SIZE;
                int tty = (int)(targetY) / BLOCK_SIZE;
                int tby = (int)(targetY + PLAYER_HEIGHT - 0.01f) / BLOCK_SIZE;
                if (tlx < 0) tlx = 0;
                if (trx >= WORLD_WIDTH) trx = WORLD_WIDTH - 1;
                if (tty < 0) tty = 0;
                if (tby >= WORLD_HEIGHT) tby = WORLD_HEIGHT - 1;
                bool safe = true;
                for (int bx = tlx; bx <= trx && safe; bx++) {
                    for (int by = tty; by <= tby && safe; by++) {
                        if (IsBlockSolid(bx, by)) safe = false;
                    }
                }
                if (!safe) {
                    // Try to find nearest safe spot by scanning outward
                    Vector2 best = { targetX, targetY };
                    float bestDist = 1e9f;
                    for (int ox = -2; ox <= 2; ox++) {
                        for (int oy = -2; oy <= 2; oy++) {
                            int nx = (int)(targetX) / BLOCK_SIZE + ox;
                            int ny = (int)(targetY) / BLOCK_SIZE + oy;
                            if (nx < 0 || nx >= WORLD_WIDTH - 1 || ny < 0 || ny >= WORLD_HEIGHT - 2) continue;
                            // Check if this block column has space for the player
                            float sx = nx * BLOCK_SIZE;
                            float sy = ny * BLOCK_SIZE;
                            int sminBX = (int)(sx) / BLOCK_SIZE;
                            int smaxBX = (int)(sx + PLAYER_WIDTH - 0.01f) / BLOCK_SIZE;
                            int sminBY = (int)(sy) / BLOCK_SIZE;
                            int smaxBY = (int)(sy + PLAYER_HEIGHT - 0.01f) / BLOCK_SIZE;
                            bool free = true;
                            for (int bx = sminBX; bx <= smaxBX && free; bx++) {
                                if (bx < 0 || bx >= WORLD_WIDTH) { free = false; break; }
                                for (int by = sminBY; by <= smaxBY && free; by++) {
                                    if (IsBlockSolid(bx, by)) free = false;
                                }
                            }
                            if (free) {
                                float dist2 = (sx - targetX) * (sx - targetX) + (sy - targetY) * (sy - targetY);
                                if (dist2 < bestDist) { bestDist = dist2; best.x = sx; best.y = sy; }
                            }
                        }
                    }
                    if (bestDist < 1e8f) {
                        targetX = best.x;
                        targetY = best.y;
                        safe = true;
                    }
                }
                if (safe) {
                    // Teleport
                    player.position.x = targetX;
                    player.position.y = targetY;
                    player.velocity.x = 0;
                    player.velocity.y = 0;
                    // Consume pearl
                    player.inventoryCount[player.selectedSlot]--;
                    if (player.inventoryCount[player.selectedSlot] <= 0) {
                        player.inventory[player.selectedSlot] = BLOCK_AIR;
                    }
                    // Small fall damage on landing
                    player.health -= 2;
                    if (player.health < 0) player.health = 0;
                    player.damageFlashTimer = 0.3f;
                    PlaySoundHurt();
                    ShowMessage(S(STR_MSG_ENDER_PEARL), (Color){100, 200, 255, 255});
                }
            }
            return;
        }

        // Fire bow
        if (selectedTool == ITEM_BOW) {
            // Find arrows in inventory
            int arrowSlot = -1;
            for (int i = 0; i < INVENTORY_SLOTS; i++) {
                if (player.inventory[i] == ITEM_ARROW && player.inventoryCount[i] > 0) {
                    arrowSlot = i;
                    break;
                }
            }
            if (arrowSlot >= 0) {
                // Fire toward mouse
                float px = player.position.x + PLAYER_WIDTH / 2.0f;
                float py = player.position.y + PLAYER_HEIGHT / 2.0f;
                float dx = mouseWorld.x - px;
                float dy = mouseWorld.y - py;
                float aimDist = sqrtf(dx * dx + dy * dy);
                if (aimDist > 1.0f) {
                    float speed = PROJECTILE_SPEED * 1.5f;
                    int arrowIdx = SpawnProjectile(px, py, (dx / aimDist) * speed, (dy / aimDist) * speed, true);
                    // Power enchantment: +2 arrow damage per level
                    if (arrowIdx >= 0) {
                        uint16_t be = player.itemEnchantments[player.selectedSlot];
                        if (ENCH_TYPE(be) == ENCH_POWER)
                            projectiles[arrowIdx].damage = PROJECTILE_DAMAGE + ENCH_LEVEL(be) * 2;
                    }
                    // Sync projectile to network
                    if (NetIsClient()) {
                        uint8_t buf[64];
                        PktProjectileSpawn ps;
                        ps.x = px; ps.y = py;
                        ps.vx = (dx / aimDist) * speed; ps.vy = (dy / aimDist) * speed;
                        ps.fromPlayer = true; ps.playerId = (uint8_t)localPlayerId;
                        buf[0] = PKT_PROJECTILE_SPAWN;
                        memcpy(buf + 1, &ps, sizeof(PktProjectileSpawn));
                        NetSendToServer(buf, 1 + sizeof(PktProjectileSpawn), false);
                    }
                    PlaySoundBowFire();
                    // Consume arrow
                    player.inventoryCount[arrowSlot]--;
                    if (player.inventoryCount[arrowSlot] <= 0)
                        player.inventory[arrowSlot] = BLOCK_AIR;
                    // Bow durability (Unbreaking check)
                    {
                        uint16_t bowEnch = player.itemEnchantments[player.selectedSlot];
                        bool skipBowDur = false;
                        if (ENCH_TYPE(bowEnch) == ENCH_UNBREAKING) {
                            skipBowDur = (rand() % (ENCH_LEVEL(bowEnch) + 1)) != 0;
                        }
                        if (!skipBowDur) player.toolDurability[player.selectedSlot]--;
                        if (player.toolDurability[player.selectedSlot] <= 0) {
                            player.inventory[player.selectedSlot] = BLOCK_AIR;
                            player.inventoryCount[player.selectedSlot] = 0;
                            player.toolDurability[player.selectedSlot] = 0;
                            player.itemEnchantments[player.selectedSlot] = 0;
                            ShowMessage(S(STR_MSG_TOOL_BROKE), (Color){240, 80, 80, 255});
                        }
                    }
                }
                return;
            } else {
                ShowMessage(S(STR_MSG_NO_ARROWS), (Color){240, 80, 80, 255});
                return;
            }
        }

        // Fishing rod
        if (selectedTool == ITEM_FISHING_ROD) {
            // Check for existing fishing line to retract
            for (int i = 0; i < MAX_PROJECTILES; i++) {
                if (projectiles[i].active && projectiles[i].isFishing) {
                    if (projectiles[i].hasBite) {
                        // Reel in with catch
                        int roll = rand() % 100;
                        int catchItem;
                        if (roll < 60) {
                            catchItem = ITEM_RAW_FISH;
                        } else if (roll < 85) {
                            int junk = rand() % 3;
                            catchItem = (junk == 0) ? ITEM_STICK : (junk == 1) ? ITEM_BONE : ITEM_RAW_CHICKEN;
                        } else {
                            int treasure = rand() % 4;
                            catchItem = (treasure == 0) ? ITEM_ENDER_PEARL : (treasure == 1) ? ITEM_IRON_INGOT :
                                        (treasure == 2) ? ITEM_BOW : ITEM_STRING;
                        }
                        AddToInventory((BlockType)catchItem);
                        ShowMessage(S(STR_FISH_CATCH), (Color){100, 200, 255, 255});
                        PlaySoundPickup();
                        UnlockAchievement(ACH_ANGLER);
                        projectiles[i].active = false;
                    } else {
                        // Just retract with nothing
                        projectiles[i].active = false;
                        ShowMessage(S(STR_FISH_RETRACT), (Color){150, 150, 150, 255});
                    }
                    return;
                }
            }
            // Cast new fishing line
            float px = player.position.x + PLAYER_WIDTH / 2.0f;
            float py = player.position.y + PLAYER_HEIGHT / 2.0f;
            float dx = mouseWorld.x - px;
            float dy = mouseWorld.y - py;
            float castDist = sqrtf(dx * dx + dy * dy);
            if (castDist > 1.0f) {
                float speed = PROJECTILE_SPEED * 1.2f;
                float maxDist = FISHING_ROD_MAX_RANGE;
                if (castDist > maxDist) { dx = dx / castDist * maxDist; dy = dy / castDist * maxDist; castDist = maxDist; }
                int projIdx = SpawnProjectile(px, py, (dx / castDist) * speed, (dy / castDist) * speed, true);
                // Mark the newly spawned projectile as fishing line
                if (projIdx >= 0 && projIdx < MAX_PROJECTILES) {
                    projectiles[projIdx].isFishing = true;
                    projectiles[projIdx].fishTimer = FISHING_MIN_DELAY + (float)(rand() % (int)(FISHING_MAX_DELAY - FISHING_MIN_DELAY));
                    projectiles[projIdx].hasBite = false;
                    projectiles[projIdx].lifetime = 120.0f; // 2 minute total lifetime
                }
                ShowMessage(S(STR_FISH_CAST), (Color){150, 200, 255, 255});
                PlaySoundSplash();
            }
            return;
        }

        // Cauldron interaction
        if (world[blockX][blockY] == BLOCK_CAULDRON) {
            int cdIdx = -1;
            for (int ci = 0; ci < cauldronCount; ci++) {
                if (cauldrons[ci].x == blockX && cauldrons[ci].y == blockY) {
                    cdIdx = ci;
                    break;
                }
            }
            if (cdIdx < 0 && cauldronCount < MAX_CAULDRONS) {
                cdIdx = cauldronCount++;
                cauldrons[cdIdx].x = blockX;
                cauldrons[cdIdx].y = blockY;
                cauldrons[cdIdx].fillLevel = 0;
            }
            if (cdIdx >= 0) {
                // Right-click with water bucket -> fill
                if (selectedTool == ITEM_WATER_BUCKET && cauldrons[cdIdx].fillLevel < 3) {
                    cauldrons[cdIdx].fillLevel = 3;
                    player.inventory[player.selectedSlot] = ITEM_BUCKET;
                    PlaySoundSplash();
                    ShowMessage(S(STR_MSG_CAULDRON_FILLED), (Color){100, 150, 255, 255});
                    return;
                }
                // Right-click with empty bucket -> empty
                if (selectedTool == ITEM_BUCKET && cauldrons[cdIdx].fillLevel > 0) {
                    cauldrons[cdIdx].fillLevel = 0;
                    player.inventory[player.selectedSlot] = ITEM_WATER_BUCKET;
                    PlaySoundSplash();
                    ShowMessage(S(STR_MSG_CAULDRON_EMPTY), (Color){100, 150, 255, 255});
                    return;
                }
                // Right-click with nothing -> drink water (restore oxygen)
                if (cauldrons[cdIdx].fillLevel >= 2) {
                    player.oxygen = MAX_OXYGEN;
                    cauldrons[cdIdx].fillLevel = 1;
                    PlaySoundDrink();
                    ShowMessage(S(STR_MSG_CAULDRON_DRINK), (Color){100, 200, 255, 255});
                    return;
                }
            }
            return;
        }

        // Water bucket: place water source
        if (selectedTool == ITEM_WATER_BUCKET) {
            if (world[blockX][blockY] == BLOCK_AIR || world[blockX][blockY] == BLOCK_WATER) {
                SetWaterSource(blockX, blockY);
                player.inventory[player.selectedSlot] = ITEM_BUCKET; // Empty bucket
                PlaySoundPlace(BLOCK_WATER);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                return;
            }
        }
        // Empty bucket: collect water or lava
        if (selectedTool == ITEM_BUCKET) {
            if (world[blockX][blockY] == BLOCK_WATER) {
                RemoveWaterAt(blockX, blockY);
                player.inventory[player.selectedSlot] = ITEM_WATER_BUCKET;
                PlaySoundPlace(BLOCK_WATER);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                return;
            }
            if (world[blockX][blockY] == BLOCK_LAVA) {
                RemoveLavaAt(blockX, blockY);
                player.inventory[player.selectedSlot] = ITEM_LAVA_BUCKET;
                PlaySoundPlace(BLOCK_LAVA);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                return;
            }
        }
        // Lava bucket: place lava source
        if (selectedTool == ITEM_LAVA_BUCKET) {
            if (world[blockX][blockY] == BLOCK_AIR || world[blockX][blockY] == BLOCK_WATER) {
                if (world[blockX][blockY] == BLOCK_WATER) RemoveWaterAt(blockX, blockY);
                SetLavaSource(blockX, blockY);
                player.inventory[player.selectedSlot] = ITEM_BUCKET;
                PlaySoundPlace(BLOCK_LAVA);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                return;
            }
        }

        // Breeding: feed passive mobs with food
        static float breedCooldownTimer = 0.0f;
        breedCooldownTimer -= GetFrameTime();
        if (breedCooldownTimer <= 0 && IsFood(selectedTool)) {
            for (int i = 0; i < MAX_MOBS; i++) {
                if (!mobs[i].active || mobs[i].isBaby) continue;
                if (mobs[i].type != MOB_PIG && mobs[i].type != MOB_COW &&
                    mobs[i].type != MOB_SHEEP && mobs[i].type != MOB_CHICKEN) continue;
                if (mobs[i].loveTimer > 0) continue;
                int mw = GetMobWidth(mobs[i].type);
                int mh = GetMobHeight(mobs[i].type);
                float mx = mobs[i].position.x, my = mobs[i].position.y;
                if (mouseWorld.x >= mx && mouseWorld.x <= mx + mw &&
                    mouseWorld.y >= my && mouseWorld.y <= my + mh) {
                    float dx = (player.position.x + PLAYER_WIDTH / 2) - (mx + mw / 2);
                    float dy = (player.position.y + PLAYER_HEIGHT / 2) - (my + mh / 2);
                    if (dx * dx + dy * dy < (BREAK_RANGE * BLOCK_SIZE) * (BREAK_RANGE * BLOCK_SIZE)) {
                        // Feed the mob
                        mobs[i].loveTimer = 15.0f; // 15 seconds of love mode
                        player.inventoryCount[player.selectedSlot]--;
                        if (player.inventoryCount[player.selectedSlot] <= 0) {
                            player.inventory[player.selectedSlot] = BLOCK_AIR;
                        }
                        PlaySoundEat();
                        // Check for nearby mob of same type in love mode
                        for (int j = 0; j < MAX_MOBS; j++) {
                            if (j == i || !mobs[j].active) continue;
                            if (mobs[j].type != mobs[i].type || mobs[j].loveTimer <= 0) continue;
                            float bdx = mobs[j].position.x - mobs[i].position.x;
                            float bdy = mobs[j].position.y - mobs[i].position.y;
                            if (bdx * bdx + bdy * bdy < 100 * 100) {
                                // Spawn baby
                                float babyX = (mobs[i].position.x + mobs[j].position.x) / 2;
                                float babyY = (mobs[i].position.y + mobs[j].position.y) / 2;
                                Mob *baby = SpawnMob(mobs[i].type, babyX, babyY);
                                if (baby) {
                                    baby->isBaby = true;
                                    baby->growTimer = 120.0f; // 2 minutes to grow
                                    baby->health = baby->maxHealth / 2;
                                    baby->maxHealth = baby->maxHealth / 2;
                                    breedCooldownTimer = 5.0f; // 5 second global cooldown
                                    UnlockAchievement(ACH_BREEDER);
                                }
                                mobs[i].loveTimer = 0;
                                mobs[j].loveTimer = 0;
                                break;
                            }
                        }
                        return;
                    }
                }
            }
        }

        // Villager interaction: open trade UI
        for (int i = 0; i < MAX_MOBS; i++) {
            if (!mobs[i].active || mobs[i].type != MOB_VILLAGER) continue;
            int mw = GetMobWidth(MOB_VILLAGER);
            int mh = GetMobHeight(MOB_VILLAGER);
            float mx = mobs[i].position.x, my = mobs[i].position.y;
            if (mouseWorld.x >= mx && mouseWorld.x <= mx + mw &&
                mouseWorld.y >= my && mouseWorld.y <= my + mh) {
                float dx = (player.position.x + PLAYER_WIDTH / 2) - (mx + mw / 2);
                float dy = (player.position.y + PLAYER_HEIGHT / 2) - (my + mh / 2);
                if (dx * dx + dy * dy < (BREAK_RANGE * BLOCK_SIZE) * (BREAK_RANGE * BLOCK_SIZE)) {
                    tradeOpen = true;
                    inventoryOpen = true;
                    gamePaused = false;
                    PlaySoundCraft();
                    return;
                }
            }
        }

        // Hoe: till dirt into farmland
        if (IsHoe(selectedTool)) {
            if (world[blockX][blockY] == BLOCK_DIRT || world[blockX][blockY] == BLOCK_GRASS) {
                world[blockX][blockY] = BLOCK_FARMLAND;
                NetSyncBlockChange(blockX, blockY, BLOCK_FARMLAND);
                PlaySoundPlace(BLOCK_DIRT);
                UpdateLightAt(blockX, blockY);
                InvalidateChunkAt(blockX, blockY);
                // Consume hoe durability
                int slot = player.selectedSlot;
                player.toolDurability[slot]--;
                if (player.toolDurability[slot] <= 0) {
                    player.inventory[slot] = BLOCK_AIR;
                    player.inventoryCount[slot] = 0;
                    player.toolDurability[slot] = 0;
                    player.itemEnchantments[slot] = 0;
                    ShowMessage(S(STR_MSG_TOOL_BROKE), (Color){240, 80, 80, 255});
                }
                return;
            }
        }

        // Seeds: plant on farmland
        if (selectedTool == ITEM_WHEAT_SEEDS) {
            if (world[blockX][blockY] == BLOCK_FARMLAND && world[blockX][blockY - 1] == BLOCK_AIR) {
                world[blockX][blockY - 1] = BLOCK_CROPS;
                SetCropGrowth(blockX, blockY - 1, 0); // fresh plant starts at stage 0
                RegisterCrop(blockX, blockY - 1);     // track for growth (see UpdateCrops)
                NetSyncBlockChange(blockX, blockY - 1, BLOCK_CROPS);
                PlaySoundPlace(BLOCK_TALL_GRASS);
                UpdateLightAt(blockX, blockY - 1);
                InvalidateChunkAt(blockX, blockY - 1);
                player.inventoryCount[player.selectedSlot]--;
                if (player.inventoryCount[player.selectedSlot] <= 0) {
                    player.inventory[player.selectedSlot] = BLOCK_AIR;
                }
                return;
            }
        }

        if (IsTool(selectedTool) || IsFood(selectedTool) || IsArmor(selectedTool)) return; // Can't place tools, food, or armor
        if (selectedTool >= BLOCK_COUNT || !blockInfo[selectedTool].breakable) return; // Can't place non-block items
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
                    bool wasWater = (world[blockX][blockY] == BLOCK_WATER);
                    if (wasWater) RemoveWaterAt(blockX, blockY);
                    world[blockX][blockY] = selectedTool;
                    NetSyncBlockChange(blockX, blockY, selectedTool);
                    if (selectedTool == BLOCK_STONE_PRESSURE_PLATE) RegisterPressurePlate(blockX, blockY);
                    player.inventoryCount[player.selectedSlot]--;
                    if (player.inventoryCount[player.selectedSlot] <= 0) {
                        player.inventory[player.selectedSlot] = BLOCK_AIR;
                    }
                    PlaySoundPlace(selectedTool);
                    totalBlocksPlaced++;
                    if (wasWater) {
                        SpawnDamageParticles(blockX * BLOCK_SIZE + BLOCK_SIZE / 2,
                                             blockY * BLOCK_SIZE + BLOCK_SIZE / 2,
                                             (Color){100, 160, 220, 200});
                        PlaySoundSplash();
                    }
                    UpdateLightAt(blockX, blockY);
                    InvalidateChunkAt(blockX, blockY);
                    if (blockX % CHUNK_SIZE == 0) InvalidateChunkAt(blockX - 1, blockY);
                    if (blockX % CHUNK_SIZE == CHUNK_SIZE - 1) InvalidateChunkAt(blockX + 1, blockY);
                    UpdateRedstoneAt(blockX, blockY);
                    // Gravity: sand/gravel falls when placed
                    if (IsGravityBlock(selectedTool)) {
                        world[blockX][blockY] = BLOCK_AIR;
                        NetSyncBlockChange(blockX, blockY, BLOCK_AIR);
                        int landY = blockY;
                        // Search downward for landing spot
                        while (landY < WORLD_HEIGHT - 1 && (world[blockX][landY + 1] == BLOCK_AIR || world[blockX][landY + 1] == BLOCK_WATER)) landY++;
                        world[blockX][landY] = selectedTool;
                        NetSyncBlockChange(blockX, landY, selectedTool);
                        InvalidateChunkAt(blockX, blockY);
                        InvalidateChunkAt(blockX, landY);
                        UpdateLightAt(blockX, landY);
                        ApplyGravityAt(blockX, landY);
                    }
                }
            }
        }
    }
}

float GetMiningProgress(void)
{
    return miningProgress;
}

int GetMiningBlockX(void)
{
    return miningBlockX;
}

int GetMiningBlockY(void)
{
    return miningBlockY;
}

//----------------------------------------------------------------------------------
// Player Update
//----------------------------------------------------------------------------------
void UpdatePlayer(float dt)
{
    if (player.playerDead) return;
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
    if (gamePaused || inventoryOpen || player.playerDead) return;

    bool underwater = IsPlayerUnderwater();

    // --- Underwater bubbles (local player only) ---
    if (underwater && !player.netControlled) {
        static float bubbleTimer = 0.0f;
        bubbleTimer += dt;
        if (bubbleTimer >= 0.15f) {
            bubbleTimer = 0.0f;
            SpawnBubble(player.position.x + PLAYER_WIDTH / 2, player.position.y + PLAYER_HEIGHT / 3);
        }
    }

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
                if (!player.netControlled) pendingDeathCause = STR_DEATH_DROWN;
                player.damageFlashTimer = 0.3f;
                if (!player.netControlled) PlaySoundHurt();
            }
        }
    } else {
        // Recover oxygen when out of water
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

    // --- Lava damage ---
    {
        int pbx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
        int pby = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
        if (pbx >= 0 && pbx < WORLD_WIDTH && pby >= 0 && pby < WORLD_HEIGHT && world[pbx][pby] == BLOCK_LAVA) {
            player.health -= 4.0f * dt; // 4 hearts/sec in lava
            if (player.health < 0) player.health = 0;
            if (!player.netControlled) pendingDeathCause = STR_DEATH_LAVA;
            player.damageFlashTimer = 0.3f;
            if (!player.netControlled && player.health > 0) PlaySoundHurt();
            // Fire particles
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT / 2,
                                 (Color){255, 150, 30, 255});
        }
    }

    // --- Cactus damage ---
    {
        static float cactusTimer = 0.0f;
        int pbx = (int)(player.position.x + PLAYER_WIDTH / 2) / BLOCK_SIZE;
        int pby = (int)(player.position.y + PLAYER_HEIGHT / 2) / BLOCK_SIZE;
        if (pbx >= 0 && pbx < WORLD_WIDTH && pby >= 0 && pby < WORLD_HEIGHT && world[pbx][pby] == BLOCK_CACTUS) {
            cactusTimer += dt;
            if (cactusTimer >= 0.5f) {
                cactusTimer = 0.0f;
                player.health -= 1.0f;
                if (player.health < 0) player.health = 0;
                if (!player.netControlled) pendingDeathCause = STR_DEATH_CACTUS;
                player.damageFlashTimer = 0.3f;
                if (!player.netControlled && player.health > 0) PlaySoundHurt();
                ShowMessage(S(STR_MSG_CACTUS_DAMAGE), (Color){80, 220, 80, 255});
            }
        } else {
            cactusTimer = 0.0f;
        }
    }

    // --- Hunger ---
    float hungerRate = HUNGER_DRAIN_RATE;
    if (gameDifficulty == DIFFICULTY_PEACEFUL) hungerRate *= 0.25f;
    else if (gameDifficulty == DIFFICULTY_EASY) hungerRate *= 0.5f;
    else if (gameDifficulty == DIFFICULTY_HARD) hungerRate *= 1.5f;
    if (player.sprinting) hungerRate *= HUNGER_SPRINT_MULT;
    player.hungerTimer += dt;
    if (player.hungerTimer >= 1.0f / hungerRate) {
        player.hungerTimer -= 1.0f / hungerRate;
        if (player.hunger > 0) {
            player.hunger--;
            // Warn when hunger gets low (local player only)
            if (!player.netControlled) {
                if (player.hunger == 6) {
                    ShowMessage(S(STR_MSG_HUNGRY), (Color){220, 180, 60, 255});
                } else if (player.hunger == 2) {
                    ShowMessage(S(STR_MSG_STARVING), (Color){240, 100, 60, 255});
                }
            }
        }
    }

    // Hunger damage at 0 hunger
    if (player.hunger <= 0) {
        player.hungerDamageTimer += dt;
        if (player.hungerDamageTimer >= 1.0f / HUNGER_DAMAGE_RATE) {
            player.hungerDamageTimer -= 1.0f / HUNGER_DAMAGE_RATE;
            player.health--;
            if (player.health < 0) player.health = 0;
            if (!player.netControlled) pendingDeathCause = STR_DEATH_STARVE;
            player.damageFlashTimer = 0.3f;
            if (!player.netControlled) PlaySoundHurt();
        }
    } else {
        player.hungerDamageTimer = 0.0f;
    }

    // --- Health Regen ---
    int regenHungerThreshold = HEALTH_REGEN_HUNGER;
    float regenRate = HEALTH_REGEN_RATE;
    if (gameDifficulty == DIFFICULTY_PEACEFUL) { regenHungerThreshold = 0; regenRate *= 3.0f; }
    else if (gameDifficulty == DIFFICULTY_EASY) { regenHungerThreshold = 14; }
    if (player.health < MAX_HEALTH && player.hunger >= regenHungerThreshold) {
        player.regenTimer += dt;
        if (player.regenTimer >= 1.0f / regenRate) {
            player.regenTimer -= 1.0f / regenRate;
            player.health++;
            if (!player.netControlled)
            SpawnDamageParticles(player.position.x + PLAYER_WIDTH / 2,
                                 player.position.y + PLAYER_HEIGHT / 2,
                                 (Color){80, 220, 80, 200});
        }
    } else {
        player.regenTimer = 0.0f;
    }

    // --- Death ---
    if (player.health <= 0 && !player.playerDead) {
        player.playerDead = true;
        player.health = 0;
        if (!player.netControlled) {
            SetDeathCause(pendingDeathCause);
            PlaySoundDeath();
        }
    }
}

void RespawnPlayer(void)
{
    // Lose non-hotbar items on death
    for (int i = HOTBAR_SLOTS; i < INVENTORY_SLOTS; i++) {
        player.inventory[i] = BLOCK_AIR;
        player.inventoryCount[i] = 0;
        player.toolDurability[i] = 0;
    }
    // Lose all armor on death
    for (int i = 0; i < 4; i++) {
        player.armor[i] = BLOCK_AIR;
        player.armorDurability[i] = 0;
        player.armorEnchantments[i] = 0;
    }
    int spawnX, spawnY;
    if (player.spawnX >= 0 && player.spawnY >= 0 &&
        player.spawnX < WORLD_WIDTH && player.spawnY + 1 < WORLD_HEIGHT &&
        world[player.spawnX][player.spawnY + 1] == BLOCK_BED) {
        spawnX = player.spawnX;
        spawnY = player.spawnY;
    } else {
        player.spawnX = -1;
        player.spawnY = -1;
        FindSpawnPoint(&spawnX, &spawnY);
    }
    player.position = (Vector2){ spawnX * BLOCK_SIZE, spawnY * BLOCK_SIZE };
    player.velocity = (Vector2){ 0, 0 };
    player.onGround = false;
    player.selectedSlot = 0;
    player.health = MAX_HEALTH;
    player.hunger = MAX_HUNGER;
    player.oxygen = MAX_OXYGEN;
    player.xp = 0;
    player.oxygenTimer = 0.0f;
    player.hungerTimer = 0.0f;
    player.regenTimer = 0.0f;
    player.drownTimer = 0.0f;
    player.hungerDamageTimer = 0.0f;
    player.damageFlashTimer = 0.0f;
    player.knockbackTimer = 0.0f;
    player.sprinting = false;
    player.playerDead = false;
    player.facingRight = true;
    player.wasInWater = false;
    player.footstepTimer = 0.0f;
    player.fallPeakVel = 0.0f;
    player.coyoteTimer = 0.0f;
    player.jumpBufferTimer = 0.0f;
    player.cameraShakeIntensity = 0.0f;
    player.cameraShakeTimer = 0.0f;
    InitCameraSystem();
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

    // Smooth camera lookahead (separate lerp so it doesn't jump on turn)
    static float currentLookahead = 0.0f;
    float targetLookahead = 0.0f;
    if (fabsf(player.velocity.x) > 20.0f) {
        targetLookahead = player.facingRight ? CAMERA_LOOKAHEAD : -CAMERA_LOOKAHEAD;
    }
    currentLookahead += (targetLookahead - currentLookahead) * 3.0f * dt;
    playerCenter.x += currentLookahead;

    camera.target = Vector2Lerp(camera.target, playerCenter, 8.0f * dt);

    // Camera shake with smooth decay
    if (player.cameraShakeIntensity > 0.01f) {
        player.cameraShakeIntensity -= CAMERA_SHAKE_DECAY * dt;
        if (player.cameraShakeIntensity < 0.0f) player.cameraShakeIntensity = 0.0f;
        float shake = player.cameraShakeIntensity;
        float offsetX = ((float)(rand() % 100) / 50.0f - 1.0f) * shake;
        float offsetY = ((float)(rand() % 100) / 50.0f - 1.0f) * shake;
        camera.target.x += offsetX;
        camera.target.y += offsetY;
    }
}

void TriggerCameraShake(float intensity, float duration)
{
    (void)duration;
    if (intensity > player.cameraShakeIntensity) {
        player.cameraShakeIntensity = intensity;
    }
}

//----------------------------------------------------------------------------------
// Hotbar Input
//----------------------------------------------------------------------------------
void UpdateHotbar(void)
{
    if (inventoryOpen) return;

    int oldSlot = player.selectedSlot;

    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        if (Win32IsKeyPressed(KEY_ONE + i)) player.selectedSlot = i;
    }

    float wheel = Win32GetMouseWheelMove();
    if (wheel < 0) player.selectedSlot = (player.selectedSlot + 1) % HOTBAR_SLOTS;
    if (wheel > 0) player.selectedSlot = (player.selectedSlot - 1 + HOTBAR_SLOTS) % HOTBAR_SLOTS;

    // Show item name when switching slots
    if (player.selectedSlot != oldSlot) {
        BlockType item = (BlockType)player.inventory[player.selectedSlot];
        if (item != BLOCK_AIR) {
            ShowMessage(GetBlockName(item), (Color){220, 220, 220, 255});
            messageTimer = 0.8f;
        }
    }

    // Q to drop selected hotbar item
    if (Win32IsKeyPressed(KEY_Q)) {
        int slot = player.selectedSlot;
        uint8_t item = player.inventory[slot];
        if (item != BLOCK_AIR) {
            int count = player.inventoryCount[slot];
            float px = player.position.x + PLAYER_WIDTH / 2;
            float py = player.position.y + PLAYER_HEIGHT / 2;
            SpawnItemEntity(item, count, px, py);
            player.inventory[slot] = BLOCK_AIR;
            player.inventoryCount[slot] = 0;
            player.toolDurability[slot] = 0;
            player.itemEnchantments[slot] = 0;
            PlaySoundDrop();
        }
    }
}
