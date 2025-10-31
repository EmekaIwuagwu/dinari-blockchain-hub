#include "txindex.h"
#include "serialization/serializer.h"
#include <filesystem>

namespace dinari {

bool TxIndex::Open(const std::string& dataDir) {
    // Create txindex subdirectory
    std::filesystem::path txindexPath = std::filesystem::path(dataDir) / "txindex";
    std::filesystem::create_directories(txindexPath);

    db = std::make_unique<Database>();
    return db->Open(txindexPath.string(), true);
}

void TxIndex::Close() {
    if (db) {
        db->Close();
    }
}

bytes TxIndex::MakeTxLocationKey(const Hash256& txid) const {
    bytes key(1 + txid.size());
    key[0] = PREFIX_TX_LOCATION;
    std::copy(txid.begin(), txid.end(), key.begin() + 1);
    return key;
}

bytes TxIndex::MakeUTXOKey(const OutPoint& outpoint) const {
    bytes key(1 + 32 + 4);  // prefix + txid + vout
    key[0] = PREFIX_UTXO;

    std::copy(outpoint.txid.begin(), outpoint.txid.end(), key.begin() + 1);

    for (size_t i = 0; i < 4; ++i) {
        key[1 + 32 + i] = static_cast<byte>((outpoint.vout >> (8 * i)) & 0xFF);
    }

    return key;
}

bytes TxIndex::MakeAddressUTXOKey(const Address& address, const OutPoint& outpoint) const {
    bytes key(1 + address.size() + 32 + 4);
    key[0] = PREFIX_ADDR_UTXO;

    size_t offset = 1;
    std::copy(address.begin(), address.end(), key.begin() + offset);
    offset += address.size();

    std::copy(outpoint.txid.begin(), outpoint.txid.end(), key.begin() + offset);
    offset += 32;

    for (size_t i = 0; i < 4; ++i) {
        key[offset + i] = static_cast<byte>((outpoint.vout >> (8 * i)) & 0xFF);
    }

    return key;
}

bytes TxIndex::MakeUTXOCountKey() const {
    return bytes{PREFIX_UTXO_COUNT};
}

bool TxIndex::IndexTransaction(const Transaction& tx, BlockHeight height, uint32_t txIndex) {
    if (!db || !db->IsOpen()) return false;

    Hash256 txid = tx.GetHash();

    // Serialize location
    bytes locationData(sizeof(BlockHeight) + sizeof(uint32_t));
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        locationData[i] = static_cast<byte>((height >> (8 * i)) & 0xFF);
    }
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        locationData[sizeof(BlockHeight) + i] = static_cast<byte>((txIndex >> (8 * i)) & 0xFF);
    }

    return db->Write(MakeTxLocationKey(txid), locationData);
}

std::optional<TxLocation> TxIndex::GetTxLocation(const Hash256& txid) const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto locationData = db->Read(MakeTxLocationKey(txid));
    if (!locationData || locationData->size() != sizeof(BlockHeight) + sizeof(uint32_t)) {
        return std::nullopt;
    }

    BlockHeight height = 0;
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        height |= static_cast<BlockHeight>((*locationData)[i]) << (8 * i);
    }

    uint32_t txIndex = 0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        txIndex |= static_cast<uint32_t>((*locationData)[sizeof(BlockHeight) + i]) << (8 * i);
    }

    return TxLocation(height, txIndex);
}

bool TxIndex::AddUTXO(const OutPoint& outpoint, const TxOut& output) {
    if (!db || !db->IsOpen()) return false;

    // Serialize output
    bytes outputData = Serializer::Serialize(output);

    Database::Batch batch;

    // Add to main UTXO set
    batch.Put(MakeUTXOKey(outpoint), outputData);

    // Add to address index
    batch.Put(MakeAddressUTXOKey(output.scriptPubKey, outpoint), outputData);

    bool success = db->WriteBatch(batch);

    if (success) {
        UpdateUTXOCount(1);
    }

    return success;
}

