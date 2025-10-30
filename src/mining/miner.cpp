#include "miner.h"
#include "consensus/difficulty.h"
#include "util/logger.h"
#include "util/time.h"
#include "wallet/address.h"
#include <algorithm>

namespace dinari {

Miner::Miner(Blockchain& chain, const MiningConfig& cfg)
    : blockchain(chain)
    , config(cfg)
    , mining(false)
    , shouldStop(false)
    , blocksFound(0)
    , hashesComputed(0)
    , startTime(0) {
}

Miner::~Miner() {
    Stop();
}

bool Miner::Start() {
    if (mining.load()) {
        return true;
    }

    if (!config.coinbaseAddress.IsValid()) {
        LOG_ERROR("Miner", "Invalid coinbase address");
        return false;
    }

    LOG_INFO("Miner", "Starting miner with " + std::to_string(config.numThreads) + " threads");

    shouldStop.store(false);
    mining.store(true);
    startTime = Time::GetCurrentTime();

    // Start mining threads
    for (uint32_t i = 0; i < config.numThreads; ++i) {
        minerThreads.emplace_back(&Miner::MinerThreadFunc, this, i);
    }

    LOG_INFO("Miner", "Miner started");

    return true;
}

void Miner::Stop() {
    if (!mining.load()) {
        return;
    }

    LOG_INFO("Miner", "Stopping miner");

    shouldStop.store(true);
    mining.store(false);

    // Wait for threads
    for (auto& thread : minerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    minerThreads.clear();

    LOG_INFO("Miner", "Miner stopped");
}

MiningStats Miner::GetStats() const {
    MiningStats stats;
    stats.blocksFound = blocksFound.load();
    stats.hashesComputed = hashesComputed.load();
    stats.startTime = startTime;

    Timestamp now = Time::GetCurrentTime();
    if (now > startTime) {
        stats.hashrate = static_cast<double>(stats.hashesComputed) / (now - startTime);
    }

    return stats;
}

void Miner::SetBlockFoundCallback(BlockFoundCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    blockFoundCallback = callback;
}

bool Miner::MineBlock(Block& block, uint64_t maxIterations) {
    Timestamp startTime = Time::GetCurrentTime();

    uint64_t hashes = 0;
    Nonce nonce = 0;

    uint32_t bits = block.header.bits;
    Hash256 target = CPUMiner::BitsToTarget(bits);

    while (nonce < config.maxNonce) {
        if (maxIterations > 0 && hashes >= maxIterations) {
            return false;
        }

        block.header.nonce = nonce;
        block.header.timestamp = Time::GetCurrentTime();

        Hash256 hash = block.header.GetHash();
        hashes++;

        if (CPUMiner::CheckTarget(hash, target)) {
            hashesComputed.fetch_add(hashes);

            Timestamp elapsed = Time::GetCurrentTime() - startTime;
            double hashrate = elapsed > 0 ? static_cast<double>(hashes) / elapsed : 0.0;

            LOG_INFO("Miner", "Block found! Nonce: " + std::to_string(nonce) +
                     ", Hashes: " + std::to_string(hashes) +
                     ", Hashrate: " + std::to_string(hashrate) + " H/s");

            return true;
        }

        nonce++;

        // Update timestamp periodically
        if (nonce % 1000000 == 0) {
            hashesComputed.fetch_add(1000000);
        }
    }

    hashesComputed.fetch_add(hashes);
    return false;
}

void Miner::MinerThreadFunc(uint32_t threadId) {
    LOG_INFO("Miner", "Mining thread " + std::to_string(threadId) + " started");

    while (!shouldStop.load()) {
        // Create candidate block
        Block candidateBlock;
        if (!CreateCandidateBlock(candidateBlock)) {
            LOG_WARNING("Miner", "Failed to create candidate block");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }

        // Calculate nonce range for this thread
        Nonce startNonce = (config.maxNonce / config.numThreads) * threadId;
        Nonce endNonce = startNonce + (config.maxNonce / config.numThreads);

        // Mine
        std::atomic<uint64_t> threadHashes(0);
        if (CPUMiner::Mine(candidateBlock, startNonce, endNonce, shouldStop, threadHashes)) {
            // Solution found!
            blocksFound.fetch_add(1);
            OnBlockFound(candidateBlock);
        }

        hashesComputed.fetch_add(threadHashes.load());
    }

    LOG_INFO("Miner", "Mining thread " + std::to_string(threadId) + " stopped");
}

bool Miner::CreateCandidateBlock(Block& block) {
    // Get blockchain info
    const BlockIndex* tip = blockchain.GetBestBlock();
    if (!tip) {
        return false;
    }

    BlockHeight height = tip->height + 1;

    // Create block header
    block.header.version = 1;
    block.header.prevBlockHash = tip->hash;
    block.header.timestamp = Time::GetCurrentTime();
    block.header.bits = GetNextDifficulty();
    block.header.nonce = 0;

    // Create coinbase transaction
    Transaction coinbase;
    coinbase.version = 1;
    coinbase.lockTime = 0;

    // Coinbase input
    TxIn coinbaseInput;
    coinbaseInput.prevOut.hash = Hash256{0};
    coinbaseInput.prevOut.n = 0xFFFFFFFF;
    coinbaseInput.sequence = 0xFFFFFFFF;

    // Block height in scriptSig
    bytes heightScript;
    heightScript.push_back(static_cast<byte>(height & 0xFF));
    heightScript.push_back(static_cast<byte>((height >> 8) & 0xFF));
    heightScript.push_back(static_cast<byte>((height >> 16) & 0xFF));
    heightScript.push_back(static_cast<byte>((height >> 24) & 0xFF));
    coinbaseInput.scriptSig = heightScript;

    coinbase.vin.push_back(coinbaseInput);

    // Coinbase output
    TxOut coinbaseOutput;
    coinbaseOutput.value = GetBlockReward(height);
    coinbaseOutput.scriptPubKey = AddressGenerator::GenerateScriptPubKey(config.coinbaseAddress);

    coinbase.vout.push_back(coinbaseOutput);

    block.transactions.push_back(coinbase);

    // Add transactions from mempool
    const MemPool& mempool = blockchain.GetMemPool();
    std::vector<Transaction> mempoolTxs = mempool.GetTransactionsForMining(MAX_BLOCK_SIZE - 1000, 1000);

    for (const auto& tx : mempoolTxs) {
        block.transactions.push_back(tx);
    }

    // Calculate merkle root
    std::vector<Hash256> txHashes;
    for (const auto& tx : block.transactions) {
        txHashes.push_back(tx.GetHash());
    }
    block.header.merkleRoot = MerkleTree::ComputeMerkleRoot(txHashes);

    return true;
}

bool Miner::CheckProofOfWork(const Hash256& hash, uint32_t bits) {
    Hash256 target = CPUMiner::BitsToTarget(bits);
    return CPUMiner::CheckTarget(hash, target);
}

uint32_t Miner::GetNextDifficulty() {
    const BlockIndex* tip = blockchain.GetBestBlock();
    if (!tip) {
        return 0x1d00ffff;  // Genesis difficulty
    }

    return GetNextWorkRequired(tip, blockchain);
}

void Miner::OnBlockFound(const Block& block) {
    LOG_INFO("Miner", "Block found: " + block.GetHash().ToHex());

    // Add block to blockchain
    if (blockchain.AcceptBlock(block)) {
        LOG_INFO("Miner", "Block accepted by blockchain");
    } else {
        LOG_ERROR("Miner", "Block rejected by blockchain");
    }

    // Call callback
    std::lock_guard<std::mutex> lock(callbackMutex);
    if (blockFoundCallback) {
        blockFoundCallback(block);
    }
}

// CPUMiner implementation

bool CPUMiner::Mine(Block& block,
                   Nonce startNonce,
                   Nonce endNonce,
                   const std::atomic<bool>& shouldStop,
                   std::atomic<uint64_t>& hashesComputed) {
    Hash256 target = BitsToTarget(block.header.bits);

    for (Nonce nonce = startNonce; nonce < endNonce; ++nonce) {
        if (shouldStop.load()) {
            return false;
        }

        block.header.nonce = nonce;

        // Update timestamp every 1000 iterations
        if (nonce % 1000 == 0) {
            block.header.timestamp = Time::GetCurrentTime();
        }

        Hash256 hash = block.header.GetHash();
        hashesComputed.fetch_add(1);

        if (CheckTarget(hash, target)) {
            return true;
        }
    }

    return false;
}

Hash256 CPUMiner::BitsToTarget(uint32_t bits) {
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x00FFFFFF;

    Hash256 target{0};

    if (exponent <= 3) {
        mantissa >>= (8 * (3 - exponent));
        target[29] = (mantissa >> 16) & 0xFF;
        target[30] = (mantissa >> 8) & 0xFF;
        target[31] = mantissa & 0xFF;
    } else {
        size_t offset = 32 - exponent;
        if (offset < 29) {
            target[offset] = mantissa & 0xFF;
            target[offset + 1] = (mantissa >> 8) & 0xFF;
            target[offset + 2] = (mantissa >> 16) & 0xFF;
        }
    }

    return target;
}

bool CPUMiner::CheckTarget(const Hash256& hash, const Hash256& target) {
    // Hash must be less than or equal to target
    for (int i = 0; i < 32; ++i) {
        if (hash[i] < target[i]) {
            return true;
        } else if (hash[i] > target[i]) {
            return false;
        }
    }
    return true;
}

} // namespace dinari
