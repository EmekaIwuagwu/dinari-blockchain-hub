#include "wallet.h"
#include "util/logger.h"
#include "util/time.h"
#include "util/serialize.h"
#include "core/script.h"
#include <fstream>
#include <algorithm>

namespace dinari {

// TransactionBuilder implementation

TransactionBuilder::TransactionBuilder()
    : feeRate(10)  // Default 10 satoshis/byte
    , totalInput(0)
    , totalOutput(0) {
}

TransactionBuilder& TransactionBuilder::AddInput(const OutPoint& outpoint, const TxOut& prevOut) {
    Input input;
    input.outpoint = outpoint;
    input.prevOut = prevOut;

    inputs.push_back(input);
    totalInput += prevOut.value;

    return *this;
}

TransactionBuilder& TransactionBuilder::AddOutput(const Address& addr, Amount value) {
    TxOut output;
    output.value = value;
    output.scriptPubKey = AddressGenerator::GenerateScriptPubKey(addr);

    outputs.push_back(output);
    totalOutput += value;

    return *this;
}

TransactionBuilder& TransactionBuilder::SetChangeAddress(const Address& addr) {
    changeAddress = addr;
    return *this;
}

TransactionBuilder& TransactionBuilder::SetFeeRate(Amount rate) {
    feeRate = rate;
    return *this;
}

bool TransactionBuilder::Build(Transaction& tx, Amount& fee) {
    if (inputs.empty()) {
        LOG_ERROR("TxBuilder", "No inputs");
        return false;
    }

    if (outputs.empty()) {
        LOG_ERROR("TxBuilder", "No outputs");
        return false;
    }

    // Estimate transaction size
    size_t estimatedSize = EstimateTransactionSize();
    fee = EstimateFee(estimatedSize);

    // Check if we have enough input
    if (totalInput < totalOutput + fee) {
        LOG_ERROR("TxBuilder", "Insufficient funds: need " +
                 std::to_string(totalOutput + fee) +
                 ", have " + std::to_string(totalInput));
        return false;
    }

    // Calculate change
    Amount change = totalInput - totalOutput - fee;

    // Build transaction
    tx.version = 1;
    tx.lockTime = 0;

    // Add inputs
    for (const auto& input : inputs) {
        TxIn txin;
        txin.prevOut = input.outpoint;
        txin.sequence = 0xFFFFFFFF;
        tx.vin.push_back(txin);
    }

    // Add outputs
    tx.vout = outputs;

    // Add change output if significant
    if (change >= DUST_THRESHOLD && changeAddress.IsValid()) {
        TxOut changeOut;
        changeOut.value = change;
        changeOut.scriptPubKey = AddressGenerator::GenerateScriptPubKey(changeAddress);
        tx.vout.push_back(changeOut);
    } else if (change > 0) {
        // Add to fee
        fee += change;
    }

    LOG_INFO("TxBuilder", "Built transaction: " + std::to_string(tx.vin.size()) +
             " inputs, " + std::to_string(tx.vout.size()) + " outputs, fee: " +
             std::to_string(fee));

    return true;
}

bool TransactionBuilder::Sign(Transaction& tx, const KeyStore& keystore) {
    for (size_t i = 0; i < tx.vin.size(); ++i) {
        if (i >= inputs.size()) {
            LOG_ERROR("TxBuilder", "Input index out of range");
            return false;
        }

        const TxOut& prevOut = inputs[i].prevOut;

        // Extract address from scriptPubKey
        Address addr;
        if (!AddressGenerator::ExtractAddress(prevOut.scriptPubKey, addr)) {
            LOG_ERROR("TxBuilder", "Failed to extract address from scriptPubKey");
            return false;
        }

        // Get key for address
        Hash160 keyID = addr.GetHash();
        Key key;
        if (!keystore.GetKey(keyID, key)) {
            LOG_ERROR("TxBuilder", "Key not found for address: " + addr.ToString());
            return false;
        }

        // Create signature
        Hash256 sighash = tx.GetSignatureHash(i, prevOut.scriptPubKey);
        bytes signature = crypto::ECDSA::Sign(sighash, key.privKey);

        if (signature.empty()) {
            LOG_ERROR("TxBuilder", "Failed to sign input " + std::to_string(i));
            return false;
        }

        // Add SIGHASH_ALL flag
        signature.push_back(0x01);

        // Build scriptSig for P2PKH
        bytes scriptSig;
        scriptSig.push_back(static_cast<byte>(signature.size()));
        scriptSig.insert(scriptSig.end(), signature.begin(), signature.end());
        scriptSig.push_back(static_cast<byte>(key.pubKey.size()));
        scriptSig.insert(scriptSig.end(), key.pubKey.begin(), key.pubKey.end());

        tx.vin[i].scriptSig = scriptSig;
    }

    LOG_INFO("TxBuilder", "Transaction signed");

    return true;
}

Amount TransactionBuilder::EstimateFee(size_t txSize) const {
    return txSize * feeRate;
}

size_t TransactionBuilder::EstimateTransactionSize() const {
    // Rough estimation:
    // Version: 4 bytes
    // Input count: 1 byte (varint)
    // Inputs: 148 bytes each (outpoint + scriptSig + sequence)
    // Output count: 1 byte (varint)
    // Outputs: 34 bytes each (value + scriptPubKey)
    // Locktime: 4 bytes

    size_t size = 4 + 1 + 1 + 4;  // Version + in count + out count + locktime
    size += inputs.size() * 148;
    size += outputs.size() * 34;
    size += 34;  // Change output

    return size;
}

void TransactionBuilder::Clear() {
    inputs.clear();
    outputs.clear();
    changeAddress = Address();
    totalInput = 0;
    totalOutput = 0;
}

// Wallet implementation

Wallet::Wallet(const WalletConfig& cfg)
    : config(cfg)
    , keystore(std::make_unique<CryptoKeyStore>())
    , nextReceivingIndex(0)
    , nextChangeIndex(0) {
}

Wallet::~Wallet() {
    Save();
}

bool Wallet::Initialize() {
    LOG_INFO("Wallet", "Initializing wallet");

    if (config.useHDWallet) {
        hdWallet = std::make_unique<HDWallet>();
    }

    // Try to load existing wallet
    if (Load()) {
        LOG_INFO("Wallet", "Loaded existing wallet");
        return true;
    }

    LOG_INFO("Wallet", "Creating new wallet");

    return true;
}

bool Wallet::CreateFromMnemonic(const std::vector<std::string>& mnemonic,
                                const std::string& passphrase) {
    if (!BIP39::ValidateMnemonic(mnemonic)) {
        LOG_ERROR("Wallet", "Invalid mnemonic");
        return false;
    }

    // Convert mnemonic to seed
    bytes seed;
    if (!BIP39::MnemonicToSeed(mnemonic, passphrase, seed)) {
        LOG_ERROR("Wallet", "Failed to generate seed from mnemonic");
        return false;
    }

    return ImportSeed(seed);
}

std::vector<std::string> Wallet::GenerateNewMnemonic(size_t words) {
    std::vector<std::string> mnemonic;

    if (!BIP39::GenerateRandomMnemonic(words, mnemonic)) {
        LOG_ERROR("Wallet", "Failed to generate mnemonic");
        return {};
    }

    return mnemonic;
}

bool Wallet::ImportSeed(const bytes& seed) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!hdWallet) {
        hdWallet = std::make_unique<HDWallet>();
    }

    if (!hdWallet->SetSeed(seed)) {
        LOG_ERROR("Wallet", "Failed to set seed");
        return false;
    }

    // Derive account key (m/44'/0'/0')
    if (!BIP44::DeriveAccount(hdWallet->GetMasterKey(), config.hdAccount, accountKey)) {
        LOG_ERROR("Wallet", "Failed to derive account key");
        return false;
    }

    // Generate initial keypool
    for (size_t i = 0; i < config.keyPoolSize; ++i) {
        Address addr;
        ExtendedKey key;
        DeriveNextAddress(false, addr, key);
    }

    LOG_INFO("Wallet", "HD wallet initialized with BIP44 account " +
             std::to_string(config.hdAccount));

    return true;
}

