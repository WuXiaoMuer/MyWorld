#include "net.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

//----------------------------------------------------------------------------------
// Internal Types
//----------------------------------------------------------------------------------
typedef struct {
    bool active;
    float lastSeen;         // Time of last packet from this client
    PktInput lastInput;     // Most recent input received
    struct sockaddr_in addr; // Client's address
    uint16_t lastRecvSeq;   // Highest received sequence number
} NetClient;

//----------------------------------------------------------------------------------
// Internal State
//----------------------------------------------------------------------------------
static bool netInitialized = false;
static bool netIsHost = false;
static bool netIsClient = false;
static SOCKET netSocket = INVALID_SOCKET;
static int netLocalPlayerId = 0;
static uint16_t netSendSeq = 0;

// Host state
static NetClient netClients[NET_MAX_PLAYERS];
static int netClientCount = 0;

// Receive buffer
typedef struct {
    uint8_t data[NET_PACKET_MAX];
    int size;
    int fromId;
} RecvPacket;

static RecvPacket recvBuf[NET_RECV_BUF_SIZE];
static int recvCount = 0;

// Reliable packet retransmission buffer
#define RELIABLE_BUF_SIZE 64
typedef struct {
    uint8_t data[NET_PACKET_MAX];
    int size;
    int destId;
    uint16_t seq;
    float sendTime;
    bool active;
} ReliablePacket;

static ReliablePacket reliableBuf[RELIABLE_BUF_SIZE];
static int reliableBufHead = 0;

// Timing
static double netStartTime = 0.0;

//----------------------------------------------------------------------------------
// Platform Helpers
//----------------------------------------------------------------------------------
static double GetNetTimeSeconds(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
#endif
}

static void SetNonBlocking(SOCKET sock)
{
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
}

static bool AddrEqual(const struct sockaddr_in *a, const struct sockaddr_in *b)
{
    return a->sin_addr.s_addr == b->sin_addr.s_addr &&
           a->sin_port == b->sin_port;
}

//----------------------------------------------------------------------------------
// Init/Shutdown
//----------------------------------------------------------------------------------
bool NetInit(void)
{
    if (netInitialized) return true;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

    netStartTime = GetNetTimeSeconds();
    netInitialized = true;
    return true;
}

void NetShutdown(void)
{
    if (!netInitialized) return;

    if (netSocket != INVALID_SOCKET) {
        closesocket(netSocket);
        netSocket = INVALID_SOCKET;
    }

#ifdef _WIN32
    WSACleanup();
#endif

    netIsHost = false;
    netIsClient = false;
    netInitialized = false;
}

//----------------------------------------------------------------------------------
// Host Mode
//----------------------------------------------------------------------------------
bool NetHostStart(int port)
{
    if (!netInitialized && !NetInit()) return false;

    netSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (netSocket == INVALID_SOCKET) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(netSocket, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(netSocket);
        netSocket = INVALID_SOCKET;
        return false;
    }

    SetNonBlocking(netSocket);
    netIsHost = true;
    netIsClient = false;
    netLocalPlayerId = 0;
    netClientCount = 0;
    memset(netClients, 0, sizeof(netClients));

    // Host is always player 0
    netClients[0].active = true;
    netClients[0].lastSeen = (float)GetNetTimeSeconds();

    return true;
}

void NetHostStop(void)
{
    // Send disconnect to all clients
    PktDisconnect pkt;
    pkt.playerId = 0;
    snprintf(pkt.reason, sizeof(pkt.reason), "Host closed");
    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_DISCONNECT;
    memcpy(buf + 1, &pkt, sizeof(pkt));
    NetSendToAll(buf, 1 + sizeof(pkt), false);

    if (netSocket != INVALID_SOCKET) {
        closesocket(netSocket);
        netSocket = INVALID_SOCKET;
    }
    netIsHost = false;
    netClientCount = 0;
}

int NetHostGetClientCount(void)
{
    return netClientCount;
}

