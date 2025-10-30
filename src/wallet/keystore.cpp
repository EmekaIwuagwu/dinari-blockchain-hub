#include "keystore.h"
#include "crypto/aes.h"
#include "util/logger.h"
#include "util/security.h"
#include <algorithm>
#include <openssl/rand.h>

namespace dinari {

// KeyStore base implementation

bool KeyStore::AddKeyFromPrivate(const Hash256& privKey, const KeyMetadata& metadata) {
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
    if (pubKey.empty()) {
        return false;
    }

    Key key(privKey, pubKey, metadata);
    return AddKey(key);
}

bool KeyStore::Sign(const Hash160& keyID, const Hash256& hash, bytes& signature) const {
    Key key;
    if (!GetKey(keyID, key)) {
        return false;
    }

    signature = crypto::ECDSA::Sign(hash, key.privKey);
    return !signature.empty();
}

// BasicKeyStore implementation

bool BasicKeyStore::AddKey(const Key& key) {
    std::lock_guard<std::mutex> lock(mutex);

    Hash160 keyID = GetKeyID(key.pubKey);
    keys[keyID] = key;

    LOG_DEBUG("KeyStore", "Added key: " + keyID.ToHex());

    return true;
}

bool BasicKeyStore::HaveKey(const Hash160& keyID) const {
    std::lock_guard<std::mutex> lock(mutex);
    return keys.count(keyID) > 0;
}

bool BasicKeyStore::GetKey(const Hash160& keyID, Key& key) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = keys.find(keyID);
    if (it == keys.end()) {
        return false;
    }

    key = it->second;
    return true;
}

bool BasicKeyStore::GetPubKey(const Hash160& keyID, bytes& pubKey) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = keys.find(keyID);
    if (it == keys.end()) {
        return false;
    }

    pubKey = it->second.pubKey;
    return true;
}

std::set<Hash160> BasicKeyStore::GetKeys() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::set<Hash160> keySet;
    for (const auto& pair : keys) {
        keySet.insert(pair.first);
    }

    return keySet;
}

size_t BasicKeyStore::GetKeyCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return keys.size();
}

void BasicKeyStore::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    keys.clear();
    LOG_INFO("KeyStore", "Cleared all keys");
}

Hash160 BasicKeyStore::GetKeyID(const bytes& pubKey) const {
    return crypto::Hash::Hash160(pubKey);
}

// CryptoKeyStore implementation

CryptoKeyStore::CryptoKeyStore()
    : encrypted(false)
    , unlocked(false) {
}

CryptoKeyStore::~CryptoKeyStore() {
    // Clear sensitive data
    if (!masterKey.empty()) {
        std::fill(masterKey.begin(), masterKey.end(), 0);
    }
}