bool Wallet::ImportPrivateKey(const Hash256& privKey, const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex);

    KeyMetadata metadata;
    metadata.label = label;
    metadata.creationTime = Time::GetCurrentTime();

    if (!keystore->AddKeyFromPrivate(privKey, metadata)) {
        LOG_ERROR("Wallet", "Failed to import private key");
        return false;
    }

    // Generate address
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
    Address addr = AddressGenerator::GenerateP2PKH(pubKey);

    AddressMetadata addrMetadata;
    addrMetadata.label = label;
    addrMetadata.creationTime = metadata.creationTime;
    addrMetadata.isMine = true;

    addressBook.AddAddress(addr, addrMetadata);

    LOG_INFO("Wallet", "Imported private key: " + addr.ToString());

    return true;
}

Address Wallet::GetNewAddress(const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex);

    Address addr;
    ExtendedKey key;

    if (!DeriveNextAddress(false, addr, key)) {
        LOG_ERROR("Wallet", "Failed to generate new address");
        return Address();
    }

    // Set label if provided
    if (!label.empty()) {
        addressBook.SetLabel(addr, label);
    }

    LOG_INFO("Wallet", "Generated new address: " + addr.ToString());

    return addr;
}

Address Wallet::GetNewChangeAddress() {
    std::lock_guard<std::mutex> lock(mutex);

    Address addr;
    ExtendedKey key;

    if (!DeriveNextAddress(true, addr, key)) {
        LOG_ERROR("Wallet", "Failed to generate new change address");
        return Address();
    }

    LOG_DEBUG("Wallet", "Generated change address: " + addr.ToString());

    return addr;
}

