#include "block.h"
#include "merkle.h"
#include "crypto/hash.h"
#include "util/logger.h"
#include "util/time.h"
#include "dinari/constants.h"
#include <sstream>

namespace dinari {

// BlockHeader implementation

void BlockHeader::SerializeImpl(Serializer& s) const {
    s.WriteUInt32(version);
    s.WriteHash256(prevBlockHash);
    s.WriteHash256(merkleRoot);
    s.WriteUInt64(timestamp);
    s.WriteUInt32(bits);
    s.WriteUInt64(nonce);
}

void BlockHeader::DeserializeImpl(Deserializer& d) {
    version = d.ReadUInt32();
    prevBlockHash = d.ReadHash256();
    merkleRoot = d.ReadHash256();
    timestamp = d.ReadUInt64();
    bits = d.ReadUInt32();
    nonce = d.ReadUInt64();

    hashCached = false;
}

Hash256 BlockHeader::GetHash() const {
    if (hashCached) {
        return cachedHash;
    }

    Serializer s;
    SerializeImpl(s);
    cachedHash = crypto::Hash::DoubleSHA256(s.GetData());
    hashCached = true;

    return cachedHash;
}

bool BlockHeader::IsValid() const {
    // Check version
    if (version == 0) {
        return false;
    }

    // Check timestamp (not too far in future)
    if (Time::IsInFuture(timestamp, 2 * 60 * 60)) {  // 2 hours tolerance
        return false;
    }

    // Check proof-of-work
    if (!CheckProofOfWork()) {
        return false;
    }

    return true;
}

bool BlockHeader::CheckProofOfWork() const {
    return crypto::Hash::CheckProofOfWork(GetHash(), bits);
}

Hash256 BlockHeader::GetTarget() const {
    return crypto::Hash::CompactToTarget(bits);
}

uint256_t BlockHeader::GetWork() const {
    // Work = 2^256 / (target + 1)
    // For simplicity, we approximate as: work = difficulty
    // This is good enough for chain selection
    Hash256 target = GetTarget();

    // Convert target to uint256_t (simplified)
    // Higher difficulty = more work
    // Lower target = higher difficulty = more work

    // For now, return a simple work value based on bits
    // In production, this would be a proper bignum calculation
    uint256_t work = 0x100000000ULL / (bits & 0x00FFFFFF);

    return work;
}

std::string BlockHeader::ToString() const {
    std::ostringstream oss;
    oss << "BlockHeader(\n";
    oss << "  Hash: " << crypto::Hash::ToHex(GetHash()) << "\n";
    oss << "  Version: " << version << "\n";
    oss << "  PrevBlock: " << crypto::Hash::ToHex(prevBlockHash) << "\n";
    oss << "  MerkleRoot: " << crypto::Hash::ToHex(merkleRoot) << "\n";
    oss << "  Timestamp: " << timestamp << " (" << Time::FormatTimestamp(timestamp) << ")\n";
    oss << "  Bits: 0x" << std::hex << bits << std::dec << "\n";
    oss << "  Nonce: " << nonce << "\n";
    oss << ")";
    return oss.str();
}

bool BlockHeader::operator==(const BlockHeader& other) const {
    return GetHash() == other.GetHash();
}

// Block implementation

void Block::SerializeImpl(Serializer& s) const {
    header.SerializeImpl(s);
    s.WriteCompactSize(transactions.size());
    for (const auto& tx : transactions) {
        tx.SerializeImpl(s);
    }
}

void Block::DeserializeImpl(Deserializer& d) {
    header.DeserializeImpl(d);

    uint64_t txCount = d.ReadCompactSize();
    transactions.resize(txCount);
    for (auto& tx : transactions) {
        tx.DeserializeImpl(d);
    }

    sizeCached = false;
}

size_t Block::GetSize() const {
    if (sizeCached) {
        return cachedSize;
    }

    cachedSize = GetSerializedSize();
    sizeCached = true;

    return cachedSize;
}

size_t Block::GetSerializedSize() const {
    Serializer s;
    SerializeImpl(s);
    return s.Size();
}

bool Block::IsValid() const {
    // Check header
    if (!header.IsValid()) {
        LOG_ERROR("Block", "Invalid block header");
        return false;
    }

    // Must have at least one transaction (coinbase)
    if (transactions.empty()) {
        LOG_ERROR("Block", "Block has no transactions");
        return false;
    }

    // Check size limits
    if (GetSize() > MAX_BLOCK_SIZE) {
        LOG_ERROR("Block", "Block size exceeds maximum");
        return false;
    }

    // Check transactions
    if (!CheckTransactions()) {
        return false;
    }

    // Check merkle root
    if (!CheckMerkleRoot()) {
        LOG_ERROR("Block", "Invalid merkle root");
        return false;
    }

    return true;
}

bool Block::CheckTransactions() const {
    // First transaction must be coinbase
    if (!transactions[0].IsCoinbase()) {
        LOG_ERROR("Block", "First transaction is not coinbase");
        return false;
    }

    // Only first transaction can be coinbase
    for (size_t i = 1; i < transactions.size(); ++i) {
        if (transactions[i].IsCoinbase()) {
            LOG_ERROR("Block", "Non-first transaction is coinbase");
            return false;
        }
    }

    // Check each transaction
    for (const auto& tx : transactions) {
        if (!tx.IsValid()) {
            LOG_ERROR("Block", "Invalid transaction in block");
            return false;
        }
    }

    // Check for duplicate transactions
    std::set<Hash256> txHashes;
    for (const auto& tx : transactions) {
        Hash256 hash = tx.GetHash();
        if (txHashes.count(hash)) {
            LOG_ERROR("Block", "Duplicate transaction in block");
            return false;
        }
        txHashes.insert(hash);
    }

    return true;
}

bool Block::CheckMerkleRoot() const {
    Hash256 calculated = CalculateMerkleRoot();
    if (calculated != header.merkleRoot) {
        LOG_ERROR("Block", "Merkle root mismatch");
        LOG_ERROR("Block", "Expected: " + crypto::Hash::ToHex(header.merkleRoot));
        LOG_ERROR("Block", "Calculated: " + crypto::Hash::ToHex(calculated));
        return false;
    }
    return true;
}

Hash256 Block::CalculateMerkleRoot() const {
    std::vector<Hash256> txHashes;
    txHashes.reserve(transactions.size());

    for (const auto& tx : transactions) {
        txHashes.push_back(tx.GetHash());
    }

    return crypto::Hash::ComputeMerkleRoot(txHashes);
}

const Transaction& Block::GetCoinbaseTransaction() const {
    if (transactions.empty()) {
        throw std::runtime_error("Block has no transactions");
    }
    return transactions[0];
}

Transaction& Block::GetCoinbaseTransaction() {
    if (transactions.empty()) {
        throw std::runtime_error("Block has no transactions");
    }
    return transactions[0];
}

bool Block::HasCoinbase() const {
    return !transactions.empty() && transactions[0].IsCoinbase();
}

Amount Block::GetTotalFees(const UTXOSet& utxos) const {
    Amount totalFees = 0;

    // Skip coinbase (first transaction)
    for (size_t i = 1; i < transactions.size(); ++i) {
        totalFees += transactions[i].GetFee(utxos);
    }

    return totalFees;
}

Amount Block::GetBlockReward() const {
    if (!HasCoinbase()) {
        return 0;
    }

    return GetCoinbaseTransaction().GetOutputValue();
}

std::string Block::ToString() const {
    std::ostringstream oss;
    oss << "Block(\n";
    oss << "  " << header.ToString() << "\n";
    oss << "  Transactions: " << transactions.size() << "\n";
    oss << "  Size: " << GetSize() << " bytes\n";
    oss << ")";
    return oss.str();
}

bool Block::operator==(const Block& other) const {
    return GetHash() == other.GetHash();
}

// BlockIndex implementation

void BlockIndex::UpdateChainWork() {
    if (prev) {
        chainWork = prev->chainWork + block->header.GetWork();
    } else {
        chainWork = block->header.GetWork();
    }
}

// BlockBuilder implementation

BlockBuilder::BlockBuilder() {
    block.header.version = 1;
    block.header.timestamp = Time::GetCurrentTime();
}

BlockBuilder& BlockBuilder::SetVersion(uint32_t version) {
    block.header.version = version;
    return *this;
}

BlockBuilder& BlockBuilder::SetPrevBlockHash(const Hash256& hash) {
    block.header.prevBlockHash = hash;
    return *this;
}

BlockBuilder& BlockBuilder::SetTimestamp(Timestamp time) {
    block.header.timestamp = time;
    return *this;
}

BlockBuilder& BlockBuilder::SetBits(uint32_t bits) {
    block.header.bits = bits;
    return *this;
}

BlockBuilder& BlockBuilder::SetNonce(Nonce nonce) {
    block.header.nonce = nonce;
    return *this;
}

BlockBuilder& BlockBuilder::AddTransaction(const Transaction& tx) {
    block.transactions.push_back(tx);
    return *this;
}

BlockBuilder& BlockBuilder::SetCoinbase(const Transaction& coinbase) {
    if (block.transactions.empty()) {
        block.transactions.push_back(coinbase);
    } else {
        block.transactions[0] = coinbase;
    }
    return *this;
}

BlockBuilder& BlockBuilder::SetTransactions(const std::vector<Transaction>& txs) {
    block.transactions = txs;
    return *this;
}

Block BlockBuilder::Build() {
    // Calculate merkle root
    block.header.merkleRoot = block.CalculateMerkleRoot();

    return block;
}

void BlockBuilder::Reset() {
    block = Block();
    block.header.version = 1;
    block.header.timestamp = Time::GetCurrentTime();
}

// Helper functions

Block CreateGenesisBlock(Timestamp timestamp, uint32_t bits, Nonce nonce,
                        const std::string& genesisMessage) {
    // Create coinbase transaction with entire initial supply
    Transaction coinbase;
    coinbase.version = 1;

    // Coinbase input with genesis message
    TxIn coinbaseInput;
    coinbaseInput.prevOut = OutPoint();  // Null outpoint

    Serializer s;
    s.WriteCompactSize(0);  // Height 0
    s.WriteString(genesisMessage);
    coinbaseInput.scriptSig = s.MoveData();

    coinbase.inputs.push_back(coinbaseInput);

    // Coinbase output with 700 Trillion DNT
    // Note: In a real deployment, this would likely be distributed differently
    TxOut coinbaseOutput;
    coinbaseOutput.value = MAX_MONEY;  // 700 Trillion DNT

    // Genesis output script (can be unspendable or to a specific address)
    Serializer scriptS;
    scriptS.WriteUInt8(0x76);  // OP_DUP
    scriptS.WriteUInt8(0xa9);  // OP_HASH160
    scriptS.WriteUInt8(20);    // Push 20 bytes
    Hash160 genesisHash{};     // All zeros or specific address
    scriptS.WriteHash160(genesisHash);
    scriptS.WriteUInt8(0x88);  // OP_EQUALVERIFY
    scriptS.WriteUInt8(0xac);  // OP_CHECKSIG
    coinbaseOutput.scriptPubKey = scriptS.MoveData();

    coinbase.outputs.push_back(coinbaseOutput);
    coinbase.lockTime = 0;

    // Build genesis block
    BlockBuilder builder;
    builder.SetVersion(1)
           .SetPrevBlockHash(Hash256{})  // All zeros
           .SetTimestamp(timestamp)
           .SetBits(bits)
           .SetNonce(nonce)
           .SetCoinbase(coinbase);

    Block genesis = builder.Build();

    LOG_INFO("Genesis", "Created genesis block");
    LOG_INFO("Genesis", "Hash: " + crypto::Hash::ToHex(genesis.GetHash()));
    LOG_INFO("Genesis", "Merkle Root: " + crypto::Hash::ToHex(genesis.header.merkleRoot));
    LOG_INFO("Genesis", "Initial Supply: " + FormatAmount(MAX_MONEY));

    return genesis;
}

bool MineBlock(Block& block, uint64_t maxIterations) {
    LOG_INFO("Mining", "Starting mining...");
    LOG_INFO("Mining", "Target: 0x" + std::to_string(block.header.bits));

    uint64_t iterations = 0;
    Nonce startNonce = block.header.nonce;

    Timer timer;

    while (maxIterations == 0 || iterations < maxIterations) {
        // Clear cached hash
        block.header.hashCached = false;

        // Check if valid
        if (block.header.CheckProofOfWork()) {
            double elapsed = timer.ElapsedSeconds();
            double hashrate = iterations / elapsed;

            LOG_INFO("Mining", "Found valid nonce: " + std::to_string(block.header.nonce));
            LOG_INFO("Mining", "Hash: " + crypto::Hash::ToHex(block.GetHash()));
            LOG_INFO("Mining", "Iterations: " + std::to_string(iterations));
            LOG_INFO("Mining", "Time: " + std::to_string(elapsed) + " seconds");
            LOG_INFO("Mining", "Hashrate: " + std::to_string(hashrate) + " H/s");

            return true;
        }

        // Increment nonce
        block.header.nonce++;
        iterations++;

        // Log progress every million iterations
        if (iterations % 1000000 == 0) {
            LOG_DEBUG("Mining", "Tried " + std::to_string(iterations) + " nonces...");
        }

        // If we've exhausted nonce space, update timestamp
        if (block.header.nonce == 0) {
            block.header.timestamp = Time::GetCurrentTime();
            LOG_DEBUG("Mining", "Nonce overflow, updating timestamp");
        }
    }

    LOG_WARNING("Mining", "Max iterations reached without finding valid nonce");
    return false;
}

} // namespace dinari
