#ifndef DINARI_WALLET_ADDRESS_H
#define DINARI_WALLET_ADDRESS_H

#include "dinari/types.h"
#include "crypto/hash.h"
#include "crypto/base58.h"
#include <string>
#include <map>
#include <set>
#include <mutex>

namespace dinari {

/**
 * @brief Address type
 */
enum class AddressType {
    P2PKH,      // Pay-to-Public-Key-Hash
    P2SH,       // Pay-to-Script-Hash
    P2PK        // Pay-to-Public-Key (legacy)
};

/**
 * @brief Address metadata
 */
struct AddressMetadata {
    std::string label;
    Timestamp creationTime;
    AddressType type;
    bool isChange;
    bool isMine;
    uint32_t derivationIndex;  // For HD wallet addresses

    AddressMetadata()
        : creationTime(0)
        , type(AddressType::P2PKH)
        , isChange(false)
        , isMine(false)
        , derivationIndex(0) {}
};

/**
 * @brief Dinari address
 */
class Address {
public:
    Address();
    explicit Address(const std::string& addr);
    Address(const Hash160& hash, AddressType type = AddressType::P2PKH);
    Address(const bytes& pubKey);

    /**
     * @brief Get address string (with 'D' prefix)
     */
    std::string ToString() const;

    /**
     * @brief Get address hash
     */
    const Hash160& GetHash() const { return hash; }

    /**
     * @brief Get address type
     */
    AddressType GetType() const { return type; }

    /**
     * @brief Check if address is valid
     */
    bool IsValid() const;

    /**
     * @brief Parse address string
     */
    bool Parse(const std::string& addr);

    /**
     * @brief Create P2PKH address from public key
     */
    static Address FromPubKey(const bytes& pubKey);

    /**
     * @brief Create P2SH address from script
     */
    static Address FromScript(const bytes& script);

    /**
     * @brief Validate address string
     */
    static bool Validate(const std::string& addr);

    // Comparison operators
    bool operator==(const Address& other) const {
        return hash == other.hash && type == other.type;
    }

    bool operator!=(const Address& other) const {
        return !(*this == other);
    }

    bool operator<(const Address& other) const {
        if (type != other.type) {
            return type < other.type;
        }
        return hash < other.hash;
    }

private:
    Hash160 hash;
    AddressType type;
};

/**
 * @brief Address book manager
 *
 * Manages address metadata, labels, and lookups
 */
class AddressBook {
public:
    AddressBook();
    ~AddressBook() = default;

    /**
     * @brief Add address with metadata
     */
    bool AddAddress(const Address& addr, const AddressMetadata& metadata);

    /**
     * @brief Add address with label
     */
    bool AddAddress(const Address& addr, const std::string& label = "");

    /**
     * @brief Remove address
     */
    bool RemoveAddress(const Address& addr);

    /**
     * @brief Check if address exists
     */
    bool HaveAddress(const Address& addr) const;

    /**
     * @brief Get address metadata
     */
    bool GetMetadata(const Address& addr, AddressMetadata& metadata) const;

    /**
     * @brief Set address label
     */
    bool SetLabel(const Address& addr, const std::string& label);

    /**
     * @brief Get address label
     */
    std::string GetLabel(const Address& addr) const;

    /**
     * @brief Mark address as mine
     */
    bool SetIsMine(const Address& addr, bool isMine);

    /**
     * @brief Check if address is mine
     */
    bool IsMine(const Address& addr) const;

    /**
     * @brief Get all addresses
     */
    std::vector<Address> GetAllAddresses() const;

    /**
     * @brief Get addresses by type
     */
    std::vector<Address> GetAddressesByType(AddressType type) const;

    /**
     * @brief Get owned addresses
     */
    std::vector<Address> GetMyAddresses() const;

    /**
     * @brief Get change addresses
     */
    std::vector<Address> GetChangeAddresses() const;

    /**
     * @brief Get receiving addresses
     */
    std::vector<Address> GetReceivingAddresses() const;

    /**
     * @brief Get address count
     */
    size_t GetAddressCount() const;

    /**
     * @brief Clear all addresses
     */
    void Clear();

private:
    std::map<Address, AddressMetadata> addresses;
    mutable std::mutex mutex;
};

/**
 * @brief Address generator
 *
 * Generates new addresses from keys or HD paths
 */
class AddressGenerator {
public:
    /**
     * @brief Generate P2PKH address from public key
     */
    static Address GenerateP2PKH(const bytes& pubKey);

    /**
     * @brief Generate P2SH address from script
     */
    static Address GenerateP2SH(const bytes& script);

    /**
     * @brief Generate address from private key
     */
    static Address GenerateFromPrivateKey(const Hash256& privKey);

    /**
     * @brief Generate script pub key for address
     */
    static bytes GenerateScriptPubKey(const Address& addr);

    /**
     * @brief Extract address from script pub key
     */
    static bool ExtractAddress(const bytes& scriptPubKey, Address& addr);
};

} // namespace dinari

#endif // DINARI_WALLET_ADDRESS_H
