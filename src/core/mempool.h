#ifndef DINARI_CORE_MEMPOOL_H
#define DINARI_CORE_MEMPOOL_H

#include "dinari/types.h"
#include "transaction.h"
#include "util/time.h"
#include <map>
#include <set>
#include <mutex>
#include <memory>

namespace dinari {

/**
 * @brief Memory pool entry
 *
 * Contains a transaction and metadata about when it was added and its priority.
 */
class MemPoolEntry {
public:
    Transaction tx;
    Timestamp timeAdded;
    Amount fee;
    size_t size;
    double priority;

    MemPoolEntry() : timeAdded(0), fee(0), size(0), priority(0.0) {}

    MemPoolEntry(const Transaction& transaction, Amount txFee, double txPriority)
        : tx(transaction)
        , timeAdded(Time::GetCurrentTime())
        , fee(txFee)
        , size(transaction.GetSize())
        , priority(txPriority) {}

    // Get fee rate (fee per byte)
    Amount GetFeeRate() const {
        return size > 0 ? fee / size : 0;
    }

    // Get transaction hash
    Hash256 GetHash() const {
        return tx.GetHash();
    }
};

/**
 * @brief Memory Pool (MemPool)
 *
 * Stores unconfirmed transactions waiting to be included in blocks.
 * Thread-safe for concurrent access.
 */
class MemPool {
public:
    MemPool();
    ~MemPool();

    /**
     * @brief Add transaction to mempool
     *
     * @param tx Transaction to add
     * @param utxos UTXO set for validation
     * @param currentHeight Current blockchain height
     * @return true if added successfully
     */
    bool AddTransaction(const Transaction& tx, const class UTXOSet& utxos,
                       BlockHeight currentHeight);

    /**
     * @brief Remove transaction from mempool
     *
     * @param txHash Transaction hash
     * @return true if removed
     */
    bool RemoveTransaction(const Hash256& txHash);

    /**
     * @brief Remove transactions (e.g., after block confirmation)
     *
     * @param txHashes List of transaction hashes to remove
     */
    void RemoveTransactions(const std::vector<Hash256>& txHashes);

    /**
     * @brief Check if transaction exists in mempool
     *
     * @param txHash Transaction hash
     * @return true if exists
     */
    bool HasTransaction(const Hash256& txHash) const;

    /**
     * @brief Get transaction from mempool
     *
     * @param txHash Transaction hash
     * @return Pointer to transaction (nullptr if not found)
     */
    const Transaction* GetTransaction(const Hash256& txHash) const;

    /**
     * @brief Get mempool entry
     *
     * @param txHash Transaction hash
     * @return Pointer to entry (nullptr if not found)
     */
    const MemPoolEntry* GetEntry(const Hash256& txHash) const;

    /**
     * @brief Get all transactions
     *
     * @return Vector of all transactions in mempool
     */
    std::vector<Transaction> GetAllTransactions() const;

    /**
     * @brief Get transactions for mining (ordered by priority/fee)
     *
     * @param maxSize Maximum total size
     * @param maxCount Maximum number of transactions
     * @return Vector of transactions for block template
     */
    std::vector<Transaction> GetTransactionsForMining(size_t maxSize = MAX_BLOCK_SIZE,
                                                     size_t maxCount = 10000) const;

    /**
     * @brief Get number of transactions in mempool
     *
     * @return Transaction count
     */
    size_t Size() const;

    /**
     * @brief Get total size of all transactions
     *
     * @return Total size in bytes
     */
    size_t GetTotalSize() const;

    /**
     * @brief Clear all transactions
     */
    void Clear();

    /**
     * @brief Remove low-fee transactions to make space
     *
     * @param targetSize Target size after trimming
     */
    void TrimToSize(size_t targetSize);

    /**
     * @brief Check if mempool is full
     *
     * @return true if at capacity
     */
    bool IsFull() const;

    /**
     * @brief Get mempool statistics
     */
    struct Stats {
        size_t transactionCount;
        size_t totalSize;
        Amount totalFees;
        Amount minFeeRate;
        Amount maxFeeRate;
        Amount avgFeeRate;
    };

    Stats GetStats() const;

    /**
     * @brief Check if transaction conflicts with any in mempool
     *
     * @param tx Transaction to check
     * @return true if conflicts (double-spend)
     */
    bool CheckForConflicts(const Transaction& tx) const;

    /**
     * @brief Remove conflicting transactions
     *
     * @param tx Transaction that conflicts
     */
    void RemoveConflicts(const Transaction& tx);

    /**
     * @brief Validate transaction for mempool acceptance
     *
     * @param tx Transaction to validate
     * @param utxos UTXO set
     * @param currentHeight Current height
     * @param error Output error message
     * @return true if valid
     */
    bool ValidateForMempool(const Transaction& tx, const UTXOSet& utxos,
                           BlockHeight currentHeight, std::string& error) const;

private:
    // Transaction storage (txHash -> entry)
    std::map<Hash256, MemPoolEntry> transactions;

    // Index for inputs (outpoint -> spending tx hash)
    std::map<OutPoint, Hash256> inputIndex;

    // Fee rate index (for quick retrieval of high-fee transactions)
    std::multimap<Amount, Hash256> feeIndex;

    // Thread safety
    mutable std::mutex mutex;

    // Statistics
    size_t totalSize;
    Amount totalFees;

    // Helper methods
    void AddToIndices(const Hash256& txHash, const MemPoolEntry& entry);
    void RemoveFromIndices(const Hash256& txHash, const MemPoolEntry& entry);
    bool CheckTransactionStandard(const Transaction& tx) const;
};

} // namespace dinari

#endif // DINARI_CORE_MEMPOOL_H
