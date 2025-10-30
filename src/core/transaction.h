#ifndef DINARI_CORE_TRANSACTION_H
#define DINARI_CORE_TRANSACTION_H

#include "dinari/types.h"
#include "util/serialize.h"
#include "crypto/hash.h"
#include <vector>
#include <string>

namespace dinari {

/**
 * @brief Transaction Output (TxOut)
 *
 * Represents an output in a transaction, specifying an amount and the
 * conditions (script) required to spend it.
 */
class TxOut {
public:
    Amount value;           // Amount in smallest unit
    bytes scriptPubKey;     // Script defining spending conditions

    TxOut() : value(0) {}
    TxOut(Amount val, const bytes& script) : value(val), scriptPubKey(script) {}

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Validation
    bool IsValid() const;
    bool IsDust() const;  // Check if output is dust (too small to be economical)

    // Get size
    size_t GetSize() const;

    // Equality
    bool operator==(const TxOut& other) const;
    bool operator!=(const TxOut& other) const { return !(*this == other); }
};

/**
 * @brief Transaction Input Point (OutPoint)
 *
 * References a specific output from a previous transaction.
 */
class OutPoint {
public:
    Hash256 txHash;         // Hash of the transaction containing the output
    TxOutIndex index;       // Index of the output in that transaction

    OutPoint() : index(0xFFFFFFFF) {}
    OutPoint(const Hash256& hash, TxOutIndex idx) : txHash(hash), index(idx) {}

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Check if this is a null outpoint (coinbase)
    bool IsNull() const;

    // Equality
    bool operator==(const OutPoint& other) const;
    bool operator!=(const OutPoint& other) const { return !(*this == other); }
    bool operator<(const OutPoint& other) const;

    // String representation
    std::string ToString() const;
};

/**
 * @brief Transaction Input (TxIn)
 *
 * Represents an input in a transaction, spending a previous output.
 */
class TxIn {
public:
    OutPoint prevOut;       // Reference to previous output being spent
    bytes scriptSig;        // Script providing proof of ownership
    uint32_t sequence;      // Sequence number (for relative lock time)

    TxIn() : sequence(0xFFFFFFFF) {}
    TxIn(const OutPoint& prev, const bytes& script = bytes(), uint32_t seq = 0xFFFFFFFF)
        : prevOut(prev), scriptSig(script), sequence(seq) {}

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Check if this is a coinbase input
    bool IsCoinbase() const { return prevOut.IsNull(); }

    // Get size
    size_t GetSize() const;

    // Equality
    bool operator==(const TxIn& other) const;
    bool operator!=(const TxIn& other) const { return !(*this == other); }
};

/**
 * @brief Witness data for SegWit transactions
 *
 * Contains signature and public key data separated from the transaction.
 * Not implemented in basic version, placeholder for future SegWit support.
 */
class TxWitness {
public:
    std::vector<bytes> stack;

    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    bool IsNull() const { return stack.empty(); }
};

/**
 * @brief Transaction
 *
 * Core transaction structure for the Dinari blockchain.
 * Uses a UTXO model similar to Bitcoin.
 */
class Transaction {
public:
    uint32_t version;               // Transaction version
    std::vector<TxIn> inputs;       // Transaction inputs
    std::vector<TxOut> outputs;     // Transaction outputs
    uint32_t lockTime;              // Lock time (0 = no lock)

    // Cached hash (computed once)
    mutable Hash256 cachedHash;
    mutable bool hashCached;

    Transaction() : version(1), lockTime(0), hashCached(false) {}

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Get serialized size
    size_t GetSize() const;

    // Get transaction hash (TXID)
    Hash256 GetHash() const;

    // Get hash for signing (removes scriptSig from inputs)
    Hash256 GetSignatureHash(size_t inputIndex, const bytes& scriptCode,
                            uint32_t hashType = 1) const;

    // Validation
    bool IsValid() const;
    bool IsCoinbase() const;
    bool IsFinal(BlockHeight height, Timestamp time) const;

    // Calculate total output value
    Amount GetOutputValue() const;

    // Calculate total input value (requires UTXO set)
    Amount GetInputValue(const class UTXOSet& utxos) const;

    // Calculate fee (requires UTXO set)
    Amount GetFee(const class UTXOSet& utxos) const;

    // Check if transaction is standard
    bool IsStandard() const;

    // Get transaction priority (for mining)
    double GetPriority(const UTXOSet& utxos, BlockHeight currentHeight) const;

    // Get virtual size (for fee calculation)
    size_t GetVirtualSize() const;

    // String representation
    std::string ToString() const;

    // Equality
    bool operator==(const Transaction& other) const;
    bool operator!=(const Transaction& other) const { return !(*this == other); }
};

/**
 * @brief Create a coinbase transaction
 *
 * @param height Block height
 * @param minerAddress Address to receive block reward
 * @param extraNonce Extra nonce for mining
 * @param blockReward Block reward amount
 * @return Coinbase transaction
 */
Transaction CreateCoinbaseTransaction(BlockHeight height,
                                     const std::string& minerAddress,
                                     uint32_t extraNonce,
                                     Amount blockReward);

/**
 * @brief Calculate block reward based on height
 *
 * Implements halving schedule: reward halves every 210,000 blocks
 *
 * @param height Block height
 * @return Block reward amount
 */
Amount GetBlockReward(BlockHeight height);

/**
 * @brief Transaction builder helper class
 */
class TransactionBuilder {
public:
    TransactionBuilder();

    // Set version
    TransactionBuilder& SetVersion(uint32_t ver);

    // Add input
    TransactionBuilder& AddInput(const OutPoint& prevOut, const bytes& scriptSig = bytes());
    TransactionBuilder& AddInput(const Hash256& txHash, TxOutIndex index, const bytes& scriptSig = bytes());

    // Add output
    TransactionBuilder& AddOutput(Amount value, const bytes& scriptPubKey);
    TransactionBuilder& AddOutput(Amount value, const std::string& address);

    // Set lock time
    TransactionBuilder& SetLockTime(uint32_t lockTime);

    // Build transaction
    Transaction Build() const;

    // Reset builder
    void Reset();

private:
    Transaction tx;
};

// Hash specialization for OutPoint (for use in maps/sets)
} // namespace dinari

namespace std {
    template<>
    struct hash<dinari::OutPoint> {
        size_t operator()(const dinari::OutPoint& outpoint) const {
            // Combine hash of txHash and index
            size_t h1 = 0;
            for (auto b : outpoint.txHash) {
                h1 ^= std::hash<uint8_t>{}(b) + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
            }
            return h1 ^ (std::hash<uint32_t>{}(outpoint.index) << 1);
        }
    };
}

#endif // DINARI_CORE_TRANSACTION_H