//----------------------------------------------------------------------------------
// Client Mode
//----------------------------------------------------------------------------------
bool NetClientConnect(const char *ip, int port, const char *playerName)
{
    if (!netInitialized && !NetInit()) return false;

    netSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (netSocket == INVALID_SOCKET) return false;

    SetNonBlocking(netSocket);

    // Store server address as client slot 0
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serverAddr.sin_addr);

    memset(netClients, 0, sizeof(netClients));
    netClients[0].active = true;
    netClients[0].addr = serverAddr;
    netClients[0].lastSeen = (float)GetNetTimeSeconds();

    netIsClient = true;
    netIsHost = false;
    netLocalPlayerId = -1; // Will be assigned by server

    // Send join request
    PktJoin join;
    memset(&join, 0, sizeof(join));
    snprintf(join.playerName, sizeof(join.playerName), "%s", playerName ? playerName : "Player");

    uint8_t buf[NET_PACKET_MAX];
    buf[0] = PKT_JOIN;
    memcpy(buf + 1, &join, sizeof(join));
    NetSendToServer(buf, 1 + sizeof(join), true);

    return true;
}

void NetClientDisconnect(void)
{
    if (netIsClient && netSocket != INVALID_SOCKET) {
        uint8_t buf[4];
        buf[0] = PKT_DISCONNECT;
        NetSendToServer(buf, 1, false);
    }

    if (netSocket != INVALID_SOCKET) {
        closesocket(netSocket);
        netSocket = INVALID_SOCKET;
    }
    netIsClient = false;
    netLocalPlayerId = 0;
}

//----------------------------------------------------------------------------------
// Status
//----------------------------------------------------------------------------------
bool NetIsHost(void) { return netIsHost; }
bool NetIsClient(void) { return netIsClient; }
bool NetIsConnected(void) { return netIsHost || netIsClient; }
int NetGetLocalPlayerId(void) { return netLocalPlayerId; }

int NetGetPlayerCount(void)
{
    int count = 0;
    for (int i = 0; i < NET_MAX_PLAYERS; i++) {
        if (netClients[i].active) count++;
    }
    return count;
}

float NetGetTime(void)
{
    return (float)(GetNetTimeSeconds() - netStartTime);
}

void NetGetLocalIP(char *buf, int bufSize)
{
    snprintf(buf, bufSize, "127.0.0.1");
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct hostent *he = gethostbyname(hostname);
        if (he && he->h_addr_list[0]) {
            struct in_addr addr;
            memcpy(&addr, he->h_addr_list[0], sizeof(addr));
            snprintf(buf, bufSize, "%s", inet_ntoa(addr));
        }
    }
}

//----------------------------------------------------------------------------------
// Send Functions
//----------------------------------------------------------------------------------
void NetSendTo(int playerId, const void *data, int size, bool reliable)
{
    if (netSocket == INVALID_SOCKET) return;
    if (playerId < 0 || playerId >= NET_MAX_PLAYERS) return;
    if (!netClients[playerId].active) return;
    if (size < 1 || size > NET_PACKET_MAX - (int)sizeof(PacketHeader)) return;

    // Build packet with header
    uint8_t buf[NET_PACKET_MAX];
    PacketHeader *hdr = (PacketHeader *)buf;
    hdr->type = ((uint8_t *)data)[0];
    hdr->seq = reliable ? ++netSendSeq : 0;
    hdr->ack = netClients[playerId].lastRecvSeq;
    memcpy(buf + sizeof(PacketHeader), (uint8_t *)data + 1, size - 1);

    int totalSize = sizeof(PacketHeader) + size - 1;
    sendto(netSocket, (const char *)buf, totalSize, 0,
           (struct sockaddr *)&netClients[playerId].addr,
           sizeof(struct sockaddr_in));

    // Store in retransmission buffer for reliable packets
    if (reliable && hdr->seq > 0) {
        ReliablePacket *rp = &reliableBuf[reliableBufHead];
        memcpy(rp->data, buf, totalSize);
        rp->size = totalSize;
        rp->destId = playerId;
        rp->seq = hdr->seq;
        rp->sendTime = (float)GetNetTimeSeconds();
        rp->active = true;
        reliableBufHead = (reliableBufHead + 1) % RELIABLE_BUF_SIZE;
    }
}

void NetSendToAll(const void *data, int size, bool reliable)
{
    for (int i = 1; i < NET_MAX_PLAYERS; i++) {
        if (netClients[i].active) {
            NetSendTo(i, data, size, reliable);
        }
    }
}

void NetSendToServer(const void *data, int size, bool reliable)
{
    if (!netIsClient) return;
    NetSendTo(0, data, size, reliable);
}

