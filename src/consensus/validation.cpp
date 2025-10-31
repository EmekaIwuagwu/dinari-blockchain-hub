#include "validation.h"
#include "difficulty.h"
#include "blockchain/blockchain.h"
#include "core/utxo.h"
#include "util/logger.h"
#include "util/time.h"
#include "dinari/constants.h"

namespace dinari {

ValidationResult ConsensusValidator::ValidateBlock(const Block& block,
                                                   const BlockIndex* prevBlock,
                                                   BlockHeight height,
                                                   const Blockchain& blockchain,
                                                   const UTXOSet& utxos) {
    // Quick checks first
    auto quickResult = ContextCheckValidator::QuickBlockCheck(block);
    if (!quickResult) {
        return quickResult;
    }

    // Validate header
    auto headerResult = ValidateBlockHeader(block.header, prevBlock, blockchain);
    if (!headerResult) {
        return headerResult;
    }

    // Validate block size
    auto sizeResult = ValidateBlockSize(block);
    if (!sizeResult) {
        return sizeResult;
    }

    // Validate sigops
    auto sigopsResult = ValidateBlockSigOps(block);
    if (!sigopsResult) {
        return sigopsResult;
    }

    // First transaction must be coinbase
    if (!block.transactions[0].IsCoinbase()) {
        return ValidationResult::Invalid("First transaction must be coinbase");
    }

    // Validate coinbase
    Amount blockReward = GetBlockReward(height);
    Amount totalFees = block.GetTotalFees(utxos);
    auto coinbaseResult = ValidateCoinbase(block.transactions[0], height,
                                          blockReward, totalFees);
    if (!coinbaseResult) {
        return coinbaseResult;
    }

    // Validate all transactions
    for (size_t i = 0; i < block.transactions.size(); ++i) {
        const auto& tx = block.transactions[i];

        // Coinbase already validated
        if (i == 0) continue;

        // Transaction must not be coinbase
        if (tx.IsCoinbase()) {
            return ValidationResult::Invalid("Non-first transaction is coinbase");
        }

        // Validate transaction
        auto txResult = ValidateTransaction(tx, height, utxos, true);
        if (!txResult) {
            return ValidationResult::Invalid("Invalid transaction: " + txResult.error);
        }

        // Check finality
        if (!IsFinalTransaction(tx, height, block.header.timestamp)) {
            return ValidationResult::Invalid("Transaction not final");
        }
    }

    // Check merkle root
    if (!block.CheckMerkleRoot()) {
        return ValidationResult::Invalid("Invalid merkle root");
    }

    // Validate money supply
    // Note: Total supply calculation requires summing all coinbase outputs in the blockchain
    Amount totalSupply = 0;
    auto moneyResult = ValidateMoneySupply(block, totalSupply);
    if (!moneyResult) {
        return moneyResult;
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateBlockHeader(const BlockHeader& header,
                                                         const BlockIndex* prevBlock,
                                                         const Blockchain& blockchain) {
    // Validate proof-of-work
    if (!header.CheckProofOfWork()) {
        return ValidationResult::Invalid("Invalid proof-of-work");
    }

    // Validate difficulty
    if (prevBlock) {
        uint32_t expectedBits = DifficultyCalculator::GetExpectedDifficulty(prevBlock, blockchain);
        if (header.bits != expectedBits) {
            return ValidationResult::Invalid("Incorrect difficulty target");
        }
    }

    // Validate timestamp
    auto timeResult = ValidateBlockTime(header.timestamp, prevBlock);
    if (!timeResult) {
        return timeResult;
    }

    // Validate version
    if (header.version == 0) {
        return ValidationResult::Invalid("Invalid block version");
    }

    // Validate previous block hash
    if (prevBlock) {
        if (header.prevBlockHash != prevBlock->GetBlockHash()) {
            return ValidationResult::Invalid("Previous block hash mismatch");
        }
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateTransaction(const Transaction& tx,
                                                         BlockHeight height,
                                                         const UTXOSet& utxos,
                                                         bool inBlock) {
    // Quick checks
    auto quickResult = ContextCheckValidator::QuickTransactionCheck(tx);
    if (!quickResult) {
        return quickResult;
    }

    // Coinbase can only be in blocks
    if (tx.IsCoinbase() && !inBlock) {
        return ValidationResult::Invalid("Coinbase transaction outside block");
    }

    // Non-coinbase transactions need inputs
    if (!tx.IsCoinbase()) {
        std::string error;
        if (!CheckTransactionInputs(tx, utxos, height, error)) {
            return ValidationResult::Invalid(error);
        }
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateCoinbase(const Transaction& tx,
                                                      [[maybe_unused]] BlockHeight height,
                                                      Amount blockReward,
                                                      Amount totalFees) {
    if (!tx.IsCoinbase()) {
        return ValidationResult::Invalid("Transaction is not coinbase");
    }

    // Check output value
    Amount outputValue = tx.GetOutputValue();
    Amount maxAllowed = blockReward + totalFees;

    if (outputValue > maxAllowed) {
        return ValidationResult::Invalid("Coinbase output exceeds block reward + fees");
    }

    // Check coinbase script size
    if (tx.inputs[0].scriptSig.size() < 2 || tx.inputs[0].scriptSig.size() > 100) {
        return ValidationResult::Invalid("Invalid coinbase script size");
    }

    return ValidationResult::Valid();
}

bool ConsensusValidator::IsFinalTransaction(const Transaction& tx,
                                           BlockHeight height,
                                           Timestamp blockTime) {
    return tx.IsFinal(height, blockTime);
}

ValidationResult ConsensusValidator::ValidateBlockSize(const Block& block) {
    size_t blockSize = block.GetSize();

    if (blockSize > MAX_BLOCK_SIZE) {
        return ValidationResult::Invalid("Block size exceeds maximum");
    }

    if (blockSize < 81) {  // Minimum: header + 1 empty tx
        return ValidationResult::Invalid("Block size too small");
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateBlockSigOps(const Block& block) {
    size_t totalSigOps = 0;

    for (const auto& tx : block.transactions) {
        totalSigOps += CountSigOps(tx);
    }

    if (totalSigOps > MAX_BLOCK_SIGOPS) {
        return ValidationResult::Invalid("Block has too many sigops");
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateBlockTime(Timestamp timestamp,
                                                       const BlockIndex* prevBlock) {
    // Check timestamp is not too far in future
    if (Time::IsInFuture(timestamp, 2 * 60 * 60)) {  // 2 hours
        return ValidationResult::Invalid("Block timestamp too far in future");
    }

    // Check timestamp is after previous block
    if (prevBlock && timestamp <= prevBlock->GetBlockTime()) {
        return ValidationResult::Invalid("Block timestamp not after previous block");
    }

    return ValidationResult::Valid();
}

ValidationResult ConsensusValidator::ValidateMoneySupply(const Block& block,
                                                         Amount totalSupply) {
    Amount blockValue = block.GetBlockReward();

    if (totalSupply + blockValue > MAX_MONEY) {
        return ValidationResult::Invalid("Block would exceed maximum money supply");
    }

    return ValidationResult::Valid();
}

size_t ConsensusValidator::CountSigOps(const Transaction& tx) {
    // Simplified sigop counting
    // In production, this would count actual signature operations in scripts
    size_t sigops = 0;

    for (const auto& input : tx.inputs) {
        // Count OP_CHECKSIG and OP_CHECKMULTISIG in scriptSig
        for (const auto& byte : input.scriptSig) {
            if (byte == 0xac || byte == 0xad) {  // OP_CHECKSIG, OP_CHECKSIGVERIFY
                sigops++;
            } else if (byte == 0xae || byte == 0xaf) {  // OP_CHECKMULTISIG, OP_CHECKMULTISIGVERIFY
                sigops += 20;  // Conservative estimate
            }
        }
    }

    for (const auto& output : tx.outputs) {
        for (const auto& byte : output.scriptPubKey) {
            if (byte == 0xac || byte == 0xad) {
                sigops++;
            } else if (byte == 0xae || byte == 0xaf) {
                sigops += 20;
            }
        }
    }

    return sigops;
}

bool ConsensusValidator::CheckTransactionInputs(const Transaction& tx,
                                               const UTXOSet& utxos,
                                               BlockHeight height,
                                               std::string& error) {
    Amount totalIn = 0;

    for (const auto& input : tx.inputs) {
        // Check UTXO exists
        const UTXOEntry* utxo = utxos.GetUTXOEntry(input.prevOut);
        if (!utxo) {
            error = "Input references non-existent UTXO";
            return false;
        }

        // Check maturity (coinbase must have 100 confirmations)
        if (!utxo->IsSpendable(height)) {
            error = "Input references immature coinbase";
            return false;
        }

        totalIn += utxo->output.value;

        // Check for overflow
        if (!MoneyRange(totalIn)) {
            error = "Input value overflow";
            return false;
        }
    }

    // Check outputs don't exceed inputs
    Amount totalOut = tx.GetOutputValue();

    if (totalOut > totalIn) {
        error = "Outputs exceed inputs";
        return false;
    }

    return true;
}

// ContextCheckValidator implementation

ValidationResult ContextCheckValidator::QuickBlockCheck(const Block& block) {
    // Check basic block validity
    if (!block.IsValid()) {
        return ValidationResult::Invalid("Block failed basic validation");
    }

    // Must have transactions
    if (block.transactions.empty()) {
        return ValidationResult::Invalid("Block has no transactions");
    }

    // First must be coinbase
    if (!block.transactions[0].IsCoinbase()) {
        return ValidationResult::Invalid("First transaction not coinbase");
    }

    // Only first can be coinbase
    for (size_t i = 1; i < block.transactions.size(); ++i) {
        if (block.transactions[i].IsCoinbase()) {
            return ValidationResult::Invalid("Multiple coinbase transactions");
        }
    }

    return ValidationResult::Valid();
}

ValidationResult ContextCheckValidator::QuickTransactionCheck(const Transaction& tx) {
    // Check basic transaction validity
    if (!tx.IsValid()) {
        return ValidationResult::Invalid("Transaction failed basic validation");
    }

    // Check for duplicate inputs
    std::set<OutPoint> seenInputs;
    for (const auto& input : tx.inputs) {
        if (seenInputs.count(input.prevOut)) {
            return ValidationResult::Invalid("Duplicate input");
        }
        seenInputs.insert(input.prevOut);
    }

    return ValidationResult::Valid();
}

ValidationResult ContextCheckValidator::QuickHeaderCheck(const BlockHeader& header) {
    if (!header.IsValid()) {
        return ValidationResult::Invalid("Header failed basic validation");
    }

    return ValidationResult::Valid();
}

} // namespace dinari
