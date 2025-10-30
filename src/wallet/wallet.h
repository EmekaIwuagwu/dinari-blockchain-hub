#ifndef DINARI_WALLET_WALLET_H
#define DINARI_WALLET_WALLET_H

#include "keystore.h"
#include "hdwallet.h"
#include "address.h"
#include "core/transaction.h"
#include "core/utxo.h"
#include <memory>
#include <mutex>
#include <optional>

namespace dinari {

/**
 * @brief Wallet configuration
 */
struct WalletConfig {
    std::string dataDir;
    bool useHDWallet;
    uint32_t hdAccount;
    size_t keyPoolSize;

    WalletConfig()
        : dataDir(".")
        , useHDWallet(true)
        , hdAccount(0)
        , keyPoolSize(100) {}
};

/**
 * @brief Transaction builder
 *
 * Helps create and sign transactions
 */
class TransactionBuilder {
public:
    TransactionBuilder();

    /**
     * @brief Add input
     */
    TransactionBuilder& AddInput(const OutPoint& outpoint, const TxOut& prevOut);

    /**
     * @brief Add output
     */
    TransactionBuilder& AddOutput(const Address& addr, Amount value);

    /**
     * @brief Set change address
     */
    TransactionBuilder& SetChangeAddress(const Address& addr);

    /**
     * @brief Set fee rate (satoshis per byte)
     */
    TransactionBuilder& SetFeeRate(Amount feeRate);

    /**
     * @brief Build unsigned transaction
     */
    bool Build(Transaction& tx, Amount& fee);

    /**
     * @brief Sign transaction with key store
     */
    bool Sign(Transaction& tx, const KeyStore& keystore);

    /**
     * @brief Get total input value
     */
    Amount GetInputValue() const { return totalInput; }

    /**
     * @brief Get total output value
     */
    Amount GetOutputValue() const { return totalOutput; }

    /**
     * @brief Clear builder
     */
    void Clear();

private:
    struct Input {
        OutPoint outpoint;
        TxOut prevOut;
    };

    std::vector<Input> inputs;
    std::vector<TxOut> outputs;
    Address changeAddress;
    Amount feeRate;
    Amount totalInput;
    Amount totalOutput;

    Amount EstimateFee(size_t txSize) const;
    size_t EstimateTransactionSize() const;
};

/**
 * @brief Main wallet class
 *
 * Comprehensive wallet implementation with:
 * - Key management (encrypted storage)
 * - HD wallet support (BIP32/39/44)
 * - Address generation and management
 * - Transaction creation and signing
 * - Balance tracking
 * - UTXO management
 */
class Wallet {
public:
    explicit Wallet(const WalletConfig& config = WalletConfig());
    ~Wallet();

    /**
     * @brief Initialize wallet
     */
    bool Initialize();

    /**
     * @brief Create new HD wallet from mnemonic
     */
    bool CreateFromMnemonic(const std::vector<std::string>& mnemonic,
                           const std::string& passphrase = "");

    /**
     * @brief Generate new mnemonic
     */
    std::vector<std::string> GenerateNewMnemonic(size_t words = 12);

    /**
     * @brief Import HD wallet from seed
     */
    bool ImportSeed(const bytes& seed);

    /**
     * @brief Import private key
     */
    bool ImportPrivateKey(const Hash256& privKey, const std::string& label = "");

    /**
     * @brief Generate new address
     */
    Address GetNewAddress(const std::string& label = "");

    /**
     * @brief Generate new change address
     */
    Address GetNewChangeAddress();

    /**
     * @brief Get all addresses
     */
    std::vector<Address> GetAddresses() const;

    /**
     * @brief Get receiving addresses
     */
    std::vector<Address> GetReceivingAddresses() const;

    /**
     * @brief Check if address is mine
     */
    bool IsMine(const Address& addr) const;

    /**
     * @brief Encrypt wallet with passphrase
     */
    bool EncryptWallet(const std::string& passphrase);

    /**
     * @brief Lock wallet
     */
    void Lock();

    /**
     * @brief Unlock wallet
     */
    bool Unlock(const std::string& passphrase);

