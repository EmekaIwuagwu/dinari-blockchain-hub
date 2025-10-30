#ifndef DINARI_WALLET_KEYSTORE_H
#define DINARI_WALLET_KEYSTORE_H

#include "dinari/types.h"
#include "crypto/hash.h"
#include "crypto/ecdsa.h"
#include <map>
#include <set>
#include <mutex>
#include <optional>

namespace dinari {

/**
 * @brief Key metadata
 */
struct KeyMetadata {
    Timestamp creationTime;
    std::string label;
    std::string hdPath;  // BIP32 derivation path
    bool isChange;

    KeyMetadata()
        : creationTime(0)
        , isChange(false) {}
};

/**
 * @brief Private key with metadata
 */
struct Key {
    Hash256 privKey;
    bytes pubKey;
    KeyMetadata metadata;

    Key() {}
    Key(const Hash256& priv, const bytes& pub, const KeyMetadata& meta = KeyMetadata())
        : privKey(priv)
        , pubKey(pub)
        , metadata(meta) {}
};

/**
 * @brief Key store interface
 *
 * Base interface for key storage. Can be encrypted or unencrypted.
 */
class KeyStore {
public:
    virtual ~KeyStore() = default;

    /**
     * @brief Add a private key
     */
    virtual bool AddKey(const Key& key) = 0;

    /**
     * @brief Add a key by private key (generates public key)
     */
    virtual bool AddKeyFromPrivate(const Hash256& privKey, const KeyMetadata& metadata = KeyMetadata());

    /**
     * @brief Check if key exists
     */
    virtual bool HaveKey(const Hash160& keyID) const = 0;

    /**
     * @brief Get private key
     */
    virtual bool GetKey(const Hash160& keyID, Key& key) const = 0;

    /**
     * @brief Get public key
     */
    virtual bool GetPubKey(const Hash160& keyID, bytes& pubKey) const = 0;

    /**
     * @brief Get all key IDs
     */
    virtual std::set<Hash160> GetKeys() const = 0;

    /**
     * @brief Sign hash with key
     */
    virtual bool Sign(const Hash160& keyID, const Hash256& hash, bytes& signature) const;

    /**
     * @brief Check if wallet is locked
     */
    virtual bool IsLocked() const { return false; }
};

/**
 * @brief Basic unencrypted key store
 */
class BasicKeyStore : public KeyStore {
public:
    BasicKeyStore() = default;
    virtual ~BasicKeyStore() = default;

    bool AddKey(const Key& key) override;
    bool HaveKey(const Hash160& keyID) const override;
    bool GetKey(const Hash160& keyID, Key& key) const override;
    bool GetPubKey(const Hash160& keyID, bytes& pubKey) const override;
    std::set<Hash160> GetKeys() const override;

    /**
     * @brief Get key count
     */
    size_t GetKeyCount() const;

    /**
     * @brief Clear all keys
     */
    void Clear();

protected:
    std::map<Hash160, Key> keys;
    mutable std::mutex mutex;

    Hash160 GetKeyID(const bytes& pubKey) const;
};

/**
 * @brief Encrypted key store
 *
 * Stores keys encrypted with a master key derived from passphrase.
 * Uses AES-256-CBC encryption.
 */
class CryptoKeyStore : public BasicKeyStore {
public:
    CryptoKeyStore();
    virtual ~CryptoKeyStore();

    /**
     * @brief Encrypt wallet with passphrase
     */
    bool EncryptWallet(const std::string& passphrase);

    /**
     * @brief Lock wallet (clear master key from memory)
     */
    void Lock();

    /**
     * @brief Unlock wallet with passphrase
     */
    bool Unlock(const std::string& passphrase);

    /**
     * @brief Change passphrase
     */
    bool ChangePassphrase(const std::string& oldPassphrase, const std::string& newPassphrase);

    /**
     * @brief Check if encrypted
     */
    bool IsEncrypted() const { return encrypted; }

    /**
     * @brief Check if locked
     */
    bool IsLocked() const override { return encrypted && !unlocked; }

    // Override key operations to handle encryption
    bool AddKey(const Key& key) override;
    bool GetKey(const Hash160& keyID, Key& key) const override;

    /**
     * @brief Get encrypted keys for serialization
     */
    std::map<Hash160, bytes> GetEncryptedKeys() const;

    /**
     * @brief Add encrypted key (for deserialization)
     */
    bool AddEncryptedKey(const Hash160& keyID, const bytes& encryptedKey, const KeyMetadata& metadata);

    /**
     * @brief Get master key salt
     */
    const bytes& GetMasterKeySalt() const { return masterKeySalt; }

    /**
     * @brief Set master key salt (for deserialization)
     */
    void SetMasterKeySalt(const bytes& salt) { masterKeySalt = salt; }

private:
    bool encrypted;
    bool unlocked;

    // Master key for encryption (derived from passphrase)
    bytes masterKey;
    bytes masterKeySalt;

    // Encrypted private keys
    std::map<Hash160, bytes> encryptedKeys;

    // Derive master key from passphrase
    bytes DeriveMasterKey(const std::string& passphrase, const bytes& salt) const;

    // Encrypt/decrypt key
    bytes EncryptKey(const Hash256& privKey) const;
    bool DecryptKey(const bytes& encrypted, Hash256& privKey) const;
};

} // namespace dinari

#endif // DINARI_WALLET_KEYSTORE_H
