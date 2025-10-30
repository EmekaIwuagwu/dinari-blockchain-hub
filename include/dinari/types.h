#ifndef DINARI_TYPES_H
#define DINARI_TYPES_H

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <memory>

namespace dinari {

// Basic types
using byte = uint8_t;
using bytes = std::vector<byte>;

// Hash types
using Hash256 = std::array<byte, 32>;  // SHA-256 hash
using Hash160 = std::array<byte, 20>;  // RIPEMD-160 hash

// Blockchain types
using BlockHeight = uint32_t;
using Timestamp = uint64_t;
using Amount = uint64_t;  // Amount in smallest unit (satoshi-equivalent)
using Nonce = uint64_t;
using Difficulty = uint32_t;

// Network types
using Port = uint16_t;
using ProtocolVersion = uint32_t;

// Transaction types
using TxIndex = uint32_t;
using TxOutIndex = uint32_t;

// Constants for amounts
constexpr Amount COIN = 100000000;  // 1 DNT = 100,000,000 smallest units
constexpr Amount MAX_MONEY = 700000000000000ULL * COIN;  // 700 Trillion DNT

// Network magic bytes
constexpr uint32_t MAINNET_MAGIC = 0xD1A2B3C4;
constexpr uint32_t TESTNET_MAGIC = 0xD5E6F7A8;

// Block parameters
constexpr size_t MAX_BLOCK_SIZE = 2 * 1024 * 1024;  // 2MB
constexpr size_t MAX_BLOCK_SIGOPS = 20000;
constexpr BlockHeight COINBASE_MATURITY = 100;  // Blocks before coinbase can be spent

// Network parameters
constexpr Port DEFAULT_PORT = 9333;
constexpr Port DEFAULT_TESTNET_PORT = 19333;
constexpr Port DEFAULT_RPC_PORT = 9334;
constexpr Port DEFAULT_RPC_TESTNET_PORT = 19334;
constexpr size_t MAX_PEER_CONNECTIONS = 125;
constexpr size_t MAX_OUTBOUND_CONNECTIONS = 8;

// Consensus parameters
constexpr BlockHeight DIFFICULTY_ADJUSTMENT_INTERVAL = 2016;  // Blocks
constexpr Timestamp TARGET_BLOCK_TIME = 600;  // 10 minutes in seconds
constexpr BlockHeight SUBSIDY_HALVING_INTERVAL = 210000;  // Blocks

// Mining parameters
constexpr Difficulty INITIAL_DIFFICULTY = 0x1d00ffff;  // Compact format
constexpr uint32_t MAX_NONCE = 0xFFFFFFFF;

// Memory pool parameters
constexpr size_t MAX_MEMPOOL_SIZE = 300 * 1024 * 1024;  // 300MB
constexpr Amount MIN_RELAY_TX_FEE = 1000;  // Minimum fee per KB

// Wallet parameters
constexpr size_t WALLET_VERSION = 1;
constexpr size_t MNEMONIC_WORDS = 24;  // BIP39 24-word mnemonic

// Address parameters
constexpr char ADDRESS_PREFIX = 'D';  // Dinari address prefix
constexpr byte PUBKEY_ADDRESS_VERSION = 30;  // Leads to 'D' prefix
constexpr byte SCRIPT_ADDRESS_VERSION = 50;
constexpr byte TESTNET_ADDRESS_VERSION = 111;

// Result types
template<typename T>
struct Result {
    bool success;
    T value;
    std::string error;

    Result() : success(false) {}
    Result(const T& val) : success(true), value(val) {}
    Result(bool s, const std::string& err) : success(s), error(err) {}

    bool isSuccess() const { return success; }
    bool isError() const { return !success; }

    static Result Success(const T& val) {
        return Result(val);
    }

    static Result Error(const std::string& err) {
        return Result(false, err);
    }
};

// Forward declarations
class Transaction;
class Block;
class BlockHeader;
class Blockchain;
class Wallet;
class UTXOSet;
class MemPool;
class Peer;
class NetworkManager;

// Smart pointer aliases
template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

// Helper functions
inline bool MoneyRange(Amount amount) {
    return (amount >= 0 && amount <= MAX_MONEY);
}

inline std::string FormatAmount(Amount amount) {
    double dnt = static_cast<double>(amount) / COIN;
    return std::to_string(dnt) + " DNT";
}

} // namespace dinari

#endif // DINARI_TYPES_H
