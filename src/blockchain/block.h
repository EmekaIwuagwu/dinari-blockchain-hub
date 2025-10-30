#ifndef DINARI_BLOCKCHAIN_BLOCK_H
#define DINARI_BLOCKCHAIN_BLOCK_H

#include "dinari/types.h"
#include "core/transaction.h"
#include "util/serialize.h"
#include <vector>
#include <memory>

namespace dinari {

/**
 * @brief Block Header
 *
 * Contains metadata about the block including PoW information.
 */
class BlockHeader {
public:
    uint32_t version;           // Block version
    Hash256 prevBlockHash;      // Hash of previous block
    Hash256 merkleRoot;         // Merkle root of transactions
    Timestamp timestamp;        // Block creation time
    uint32_t bits;              // Difficulty target (compact format)
    Nonce nonce;                // Proof-of-work nonce

    // Cached hash
    mutable Hash256 cachedHash;
    mutable bool hashCached;

    BlockHeader()
        : version(1)
        , timestamp(0)
        , bits(0)
        , nonce(0)
        , hashCached(false) {
        prevBlockHash.fill(0);
        merkleRoot.fill(0);
    }

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Get block hash
    Hash256 GetHash() const;

    // Check if header is valid
    bool IsValid() const;

    // Check if proof-of-work is valid
    bool CheckProofOfWork() const;

    // Get target difficulty from bits
    Hash256 GetTarget() const;

    // Calculate work represented by this block
    uint256_t GetWork() const;

    // String representation
    std::string ToString() const;

    // Equality
    bool operator==(const BlockHeader& other) const;
    bool operator!=(const BlockHeader& other) const { return !(*this == other); }
};

/**
 * @brief Block
 *
 * Contains the block header and all transactions in the block.
 */
class Block {
public:
    BlockHeader header;
    std::vector<Transaction> transactions;

    // Cached size
    mutable size_t cachedSize;
    mutable bool sizeCached;

    Block() : sizeCached(false), cachedSize(0) {}

    // Serialization
    void SerializeImpl(Serializer& s) const;
    void DeserializeImpl(Deserializer& d);

    // Get block hash
    Hash256 GetHash() const { return header.GetHash(); }

    // Get block size
    size_t GetSize() const;

    // Get serialized size of block (for size limit checks)
    size_t GetSerializedSize() const;

    // Validation
    bool IsValid() const;
    bool CheckTransactions() const;
    bool CheckMerkleRoot() const;

    // Calculate Merkle root from transactions
    Hash256 CalculateMerkleRoot() const;

    // Get coinbase transaction
    const Transaction& GetCoinbaseTransaction() const;
    Transaction& GetCoinbaseTransaction();

    // Check if block has coinbase
    bool HasCoinbase() const;

    // Get total transaction fees (excluding coinbase)
    Amount GetTotalFees(const class UTXOSet& utxos) const;

    // Get block reward (coinbase output value)
    Amount GetBlockReward() const;

    // Get number of transactions
    size_t GetTransactionCount() const { return transactions.size(); }

    // Check if block is empty (genesis or special)
    bool IsEmpty() const { return transactions.empty(); }

    // String representation
    std::string ToString() const;

    // Equality
    bool operator==(const Block& other) const;
    bool operator!=(const Block& other) const { return !(*this == other); }
};

/**
 * @brief Block index entry
 *
 * Contains a block and metadata about its position in the chain.
 */
class BlockIndex {
public:
    // Block data
    SharedPtr<Block> block;

    // Chain metadata
    BlockHeight height;
    uint256_t chainWork;        // Total work from genesis to this block
    BlockIndex* prev;           // Previous block in chain
    std::vector<BlockIndex*> next;  // Possible next blocks (for forks)

    // Status flags
    bool isValid;
    bool isMainChain;
    bool hasData;               // Whether we have the full block data

    BlockIndex()
        : height(0)
        , chainWork(0)
        , prev(nullptr)
        , isValid(false)
        , isMainChain(false)
        , hasData(false) {}

    explicit BlockIndex(const SharedPtr<Block>& blk, BlockHeight h)
        : block(blk)
        , height(h)
        , chainWork(0)
        , prev(nullptr)
        , isValid(false)
        , isMainChain(false)
        , hasData(true) {}

    // Get block hash
    Hash256 GetBlockHash() const {
        return block ? block->GetHash() : Hash256{};
    }

    // Get timestamp
    Timestamp GetBlockTime() const {
        return block ? block->header.timestamp : 0;
    }

    // Get target difficulty
    uint32_t GetBits() const {
        return block ? block->header.bits : 0;
    }

    // Calculate total work up to this block
    void UpdateChainWork();

    // Check if this block is in the main chain
    bool IsInMainChain() const { return isMainChain; }

    // Get block header
    const BlockHeader& GetHeader() const {
        static BlockHeader empty;
        return block ? block->header : empty;
    }
};

/**
 * @brief Block builder helper
 *
 * Helps construct valid blocks for mining.
 */
class BlockBuilder {
public:
    BlockBuilder();

    // Set header fields
    BlockBuilder& SetVersion(uint32_t version);
    BlockBuilder& SetPrevBlockHash(const Hash256& hash);
    BlockBuilder& SetTimestamp(Timestamp time);
    BlockBuilder& SetBits(uint32_t bits);
    BlockBuilder& SetNonce(Nonce nonce);

    // Add transactions
    BlockBuilder& AddTransaction(const Transaction& tx);
    BlockBuilder& SetCoinbase(const Transaction& coinbase);

    // Set all transactions
    BlockBuilder& SetTransactions(const std::vector<Transaction>& txs);

    // Build block (calculates merkle root)
    Block Build();

    // Reset builder
    void Reset();

private:
    Block block;
};

/**
 * @brief Create genesis block
 *
 * Creates the genesis block for the Dinari blockchain with 700T DNT initial supply.
 *
 * @param timestamp Genesis block timestamp
 * @param bits Initial difficulty
 * @param nonce Nonce for PoW
 * @param genesisMessage Genesis message
 * @return Genesis block
 */
Block CreateGenesisBlock(Timestamp timestamp, uint32_t bits, Nonce nonce,
                        const std::string& genesisMessage);

/**
 * @brief Mine block (find valid nonce)
 *
 * Performs proof-of-work mining to find a valid nonce.
 *
 * @param block Block to mine (nonce will be updated)
 * @param maxIterations Maximum number of iterations (0 = unlimited)
 * @return true if valid nonce found
 */
bool MineBlock(Block& block, uint64_t maxIterations = 0);

} // namespace dinari

#endif // DINARI_BLOCKCHAIN_BLOCK_H