    /**
     * @brief Unlock wallet with timeout (auto-lock after specified seconds)
     * @param passphrase Wallet passphrase
     * @param timeoutSeconds Seconds until auto-lock (0 = no auto-lock)
     */
    bool UnlockWithTimeout(const std::string& passphrase, uint32_t timeoutSeconds);

    /**
     * @brief Check if wallet is locked
     */
    bool IsLocked() const;

    /**
     * @brief Check if wallet is encrypted
     */
    bool IsEncrypted() const;

    /**
     * @brief Change passphrase
     */
    bool ChangePassphrase(const std::string& oldPassphrase, const std::string& newPassphrase);

    /**
     * @brief Get balance
     */
    Amount GetBalance() const;

    /**
     * @brief Get unconfirmed balance
     */
    Amount GetUnconfirmedBalance() const;

    /**
     * @brief Get available balance (confirmed and spendable)
     */
    Amount GetAvailableBalance() const;

    /**
     * @brief Create transaction
     *
     * @param recipients Map of address to amount
     * @param feeRate Fee rate in satoshis per byte
     * @param tx Output transaction
     * @return true on success
     */
    bool CreateTransaction(const std::map<Address, Amount>& recipients,
                          Amount feeRate,
                          Transaction& tx);

    /**
     * @brief Send transaction
     *
     * Creates, signs, and returns transaction ready for broadcast
     */
    bool SendTransaction(const std::map<Address, Amount>& recipients,
                        Amount feeRate,
                        Transaction& tx);

    /**
     * @brief Sign transaction
     */
    bool SignTransaction(Transaction& tx);

    /**
     * @brief Get wallet UTXOs
     */
    std::vector<std::pair<OutPoint, TxOut>> GetUTXOs() const;

    /**
     * @brief Add UTXO to wallet
     */
    bool AddUTXO(const OutPoint& outpoint, const TxOut& txout, BlockHeight height);

    /**
     * @brief Remove UTXO from wallet
     */
    bool RemoveUTXO(const OutPoint& outpoint);

    /**
     * @brief Process transaction (update wallet state)
     */
    bool ProcessTransaction(const Transaction& tx, BlockHeight height);

    /**
     * @brief Get transaction history
     */
    std::vector<Transaction> GetTransactions() const;

    /**
     * @brief Save wallet to disk
     */
    bool Save();

    /**
     * @brief Load wallet from disk
     */
    bool Load();

    /**
     * @brief Get wallet info
     */
    struct WalletInfo {
        size_t keyCount;
        size_t addressCount;
        size_t utxoCount;
        Amount balance;
        bool encrypted;
        bool locked;
        bool hdEnabled;
    };

    WalletInfo GetInfo() const;

private:
    WalletConfig config;

    // Key management
    std::unique_ptr<CryptoKeyStore> keystore;

    // HD wallet
    std::unique_ptr<HDWallet> hdWallet;
    ExtendedKey accountKey;
    uint32_t nextReceivingIndex;
    uint32_t nextChangeIndex;

    // Address management
    AddressBook addressBook;

    // UTXO tracking
    std::map<OutPoint, TxOut> walletUTXOs;
    std::map<OutPoint, BlockHeight> utxoHeights;

    // Transaction history
    std::vector<Transaction> transactions;

    // Synchronization
    mutable std::mutex mutex;

    // Auto-lock functionality
    Timestamp unlockUntil;
    std::atomic<bool> autoLockRunning;
    std::thread autoLockThread;
    void AutoLockThreadFunc();

    // Helper methods
    bool DeriveNextAddress(bool isChange, Address& addr, ExtendedKey& key);
    bool SelectCoins(Amount targetValue, Amount feeRate,
                    std::vector<std::pair<OutPoint, TxOut>>& selected,
                    Amount& selectedValue);
    bytes GetScriptPubKeyForAddress(const Address& addr);
    bool ExtractAddressFromScriptPubKey(const bytes& scriptPubKey, Address& addr);

    std::string GetWalletFilePath() const;
    bool SaveToFile(const std::string& filepath);
    bool LoadFromFile(const std::string& filepath);
};

} // namespace dinari

#endif // DINARI_WALLET_WALLET_H