std::vector<Address> Wallet::GetAddresses() const {
    return addressBook.GetAllAddresses();
}

std::vector<Address> Wallet::GetReceivingAddresses() const {
    return addressBook.GetReceivingAddresses();
}

bool Wallet::IsMine(const Address& addr) const {
    return addressBook.IsMine(addr);
}

bool Wallet::EncryptWallet(const std::string& passphrase) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!keystore->EncryptWallet(passphrase)) {
        return false;
    }

    Save();

    LOG_INFO("Wallet", "Wallet encrypted");

    return true;
}

void Wallet::Lock() {
    keystore->Lock();
    LOG_INFO("Wallet", "Wallet locked");
}

bool Wallet::Unlock(const std::string& passphrase) {
    if (!keystore->Unlock(passphrase)) {
        LOG_ERROR("Wallet", "Failed to unlock wallet");
        return false;
    }

    LOG_INFO("Wallet", "Wallet unlocked");

    return true;
}

bool Wallet::IsLocked() const {
    return keystore->IsLocked();
}

bool Wallet::IsEncrypted() const {
    return keystore->IsEncrypted();
}

bool Wallet::ChangePassphrase(const std::string& oldPassphrase, const std::string& newPassphrase) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!keystore->ChangePassphrase(oldPassphrase, newPassphrase)) {
        return false;
    }

    Save();

    LOG_INFO("Wallet", "Passphrase changed");

    return true;
}

Amount Wallet::GetBalance() const {
    std::lock_guard<std::mutex> lock(mutex);

    Amount balance = 0;

    for (const auto& pair : walletUTXOs) {
        balance += pair.second.value;
    }

    return balance;
}

Amount Wallet::GetUnconfirmedBalance() const {
    // TODO: Track unconfirmed transactions
    return 0;
}

Amount Wallet::GetAvailableBalance() const {
    std::lock_guard<std::mutex> lock(mutex);

    Amount balance = 0;

    for (const auto& pair : walletUTXOs) {
        // Check if mature (for coinbase outputs)
        const OutPoint& outpoint = pair.first;
        auto heightIt = utxoHeights.find(outpoint);

        if (heightIt != utxoHeights.end()) {
            // TODO: Check coinbase maturity
            balance += pair.second.value;
        } else {
            balance += pair.second.value;
        }
    }

    return balance;
}