bool CryptoKeyStore::EncryptWallet(const std::string& passphrase) {
    if (encrypted) {
        LOG_ERROR("KeyStore", "Wallet is already encrypted");
        return false;
    }

    if (passphrase.empty()) {
        LOG_ERROR("KeyStore", "Empty passphrase not allowed");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Generate cryptographically secure random salt using OpenSSL
    masterKeySalt = Security::SecureRandomBytes(32);
    if (masterKeySalt.empty() || masterKeySalt.size() != 32) {
        LOG_ERROR("KeyStore", "Failed to generate secure random salt");
        return false;
    }

    // Derive master key
    masterKey = DeriveMasterKey(passphrase, masterKeySalt);

    // Encrypt all existing keys
    for (const auto& pair : keys) {
        bytes encryptedKey = EncryptKey(pair.second.privKey);
        if (encryptedKey.empty()) {
            LOG_ERROR("KeyStore", "Failed to encrypt key");
            masterKey.clear();
            masterKeySalt.clear();
            return false;
        }

        encryptedKeys[pair.first] = encryptedKey;
    }

    encrypted = true;
    unlocked = true;

    LOG_INFO("KeyStore", "Wallet encrypted with " + std::to_string(keys.size()) + " keys");

    return true;
}

void CryptoKeyStore::Lock() {
    if (!encrypted) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Clear master key
    std::fill(masterKey.begin(), masterKey.end(), 0);
    masterKey.clear();

    // Clear unencrypted keys
    keys.clear();

    unlocked = false;

    LOG_INFO("KeyStore", "Wallet locked");
}

bool CryptoKeyStore::Unlock(const std::string& passphrase) {
    if (!encrypted) {
        LOG_ERROR("KeyStore", "Wallet is not encrypted");
        return false;
    }

    if (unlocked) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Derive master key from passphrase
    bytes derivedKey = DeriveMasterKey(passphrase, masterKeySalt);

    // Try to decrypt keys
    std::map<Hash160, Key> decryptedKeys;

    for (const auto& pair : encryptedKeys) {
        Hash256 privKey;

        // Temporarily set master key for decryption
        masterKey = derivedKey;

        if (!DecryptKey(pair.second, privKey)) {
            // Wrong passphrase
            std::fill(derivedKey.begin(), derivedKey.end(), 0);
            std::fill(masterKey.begin(), masterKey.end(), 0);
            masterKey.clear();
            LOG_ERROR("KeyStore", "Failed to unlock wallet: incorrect passphrase");
            return false;
        }

        bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
        Key key(privKey, pubKey);

        decryptedKeys[pair.first] = key;
    }

    // Success - store decrypted keys
    keys = decryptedKeys;
    masterKey = derivedKey;
    unlocked = true;

    LOG_INFO("KeyStore", "Wallet unlocked with " + std::to_string(keys.size()) + " keys");

    return true;
}

bool CryptoKeyStore::ChangePassphrase(const std::string& oldPassphrase, const std::string& newPassphrase) {
    if (!encrypted) {
        LOG_ERROR("KeyStore", "Wallet is not encrypted");
        return false;
    }

    if (newPassphrase.empty()) {
        LOG_ERROR("KeyStore", "Empty passphrase not allowed");
        return false;
    }

    // Unlock with old passphrase
    if (!Unlock(oldPassphrase)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Generate new cryptographically secure random salt using OpenSSL
    masterKeySalt = Security::SecureRandomBytes(32);
    if (masterKeySalt.empty() || masterKeySalt.size() != 32) {
        LOG_ERROR("KeyStore", "Failed to generate secure random salt");
        return false;
    }

    // Derive new master key
    bytes oldMasterKey = masterKey;
    masterKey = DeriveMasterKey(newPassphrase, masterKeySalt);

    // Re-encrypt all keys with new master key
    std::map<Hash160, bytes> newEncryptedKeys;

    for (const auto& pair : keys) {
        bytes encryptedKey = EncryptKey(pair.second.privKey);
        if (encryptedKey.empty()) {
            LOG_ERROR("KeyStore", "Failed to re-encrypt key");
            // Restore old master key
            masterKey = oldMasterKey;
            return false;
        }

        newEncryptedKeys[pair.first] = encryptedKey;
    }

    // Clear old master key
    std::fill(oldMasterKey.begin(), oldMasterKey.end(), 0);

    encryptedKeys = newEncryptedKeys;

    LOG_INFO("KeyStore", "Passphrase changed successfully");

    return true;
}

bool CryptoKeyStore::AddKey(const Key& key) {
    if (encrypted && !unlocked) {
        LOG_ERROR("KeyStore", "Wallet is locked");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    Hash160 keyID = GetKeyID(key.pubKey);

    if (encrypted) {
        // Encrypt and store
        bytes encryptedKey = EncryptKey(key.privKey);
        if (encryptedKey.empty()) {
            return false;
        }

        encryptedKeys[keyID] = encryptedKey;
    }

    // Store unencrypted for use
    keys[keyID] = key;

    LOG_DEBUG("KeyStore", "Added key: " + keyID.ToHex());

    return true;
}

bool CryptoKeyStore::GetKey(const Hash160& keyID, Key& key) const {
    if (encrypted && !unlocked) {
        LOG_ERROR("KeyStore", "Wallet is locked");
        return false;
    }

    return BasicKeyStore::GetKey(keyID, key);
}

std::map<Hash160, bytes> CryptoKeyStore::GetEncryptedKeys() const {
    std::lock_guard<std::mutex> lock(mutex);
    return encryptedKeys;
}

bool CryptoKeyStore::AddEncryptedKey(const Hash160& keyID, const bytes& encryptedKey, const KeyMetadata& metadata) {
    std::lock_guard<std::mutex> lock(mutex);

    encryptedKeys[keyID] = encryptedKey;
    encrypted = true;

    // If unlocked, decrypt and add to keys
    if (unlocked) {
        Hash256 privKey;
        if (DecryptKey(encryptedKey, privKey)) {
            bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
            Key key(privKey, pubKey, metadata);
            keys[keyID] = key;
        }
    }

    return true;
}

bytes CryptoKeyStore::DeriveMasterKey(const std::string& passphrase, const bytes& salt) const {
    // Use PBKDF2 with 100,000 iterations
    bytes passphraseBytes(passphrase.begin(), passphrase.end());
    return crypto::Hash::PBKDF2_SHA512(passphraseBytes, salt, 100000, 32);
}

bytes CryptoKeyStore::EncryptKey(const Hash256& privKey) const {
    if (masterKey.empty()) {
        return bytes();
    }

    // Convert Hash256 to bytes
    bytes plaintext(privKey.begin(), privKey.end());

    // Generate cryptographically secure random IV using OpenSSL
    bytes iv = Security::SecureRandomBytes(16);
    if (iv.empty() || iv.size() != 16) {
        LOG_ERROR("KeyStore", "Failed to generate secure random IV");
        return bytes();
    }

    // Encrypt
    bytes ciphertext = crypto::AES::Encrypt(plaintext, masterKey, iv);
    if (ciphertext.empty()) {
        return bytes();
    }

    // Prepend IV
    bytes result;
    result.reserve(iv.size() + ciphertext.size());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

bool CryptoKeyStore::DecryptKey(const bytes& encrypted, Hash256& privKey) const {
    if (masterKey.empty() || encrypted.size() < 16) {
        return false;
    }

    // Extract IV
    bytes iv(encrypted.begin(), encrypted.begin() + 16);

    // Extract ciphertext
    bytes ciphertext(encrypted.begin() + 16, encrypted.end());

    // Decrypt
    bytes plaintext = crypto::AES::Decrypt(ciphertext, masterKey, iv);
    if (plaintext.size() != 32) {
        return false;
    }

    // Convert to Hash256
    std::copy(plaintext.begin(), plaintext.end(), privKey.begin());

    // Clear plaintext
    std::fill(plaintext.begin(), plaintext.end(), 0);

    return true;
}

} // namespace dinari
