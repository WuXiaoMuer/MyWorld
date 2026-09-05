#ifndef NET_H
#define NET_H

#include <stdbool.h>
#include <stdint.h>

//----------------------------------------------------------------------------------
// Network Constants
//----------------------------------------------------------------------------------
#define NET_PORT            7777
#define NET_MAX_PLAYERS     4
#define NET_PACKET_MAX      1400
#define NET_TICK_RATE       20      // Server broadcasts state at 20 Hz
#define NET_TICK_INTERVAL   (1.0f / NET_TICK_RATE)
#define NET_TIMEOUT         10.0f   // Seconds before disconnecting idle client
#define NET_RECV_BUF_SIZE   64      // Max packets buffered per frame
#define NET_CHEST_SLOTS     27      // Must match CHEST_SLOTS in types.h
#define NET_INVENTORY_SLOTS 36      // Must match INVENTORY_SLOTS in types.h

//----------------------------------------------------------------------------------
// Packet Types
//----------------------------------------------------------------------------------
typedef enum {
    PKT_JOIN = 1,           // Client -> Server: request to join
    PKT_WELCOME,            // Server -> Client: world seed + player ID
    PKT_INPUT,              // Client -> Server: player input state
    PKT_PLAYER_STATE,       // Server -> Client: all player positions
    PKT_MOB_STATE,          // Server -> Client: mob positions
    PKT_ENTITY_SPAWN,       // Server -> Client: item entity spawned
    PKT_ENTITY_PICKUP,      // Server -> Client: item entity picked up
    PKT_PROJECTILE_SPAWN,   // Server -> Client: projectile spawned
    PKT_BLOCK_CHANGE,       // Bidirectional: block placed/broken
    PKT_DAMAGE_MOB,         // Client -> Server: player hit a mob
    PKT_DAMAGE_PLAYER,      // Server -> Client: player took damage
    PKT_INVENTORY_SYNC,     // Bidirectional: full inventory sync
    PKT_CRAFT_REQUEST,      // Client -> Server: request to craft a recipe
    PKT_BOW_REQUEST,        // Client -> Server: fire an arrow
    PKT_ENDER_PEARL_REQUEST,// Client -> Server: use ender pearl
    PKT_PLAYER_TELEPORT,    // Server -> Client: player was teleported
    PKT_ENCHANT_REQUEST,    // Client -> Server: apply enchantment
    PKT_FISHING_REQUEST,    // Client -> Server: cast/retract fishing rod
    PKT_CAULDRON_SYNC,      // Bidirectional: cauldron state sync
    PKT_ITEM_DROP,          // Client -> Server: drop item from inventory
    PKT_ACHIEVEMENT_UNLOCK, // Bidirectional: achievement unlocked
    PKT_SOUND_EVENT,        // Server -> Client: play sound at position
    PKT_TIME_SYNC,          // Server -> Client: day/night time
    PKT_WEATHER_SYNC,       // Server -> Client: weather change
    PKT_CHAT,               // Bidirectional: chat message
    PKT_CHEST_OPEN,         // Client -> Server: request chest contents
    PKT_CHEST_SYNC,         // Bidirectional: full chest slot data
    PKT_CHEST_CLOSE,        // Client -> Server: closed chest UI
    PKT_FURNACE_OPEN,       // Client -> Server: request furnace contents
    PKT_FURNACE_SYNC,       // Bidirectional: full furnace slot data
    PKT_FURNACE_CLOSE,      // Client -> Server: closed furnace UI
    PKT_DISCONNECT,         // Bidirectional: disconnect notice
    PKT_PING,               // Bidirectional: keepalive
    PKT_GAMEMODE_SYNC,      // Server -> Client: game mode changed
    PKT_FLUID_REQUEST,       // Client -> host: validated bucket action
    PKT_FLUID_DELTA,         // Host -> clients: authoritative fluid cells
    PKT_FLUID_SNAPSHOT       // Host -> joining client: fluid state batch
} PacketType;

//----------------------------------------------------------------------------------
// Packet Header (5 bytes)
//----------------------------------------------------------------------------------
typedef struct {
    uint8_t type;           // PacketType
    uint16_t seq;           // Sequence number (for reliability)
    uint16_t ack;           // Acknowledgment of last received seq
} PacketHeader;

#define FLUID_REQUEST_PLACE    0
#define FLUID_REQUEST_COLLECT  1
#define FLUID_REQUEST_BEGIN_SNAPSHOT 2
#define FLUID_REQUEST_ACK_SNAPSHOT   3