bool Wallet::CreateTransaction(const std::map<Address, Amount>& recipients,
                               Amount feeRate,
                               Transaction& tx) {
    std::lock_guard<std::mutex> lock(mutex);

    if (IsLocked()) {
        LOG_ERROR("Wallet", "Wallet is locked");
        return false;
    }

    // Calculate total output
    Amount totalOutput = 0;
    for (const auto& pair : recipients) {
        totalOutput += pair.second;
    }

    // Select coins
    std::vector<std::pair<OutPoint, TxOut>> selectedCoins;
    Amount selectedValue;

    if (!SelectCoins(totalOutput, feeRate, selectedCoins, selectedValue)) {
        LOG_ERROR("Wallet", "Insufficient funds");
        return false;
    }

    // Build transaction
    TransactionBuilder builder;
    builder.SetFeeRate(feeRate);

    // Add inputs
    for (const auto& coin : selectedCoins) {
        builder.AddInput(coin.first, coin.second);
    }

    // Add outputs
    for (const auto& recipient : recipients) {
        builder.AddOutput(recipient.first, recipient.second);
    }

    // Set change address
    Address changeAddr = GetNewChangeAddress();
    builder.SetChangeAddress(changeAddr);

    // Build
    Amount fee;
    if (!builder.Build(tx, fee)) {
        LOG_ERROR("Wallet", "Failed to build transaction");
        return false;
    }

    LOG_INFO("Wallet", "Created transaction with fee: " + std::to_string(fee));

    return true;
}

bool Wallet::SendTransaction(const std::map<Address, Amount>& recipients,
                             Amount feeRate,
                             Transaction& tx) {
    if (!CreateTransaction(recipients, feeRate, tx)) {
        return false;
    }

    return SignTransaction(tx);
}

bool Wallet::SignTransaction(Transaction& tx) {
    std::lock_guard<std::mutex> lock(mutex);

    if (IsLocked()) {
        LOG_ERROR("Wallet", "Wallet is locked");
        return false;
    }

    TransactionBuilder builder;

    // Build scriptSigs for inputs
    for (size_t i = 0; i < tx.vin.size(); ++i) {
        const TxIn& txin = tx.vin[i];

        // Find previous output
        auto it = walletUTXOs.find(txin.prevOut);
        if (it == walletUTXOs.end()) {
            LOG_ERROR("Wallet", "Previous output not found");
            return false;
        }

        builder.AddInput(txin.prevOut, it->second);
    }

    // Copy outputs
    for (const auto& txout : tx.vout) {
        Address addr;
        if (AddressGenerator::ExtractAddress(txout.scriptPubKey, addr)) {
            builder.AddOutput(addr, txout.value);
        }
    }

    if (!builder.Sign(tx, *keystore)) {
        LOG_ERROR("Wallet", "Failed to sign transaction");
        return false;
    }

    LOG_INFO("Wallet", "Signed transaction: " + tx.GetHash().ToHex());

    return true;
}

std::vector<std::pair<OutPoint, TxOut>> Wallet::GetUTXOs() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::pair<OutPoint, TxOut>> utxos;
    utxos.reserve(walletUTXOs.size());

    for (const auto& pair : walletUTXOs) {
        utxos.push_back(pair);
    }

    return utxos;
}

bool Wallet::AddUTXO(const OutPoint& outpoint, const TxOut& txout, BlockHeight height) {
    std::lock_guard<std::mutex> lock(mutex);

    walletUTXOs[outpoint] = txout;
    utxoHeights[outpoint] = height;

    LOG_DEBUG("Wallet", "Added UTXO: " + std::to_string(txout.value) + " satoshis");

    return true;
}

bool Wallet::RemoveUTXO(const OutPoint& outpoint) {
    std::lock_guard<std::mutex> lock(mutex);

    walletUTXOs.erase(outpoint);
    utxoHeights.erase(outpoint);

    return true;
}

bool Wallet::ProcessTransaction(const Transaction& tx, BlockHeight height) {
    std::lock_guard<std::mutex> lock(mutex);

    // Check outputs for payments to our addresses
    for (size_t i = 0; i < tx.vout.size(); ++i) {
        const TxOut& txout = tx.vout[i];

        Address addr;
        if (AddressGenerator::ExtractAddress(txout.scriptPubKey, addr) && IsMine(addr)) {
            OutPoint outpoint;
            outpoint.hash = tx.GetHash();
            outpoint.n = static_cast<uint32_t>(i);

            AddUTXO(outpoint, txout, height);

            LOG_INFO("Wallet", "Received " + std::to_string(txout.value) +
                     " satoshis to " + addr.ToString());
        }
    }

    // Remove spent outputs
    for (const TxIn& txin : tx.vin) {
        if (walletUTXOs.count(txin.prevOut) > 0) {
            RemoveUTXO(txin.prevOut);

            LOG_INFO("Wallet", "Spent UTXO");
        }
    }

    // Add to transaction history
    transactions.push_back(tx);

    return true;
}

