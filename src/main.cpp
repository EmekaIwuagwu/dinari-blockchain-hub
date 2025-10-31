/**
 * @file main.cpp
 * @brief Dinari Blockchain - Main Entry Point
 *
 * This is the main entry point for the Dinari blockchain node.
 * Supports multiple modes: full node, mining, wallet operations.
 *
 * Copyright (C) 2025 The Dinari Blockchain Developers
 */

#include "dinari/version.h"
#include "dinari/constants.h"
#include "util/logger.h"
#include "util/config.h"
#include "util/time.h"
#include "blockchain/blockchain.h"
#include "network/node.h"
#include "rpc/rpcserver.h"
#include "wallet/wallet.h"
#include "mining/miner.h"

#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>
#include <thread>

using namespace dinari;

// Global flag for shutdown signal
std::atomic<bool> g_shutdownRequested(false);

// Global components
std::unique_ptr<Blockchain> g_blockchain;
std::unique_ptr<NetworkNode> g_networkNode;
std::unique_ptr<RPCServer> g_rpcServer;
std::unique_ptr<Wallet> g_wallet;
std::unique_ptr<Miner> g_miner;

// Signal handler for graceful shutdown
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutdown signal received. Gracefully shutting down..." << std::endl;
        g_shutdownRequested = true;
    }
}

// Print banner
void PrintBanner() {
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "║              DINARI BLOCKCHAIN - PRODUCTION                ║" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "║  Version: " << GetVersionString() << std::string(48 - GetVersionString().length(), ' ') << "║" << std::endl;
    std::cout << "║  Protocol: " << PROTOCOL_VERSION << std::string(45, ' ') << "║" << std::endl;
    std::cout << "║  Token: DNT (Dinari)                                       ║" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "║  Copyright (C) 2025 Dinari Blockchain Developers          ║" << std::endl;
    std::cout << "║                                                            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}

// Print usage information
void PrintUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message" << std::endl;
    std::cout << "  -v, --version           Show version information" << std::endl;
    std::cout << "  --config=<file>         Load configuration from file" << std::endl;
    std::cout << "  --datadir=<dir>         Set data directory" << std::endl;
    std::cout << "  --testnet               Run on testnet" << std::endl;
    std::cout << "  --daemon                Run as daemon (background)" << std::endl;
    std::cout << "  --mining                Enable mining" << std::endl;
    std::cout << "  --miningthreads=<n>     Number of mining threads" << std::endl;
    std::cout << "  --miningaddress=<addr>  Mining reward address" << std::endl;
    std::cout << "  --server                Enable RPC server" << std::endl;
    std::cout << "  --rpcport=<port>        RPC server port" << std::endl;
    std::cout << "  --rpcuser=<user>        RPC username" << std::endl;
    std::cout << "  --rpcpassword=<pass>    RPC password" << std::endl;
    std::cout << "  --port=<port>           P2P network port" << std::endl;
    std::cout << "  --listen                Accept incoming connections" << std::endl;
    std::cout << "  --loglevel=<level>      Log level (trace, debug, info, warning, error)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --testnet                  # Run testnet node" << std::endl;
    std::cout << "  " << programName << " --mining --miningthreads=4  # Run mining node" << std::endl;
    std::cout << "  " << programName << " --config=custom.conf        # Use custom config" << std::endl;
    std::cout << "  " << programName << " --server --rpcport=9334     # Run with RPC server" << std::endl;
    std::cout << std::endl;
}

// Initialize application
bool Initialize(int argc, char** argv) {
    // Set up signal handlers
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // Parse command line arguments first
    Config::Instance().ParseCommandLine(argc, argv);

    // Check for help or version
    if (Config::Instance().GetBool("help") || Config::Instance().GetBool("h")) {
        PrintUsage(argv[0]);
        return false;
    }

    if (Config::Instance().GetBool("version") || Config::Instance().GetBool("v")) {
        std::cout << GetBuildInfo() << std::endl;
        return false;
    }

    // Load configuration file if specified
    std::string configFile = Config::Instance().GetString("config");
    if (!configFile.empty()) {
        Config::Instance().LoadFromFile(configFile);
    } else {
        // Try to load default config file
        std::string defaultConfig = Config::Instance().IsTestnet() ?
            "config/testnet.conf" : "config/mainnet.conf";
        Config::Instance().LoadFromFile(defaultConfig);
    }

    // Initialize logger
    std::string logLevel = Config::Instance().GetString("loglevel", "info");
    LogLevel level = LogLevel::INFO;

    if (logLevel == "trace") level = LogLevel::TRACE;
    else if (logLevel == "debug") level = LogLevel::DEBUG;
    else if (logLevel == "info") level = LogLevel::INFO;
    else if (logLevel == "warning") level = LogLevel::WARNING;
    else if (logLevel == "error") level = LogLevel::ERROR;

    std::string logFile = Config::Instance().GetDataDir() + "/" +
                         Config::Instance().GetString("logfile", "debug.log");

    Logger::Instance().Initialize(logFile, level);
    Logger::Instance().SetConsoleOutput(Config::Instance().GetBool("printtoconsole", true));

    // Log startup
    LOG_INFO("Main", "=======================================================");
    LOG_INFO("Main", "Dinari Blockchain Node Starting");
    LOG_INFO("Main", "Version: " + GetVersionString());
    LOG_INFO("Main", "Network: " + std::string(Config::Instance().IsTestnet() ? "TESTNET" : "MAINNET"));
    LOG_INFO("Main", "Data Directory: " + Config::Instance().GetDataDir());
    LOG_INFO("Main", "=======================================================");

    return true;
}

