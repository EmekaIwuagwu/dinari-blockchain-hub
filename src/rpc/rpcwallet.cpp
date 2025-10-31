#include "rpcwallet.h"
#include "util/logger.h"

namespace dinari {

void WalletRPC::RegisterCommands(RPCServer& server) {
    // Address management
    server.RegisterCommand(RPCCommand(
        "getnewaddress",
        GetNewAddress,
        "wallet",
        "Returns a new Dinari address for receiving payments",
        "getnewaddress [label]",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "getaddressinfo",
        GetAddressInfo,
        "wallet",
        "Return information about the given dinari address",
        "getaddressinfo <address>",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "listaddresses",
        ListAddresses,
        "wallet",
        "Returns the list of all addresses in the wallet",
        "listaddresses",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "validateaddress",
        ValidateAddress,
        "wallet",
        "Return information about the given dinari address",
        "validateaddress <address>",
        false
    ));

    // Balance queries
    server.RegisterCommand(RPCCommand(
        "getbalance",
        GetBalance,
        "wallet",
        "Returns the total available balance",
        "getbalance",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "getunconfirmedbalance",
        GetUnconfirmedBalance,
        "wallet",
        "Returns the server's total unconfirmed balance",
        "getunconfirmedbalance",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "listunspent",
        ListUnspent,
        "wallet",
        "Returns array of unspent transaction outputs",
        "listunspent [minconf=1] [maxconf=9999999]",
        true
    ));

    // Transactions
    server.RegisterCommand(RPCCommand(
        "sendtoaddress",
        SendToAddress,
        "wallet",
        "Send an amount to a given address",
        "sendtoaddress <address> <amount> [comment]",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "sendtoken",
        SendToken,
        "wallet",
        "Send DNT tokens with detailed request/response format",
        "sendtoken {\"addressTo\":\"<address>\", \"amount\":<amount>, \"addressFrom\":\"<address>\" (optional)}",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "listtransactions",
        ListTransactions,
        "wallet",
        "Returns up to 'count' most recent transactions",
        "listtransactions [count=10]",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "gettransaction",
        GetTransaction,
        "wallet",
        "Get detailed information about in-wallet transaction",
        "gettransaction <txid>",
        true
    ));

    // Wallet management
    server.RegisterCommand(RPCCommand(
        "getwalletinfo",
        GetWalletInfo,
        "wallet",
        "Returns an object containing various wallet state info",
        "getwalletinfo",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "encryptwallet",
        EncryptWallet,
        "wallet",
        "Encrypts the wallet with 'passphrase'",
        "encryptwallet <passphrase>",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "walletlock",
        WalletLock,
        "wallet",
        "Removes the wallet encryption key from memory, locking the wallet",
        "walletlock",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "walletpassphrase",
        WalletPassphrase,
        "wallet",
        "Stores the wallet decryption key in memory for 'timeout' seconds",
        "walletpassphrase <passphrase> <timeout>",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "walletpassphrasechange",
        WalletPassphraseChange,
        "wallet",
        "Changes the wallet passphrase from 'oldpassphrase' to 'newpassphrase'",
        "walletpassphrasechange <oldpassphrase> <newpassphrase>",
        true
    ));

    // HD wallet
    server.RegisterCommand(RPCCommand(
        "importmnemonic",
        ImportMnemonic,
        "wallet",
        "Import HD wallet from mnemonic phrase",
        "importmnemonic <mnemonic> [passphrase]",
        true
    ));

    server.RegisterCommand(RPCCommand(
        "importprivkey",
        ImportPrivKey,
        "wallet",
        "Adds a private key to your wallet",
        "importprivkey <privkey> [label]",
        true
    ));

    LOG_INFO("RPC", "Registered wallet RPC commands");
}

// Command implementations

JSONValue WalletRPC::GetNewAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 0, 1);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::string label = "";
    if (req.params.size() > 0) {
        label = RPCHelper::GetStringParam(req, 0);
    }

    Address addr = wallet->GetNewAddress(label);
    if (!addr.IsValid()) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Failed to generate new address");
    }

    return JSONValue(addr.ToString());
}

