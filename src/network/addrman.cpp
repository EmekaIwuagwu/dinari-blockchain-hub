#include "addrman.h"
#include "netbase.h"
#include "util/logger.h"
#include "util/time.h"
#include <fstream>
#include <algorithm>

namespace dinari {

// Constants
constexpr uint32_t MAX_ATTEMPTS = 10;
constexpr Timestamp RETRY_DELAY = 3600;  // 1 hour
constexpr Timestamp ADDRESS_EXPIRY = 7 * 24 * 3600;  // 7 days

AddressManager::AddressManager()
    : rng(std::random_device{}()) {
}

bool AddressManager::Initialize(bool testnet) {
    LOG_INFO("AddrMan", "Initializing address manager");

    // Add seed peers
    AddSeedPeers(testnet);

    // Query DNS seeds
    QueryDNSSeeds(testnet);

    LOG_INFO("AddrMan", "Address manager initialized with " +
             std::to_string(GetAddressCount()) + " addresses");

    return true;
}

bool AddressManager::Add(const NetworkAddress& addr) {
    if (!addr.IsValid() || !addr.IsRoutable()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    std::string key = AddressKey(addr);

    auto it = addresses.find(key);
    if (it != addresses.end()) {
        // Update timestamp
        it->second.lastSeen = Time::GetCurrentTime();
        return true;
    }

    // Add new address
    AddressEntry entry(addr);
    entry.lastSeen = Time::GetCurrentTime();
    addresses[key] = entry;

    LOG_DEBUG("AddrMan", "Added address: " + addr.ToString());

    return true;
}

size_t AddressManager::Add(const std::vector<NetworkAddress>& addrs) {
    size_t added = 0;

    for (const auto& addr : addrs) {
        if (Add(addr)) {
            added++;
        }
    }

    if (added > 0) {
        LOG_INFO("AddrMan", "Added " + std::to_string(added) + " addresses");
    }

    return added;
}

void AddressManager::MarkGood(const NetworkAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex);

    std::string key = AddressKey(addr);
    auto it = addresses.find(key);

    if (it != addresses.end()) {
        it->second.lastSeen = Time::GetCurrentTime();
        it->second.attempts = 0;
        UpdateScore(it->second, true);

        LOG_DEBUG("AddrMan", "Marked address as good: " + addr.ToString());
    }
}

void AddressManager::MarkAttempt(const NetworkAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex);

    std::string key = AddressKey(addr);
    auto it = addresses.find(key);

    if (it != addresses.end()) {
        it->second.lastTry = Time::GetCurrentTime();
        it->second.attempts++;

        LOG_DEBUG("AddrMan", "Marked attempt for: " + addr.ToString() +
                 " (attempts: " + std::to_string(it->second.attempts) + ")");
    }
}

void AddressManager::MarkFailed(const NetworkAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex);

    std::string key = AddressKey(addr);
    auto it = addresses.find(key);

    if (it != addresses.end()) {
        UpdateScore(it->second, false);

        // Remove if too many failures
        if (it->second.attempts >= MAX_ATTEMPTS) {
            LOG_WARNING("AddrMan", "Removing failed address: " + addr.ToString());
            addresses.erase(it);
        } else {
            LOG_DEBUG("AddrMan", "Marked address as failed: " + addr.ToString());
        }
    }
}

void AddressManager::MarkConnected(const NetworkAddress& addr, bool isConnected) {
    std::lock_guard<std::mutex> lock(mutex);

    std::string key = AddressKey(addr);
    auto it = addresses.find(key);

    if (it != addresses.end()) {
        it->second.connected = isConnected;

        if (isConnected) {
            connected.insert(key);
        } else {
            connected.erase(key);
        }
    }
}

bool AddressManager::GetAddress(NetworkAddress& addr) {
    std::lock_guard<std::mutex> lock(mutex);

    if (addresses.empty()) {
        return false;
    }

    // Build list of candidate addresses
    std::vector<std::string> candidates;

    Timestamp now = Time::GetCurrentTime();

    for (const auto& pair : addresses) {
        const AddressEntry& entry = pair.second;

        // Skip connected addresses
        if (entry.connected) {
            continue;
        }

        // Skip if too many attempts without enough delay
        if (!ShouldRetry(entry)) {
            continue;
        }

        // Skip expired addresses
        if (entry.lastSeen > 0 && now - entry.lastSeen > ADDRESS_EXPIRY) {
            continue;
        }

        candidates.push_back(pair.first);
    }

    if (candidates.empty()) {
        return false;
    }

    // Select random candidate (weighted by score)
    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    std::string key = candidates[dist(rng)];

    addr = addresses[key].addr;

    return true;
}

std::vector<NetworkAddress> AddressManager::GetAddresses(size_t count) {
    std::vector<NetworkAddress> result;

    std::lock_guard<std::mutex> lock(mutex);

    // Collect all good addresses
    std::vector<std::string> candidates;

    for (const auto& pair : addresses) {
        if (!pair.second.connected && IsGood(pair.second)) {
            candidates.push_back(pair.first);
        }
    }

    // Shuffle and take up to count
    std::shuffle(candidates.begin(), candidates.end(), rng);

    size_t limit = std::min(count, candidates.size());
    for (size_t i = 0; i < limit; ++i) {
        result.push_back(addresses[candidates[i]].addr);
    }

    return result;
}

std::vector<NetworkAddress> AddressManager::GetAllAddresses() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<NetworkAddress> result;
    result.reserve(addresses.size());

    for (const auto& pair : addresses) {
        result.push_back(pair.second.addr);
    }

    return result;
}

