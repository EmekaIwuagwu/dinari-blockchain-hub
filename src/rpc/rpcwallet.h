#ifndef DINARI_RPC_RPCWALLET_H
#define DINARI_RPC_RPCWALLET_H

#include "rpcserver.h"

namespace dinari {

/**
 * @brief Wallet RPC commands
 *
 * Implements wallet management commands:
 * - getnewaddress
 * - getbalance
 * - sendtoaddress
 * - getwalletinfo
 * - encryptwallet
 * - walletlock
 * - walletpassphrase
 * - listaddresses
 * - listtransactions
 * - listunspent
 */
class WalletRPC {
public:
    /**
     * @brief Register all wallet RPC commands
     */
    static void RegisterCommands(RPCServer& server);

private:
    // Address management
    static JSONValue GetNewAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetAddressInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ListAddresses(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ValidateAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // Balance queries
    static JSONValue GetBalance(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetUnconfirmedBalance(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ListUnspent(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // Transactions
    static JSONValue SendToAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue SendToken(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ListTransactions(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue GetTransaction(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // Wallet management
    static JSONValue GetWalletInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue EncryptWallet(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue WalletLock(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue WalletPassphrase(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue WalletPassphraseChange(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);

    // HD wallet
    static JSONValue GetMnemonic(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ImportMnemonic(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
    static JSONValue ImportPrivKey(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node);
};

} // namespace dinari

#endif // DINARI_RPC_RPCWALLET_H
