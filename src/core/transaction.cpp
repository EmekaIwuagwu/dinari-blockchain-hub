#include "transaction.h"
#include "utxo.h"
#include "crypto/hash.h"
#include "crypto/base58.h"
#include "dinari/constants.h"
#include "util/logger.h"
#include <sstream>
#include <algorithm>

namespace dinari {

// TxOut implementation

void TxOut::SerializeImpl(Serializer& s) const {
    s.WriteUInt64(value);
    s.WriteCompactSize(scriptPubKey.size());
    s.WriteBytes(scriptPubKey);
}

void TxOut::DeserializeImpl(Deserializer& d) {
    value = d.ReadUInt64();
    uint64_t scriptSize = d.ReadCompactSize();
    scriptPubKey = d.ReadBytes(scriptSize);
}

bool TxOut::IsValid() const {
    // Check value is in valid range
    if (!MoneyRange(value)) {
        return false;
    }

    // Check script size is reasonable
    if (scriptPubKey.size() > 10000) {
        return false;
    }

    return true;
}

bool TxOut::IsDust() const {
    // Dust threshold: outputs smaller than cost to spend them
    // Using 3x the minimum relay fee as threshold
    return value < (MIN_RELAY_TX_FEE * 3);
}

size_t TxOut::GetSize() const {
    Serializer s;
    SerializeImpl(s);
    return s.Size();
}

bool TxOut::operator==(const TxOut& other) const {
    return value == other.value && scriptPubKey == other.scriptPubKey;
}

// OutPoint implementation

void OutPoint::SerializeImpl(Serializer& s) const {
    s.WriteHash256(txHash);
    s.WriteUInt32(index);
}

void OutPoint::DeserializeImpl(Deserializer& d) {
    txHash = d.ReadHash256();
    index = d.ReadUInt32();
}

bool OutPoint::IsNull() const {
    // Null outpoint has all-zero hash and max index
    return std::all_of(txHash.begin(), txHash.end(), [](byte b) { return b == 0; }) &&
           index == 0xFFFFFFFF;
}

bool OutPoint::operator==(const OutPoint& other) const {
    return txHash == other.txHash && index == other.index;
}

bool OutPoint::operator<(const OutPoint& other) const {
    if (txHash != other.txHash) {
        return txHash < other.txHash;
    }
    return index < other.index;
}

std::string OutPoint::ToString() const {
    return crypto::Hash::ToHex(txHash) + ":" + std::to_string(index);
}

// TxIn implementation

void TxIn::SerializeImpl(Serializer& s) const {
    prevOut.SerializeImpl(s);
    s.WriteCompactSize(scriptSig.size());
    s.WriteBytes(scriptSig);
    s.WriteUInt32(sequence);
}

void TxIn::DeserializeImpl(Deserializer& d) {
    prevOut.DeserializeImpl(d);
    uint64_t scriptSize = d.ReadCompactSize();
    scriptSig = d.ReadBytes(scriptSize);
    sequence = d.ReadUInt32();
}

size_t TxIn::GetSize() const {
    Serializer s;
    SerializeImpl(s);
    return s.Size();
}

bool TxIn::operator==(const TxIn& other) const {
    return prevOut == other.prevOut &&
           scriptSig == other.scriptSig &&
           sequence == other.sequence;
}

// TxWitness implementation

void TxWitness::SerializeImpl(Serializer& s) const {
    s.WriteCompactSize(stack.size());
    for (const auto& item : stack) {
        s.WriteCompactSize(item.size());
        s.WriteBytes(item);
    }
}

void TxWitness::DeserializeImpl(Deserializer& d) {
    uint64_t count = d.ReadCompactSize();
    stack.resize(count);
    for (auto& item : stack) {
        uint64_t size = d.ReadCompactSize();
        item = d.ReadBytes(size);
    }
}

// Transaction implementation

void Transaction::SerializeImpl(Serializer& s) const {
    s.WriteUInt32(version);
    s.WriteCompactSize(inputs.size());
    for (const auto& input : inputs) {
        input.SerializeImpl(s);
    }
    s.WriteCompactSize(outputs.size());
    for (const auto& output : outputs) {
        output.SerializeImpl(s);
    }
    s.WriteUInt32(lockTime);
}

void Transaction::DeserializeImpl(Deserializer& d) {
    version = d.ReadUInt32();

    uint64_t inputCount = d.ReadCompactSize();
    inputs.resize(inputCount);
    for (auto& input : inputs) {
        input.DeserializeImpl(d);
    }

    uint64_t outputCount = d.ReadCompactSize();
    outputs.resize(outputCount);
    for (auto& output : outputs) {
        output.DeserializeImpl(d);
    }

    lockTime = d.ReadUInt32();

    // Clear cached hash
    hashCached = false;
}

size_t Transaction::GetSize() const {
    Serializer s;
    SerializeImpl(s);
    return s.Size();
}

Hash256 Transaction::GetHash() const {
    if (hashCached) {
        return cachedHash;
    }

    Serializer s;
    SerializeImpl(s);
    cachedHash = crypto::Hash::DoubleSHA256(s.GetData());
    hashCached = true;

    return cachedHash;
}

Hash256 Transaction::GetSignatureHash(size_t inputIndex, const bytes& scriptCode,
                                     uint32_t hashType) const {
    // Create a copy of the transaction for signature hashing
    Serializer s;

    s.WriteUInt32(version);
    s.WriteCompactSize(inputs.size());

    for (size_t i = 0; i < inputs.size(); ++i) {
        // Serialize previous output
        inputs[i].prevOut.SerializeImpl(s);

        // Only include scriptCode for the input being signed
        if (i == inputIndex) {
            s.WriteCompactSize(scriptCode.size());
            s.WriteBytes(scriptCode);
        } else {
            s.WriteCompactSize(0);  // Empty script
        }

        s.WriteUInt32(inputs[i].sequence);
    }

    s.WriteCompactSize(outputs.size());
    for (const auto& output : outputs) {
        output.SerializeImpl(s);
    }

    s.WriteUInt32(lockTime);
    s.WriteUInt32(hashType);  // Append hash type

    return crypto::Hash::DoubleSHA256(s.GetData());
}

bool Transaction::IsValid() const {
    // Check version
    if (version == 0 || version > 2) {
        return false;
    }

    // Must have inputs and outputs (except coinbase)
    if (inputs.empty() || outputs.empty()) {
        return false;
    }

    // Check that outputs are valid
    Amount totalOut = 0;
    for (const auto& output : outputs) {
        if (!output.IsValid()) {
            return false;
        }

        totalOut += output.value;
        if (!MoneyRange(totalOut)) {
            return false;
        }
    }

    // Coinbase transaction checks
    if (IsCoinbase()) {
        // Coinbase must have exactly one input
        if (inputs.size() != 1) {
            return false;
        }

        // Coinbase script must be between 2 and 100 bytes
        if (inputs[0].scriptSig.size() < 2 || inputs[0].scriptSig.size() > 100) {
            return false;
        }
    } else {
        // Non-coinbase transactions must not have null prevouts
        for (const auto& input : inputs) {
            if (input.prevOut.IsNull()) {
                return false;
            }
        }
    }

    // Check size
    if (GetSize() > MAX_BLOCK_SIZE) {
        return false;
    }

    return true;
}

bool Transaction::IsCoinbase() const {
    return inputs.size() == 1 && inputs[0].IsCoinbase();
}

bool Transaction::IsFinal(BlockHeight height, Timestamp time) const {
    // If lock time is 0, transaction is final
    if (lockTime == 0) {
        return true;
    }

    // Lock time is either a block height or timestamp
    uint64_t lockTimeThreshold = 500000000;  // Threshold between height and time

    // Check lock time against current height or time
    if (lockTime < lockTimeThreshold) {
        // Lock time is a block height
        if (lockTime <= height) {
            return true;
        }
    } else {
        // Lock time is a timestamp
        if (lockTime <= time) {
            return true;
        }
    }

    // Check if any input has non-final sequence
    for (const auto& input : inputs) {
        if (input.sequence != 0xFFFFFFFF) {
            return false;
        }
    }

    return true;
}

Amount Transaction::GetOutputValue() const {
    Amount total = 0;
    for (const auto& output : outputs) {
        total += output.value;
    }
    return total;
}

Amount Transaction::GetInputValue(const UTXOSet& utxos) const {
    if (IsCoinbase()) {
        return 0;
    }

    Amount total = 0;
    for (const auto& input : inputs) {
        const TxOut* utxo = utxos.GetUTXO(input.prevOut);
        if (utxo) {
            total += utxo->value;
        }
    }
    return total;
}

Amount Transaction::GetFee(const UTXOSet& utxos) const {
    if (IsCoinbase()) {
        return 0;
    }

    Amount inputs = GetInputValue(utxos);
    Amount outputs = GetOutputValue();

    if (inputs < outputs) {
        return 0;  // Invalid transaction
    }

    return inputs - outputs;
}

bool Transaction::IsStandard() const {
    // Check version
    if (version > 2) {
        return false;
    }

    // Check size
    if (GetSize() > MAX_BLOCK_SIZE / 5) {
        return false;
    }

    // Check each output
    for (const auto& output : outputs) {
        // Check for dust outputs
        if (output.IsDust()) {
            return false;
        }

        // Check script size
        if (output.scriptPubKey.size() > 1650) {
            return false;
        }
    }

    // Check each input
    for (const auto& input : inputs) {
        // Check scriptSig size
        if (input.scriptSig.size() > 1650) {
            return false;
        }
    }

    return true;
}

double Transaction::GetPriority(const UTXOSet& utxos, BlockHeight currentHeight) const {
    if (IsCoinbase()) {
        return 0.0;
    }

    // Priority = sum(input_value * input_age) / tx_size
    // Higher priority transactions get included in blocks first

    double sumValueAge = 0.0;

    for (const auto& input : inputs) {
        const TxOut* utxo = utxos.GetUTXO(input.prevOut);
        if (utxo) {
            BlockHeight utxoHeight = utxos.GetUTXOHeight(input.prevOut);
            if (utxoHeight <= currentHeight) {
                uint32_t age = currentHeight - utxoHeight;
                sumValueAge += static_cast<double>(utxo->value) * age;
            }
        }
    }

    return sumValueAge / GetSize();
}

size_t Transaction::GetVirtualSize() const {
    // For basic transactions without SegWit, virtual size = actual size
    // With SegWit, this would be calculated differently
    return GetSize();
}

std::string Transaction::ToString() const {
    std::ostringstream oss;
    oss << "Transaction(" << crypto::Hash::ToHex(GetHash()) << ")\n";
    oss << "  Version: " << version << "\n";
    oss << "  Inputs: " << inputs.size() << "\n";
    for (size_t i = 0; i < inputs.size(); ++i) {
        oss << "    [" << i << "] " << inputs[i].prevOut.ToString() << "\n";
    }
    oss << "  Outputs: " << outputs.size() << "\n";
    for (size_t i = 0; i < outputs.size(); ++i) {
        oss << "    [" << i << "] " << FormatAmount(outputs[i].value) << "\n";
    }
    oss << "  LockTime: " << lockTime << "\n";
    return oss.str();
}

bool Transaction::operator==(const Transaction& other) const {
    return GetHash() == other.GetHash();
}

// Helper functions

Amount GetBlockReward(BlockHeight height) {
    // Initial block reward: Calculate from 700T total supply
    // We'll use a simple model: distribute over 50 years (approx 2,628,000 blocks)
    // With halving every 210,000 blocks

    // Initial reward per block
    const Amount INITIAL_REWARD = 50 * COIN;  // 50 DNT per block

    // Number of halvings
    uint32_t halvings = height / SUBSIDY_HALVING_INTERVAL;

    // If too many halvings, reward is 0
    if (halvings >= 64) {
        return 0;
    }

    // Calculate reward with halving
    Amount reward = INITIAL_REWARD >> halvings;

    return reward;
}

Transaction CreateCoinbaseTransaction(BlockHeight height,
                                     const std::string& minerAddress,
                                     uint32_t extraNonce,
                                     Amount blockReward) {
    Transaction tx;
    tx.version = 1;

    // Create coinbase input
    TxIn coinbaseInput;
    coinbaseInput.prevOut = OutPoint();  // Null outpoint

    // Coinbase script: height + extra nonce + arbitrary data
    Serializer s;
    s.WriteCompactSize(height);
    s.WriteUInt32(extraNonce);
    s.WriteString("Dinari Blockchain");
    coinbaseInput.scriptSig = s.MoveData();

    tx.inputs.push_back(coinbaseInput);

    // Create output to miner
    TxOut coinbaseOutput;
    coinbaseOutput.value = blockReward;

    // Create simple P2PKH script for miner address
    // Note: Address parsing and script generation requires Address class integration
    // For now, create a placeholder script
    Hash160 addressHash{};  // Parse from minerAddress
    Serializer scriptS;
    scriptS.WriteUInt8(0x76);  // OP_DUP
    scriptS.WriteUInt8(0xa9);  // OP_HASH160
    scriptS.WriteUInt8(20);    // Push 20 bytes
    scriptS.WriteHash160(addressHash);
    scriptS.WriteUInt8(0x88);  // OP_EQUALVERIFY
    scriptS.WriteUInt8(0xac);  // OP_CHECKSIG
    coinbaseOutput.scriptPubKey = scriptS.MoveData();

    tx.outputs.push_back(coinbaseOutput);

    tx.lockTime = 0;

    return tx;
}

// TransactionBuilder implementation

TransactionBuilder::TransactionBuilder() {
    tx.version = 1;
    tx.lockTime = 0;
}

TransactionBuilder& TransactionBuilder::SetVersion(uint32_t ver) {
    tx.version = ver;
    return *this;
}

TransactionBuilder& TransactionBuilder::AddInput(const OutPoint& prevOut, const bytes& scriptSig) {
    tx.inputs.emplace_back(prevOut, scriptSig);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddInput(const Hash256& txHash, TxOutIndex index,
                                                const bytes& scriptSig) {
    return AddInput(OutPoint(txHash, index), scriptSig);
}

TransactionBuilder& TransactionBuilder::AddOutput(Amount value, const bytes& scriptPubKey) {
    tx.outputs.emplace_back(value, scriptPubKey);
    return *this;
}

TransactionBuilder& TransactionBuilder::AddOutput(Amount value, const std::string& address) {
    // Parse address and create script
    Hash160 hash;
    byte version;

    if (!crypto::Base58::DecodeAddress(address, hash, version)) {
        LOG_ERROR("TransactionBuilder", "Invalid address: " + address);
        return *this;
    }

    // Create P2PKH script
    Serializer s;
    s.WriteUInt8(0x76);  // OP_DUP
    s.WriteUInt8(0xa9);  // OP_HASH160
    s.WriteUInt8(20);    // Push 20 bytes
    s.WriteHash160(hash);
    s.WriteUInt8(0x88);  // OP_EQUALVERIFY
    s.WriteUInt8(0xac);  // OP_CHECKSIG

    return AddOutput(value, s.MoveData());
}

TransactionBuilder& TransactionBuilder::SetLockTime(uint32_t lockTime) {
    tx.lockTime = lockTime;
    return *this;
}

Transaction TransactionBuilder::Build() const {
    return tx;
}

void TransactionBuilder::Reset() {
    tx = Transaction();
    tx.version = 1;
    tx.lockTime = 0;
}

} // namespace dinari
