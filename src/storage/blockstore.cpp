#include "blockstore.h"
#include "serialization/serializer.h"
#include <filesystem>

namespace dinari {

bool BlockStore::Open(const std::string& dataDir) {
    // Create blocks subdirectory
    std::filesystem::path blocksPath = std::filesystem::path(dataDir) / "blocks";
    std::filesystem::create_directories(blocksPath);

    db = std::make_unique<Database>();
    return db->Open(blocksPath.string(), true);
}

void BlockStore::Close() {
    if (db) {
        db->Close();
    }
}

bytes BlockStore::MakeBlockKey(BlockHeight height) const {
    bytes key(1 + sizeof(BlockHeight));
    key[0] = PREFIX_BLOCK;

    // Store height in big-endian for proper ordering
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        key[1 + i] = static_cast<byte>((height >> (8 * (sizeof(BlockHeight) - 1 - i))) & 0xFF);
    }

    return key;
}

bytes BlockStore::MakeHashKey(const Hash256& hash) const {
    bytes key(1 + hash.size());
    key[0] = PREFIX_HASH;
    std::copy(hash.begin(), hash.end(), key.begin() + 1);
    return key;
}

bytes BlockStore::MakeBestKey() const {
    return bytes{PREFIX_BEST};
}

bytes BlockStore::MakeHeightKey() const {
    return bytes{PREFIX_HEIGHT};
}

bytes BlockStore::MakeWorkKey() const {
    return bytes{PREFIX_WORK};
}

bool BlockStore::WriteBlock(const Block& block, BlockHeight height) {
    if (!db || !db->IsOpen()) return false;

    // Serialize block
    bytes blockData = Serializer::Serialize(block);

    // Create batch for atomic write
    Database::Batch batch;

    // Write block data: b<height> → block
    batch.Put(MakeBlockKey(height), blockData);

    // Write hash index: h<hash> → height
    Hash256 blockHash = block.GetHash();
    bytes heightBytes(sizeof(BlockHeight));
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        heightBytes[i] = static_cast<byte>((height >> (8 * i)) & 0xFF);
    }
    batch.Put(MakeHashKey(blockHash), heightBytes);

    return db->WriteBatch(batch);
}

std::optional<Block> BlockStore::ReadBlock(BlockHeight height) const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto blockData = db->Read(MakeBlockKey(height));
    if (!blockData) return std::nullopt;

    try {
        return Serializer::Deserialize<Block>(*blockData);
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

std::optional<BlockHeight> BlockStore::GetBlockHeight(const Hash256& hash) const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto heightBytes = db->Read(MakeHashKey(hash));
    if (!heightBytes || heightBytes->size() != sizeof(BlockHeight)) {
        return std::nullopt;
    }

    BlockHeight height = 0;
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        height |= static_cast<BlockHeight>((*heightBytes)[i]) << (8 * i);
    }

    return height;
}

std::optional<Block> BlockStore::ReadBlockByHash(const Hash256& hash) const {
    auto height = GetBlockHeight(hash);
    if (!height) return std::nullopt;

    return ReadBlock(*height);
}

bool BlockStore::HasBlock(const Hash256& hash) const {
    if (!db || !db->IsOpen()) return false;
    return db->Exists(MakeHashKey(hash));
}

std::optional<Hash256> BlockStore::GetBestBlockHash() const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto hashBytes = db->Read(MakeBestKey());
    if (!hashBytes || hashBytes->size() != 32) {
        return std::nullopt;
    }

    Hash256 hash;
    std::copy(hashBytes->begin(), hashBytes->end(), hash.begin());
    return hash;
}

bool BlockStore::SetBestBlockHash(const Hash256& hash) {
    if (!db || !db->IsOpen()) return false;

    bytes hashBytes(hash.begin(), hash.end());
    return db->Write(MakeBestKey(), hashBytes);
}

std::optional<BlockHeight> BlockStore::GetChainHeight() const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto heightBytes = db->Read(MakeHeightKey());
    if (!heightBytes || heightBytes->size() != sizeof(BlockHeight)) {
        return std::nullopt;
    }

    BlockHeight height = 0;
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        height |= static_cast<BlockHeight>((*heightBytes)[i]) << (8 * i);
    }

    return height;
}

bool BlockStore::SetChainHeight(BlockHeight height) {
    if (!db || !db->IsOpen()) return false;

    bytes heightBytes(sizeof(BlockHeight));
    for (size_t i = 0; i < sizeof(BlockHeight); ++i) {
        heightBytes[i] = static_cast<byte>((height >> (8 * i)) & 0xFF);
    }

    return db->Write(MakeHeightKey(), heightBytes);
}

std::optional<uint64_t> BlockStore::GetTotalWork() const {
    if (!db || !db->IsOpen()) return std::nullopt;

    auto workBytes = db->Read(MakeWorkKey());
    if (!workBytes || workBytes->size() != 8) {
        return std::nullopt;
    }

    uint64_t work = 0;
    for (size_t i = 0; i < 8; ++i) {
        work |= uint64_t((*workBytes)[i]) << (8 * i);
    }

    return work;
}

bool BlockStore::SetTotalWork(const uint64_t& work) {
    if (!db || !db->IsOpen()) return false;

    bytes workBytes(8);
    for (size_t i = 0; i < 8; ++i) {
        workBytes[i] = static_cast<byte>((work >> (8 * i)) & 0xFF);
    }

    return db->Write(MakeWorkKey(), workBytes);
}

bool BlockStore::DeleteBlock(BlockHeight height) {
    if (!db || !db->IsOpen()) return false;

    // Read block to get hash
    auto block = ReadBlock(height);
    if (!block) return false;

    Hash256 blockHash = block->GetHash();

    // Create batch for atomic delete
    Database::Batch batch;
    batch.Delete(MakeBlockKey(height));
    batch.Delete(MakeHashKey(blockHash));

    return db->WriteBatch(batch);
}

std::string BlockStore::GetStats() const {
    if (!db || !db->IsOpen()) return "BlockStore not open";
    return db->GetStats();
}

void BlockStore::Compact() {
    if (db && db->IsOpen()) {
        db->Compact();
    }
}

} // namespace dinari
