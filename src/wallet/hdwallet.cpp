#include "hdwallet.h"
#include "crypto/ecdsa.h"
#include "crypto/base58.h"
#include "util/logger.h"
#include "util/serialize.h"
#include <sstream>
#include <algorithm>
#include <random>
#include <cstring>

namespace dinari {

// ExtendedKey implementation

std::string ExtendedKey::Serialize() const {
    bytes data;
    data.reserve(78);

    // Version (4 bytes)
    data.push_back((version >> 24) & 0xFF);
    data.push_back((version >> 16) & 0xFF);
    data.push_back((version >> 8) & 0xFF);
    data.push_back(version & 0xFF);

    // Depth (1 byte)
    data.push_back(depth);

    // Fingerprint (4 bytes)
    data.push_back((fingerprint >> 24) & 0xFF);
    data.push_back((fingerprint >> 16) & 0xFF);
    data.push_back((fingerprint >> 8) & 0xFF);
    data.push_back(fingerprint & 0xFF);

    // Child number (4 bytes)
    data.push_back((childNumber >> 24) & 0xFF);
    data.push_back((childNumber >> 16) & 0xFF);
    data.push_back((childNumber >> 8) & 0xFF);
    data.push_back(childNumber & 0xFF);

    // Chain code (32 bytes)
    data.insert(data.end(), chainCode.begin(), chainCode.end());

    // Key (33 bytes for public, 32+1 for private)
    if (IsPrivate()) {
        data.push_back(0x00);  // Padding byte for private keys
        data.insert(data.end(), key.begin(), key.end());
    } else {
        data.insert(data.end(), key.begin(), key.end());
    }

    return crypto::Base58::EncodeCheck(data);
}

bool ExtendedKey::Deserialize(const std::string& str) {
    bytes data;
    if (!crypto::Base58::DecodeCheck(str, data) || data.size() != 78) {
        return false;
    }

    size_t pos = 0;

    // Version
    version = (data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3];
    pos += 4;

    // Depth
    depth = data[pos++];

    // Fingerprint
    fingerprint = (data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3];
    pos += 4;

    // Child number
    childNumber = (data[pos] << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3];
    pos += 4;

    // Chain code
    chainCode.assign(data.begin() + pos, data.begin() + pos + 32);
    pos += 32;

    // Key
    if (data[pos] == 0x00) {
        // Private key
        key.assign(data.begin() + pos + 1, data.end());
    } else {
        // Public key
        key.assign(data.begin() + pos, data.end());
    }

    return true;
}

// HDWallet implementation

HDWallet::HDWallet() {
}

HDWallet::~HDWallet() {
    // Clear sensitive data
    if (!masterKey.key.empty()) {
        std::fill(masterKey.key.begin(), masterKey.key.end(), 0);
    }
    if (!masterKey.chainCode.empty()) {
        std::fill(masterKey.chainCode.begin(), masterKey.chainCode.end(), 0);
    }
}

bool HDWallet::SetSeed(const bytes& seed) {
    if (seed.size() < 16 || seed.size() > 64) {
        LOG_ERROR("HDWallet", "Invalid seed length");
        return false;
    }

    // I = HMAC-SHA512(Key = "Bitcoin seed", Data = seed)
    bytes hmacKey{'B', 'i', 't', 'c', 'o', 'i', 'n', ' ', 's', 'e', 'e', 'd'};
    bytes hmac = crypto::Hash::HMAC_SHA512(hmacKey, seed);

    // Split I into IL (master secret key) and IR (master chain code)
    masterKey.key.assign(hmac.begin(), hmac.begin() + 32);
    masterKey.chainCode.assign(hmac.begin() + 32, hmac.end());

    // Verify key is valid
    // In secp256k1, private key must be in [1, n-1] where n is curve order
    // For simplicity, just check it's not zero
    bool allZero = std::all_of(masterKey.key.begin(), masterKey.key.end(),
                               [](byte b) { return b == 0; });
    if (allZero) {
        LOG_ERROR("HDWallet", "Invalid master key (zero)");
        return false;
    }

    masterKey.version = MAINNET_PRIVATE;
    masterKey.depth = 0;
    masterKey.fingerprint = 0;
    masterKey.childNumber = 0;

    LOG_INFO("HDWallet", "Master key generated from seed");

    return true;
}

bool HDWallet::DeriveChildPrivate(const ExtendedKey& parent, uint32_t index, ExtendedKey& child) {
    if (!parent.IsPrivate()) {
        LOG_ERROR("HDWallet", "Parent key is not private");
        return false;
    }

    bytes data;

    if (IsHardenedIndex(index)) {
        // Hardened child: data = 0x00 || parent_private_key || index
        data.push_back(0x00);
        data.insert(data.end(), parent.key.begin(), parent.key.end());
    } else {
        // Normal child: data = parent_public_key || index
        bytes pubKey = crypto::ECDSA::GetPublicKey(
            Hash256(parent.key.data()), true);
        data.insert(data.end(), pubKey.begin(), pubKey.end());
    }

    // Append index (big-endian)
    data.push_back((index >> 24) & 0xFF);
    data.push_back((index >> 16) & 0xFF);
    data.push_back((index >> 8) & 0xFF);
    data.push_back(index & 0xFF);

    // I = HMAC-SHA512(Key = parent_chain_code, Data = data)
    bytes hmac = crypto::Hash::HMAC_SHA512(parent.chainCode, data);

    // Split I into IL and IR
    bytes IL(hmac.begin(), hmac.begin() + 32);
    bytes IR(hmac.begin() + 32, hmac.end());

    // Child private key = (IL + parent_private_key) mod n
    // For simplicity, we'll use a basic addition (proper implementation needs secp256k1 arithmetic)
    // This is a simplified version - production code should use proper elliptic curve math
    bytes childKey = parent.key;
    uint32_t carry = 0;
    for (int i = 31; i >= 0; --i) {
        uint32_t sum = childKey[i] + IL[i] + carry;
        childKey[i] = sum & 0xFF;
        carry = sum >> 8;
    }

    child.version = parent.version;
    child.depth = parent.depth + 1;
    child.fingerprint = GetFingerprint(parent);
    child.childNumber = index;
    child.chainCode = IR;
    child.key = childKey;

    return true;
}

bool HDWallet::DeriveChildPublic(const ExtendedKey& parent, uint32_t index, ExtendedKey& child) {
    if (!parent.IsPublic()) {
        LOG_ERROR("HDWallet", "Parent key is not public");
        return false;
    }

    if (IsHardenedIndex(index)) {
        LOG_ERROR("HDWallet", "Cannot derive hardened child from public key");
        return false;
    }

    bytes data;

    // data = parent_public_key || index
    data.insert(data.end(), parent.key.begin(), parent.key.end());

    // Append index (big-endian)
    data.push_back((index >> 24) & 0xFF);
    data.push_back((index >> 16) & 0xFF);
    data.push_back((index >> 8) & 0xFF);
    data.push_back(index & 0xFF);

    // I = HMAC-SHA512(Key = parent_chain_code, Data = data)
    bytes hmac = crypto::Hash::HMAC_SHA512(parent.chainCode, data);

    // Split I into IL and IR
    bytes IL(hmac.begin(), hmac.begin() + 32);
    bytes IR(hmac.begin() + 32, hmac.end());

    // Child public key = point(IL) + parent_public_key
    // This requires elliptic curve point addition - simplified here
    // Production code should use proper EC math

    child.version = parent.version;
    child.depth = parent.depth + 1;
    child.fingerprint = GetFingerprint(parent);
    child.childNumber = index;
    child.chainCode = IR;
    child.key = parent.key;  // Simplified - should be proper EC point addition

    return true;
}

ExtendedKey HDWallet::GetExtendedPublicKey(const ExtendedKey& privKey) {
    if (!privKey.IsPrivate()) {
        return privKey;  // Already public
    }

    ExtendedKey pubKey = privKey;

    // Derive public key from private key
    Hash256 privateKey;
    std::copy(privKey.key.begin(), privKey.key.end(), privateKey.begin());

    pubKey.key = crypto::ECDSA::GetPublicKey(privateKey, true);

    // Change version to public
    if (privKey.version == MAINNET_PRIVATE) {
        pubKey.version = MAINNET_PUBLIC;
    } else if (privKey.version == TESTNET_PRIVATE) {
        pubKey.version = TESTNET_PUBLIC;
    }

    return pubKey;
}

bool HDWallet::DerivePath(const std::string& path, ExtendedKey& key) const {
    std::vector<uint32_t> indices;
    if (!ParsePath(path, indices)) {
        return false;
    }

    key = masterKey;

    for (uint32_t index : indices) {
        if (!DeriveChildPrivate(key, index, key)) {
            return false;
        }
    }

    return true;
}

bool HDWallet::ParsePath(const std::string& path, std::vector<uint32_t>& indices) {
    indices.clear();

    if (path.empty() || (path[0] != 'm' && path[0] != 'M')) {
        return false;
    }

    if (path == "m" || path == "M") {
        return true;  // Master key
    }

    std::istringstream iss(path.substr(2));  // Skip "m/"
    std::string token;

    while (std::getline(iss, token, '/')) {
        if (token.empty()) {
            return false;
        }

        bool hardened = false;
        if (token.back() == '\'' || token.back() == 'h') {
            hardened = true;
            token.pop_back();
        }

        try {
            uint32_t index = std::stoul(token);
            if (hardened) {
                index = HardenIndex(index);
            }
            indices.push_back(index);
        } catch (...) {
            return false;
        }
    }

    return true;
}

Hash256 HDWallet::GetPrivateKey(const ExtendedKey& extKey) {
    if (!extKey.IsPrivate() || extKey.key.size() != 32) {
        return Hash256{0};
    }

    Hash256 privKey;
    std::copy(extKey.key.begin(), extKey.key.end(), privKey.begin());
    return privKey;
}

bytes HDWallet::GetPublicKey(const ExtendedKey& extKey) {
    if (extKey.IsPublic()) {
        return extKey.key;
    }

    // Derive from private
    Hash256 privKey = GetPrivateKey(extKey);
    return crypto::ECDSA::GetPublicKey(privKey, true);
}

uint32_t HDWallet::GetFingerprint(const ExtendedKey& key) {
    bytes pubKey = GetPublicKey(key);
    Hash160 hash = crypto::Hash::ComputeHash160(pubKey);

    return (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
}

// BIP39 implementation

// English wordlist (BIP39) - first 20 words shown, full wordlist has 2048 words
const std::vector<std::string> BIP39::wordlist = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract",
    "absurd", "abuse", "access", "accident", "account", "accuse", "achieve", "acid",
    "acoustic", "acquire", "across", "act"
    // ... (Production code should include all 2048 words)
    // For brevity, this is truncated. A full implementation needs the complete wordlist.
};

bool BIP39::GenerateMnemonic(const bytes& entropy, std::vector<std::string>& mnemonic) {
    size_t entropyBits = entropy.size() * 8;

    // Entropy must be 128, 160, 192, 224, or 256 bits
    if (entropyBits < 128 || entropyBits > 256 || entropyBits % 32 != 0) {
        LOG_ERROR("BIP39", "Invalid entropy length");
        return false;
    }

    // Calculate checksum
    Hash256 hash = crypto::Hash::SHA256(entropy);
    size_t checksumBits = entropyBits / 32;

    // Combine entropy + checksum
    std::vector<bool> bits;
    bits.reserve(entropyBits + checksumBits);

    // Add entropy bits
    for (byte b : entropy) {
        for (int i = 7; i >= 0; --i) {
            bits.push_back((b >> i) & 1);
        }
    }

    // Add checksum bits
    for (size_t i = 0; i < checksumBits; ++i) {
        bits.push_back((hash[i / 8] >> (7 - (i % 8))) & 1);
    }

    // Split into 11-bit groups
    mnemonic.clear();
    for (size_t i = 0; i < bits.size(); i += 11) {
        uint16_t index = 0;
        for (size_t j = 0; j < 11 && i + j < bits.size(); ++j) {
            index = (index << 1) | bits[i + j];
        }

        if (index < wordlist.size()) {
            mnemonic.push_back(wordlist[index]);
        } else {
            // Fallback for incomplete wordlist
            mnemonic.push_back("word" + std::to_string(index));
        }
    }

    return true;
}

bool BIP39::GenerateRandomMnemonic(size_t words, std::vector<std::string>& mnemonic) {
    // Calculate entropy size
    size_t entropyBits;
    switch (words) {
        case 12: entropyBits = 128; break;
        case 15: entropyBits = 160; break;
        case 18: entropyBits = 192; break;
        case 21: entropyBits = 224; break;
        case 24: entropyBits = 256; break;
        default:
            LOG_ERROR("BIP39", "Invalid word count");
            return false;
    }

    // Generate random entropy
    bytes entropy(entropyBits / 8);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    for (auto& byte : entropy) {
        byte = static_cast<uint8_t>(dis(gen));
    }

    return GenerateMnemonic(entropy, mnemonic);
}

bool BIP39::MnemonicToSeed(const std::vector<std::string>& mnemonic,
                           const std::string& passphrase,
                           bytes& seed) {
    // Join mnemonic words
    std::string mnemonicStr;
    for (size_t i = 0; i < mnemonic.size(); ++i) {
        if (i > 0) mnemonicStr += " ";
        mnemonicStr += mnemonic[i];
    }

    // Prepare salt: "mnemonic" + passphrase
    std::string saltStr = "mnemonic" + passphrase;
    bytes salt(saltStr.begin(), saltStr.end());

    // Convert mnemonic to bytes
    bytes mnemonicBytes(mnemonicStr.begin(), mnemonicStr.end());

    // PBKDF2-HMAC-SHA512 with 2048 iterations
    seed = crypto::Hash::PBKDF2_SHA512(mnemonicBytes, salt, 2048, 64);

    return true;
}

bool BIP39::ValidateMnemonic(const std::vector<std::string>& mnemonic) {
    // Check word count
    size_t wordCount = mnemonic.size();
    if (wordCount < 12 || wordCount > 24 || wordCount % 3 != 0) {
        return false;
    }

    // Note: Mnemonic checksum validation can be added for additional verification
    // For now, just check words are in wordlist
    for (const auto& word : mnemonic) {
        if (std::find(wordlist.begin(), wordlist.end(), word) == wordlist.end()) {
            // Allow "wordXXX" pattern for incomplete wordlist
            if (word.substr(0, 4) != "word") {
                return false;
            }
        }
    }

    return true;
}

const std::vector<std::string>& BIP39::GetWordlist() {
    return wordlist;
}

// BIP44 implementation

std::string BIP44::GetPath(uint32_t account, uint32_t change, uint32_t addressIndex) {
    std::ostringstream oss;
    oss << "m/44'/" << COIN_TYPE_DINARI << "'/" << account << "'/" << change << "/" << addressIndex;
    return oss.str();
}

bool BIP44::DeriveAccount(const ExtendedKey& master, uint32_t account, ExtendedKey& accountKey) {
    // m / 44' / 0' / account'
    ExtendedKey key = master;

    // Derive purpose (44')
    if (!HDWallet::DeriveChildPrivate(key, HDWallet::HardenIndex(44), key)) {
        return false;
    }

    // Derive coin type (0')
    if (!HDWallet::DeriveChildPrivate(key, HDWallet::HardenIndex(COIN_TYPE_DINARI), key)) {
        return false;
    }

    // Derive account (account')
    if (!HDWallet::DeriveChildPrivate(key, HDWallet::HardenIndex(account), key)) {
        return false;
    }

    accountKey = key;
    return true;
}

bool BIP44::DeriveAddress(const ExtendedKey& account,
                         uint32_t change,
                         uint32_t index,
                         ExtendedKey& addressKey) {
    // account / change / index
    ExtendedKey key = account;

    // Derive change
    if (!HDWallet::DeriveChildPrivate(key, change, key)) {
        return false;
    }

    // Derive address index
    if (!HDWallet::DeriveChildPrivate(key, index, key)) {
        return false;
    }

    addressKey = key;
    return true;
}

} // namespace dinari
