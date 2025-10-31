#ifndef DINARI_CONSENSUS_VALIDATION_H
#define DINARI_CONSENSUS_VALIDATION_H

#include "dinari/types.h"
#include "blockchain/block.h"
#include "core/transaction.h"
#include <string>

namespace dinari {

/**
 * @brief Consensus validation rules for Dinari blockchain
 *
 * Implements all consensus rules that blocks and transactions must follow.
 * These rules ensure all nodes agree on blockchain validity.
 */

/**
 * @brief Validation result
 */
struct ValidationResult {
    bool valid;
    std::string error;

    ValidationResult() : valid(true) {}
    ValidationResult(bool v, const std::string& err = "") : valid(v), error(err) {}

    static ValidationResult Valid() { return ValidationResult(true); }
    static ValidationResult Invalid(const std::string& err) {
        return ValidationResult(false, err);
    }

    operator bool() const { return valid; }
};

/**
 * @brief Consensus validator
 */
class ConsensusValidator {
public:
    /**
     * @brief Validate block against consensus rules
     *
     * @param block Block to validate
     * @param prevBlock Previous block in chain
     * @param height Block height
     * @param blockchain Blockchain reference
     * @param utxos UTXO set
     * @return Validation result
     */
    static ValidationResult ValidateBlock(const Block& block,
                                         const BlockIndex* prevBlock,
                                         BlockHeight height,
                                         const class Blockchain& blockchain,
                                         const class UTXOSet& utxos);

    /**
     * @brief Validate block header
     *
     * @param header Block header to validate
     * @param prevBlock Previous block
     * @param blockchain Blockchain reference
     * @return Validation result
     */
    static ValidationResult ValidateBlockHeader(const BlockHeader& header,
                                               const BlockIndex* prevBlock,
                                               const Blockchain& blockchain);

    /**
     * @brief Validate transaction in context
     *
     * @param tx Transaction to validate
     * @param height Current block height
     * @param utxos UTXO set
     * @param inBlock Whether transaction is in a block
     * @return Validation result
     */
    static ValidationResult ValidateTransaction(const Transaction& tx,
                                               BlockHeight height,
                                               const UTXOSet& utxos,
                                               bool inBlock = false);

    /**
     * @brief Validate coinbase transaction
     *
     * @param tx Coinbase transaction
     * @param height Block height
     * @param blockReward Expected block reward
     * @param totalFees Total transaction fees in block
     * @return Validation result
     */
    static ValidationResult ValidateCoinbase(const Transaction& tx,
                                            BlockHeight height,
                                            Amount blockReward,
                                            Amount totalFees);

    /**
     * @brief Check if transaction is final
     *
     * @param tx Transaction to check
     * @param height Block height
     * @param blockTime Block timestamp
     * @return true if final
     */
    static bool IsFinalTransaction(const Transaction& tx,
                                   BlockHeight height,
                                   Timestamp blockTime);

    /**
     * @brief Validate block size
     *
     * @param block Block to validate
     * @return Validation result
     */
    static ValidationResult ValidateBlockSize(const Block& block);

    /**
     * @brief Validate block sigops (signature operations)
     *
     * @param block Block to validate
     * @return Validation result
     */
    static ValidationResult ValidateBlockSigOps(const Block& block);

    /**
     * @brief Validate block timestamp
     *
     * @param timestamp Block timestamp
     * @param prevBlock Previous block
     * @return Validation result
     */
    static ValidationResult ValidateBlockTime(Timestamp timestamp,
                                             const BlockIndex* prevBlock);

    /**
     * @brief Check if newly minted coins would exceed maximum supply
     *
     * @param totalSupply Current total supply prior to block
     * @param newlyMinted Coins created in the candidate block
     * @return Validation result
     */
    static ValidationResult ValidateMoneySupply(Amount totalSupply,
                                                Amount newlyMinted);

private:
    // Helper methods
    static size_t CountSigOps(const Transaction& tx);
    static bool CheckTransactionInputs(const Transaction& tx,
                                      const UTXOSet& utxos,
                                      BlockHeight height,
                                      std::string& error);
};

/**
 * @brief Context check validator (checks that don't require full validation)
 */
class ContextCheckValidator {
public:
    /**
     * @brief Quick validation checks that don't require UTXO set
     *
     * @param block Block to check
     * @return Validation result
     */
    static ValidationResult QuickBlockCheck(const Block& block);

    /**
     * @brief Quick transaction checks
     *
     * @param tx Transaction to check
     * @return Validation result
     */
    static ValidationResult QuickTransactionCheck(const Transaction& tx);

    /**
     * @brief Check block header without full context
     *
     * @param header Block header
     * @return Validation result
     */
    static ValidationResult QuickHeaderCheck(const BlockHeader& header);
};

} // namespace dinari

#endif // DINARI_CONSENSUS_VALIDATION_H
