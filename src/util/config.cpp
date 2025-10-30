#include "config.h"
#include "logger.h"
#include "dinari/constants.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace dinari {

Config& Config::Instance() {
    static Config instance;
    return instance;
}

Config::Config() {
    SetDefaults();
}

void Config::SetDefaults() {
    // Network defaults
    Set(config::TESTNET, false);
    Set(config::PORT, static_cast<int>(DEFAULT_PORT));
    Set(config::RPC_PORT, static_cast<int>(DEFAULT_RPC_PORT));
    Set(config::RPC_BIND, "127.0.0.1");
    Set(config::MAX_CONNECTIONS, static_cast<int>(MAX_PEER_CONNECTIONS));

    // Data defaults
    Set(config::DATA_DIR, DEFAULT_DATA_DIR);
    Set(config::DB_CACHE, 300);  // 300 MB
    Set(config::TX_INDEX, false);
    Set(config::PRUNE, 0);  // 0 = no pruning

    // Wallet defaults
    Set(config::DISABLE_WALLET, false);
    Set(config::KEY_POOL, 1000);

    // Mining defaults
    Set(config::MINING, false);
    Set(config::MINING_THREADS, 1);

    // Logging defaults
    Set(config::LOG_LEVEL, "info");
    Set(config::LOG_FILE, "debug.log");
    Set(config::PRINT_TO_CONSOLE, true);

    // Performance defaults
    Set(config::PAR, 4);  // 4 script verification threads
    Set(config::MAX_MEMPOOL, 300);  // 300 MB
    Set(config::MAX_UPLOAD_TARGET, 0);  // 0 = unlimited

    // Advanced defaults
    Set(config::DAEMON, false);
    Set(config::SERVER, true);
    Set(config::REST, false);
}

void Config::ApplyTestnetDefaults() {
    Set(config::PORT, static_cast<int>(DEFAULT_TESTNET_PORT));
    Set(config::RPC_PORT, static_cast<int>(DEFAULT_RPC_TESTNET_PORT));
}

bool Config::LoadFromFile(const std::string& configFile) {
    std::ifstream file(configFile);
    if (!file.is_open()) {
        LOG_WARNING("Config", "Could not open config file: " + configFile);
        return false;
    }

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        ++lineNum;

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key=value
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            LOG_WARNING("Config", "Invalid line " + std::to_string(lineNum) + " in config file: " + line);
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        Set(key, value);
    }

    file.close();

    // Apply testnet defaults if testnet is enabled
    if (IsTestnet()) {
        ApplyTestnetDefaults();
    }

    LOG_INFO("Config", "Loaded configuration from: " + configFile);
    return true;
}

bool Config::ParseCommandLine(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        // Remove leading dashes
        if (arg.substr(0, 2) == "--") {
            arg = arg.substr(2);
        } else if (arg[0] == '-') {
            arg = arg.substr(1);
        } else {
            continue;
        }

        // Check for key=value format
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
            std::string key = arg.substr(0, pos);
            std::string value = arg.substr(pos + 1);
            Set(key, value);
        } else {
            // Boolean flag (no value means true)
            Set(arg, "1");
        }
    }

    // Apply testnet defaults if testnet is enabled
    if (IsTestnet()) {
        ApplyTestnetDefaults();
    }

    return true;
}

std::string Config::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        return it->second;
    }
    return defaultValue;
}

int Config::GetInt(const std::string& key, int defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

uint64_t Config::GetUInt64(const std::string& key, uint64_t defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        try {
            return std::stoull(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

bool Config::GetBool(const std::string& key, bool defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        std::string value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return (value == "1" || value == "true" || value == "yes" || value == "on");
    }
    return defaultValue;
}

std::vector<std::string> Config::GetStringArray(const std::string& key) const {
    std::vector<std::string> result;

    // Look for keys with same name (can have multiple)
    for (const auto& [k, v] : values) {
        if (k == key) {
            result.push_back(v);
        }
    }

    return result;
}

void Config::Set(const std::string& key, const std::string& value) {
    values[key] = value;
}

void Config::Set(const std::string& key, int value) {
    values[key] = std::to_string(value);
}

void Config::Set(const std::string& key, uint64_t value) {
    values[key] = std::to_string(value);
}

void Config::Set(const std::string& key, bool value) {
    values[key] = value ? "1" : "0";
}

bool Config::Has(const std::string& key) const {
    return values.find(key) != values.end();
}

bool Config::IsTestnet() const {
    return GetBool(config::TESTNET, false);
}

std::string Config::GetDataDir() const {
    std::string dataDir = GetString(config::DATA_DIR);

    // Append /testnet if testnet is enabled
    if (IsTestnet()) {
        if (dataDir.back() != '/' && dataDir.back() != '\\') {
            dataDir += "/";
        }
        dataDir += "testnet";
    }

    return dataDir;
}

Port Config::GetPort() const {
    if (IsTestnet()) {
        return static_cast<Port>(GetInt(config::PORT, DEFAULT_TESTNET_PORT));
    }
    return static_cast<Port>(GetInt(config::PORT, DEFAULT_PORT));
}

Port Config::GetRPCPort() const {
    if (IsTestnet()) {
        return static_cast<Port>(GetInt(config::RPC_PORT, DEFAULT_RPC_TESTNET_PORT));
    }
    return static_cast<Port>(GetInt(config::RPC_PORT, DEFAULT_RPC_PORT));
}

void Config::Print() const {
    LOG_INFO("Config", "=== Configuration ===");
    for (const auto& [key, value] : values) {
        // Hide sensitive values
        if (key.find("password") != std::string::npos) {
            LOG_INFO("Config", key + " = *****");
        } else {
            LOG_INFO("Config", key + " = " + value);
        }
    }
    LOG_INFO("Config", "====================");
}

std::vector<std::string> Config::GetAllKeys() const {
    std::vector<std::string> keys;
    keys.reserve(values.size());
    for (const auto& [key, _] : values) {
        keys.push_back(key);
    }
    return keys;
}

} // namespace dinari