JSONValue WalletRPC::GetAddressInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 1);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::string addrStr = RPCHelper::GetStringParam(req, 0);
    Address addr(addrStr);

    if (!addr.IsValid()) {
        RPCHelper::ThrowError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    JSONObject obj;
    obj.SetString("address", addr.ToString());
    obj.SetBool("ismine", wallet->IsMine(addr));
    obj.SetBool("isvalid", true);

    return JSONValue(obj.Serialize());
}

JSONValue WalletRPC::ListAddresses(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::vector<Address> addresses = wallet->GetAddresses();

    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < addresses.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "\"" << addresses[i].ToString() << "\"";
    }
    oss << "]";

    return JSONValue(oss.str());
}

JSONValue WalletRPC::ValidateAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 1);

    std::string addrStr = RPCHelper::GetStringParam(req, 0);

    JSONObject obj;
    obj.SetString("address", addrStr);

    bool isValid = Address::Validate(addrStr);
    obj.SetBool("isvalid", isValid);

    if (isValid && wallet) {
        Address addr(addrStr);
        obj.SetBool("ismine", wallet->IsMine(addr));
    }

    return JSONValue(obj.Serialize());
}

JSONValue WalletRPC::GetBalance(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    Amount balance = wallet->GetAvailableBalance();

    // Return balance in DNT (divide by COIN)
    double dntBalance = static_cast<double>(balance) / COIN;

    return JSONValue(dntBalance);
}

JSONValue WalletRPC::GetUnconfirmedBalance(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    Amount balance = wallet->GetUnconfirmedBalance();
    double dntBalance = static_cast<double>(balance) / COIN;

    return JSONValue(dntBalance);
}

JSONValue WalletRPC::ListUnspent(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 0, 2);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::vector<std::pair<OutPoint, TxOut>> utxos = wallet->GetUTXOs();

    std::ostringstream oss;
    oss << "[";

    for (size_t i = 0; i < utxos.size(); ++i) {
        if (i > 0) oss << ",";

        JSONObject obj;
        obj.SetString("txid", utxos[i].first.hash.ToHex());
        obj.SetInt("vout", utxos[i].first.n);
        obj.SetDouble("amount", static_cast<double>(utxos[i].second.value) / COIN);

        oss << obj.Serialize();
    }

    oss << "]";

    return JSONValue(oss.str());
}

JSONValue WalletRPC::SendToAddress(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 2, 3);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (wallet->IsLocked()) {
        RPCHelper::ThrowError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first");
    }

    // Parse parameters
    std::string addrStr = RPCHelper::GetStringParam(req, 0);
    double amount = RPCHelper::GetDoubleParam(req, 1);

    Address toAddr(addrStr);
    if (!toAddr.IsValid()) {
        RPCHelper::ThrowError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dinari address");
    }

    // Convert amount to satoshis
    Amount amountSatoshis = static_cast<Amount>(amount * COIN);

    if (amountSatoshis <= 0 || amountSatoshis > MAX_MONEY) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    // Create transaction
    std::map<Address, Amount> recipients;
    recipients[toAddr] = amountSatoshis;

    Transaction tx;
    if (!wallet->SendTransaction(recipients, 10, tx)) {  // 10 sat/byte fee rate
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Failed to create transaction");
    }

    // Broadcast transaction
    if (node) {
        node->BroadcastTransaction(tx);
    }

    LOG_INFO("RPC", "Sent " + std::to_string(amount) + " DNT to " + addrStr);

    return JSONValue(crypto::Hash::ToHex(tx.GetHash()));
}

