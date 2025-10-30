#ifndef DINARI_WALLET_HDWALLET_H
#define DINARI_WALLET_HDWALLET_H

#include "dinari/types.h"
#include "crypto/hash.h"
#include <string>
#include <vector>
#include <array>

namespace dinari {

/**
 * @brief BIP32 extended key
 */
struct ExtendedKey {
    uint32_t version;           // 0x0488B21E (xpub) or 0x0488ADE4 (xprv)
    uint8_t depth;              // 0 for master, 1 for level-1 derived keys, etc.
    uint32_t fingerprint;       // Parent key fingerprint
    uint32_t childNumber;       // Child index
    bytes chainCode;            // 32 bytes
    bytes key;                  // 33 bytes (public) or 32 bytes (private)

    ExtendedKey()
        : version(0)
        , depth(0)
        , fingerprint(0)
        , childNumber(0) {
        chainCode.resize(32);
    }

    bool IsPrivate() const { return key.size() == 32; }
    bool IsPublic() const { return key.size() == 33; }

    std::string Serialize() const;
    bool Deserialize(const std::string& str);
};

/**
 * @brief BIP32 HD wallet key derivation
 *
 * Implements BIP32 hierarchical deterministic wallet:
 * - Master key generation from seed
 * - Child key derivation (normal and hardened)
 * - Public key derivation from extended public keys
 * - Serialization to base58 (xprv/xpub format)
 */
class HDWallet {
public:
    HDWallet();
    ~HDWallet();

    /**
     * @brief Generate master key from seed
     *
     * @param seed Random seed (128-512 bits recommended)
     * @return true on success
     */
    bool SetSeed(const bytes& seed);

    /**
     * @brief Get master extended private key
     */
    ExtendedKey GetMasterKey() const { return masterKey; }

    /**
     * @brief Derive child extended private key
     *
     * @param parent Parent extended private key
     * @param index Child index (use index | 0x80000000 for hardened)
     * @param child Output child key
     * @return true on success
     */
    static bool DeriveChildPrivate(const ExtendedKey& parent, uint32_t index, ExtendedKey& child);

    /**
     * @brief Derive child extended public key
     *
     * @param parent Parent extended public key
     * @param index Child index (must not be hardened)
     * @param child Output child key
     * @return true on success
     */
    static bool DeriveChildPublic(const ExtendedKey& parent, uint32_t index, ExtendedKey& child);

    /**
     * @brief Get extended public key from extended private key
     */
    static ExtendedKey GetExtendedPublicKey(const ExtendedKey& privKey);

    /**
     * @brief Derive key from path
     *
     * @param path Derivation path (e.g., "m/44'/0'/0'/0/0")
     * @param key Output derived key
     * @return true on success
     */
    bool DerivePath(const std::string& path, ExtendedKey& key) const;

    /**
     * @brief Parse derivation path
     *
     * @param path Path string (e.g., "m/44'/0'/0'/0/0")
     * @param indices Output index array
     * @return true on success
     */
    static bool ParsePath(const std::string& path, std::vector<uint32_t>& indices);

    /**
     * @brief Get private key from extended key
     */
    static Hash256 GetPrivateKey(const ExtendedKey& extKey);

    /**
     * @brief Get public key from extended key
     */
    static bytes GetPublicKey(const ExtendedKey& extKey);

    /**
     * @brief Check if index is hardened
     */
    static bool IsHardenedIndex(uint32_t index) { return index >= 0x80000000; }

    /**
     * @brief Make index hardened
     */
    static uint32_t HardenIndex(uint32_t index) { return index | 0x80000000; }

    /**
     * @brief Get fingerprint of key
     */
    static uint32_t GetFingerprint(const ExtendedKey& key);

private:
    ExtendedKey masterKey;

