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

#include <iostream>
#include <csignal>
#include <atomic>

using namespace dinari;

// Global flag for shutdown signal
std::atomic<bool> g_shutdownRequested(false);

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
    std::cout << "  --server                Enable RPC server" << std::endl;
    std::cout << "  --rpcport=<port>        RPC server port" << std::endl;
    std::cout << "  --rpcuser=<user>        RPC username" << std::endl;
    std::cout << "  --rpcpassword=<pass>    RPC password" << std::endl;
    std::cout << "  --loglevel=<level>      Log level (trace, debug, info, warning, error)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " --testnet                  # Run testnet node" << std::endl;
    std::cout << "  " << programName << " --mining --miningthreads=4  # Run mining node" << std::endl;
    std::cout << "  " << programName << " --config=custom.conf        # Use custom config" << std::endl;
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

    // TODO: Initialize blockchain
    // TODO: Initialize network
    // TODO: Initialize RPC server
    // TODO: Initialize wallet (if enabled)
    // TODO: Start mining (if enabled)

    // Main loop
    while (!g_shutdownRequested) {
        // TODO: Process blockchain operations
        // TODO: Handle network events
        // TODO: Process RPC requests

        // For now, just sleep
        Time::SleepMillis(1000);
    }

    LOG_INFO("Main", "Shutting down node services...");

    // TODO: Stop mining
    // TODO: Stop RPC server
    // TODO: Close network connections
    // TODO: Flush blockchain database
    // TODO: Close wallet

    return 0;
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
