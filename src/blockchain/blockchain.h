#ifndef DINARI_BLOCKCHAIN_BLOCKCHAIN_H
#define DINARI_BLOCKCHAIN_BLOCKCHAIN_H

#include "dinari/types.h"
#include "block.h"
#include "core/utxo.h"
#include "core/mempool.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace dinari {

/**
 * @brief Main blockchain class
 *
 * Manages the blockchain state, including:
 * - Block storage and indexing
 * - Chain selection (heaviest/longest chain)
 * - Block validation and acceptance
 * - Chain reorganization
 * - UTXO set management
 * - Integration with mempool
 */
class Blockchain {
public:
    Blockchain();
    ~Blockchain();

    /**
     * @brief Initialize blockchain with genesis block
     *
     * @param genesisBlock Genesis block
     * @return true if initialized successfully
     */
    bool Initialize(const Block& genesisBlock);

    /**
     * @brief Process and add new block to chain
     *
     * @param block Block to add
     * @return true if accepted
     */
    bool AcceptBlock(const Block& block);

    /**
     * @brief Get block by hash
     *
     * @param hash Block hash
     * @return Pointer to block (nullptr if not found)
     */
    const Block* GetBlock(const Hash256& hash) const;

    /**
     * @brief Get block index by hash
     *
     * @param hash Block hash
     * @return Pointer to block index (nullptr if not found)
     */
    const BlockIndex* GetBlockIndex(const Hash256& hash) const;

    /**
     * @brief Get block index by height (on main chain)
     *
     * @param height Block height
     * @return Pointer to block index (nullptr if not found)
     */
    const BlockIndex* GetBlockIndex(BlockHeight height) const;

    /**
     * @brief Get best (tip) block of main chain
     *
     * @return Pointer to best block index
     */
    const BlockIndex* GetBestBlock() const { return bestBlock; }

    /**
     * @brief Get current blockchain height
     *
     * @return Height of best block
     */
    BlockHeight GetHeight() const;

    /**
     * @brief Get total chain work
     *
     * @return Total work from genesis to tip
     */
    uint256_t GetChainWork() const;

    /**
     * @brief Check if block exists in chain
     *
     * @param hash Block hash
     * @return true if exists
     */
    bool HasBlock(const Hash256& hash) const;

    /**
     * @brief Check if block is on main chain
     *
     * @param hash Block hash
     * @return true if on main chain
     */
    bool IsOnMainChain(const Hash256& hash) const;

    /**
     * @brief Get blocks in height range
     *
     * @param startHeight Start height (inclusive)
     * @param endHeight End height (inclusive)
     * @return Vector of block hashes
     */
    std::vector<Hash256> GetBlocksInRange(BlockHeight startHeight,
                                          BlockHeight endHeight) const;

    /**
     * @brief Get UTXO set
     *
     * @return Reference to UTXO set
     */
    const UTXOSet& GetUTXOSet() const { return utxos; }
    UTXOSet& GetUTXOSet() { return utxos; }

    /**
     * @brief Get mempool
     *
     * @return Reference to mempool
     */
    const MemPool& GetMemPool() const { return mempool; }
    MemPool& GetMemPool() { return mempool; }

    /**
     * @brief Find fork point between two blocks
     *
     * @param block1 First block
     * @param block2 Second block
     * @return Fork point block index
     */
    const BlockIndex* FindFork(const BlockIndex* block1,
                               const BlockIndex* block2) const;

    /**
     * @brief Get blockchain statistics
     */
    struct Stats {
        BlockHeight height;
        size_t totalBlocks;
        size_t orphanBlocks;
        uint256_t totalWork;
        Hash256 bestBlockHash;
        Amount totalSupply;
        size_t utxoCount;
        size_t mempoolSize;
    };

    Stats GetStats() const;

    /**
     * @brief Validate entire blockchain
     *
     * @return true if valid
     */
    bool ValidateChain() const;

