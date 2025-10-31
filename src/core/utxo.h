#ifndef DINARI_CORE_UTXO_H
#define DINARI_CORE_UTXO_H

#include "dinari/types.h"
#include "transaction.h"
#include <map>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace dinari {

/**
 * @brief UTXO (Unspent Transaction Output) Entry
 *
 * Stores information about an unspent output including the output itself
 * and metadata about when it was created.
 */
class UTXOEntry {
public:
    TxOut output;           // The actual output
    BlockHeight height;     // Height when this UTXO was created
    bool isCoinbase;        // Whether this came from a coinbase transaction

    UTXOEntry() : height(0), isCoinbase(false) {}
    UTXOEntry(const TxOut& out, BlockHeight h, bool coinbase)
        : output(out), height(h), isCoinbase(coinbase) {}

    // Check if UTXO is mature (coinbase outputs require 100 confirmations)
    bool IsMature(BlockHeight currentHeight) const;

    // Check if UTXO is spendable at given height
    bool IsSpendable(BlockHeight currentHeight) const;

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);
};

/**
 * @brief UTXO Set
 *
 * Maintains the set of all unspent transaction outputs in the blockchain.
 * This is the core data structure that determines which outputs can be spent.
 *
 * Thread-safe for concurrent read/write access.
 */
class UTXOSet {
public:
    UTXOSet();
    ~UTXOSet();

    // Add a UTXO
    void AddUTXO(const OutPoint& outpoint, const UTXOEntry& entry);
    void AddUTXO(const OutPoint& outpoint, const TxOut& output,
                BlockHeight height, bool isCoinbase);

    // Remove a UTXO (when spent)
    bool RemoveUTXO(const OutPoint& outpoint);

    // Check if UTXO exists
    bool HasUTXO(const OutPoint& outpoint) const;

    // Get UTXO (returns nullptr if not found)
    const TxOut* GetUTXO(const OutPoint& outpoint) const;
    const UTXOEntry* GetUTXOEntry(const OutPoint& outpoint) const;

    // Get UTXO height
    BlockHeight GetUTXOHeight(const OutPoint& outpoint) const;

    // Apply transaction to UTXO set (add outputs, remove inputs)
    bool ApplyTransaction(const Transaction& tx, BlockHeight height);

    // Revert transaction from UTXO set (remove outputs, restore inputs)
    bool RevertTransaction(const Transaction& tx,
                          const std::map<OutPoint, UTXOEntry>& previousUTXOs);

    // Get all UTXOs for an address (requires address index)
    std::vector<std::pair<OutPoint, UTXOEntry>> GetUTXOsForAddress(
        const Hash160& addressHash) const;

    // Get total number of UTXOs
    size_t GetSize() const;

    // Get total value of all UTXOs
    Amount GetTotalValue() const;

    // Clear all UTXOs
    void Clear();

    // Flush to disk (if using persistent storage)
    bool Flush();

    // Load from disk
    bool Load();

    // Get statistics
    struct Stats {
        size_t totalUTXOs;
        Amount totalValue;
        size_t coinbaseUTXOs;
        size_t regularUTXOs;
        size_t matureUTXOs;
        size_t immatureUTXOs;
    };
    Stats GetStats(BlockHeight currentHeight) const;

    // Memory management
    void Prune(BlockHeight keepDepth);  // Remove old spent UTXOs from cache

    // Validation
    bool ValidateTransaction(const Transaction& tx, BlockHeight currentHeight) const;

private:
    // UTXO storage (in-memory, will be backed by database in production)
    std::unordered_map<OutPoint, UTXOEntry> utxos;

    // Address index (optional, for wallet queries)
    std::unordered_map<Hash160, std::vector<OutPoint>> addressIndex;

    // Thread safety
    mutable std::mutex mutex;

    // Helper methods
    void BuildAddressIndex();
    std::optional<Hash160> ExtractAddressFromScript(const bytes& script) const;
};

/**
 * @brief UTXO Cache
 *
 * Provides a caching layer on top of the UTXO set for better performance.
 * Caches recently accessed UTXOs in memory.
 */
class UTXOCache {
public:
    UTXOCache(UTXOSet& baseSet, size_t maxCacheSize = 10000);

    // Same interface as UTXOSet, but with caching
    const TxOut* GetUTXO(const OutPoint& outpoint);
    bool HasUTXO(const OutPoint& outpoint);

    // Flush cache to base set
    void Flush();

    // Clear cache
    void Clear();

    // Get cache statistics
    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t size;
        double hitRate;
    };
    CacheStats GetStats() const;

private:
    UTXOSet& baseSet;
    std::unordered_map<OutPoint, std::unique_ptr<UTXOEntry>> cache;
    size_t maxCacheSize;
    size_t hits;
    size_t misses;
    mutable std::mutex mutex;
};

/**
 * @brief Coin Selection Algorithm
 *
 * Selects UTXOs to use as inputs for a transaction.
 * Implements various coin selection strategies.
 */
class CoinSelector {
public:
    enum class Strategy {
        LARGEST_FIRST,      // Select largest coins first
        SMALLEST_FIRST,     // Select smallest coins first
        RANDOM,             // Random selection
        BRANCH_AND_BOUND    // Optimal selection (Branch and Bound algorithm)
    };

    struct SelectionResult {
        std::vector<OutPoint> selected;
        Amount totalValue;
        Amount fee;
        Amount change;
        bool success;
        std::string error;
    };

    CoinSelector(const UTXOSet& utxos);

    // Select coins for a given target amount
    SelectionResult SelectCoins(Amount targetAmount, Amount feeRate,
                               const std::vector<OutPoint>& availableCoins,
                               Strategy strategy = Strategy::BRANCH_AND_BOUND);

    // Calculate fee for transaction
    Amount CalculateFee(size_t txSize, Amount feeRate);

    // Estimate transaction size
    size_t EstimateTransactionSize(size_t numInputs, size_t numOutputs);

private:
    const UTXOSet& utxos;

    // Selection strategies
    SelectionResult SelectLargestFirst(Amount target, Amount feeRate,
                                      std::vector<OutPoint> coins);
    SelectionResult SelectSmallestFirst(Amount target, Amount feeRate,
                                       std::vector<OutPoint> coins);
    SelectionResult SelectRandom(Amount target, Amount feeRate,
                                std::vector<OutPoint> coins);
    SelectionResult SelectBranchAndBound(Amount target, Amount feeRate,
                                        std::vector<OutPoint> coins);
};

} // namespace dinari

#endif // DINARI_CORE_UTXO_H
