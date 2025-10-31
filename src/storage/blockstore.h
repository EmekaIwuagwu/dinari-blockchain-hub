#ifndef DINARI_STORAGE_BLOCKSTORE_H
#define DINARI_STORAGE_BLOCKSTORE_H

#include "database.h"
#include "dinari/types.h"
#include "blockchain/block.h"
#include <memory>
#include <optional>
#include <boost/multiprecision/cpp_int.hpp>

namespace dinari {

/**
 * @brief Persistent block storage
 *
 * Stores blocks indexed by:
 * - Height → Block
 * - Block hash → Height
 * - Best block hash
 * - Total chain work
 */
class BlockStore {
public:
    BlockStore() = default;

    /**
     * @brief Open block store
     * @param dataDir Data directory path
     */
    bool Open(const std::string& dataDir);

    /**
     * @brief Close block store
     */
    void Close();

    /**
     * @brief Check if open
     */
    bool IsOpen() const { return db && db->IsOpen(); }

    /**
     * @brief Write block to storage
     * @param block Block to store
     * @param height Block height
     */
    bool WriteBlock(const Block& block, BlockHeight height);

    /**
     * @brief Read block by height
     */
    std::optional<Block> ReadBlock(BlockHeight height) const;

    /**
     * @brief Read block by hash
     */
    std::optional<Block> ReadBlockByHash(const Hash256& hash) const;

    /**
     * @brief Get block height by hash
     */
    std::optional<BlockHeight> GetBlockHeight(const Hash256& hash) const;

    /**
     * @brief Check if block exists
     */
    bool HasBlock(const Hash256& hash) const;

    /**
     * @brief Get best block hash
     */
    std::optional<Hash256> GetBestBlockHash() const;

    /**
     * @brief Set best block hash
     */
    bool SetBestBlockHash(const Hash256& hash);

    /**
     * @brief Get chain height
     */
    std::optional<BlockHeight> GetChainHeight() const;

    /**
     * @brief Set chain height
     */
    bool SetChainHeight(BlockHeight height);

    /**
     * @brief Get total chain work
     */
    std::optional<boost::multiprecision::uint256_t> GetTotalWork() const;

    /**
     * @brief Set total chain work
     */
    bool SetTotalWork(const boost::multiprecision::uint256_t& work);

    /**
     * @brief Delete block
     */
    bool DeleteBlock(BlockHeight height);

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
    static constexpr char PREFIX_BLOCK = 'b';      // b<height> → block
    static constexpr char PREFIX_HASH = 'h';       // h<hash> → height
    static constexpr char PREFIX_BEST = 'B';       // B → best block hash
    static constexpr char PREFIX_HEIGHT = 'H';     // H → chain height
    static constexpr char PREFIX_WORK = 'W';       // W → total work

    bytes MakeBlockKey(BlockHeight height) const;
    bytes MakeHashKey(const Hash256& hash) const;
    bytes MakeBestKey() const;
    bytes MakeHeightKey() const;
    bytes MakeWorkKey() const;
};

} // namespace dinari

#endif // DINARI_STORAGE_BLOCKSTORE_H
