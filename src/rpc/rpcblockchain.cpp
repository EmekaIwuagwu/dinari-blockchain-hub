#include "rpcblockchain.h"
#include "util/logger.h"
#include "util/time.h"
#include "wallet/address.h"
#include <ios>
#include <iomanip>
#include <sstream>

namespace dinari {

void BlockchainRPC::RegisterCommands(RPCServer& server) {
    // Blockchain query commands
    server.RegisterCommand(RPCCommand(
        "getblockcount",
        GetBlockCount,
        "blockchain",
        "Returns the height of the most-work fully-validated chain",
        "getblockcount"
    ));

    server.RegisterCommand(RPCCommand(
        "getblockhash",
        GetBlockHash,
        "blockchain",
        "Returns hash of block in best-block-chain at height provided",
        "getblockhash <height>"
    ));

    server.RegisterCommand(RPCCommand(
        "getblock",
        GetBlock,
        "blockchain",
        "Returns block information for the given block hash",
        "getblock <blockhash> [verbose=true]"
    ));

    server.RegisterCommand(RPCCommand(
        "getbestblockhash",
        GetBestBlockHash,
        "blockchain",
        "Returns the hash of the best (tip) block in the longest blockchain",
        "getbestblockhash"
    ));

    server.RegisterCommand(RPCCommand(
        "getdifficulty",
        GetDifficulty,
        "blockchain",
        "Returns the proof-of-work difficulty as a multiple of the minimum difficulty",
        "getdifficulty"
    ));

    server.RegisterCommand(RPCCommand(
        "getblockchaininfo",
        GetBlockchainInfo,
        "blockchain",
        "Returns an object containing various state info regarding blockchain processing",
        "getblockchaininfo"
    ));

    server.RegisterCommand(RPCCommand(
        "gettxout",
        GetTxOut,
        "blockchain",
        "Returns details about an unspent transaction output (UTXO)",
        "gettxout <txid> <n>"
    ));

    // Mempool commands
    server.RegisterCommand(RPCCommand(
        "getmempoolinfo",
        GetMempoolInfo,
        "blockchain",
        "Returns details on the active state of the TX memory pool",
        "getmempoolinfo"
    ));

    server.RegisterCommand(RPCCommand(
        "getrawmempool",
        GetRawMempool,
        "blockchain",
        "Returns all transaction ids in memory pool",
        "getrawmempool [verbose=false]"
    ));

    // Blockchain Explorer commands
    server.RegisterCommand(RPCCommand(
        "getrawtransaction",
        GetRawTransaction,
        "blockchain",
        "Returns the raw transaction data by transaction hash",
        "getrawtransaction <txid> [verbose=true]"
    ));

    server.RegisterCommand(RPCCommand(
        "listblocks",
        ListBlocks,
        "blockchain",
        "Returns a list of blocks with details (height, hash, timestamp, transactions, miner)",
        "listblocks [start_height=0] [count=10]"
    ));

    // Utility commands
    server.RegisterCommand(RPCCommand(
        "help",
        Help,
        "control",
        "List all commands, or get help for a specified command",
        "help [command]"
    ));

    server.RegisterCommand(RPCCommand(
        "stop",
        Stop,
        "control",
        "Stop Dinari server",
        "stop"
    ));

    LOG_INFO("RPC", "Registered blockchain RPC commands");
}

// Command implementations

JSONValue BlockchainRPC::GetBlockCount(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    const BlockIndex* tip = chain.GetBestBlock();
    if (!tip) {
        return JSONValue(static_cast<int64_t>(0));
    }

    return JSONValue(static_cast<int64_t>(tip->height));
}

JSONValue BlockchainRPC::GetBlockHash(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 1);

    int64_t height = RPCHelper::GetIntParam(req, 0);

    if (height < 0) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    Hash256 blockHash = chain.GetBlockHash(static_cast<BlockHeight>(height));
    if (blockHash == Hash256{0}) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Block height out of range");
    }

    return JSONValue(blockHash.ToHex());
}

JSONValue BlockchainRPC::GetBlock(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 1, 2);

    std::string hashStr = RPCHelper::GetStringParam(req, 0);
    bool verbose = true;

    if (req.params.size() > 1) {
        verbose = RPCHelper::GetBoolParam(req, 1);
    }

    Hash256 blockHash;
    if (!blockHash.FromHex(hashStr)) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid block hash");
    }

    auto block = chain.GetBlock(blockHash);
    if (!block) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Block not found");
    }

    if (!verbose) {
        // Return hex-encoded block
        bytes serialized = block->Serialize();
        return JSONValue(block->GetHash().ToHex());  // Simplified
    }

    // Return JSON object with block details
    JSONObject obj = RPCHelper::BlockToJSON(*block, chain);

    // Add transaction hashes
    obj.SetInt("tx_count", block->transactions.size());

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::GetBestBlockHash(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    const BlockIndex* tip = chain.GetBestBlock();
    if (!tip) {
        return JSONValue("");
    }

    return JSONValue(tip->hash.ToHex());
}