typedef struct {
    uint16_t x, y;
    uint8_t action;
    uint8_t slot;
} PktFluidRequest;

typedef struct {
    uint16_t x, y;
    uint8_t blockType;
    uint8_t kind;
    uint8_t level;
    uint8_t source;
} PktFluidCell;

typedef struct {
    uint16_t snapshotId;
    uint16_t batchIndex;
    uint16_t batchCount;
    uint8_t count;
} PktFluidSnapshotHeader;



// PKT_JOIN
typedef struct {
    char playerName[32];
} PktJoin;

// PKT_WELCOME
typedef struct {
    uint8_t playerId;
    uint32_t worldSeed;
    int worldW, worldH;
    float timeOfDay;
    uint8_t weatherType;
    float weatherDuration;
    float spawnX, spawnY;       // Host's player spawn position
    uint8_t gameMode;           // 0=survival, 1=creative
} PktWelcome;

// PKT_INPUT
typedef struct {
    float moveX;            // -1.0 to 1.0
    bool jump;
    bool sprint;
    bool attack;            // Left click
    bool place;             // Right click
    bool use;               // E key
    float cursorX, cursorY; // World-space cursor position
    int selectedSlot;
} PktInput;

// PKT_PLAYER_STATE - sent as array
typedef struct {
    uint8_t playerId;
    float x, y;
    float vx, vy;
    bool facingRight;
    bool sprinting;
    bool onGround;
    int selectedSlot;
    int health;
    uint8_t armor[4];
    char playerName[32];
} PktPlayerInfo;

typedef struct {
    uint8_t count;
    PktPlayerInfo players[NET_MAX_PLAYERS];
} PktPlayerState;

// PKT_MOB_STATE - sent as array
typedef struct {
    uint8_t index;          // Mob's index in the server mobs[] array
    uint8_t type;
    float x, y;
    float vx, vy;
    int health;
    bool active;
    bool facingRight;
} PktMobInfo;

typedef struct {
    uint8_t count;
    PktMobInfo mobs[32]; // MAX_MOBS
} PktMobState;

// PKT_BLOCK_CHANGE
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t blockType;
} PktBlockChange;

// PKT_ENTITY_SPAWN
typedef struct {
    uint8_t itemType;
    int count;
    float x, y;
} PktEntitySpawn;

// PKT_ENTITY_PICKUP
typedef struct {
    uint16_t entityIndex;
    uint8_t playerId;
    uint8_t itemType;       // Added for Milestone 2: what was picked up
    int count;              // Added for Milestone 2: how many
} PktEntityPickup;

// PKT_PROJECTILE_SPAWN
typedef struct {
    float x, y;
    float vx, vy;
    bool fromPlayer;
    uint8_t playerId;       // which player fired (if fromPlayer)
    bool isFishing;         // true = fishing rod bobber (added for Milestone 2)
} PktProjectileSpawn;

// PKT_DAMAGE_MOB
typedef struct {
    uint8_t mobIndex;
    int damage;
} PktDamageMob;

// PKT_DAMAGE_PLAYER
typedef struct {
    uint8_t playerId;
    int damage;
    float knockbackX, knockbackY;
} PktDamagePlayer;

// PKT_TIME_SYNC
typedef struct {
    float timeOfDay;
} PktTimeSync;

// PKT_WEATHER_SYNC
typedef struct {
    uint8_t weatherType;
    float duration;
} PktWeatherSync;

// PKT_CHAT
typedef struct {
    uint8_t playerId;
    char message[128];
} PktChat;

// PKT_PING
typedef struct {
    uint32_t timestamp;
} PktPing;

// PKT_DISCONNECT
typedef struct {
    uint8_t playerId;
    char reason[64];
} PktDisconnect;

// PKT_CHEST_OPEN / PKT_CHEST_CLOSE
typedef struct {
    int16_t x, y;
} PktChestOpen;

// PKT_CHEST_SYNC
typedef struct {
    int16_t x, y;
    uint8_t items[NET_CHEST_SLOTS];
    int counts[NET_CHEST_SLOTS];
    int durability[NET_CHEST_SLOTS];
    uint16_t enchantments[NET_CHEST_SLOTS];
} PktChestSync;

// PKT_FURNACE_OPEN / PKT_FURNACE_CLOSE
typedef struct {
    int16_t x, y;
} PktFurnaceOpen;

