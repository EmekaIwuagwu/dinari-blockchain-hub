#ifndef DINARI_NETWORK_PROTOCOL_H
#define DINARI_NETWORK_PROTOCOL_H

#include "dinari/types.h"
#include <string>
#include <array>

namespace dinari {

/**
 * @brief Network protocol version
 */
constexpr uint32_t PROTOCOL_VERSION = 70001;
constexpr uint32_t MIN_PROTOCOL_VERSION = 70001;

/**
 * @brief Network magic bytes (mainnet)
 */
constexpr uint32_t MAINNET_MAGIC = 0xD9B4BEF9;
constexpr uint32_t TESTNET_MAGIC = 0xDAB5BFFA;

/**
 * @brief Network ports
 */
constexpr uint16_t DEFAULT_PORT = 9333;
constexpr uint16_t DEFAULT_RPC_PORT = 9334;
constexpr uint16_t TESTNET_PORT = 19333;
constexpr uint16_t TESTNET_RPC_PORT = 19334;

/**
 * @brief Network timeouts and limits
 */
constexpr uint32_t PING_INTERVAL = 120;  // seconds
constexpr uint32_t TIMEOUT_INTERVAL = 900;  // 15 minutes
constexpr uint32_t MAX_OUTBOUND_CONNECTIONS = 8;
constexpr uint32_t MAX_INBOUND_CONNECTIONS = 125;
constexpr uint32_t MAX_ADDRS_PER_MESSAGE = 1000;
constexpr uint32_t MAX_INV_PER_MESSAGE = 50000;
constexpr uint32_t MAX_HEADERS_PER_MESSAGE = 2000;
constexpr size_t MAX_MESSAGE_SIZE = 32 * 1024 * 1024;  // 32MB

/**
 * @brief Network message types
 */
enum class NetMsgType : uint32_t {
    // Connection messages
    VERSION = 0x01,
    VERACK = 0x02,

    // Keepalive
    PING = 0x10,
    PONG = 0x11,

    // Peer discovery
    ADDR = 0x20,
    GETADDR = 0x21,

    // Inventory and data
    INV = 0x30,
    GETDATA = 0x31,
    NOTFOUND = 0x32,

    // Blocks
    GETBLOCKS = 0x40,
    GETHEADERS = 0x41,
    BLOCK = 0x42,
    HEADERS = 0x43,

    // Transactions
    TX = 0x50,
    MEMPOOL = 0x51,

    // Rejection
    REJECT = 0x60,

    // Other
    ALERT = 0x70,
    FILTERLOAD = 0x71,
    FILTERADD = 0x72,
    FILTERCLEAR = 0x73,
    MERKLEBLOCK = 0x74
};

/**
 * @brief Inventory types
 */
enum class InvType : uint32_t {
    ERROR = 0,
    TX = 1,
    BLOCK = 2,
    FILTERED_BLOCK = 3,
    COMPACT_BLOCK = 4
};

/**
 * @brief Service flags
 */
enum ServiceFlags : uint64_t {
    NODE_NONE = 0,
    NODE_NETWORK = (1 << 0),      // Can serve full blocks
    NODE_GETUTXO = (1 << 1),      // Can respond to UTXO queries
    NODE_BLOOM = (1 << 2),        // Can handle bloom filters
    NODE_WITNESS = (1 << 3),      // Supports witness data
    NODE_COMPACT_FILTERS = (1 << 6),  // Supports compact filters
    NODE_NETWORK_LIMITED = (1 << 10)  // Limited network mode
};

/**
 * @brief Rejection codes
 */
enum class RejectCode : uint8_t {
    MALFORMED = 0x01,
    INVALID = 0x10,
    OBSOLETE = 0x11,
    DUPLICATE = 0x12,
    NONSTANDARD = 0x40,
    DUST = 0x41,
    INSUFFICIENTFEE = 0x42,
    CHECKPOINT = 0x43
};

/**
 * @brief Network address
 */
struct NetworkAddress {
    uint64_t services;
    std::array<uint8_t, 16> ip;  // IPv6 format (IPv4 mapped)
    uint16_t port;
    Timestamp timestamp;  // Only in ADDR messages

    NetworkAddress()
        : services(NODE_NETWORK)
        , ip({0})
        , port(DEFAULT_PORT)
        , timestamp(0) {}

    bool IsIPv4() const {
        return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0 &&
               ip[4] == 0 && ip[5] == 0 && ip[6] == 0 && ip[7] == 0 &&
               ip[8] == 0 && ip[9] == 0 && ip[10] == 0xff && ip[11] == 0xff;
    }

    std::string ToString() const;

    bool IsValid() const;
    bool IsRoutable() const;
    bool IsLocal() const;
};

/**
 * @brief Inventory item
 */
struct InvItem {
    InvType type;
    Hash256 hash;

    InvItem() : type(InvType::ERROR), hash({0}) {}
    InvItem(InvType t, const Hash256& h) : type(t), hash(h) {}

    bool operator==(const InvItem& other) const {
        return type == other.type && hash == other.hash;
    }
};

/**
 * @brief Block locator
 */
struct BlockLocator {
    std::vector<Hash256> hashes;

    BlockLocator() {}
    explicit BlockLocator(const std::vector<Hash256>& h) : hashes(h) {}
};

/**
 * @brief Convert message type to string
 */
inline const char* GetMessageTypeName(NetMsgType type) {
    switch (type) {
        case NetMsgType::VERSION: return "version";
        case NetMsgType::VERACK: return "verack";
        case NetMsgType::PING: return "ping";
        case NetMsgType::PONG: return "pong";
        case NetMsgType::ADDR: return "addr";
        case NetMsgType::GETADDR: return "getaddr";
        case NetMsgType::INV: return "inv";
        case NetMsgType::GETDATA: return "getdata";
        case NetMsgType::NOTFOUND: return "notfound";
        case NetMsgType::GETBLOCKS: return "getblocks";
        case NetMsgType::GETHEADERS: return "getheaders";
        case NetMsgType::BLOCK: return "block";
        case NetMsgType::HEADERS: return "headers";
        case NetMsgType::TX: return "tx";
        case NetMsgType::MEMPOOL: return "mempool";
        case NetMsgType::REJECT: return "reject";
        case NetMsgType::ALERT: return "alert";
        case NetMsgType::FILTERLOAD: return "filterload";
        case NetMsgType::FILTERADD: return "filteradd";
        case NetMsgType::FILTERCLEAR: return "filterclear";
        case NetMsgType::MERKLEBLOCK: return "merkleblock";
        default: return "unknown";
    }
}

/**
 * @brief DNS seeds for peer discovery
 */
const std::vector<std::string> MAINNET_DNS_SEEDS = {
    "seed.dinari.io",
    "seed1.dinari.io",
    "seed2.dinari.io",
    "dnsseed.dinari.network"
};

const std::vector<std::string> TESTNET_DNS_SEEDS = {
    "testnet-seed.dinari.io",
    "testnet-seed1.dinari.io"
};

/**
 * @brief Hardcoded seed peers
 */
const std::vector<std::string> MAINNET_SEED_PEERS = {
    "seed1.dinari.io:9333",
    "seed2.dinari.io:9333"
};

const std::vector<std::string> TESTNET_SEED_PEERS = {
    "testnet-seed.dinari.io:19333"
};

} // namespace dinari

#endif // DINARI_NETWORK_PROTOCOL_H
