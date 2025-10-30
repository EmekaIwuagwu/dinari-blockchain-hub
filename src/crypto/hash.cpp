#include "hash.h"
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

namespace dinari {
namespace crypto {

// SHA-256 implementations
Hash256 Hash::SHA256(const bytes& data) {
    return SHA256(data.data(), data.size());
}

Hash256 Hash::SHA256(const std::string& data) {
    return SHA256(reinterpret_cast<const byte*>(data.data()), data.size());
}

Hash256 Hash::SHA256(const byte* data, size_t len) {
    Hash256 hash;
    ::SHA256(data, len, hash.data());
    return hash;
}

// Double SHA-256 implementations
Hash256 Hash::DoubleSHA256(const bytes& data) {
    return DoubleSHA256(data.data(), data.size());
}

Hash256 Hash::DoubleSHA256(const byte* data, size_t len) {
    Hash256 first = SHA256(data, len);
    return SHA256(first.data(), first.size());
}

// RIPEMD-160 implementations
Hash160 Hash::RIPEMD160(const bytes& data) {
    return RIPEMD160(data.data(), data.size());
}

Hash160 Hash::RIPEMD160(const byte* data, size_t len) {
    Hash160 hash;
    ::RIPEMD160(data, len, hash.data());
    return hash;
}

// Hash160 (SHA-256 + RIPEMD-160)
Hash160 Hash::Hash160(const bytes& data) {
    return Hash160(data.data(), data.size());
}

Hash160 Hash::Hash160(const byte* data, size_t len) {
    Hash256 sha = SHA256(data, len);
    return RIPEMD160(sha.data(), sha.size());
}

// Hex conversion
std::string Hash::ToHex(const Hash256& hash) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

std::string Hash::ToHex(const Hash160& hash) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : hash) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

Hash256 Hash::FromHex256(const std::string& hex) {
    if (hex.length() != 64) {
        throw std::invalid_argument("Invalid hex string length for Hash256");
    }

    Hash256 hash;
    for (size_t i = 0; i < 32; ++i) {
        std::string byteString = hex.substr(i * 2, 2);
        hash[i] = static_cast<byte>(std::stoul(byteString, nullptr, 16));
    }
    return hash;
}

Hash160 Hash::FromHex160(const std::string& hex) {
    if (hex.length() != 40) {
        throw std::invalid_argument("Invalid hex string length for Hash160");
    }

    Hash160 hash;
    for (size_t i = 0; i < 20; ++i) {
        std::string byteString = hex.substr(i * 2, 2);
        hash[i] = static_cast<byte>(std::stoul(byteString, nullptr, 16));
    }
    return hash;
}

// Merkle tree functions
Hash256 Hash::MerkleHash(const Hash256& left, const Hash256& right) {
    bytes combined;
    combined.reserve(64);
    combined.insert(combined.end(), left.begin(), left.end());
    combined.insert(combined.end(), right.begin(), right.end());
    return DoubleSHA256(combined);
}

Hash256 Hash::ComputeMerkleRoot(const std::vector<Hash256>& hashes) {
    if (hashes.empty()) {
        return Hash256{};  // Return zero hash for empty input
    }

    if (hashes.size() == 1) {
        return hashes[0];
    }

    std::vector<Hash256> level = hashes;

    while (level.size() > 1) {
        std::vector<Hash256> nextLevel;

        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 < level.size()) {
                nextLevel.push_back(MerkleHash(level[i], level[i + 1]));
            } else {
                // If odd number of elements, duplicate the last one
                nextLevel.push_back(MerkleHash(level[i], level[i]));
            }
        }

        level = std::move(nextLevel);
    }

    return level[0];
}

// HMAC implementations
Hash256 Hash::HMAC_SHA256(const bytes& key, const bytes& data) {
    Hash256 result;
    unsigned int len = 32;

    ::HMAC(EVP_sha256(), key.data(), key.size(),
           data.data(), data.size(), result.data(), &len);

    return result;
}

bytes Hash::HMAC_SHA512(const bytes& key, const bytes& data) {
    bytes result(64);
    unsigned int len = 64;

    ::HMAC(EVP_sha512(), key.data(), key.size(),
           data.data(), data.size(), result.data(), &len);

    return result;
}

