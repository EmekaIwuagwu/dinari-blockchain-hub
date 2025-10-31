#ifndef DINARI_STORAGE_DATABASE_H
#define DINARI_STORAGE_DATABASE_H

#include "dinari/types.h"
#include <memory>
#include <string>
#include <optional>

namespace leveldb {
    class DB;
    class WriteBatch;
    class Iterator;
}

namespace dinari {

/**
 * @brief Database wrapper for LevelDB
 *
 * Provides persistent key-value storage for blockchain data
 */
class Database {
public:
    /**
     * @brief Open database at specified path
     * @param path Database directory path
     * @param createIfMissing Create database if it doesn't exist
     * @return true if successful
     */
    bool Open(const std::string& path, bool createIfMissing = true);

    /**
     * @brief Close database
     */
    void Close();

    /**
     * @brief Check if database is open
     */
    bool IsOpen() const { return db != nullptr; }

    /**
     * @brief Write key-value pair
     */
    bool Write(const bytes& key, const bytes& value);

    /**
     * @brief Read value for key
     * @return Value if key exists, nullopt otherwise
     */
    std::optional<bytes> Read(const bytes& key) const;

    /**
     * @brief Delete key
     */
    bool Delete(const bytes& key);

    /**
     * @brief Check if key exists
     */
    bool Exists(const bytes& key) const;

    /**
     * @brief Batch write operations
     */
    class Batch {
    public:
        Batch();
        ~Batch();

        void Put(const bytes& key, const bytes& value);
        void Delete(const bytes& key);
        void Clear();

    private:
        friend class Database;
        std::unique_ptr<leveldb::WriteBatch> batch;
    };

    /**
     * @brief Write batch atomically
     */
    bool WriteBatch(const Batch& batch);

    /**
     * @brief Iterator for scanning database
     */
    class Iterator {
    public:
        ~Iterator();

        bool Valid() const;
        void SeekToFirst();
        void SeekToLast();
        void Seek(const bytes& key);
        void Next();
        void Prev();

        bytes Key() const;
        bytes Value() const;

    private:
        friend class Database;
        Iterator(leveldb::Iterator* it);
        std::unique_ptr<leveldb::Iterator> iter;
    };

    /**
     * @brief Create iterator for scanning
     */
    std::unique_ptr<Iterator> NewIterator() const;

    /**
     * @brief Get database statistics
     */
    std::string GetStats() const;

    /**
     * @brief Compact database
     */
    void Compact();

    Database() = default;
    ~Database();

    // No copy
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

private:
    std::unique_ptr<leveldb::DB> db;
};

} // namespace dinari

#endif // DINARI_STORAGE_DATABASE_H
