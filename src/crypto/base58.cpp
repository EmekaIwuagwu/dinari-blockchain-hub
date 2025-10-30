#include "base58.h"
#include "hash.h"
#include "dinari/constants.h"
#include <algorithm>
#include <cstring>

namespace dinari {
namespace crypto {

// Base58 alphabet (same as Bitcoin)
const char* Base58::ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

// Character map for decoding (initialized once)
const int8_t Base58::MAP[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15,16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29,30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39,40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54,55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

std::string Base58::Encode(const bytes& data) {
    if (data.empty()) {
        return "";
    }

    // Count leading zeros
    size_t leadingZeros = 0;
    while (leadingZeros < data.size() && data[leadingZeros] == 0) {
        ++leadingZeros;
    }

    // Allocate enough space for result
    size_t size = (data.size() - leadingZeros) * 138 / 100 + 1;
    std::vector<byte> b58(size);

    // Process the bytes
    for (size_t i = leadingZeros; i < data.size(); ++i) {
        int carry = data[i];
        for (auto it = b58.rbegin(); it != b58.rend(); ++it) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
    }

    // Skip leading zeros in b58
    auto it = b58.begin();
    while (it != b58.end() && *it == 0) {
        ++it;
    }

    // Translate to Base58 alphabet
    std::string result;
    result.reserve(leadingZeros + (b58.end() - it));
    result.assign(leadingZeros, '1');  // Leading zeros become '1'

    while (it != b58.end()) {
        result += ALPHABET[*(it++)];
    }

    return result;
}

bytes Base58::Decode(const std::string& encoded) {
    if (encoded.empty()) {
        return bytes();
    }

    // Count leading '1's
    size_t leadingOnes = 0;
    while (leadingOnes < encoded.size() && encoded[leadingOnes] == '1') {
        ++leadingOnes;
    }

    // Allocate enough space
    size_t size = encoded.size() * 733 / 1000 + 1;
    std::vector<byte> b256(size);

    // Process characters
    for (size_t i = leadingOnes; i < encoded.size(); ++i) {
        int carry = MAP[static_cast<byte>(encoded[i])];
        if (carry == -1) {
            return bytes();  // Invalid character
        }

        for (auto it = b256.rbegin(); it != b256.rend(); ++it) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
    }

    // Skip leading zeros
    auto it = b256.begin();
    while (it != b256.end() && *it == 0) {
        ++it;
    }

    // Create result with leading zeros
    bytes result;
    result.reserve(leadingOnes + (b256.end() - it));
    result.assign(leadingOnes, 0);
    result.insert(result.end(), it, b256.end());

    return result;
}

bytes Base58::CalculateChecksum(const bytes& data) {
    Hash256 hash = Hash::DoubleSHA256(data);
    return bytes(hash.begin(), hash.begin() + 4);
}

std::string Base58::EncodeCheck(const bytes& data) {
    bytes checksum = CalculateChecksum(data);
    bytes combined = data;
    combined.insert(combined.end(), checksum.begin(), checksum.end());
    return Encode(combined);
}

bool Base58::DecodeCheck(const std::string& encoded, bytes& result) {
    bytes decoded = Decode(encoded);
    if (decoded.size() < 4) {
        return false;
    }

    // Split data and checksum
    bytes data(decoded.begin(), decoded.end() - 4);
    bytes checksum(decoded.end() - 4, decoded.end());

    // Verify checksum
    bytes calculatedChecksum = CalculateChecksum(data);
    if (checksum != calculatedChecksum) {
        return false;
    }

    result = data;
    return true;
}

std::string Base58::EncodeAddress(const Hash160& hash, byte version) {
    bytes data;
    data.reserve(21);
    data.push_back(version);
    data.insert(data.end(), hash.begin(), hash.end());
    return EncodeCheck(data);
}

bool Base58::DecodeAddress(const std::string& address, Hash160& hash, byte& version) {
    bytes decoded;
    if (!DecodeCheck(address, decoded)) {
        return false;
    }

    if (decoded.size() != 21) {
        return false;
    }

    version = decoded[0];
    std::copy(decoded.begin() + 1, decoded.end(), hash.begin());
    return true;
}

bool Base58::IsValidAddress(const std::string& address) {
    Hash160 hash;
    byte version;

    if (!DecodeAddress(address, hash, version)) {
        return false;
    }

    // Check if version is valid for Dinari
    return (version == PUBKEY_ADDRESS_VERSION ||
            version == SCRIPT_ADDRESS_VERSION ||
            version == TESTNET_ADDRESS_VERSION);
}

std::string Base58::EncodePrivateKey(const Hash256& privkey, bool compressed, bool testnet) {
    bytes data;
    data.reserve(compressed ? 34 : 33);

    // Version byte (mainnet: 128, testnet: 239)
    data.push_back(testnet ? 0xEF : 0x80);

    // Private key
    data.insert(data.end(), privkey.begin(), privkey.end());

    // Compression flag
    if (compressed) {
        data.push_back(0x01);
    }

    return EncodeCheck(data);
}

bool Base58::DecodePrivateKey(const std::string& wif, Hash256& privkey,
                              bool& compressed, bool& testnet) {
    bytes decoded;
    if (!DecodeCheck(wif, decoded)) {
        return false;
    }

    // Check size (33 for uncompressed, 34 for compressed)
    if (decoded.size() != 33 && decoded.size() != 34) {
        return false;
    }

    // Check version byte
    byte version = decoded[0];
    if (version != 0x80 && version != 0xEF) {
        return false;
    }

    testnet = (version == 0xEF);
    compressed = (decoded.size() == 34);

    // Extract private key
    std::copy(decoded.begin() + 1, decoded.begin() + 33, privkey.begin());

    // Verify compression flag if present
    if (compressed && decoded[33] != 0x01) {
        return false;
    }

    return true;
}

} // namespace crypto
} // namespace dinari