// PBKDF2 implementation
bytes Hash::PBKDF2_SHA256(const std::string& password, const bytes& salt,
                          int iterations, size_t keylen) {
    bytes result(keylen);

    if (PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                          salt.data(), salt.size(),
                          iterations, EVP_sha256(),
                          keylen, result.data()) != 1) {
        throw std::runtime_error("PBKDF2 failed");
    }

    return result;
}

// Scrypt implementation (basic version - production should use libscrypt)
bytes Hash::Scrypt(const std::string& password, const bytes& salt,
                   uint64_t N, uint32_t r, uint32_t p, size_t keylen) {
    bytes result(keylen);

    // Note: OpenSSL 3.0+ has EVP_PBE_scrypt
    // For production, use libscrypt or OpenSSL 3.0+
    #if OPENSSL_VERSION_NUMBER >= 0x30000000L
    if (EVP_PBE_scrypt(password.c_str(), password.length(),
                       salt.data(), salt.size(),
                       N, r, p, 0, result.data(), keylen) != 1) {
        throw std::runtime_error("Scrypt failed");
    }
    #else
    // Fallback to PBKDF2 for older OpenSSL versions
    // TODO: Integrate libscrypt for proper scrypt support
    return PBKDF2_SHA256(password, salt, 10000, keylen);
    #endif

    return result;
}

// Proof of Work functions
bool Hash::CheckProofOfWork(const Hash256& hash, uint32_t target) {
    Hash256 targetHash = CompactToTarget(target);

    // Compare hashes (little-endian comparison)
    for (int i = 31; i >= 0; --i) {
        if (hash[i] < targetHash[i]) return true;
        if (hash[i] > targetHash[i]) return false;
    }
    return true;  // Equal is valid
}

Hash256 Hash::CompactToTarget(uint32_t compact) {
    Hash256 target{};

    int size = compact >> 24;
    uint32_t word = compact & 0x007fffff;

    if (size <= 3) {
        word >>= 8 * (3 - size);
        target[0] = (word >> 16) & 0xff;
        target[1] = (word >> 8) & 0xff;
        target[2] = word & 0xff;
    } else {
        int offset = size - 3;
        if (offset < 29) {
            target[offset] = word & 0xff;
            target[offset + 1] = (word >> 8) & 0xff;
            target[offset + 2] = (word >> 16) & 0xff;
        }
    }

    return target;
}

uint32_t Hash::TargetToCompact(const Hash256& target) {
    // Find the most significant non-zero byte
    int size = 32;
    while (size > 0 && target[size - 1] == 0) {
        --size;
    }

    if (size == 0) {
        return 0;
    }

    uint32_t compact = 0;
    if (size <= 3) {
        compact = target[0] << 16;
        if (size > 1) compact |= target[1] << 8;
        if (size > 2) compact |= target[2];
        compact >>= 8 * (3 - size);
    } else {
        compact = target[size - 1];
        compact <<= 8;
        compact |= target[size - 2];
        compact <<= 8;
        compact |= target[size - 3];
    }

    // If the sign bit is set, divide by 256 and increment size
    if (compact & 0x00800000) {
        compact >>= 8;
        size++;
    }

    compact |= size << 24;
    return compact;
}

// SHA256Hasher implementation
class SHA256Hasher::Impl {
public:
    SHA256_CTX ctx;
};

SHA256Hasher::SHA256Hasher() : pimpl(new Impl()) {
    Reset();
}

SHA256Hasher::~SHA256Hasher() = default;

void SHA256Hasher::Update(const bytes& data) {
    Update(data.data(), data.size());
}

void SHA256Hasher::Update(const byte* data, size_t len) {
    SHA256_Update(&pimpl->ctx, data, len);
}

void SHA256Hasher::Update(const std::string& data) {
    Update(reinterpret_cast<const byte*>(data.data()), data.size());
}

Hash256 SHA256Hasher::Finalize() {
    Hash256 hash;
    SHA256_Final(hash.data(), &pimpl->ctx);
    return hash;
}

void SHA256Hasher::Reset() {
    SHA256_Init(&pimpl->ctx);
}

} // namespace crypto
} // namespace dinari
