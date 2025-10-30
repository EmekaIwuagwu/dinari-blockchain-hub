#ifndef DINARI_UTIL_CONFIG_H
#define DINARI_UTIL_CONFIG_H

#include "dinari/types.h"
#include <string>
#include <map>
#include <vector>

namespace dinari {

/**
 * @brief Configuration management for Dinari blockchain
 *
 * Handles loading and parsing configuration from files and command-line arguments.
 * Supports mainnet and testnet configurations.
 */

class Config {
public:
    static Config& Instance();

    // Load configuration from file
    bool LoadFromFile(const std::string& configFile);

    // Parse command-line arguments
    bool ParseCommandLine(int argc, char** argv);

    // Get configuration values
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    int GetInt(const std::string& key, int defaultValue = 0) const;
    uint64_t GetUInt64(const std::string& key, uint64_t defaultValue = 0) const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    std::vector<std::string> GetStringArray(const std::string& key) const;

    // Set configuration values
    void Set(const std::string& key, const std::string& value);
    void Set(const std::string& key, int value);
    void Set(const std::string& key, uint64_t value);
    void Set(const std::string& key, bool value);

    // Check if key exists
    bool Has(const std::string& key) const;

    // Network settings
    bool IsTestnet() const;
    std::string GetDataDir() const;
    Port GetPort() const;
    Port GetRPCPort() const;

    // Print configuration
    void Print() const;

    // Get all keys
    std::vector<std::string> GetAllKeys() const;

private:
    Config();
    ~Config() = default;

    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    std::map<std::string, std::string> values;

    void SetDefaults();
    void ApplyTestnetDefaults();
};

// Configuration keys
namespace config {
    // Network
    constexpr const char* TESTNET = "testnet";
    constexpr const char* PORT = "port";
    constexpr const char* RPC_PORT = "rpcport";
    constexpr const char* RPC_USER = "rpcuser";
    constexpr const char* RPC_PASSWORD = "rpcpassword";
    constexpr const char* RPC_BIND = "rpcbind";
    constexpr const char* BIND = "bind";
    constexpr const char* CONNECT = "connect";
    constexpr const char* ADD_NODE = "addnode";
    constexpr const char* MAX_CONNECTIONS = "maxconnections";

    // Data
    constexpr const char* DATA_DIR = "datadir";
    constexpr const char* DB_CACHE = "dbcache";
    constexpr const char* TX_INDEX = "txindex";
    constexpr const char* PRUNE = "prune";

    // Wallet
    constexpr const char* WALLET = "wallet";
    constexpr const char* WALLET_DIR = "walletdir";
    constexpr const char* DISABLE_WALLET = "disablewallet";
    constexpr const char* KEY_POOL = "keypool";

    // Mining
    constexpr const char* MINING = "mining";
    constexpr const char* MINING_THREADS = "miningthreads";
    constexpr const char* MINING_ADDRESS = "miningaddress";

    // Logging
    constexpr const char* LOG_LEVEL = "loglevel";
    constexpr const char* LOG_FILE = "logfile";
    constexpr const char* PRINT_TO_CONSOLE = "printtoconsole";

    // Performance
    constexpr const char* PAR = "par";  // Number of script verification threads
    constexpr const char* MAX_MEMPOOL = "maxmempool";
    constexpr const char* MAX_UPLOAD_TARGET = "maxuploadtarget";

    // Advanced
    constexpr const char* DAEMON = "daemon";
    constexpr const char* SERVER = "server";
    constexpr const char* REST = "rest";
}

} // namespace dinari

#endif // DINARI_UTIL_CONFIG_H
