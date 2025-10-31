#include "hash.h"

// Suppress OpenSSL 3.0 deprecation warnings for now
// TODO: Migrate to EVP API in future
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#pragma GCC diagnostic pop

#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>

namespace {
using boost::multiprecision::uint256_t;

uint256_t Uint256FromHash(const dinari::Hash256& hash) {
    uint256_t value = 0;
    for (int i = 31; i >= 0; --i) {
        value <<= 8;
        value |= hash[i];
    }
    return value;
}

dinari::Hash256 HashFromUint256(const uint256_t& value) {
    dinari::Hash256 result{};
    uint256_t temp = value;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = static_cast<dinari::byte>(temp & 0xff);
        temp >>= 8;
    }
    return result;
}

const uint256_t& MaxUint256() {
    static const uint256_t max = (uint256_t(1) << 256) - 1;
    return max;
}
} // namespace

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

// Hash160 (SHA-256 + RIPEMD-160) - used for address generation
Hash160 Hash::ComputeHash160(const bytes& data) {
    return ComputeHash160(data.data(), data.size());
}

Hash160 Hash::ComputeHash160(const byte* data, size_t len) {
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

// PBKDF2-SHA512 implementation (for BIP39)
bytes Hash::PBKDF2_SHA512(const bytes& password, const bytes& salt,
                          int iterations, size_t keylen) {
    bytes result(keylen);

    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(password.data()), password.size(),
                          salt.data(), salt.size(),
                          iterations, EVP_sha512(),
                          keylen, result.data()) != 1) {
        throw std::runtime_error("PBKDF2-SHA512 failed");
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
    // Note: Full scrypt implementation requires libscrypt library integration
    return PBKDF2_SHA256(password, salt, 10000, keylen);
    #endif

    return result;
}

// Proof of Work functions
bool Hash::CheckProofOfWork(const Hash256& hash, uint32_t target) {
    Hash256 targetHash = CompactToTarget(target);
    uint256_t targetValue = Uint256FromHash(targetHash);

    if (targetValue == 0 || targetValue > MaxUint256()) {
        return false;
    }

    uint256_t hashValue = Uint256FromHash(hash);
    return hashValue <= targetValue;
}

Hash256 Hash::CompactToTarget(uint32_t compact) {
    // Extract exponent (size in bytes) and mantissa (coefficient)
    uint32_t exponent = compact >> 24;
    uint32_t mantissa = compact & 0x00ffffff;  // 24-bit mantissa
    bool negative = (compact & 0x00800000) != 0;  // Check sign bit

    // Negative targets and zero mantissa are invalid
    if (negative || mantissa == 0 || exponent == 0) {
        return Hash256{};
    }

    // Prevent overflow: exponent should not exceed 32 (256 bits / 8)
    if (exponent > 32) {
        return Hash256{};
    }

    uint256_t result = mantissa;

    // Shift by (exponent - 3) bytes since mantissa represents 3 bytes
    int32_t shift = static_cast<int32_t>(exponent) - 3;

    if (shift > 0 && shift <= 29) {  // Max shift: 32 - 3 = 29 bytes
        result <<= static_cast<uint32_t>(shift) * 8;
    } else if (shift < 0) {
        result >>= static_cast<uint32_t>(-shift) * 8;
    }

    // Cap at maximum value (shouldn't happen with proper exponent check)
    if (result > MaxUint256()) {
        result = MaxUint256();
    }

    return HashFromUint256(result);
}

uint32_t Hash::TargetToCompact(const Hash256& target) {
    uint256_t value = Uint256FromHash(target);

    if (value == 0) {
        return 0;
    }

    // Find the most significant byte position
    int exponent = (boost::multiprecision::msb(value) / 8) + 1;
    uint256_t mantissa = value;

    // Normalize mantissa to 3 bytes
    if (exponent <= 3) {
        mantissa <<= 8 * (3 - exponent);
    } else {
        mantissa >>= 8 * (exponent - 3);
    }

    // If the high bit (sign bit) is set, shift right by one byte and increment exponent
    // This ensures the mantissa is positive (sign bit = 0)
    if (mantissa & uint256_t(0x00800000)) {
        mantissa >>= 8;
        exponent++;
    }

    // Ensure mantissa fits in 23 bits (no sign bit)
    mantissa &= uint256_t(0x007fffff);

    // Cap exponent at maximum value
    if (exponent > 0xff) {
        exponent = 0xff;
        mantissa = 0x007fffff;
    }

    uint32_t compact = static_cast<uint32_t>(mantissa.convert_to<uint32_t>());
    compact |= static_cast<uint32_t>(exponent) << 24;
    return compact;
}

// SHA256Hasher implementation
class SHA256Hasher::Impl {
public:
    SHA256_CTX ctx;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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

#pragma GCC diagnostic pop

} // namespace crypto
} // namespace dinari
