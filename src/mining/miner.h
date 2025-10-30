#ifndef DINARI_MINING_MINER_H
#define DINARI_MINING_MINER_H

#include "dinari/types.h"
#include "blockchain/block.h"
#include "blockchain/blockchain.h"
#include "core/mempool.h"
#include "wallet/address.h"
#include <atomic>
#include <thread>
#include <functional>

namespace dinari {

/**
 * @brief Mining configuration
 */
struct MiningConfig {
    Address coinbaseAddress;
    uint32_t numThreads;
    uint64_t maxNonce;

    MiningConfig()
        : numThreads(1)
        , maxNonce(0xFFFFFFFF) {}
};

/**
 * @brief Mining statistics
 */
struct MiningStats {
    uint64_t blocksFound;
    uint64_t hashesComputed;
    Timestamp startTime;
    double hashrate;  // Hashes per second

    MiningStats()
        : blocksFound(0)
        , hashesComputed(0)
        , startTime(0)
        , hashrate(0.0) {}
};

/**
 * @brief Block miner
 *
 * Implements proof-of-work mining:
 * - Creates candidate blocks from mempool
 * - Performs hash-based mining
 * - Multi-threaded mining support
 * - Hashrate calculation
 */
class Miner {
public:
    /**
     * @brief Callback when block is found
     */
    using BlockFoundCallback = std::function<void(const Block&)>;

    Miner(Blockchain& chain, const MiningConfig& config);
    ~Miner();

    /**
     * @brief Start mining
     */
    bool Start();

    /**
     * @brief Stop mining
     */
    void Stop();

    /**
     * @brief Check if mining
     */
    bool IsMining() const { return mining.load(); }

    /**
     * @brief Get mining statistics
     */
    MiningStats GetStats() const;

    /**
     * @brief Set block found callback
     */
    void SetBlockFoundCallback(BlockFoundCallback callback);

    /**
     * @brief Mine a single block (blocking)
     */
    bool MineBlock(Block& block, uint64_t maxIterations = 0);

private:
    Blockchain& blockchain;
    MiningConfig config;

    // Mining state
    std::atomic<bool> mining;
    std::atomic<bool> shouldStop;
    std::vector<std::thread> minerThreads;

    // Statistics
    std::atomic<uint64_t> blocksFound;
    std::atomic<uint64_t> hashesComputed;
    Timestamp startTime;

    // Callback
    BlockFoundCallback blockFoundCallback;
    mutable std::mutex callbackMutex;

    // Mining methods
    void MinerThreadFunc(uint32_t threadId);
    bool CreateCandidateBlock(Block& block);
    bool CheckProofOfWork(const Hash256& hash, uint32_t bits);
    uint32_t GetNextDifficulty();

    void OnBlockFound(const Block& block);
};

/**
 * @brief CPU mining helper
 */
class CPUMiner {
public:
    /**
     * @brief Mine a block with CPU
     *
     * @param block Block to mine (header will be modified)
     * @param startNonce Starting nonce value
     * @param endNonce Ending nonce value
     * @param shouldStop Atomic flag to stop mining
     * @param hashesComputed Counter for hashes computed
     * @return true if solution found
     */
    static bool Mine(Block& block,
                    Nonce startNonce,
                    Nonce endNonce,
                    const std::atomic<bool>& shouldStop,
                    std::atomic<uint64_t>& hashesComputed);

    /**
     * @brief Calculate target from bits
     */
    static Hash256 BitsToTarget(uint32_t bits);

    /**
     * @brief Check if hash meets target
     */
    static bool CheckTarget(const Hash256& hash, const Hash256& target);
};

} // namespace dinari

#endif // DINARI_MINING_MINER_H
