#ifndef DINARI_NETWORK_ADDRMAN_H
#define DINARI_NETWORK_ADDRMAN_H

#include "protocol.h"
#include "dinari/types.h"
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <random>

namespace dinari {

/**
 * @brief Address entry with metadata
 */
struct AddressEntry {
    NetworkAddress addr;
    Timestamp lastSeen;
    Timestamp lastTry;
    uint32_t attempts;
    bool connected;
    uint32_t score;  // Success score

    AddressEntry()
        : lastSeen(0)
        , lastTry(0)
        , attempts(0)
        , connected(false)
        , score(0) {}

    explicit AddressEntry(const NetworkAddress& a)
        : addr(a)
        , lastSeen(0)
        , lastTry(0)
        , attempts(0)
        , connected(false)
        , score(0) {}
};

/**
 * @brief Address manager
 *
 * Manages peer addresses for connection:
 * - Stores known peer addresses
 * - Provides addresses for outbound connections
 * - Tracks connection success/failure
 * - Integrates DNS seeds
 * - Persists to disk
 */
class AddressManager {
public:
    AddressManager();
    ~AddressManager() = default;

    /**
     * @brief Initialize from DNS seeds
     */
    bool Initialize(bool testnet = false);

    /**
     * @brief Add address
     */
    bool Add(const NetworkAddress& addr);

    /**
     * @brief Add multiple addresses
     */
    size_t Add(const std::vector<NetworkAddress>& addrs);

    /**
     * @brief Mark address as good (successful connection)
     */
    void MarkGood(const NetworkAddress& addr);

    /**
     * @brief Mark address as attempted
     */
    void MarkAttempt(const NetworkAddress& addr);

    /**
     * @brief Mark address as failed
     */
    void MarkFailed(const NetworkAddress& addr);

    /**
     * @brief Mark address as connected
     */
    void MarkConnected(const NetworkAddress& addr, bool connected);

    /**
     * @brief Get random address for connection
     */
    bool GetAddress(NetworkAddress& addr);

    /**
     * @brief Get multiple addresses
     */
    std::vector<NetworkAddress> GetAddresses(size_t count = 10);

    /**
     * @brief Get all addresses
     */
    std::vector<NetworkAddress> GetAllAddresses() const;

    /**
     * @brief Get address count
     */
    size_t GetAddressCount() const;

    /**
     * @brief Clear all addresses
     */
    void Clear();

    /**
     * @brief Load addresses from file
     */
    bool LoadFromFile(const std::string& filename);

    /**
     * @brief Save addresses to file
     */
    bool SaveToFile(const std::string& filename) const;

    /**
     * @brief Query DNS seeds
     */
    bool QueryDNSSeeds(bool testnet = false);

    /**
     * @brief Add hardcoded seed peers
     */
    void AddSeedPeers(bool testnet = false);

private:
    // Address storage
    std::map<std::string, AddressEntry> addresses;  // Key: ip:port

    // Connected addresses
    std::set<std::string> connected;

    // Synchronization
    mutable std::mutex mutex;

    // Random number generation
    std::mt19937 rng;

    // Helper methods
    std::string AddressKey(const NetworkAddress& addr) const;
    bool IsGood(const AddressEntry& entry) const;
    bool ShouldRetry(const AddressEntry& entry) const;
    void UpdateScore(AddressEntry& entry, bool success);
};

} // namespace dinari

#endif // DINARI_NETWORK_ADDRMAN_H