//----------------------------------------------------------------------------------
// Receive Functions (called each frame)
//----------------------------------------------------------------------------------
void NetPoll(void)
{
    if (netSocket == INVALID_SOCKET) return;

    recvCount = 0;
    uint8_t buf[NET_PACKET_MAX];
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);

    while (recvCount < NET_RECV_BUF_SIZE) {
        fromLen = sizeof(fromAddr);
        int n = recvfrom(netSocket, (char *)buf, NET_PACKET_MAX, 0,
                         (struct sockaddr *)&fromAddr, &fromLen);
        if (n <= 0) break;

        // Skip packet header for payload
        if (n < (int)sizeof(PacketHeader)) continue;

        PacketHeader *hdr = (PacketHeader *)buf;
        int payloadSize = n - sizeof(PacketHeader);

        // Find or assign client ID
        int fromId = -1;

        if (netIsHost) {
            // Find existing client by address
            for (int i = 1; i < NET_MAX_PLAYERS; i++) {
                if (netClients[i].active && AddrEqual(&netClients[i].addr, &fromAddr)) {
                    fromId = i;
                    break;
                }
            }

            // New client?
            if (fromId == -1 && hdr->type == PKT_JOIN) {
                // Find free slot
                for (int i = 1; i < NET_MAX_PLAYERS; i++) {
                    if (!netClients[i].active) {
                        netClients[i].active = true;
                        netClients[i].addr = fromAddr;
                        netClients[i].lastSeen = NetGetTime();
                        fromId = i;
                        netClientCount++;
                        break;
                    }
                }
            }

            if (fromId >= 0) {
                netClients[fromId].lastSeen = NetGetTime();
                // Track highest received sequence for ACK
                if (hdr->seq > 0 && hdr->seq > netClients[fromId].lastRecvSeq) {
                    netClients[fromId].lastRecvSeq = hdr->seq;
                }
            }
        } else {
            // Client: packets come from server (slot 0)
            fromId = 0;
            if (hdr->seq > 0 && hdr->seq > netClients[0].lastRecvSeq) {
                netClients[0].lastRecvSeq = hdr->seq;
            }
        }

        if (fromId < 0) continue;

        // Process ACK: clear retransmission entries for acknowledged packets
        if (hdr->ack > 0) {
            for (int i = 0; i < RELIABLE_BUF_SIZE; i++) {
                if (reliableBuf[i].active && reliableBuf[i].destId == fromId &&
                    reliableBuf[i].seq <= hdr->ack) {
                    reliableBuf[i].active = false;
                }
            }
        }

        // Duplicate detection: skip if we already received this sequence
        if (hdr->seq > 0) {
            uint16_t last = netClients[fromId].lastRecvSeq;
            // Handle wraparound: compare using modular arithmetic
            int16_t diff = (int16_t)(hdr->seq - last);
            if (diff <= 0 && diff > -32) {
                continue; // Already received or too old
            }
        }

        // Store in receive buffer
        recvBuf[recvCount].size = payloadSize + 1; // +1 for type byte
        recvBuf[recvCount].fromId = fromId;
        recvBuf[recvCount].data[0] = hdr->type;
        memcpy(recvBuf[recvCount].data + 1, buf + sizeof(PacketHeader), payloadSize);
        recvCount++;
    }

    // Retransmit reliable packets that haven't been ACKed
    {
        float now = (float)GetNetTimeSeconds();
        for (int i = 0; i < RELIABLE_BUF_SIZE; i++) {
            if (reliableBuf[i].active && (now - reliableBuf[i].sendTime) > 0.5f) {
                // Resend
                if (netClients[reliableBuf[i].destId].active) {
                    sendto(netSocket, (const char *)reliableBuf[i].data, reliableBuf[i].size, 0,
                           (struct sockaddr *)&netClients[reliableBuf[i].destId].addr,
                           sizeof(struct sockaddr_in));
                    reliableBuf[i].sendTime = now;
                }
            }
        }
    }

    // Timeout check for host
    if (netIsHost) {
        float now = NetGetTime();
        for (int i = 1; i < NET_MAX_PLAYERS; i++) {
            if (netClients[i].active && (now - netClients[i].lastSeen) > NET_TIMEOUT) {
                netClients[i].active = false;
                if (netClientCount > 0) netClientCount--;
            }
        }
    }
}

int NetGetReceivedCount(void)
{
    return recvCount;
}

const void *NetGetReceived(int index, int *size, int *fromId)
{
    if (index < 0 || index >= recvCount) return NULL;
    if (size) *size = recvBuf[index].size;
    if (fromId) *fromId = recvBuf[index].fromId;
    return recvBuf[index].data;
}