size_t AddressManager::GetAddressCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return addresses.size();
}

void AddressManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    addresses.clear();
    connected.clear();
    LOG_INFO("AddrMan", "Cleared all addresses");
}

bool AddressManager::LoadFromFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG_WARNING("AddrMan", "Could not open address file: " + filename);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);
    addresses.clear();

    // Simple format: count followed by entries
    uint32_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(count));

    for (uint32_t i = 0; i < count && file.good(); ++i) {
        NetworkAddress addr;
        Timestamp lastSeen, lastTry;
        uint32_t attempts, score;

        file.read(reinterpret_cast<char*>(&addr.services), sizeof(addr.services));
        file.read(reinterpret_cast<char*>(addr.ip.data()), 16);
        file.read(reinterpret_cast<char*>(&addr.port), sizeof(addr.port));
        file.read(reinterpret_cast<char*>(&lastSeen), sizeof(lastSeen));
        file.read(reinterpret_cast<char*>(&lastTry), sizeof(lastTry));
        file.read(reinterpret_cast<char*>(&attempts), sizeof(attempts));
        file.read(reinterpret_cast<char*>(&score), sizeof(score));

        if (addr.IsValid() && addr.IsRoutable()) {
            AddressEntry entry(addr);
            entry.lastSeen = lastSeen;
            entry.lastTry = lastTry;
            entry.attempts = attempts;
            entry.score = score;

            addresses[AddressKey(addr)] = entry;
        }
    }

    LOG_INFO("AddrMan", "Loaded " + std::to_string(addresses.size()) +
             " addresses from " + filename);

    return true;
}

bool AddressManager::SaveToFile(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("AddrMan", "Could not create address file: " + filename);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Write count
    uint32_t count = static_cast<uint32_t>(addresses.size());
    file.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Write entries
    for (const auto& pair : addresses) {
        const AddressEntry& entry = pair.second;

        file.write(reinterpret_cast<const char*>(&entry.addr.services), sizeof(entry.addr.services));
        file.write(reinterpret_cast<const char*>(entry.addr.ip.data()), 16);
        file.write(reinterpret_cast<const char*>(&entry.addr.port), sizeof(entry.addr.port));
        file.write(reinterpret_cast<const char*>(&entry.lastSeen), sizeof(entry.lastSeen));
        file.write(reinterpret_cast<const char*>(&entry.lastTry), sizeof(entry.lastTry));
        file.write(reinterpret_cast<const char*>(&entry.attempts), sizeof(entry.attempts));
        file.write(reinterpret_cast<const char*>(&entry.score), sizeof(entry.score));
    }

    LOG_INFO("AddrMan", "Saved " + std::to_string(addresses.size()) +
             " addresses to " + filename);

    return true;
}

bool AddressManager::QueryDNSSeeds(bool testnet) {
    const auto& seeds = testnet ? TESTNET_DNS_SEEDS : MAINNET_DNS_SEEDS;

    LOG_INFO("AddrMan", "Querying " + std::to_string(seeds.size()) + " DNS seeds");

    size_t totalAdded = 0;

    for (const auto& seed : seeds) {
        auto addrs = NetBase::LookupHost(seed, testnet ? TESTNET_PORT : DEFAULT_PORT, false);

        if (!addrs.empty()) {
            size_t added = Add(addrs);
            totalAdded += added;

            LOG_INFO("AddrMan", "DNS seed " + seed + " returned " +
                     std::to_string(addrs.size()) + " addresses");
        } else {
            LOG_WARNING("AddrMan", "DNS seed " + seed + " failed");
        }
    }

    LOG_INFO("AddrMan", "DNS seeds provided " + std::to_string(totalAdded) + " addresses");

    return totalAdded > 0;
}

void AddressManager::AddSeedPeers(bool testnet) {
    const auto& seeds = testnet ? TESTNET_SEED_PEERS : MAINNET_SEED_PEERS;

    LOG_INFO("AddrMan", "Adding " + std::to_string(seeds.size()) + " seed peers");

    for (const auto& seedStr : seeds) {
        NetworkAddress addr;
        if (NetBase::ParseAddress(seedStr, addr, testnet ? TESTNET_PORT : DEFAULT_PORT)) {
            Add(addr);
        }
    }
}

// Private methods

std::string AddressManager::AddressKey(const NetworkAddress& addr) const {
    return addr.ToString();
}

bool AddressManager::IsGood(const AddressEntry& entry) const {
    Timestamp now = Time::GetCurrentTime();

    // Not too old
    if (entry.lastSeen > 0 && now - entry.lastSeen > ADDRESS_EXPIRY) {
        return false;
    }

    // Not too many failures
    if (entry.attempts >= MAX_ATTEMPTS) {
        return false;
    }

    return true;
}

bool AddressManager::ShouldRetry(const AddressEntry& entry) const {
    if (entry.attempts == 0) {
        return true;
    }

    Timestamp now = Time::GetCurrentTime();
    Timestamp delay = RETRY_DELAY * (1 << std::min(entry.attempts - 1, 5u));

    return (now - entry.lastTry) > delay;
}

void AddressManager::UpdateScore(AddressEntry& entry, bool success) {
    if (success) {
        entry.score = std::min(entry.score + 1, 100u);
    } else {
        entry.score = entry.score > 0 ? entry.score - 1 : 0;
    }
}

} // namespace dinari
