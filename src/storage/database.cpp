#include "database.h"
#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <iostream>

namespace dinari {

// Database implementation
bool Database::Open(const std::string& path, bool createIfMissing) {
    leveldb::Options options;
    options.create_if_missing = createIfMissing;
    options.compression = leveldb::kSnappyCompression;
    options.write_buffer_size = 64 * 1024 * 1024; // 64MB write buffer
    options.max_open_files = 1000;
    options.block_cache = nullptr; // Will use default

    leveldb::DB* dbPtr = nullptr;
    leveldb::Status status = leveldb::DB::Open(options, path, &dbPtr);

    if (!status.ok()) {
        std::cerr << "Failed to open database: " << status.ToString() << std::endl;
        return false;
    }

    db.reset(dbPtr);
    return true;
}

void Database::Close() {
    db.reset();
}

Database::~Database() {
    Close();
}

bool Database::Write(const bytes& key, const bytes& value) {
    if (!db) return false;

    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Slice valueSlice(reinterpret_cast<const char*>(value.data()), value.size());

    leveldb::WriteOptions options;
    options.sync = false; // Set to true for critical operations

    leveldb::Status status = db->Put(options, keySlice, valueSlice);
    return status.ok();
}

std::optional<bytes> Database::Read(const bytes& key) const {
    if (!db) return std::nullopt;

    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());

    std::string value;
    leveldb::ReadOptions options;
    leveldb::Status status = db->Get(options, keySlice, &value);

    if (!status.ok()) {
        return std::nullopt;
    }

    bytes result(value.begin(), value.end());
    return result;
}

bool Database::Delete(const bytes& key) {
    if (!db) return false;

    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());

    leveldb::WriteOptions options;
    options.sync = false;

    leveldb::Status status = db->Delete(options, keySlice);
    return status.ok();
}

bool Database::Exists(const bytes& key) const {
    return Read(key).has_value();
}

// Batch implementation
Database::Batch::Batch() : batch(std::make_unique<leveldb::WriteBatch>()) {}

Database::Batch::~Batch() = default;

void Database::Batch::Put(const bytes& key, const bytes& value) {
    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    leveldb::Slice valueSlice(reinterpret_cast<const char*>(value.data()), value.size());
    batch->Put(keySlice, valueSlice);
}

void Database::Batch::Delete(const bytes& key) {
    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    batch->Delete(keySlice);
}

void Database::Batch::Clear() {
    batch->Clear();
}

bool Database::WriteBatch(const Batch& batch) {
    if (!db) return false;

    leveldb::WriteOptions options;
    options.sync = true; // Batch writes are synced for consistency

    leveldb::Status status = db->Write(options, batch.batch.get());
    return status.ok();
}

// Iterator implementation
Database::Iterator::Iterator(leveldb::Iterator* it) : iter(it) {}

Database::Iterator::~Iterator() = default;

bool Database::Iterator::Valid() const {
    return iter && iter->Valid();
}

void Database::Iterator::SeekToFirst() {
    if (iter) iter->SeekToFirst();
}

void Database::Iterator::SeekToLast() {
    if (iter) iter->SeekToLast();
}

void Database::Iterator::Seek(const bytes& key) {
    if (!iter) return;
    leveldb::Slice keySlice(reinterpret_cast<const char*>(key.data()), key.size());
    iter->Seek(keySlice);
}

void Database::Iterator::Next() {
    if (iter) iter->Next();
}

void Database::Iterator::Prev() {
    if (iter) iter->Prev();
}

bytes Database::Iterator::Key() const {
    if (!iter || !iter->Valid()) return bytes();

    leveldb::Slice key = iter->key();
    return bytes(key.data(), key.data() + key.size());
}

bytes Database::Iterator::Value() const {
    if (!iter || !iter->Valid()) return bytes();

    leveldb::Slice value = iter->value();
    return bytes(value.data(), value.data() + value.size());
}

std::unique_ptr<Database::Iterator> Database::NewIterator() const {
    if (!db) return nullptr;

    leveldb::ReadOptions options;
    return std::unique_ptr<Iterator>(new Iterator(db->NewIterator(options)));
}

std::string Database::GetStats() const {
    if (!db) return "Database not open";

    std::string stats;
    if (db->GetProperty("leveldb.stats", &stats)) {
        return stats;
    }
    return "No statistics available";
}

void Database::Compact() {
    if (!db) return;
    db->CompactRange(nullptr, nullptr);
}

} // namespace dinari
