#ifndef DINARI_CONSENSUS_DIFFICULTY_H
#define DINARI_CONSENSUS_DIFFICULTY_H

#include "dinari/types.h"
#include "blockchain/block.h"
#include <vector>

namespace dinari {

/**
 * @brief Difficulty adjustment for Proof-of-Work
 *
 * Implements Bitcoin-style difficulty adjustment algorithm.
 * Adjusts every 2016 blocks to maintain target block time of 10 minutes.
 */

class DifficultyAdjuster {
public:
    /**
     * @brief Calculate next difficulty target
     *
     * @param lastBlock Last block before adjustment
     * @param blockchain Reference to blockchain for historical data
     * @return New difficulty target (compact format)
     */
    static uint32_t GetNextWorkRequired(const BlockIndex* lastBlock,
                                       const class Blockchain& blockchain);

    /**
     * @brief Check if it's time for difficulty adjustment
     *
     * @param height Block height
     * @return true if adjustment should occur
     */
    static bool ShouldAdjustDifficulty(BlockHeight height);

    /**
     * @brief Calculate actual timespan from blocks
     *
     * @param firstBlock First block in adjustment period
     * @param lastBlock Last block in adjustment period
     * @return Actual timespan in seconds
     */
    static Timestamp CalculateActualTimespan(const BlockIndex* firstBlock,
                                            const BlockIndex* lastBlock);

    /**
     * @brief Limit timespan to prevent extreme adjustments
     *
     * @param actualTimespan Actual timespan
     * @param targetTimespan Target timespan
     * @return Limited timespan
     */
    static Timestamp LimitTimespan(Timestamp actualTimespan, Timestamp targetTimespan);

    /**
     * @brief Get difficulty from compact bits
     *
     * @param bits Compact difficulty target
     * @return Difficulty value (higher = harder)
     */
    static double GetDifficulty(uint32_t bits);

    /**
     * @brief Get target timespan for adjustment period
     *
     * @return Target timespan in seconds (2016 blocks * 600 seconds)
     */
    static constexpr Timestamp GetTargetTimespan() {
        return DIFFICULTY_ADJUSTMENT_INTERVAL * TARGET_BLOCK_TIME;
    }

    /**
     * @brief Get minimum difficulty (testnet)
     *
     * @return Minimum difficulty bits
     */
    static constexpr uint32_t GetMinimumDifficulty() {
        return 0x207fffff;  // Very easy difficulty for testing
    }

    /**
     * @brief Get initial difficulty (mainnet)
     *
     * @return Initial difficulty bits
     */
    static constexpr uint32_t GetInitialDifficulty() {
        return INITIAL_DIFFICULTY;
    }

    /**
     * @brief Check if difficulty bits are valid
     *
     * @param bits Difficulty bits to check
     * @return true if valid
     */
    static bool IsValidDifficulty(uint32_t bits);

private:
    // Prevent instantiation
    DifficultyAdjuster() = delete;

    // Helper to get block at specific height
    static const BlockIndex* GetBlockAtHeight(const Blockchain& blockchain,
                                             BlockHeight height);
};

/**
 * @brief Difficulty calculator for block validation
 */
class DifficultyCalculator {
public:
    /**
     * @brief Verify block difficulty is correct
     *
     * @param block Block to verify
     * @param prevBlock Previous block
     * @param blockchain Blockchain reference
     * @return true if difficulty is correct
     */
    static bool VerifyBlockDifficulty(const Block& block,
                                     const BlockIndex* prevBlock,
                                     const Blockchain& blockchain);

    /**
     * @brief Get expected difficulty for next block
     *
     * @param prevBlock Previous block
     * @param blockchain Blockchain reference
     * @return Expected difficulty bits
     */
    static uint32_t GetExpectedDifficulty(const BlockIndex* prevBlock,
                                         const Blockchain& blockchain);

    /**
     * @brief Check if block meets difficulty target
     *
     * @param block Block to check
     * @return true if block hash meets target
     */
    static bool CheckBlockDifficulty(const Block& block);
};

} // namespace dinari

#endif // DINARI_CONSENSUS_DIFFICULTY_H
