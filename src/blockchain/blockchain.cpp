#include "blockchain.h"
#include "consensus/validation.h"
#include "consensus/difficulty.h"
#include "util/logger.h"
#include "util/time.h"
#include "dinari/constants.h"
#include <algorithm>

namespace dinari {

Blockchain::Blockchain()
    : bestBlock(nullptr)
    , genesisBlock(nullptr) {
}

Blockchain::~Blockchain() {
}

bool Blockchain::Initialize(const Block& genesis) {
    std::lock_guard<std::mutex> lock(mutex);

    LOG_INFO("Blockchain", "Initializing blockchain with genesis block");

    // Validate genesis block
    if (!genesis.IsValid()) {
        LOG_ERROR("Blockchain", "Invalid genesis block");
        return false;
    }

    // Store genesis block
    Hash256 genesisHash = genesis.GetHash();
    auto genesisBlockPtr = std::make_shared<Block>(genesis);
    blocks[genesisHash] = genesisBlockPtr;

    // Create genesis block index
    genesisBlock = CreateBlockIndex(genesisBlockPtr, 0);
    genesisBlock->isMainChain = true;
    genesisBlock->isValid = true;

    // Set as best block
    bestBlock = genesisBlock;

    // Add to height index
    heightIndex[0] = genesisHash;

    // Initialize UTXO set with genesis outputs
    if (!UpdateUTXOs(genesis, 0)) {
        LOG_ERROR("Blockchain", "Failed to initialize UTXO set");
        return false;
    }

    LOG_INFO("Blockchain", "Blockchain initialized");
    LOG_INFO("Blockchain", "Genesis hash: " + crypto::Hash::ToHex(genesisHash));
    LOG_INFO("Blockchain", "Genesis supply: " + FormatAmount(genesis.GetBlockReward()));

    return true;
}

bool Blockchain::AcceptBlock(const Block& block) {
    std::lock_guard<std::mutex> lock(mutex);

    Hash256 blockHash = block.GetHash();

    LOG_INFO("Blockchain", "Processing block: " + crypto::Hash::ToHex(blockHash).substr(0, 16) + "...");

    // Check if we already have this block
    if (HasBlock(blockHash)) {
        LOG_DEBUG("Blockchain", "Block already exists");
        return false;
    }

    // Quick validation
    auto quickResult = ContextCheckValidator::QuickBlockCheck(block);
    if (!quickResult) {
        LOG_ERROR("Blockchain", "Block failed quick validation: " + quickResult.error);
        return false;
    }

    // Find previous block
    const BlockIndex* prevBlock = GetBlockIndex(block.header.prevBlockHash);

    // If previous block not found, add to orphans
    if (!prevBlock) {
        LOG_WARNING("Blockchain", "Previous block not found, adding to orphans");
        auto blockPtr = std::make_shared<Block>(block);
        AddOrphan(blockPtr);
        return false;
    }

    // Calculate height
    BlockHeight height = prevBlock->height + 1;

    LOG_DEBUG("Blockchain", "Block height: " + std::to_string(height));

    // Store block
    auto blockPtr = std::make_shared<Block>(block);
    blocks[blockHash] = blockPtr;

    // Create block index
    BlockIndex* blockIndex = CreateBlockIndex(blockPtr, height);
    blockIndex->prev = const_cast<BlockIndex*>(prevBlock);
    const_cast<BlockIndex*>(prevBlock)->next.push_back(blockIndex);

    // Full validation
    auto validationResult = ConsensusValidator::ValidateBlock(
        block, prevBlock, height, *this, utxos);

    if (!validationResult) {
        LOG_ERROR("Blockchain", "Block validation failed: " + validationResult.error);
        blockIndex->isValid = false;
        return false;
    }

    blockIndex->isValid = true;

    // Connect block
    if (!ConnectBlock(block, blockIndex)) {
        LOG_ERROR("Blockchain", "Failed to connect block");
        return false;
    }

    // Check if this creates a better chain
    if (IsBetterChain(blockIndex)) {
        LOG_INFO("Blockchain", "New best block found");
        if (!SetBestChain(blockIndex)) {
            LOG_ERROR("Blockchain", "Failed to set best chain");
            return false;
        }
    }

    // Process orphans that may now be connectible
    ProcessOrphans(blockHash);

    LOG_INFO("Blockchain", "Block accepted at height " + std::to_string(height));

    return true;
}

bool Blockchain::ConnectBlock(const Block& block, BlockIndex* blockIndex) {
    // Update chain work
    blockIndex->UpdateChainWork();

    // Update UTXO set
    if (!UpdateUTXOs(block, blockIndex->height)) {
        return false;
    }

    // Remove transactions from mempool
    RemoveFromMempool(block);

    return true;
}

bool Blockchain::DisconnectBlock(BlockIndex* blockIndex) {
    if (!blockIndex || !blockIndex->block) {
        return false;
    }

    LOG_INFO("Blockchain", "Disconnecting block at height " +
             std::to_string(blockIndex->height));

    // Revert UTXO changes
    if (!RevertUTXOs(*blockIndex->block)) {
        LOG_ERROR("Blockchain", "Failed to revert UTXO changes");
        return false;
    }

    // Add transactions back to mempool (except coinbase)
    for (size_t i = 1; i < blockIndex->block->transactions.size(); ++i) {
        const auto& tx = blockIndex->block->transactions[i];
        mempool.AddTransaction(tx, utxos, blockIndex->height);
    }

    blockIndex->isMainChain = false;

    return true;
}

bool Blockchain::SetBestChain(BlockIndex* newTip) {
    if (!newTip) {
        return false;
    }

    // If new tip is already best, nothing to do
    if (newTip == bestBlock) {
        return true;
    }

    // Find fork point
    const BlockIndex* fork = FindFork(bestBlock, newTip);

    if (!fork) {
        LOG_ERROR("Blockchain", "Could not find fork point");
        return false;
    }

    LOG_INFO("Blockchain", "Fork point at height " + std::to_string(fork->height));

    // Reorganize if necessary
    if (fork != bestBlock) {
        if (!Reorganize(newTip)) {
            return false;
        }
    }

    // Update best block
    bestBlock = newTip;

    // Update main chain flags and height index
    UpdateMainChain(newTip);

    LOG_INFO("Blockchain", "New best block: " +
             crypto::Hash::ToHex(newTip->GetBlockHash()).substr(0, 16) + "...");
    LOG_INFO("Blockchain", "Height: " + std::to_string(newTip->height));
    LOG_INFO("Blockchain", "Chain work: " + std::to_string(newTip->chainWork));

    return true;
}

bool Blockchain::Reorganize(BlockIndex* newTip) {
    LOG_WARNING("Blockchain", "Chain reorganization required");

    // Find fork point
    const BlockIndex* fork = FindFork(bestBlock, newTip);

    if (!fork) {
        LOG_ERROR("Blockchain", "Cannot reorganize: no fork point");
        return false;
    }

    // Disconnect blocks from old chain
    std::vector<BlockIndex*> toDisconnect;
    BlockIndex* current = bestBlock;
    while (current != fork) {
        toDisconnect.push_back(current);
        current = current->prev;
    }

    LOG_INFO("Blockchain", "Disconnecting " + std::to_string(toDisconnect.size()) + " blocks");

    for (auto* block : toDisconnect) {
        if (!DisconnectBlock(block)) {
            LOG_ERROR("Blockchain", "Failed to disconnect block during reorganization");
            return false;
        }
    }

    // Connect blocks from new chain
    std::vector<BlockIndex*> toConnect = FindPath(const_cast<BlockIndex*>(fork), newTip);

    LOG_INFO("Blockchain", "Connecting " + std::to_string(toConnect.size()) + " blocks");

    for (auto* block : toConnect) {
        if (!block->block) {
            LOG_ERROR("Blockchain", "Block data missing during reorganization");
            return false;
        }

        if (!ConnectBlock(*block->block, block)) {
            LOG_ERROR("Blockchain", "Failed to connect block during reorganization");
            return false;
        }
    }

    LOG_INFO("Blockchain", "Chain reorganization successful");

    return true;
}

std::vector<BlockIndex*> Blockchain::FindPath(BlockIndex* from, BlockIndex* to) const {
    std::vector<BlockIndex*> path;

    BlockIndex* current = to;
    while (current && current != from) {
        path.push_back(current);
        current = current->prev;
    }

    // Reverse to get correct order (from -> to)
    std::reverse(path.begin(), path.end());

    return path;
}

const BlockIndex* Blockchain::FindFork(const BlockIndex* block1,
                                      const BlockIndex* block2) const {
    if (!block1 || !block2) {
        return nullptr;
    }

    // Move both blocks to same height
    const BlockIndex* b1 = block1;
    const BlockIndex* b2 = block2;

    while (b1->height > b2->height) {
        b1 = b1->prev;
    }

    while (b2->height > b1->height) {
        b2 = b2->prev;
    }

    // Move both back until they meet
    while (b1 != b2) {
        b1 = b1->prev;
        b2 = b2->prev;

        if (!b1 || !b2) {
            return nullptr;
        }
    }

    return b1;
}

void Blockchain::UpdateMainChain(BlockIndex* tip) {
    // Clear old height index
    heightIndex.clear();

    // Mark all as not main chain
    for (auto& [hash, index] : blockIndices) {
        index->isMainChain = false;
    }

    // Mark new main chain
    BlockIndex* current = tip;
    while (current) {
        current->isMainChain = true;
        heightIndex[current->height] = current->GetBlockHash();
        current = current->prev;
    }
}

bool Blockchain::IsBetterChain(const BlockIndex* blockIndex) const {
    if (!blockIndex || !bestBlock) {
        return false;
    }

    // More work wins
    return blockIndex->chainWork > bestBlock->chainWork;
}

void Blockchain::AddOrphan(const SharedPtr<Block>& block) {
    Hash256 blockHash = block->GetHash();
    orphanBlocks[blockHash] = block;

    LOG_DEBUG("Blockchain", "Added orphan block: " +
             crypto::Hash::ToHex(blockHash).substr(0, 16) + "...");
}

void Blockchain::ProcessOrphans(const Hash256& parentHash) {
    std::vector<Hash256> toRemove;

    for (const auto& [hash, block] : orphanBlocks) {
        if (block->header.prevBlockHash == parentHash) {
            LOG_INFO("Blockchain", "Processing orphan block");
            if (AcceptBlock(*block)) {
                toRemove.push_back(hash);
            }
        }
    }

    for (const auto& hash : toRemove) {
        orphanBlocks.erase(hash);
    }
}

BlockIndex* Blockchain::CreateBlockIndex(const SharedPtr<Block>& block, BlockHeight height) {
    Hash256 blockHash = block->GetHash();

    auto index = std::make_unique<BlockIndex>(block, height);
    BlockIndex* indexPtr = index.get();

    blockIndices[blockHash] = std::move(index);

    return indexPtr;
}

bool Blockchain::UpdateUTXOs(const Block& block, BlockHeight height) {
    return utxos.ApplyTransaction(block.GetCoinbaseTransaction(), height) &&
           std::all_of(block.transactions.begin() + 1, block.transactions.end(),
                      [&](const Transaction& tx) {
                          return utxos.ApplyTransaction(tx, height);
                      });
}

bool Blockchain::RevertUTXOs(const Block& block) {
    // TODO: Store previous UTXO state for proper reversion
    // For now, we'll just remove the outputs

    for (const auto& tx : block.transactions) {
        Hash256 txHash = tx.GetHash();
        for (size_t i = 0; i < tx.outputs.size(); ++i) {
            OutPoint outpoint(txHash, static_cast<TxOutIndex>(i));
            utxos.RemoveUTXO(outpoint);
        }
    }

    return true;
}

void Blockchain::RemoveFromMempool(const Block& block) {
    std::vector<Hash256> txHashes;
    for (const auto& tx : block.transactions) {
        txHashes.push_back(tx.GetHash());
    }
    mempool.RemoveTransactions(txHashes);
}

const Block* Blockchain::GetBlock(const Hash256& hash) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = blocks.find(hash);
    if (it == blocks.end()) {
        return nullptr;
    }

