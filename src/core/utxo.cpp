#include "utxo.h"
#include "util/logger.h"
#include <algorithm>
#include <random>

namespace dinari {

// UTXOEntry implementation

bool UTXOEntry::IsMature(BlockHeight currentHeight) const {
    if (!isCoinbase) {
        return true;  // Non-coinbase outputs are always mature
    }

    // Coinbase outputs require COINBASE_MATURITY confirmations
    return (currentHeight - height) >= COINBASE_MATURITY;
}

bool UTXOEntry::IsSpendable(BlockHeight currentHeight) const {
    return IsMature(currentHeight);
}

void UTXOEntry::SerializeImpl(Serializer& s) const {
    output.SerializeImpl(s);
    s.WriteUInt32(height);
    s.WriteBool(isCoinbase);
}

void UTXOEntry::DeserializeImpl(Deserializer& d) {
    output.DeserializeImpl(d);
    height = d.ReadUInt32();
    isCoinbase = d.ReadBool();
}

// UTXOSet implementation

UTXOSet::UTXOSet() {
}

UTXOSet::~UTXOSet() {
}

void UTXOSet::AddUTXO(const OutPoint& outpoint, const UTXOEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex);

    utxos[outpoint] = entry;

    // Update address index
    if (auto addr = ExtractAddressFromScript(entry.output.scriptPubKey)) {
        addressIndex[*addr].push_back(outpoint);
    }
}

void UTXOSet::AddUTXO(const OutPoint& outpoint, const TxOut& output,
                     BlockHeight height, bool isCoinbase) {
    AddUTXO(outpoint, UTXOEntry(output, height, isCoinbase));
}

bool UTXOSet::RemoveUTXO(const OutPoint& outpoint) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = utxos.find(outpoint);
    if (it == utxos.end()) {
        return false;
    }

    // Remove from address index
    if (auto addr = ExtractAddressFromScript(it->second.output.scriptPubKey)) {
        auto& vec = addressIndex[*addr];
        vec.erase(std::remove(vec.begin(), vec.end(), outpoint), vec.end());
        if (vec.empty()) {
            addressIndex.erase(*addr);
        }
    }

    utxos.erase(it);
    return true;
}

bool UTXOSet::HasUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(mutex);
    return utxos.find(outpoint) != utxos.end();
}

const TxOut* UTXOSet::GetUTXO(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = utxos.find(outpoint);
    if (it == utxos.end()) {
        return nullptr;
    }

    return &it->second.output;
}

const UTXOEntry* UTXOSet::GetUTXOEntry(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = utxos.find(outpoint);
    if (it == utxos.end()) {
        return nullptr;
    }

    return &it->second;
}

BlockHeight UTXOSet::GetUTXOHeight(const OutPoint& outpoint) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = utxos.find(outpoint);
    if (it == utxos.end()) {
        return 0;
    }

    return it->second.height;
}

bool UTXOSet::ApplyTransaction(const Transaction& tx, BlockHeight height) {
    std::lock_guard<std::mutex> lock(mutex);

    // Remove spent outputs (inputs)
    if (!tx.IsCoinbase()) {
        for (const auto& input : tx.inputs) {
            auto it = utxos.find(input.prevOut);
            if (it == utxos.end()) {
                LOG_ERROR("UTXO", "Attempting to spend non-existent UTXO: " +
                         input.prevOut.ToString());
                return false;
            }
            utxos.erase(it);
        }
    }

    // Add new outputs
    Hash256 txHash = tx.GetHash();
    for (size_t i = 0; i < tx.outputs.size(); ++i) {
        OutPoint outpoint(txHash, static_cast<TxOutIndex>(i));
        UTXOEntry entry(tx.outputs[i], height, tx.IsCoinbase());
        utxos[outpoint] = entry;

        // Update address index
        if (auto addr = ExtractAddressFromScript(tx.outputs[i].scriptPubKey)) {
            addressIndex[*addr].push_back(outpoint);
        }
    }

    return true;
}