JSONValue BlockchainRPC::GetDifficulty(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    const BlockIndex* tip = chain.GetBestBlock();
    if (!tip) {
        return JSONValue(1.0);
    }

    // Convert bits to difficulty
    // Difficulty = max_target / current_target
    // Simplified calculation
    double difficulty = static_cast<double>(tip->bits);

    return JSONValue(difficulty);
}

JSONValue BlockchainRPC::GetBlockchainInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    JSONObject obj;

    const BlockIndex* tip = chain.GetBestBlock();

    obj.SetString("chain", "main");
    obj.SetInt("blocks", tip ? tip->height : 0);
    obj.SetInt("headers", tip ? tip->height : 0);
    obj.SetString("bestblockhash", tip ? tip->hash.ToHex() : "");
    obj.SetDouble("difficulty", tip ? static_cast<double>(tip->bits) : 1.0);
    obj.SetString("chainwork", tip ? tip->chainWork.ToHex() : "");

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::GetTxOut(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 2);

    std::string txidStr = RPCHelper::GetStringParam(req, 0);
    int64_t n = RPCHelper::GetIntParam(req, 1);

    Hash256 txid;
    if (!txid.FromHex(txidStr)) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid transaction id");
    }

    if (n < 0) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Output index out of range");
    }

    OutPoint outpoint;
    outpoint.hash = txid;
    outpoint.n = static_cast<uint32_t>(n);

    const UTXOSet& utxos = chain.GetUTXOs();
    UTXOEntry entry;

    if (!utxos.GetUTXO(outpoint, entry)) {
        // Not found or spent
        return JSONValue();  // null
    }

    // Return UTXO details
    JSONObject obj;
    obj.SetString("bestblock", chain.GetBestBlock()->hash.ToHex());
    obj.SetInt("confirmations", chain.GetBestBlock()->height - entry.height + 1);
    obj.SetInt("value", entry.output.value);
    obj.SetInt("height", entry.height);
    obj.SetBool("coinbase", entry.isCoinbase);

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::GetMempoolInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    const MemPool& mempool = chain.GetMemPool();
    MemPoolStats stats = mempool.GetStats();

    JSONObject obj;
    obj.SetInt("size", stats.transactionCount);
    obj.SetInt("bytes", stats.totalSize);
    obj.SetInt("usage", stats.memoryUsage);
    obj.SetInt("maxmempool", stats.maxMemoryUsage);

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::GetRawMempool(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 0, 1);

    bool verbose = false;
    if (req.params.size() > 0) {
        verbose = RPCHelper::GetBoolParam(req, 0);
    }

    const MemPool& mempool = chain.GetMemPool();
    std::vector<Transaction> transactions = mempool.GetAllTransactions();

    if (!verbose) {
        // Return array of txids
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < transactions.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << transactions[i].GetHash().ToHex() << "\"";
        }
        oss << "]";
        return JSONValue(oss.str());
    }

    // Return object with detailed info
    JSONObject obj;
    for (const auto& tx : transactions) {
        obj.SetObject(tx.GetHash().ToHex(), RPCHelper::TransactionToJSON(tx));
    }

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::Help(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    // TODO: Return list of all commands with descriptions
    JSONValue result("Help: List of available RPC commands");
    return result;
}

JSONValue BlockchainRPC::Stop(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    LOG_INFO("RPC", "Stop command received");

    // TODO: Initiate graceful shutdown

    return JSONValue("Dinari server stopping");
}

// Blockchain Explorer implementations