JSONValue WalletRPC::SendToken(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    // Enhanced token sending API with detailed request/response

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (wallet->IsLocked()) {
        RPCHelper::ThrowError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first");
    }

    // Parse JSON request body
    // Expected format: {"addressTo": "...", "amount": 100.5, "addressFrom": "..." (optional)}
    if (req.params.empty()) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Missing request parameters");
    }

    std::string requestJson = req.params[0];
    JSONObject requestObj;
    if (!requestObj.Parse(requestJson)) {
        RPCHelper::ThrowError(RPC_PARSE_ERROR, "Invalid JSON in request");
    }

    // Extract parameters
    std::string addressTo = requestObj.GetString("addressTo", "");
    double amount = requestObj.GetDouble("amount", 0.0);
    std::string addressFrom = requestObj.GetString("addressFrom", "");  // Optional, for display purposes

    if (addressTo.empty()) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Missing 'addressTo' parameter");
    }

    if (amount <= 0) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid 'amount' parameter");
    }

    // Validate destination address
    Address toAddr(addressTo);
    if (!toAddr.IsValid()) {
        RPCHelper::ThrowError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid destination address");
    }

    // Convert amount to satoshis
    Amount amountSatoshis = static_cast<Amount>(amount * COIN);

    if (amountSatoshis <= 0 || amountSatoshis > MAX_MONEY) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Amount out of range");
    }

    // Get source address (if not provided, use default from wallet)
    std::string fromAddress;
    if (addressFrom.empty()) {
        // Get default address from wallet
        std::vector<Address> addresses = wallet->GetAddresses();
        if (!addresses.empty()) {
            fromAddress = addresses[0].ToString();
        } else {
            RPCHelper::ThrowError(RPC_WALLET_ERROR, "No addresses in wallet");
        }
    } else {
        // Validate provided source address
        Address fromAddr(addressFrom);
        if (!fromAddr.IsValid() || !wallet->IsMine(fromAddr)) {
            RPCHelper::ThrowError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-owned source address");
        }
        fromAddress = addressFrom;
    }

    // Get current timestamp
    Timestamp timestamp = Time::GetCurrentTime();

    // Create transaction
    std::map<Address, Amount> recipients;
    recipients[toAddr] = amountSatoshis;

    Transaction tx;
    if (!wallet->SendTransaction(recipients, 10, tx)) {  // 10 sat/byte fee rate
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Failed to create transaction");
    }

    // Broadcast transaction to network
    if (node) {
        node->BroadcastTransaction(tx);
    }

    // Log transaction
    LOG_INFO("RPC", "SendToken: " + std::to_string(amount) + " DNT from " + fromAddress + " to " + addressTo);

    // Build detailed response
    // Format: {addressFrom, addressTo, amount, coin_type, timestamp, transactionHash}
    JSONObject response;
    response.SetString("addressFrom", fromAddress);
    response.SetString("addressTo", addressTo);
    response.SetDouble("amount", amount);
    response.SetString("coin_type", "DNT");
    response.SetInt("timestamp", static_cast<int64_t>(timestamp));
    response.SetString("transactionHash", crypto::Hash::ToHex(tx.GetHash()));
    response.SetString("status", "submitted_to_mempool");

    return JSONValue(response.Serialize());
}

JSONValue WalletRPC::ListTransactions(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 0, 1);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    int64_t count = 10;
    if (req.params.size() > 0) {
        count = RPCHelper::GetIntParam(req, 0);
    }

    std::vector<Transaction> transactions = wallet->GetTransactions();

    std::ostringstream oss;
    oss << "[";

    size_t start = transactions.size() > static_cast<size_t>(count) ?
                   transactions.size() - count : 0;

    for (size_t i = start; i < transactions.size(); ++i) {
        if (i > start) oss << ",";
        oss << RPCHelper::TransactionToJSON(transactions[i]).Serialize();
    }

    oss << "]";

    return JSONValue(oss.str());
}

JSONValue WalletRPC::GetTransaction(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 1);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::string txidStr = RPCHelper::GetStringParam(req, 0);

    Hash256 txid;
    if (!txid.FromHex(txidStr)) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "Invalid transaction id");
    }

    // Note: Wallet transaction history lookup requires indexed transaction database

    JSONObject obj;
    obj.SetString("txid", txidStr);

    return JSONValue(obj.Serialize());
}

JSONValue WalletRPC::GetWalletInfo(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    Wallet::WalletInfo info = wallet->GetInfo();

    JSONObject obj;
    obj.SetInt("keypool_size", info.keyCount);
    obj.SetInt("address_count", info.addressCount);
    obj.SetInt("utxo_count", info.utxoCount);
    obj.SetDouble("balance", static_cast<double>(info.balance) / COIN);
    obj.SetBool("encrypted", info.encrypted);
    obj.SetBool("locked", info.locked);
    obj.SetBool("hd_enabled", info.hdEnabled);

    return JSONValue(obj.Serialize());
}