    return it->second.get();
}

const BlockIndex* Blockchain::GetBlockIndex(const Hash256& hash) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = blockIndices.find(hash);
    if (it == blockIndices.end()) {
        return nullptr;
    }

    return it->second.get();
}

const BlockIndex* Blockchain::GetBlockIndex(BlockHeight height) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = heightIndex.find(height);
    if (it == heightIndex.end()) {
        return nullptr;
    }

    return GetBlockIndex(it->second);
}

BlockHeight Blockchain::GetHeight() const {
    std::lock_guard<std::mutex> lock(mutex);
    return bestBlock ? bestBlock->height : 0;
}

uint256_t Blockchain::GetChainWork() const {
    std::lock_guard<std::mutex> lock(mutex);
    return bestBlock ? bestBlock->chainWork : 0;
}

bool Blockchain::HasBlock(const Hash256& hash) const {
    std::lock_guard<std::mutex> lock(mutex);
    return blocks.find(hash) != blocks.end();
}

bool Blockchain::IsOnMainChain(const Hash256& hash) const {
    std::lock_guard<std::mutex> lock(mutex);

    const BlockIndex* index = GetBlockIndex(hash);
    return index && index->isMainChain;
}

std::vector<Hash256> Blockchain::GetBlocksInRange(BlockHeight startHeight,
                                                  BlockHeight endHeight) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Hash256> result;

    for (BlockHeight h = startHeight; h <= endHeight; ++h) {
        auto it = heightIndex.find(h);
        if (it != heightIndex.end()) {
            result.push_back(it->second);
        }
    }

    return result;
}