JSONValue BlockchainRPC::GetRawTransaction(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 1, 2);

    std::string txidStr = RPCHelper::GetStringParam(req, 0);
    bool verbose = true;

    if (req.params.size() > 1) {
        verbose = RPCHelper::GetBoolParam(req, 1);
    }

    Hash256 txid;
    if (!txid.FromHex(txidStr)) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid transaction id");
    }

    // First, check mempool
    const MemPool& mempool = chain.GetMemPool();
    Transaction tx;
    bool found = false;
    BlockHeight txHeight = 0;
    Hash256 blockHash;
    int confirmations = 0;

    if (mempool.GetTransaction(txid, tx)) {
        found = true;
        confirmations = 0; // Unconfirmed
    } else {
        // Search in blockchain
        // We need to iterate through blocks to find the transaction
        const BlockIndex* tip = chain.GetBestBlock();
        if (tip) {
            // Start from genesis and search
            for (BlockHeight height = 0; height <= tip->height; ++height) {
                Hash256 bhash = chain.GetBlockHash(height);
                auto block = chain.GetBlock(bhash);

                if (block) {
                    for (const auto& transaction : block->transactions) {
                        if (transaction.GetHash() == txid) {
                            tx = transaction;
                            found = true;
                            txHeight = height;
                            blockHash = bhash;
                            confirmations = tip->height - height + 1;
                            break;
                        }
                    }
                }

                if (found) break;
            }
        }
    }

    if (!found) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Transaction not found");
    }

    if (!verbose) {
        // Return hex-encoded transaction
        bytes serialized = tx.Serialize();
        std::ostringstream oss;
        for (byte b : serialized) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        return JSONValue(oss.str());
    }

    // Return JSON object with transaction details
    JSONObject obj;
    obj.SetString("txid", tx.GetHash().ToHex());
    obj.SetInt("version", tx.version);
    obj.SetInt("locktime", tx.lockTime);
    obj.SetInt("size", tx.Serialize().size());
    obj.SetBool("coinbase", tx.IsCoinbase());

    // Add inputs
    std::ostringstream vinOss;
    vinOss << "[";
    for (size_t i = 0; i < tx.vin.size(); ++i) {
        if (i > 0) vinOss << ",";
        vinOss << "{\"prevout\":{\"hash\":\"" << tx.vin[i].prevOut.hash.ToHex()
               << "\",\"n\":" << tx.vin[i].prevOut.n << "},"
               << "\"scriptSig\":\"" << "..." << "\","  // Simplified
               << "\"sequence\":" << tx.vin[i].sequence << "}";
    }
    vinOss << "]";
    obj.SetString("vin", vinOss.str());

    // Add outputs
    std::ostringstream voutOss;
    voutOss << "[";
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        if (i > 0) voutOss << ",";
        voutOss << "{\"value\":" << tx.vout[i].value
                << ",\"n\":" << i
                << ",\"scriptPubKey\":\"" << "..." << "\"}";  // Simplified
    }
    voutOss << "]";
    obj.SetString("vout", voutOss.str());

    // Add block info if confirmed
    if (confirmations > 0) {
        obj.SetString("blockhash", blockHash.ToHex());
        obj.SetInt("blockheight", txHeight);
        obj.SetInt("confirmations", confirmations);

        // Get block timestamp
        auto block = chain.GetBlock(blockHash);
        if (block) {
            obj.SetInt("blocktime", block->header.timestamp);
        }
    } else {
        obj.SetInt("confirmations", 0);
    }

    return JSONValue(obj.Serialize());
}

JSONValue BlockchainRPC::ListBlocks(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 0, 2);

    int64_t startHeight = 0;
    int64_t count = 10;

    if (req.params.size() > 0) {
        startHeight = RPCHelper::GetIntParam(req, 0);
    }

    if (req.params.size() > 1) {
        count = RPCHelper::GetIntParam(req, 1);
    }

    if (startHeight < 0) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Start height must be non-negative");
    }

    if (count < 1 || count > 100) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Count must be between 1 and 100");
    }

    const BlockIndex* tip = chain.GetBestBlock();
    if (!tip) {
        return JSONValue("[]");  // Empty array
    }

    // Build blocks array
    std::ostringstream oss;
    oss << "[";

    int64_t endHeight = std::min(startHeight + count - 1, static_cast<int64_t>(tip->height));
    bool first = true;

    for (int64_t height = startHeight; height <= endHeight; ++height) {
        Hash256 blockHash = chain.GetBlockHash(static_cast<BlockHeight>(height));
        auto block = chain.GetBlock(blockHash);

        if (!block) {
            continue;  // Skip if block not found
        }

        if (!first) oss << ",";
        first = false;

        oss << "{";
        oss << "\"height\":" << height << ",";
        oss << "\"hash\":\"" << block->GetHash().ToHex() << "\",";
        oss << "\"timestamp\":" << block->header.timestamp << ",";
        oss << "\"time\":\"" << Time::FormatTime(block->header.timestamp) << "\",";
        oss << "\"tx_count\":" << block->transactions.size() << ",";
        oss << "\"size\":" << block->Serialize().size() << ",";
        oss << "\"bits\":" << block->header.bits << ",";
        oss << "\"nonce\":" << block->header.nonce << ",";
        oss << "\"merkleroot\":\"" << block->header.merkleRoot.ToHex() << "\",";

        // Add transaction hashes
        oss << "\"transactions\":[";
        for (size_t i = 0; i < block->transactions.size(); ++i) {
            if (i > 0) oss << ",";
            oss << "\"" << block->transactions[i].GetHash().ToHex() << "\"";
        }
        oss << "],";

        // Extract miner address from coinbase (first transaction)
        std::string minerAddress = "unknown";
        if (!block->transactions.empty() && block->transactions[0].IsCoinbase()) {
            // Try to extract address from first output
            if (!block->transactions[0].vout.empty()) {
                Address addr;
                if (AddressGenerator::ExtractAddress(block->transactions[0].vout[0].scriptPubKey, addr)) {
                    minerAddress = addr.ToString();
                }
            }
        }

        oss << "\"miner\":\"" << minerAddress << "\",";
        oss << "\"confirmations\":" << (tip->height - height + 1);
        oss << "}";
    }

    oss << "]";

    return JSONValue(oss.str());
}

} // namespace dinari
