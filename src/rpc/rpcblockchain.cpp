#include "rpcblockchain.h"
#include "util/logger.h"

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

} // namespace dinari