bool UTXOSet::RevertTransaction(const Transaction& tx,
                                const std::map<OutPoint, UTXOEntry>& previousUTXOs) {
    std::lock_guard<std::mutex> lock(mutex);

    // Remove outputs that were added
    Hash256 txHash = tx.GetHash();
    for (size_t i = 0; i < tx.outputs.size(); ++i) {
        OutPoint outpoint(txHash, static_cast<TxOutIndex>(i));
        utxos.erase(outpoint);
    }

    // Restore inputs that were spent
    if (!tx.IsCoinbase()) {
        for (const auto& input : tx.inputs) {
            auto it = previousUTXOs.find(input.prevOut);
            if (it != previousUTXOs.end()) {
                utxos[input.prevOut] = it->second;
            }
        }
    }

    return true;
}

std::vector<std::pair<OutPoint, UTXOEntry>> UTXOSet::GetUTXOsForAddress(
    const Hash160& addressHash) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::pair<OutPoint, UTXOEntry>> result;

    auto it = addressIndex.find(addressHash);
    if (it != addressIndex.end()) {
        for (const auto& outpoint : it->second) {
            auto utxoIt = utxos.find(outpoint);
            if (utxoIt != utxos.end()) {
                result.emplace_back(outpoint, utxoIt->second);
            }
        }
    }

    return result;
}

size_t UTXOSet::GetSize() const {
    std::lock_guard<std::mutex> lock(mutex);
    return utxos.size();
}

Amount UTXOSet::GetTotalValue() const {
    std::lock_guard<std::mutex> lock(mutex);

    Amount total = 0;
    for (const auto& [outpoint, entry] : utxos) {
        total += entry.output.value;
    }
    return total;
}

void UTXOSet::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    utxos.clear();
    addressIndex.clear();
}

bool UTXOSet::Flush() {
    // Note: Database persistence should be implemented for production use (e.g., LevelDB/RocksDB)
    LOG_INFO("UTXO", "Flushing UTXO set to disk (not yet implemented)");
    return true;
}

bool UTXOSet::Load() {
    // Note: Database loading should be implemented for production use (e.g., LevelDB/RocksDB)
    LOG_INFO("UTXO", "Loading UTXO set from disk (not yet implemented)");
    return true;
}

UTXOSet::Stats UTXOSet::GetStats(BlockHeight currentHeight) const {
    std::lock_guard<std::mutex> lock(mutex);

    Stats stats{};
    stats.totalUTXOs = utxos.size();

    for (const auto& [outpoint, entry] : utxos) {
        stats.totalValue += entry.output.value;

        if (entry.isCoinbase) {
            stats.coinbaseUTXOs++;
        } else {
            stats.regularUTXOs++;
        }

        if (entry.IsMature(currentHeight)) {
            stats.matureUTXOs++;
        } else {
            stats.immatureUTXOs++;
        }
    }

    return stats;
}

void UTXOSet::Prune(BlockHeight keepDepth [[maybe_unused]]) {
    // Note: UTXO pruning can be implemented for optimization after initial deployment
    // This would typically be used in a pruned node
    LOG_INFO("UTXO", "Pruning UTXO set (not yet implemented)");
}

bool UTXOSet::ValidateTransaction(const Transaction& tx, BlockHeight currentHeight) const {
    std::lock_guard<std::mutex> lock(mutex);

    // Check basic validity
    if (!tx.IsValid()) {
        return false;
    }

    // Coinbase transactions are always valid (checked separately)
    if (tx.IsCoinbase()) {
        return true;
    }

    Amount inputValue = 0;

    // Check all inputs exist and are spendable
    for (const auto& input : tx.inputs) {
        auto it = utxos.find(input.prevOut);
        if (it == utxos.end()) {
            LOG_ERROR("UTXO", "Input references non-existent UTXO: " +
                     input.prevOut.ToString());
            return false;
        }

        const UTXOEntry& entry = it->second;

        // Check maturity (coinbase outputs need 100 confirmations)
        if (!entry.IsSpendable(currentHeight)) {
            LOG_ERROR("UTXO", "Input references immature coinbase output");
            return false;
        }

        inputValue += entry.output.value;
    }

    // Check that inputs >= outputs (fee can be positive)
    Amount outputValue = tx.GetOutputValue();
    if (inputValue < outputValue) {
        LOG_ERROR("UTXO", "Transaction outputs exceed inputs");
        return false;
    }

    return true;
}

