#include "mempool.h"
#include "utxo.h"
#include "consensus/validation.h"
#include "util/logger.h"
#include "util/time.h"
#include "dinari/constants.h"
#include <algorithm>

namespace dinari {

MemPool::MemPool() : totalSize(0), totalFees(0) {
}

MemPool::~MemPool() {
}

bool MemPool::AddTransaction(const Transaction& tx, const UTXOSet& utxos,
                             BlockHeight currentHeight) {
    std::lock_guard<std::mutex> lock(mutex);

    Hash256 txHash = tx.GetHash();

    // Check if already in mempool
    if (HasTransaction(txHash)) {
        LOG_DEBUG("MemPool", "Transaction already in mempool: " + crypto::Hash::ToHex(txHash));
        return false;
    }

    // Validate transaction
    std::string error;
    if (!ValidateForMempool(tx, utxos, currentHeight, error)) {
        LOG_WARNING("MemPool", "Transaction validation failed: " + error);
        return false;
    }

    // Check for conflicts (double-spend)
    if (CheckForConflicts(tx)) {
        LOG_WARNING("MemPool", "Transaction conflicts with mempool");
        return false;
    }

    // Calculate fee and priority
    Amount fee = tx.GetFee(utxos);
    double priority = tx.GetPriority(utxos, currentHeight);

    // Check if mempool is full
    if (IsFull()) {
        // Check if this transaction has higher fee rate than lowest
        Amount feeRate = fee / tx.GetSize();
        if (!feeIndex.empty()) {
            Amount lowestFeeRate = feeIndex.begin()->first;
            if (feeRate <= lowestFeeRate) {
                LOG_DEBUG("MemPool", "Mempool full, transaction fee too low");
                return false;
            }
            // Remove lowest fee transaction
            TrimToSize(MAX_MEMPOOL_SIZE - tx.GetSize());
        }
    }

    // Create entry
    MemPoolEntry entry(tx, fee, priority);

    // Add to storage
    transactions[txHash] = entry;

    // Update indices
    AddToIndices(txHash, entry);

    // Update statistics
    totalSize += entry.size;
    totalFees += entry.fee;

    LOG_INFO("MemPool", "Added transaction: " + crypto::Hash::ToHex(txHash).substr(0, 16) + "...");
    LOG_DEBUG("MemPool", "  Fee: " + FormatAmount(fee));
    LOG_DEBUG("MemPool", "  Size: " + std::to_string(entry.size) + " bytes");
    LOG_DEBUG("MemPool", "  MemPool size: " + std::to_string(Size()) + " transactions");

    return true;
}

bool MemPool::RemoveTransaction(const Hash256& txHash) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = transactions.find(txHash);
    if (it == transactions.end()) {
        return false;
    }

    const MemPoolEntry& entry = it->second;

    // Remove from indices
    RemoveFromIndices(txHash, entry);

    // Update statistics
    totalSize -= entry.size;
    totalFees -= entry.fee;

    // Remove from storage
    transactions.erase(it);

    LOG_DEBUG("MemPool", "Removed transaction: " + crypto::Hash::ToHex(txHash).substr(0, 16) + "...");

    return true;
}

void MemPool::RemoveTransactions(const std::vector<Hash256>& txHashes) {
    for (const auto& txHash : txHashes) {
        RemoveTransaction(txHash);
    }
}

bool MemPool::HasTransaction(const Hash256& txHash) const {
    std::lock_guard<std::mutex> lock(mutex);
    return transactions.find(txHash) != transactions.end();
}

const Transaction* MemPool::GetTransaction(const Hash256& txHash) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = transactions.find(txHash);
    if (it == transactions.end()) {
        return nullptr;
    }

    return &it->second.tx;
}

const MemPoolEntry* MemPool::GetEntry(const Hash256& txHash) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = transactions.find(txHash);
    if (it == transactions.end()) {
        return nullptr;
    }

    return &it->second;
}

std::vector<Transaction> MemPool::GetAllTransactions() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Transaction> result;
    result.reserve(transactions.size());

    for (const auto& [hash, entry] : transactions) {
        result.push_back(entry.tx);
    }

    return result;
}

std::vector<Transaction> MemPool::GetTransactionsForMining(size_t maxSize,
                                                          size_t maxCount) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Transaction> result;
    size_t currentSize = 0;

    // Sort by fee rate (descending)
    std::vector<std::pair<Amount, Hash256>> sorted;
    for (const auto& [hash, entry] : transactions) {
        sorted.emplace_back(entry.GetFeeRate(), hash);
    }

    std::sort(sorted.begin(), sorted.end(),
             [](const auto& a, const auto& b) { return a.first > b.first; });

    // Select transactions
    for (const auto& [feeRate, txHash] : sorted) {
        if (result.size() >= maxCount) {
            break;
        }

        auto it = transactions.find(txHash);
        if (it == transactions.end()) continue;

        const MemPoolEntry& entry = it->second;

        if (currentSize + entry.size > maxSize) {
            continue;  // Skip if doesn't fit
        }

        result.push_back(entry.tx);
        currentSize += entry.size;
    }

    return result;
}