bool TxIndex::RemoveUTXO(const OutPoint& outpoint) {
    if (!db || !db->IsOpen()) return false;

    // First read the output to get the address
    auto outputData = db->Read(MakeUTXOKey(outpoint));
    if (!outputData) return false;

    try {
        TxOut output = Serializer::Deserialize<TxOut>(*outputData);

        Database::Batch batch;

        // Remove from main UTXO set
        batch.Delete(MakeUTXOKey(outpoint));

        // Remove from address index
        batch.Delete(MakeAddressUTXOKey(output.scriptPubKey, outpoint));

        bool success = db->WriteBatch(batch);

        if (success) {
            UpdateUTXOCount(-1);
        }

        return success;
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<TxOut> TxIndex::GetUTXO(const OutPoint& outpoint) const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto outputData = db->Read(MakeUTXOKey(outpoint));
    if (!outputData) return std::nullopt;

    try {
        return Serializer::Deserialize<TxOut>(*outputData);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

bool TxIndex::HasUTXO(const OutPoint& outpoint) const {
    if (!db || !db->IsOpen()) return false;
    return db->Exists(MakeUTXOKey(outpoint));
}

std::vector<std::pair<OutPoint, TxOut>> TxIndex::GetUTXOsForAddress(const Address& address) const {
    std::vector<std::pair<OutPoint, TxOut>> utxos;

    if (!db || !db->IsOpen()) return utxos;

    // Create iterator to scan address UTXOs
    auto iter = db->NewIterator();
    if (!iter) return utxos;

    // Seek to first UTXO for this address
    bytes prefixKey(1 + address.size());
    prefixKey[0] = PREFIX_ADDR_UTXO;
    std::copy(address.begin(), address.end(), prefixKey.begin() + 1);

    iter->Seek(prefixKey);

    while (iter->Valid()) {
        bytes key = iter->Key();

        // Check if still in address range
        if (key.size() < prefixKey.size() ||
            !std::equal(prefixKey.begin(), prefixKey.end(), key.begin())) {
            break;
        }

        // Extract outpoint from key
        if (key.size() >= 1 + address.size() + 32 + 4) {
            OutPoint outpoint;

            size_t offset = 1 + address.size();
            std::copy(key.begin() + offset, key.begin() + offset + 32, outpoint.txid.begin());
            offset += 32;

            outpoint.vout = 0;
            for (size_t i = 0; i < 4; ++i) {
                outpoint.vout |= static_cast<uint32_t>(key[offset + i]) << (8 * i);
            }

            // Deserialize output
            try {
                bytes outputData = iter->Value();
                TxOut output = Serializer::Deserialize<TxOut>(outputData);
                utxos.emplace_back(outpoint, output);
            } catch (const std::exception&) {
                // Skip invalid entries
            }
        }

        iter->Next();
    }

    return utxos;
}

size_t TxIndex::GetUTXOSetSize() const {
    if (!db || !db->IsOpen()) return 0;

    auto countData = db->Read(MakeUTXOCountKey());
    if (!countData || countData->size() != sizeof(size_t)) {
        return 0;
    }

    size_t count = 0;
    for (size_t i = 0; i < sizeof(size_t); ++i) {
        count |= static_cast<size_t>((*countData)[i]) << (8 * i);
    }

    return count;
}

bool TxIndex::UpdateUTXOCount(int delta) {
    if (!db || !db->IsOpen()) return false;

    size_t count = GetUTXOSetSize();

    if (delta < 0 && count < static_cast<size_t>(-delta)) {
        count = 0;
    } else {
        count = static_cast<size_t>(static_cast<int64_t>(count) + delta);
    }

    bytes countData(sizeof(size_t));
    for (size_t i = 0; i < sizeof(size_t); ++i) {
        countData[i] = static_cast<byte>((count >> (8 * i)) & 0xFF);
    }

    return db->Write(MakeUTXOCountKey(), countData);
}

bool TxIndex::ApplyUTXOBatch(const UTXOBatch& batch) {
    if (!db || !db->IsOpen()) return false;

    Database::Batch dbBatch;

    // Apply additions
    for (const auto& [outpoint, output] : batch.additions) {
        bytes outputData = Serializer::Serialize(output);
        dbBatch.Put(MakeUTXOKey(outpoint), outputData);
        dbBatch.Put(MakeAddressUTXOKey(output.scriptPubKey, outpoint), outputData);
    }

    // Apply removals
    for (const auto& outpoint : batch.removals) {
        // Read output to get address
        auto outputData = db->Read(MakeUTXOKey(outpoint));
        if (outputData) {
            try {
                TxOut output = Serializer::Deserialize<TxOut>(*outputData);
                dbBatch.Delete(MakeUTXOKey(outpoint));
                dbBatch.Delete(MakeAddressUTXOKey(output.scriptPubKey, outpoint));
            } catch (const std::exception&) {
                // Skip invalid entries
            }
        }
    }

    bool success = db->WriteBatch(dbBatch);

    if (success) {
        int delta = static_cast<int>(batch.additions.size()) - static_cast<int>(batch.removals.size());
        UpdateUTXOCount(delta);
    }

    return success;
}

bool TxIndex::RemoveTransaction(const Hash256& txid) {
    if (!db || !db->IsOpen()) return false;
    return db->Delete(MakeTxLocationKey(txid));
}

std::string TxIndex::GetStats() const {
    if (!db || !db->IsOpen()) return "TxIndex not open";
    return db->GetStats();
}

void TxIndex::Compact() {
    if (db && db->IsOpen()) {
        db->Compact();
    }
}

} // namespace dinari
