#ifndef DINARI_CRYPTO_HASH_H
#define DINARI_CRYPTO_HASH_H

#include "dinari/types.h"
#include <string>
#include <vector>

namespace dinari {
namespace crypto {

/**
 * @brief Cryptographic hash functions for Dinari blockchain
 *
 * Provides SHA-256, double SHA-256, RIPEMD-160, and Hash160 (SHA-256 + RIPEMD-160)
 * These are the core hash functions used throughout the blockchain for:
 * - Block hashing (double SHA-256)
 * - Transaction IDs (double SHA-256)
 * - Address generation (Hash160)
 * - Merkle tree construction (double SHA-256)
 */

class Hash {
public:
    /**
     * @brief Compute SHA-256 hash
     * @param data Input data
     * @return 32-byte hash
     */
    static Hash256 SHA256(const bytes& data);
    static Hash256 SHA256(const std::string& data);
    static Hash256 SHA256(const byte* data, size_t len);

    /**
     * @brief Compute double SHA-256 hash (SHA-256 of SHA-256)
     * Used for block hashes and transaction IDs
     * @param data Input data
     * @return 32-byte hash
     */
    static Hash256 DoubleSHA256(const bytes& data);
    static Hash256 DoubleSHA256(const byte* data, size_t len);

    /**
     * @brief Compute RIPEMD-160 hash
     * @param data Input data
     * @return 20-byte hash
     */
    static Hash160 RIPEMD160(const bytes& data);
    static Hash160 RIPEMD160(const byte* data, size_t len);

    /**
     * @brief Compute Hash160 (SHA-256 followed by RIPEMD-160)
     * Used for address generation from public keys
     * @param data Input data (usually a public key)
     * @return 20-byte hash
     */
    static dinari::Hash160 ComputeHash160(const bytes& data);
    static dinari::Hash160 ComputeHash160(const byte* data, size_t len);

    /**
     * @brief Convert hash to hex string
     * @param hash Hash to convert
     * @return Hexadecimal string representation
     */
    static std::string ToHex(const Hash256& hash);
    static std::string ToHex(const dinari::Hash160& hash);

    /**
     * @brief Convert hex string to hash
     * @param hex Hexadecimal string
     * @return Hash (throws on invalid input)
     */
    static Hash256 FromHex256(const std::string& hex);
    static dinari::Hash160 FromHex160(const std::string& hex);

    /**
     * @brief Compute Merkle root from transaction hashes
     * @param hashes Vector of transaction hashes
     * @return Merkle root hash
     */
    static Hash256 ComputeMerkleRoot(const std::vector<Hash256>& hashes);

    /**
     * @brief HMAC-SHA256 (used in HD wallet key derivation)
     * @param key Key for HMAC
     * @param data Data to hash
     * @return HMAC-SHA256 result
     */
    static Hash256 HMAC_SHA256(const bytes& key, const bytes& data);

    /**
     * @brief HMAC-SHA512 (used in BIP32 HD wallet)
     * @param key Key for HMAC
     * @param data Data to hash
     * @return HMAC-SHA512 result (64 bytes)
     */
    static bytes HMAC_SHA512(const bytes& key, const bytes& data);

    /**
     * @brief PBKDF2 key derivation (used for wallet encryption)
     * @param password Password
     * @param salt Salt
     * @param iterations Number of iterations (recommend 10000+)
     * @param keylen Output key length
     * @return Derived key
     */
    static bytes PBKDF2_SHA256(const std::string& password, const bytes& salt,
                               int iterations, size_t keylen);

    /**
     * @brief PBKDF2 key derivation with SHA-512 (used for BIP39 seed generation)
     * @param password Password (or mnemonic bytes)
     * @param salt Salt
     * @param iterations Number of iterations (recommend 2048+ for BIP39)
     * @param keylen Output key length
     * @return Derived key
     */
    static bytes PBKDF2_SHA512(const bytes& password, const bytes& salt,
                               int iterations, size_t keylen);

    /**
     * @brief Scrypt key derivation (alternative to PBKDF2)
     * @param password Password
     * @param salt Salt
     * @param N CPU/memory cost parameter
     * @param r Block size parameter
     * @param p Parallelization parameter
     * @param keylen Output key length
     * @return Derived key
     */
    static bytes Scrypt(const std::string& password, const bytes& salt,
                       uint64_t N, uint32_t r, uint32_t p, size_t keylen);

    /**
     * @brief Check if hash meets difficulty target
     * @param hash Block hash
     * @param target Difficulty target (compact format)
     * @return true if hash <= target
     */
    static bool CheckProofOfWork(const Hash256& hash, uint32_t target);

    /**
     * @brief Convert compact difficulty target to full 256-bit target
     * @param compact Compact representation (4 bytes)
     * @return Full 256-bit target as Hash256
     */
    static Hash256 CompactToTarget(uint32_t compact);

    /**
     * @brief Convert full 256-bit target to compact format
     * @param target Full target
     * @return Compact representation
     */
    static uint32_t TargetToCompact(const Hash256& target);

private:
    // Helper function for Merkle tree calculation
    static Hash256 MerkleHash(const Hash256& left, const Hash256& right);
};

/**
 * @brief Hasher class for incremental hashing
 * Useful for hashing large data streams
 */
class SHA256Hasher {
public:
    SHA256Hasher();
    ~SHA256Hasher();

    // Non-copyable
    SHA256Hasher(const SHA256Hasher&) = delete;
    SHA256Hasher& operator=(const SHA256Hasher&) = delete;

    /**
     * @brief Add data to hash
     * @param data Data to add
     */
    void Update(const bytes& data);
    void Update(const byte* data, size_t len);
    void Update(const std::string& data);

    /**
     * @brief Finalize hash and get result
     * @return 32-byte hash
     */
    Hash256 Finalize();

    /**
     * @brief Reset hasher for reuse
     */
    void Reset();

private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
};

} // namespace crypto
} // namespace dinari

#endif // DINARI_CRYPTO_HASH_H
