/**
 * @file dinari-cli.cpp
 * @brief Dinari blockchain command-line interface
 *
 * Command-line client for interacting with Dinari node via RPC
 */

#include "rpc/rpcserver.h"
#include "util/logger.h"
#include "util/config.h"
#include <iostream>
#include <string>
#include <vector>

using namespace dinari;

/**
 * @brief Print usage information
 */
void PrintUsage() {
    std::cout << "Dinari CLI - Command-line interface for Dinari blockchain\n\n";
    std::cout << "Usage: dinari-cli [options] <command> [params]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -rpcconnect=<ip>    RPC server IP address (default: 127.0.0.1)\n";
    std::cout << "  -rpcport=<port>     RPC server port (default: 9334)\n";
    std::cout << "  -rpcuser=<user>     RPC username\n";
    std::cout << "  -rpcpassword=<pw>   RPC password\n";
    std::cout << "  -testnet            Use testnet\n";
    std::cout << "  -help               This help message\n\n";
    std::cout << "Blockchain commands:\n";
    std::cout << "  getblockcount                      Get current block height\n";
    std::cout << "  getblockhash <height>              Get block hash at height\n";
    std::cout << "  getblock <hash>                    Get block information\n";
    std::cout << "  getbestblockhash                   Get hash of best block\n";
    std::cout << "  getdifficulty                      Get current difficulty\n";
    std::cout << "  getblockchaininfo                  Get blockchain information\n";
    std::cout << "  getmempoolinfo                     Get mempool information\n\n";
    std::cout << "Wallet commands:\n";
    std::cout << "  getnewaddress [label]              Generate new address\n";
    std::cout << "  getbalance                         Get wallet balance\n";
    std::cout << "  sendtoaddress <addr> <amount>      Send DNT to address\n";
    std::cout << "  listaddresses                      List all wallet addresses\n";
    std::cout << "  listtransactions [count]           List recent transactions\n";
    std::cout << "  listunspent                        List unspent outputs\n";
    std::cout << "  getwalletinfo                      Get wallet information\n";
    std::cout << "  encryptwallet <passphrase>         Encrypt wallet\n";
    std::cout << "  walletlock                         Lock wallet\n";
    std::cout << "  walletpassphrase <pp> <timeout>    Unlock wallet\n\n";
    std::cout << "Control commands:\n";
    std::cout << "  help [command]                     Get help\n";
    std::cout << "  stop                               Stop Dinari server\n\n";
    std::cout << "Examples:\n";
    std::cout << "  dinari-cli getblockcount\n";
    std::cout << "  dinari-cli getnewaddress \"my address\"\n";
    std::cout << "  dinari-cli sendtoaddress D1abc... 10.5\n";
}

/**
 * @brief Parse command-line arguments
 */
bool ParseArguments(int argc, char** argv,
                   std::string& rpcHost,
                   uint16_t& rpcPort,
                   std::string& rpcUser,
                   std::string& rpcPassword,
                   std::string& command,
                   std::vector<std::string>& params) {
    // Defaults
    rpcHost = "127.0.0.1";
    rpcPort = 9334;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-help" || arg == "--help" || arg == "-h") {
            PrintUsage();
            return false;
        } else if (arg.find("-rpcconnect=") == 0) {
            rpcHost = arg.substr(12);
        } else if (arg.find("-rpcport=") == 0) {
            rpcPort = std::stoi(arg.substr(9));
        } else if (arg.find("-rpcuser=") == 0) {
            rpcUser = arg.substr(9);
        } else if (arg.find("-rpcpassword=") == 0) {
            rpcPassword = arg.substr(13);
        } else if (arg.find("-testnet") == 0) {
            rpcPort = 19334;  // Testnet default
        } else if (command.empty()) {
            command = arg;
        } else {
            params.push_back(arg);
        }
    }

    return !command.empty();
}

/**
 * @brief Send RPC request and print response
 */
bool SendRPCRequest(const std::string& rpcHost,
                   uint16_t rpcPort,
                   const std::string& rpcUser,
                   const std::string& rpcPassword,
                   const std::string& command,
                   const std::vector<std::string>& params) {
    // Build JSON-RPC request
    std::ostringstream request;
    request << "{";
    request << "\"jsonrpc\":\"2.0\",";
    request << "\"method\":\"" << command << "\",";
    request << "\"params\":[";

    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) request << ",";

        // Try to detect if param is a number
        bool isNumber = !params[i].empty() &&
                       (std::isdigit(params[i][0]) || params[i][0] == '-');

        if (isNumber) {
            request << params[i];
        } else {
            request << "\"" << params[i] << "\"";
        }
    }

    request << "],";
    request << "\"id\":1";
    request << "}";

    // In a real implementation, this would make an HTTP request to the RPC server
    // For now, just print what we would send
    std::cout << "RPC Request to " << rpcHost << ":" << rpcPort << "\n";
    std::cout << request.str() << "\n\n";

    std::cout << "Note: This is a demonstration CLI. In production:\n";
    std::cout << "- This would connect to the RPC server via HTTP\n";
    std::cout << "- Authenticate using provided credentials\n";
    std::cout << "- Send the JSON-RPC request\n";
    std::cout << "- Parse and display the response\n\n";

    std::cout << "To run a full node with RPC server, use: dinarid\n";

    return true;
}

int main(int argc, char** argv) {
    try {
        std::string rpcHost, rpcUser, rpcPassword, command;
        uint16_t rpcPort;
        std::vector<std::string> params;

        if (!ParseArguments(argc, argv, rpcHost, rpcPort, rpcUser, rpcPassword,
                           command, params)) {
            if (command.empty()) {
                PrintUsage();
            }
            return 0;
        }

        // Execute RPC request
        if (!SendRPCRequest(rpcHost, rpcPort, rpcUser, rpcPassword,
                           command, params)) {
            std::cerr << "Error: Failed to execute RPC request\n";
            return 1;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
