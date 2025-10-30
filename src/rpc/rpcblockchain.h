#ifndef DINARI_RPC_RPCBLOCKCHAIN_H
#define DINARI_RPC_RPCBLOCKCHAIN_H

#include "rpcserver.h"

namespace dinari {

/**
 * @brief Blockchain RPC commands
 *
 * Implements blockchain query commands:
 * - getblockcount
 * - getblockhash
 * - getblock
 * - getbestblockhash
 * - getdifficulty
 * - getblockchaininfo
 * - gettxout
 * - getmempoolinfo
 * - getrawmempool
 */
class BlockchainRPC {
public:
    /**
     * @brief Register all blockchain RPC commands
     */
    static void RegisterCommands(RPCServer& server);

private:
    // Blockchain query commands
    static JSONValue GetBlockCount(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetBlockHash(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetBlock(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetBestBlockHash(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetDifficulty(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetBlockchainInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetTxOut(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // Mempool commands
    static JSONValue GetMempoolInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetRawMempool(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // Utility commands
    static JSONValue Help(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue Stop(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
};

} // namespace dinari

#endif // DINARI_RPC_RPCBLOCKCHAIN_H
