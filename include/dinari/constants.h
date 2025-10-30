#ifndef DINARI_CONSTANTS_H
#define DINARI_CONSTANTS_H

#include "types.h"
#include <string>

namespace dinari {

// Blockchain identity
constexpr const char* BLOCKCHAIN_NAME = "Dinari Blockchain";
constexpr const char* TOKEN_NAME = "Dinari";
constexpr const char* TOKEN_SYMBOL = "DNT";

// Genesis block parameters
constexpr const char* GENESIS_TIMESTAMP = "2025-10-30 00:00:00 UTC - Dinari Blockchain Genesis";
constexpr const char* GENESIS_PUBKEY = "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f";

// Network strings
constexpr const char* USER_AGENT = "/Dinari:1.0.0/";
constexpr const char* PROTOCOL_VERSION_STRING = "1.0.0";
constexpr ProtocolVersion PROTOCOL_VERSION = 100000;
constexpr ProtocolVersion MIN_PEER_PROTOCOL_VERSION = 100000;

// DNS seeds for mainnet
constexpr const char* DNS_SEEDS[] = {
    "seed.dinari.network",
    "seed2.dinari.network",
    "dnsseed.dinari.io",
    "dnsseed.dinari.org"
};

// Hardcoded peers for bootstrapping (to be filled with actual IPs)
constexpr const char* HARDCODED_PEERS[] = {
    "node1.dinari.network:9333",
    "node2.dinari.network:9333"
};

// Testnet DNS seeds
constexpr const char* TESTNET_DNS_SEEDS[] = {
    "testseed.dinari.network",
    "testseed2.dinari.network"
};

// Checkpoints (block hash at specific heights) - for security
struct Checkpoint {
    BlockHeight height;
    const char* hash;
};

constexpr Checkpoint MAINNET_CHECKPOINTS[] = {
    {0, "0000000000000000000000000000000000000000000000000000000000000000"},  // Genesis
    // Add more checkpoints as blockchain grows
};

constexpr Checkpoint TESTNET_CHECKPOINTS[] = {
    {0, "0000000000000000000000000000000000000000000000000000000000000000"},  // Testnet genesis
};

// BIP32 HD wallet constants
constexpr uint32_t BIP32_HARDENED_BIT = 0x80000000;
constexpr const char* BIP44_COIN_TYPE = "1234";  // To be registered
constexpr uint32_t BIP44_COIN_TYPE_INT = 1234 | BIP32_HARDENED_BIT;

// BIP44 derivation path: m/44'/1234'/0'/0/0
constexpr const char* DEFAULT_DERIVATION_PATH = "m/44'/1234'/0'/0";

// Error messages
namespace errors {
    constexpr const char* INVALID_TRANSACTION = "Invalid transaction";
    constexpr const char* INVALID_BLOCK = "Invalid block";
    constexpr const char* INSUFFICIENT_FUNDS = "Insufficient funds";
    constexpr const char* DOUBLE_SPEND = "Double spend detected";
    constexpr const char* INVALID_SIGNATURE = "Invalid signature";
    constexpr const char* INVALID_ADDRESS = "Invalid address";
    constexpr const char* WALLET_LOCKED = "Wallet is locked";
    constexpr const char* NETWORK_ERROR = "Network error";
    constexpr const char* DATABASE_ERROR = "Database error";
}

// RPC command names
namespace rpc {
    // Blockchain commands
    constexpr const char* GET_BLOCKCHAIN_INFO = "getblockchaininfo";
    constexpr const char* GET_BLOCK_COUNT = "getblockcount";
    constexpr const char* GET_BLOCK = "getblock";
    constexpr const char* GET_BLOCK_HASH = "getblockhash";
    constexpr const char* GET_DIFFICULTY = "getdifficulty";
    constexpr const char* GET_BEST_BLOCK_HASH = "getbestblockhash";

    // Transaction commands
    constexpr const char* GET_TRANSACTION = "gettransaction";
    constexpr const char* GET_RAW_TRANSACTION = "getrawtransaction";
    constexpr const char* SEND_RAW_TRANSACTION = "sendrawtransaction";
    constexpr const char* GET_MEMPOOL_INFO = "getmempoolinfo";

    // Wallet commands
    constexpr const char* GET_NEW_ADDRESS = "getnewaddress";
    constexpr const char* GET_BALANCE = "getbalance";
    constexpr const char* SEND_TO_ADDRESS = "sendtoaddress";
    constexpr const char* LIST_TRANSACTIONS = "listtransactions";
    constexpr const char* CREATE_WALLET = "createwallet";
    constexpr const char* LOAD_WALLET = "loadwallet";
    constexpr const char* BACKUP_WALLET = "backupwallet";

    // Mining commands
    constexpr const char* GET_MINING_INFO = "getmininginfo";
    constexpr const char* SET_GENERATE = "setgenerate";
    constexpr const char* GET_BLOCK_TEMPLATE = "getblocktemplate";
    constexpr const char* SUBMIT_BLOCK = "submitblock";

    // Network commands
    constexpr const char* GET_PEER_INFO = "getpeerinfo";
    constexpr const char* GET_NET_INFO = "getnetworkinfo";
    constexpr const char* ADD_NODE = "addnode";
    constexpr const char* DISCONNECT_NODE = "disconnectnode";
}

// File names
namespace files {
    constexpr const char* WALLET_FILE = "wallet.dat";
    constexpr const char* BLOCKCHAIN_DIR = "blocks";
    constexpr const char* CHAINSTATE_DIR = "chainstate";
    constexpr const char* PEERS_FILE = "peers.dat";
    constexpr const char* BANLIST_FILE = "banlist.dat";
    constexpr const char* CONFIG_FILE = "dinari.conf";
    constexpr const char* LOG_FILE = "debug.log";
}

// Default data directory paths
#ifdef _WIN32
    constexpr const char* DEFAULT_DATA_DIR = "%APPDATA%\\Dinari";
#elif __APPLE__
    constexpr const char* DEFAULT_DATA_DIR = "~/Library/Application Support/Dinari";
#else
    constexpr const char* DEFAULT_DATA_DIR = "~/.dinari";
#endif

} // namespace dinari

#endif // DINARI_CONSTANTS_H