// Main application loop
int RunNode() {
    LOG_INFO("Main", "Starting node services...");

    try {
        // Initialize blockchain with persistent storage
        LOG_INFO("Main", "Initializing blockchain with persistent storage...");
        g_blockchain = std::make_unique<Blockchain>();

        // Get data directory
        std::string dataDir = Config::Instance().GetDataDir();
        LOG_INFO("Main", "Data directory: " + dataDir);

        // Create genesis block
        Block genesisBlock = Block::CreateGenesisBlock(
            Config::Instance().IsTestnet()
        );

        // Initialize blockchain (will load from disk if exists, or create new)
        if (!g_blockchain->Initialize(genesisBlock, dataDir)) {
            LOG_ERROR("Main", "Failed to initialize blockchain");
            return 1;
        }

        LOG_INFO("Main", "Blockchain initialized. Height: " +
                 std::to_string(g_blockchain->GetHeight()));
        LOG_INFO("Main", "Persistent storage: ENABLED");

        // Initialize wallet if enabled
        if (Config::Instance().GetBool("wallet", true)) {
            LOG_INFO("Main", "Initializing wallet...");

            WalletConfig walletConfig;
            walletConfig.dataDir = Config::Instance().GetDataDir() + "/wallet";
            walletConfig.testnet = Config::Instance().IsTestnet();

            g_wallet = std::make_unique<Wallet>(walletConfig);

            // Try to load existing wallet
            std::string walletFile = walletConfig.dataDir + "/wallet.dat";
            if (!g_wallet->Load()) {
                LOG_INFO("Main", "Creating new wallet...");
                // Generate initial address
                g_wallet->GenerateNewAddress("default");
            }

            LOG_INFO("Main", "Wallet initialized");
        }

        // Initialize network
        if (Config::Instance().GetBool("network", true)) {
            LOG_INFO("Main", "Initializing network...");

            NetworkConfig networkConfig;
            networkConfig.testnet = Config::Instance().IsTestnet();
            networkConfig.port = Config::Instance().GetInt("port",
                Config::Instance().IsTestnet() ? 19333 : 9333);
            networkConfig.listen = Config::Instance().GetBool("listen", true);
            networkConfig.dataDir = Config::Instance().GetDataDir();
            networkConfig.maxOutbound = Config::Instance().GetInt("maxconnections", 8);
            networkConfig.maxInbound = Config::Instance().GetInt("maxinbound", 125);

            g_networkNode = std::make_unique<NetworkNode>(*g_blockchain);

            if (!g_networkNode->Initialize(networkConfig)) {
                LOG_ERROR("Main", "Failed to initialize network");
                return 1;
            }

            if (!g_networkNode->Start()) {
                LOG_ERROR("Main", "Failed to start network");
                return 1;
            }

            LOG_INFO("Main", "Network initialized on port " + std::to_string(networkConfig.port));
        }

        // Initialize RPC server
        if (Config::Instance().GetBool("server", true)) {
            LOG_INFO("Main", "Initializing RPC server...");

            g_rpcServer = std::make_unique<RPCServer>(
                *g_blockchain,
                g_wallet.get()
            );

            RPCConfig rpcConfig;
            rpcConfig.port = Config::Instance().GetInt("rpcport",
                Config::Instance().IsTestnet() ? 19334 : 9334);
            rpcConfig.username = Config::Instance().GetString("rpcuser", "dinariuser");
            rpcConfig.password = Config::Instance().GetString("rpcpassword", "dinaripass");
            rpcConfig.allowedIPs = {"127.0.0.1", "::1"}; // Localhost only by default

            if (!g_rpcServer->Start(rpcConfig)) {
                LOG_ERROR("Main", "Failed to start RPC server");
                return 1;
            }

            LOG_INFO("Main", "RPC server started on port " + std::to_string(rpcConfig.port));
            LOG_WARNING("Main", "RPC Authentication: CHANGE DEFAULT CREDENTIALS!");
        }

        // Initialize mining
        if (Config::Instance().GetBool("mining", false)) {
            LOG_INFO("Main", "Initializing mining...");

            // Get mining address
            std::string miningAddrStr = Config::Instance().GetString("miningaddress");
            Address miningAddress;

            if (miningAddrStr.empty()) {
                if (g_wallet) {
                    // Use wallet address
                    auto addresses = g_wallet->GetAllAddresses();
                    if (!addresses.empty()) {
                        miningAddress = addresses[0];
                    } else {
                        miningAddress = g_wallet->GenerateNewAddress("mining");
                    }
                } else {
                    LOG_ERROR("Main", "Mining enabled but no address specified and wallet is disabled");
                    return 1;
                }
            } else {
                // Parse address from config
                if (!miningAddress.FromString(miningAddrStr)) {
                    LOG_ERROR("Main", "Invalid mining address: " + miningAddrStr);
                    return 1;
                }
            }

            LOG_INFO("Main", "Mining to address: " + miningAddress.ToString());

            MiningConfig miningConfig;
            miningConfig.coinbaseAddress = miningAddress;
            miningConfig.numThreads = Config::Instance().GetInt("miningthreads",
                std::thread::hardware_concurrency());

            g_miner = std::make_unique<Miner>(*g_blockchain, miningConfig);

            if (!g_miner->Start()) {
                LOG_ERROR("Main", "Failed to start mining");
                return 1;
            }

            LOG_INFO("Main", "Mining started with " + std::to_string(miningConfig.numThreads) + " threads");
        }

        LOG_INFO("Main", "All services started successfully");
        LOG_INFO("Main", "Node is running. Press Ctrl+C to shutdown.");

        // Main loop
        uint64_t lastStatsTime = Time::GetCurrentTime();

        while (!g_shutdownRequested) {
            // Process blockchain operations
            // (Most work is done in background threads)

            // Print periodic statistics
            uint64_t now = Time::GetCurrentTime();
            if (now - lastStatsTime >= 60) {  // Every 60 seconds
                LOG_INFO("Main", "=== Node Statistics ===");
                LOG_INFO("Main", "Blockchain Height: " + std::to_string(g_blockchain->GetHeight()));
                LOG_INFO("Main", "Best Block: " + g_blockchain->GetBestBlockHash().ToHex());

                if (g_networkNode) {
                    NetworkStats stats = g_networkNode->GetStats();
                    LOG_INFO("Main", "Network Peers: " + std::to_string(stats.totalPeers) +
                            " (In: " + std::to_string(stats.inboundPeers) +
                            ", Out: " + std::to_string(stats.outboundPeers) + ")");
                }

                if (g_wallet) {
                    Amount balance = g_wallet->GetBalance();
                    LOG_INFO("Main", "Wallet Balance: " + std::to_string(balance / COIN) + " DNT");
                }

                if (g_miner && g_miner->IsMining()) {
                    MiningStats miningStats = g_miner->GetStats();
                    LOG_INFO("Main", "Mining Hashrate: " + std::to_string(miningStats.hashrate) + " H/s");
                    LOG_INFO("Main", "Blocks Found: " + std::to_string(miningStats.blocksFound));
                }

                LOG_INFO("Main", "====================");
                lastStatsTime = now;
            }

            // Sleep to reduce CPU usage
            Time::SleepMillis(1000);
        }

        LOG_INFO("Main", "Shutting down node services...");

        // Stop mining
        if (g_miner) {
            LOG_INFO("Main", "Stopping mining...");
            g_miner->Stop();
            g_miner.reset();
        }

        // Stop RPC server
        if (g_rpcServer) {
            LOG_INFO("Main", "Stopping RPC server...");
            g_rpcServer->Stop();
            g_rpcServer.reset();
        }

        // Close network connections
        if (g_networkNode) {
            LOG_INFO("Main", "Stopping network...");
            g_networkNode->Stop();
            g_networkNode.reset();
        }

        // Save and close wallet
        if (g_wallet) {
            LOG_INFO("Main", "Saving wallet...");
            g_wallet->Save();
            g_wallet.reset();
        }

        // Flush blockchain database
        if (g_blockchain) {
            LOG_INFO("Main", "Flushing blockchain...");
            // g_blockchain->Flush();  // Implement when database is added
            g_blockchain.reset();
        }

        LOG_INFO("Main", "Shutdown complete");
        return 0;

    } catch (const std::exception& e) {
        LOG_FATAL("Main", std::string("Fatal error in RunNode: ") + e.what());
        return 1;
    }
}

// Main entry point
int main(int argc, char** argv) {
    try {
        // Print banner
        PrintBanner();

        // Initialize
        if (!Initialize(argc, argv)) {
            return 0;  // Help or version was displayed
        }

        // Run the node
        int result = RunNode();

        // Cleanup
        Logger::Instance().Close();

        return result;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        LOG_FATAL("Main", std::string("Fatal error: ") + e.what());
        return 1;

    } catch (...) {
        std::cerr << "Unknown fatal error" << std::endl;
        LOG_FATAL("Main", "Unknown fatal error");
        return 1;
    }
}