JSONValue WalletRPC::EncryptWallet(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 1);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (wallet->IsEncrypted()) {
        RPCHelper::ThrowError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet");
    }

    std::string passphrase = RPCHelper::GetStringParam(req, 0);

    if (passphrase.length() < 1) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "passphrase can not be empty");
    }

    if (!wallet->EncryptWallet(passphrase)) {
        RPCHelper::ThrowError(RPC_WALLET_ENCRYPTION_FAILED, "Failed to encrypt the wallet");
    }

    return JSONValue("wallet encrypted; Dinari server stopping, restart to run with encrypted wallet");
}

JSONValue WalletRPC::WalletLock(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 0);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (!wallet->IsEncrypted()) {
        RPCHelper::ThrowError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet");
    }

    wallet->Lock();

    return JSONValue(true);
}

JSONValue WalletRPC::WalletPassphrase(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 2);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (!wallet->IsEncrypted()) {
        RPCHelper::ThrowError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet");
    }

    std::string passphrase = RPCHelper::GetStringParam(req, 0);
    int64_t timeout = RPCHelper::GetIntParam(req, 1);

    // Use timeout-based unlock if timeout is specified and positive
    bool success = false;
    if (timeout > 0 && timeout <= (int64_t)(UINT32_MAX)) {
        success = wallet->UnlockWithTimeout(passphrase, static_cast<uint32_t>(timeout));
    } else {
        success = wallet->Unlock(passphrase);
    }

    if (!success) {
        RPCHelper::ThrowError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect");
    }

    return JSONValue(true);
}

JSONValue WalletRPC::WalletPassphraseChange(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParams(req, 2);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (!wallet->IsEncrypted()) {
        RPCHelper::ThrowError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet");
    }

    std::string oldPassphrase = RPCHelper::GetStringParam(req, 0);
    std::string newPassphrase = RPCHelper::GetStringParam(req, 1);

    if (newPassphrase.length() < 1) {
        RPCHelper::ThrowError(RPC_INVALID_PARAMETER, "passphrase can not be empty");
    }

    if (!wallet->ChangePassphrase(oldPassphrase, newPassphrase)) {
        RPCHelper::ThrowError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect");
    }

    return JSONValue(true);
}

JSONValue WalletRPC::ImportMnemonic(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 1, 2);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    std::string mnemonicStr = RPCHelper::GetStringParam(req, 0);
    std::string passphrase = "";

    if (req.params.size() > 1) {
        passphrase = RPCHelper::GetStringParam(req, 1);
    }

    // Split mnemonic into words
    std::vector<std::string> mnemonic;
    std::istringstream iss(mnemonicStr);
    std::string word;
    while (iss >> word) {
        mnemonic.push_back(word);
    }

    if (!wallet->CreateFromMnemonic(mnemonic, passphrase)) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Failed to import mnemonic");
    }

    return JSONValue("Mnemonic imported successfully");
}

JSONValue WalletRPC::ImportPrivKey(const RPCRequest& req, Blockchain& chain, Wallet* wallet, NetworkNode* node) {
    RPCHelper::CheckParamsRange(req, 1, 2);

    if (!wallet) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Wallet not loaded");
    }

    if (wallet->IsLocked()) {
        RPCHelper::ThrowError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first");
    }

    std::string privKeyStr = RPCHelper::GetStringParam(req, 0);
    std::string label = "";

    if (req.params.size() > 1) {
        label = RPCHelper::GetStringParam(req, 1);
    }

    Hash256 privKey;
    if (!privKey.FromHex(privKeyStr)) {
        RPCHelper::ThrowError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }

    if (!wallet->ImportPrivateKey(privKey, label)) {
        RPCHelper::ThrowError(RPC_WALLET_ERROR, "Failed to import private key");
    }

    return JSONValue(true);
}

} // namespace dinari