std::vector<Transaction> Wallet::GetTransactions() const {
    std::lock_guard<std::mutex> lock(mutex);
    return transactions;
}

bool Wallet::Save() {
    std::lock_guard<std::mutex> lock(mutex);

    std::string filepath = GetWalletFilePath();
    return SaveToFile(filepath);
}

bool Wallet::Load() {
    std::lock_guard<std::mutex> lock(mutex);

    std::string filepath = GetWalletFilePath();
    return LoadFromFile(filepath);
}

Wallet::WalletInfo Wallet::GetInfo() const {
    std::lock_guard<std::mutex> lock(mutex);

    WalletInfo info;
    info.keyCount = keystore->GetKeyCount();
    info.addressCount = addressBook.GetAddressCount();
    info.utxoCount = walletUTXOs.size();
    info.balance = GetBalance();
    info.encrypted = keystore->IsEncrypted();
    info.locked = keystore->IsLocked();
    info.hdEnabled = (hdWallet != nullptr);

    return info;
}

// Private helper methods

bool Wallet::DeriveNextAddress(bool isChange, Address& addr, ExtendedKey& key) {
    if (!hdWallet) {
        LOG_ERROR("Wallet", "HD wallet not initialized");
        return false;
    }

    uint32_t& index = isChange ? nextChangeIndex : nextReceivingIndex;

    // Derive address key
    if (!BIP44::DeriveAddress(accountKey, isChange ? 1 : 0, index, key)) {
        LOG_ERROR("Wallet", "Failed to derive address");
        return false;
    }

    // Get private and public keys
    Hash256 privKey = HDWallet::GetPrivateKey(key);
    bytes pubKey = HDWallet::GetPublicKey(key);

    // Generate address
    addr = AddressGenerator::GenerateP2PKH(pubKey);

    // Add key to keystore
    KeyMetadata keyMetadata;
    keyMetadata.creationTime = Time::GetCurrentTime();
    keyMetadata.hdPath = BIP44::GetPath(config.hdAccount, isChange ? 1 : 0, index);
    keyMetadata.isChange = isChange;

    Key walletKey(privKey, pubKey, keyMetadata);
    keystore->AddKey(walletKey);

    // Add address to address book
    AddressMetadata addrMetadata;
    addrMetadata.creationTime = keyMetadata.creationTime;
    addrMetadata.isChange = isChange;
    addrMetadata.isMine = true;
    addrMetadata.derivationIndex = index;

    addressBook.AddAddress(addr, addrMetadata);

    index++;

    return true;
}

bool Wallet::SelectCoins(Amount targetValue, Amount feeRate,
                         std::vector<std::pair<OutPoint, TxOut>>& selected,
                         Amount& selectedValue) {
    selected.clear();
    selectedValue = 0;

    // Simple coin selection: largest first
    std::vector<std::pair<OutPoint, TxOut>> candidates;

    for (const auto& pair : walletUTXOs) {
        candidates.push_back(pair);
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.second.value > b.second.value;
              });

    // Estimate fee per input
    Amount feePerInput = 148 * feeRate;

    for (const auto& coin : candidates) {
        selected.push_back(coin);
        selectedValue += coin.second.value;

        // Estimate current fee
        size_t numInputs = selected.size();
        Amount estimatedFee = (numInputs * 148 + 34 + 10) * feeRate;

        if (selectedValue >= targetValue + estimatedFee + feePerInput) {
            return true;
        }
    }

    return false;
}

std::string Wallet::GetWalletFilePath() const {
    return config.dataDir + "/wallet.dat";
}

bool Wallet::SaveToFile(const std::string& filepath) {
    // TODO: Implement wallet serialization
    LOG_INFO("Wallet", "Saving wallet to " + filepath);
    return true;
}

bool Wallet::LoadFromFile(const std::string& filepath) {
    // TODO: Implement wallet deserialization
    LOG_DEBUG("Wallet", "Loading wallet from " + filepath);
    return false;
}

} // namespace dinari