size_t MemPool::Size() const {
    std::lock_guard<std::mutex> lock(mutex);
    return transactions.size();
}

size_t MemPool::GetTotalSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return totalSize;
}

void MemPool::Clear() {
    std::lock_guard<std::mutex> lock(mutex);

    transactions.clear();
    inputIndex.clear();
    feeIndex.clear();
    totalSize = 0;
    totalFees = 0;

    LOG_INFO("MemPool", "Cleared mempool");
}

void MemPool::TrimToSize(size_t targetSize) {
    std::lock_guard<std::mutex> lock(mutex);

    while (totalSize > targetSize && !feeIndex.empty()) {
        // Remove transaction with lowest fee rate
        auto it = feeIndex.begin();
        Hash256 txHash = it->second;

        auto txIt = transactions.find(txHash);
        if (txIt != transactions.end()) {
            const MemPoolEntry& entry = txIt->second;

            // Remove from indices
            RemoveFromIndices(txHash, entry);

            // Update statistics
            totalSize -= entry.size;
            totalFees -= entry.fee;

            // Remove from storage
            transactions.erase(txIt);
        }

        feeIndex.erase(it);
    }
}

bool MemPool::IsFull() const {
    std::lock_guard<std::mutex> lock(mutex);
    return totalSize >= MAX_MEMPOOL_SIZE;
}

MemPool::Stats MemPool::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    Stats stats;
    stats.transactionCount = transactions.size();
    stats.totalSize = totalSize;
    stats.totalFees = totalFees;
    stats.minFeeRate = 0;
    stats.maxFeeRate = 0;
    stats.avgFeeRate = 0;

    if (!transactions.empty()) {
        Amount minFee = Amount(-1);
        Amount maxFee = 0;
        Amount totalFeeRate = 0;

        for (const auto& [hash, entry] : transactions) {
            Amount feeRate = entry.GetFeeRate();
            minFee = std::min(minFee, feeRate);
            maxFee = std::max(maxFee, feeRate);
            totalFeeRate += feeRate;
        }

        stats.minFeeRate = minFee;
        stats.maxFeeRate = maxFee;
        stats.avgFeeRate = totalFeeRate / transactions.size();
    }

    return stats;
}

bool MemPool::CheckForConflicts(const Transaction& tx) const {
    // Check if any input is already spent by another transaction in mempool
    for (const auto& input : tx.inputs) {
        if (input.IsCoinbase()) continue;

        auto it = inputIndex.find(input.prevOut);
        if (it != inputIndex.end()) {
            // This input is already being spent
            return true;
        }
    }

    return false;
}

void MemPool::RemoveConflicts(const Transaction& tx) {
    std::vector<Hash256> toRemove;

    for (const auto& input : tx.inputs) {
        if (input.IsCoinbase()) continue;

        auto it = inputIndex.find(input.prevOut);
        if (it != inputIndex.end()) {
            toRemove.push_back(it->second);
        }
    }

    for (const auto& txHash : toRemove) {
        RemoveTransaction(txHash);
    }
}

bool MemPool::ValidateForMempool(const Transaction& tx, const UTXOSet& utxos,
                                BlockHeight currentHeight, std::string& error) const {
    // Basic validation
    if (!tx.IsValid()) {
        error = "Transaction failed basic validation";
        return false;
    }

    // Must not be coinbase
    if (tx.IsCoinbase()) {
        error = "Coinbase transaction cannot be in mempool";
        return false;
    }

    // Check if standard
    if (!CheckTransactionStandard(tx)) {
        error = "Non-standard transaction";
        return false;
    }

    // Full consensus checks (including script verification)
    auto consensusResult = ConsensusValidator::ValidateTransaction(tx, currentHeight, utxos, false);
    if (!consensusResult) {
        error = consensusResult.error;
        if (error.empty()) {
            error = "Consensus validation failed";
        }
        return false;
    }

    // Check minimum fee
    Amount fee = tx.GetFee(utxos);
    Amount feeRate = fee / tx.GetSize();

    if (feeRate < MIN_RELAY_TX_FEE) {
        error = "Fee rate too low";
        return false;
    }

    return true;
}

void MemPool::AddToIndices(const Hash256& txHash, const MemPoolEntry& entry) {
    // Add to input index
    for (const auto& input : entry.tx.inputs) {
        if (!input.IsCoinbase()) {
            inputIndex[input.prevOut] = txHash;
        }
    }

    // Add to fee index
    Amount feeRate = entry.GetFeeRate();
    feeIndex.emplace(feeRate, txHash);
}

void MemPool::RemoveFromIndices(const Hash256& txHash, const MemPoolEntry& entry) {
    // Remove from input index
    for (const auto& input : entry.tx.inputs) {
        if (!input.IsCoinbase()) {
            inputIndex.erase(input.prevOut);
        }
    }

    // Remove from fee index
    Amount feeRate = entry.GetFeeRate();
    auto range = feeIndex.equal_range(feeRate);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == txHash) {
            feeIndex.erase(it);
            break;
        }
    }
}

bool MemPool::CheckTransactionStandard(const Transaction& tx) const {
    return tx.IsStandard();
}

} // namespace dinari