Blockchain::Stats Blockchain::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    Stats stats;
    stats.height = GetHeight();
    stats.totalBlocks = blocks.size();
    stats.orphanBlocks = orphanBlocks.size();
    stats.totalWork = GetChainWork();
    stats.bestBlockHash = bestBlock ? bestBlock->GetBlockHash() : Hash256{};
    stats.totalSupply = CalculateTotalSupply(stats.height);
    stats.utxoCount = utxos.GetSize();
    stats.mempoolSize = mempool.Size();

    return stats;
}

bool Blockchain::ValidateChain() const {
    std::lock_guard<std::mutex> lock(mutex);

    LOG_INFO("Blockchain", "Validating entire blockchain...");

    // Start from genesis
    const BlockIndex* current = genesisBlock;

    while (current) {
        if (!current->block) {
            LOG_ERROR("Blockchain", "Block data missing at height " +
                     std::to_string(current->height));
            return false;
        }

        // Validate block
        auto result = ConsensusValidator::ValidateBlock(
            *current->block, current->prev, current->height, *this, utxos);

        if (!result) {
            LOG_ERROR("Blockchain", "Block validation failed at height " +
                     std::to_string(current->height) + ": " + result.error);
            return false;
        }

        // Move to next
        if (current->next.empty()) {
            break;
        }

        // Follow main chain
        current = nullptr;
        for (auto* next : current->next) {
            if (next->isMainChain) {
                current = next;
                break;
            }
        }
    }

    LOG_INFO("Blockchain", "Blockchain validation successful");
    return true;
}

std::vector<Hash256> Blockchain::GetBlockLocator(const BlockIndex* startBlock) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Hash256> locator;

    const BlockIndex* current = startBlock ? startBlock : bestBlock;
    if (!current) {
        return locator;
    }

    int step = 1;
    while (current) {
        locator.push_back(current->GetBlockHash());

        // Exponentially increasing steps
        for (int i = 0; i < step && current->prev; ++i) {
            current = current->prev;
        }

        if (locator.size() > 10) {
            step *= 2;
        }

        if (!current->prev) {
            break;
        }
    }

    return locator;
}

const BlockIndex* Blockchain::FindCommonAncestor(const std::vector<Hash256>& locator) const {
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto& hash : locator) {
        const BlockIndex* index = GetBlockIndex(hash);
        if (index && index->isMainChain) {
            return index;
        }
    }

    return genesisBlock;
}

Amount Blockchain::CalculateTotalSupply(BlockHeight height) const {
    // Calculate total supply based on block rewards
    Amount total = 0;

    for (BlockHeight h = 0; h <= height; ++h) {
        total += GetBlockReward(h);
    }

    return total;
}

} // namespace dinari
