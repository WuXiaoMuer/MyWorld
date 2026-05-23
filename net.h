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
    PKT_INVENTORY_SYNC,     // Server -> Client: full inventory sync
    PKT_TIME_SYNC,          // Server -> Client: day/night time
    PKT_WEATHER_SYNC,       // Server -> Client: weather change
    PKT_CHAT,               // Bidirectional: chat message
    PKT_DISCONNECT,         // Bidirectional: disconnect notice
    PKT_PING,               // Bidirectional: keepalive
} PacketType;

//----------------------------------------------------------------------------------
// Packet Header (5 bytes)
//----------------------------------------------------------------------------------
typedef struct {
    uint8_t type;           // PacketType
    uint16_t seq;           // Sequence number (for reliability)
    uint16_t ack;           // Acknowledgment of last received seq
} PacketHeader;

//----------------------------------------------------------------------------------
// Packet Payloads
//----------------------------------------------------------------------------------

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
} PktPlayerInfo;

typedef struct {
    uint8_t count;
    PktPlayerInfo players[NET_MAX_PLAYERS];
} PktPlayerState;

// PKT_MOB_STATE - sent as array
typedef struct {
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
} PktEntityPickup;

// PKT_PROJECTILE_SPAWN
typedef struct {
    float x, y;
    float vx, vy;
    bool fromPlayer;
    uint8_t playerId;       // which player fired (if fromPlayer)
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

// PKT_DISCONNECT
typedef struct {
    uint8_t playerId;
    char reason[64];
} PktDisconnect;

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
bool NetClientConnect(const char *ip, int port);
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
