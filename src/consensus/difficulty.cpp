#include "difficulty.h"
#include "blockchain/blockchain.h"
#include "crypto/hash.h"
#include "util/logger.h"
#include "dinari/constants.h"
#include <algorithm>
#include <cmath>

namespace dinari {

bool DifficultyAdjuster::ShouldAdjustDifficulty(BlockHeight height) {
    // Adjust every DIFFICULTY_ADJUSTMENT_INTERVAL blocks
    return (height % DIFFICULTY_ADJUSTMENT_INTERVAL) == 0;
}

uint32_t DifficultyAdjuster::GetNextWorkRequired(const BlockIndex* lastBlock,
                                                 const Blockchain& blockchain) {
    // Genesis block or before first adjustment
    if (!lastBlock || lastBlock->height < DIFFICULTY_ADJUSTMENT_INTERVAL) {
        return GetInitialDifficulty();
    }

    // Only adjust at interval
    if (!ShouldAdjustDifficulty(lastBlock->height + 1)) {
        return lastBlock->GetBits();
    }

    // Find first block of adjustment period
    BlockHeight firstHeight = lastBlock->height - DIFFICULTY_ADJUSTMENT_INTERVAL + 1;
    const BlockIndex* firstBlock = GetBlockAtHeight(blockchain, firstHeight);

    if (!firstBlock) {
        LOG_ERROR("Difficulty", "Cannot find first block for adjustment");
        return lastBlock->GetBits();
    }

    // Calculate actual timespan
    Timestamp actualTimespan = CalculateActualTimespan(firstBlock, lastBlock);

    // Target timespan (2016 blocks * 10 minutes)
    Timestamp targetTimespan = GetTargetTimespan();

    // Limit timespan to prevent extreme adjustments
    actualTimespan = LimitTimespan(actualTimespan, targetTimespan);

    // Get current target
    Hash256 currentTarget = crypto::Hash::CompactToTarget(lastBlock->GetBits());

    // Calculate new target: newTarget = currentTarget * actualTimespan / targetTimespan
    // We need to do big number arithmetic here
    // For simplicity, we'll use a scaled integer approach

    // Convert to difficulty value
    double currentDifficulty = GetDifficulty(lastBlock->GetBits());

    // Adjust difficulty
    double ratio = static_cast<double>(targetTimespan) / actualTimespan;
    double newDifficulty = currentDifficulty * ratio;

    // Prevent difficulty from going too low
    double minDifficulty = GetDifficulty(GetMinimumDifficulty());
    if (newDifficulty < minDifficulty) {
        newDifficulty = minDifficulty;
    }

    // Convert back to compact bits
    // This is a simplified version - production would use proper big number library
    uint32_t newBits = lastBlock->GetBits();

    // Adjust bits based on ratio
    if (actualTimespan > targetTimespan) {
        // Took longer, make easier (increase target)
        // Increase target by decreasing difficulty
        uint32_t mantissa = newBits & 0x00ffffff;
        uint32_t exponent = newBits >> 24;

        // Multiply mantissa by ratio (with limits)
        double adjust = static_cast<double>(actualTimespan) / targetTimespan;
        adjust = std::min(adjust, 4.0);  // Max 4x easier

        mantissa = static_cast<uint32_t>(mantissa * adjust);

        // Handle overflow
        while (mantissa > 0x7fffff && exponent < 0xff) {
            mantissa >>= 8;
            exponent++;
        }

        newBits = (exponent << 24) | (mantissa & 0x00ffffff);
    } else {
        // Took shorter, make harder (decrease target)
        uint32_t mantissa = newBits & 0x00ffffff;
        uint32_t exponent = newBits >> 24;

        double adjust = static_cast<double>(targetTimespan) / actualTimespan;
        adjust = std::min(adjust, 4.0);  // Max 4x harder

        mantissa = static_cast<uint32_t>(mantissa / adjust);

        // Handle underflow
        while (mantissa < 0x008000 && exponent > 0) {
            mantissa <<= 8;
            exponent--;
        }

        newBits = (exponent << 24) | (mantissa & 0x00ffffff);
    }

    // Ensure new bits are valid
    if (!IsValidDifficulty(newBits)) {
        LOG_WARNING("Difficulty", "Invalid new difficulty, keeping old");
        return lastBlock->GetBits();
    }

    LOG_INFO("Difficulty", "Difficulty adjustment:");
    LOG_INFO("Difficulty", "  Height: " + std::to_string(lastBlock->height + 1));
    LOG_INFO("Difficulty", "  Actual timespan: " + std::to_string(actualTimespan) + "s");
    LOG_INFO("Difficulty", "  Target timespan: " + std::to_string(targetTimespan) + "s");
    LOG_INFO("Difficulty", "  Old bits: 0x" + std::to_string(lastBlock->GetBits()));
    LOG_INFO("Difficulty", "  New bits: 0x" + std::to_string(newBits));

    return newBits;
}

Timestamp DifficultyAdjuster::CalculateActualTimespan(const BlockIndex* firstBlock,
                                                     const BlockIndex* lastBlock) {
    if (!firstBlock || !lastBlock) {
        return GetTargetTimespan();
    }

    Timestamp actual = lastBlock->GetBlockTime() - firstBlock->GetBlockTime();

    // Sanity check: timespan must be positive
    if (actual <= 0) {
        LOG_WARNING("Difficulty", "Invalid timespan: " + std::to_string(actual));
        return GetTargetTimespan();
    }

    return actual;
}

Timestamp DifficultyAdjuster::LimitTimespan(Timestamp actualTimespan,
                                           Timestamp targetTimespan) {
    // Limit adjustment to 4x easier or 4x harder
    Timestamp minTimespan = targetTimespan / 4;
    Timestamp maxTimespan = targetTimespan * 4;

    if (actualTimespan < minTimespan) {
        LOG_INFO("Difficulty", "Limiting timespan from " +
                 std::to_string(actualTimespan) + " to " + std::to_string(minTimespan));
        return minTimespan;
    }

    if (actualTimespan > maxTimespan) {
        LOG_INFO("Difficulty", "Limiting timespan from " +
                 std::to_string(actualTimespan) + " to " + std::to_string(maxTimespan));
        return maxTimespan;
    }

    return actualTimespan;
}

double DifficultyAdjuster::GetDifficulty(uint32_t bits) {
    // Extract exponent and mantissa
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00ffffff;

    // Calculate difficulty
    // difficulty = max_target / current_target
    // We'll use a simplified calculation

    if (mantissa == 0 || exponent == 0) {
        return 0.0;
    }

    // Max target (difficulty 1)
    const double maxTarget = 0x00000000FFFF0000000000000000000000000000000000000000000000000000;

    // Current target
    double target = mantissa * pow(256.0, exponent - 3);

    // Difficulty
    return maxTarget / target;
}

bool DifficultyAdjuster::IsValidDifficulty(uint32_t bits) {
    // Check bits are in valid range
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00ffffff;

    // Exponent must be reasonable
    if (exponent == 0 || exponent > 0x20) {
        return false;
    }

    // Mantissa must not be zero
    if (mantissa == 0) {
        return false;
    }

    // Must not have sign bit set (mantissa < 0x800000)
    if (mantissa >= 0x800000) {
        return false;
    }

    return true;
}

const BlockIndex* DifficultyAdjuster::GetBlockAtHeight(const Blockchain& blockchain,
                                                       BlockHeight height) {
    return blockchain.GetBlockIndex(height);
}

// DifficultyCalculator implementation

bool DifficultyCalculator::VerifyBlockDifficulty(const Block& block,
                                                 const BlockIndex* prevBlock,
                                                 const Blockchain& blockchain) {
    // Get expected difficulty
    uint32_t expectedBits = GetExpectedDifficulty(prevBlock, blockchain);

    // Check if block difficulty matches expected
    if (block.header.bits != expectedBits) {
        LOG_ERROR("Difficulty", "Block difficulty mismatch");
        LOG_ERROR("Difficulty", "  Expected: 0x" + std::to_string(expectedBits));
        LOG_ERROR("Difficulty", "  Actual: 0x" + std::to_string(block.header.bits));
        return false;
    }

    // Check if block meets difficulty target
    return CheckBlockDifficulty(block);
}

uint32_t DifficultyCalculator::GetExpectedDifficulty(const BlockIndex* prevBlock,
                                                     const Blockchain& blockchain) {
    if (!prevBlock) {
        return DifficultyAdjuster::GetInitialDifficulty();
    }

    // Check if adjustment is needed
    BlockHeight nextHeight = prevBlock->height + 1;
    if (DifficultyAdjuster::ShouldAdjustDifficulty(nextHeight)) {
        return DifficultyAdjuster::GetNextWorkRequired(prevBlock, blockchain);
    }

    // No adjustment, use previous difficulty
    return prevBlock->GetBits();
}

bool DifficultyCalculator::CheckBlockDifficulty(const Block& block) {
    return block.header.CheckProofOfWork();
}

} // namespace dinari