    // BIP32 versions
    static constexpr uint32_t MAINNET_PRIVATE = 0x0488ADE4;  // xprv
    static constexpr uint32_t MAINNET_PUBLIC = 0x0488B21E;   // xpub
    static constexpr uint32_t TESTNET_PRIVATE = 0x04358394;  // tprv
    static constexpr uint32_t TESTNET_PUBLIC = 0x043587CF;   // tpub

    // Helper functions
    static bytes SerializeExtendedKey(const ExtendedKey& key);
    static bool DeserializeExtendedKey(const bytes& data, ExtendedKey& key);
};

/**
 * @brief BIP39 mnemonic phrase support
 *
 * Implements BIP39 mnemonic code for generating deterministic keys:
 * - Mnemonic generation from entropy
 * - Mnemonic to seed conversion
 * - Checksum validation
 * - Multiple wordlist support (English)
 */
class BIP39 {
public:
    /**
     * @brief Generate mnemonic from entropy
     *
     * @param entropy Random entropy (128, 160, 192, 224, or 256 bits)
     * @param mnemonic Output mnemonic words
     * @return true on success
     */
    static bool GenerateMnemonic(const bytes& entropy, std::vector<std::string>& mnemonic);

    /**
     * @brief Generate random mnemonic
     *
     * @param words Number of words (12, 15, 18, 21, or 24)
     * @param mnemonic Output mnemonic words
     * @return true on success
     */
    static bool GenerateRandomMnemonic(size_t words, std::vector<std::string>& mnemonic);

    /**
     * @brief Convert mnemonic to seed
     *
     * @param mnemonic Mnemonic words
     * @param passphrase Optional passphrase
     * @param seed Output seed (512 bits)
     * @return true on success
     */
    static bool MnemonicToSeed(const std::vector<std::string>& mnemonic,
                               const std::string& passphrase,
                               bytes& seed);

    /**
     * @brief Validate mnemonic
     *
     * @param mnemonic Mnemonic words
     * @return true if valid
     */
    static bool ValidateMnemonic(const std::vector<std::string>& mnemonic);

    /**
     * @brief Get wordlist
     */
    static const std::vector<std::string>& GetWordlist();

private:
    static const std::vector<std::string> wordlist;

    static bool EntropyToMnemonic(const bytes& entropy, std::vector<std::string>& mnemonic);
    static bool MnemonicToEntropy(const std::vector<std::string>& mnemonic, bytes& entropy);
};

/**
 * @brief BIP44 account structure
 *
 * Implements BIP44 account hierarchy:
 * m / purpose' / coin_type' / account' / change / address_index
 *
 * For Dinari:
 * m / 44' / 0' / 0' / 0 / 0  (first receiving address)
 * m / 44' / 0' / 0' / 1 / 0  (first change address)
 */
class BIP44 {
public:
    /**
     * @brief BIP44 coin types
     */
    static constexpr uint32_t COIN_TYPE_BITCOIN = 0;
    static constexpr uint32_t COIN_TYPE_TESTNET = 1;
    static constexpr uint32_t COIN_TYPE_DINARI = 0;  // Use Bitcoin's coin type

    /**
     * @brief Generate BIP44 path
     *
     * @param account Account number
     * @param change 0 for external (receiving), 1 for internal (change)
     * @param addressIndex Address index
     * @return Path string
     */
    static std::string GetPath(uint32_t account, uint32_t change, uint32_t addressIndex);

    /**
     * @brief Derive account key
     *
     * @param master Master extended key
     * @param account Account number
     * @param accountKey Output account key
     * @return true on success
     */
    static bool DeriveAccount(const ExtendedKey& master, uint32_t account, ExtendedKey& accountKey);

    /**
     * @brief Derive address key
     *
     * @param account Account key
     * @param change 0 for external, 1 for internal
     * @param index Address index
     * @param addressKey Output address key
     * @return true on success
     */
    static bool DeriveAddress(const ExtendedKey& account,
                             uint32_t change,
                             uint32_t index,
                             ExtendedKey& addressKey);
};

} // namespace dinari

#endif // DINARI_WALLET_HDWALLET_H