// PKT_FURNACE_SYNC
typedef struct {
    int16_t x, y;
    uint8_t fuel; int fuelCount;
    uint8_t input; int inputCount;
    uint8_t output; int outputCount;
    float progress;
    float fuelBurn;
    float fuelBurnMax;
} PktFurnaceSync;

// PKT_INVENTORY_SYNC - full inventory + armor state (server <-> client)
typedef struct {
    uint8_t  playerId;                                  // server->client: owner; client->server: ignored
    uint8_t  inventory[NET_INVENTORY_SLOTS];
    int      inventoryCount[NET_INVENTORY_SLOTS];
    int      toolDurability[NET_INVENTORY_SLOTS];
    uint16_t itemEnchantments[NET_INVENTORY_SLOTS];
    uint8_t  armor[4];
    int      armorDurability[4];
    uint16_t armorEnchantments[4];
} PktInventorySync;

// PKT_CRAFT_REQUEST - Client -> Server: craft recipe(s)
// Recipe index is in the global craftRecipes[] table.
typedef struct {
    int recipeIndex;
    int count;          // 1 for single craft, >1 for shift-click craft-all (capped by host)
} PktCraftRequest;

// PKT_BOW_REQUEST - Client -> Server: fire an arrow
typedef struct {
    float spawnX, spawnY;   // Projectile spawn position
    float vx, vy;           // Projectile velocity (normalized direction * speed)
    float charge;           // 0.0-1.0 charge level (affects speed/damage)
} PktBowRequest;

// PKT_ENDER_PEARL_REQUEST - Client -> Server: use ender pearl
typedef struct {
    float targetX, targetY; // Desired teleport destination
} PktEnderPearlRequest;

// PKT_PLAYER_TELEPORT - Server -> Client: player was teleported
typedef struct {
    uint8_t playerId;
    float x, y;
} PktPlayerTeleport;

// PKT_ENCHANT_REQUEST - Client -> Server: apply enchantment
typedef struct {
    int optionIndex;        // 0-2 index into the enchant session options
    int blockX, blockY;     // Enchanting table position
    int enchantType;        // EnchantmentType (sent so host can validate)
    int enchantLevel;       // 1-3
    int xpCost;             // XP cost the client was quoted
} PktEnchantRequest;

// PKT_FISHING_REQUEST - Client -> Server: cast or retract fishing rod
typedef struct {
    uint8_t action;         // 0 = CAST, 1 = RETRACT
    float vx, vy;           // Cast direction (only for CAST)
} PktFishingRequest;

// PKT_CAULDRON_SYNC - Bidirectional: cauldron state sync
typedef struct {
    int16_t x, y;
    int fillLevel;          // 0-3
} PktCauldronSync;

// PKT_ITEM_DROP - Client -> Server: drop item from inventory
typedef struct {
    int slot;               // Inventory slot to drop
} PktItemDrop;

// PKT_ACHIEVEMENT_UNLOCK - Bidirectional: achievement notification
typedef struct {
    uint8_t achievementId;
} PktAchievementUnlock;

// PKT_SOUND_EVENT - Server -> Client: play sound at position
// Sound IDs: 0=bow fire, 1=teleport/hurt, 2=fishing catch, 3=splash
typedef struct {
    uint8_t soundId;
    float x, y;         // World position (0,0 = centered, ignore position)
} PktSoundEvent;

//----------------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------------

// Init/Shutdown
bool NetInit(void);
void NetShutdown(void);

// Host mode
bool NetHostStart(int port);
void NetHostStop(void);
int NetHostGetClientCount(void);

// Client mode
bool NetClientConnect(const char *ip, int port, const char *playerName);
void NetClientDisconnect(void);

// Status
bool NetIsHost(void);
bool NetIsClient(void);
bool NetIsConnected(void);
int NetGetLocalPlayerId(void);
int NetGetPlayerCount(void);

// Send/Receive (called each frame)
void NetPoll(void);
void NetSendTo(int playerId, const void *data, int size, bool reliable);
void NetSendToAll(const void *data, int size, bool reliable);
void NetSendToServer(const void *data, int size, bool reliable);

// Receive queue
int NetGetReceivedCount(void);
const void *NetGetReceived(int index, int *size, int *fromId);

// Utilities
float NetGetTime(void);
void NetGetLocalIP(char *buf, int bufSize);

#endif // NET_H