    /**
     * @brief Get block locator (for sync)
     *
     * @param startBlock Starting block
     * @return Vector of block hashes for locator
     */
    std::vector<Hash256> GetBlockLocator(const BlockIndex* startBlock = nullptr) const;

    /**
     * @brief Find common ancestor
     *
     * @param locator Block locator from peer
     * @return Common ancestor block
     */
    const BlockIndex* FindCommonAncestor(const std::vector<Hash256>& locator) const;

private:
    // Block storage (hash -> block)
    std::unordered_map<Hash256, SharedPtr<Block>> blocks;

    // Block index storage (hash -> index)
    std::unordered_map<Hash256, UniquePtr<BlockIndex>> blockIndices;

    // Height index (height -> hash) for main chain
    std::map<BlockHeight, Hash256> heightIndex;

    // Orphan blocks (blocks without parent)
    std::unordered_map<Hash256, SharedPtr<Block>> orphanBlocks;

    // Best block (tip of main chain)
    BlockIndex* bestBlock;

    // Genesis block
    BlockIndex* genesisBlock;

    // UTXO set
    UTXOSet utxos;

    // MemPool
    MemPool mempool;

    // Thread safety
    mutable std::mutex mutex;

    // Internal methods

    /**
     * @brief Validate and connect block
     *
     * @param block Block to connect
     * @param blockIndex Block index
     * @return true if connected
     */
    bool ConnectBlock(const Block& block, BlockIndex* blockIndex);

    /**
     * @brief Disconnect block from chain
     *
     * @param blockIndex Block to disconnect
     * @return true if disconnected
     */
    bool DisconnectBlock(BlockIndex* blockIndex);

    /**
     * @brief Set best chain to new tip
     *
     * @param newTip New best block
     * @return true if successful
     */
    bool SetBestChain(BlockIndex* newTip);

    /**
     * @brief Reorganize chain to new best chain
     *
     * @param newTip New best block
     * @return true if successful
     */
    bool Reorganize(BlockIndex* newTip);

    /**
     * @brief Find path between two blocks
     *
     * @param from Starting block
     * @param to Ending block
     * @return Vector of blocks in path
     */
    std::vector<BlockIndex*> FindPath(BlockIndex* from, BlockIndex* to) const;

    /**
     * @brief Update main chain flags
     *
     * @param tip New tip block
     */
    void UpdateMainChain(BlockIndex* tip);

    /**
     * @brief Check if block index is better than current best
     *
     * @param blockIndex Block to check
     * @return true if better
     */
    bool IsBetterChain(const BlockIndex* blockIndex) const;

    /**
     * @brief Add block to orphan pool
     *
     * @param block Block to add
     */
    void AddOrphan(const SharedPtr<Block>& block);

    /**
     * @brief Process orphan blocks
     *
     * @param parentHash Parent block hash
     */
    void ProcessOrphans(const Hash256& parentHash);

    /**
     * @brief Create block index
     *
     * @param block Block
     * @param height Height
     * @return Block index
     */
    BlockIndex* CreateBlockIndex(const SharedPtr<Block>& block, BlockHeight height);

    /**
     * @brief Update UTXO set after block connection
     *
     * @param block Block
     * @param height Height
     * @return true if successful
     */
    bool UpdateUTXOs(const Block& block, BlockHeight height);

    /**
     * @brief Revert UTXO changes after block disconnection
     *
     * @param block Block
     * @return true if successful
     */
    bool RevertUTXOs(const Block& block);

    /**
     * @brief Remove transactions from mempool (after block confirmation)
     *
     * @param block Block containing transactions
     */
    void RemoveFromMempool(const Block& block);

    /**
     * @brief Calculate total supply up to height
     *
     * @param height Block height
     * @return Total supply
     */
    Amount CalculateTotalSupply(BlockHeight height) const;
};

} // namespace dinari

#endif // DINARI_BLOCKCHAIN_BLOCKCHAIN_H
