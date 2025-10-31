#ifndef DINARI_STORAGE_TXINDEX_H
#define DINARI_STORAGE_TXINDEX_H

#include "database.h"
#include "dinari/types.h"
#include "core/transaction.h"
#include "core/utxo.h"
#include <memory>
#include <optional>

namespace dinari {

/**
 * @brief Transaction location in blockchain
 */
struct TxLocation {
    BlockHeight height;
    uint32_t txIndex;  // Index within block

    TxLocation() : height(0), txIndex(0) {}
    TxLocation(BlockHeight h, uint32_t idx) : height(h), txIndex(idx) {}
};

/**
 * @brief Persistent transaction index and UTXO set
 *
 * Stores:
 * - Transaction ID → Location (height, tx index)
 * - UTXO set: OutPoint → TxOut
 * - Address → UTXOs (for wallet queries)
 */
class TxIndex {
public:
    TxIndex() = default;

    /**
     * @brief Open transaction index
     * @param dataDir Data directory path
     */
    bool Open(const std::string& dataDir);

    /**
     * @brief Close transaction index
     */
    void Close();

    /**
     * @brief Check if open
     */
    bool IsOpen() const { return db && db->IsOpen(); }

    /**
     * @brief Index transaction
     * @param tx Transaction to index
     * @param height Block height
     * @param txIndex Transaction index in block
     */
    bool IndexTransaction(const Transaction& tx, BlockHeight height, uint32_t txIndex);

    /**
     * @brief Get transaction location
     */
    std::optional<TxLocation> GetTxLocation(const Hash256& txid) const;

    /**
     * @brief Add UTXO to set
     */
    bool AddUTXO(const OutPoint& outpoint, const TxOut& output);

    /**
     * @brief Remove UTXO from set (spent)
     */
    bool RemoveUTXO(const OutPoint& outpoint);

    /**
     * @brief Get UTXO
     */
    std::optional<TxOut> GetUTXO(const OutPoint& outpoint) const;

    /**
     * @brief Check if UTXO exists (unspent)
     */
    bool HasUTXO(const OutPoint& outpoint) const;

    /**
     * @brief Get all UTXOs for an address
     */
    std::vector<std::pair<OutPoint, TxOut>> GetUTXOsForAddress(const Address& address) const;

    /**
     * @brief Get UTXO set size
     */
    size_t GetUTXOSetSize() const;

    /**
     * @brief Batch update UTXO set (for block processing)
     */
    struct UTXOBatch {
        std::vector<std::pair<OutPoint, TxOut>> additions;
        std::vector<OutPoint> removals;
    };

    /**
     * @brief Apply UTXO batch atomically
     */
    bool ApplyUTXOBatch(const UTXOBatch& batch);

    /**
     * @brief Remove transaction from index (for reorg)
     */
    bool RemoveTransaction(const Hash256& txid);

    /**
     * @brief Get database statistics
     */
    std::string GetStats() const;

    /**
     * @brief Compact database
     */
    void Compact();

private:
    std::unique_ptr<Database> db;

    // Key prefixes
    static constexpr char PREFIX_TX_LOCATION = 't';  // t<txid> → location
    static constexpr char PREFIX_UTXO = 'u';         // u<outpoint> → txout
    static constexpr char PREFIX_ADDR_UTXO = 'a';    // a<address><outpoint> → txout
    static constexpr char PREFIX_UTXO_COUNT = 'c';   // c → count

    bytes MakeTxLocationKey(const Hash256& txid) const;
    bytes MakeUTXOKey(const OutPoint& outpoint) const;
    bytes MakeAddressUTXOKey(const Address& address, const OutPoint& outpoint) const;
    bytes MakeUTXOCountKey() const;

    bool UpdateUTXOCount(int delta);
};

} // namespace dinari

#endif // DINARI_STORAGE_TXINDEX_H