void UTXOSet::BuildAddressIndex() {
    std::lock_guard<std::mutex> lock(mutex);

    addressIndex.clear();

    for (const auto& [outpoint, entry] : utxos) {
        if (auto addr = ExtractAddressFromScript(entry.output.scriptPubKey)) {
            addressIndex[*addr].push_back(outpoint);
        }
    }
}

std::optional<Hash160> UTXOSet::ExtractAddressFromScript(const bytes& script) const {
    // P2PKH (Pay to Public Key Hash)
    // Format: OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
    if (script.size() == 25 &&
        script[0] == 0x76 &&  // OP_DUP
        script[1] == 0xa9 &&  // OP_HASH160
        script[2] == 20 &&    // Push 20 bytes
        script[23] == 0x88 && // OP_EQUALVERIFY
        script[24] == 0xac) { // OP_CHECKSIG

        Hash160 hash;
        std::copy(script.begin() + 3, script.begin() + 23, hash.begin());
        return hash;
    }

    // P2SH (Pay to Script Hash)
    // Format: OP_HASH160 <20 bytes> OP_EQUAL
    if (script.size() == 23 &&
        script[0] == 0xa9 &&  // OP_HASH160
        script[1] == 20 &&    // Push 20 bytes
        script[22] == 0x87) { // OP_EQUAL

        Hash160 hash;
        std::copy(script.begin() + 2, script.begin() + 22, hash.begin());
        return hash;
    }

    // P2WPKH (Pay to Witness Public Key Hash - SegWit v0)
    // Format: OP_0 <20 bytes>
    if (script.size() == 22 &&
        script[0] == 0x00 &&  // OP_0 (witness version 0)
        script[1] == 20) {    // Push 20 bytes

        Hash160 hash;
        std::copy(script.begin() + 2, script.begin() + 22, hash.begin());
        return hash;
    }

    // P2WSH (Pay to Witness Script Hash - SegWit v0)
    // Format: OP_0 <32 bytes>
    // Note: For P2WSH, we return the first 20 bytes of the 32-byte hash
    if (script.size() == 34 &&
        script[0] == 0x00 &&  // OP_0 (witness version 0)
        script[1] == 32) {    // Push 32 bytes

        Hash160 hash;
        std::copy(script.begin() + 2, script.begin() + 22, hash.begin());
        return hash;
    }

    // Unknown script type
    return std::nullopt;
}

// UTXOCache implementation

UTXOCache::UTXOCache(UTXOSet& baseSet, size_t maxCacheSize)
    : baseSet(baseSet), maxCacheSize(maxCacheSize), hits(0), misses(0) {
}

const TxOut* UTXOCache::GetUTXO(const OutPoint& outpoint) {
    std::lock_guard<std::mutex> lock(mutex);

    // Check cache first
    auto it = cache.find(outpoint);
    if (it != cache.end()) {
        hits++;
        return &it->second->output;
    }

    // Cache miss, query base set
    misses++;
    const UTXOEntry* entry = baseSet.GetUTXOEntry(outpoint);
    if (!entry) {
        return nullptr;
    }

    // Add to cache (if not full)
    if (cache.size() < maxCacheSize) {
        cache[outpoint] = std::make_unique<UTXOEntry>(*entry);
    }

    return &entry->output;
}

bool UTXOCache::HasUTXO(const OutPoint& outpoint) {
    return GetUTXO(outpoint) != nullptr;
}

void UTXOCache::Flush() {
    std::lock_guard<std::mutex> lock(mutex);
    cache.clear();
}

void UTXOCache::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    cache.clear();
    hits = 0;
    misses = 0;
}

UTXOCache::CacheStats UTXOCache::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex);

    CacheStats stats;
    stats.hits = hits;
    stats.misses = misses;
    stats.size = cache.size();

    size_t total = hits + misses;
    stats.hitRate = total > 0 ? static_cast<double>(hits) / total : 0.0;

    return stats;
}

// CoinSelector implementation

CoinSelector::CoinSelector(const UTXOSet& utxos) : utxos(utxos) {
}

CoinSelector::SelectionResult CoinSelector::SelectCoins(
    Amount targetAmount, Amount feeRate,
    const std::vector<OutPoint>& availableCoins,
    Strategy strategy) {

    switch (strategy) {
        case Strategy::LARGEST_FIRST:
            return SelectLargestFirst(targetAmount, feeRate, availableCoins);
        case Strategy::SMALLEST_FIRST:
            return SelectSmallestFirst(targetAmount, feeRate, availableCoins);
        case Strategy::RANDOM:
            return SelectRandom(targetAmount, feeRate, availableCoins);
        case Strategy::BRANCH_AND_BOUND:
            return SelectBranchAndBound(targetAmount, feeRate, availableCoins);
        default:
            return SelectionResult{};
    }
}

Amount CoinSelector::CalculateFee(size_t txSize, Amount feeRate) {
    return (txSize * feeRate) / 1000;  // feeRate is per KB
}

size_t CoinSelector::EstimateTransactionSize(size_t numInputs, size_t numOutputs) {
    // Rough estimate:
    // Version: 4 bytes
    // Input count: 1-9 bytes (VarInt)
    // Each input: ~148 bytes (outpoint + scriptSig + sequence)
    // Output count: 1-9 bytes (VarInt)
    // Each output: ~34 bytes (value + scriptPubKey)
    // LockTime: 4 bytes

    return 10 + (numInputs * 148) + (numOutputs * 34);
}

CoinSelector::SelectionResult CoinSelector::SelectLargestFirst(
    Amount target, Amount feeRate, std::vector<OutPoint> coins) {

    SelectionResult result;
    result.success = false;

    // Sort coins by value (largest first)
    std::sort(coins.begin(), coins.end(),
        [this](const OutPoint& a, const OutPoint& b) {
            const TxOut* utxoA = utxos.GetUTXO(a);
            const TxOut* utxoB = utxos.GetUTXO(b);
            if (!utxoA || !utxoB) return false;
            return utxoA->value > utxoB->value;
        });

    Amount total = 0;
    size_t numOutputs = 2;  // Assume payment + change

    for (const auto& coin : coins) {
        const TxOut* utxo = utxos.GetUTXO(coin);
        if (!utxo) continue;

        result.selected.push_back(coin);
        total += utxo->value;

        // Calculate fee
        size_t txSize = EstimateTransactionSize(result.selected.size(), numOutputs);
        Amount fee = CalculateFee(txSize, feeRate);

        // Check if we have enough
        if (total >= target + fee) {
            result.totalValue = total;
            result.fee = fee;
            result.change = total - target - fee;
            result.success = true;
            return result;
        }
    }

    result.error = "Insufficient funds";
    return result;
}

CoinSelector::SelectionResult CoinSelector::SelectSmallestFirst(
    Amount target, Amount feeRate, std::vector<OutPoint> coins) {

    // Sort coins by value (smallest first)
    std::sort(coins.begin(), coins.end(),
        [this](const OutPoint& a, const OutPoint& b) {
            const TxOut* utxoA = utxos.GetUTXO(a);
            const TxOut* utxoB = utxos.GetUTXO(b);
            if (!utxoA || !utxoB) return false;
            return utxoA->value < utxoB->value;
        });

    return SelectLargestFirst(target, feeRate, coins);  // Same logic after sorting
}

CoinSelector::SelectionResult CoinSelector::SelectRandom(
    Amount target, Amount feeRate, std::vector<OutPoint> coins) {

    // Shuffle coins randomly
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(coins.begin(), coins.end(), g);

    return SelectLargestFirst(target, feeRate, coins);  // Same logic after shuffling
}

CoinSelector::SelectionResult CoinSelector::SelectBranchAndBound(
    Amount target, Amount feeRate, std::vector<OutPoint> coins) {
    // Note: Branch and Bound coin selection algorithm can be added for optimization
    // For now, fall back to largest first
    LOG_WARNING("CoinSelector", "Branch and Bound not implemented, using Largest First");
    return SelectLargestFirst(target, feeRate, coins);
}

} // namespace dinari
