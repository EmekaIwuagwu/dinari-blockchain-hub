#include "address.h"
#include "crypto/ecdsa.h"
#include "dinari/constants.h"
#include "core/script.h"
#include "util/logger.h"
#include <algorithm>

namespace dinari {

// Address implementation

Address::Address()
    : hash({0})
    , type(AddressType::P2PKH) {
}

Address::Address(const std::string& addr) {
    Parse(addr);
}

Address::Address(const Hash160& h, AddressType t)
    : hash(h)
    , type(t) {
}

Address::Address(const bytes& pubKey) {
    hash = crypto::Hash::Hash160(pubKey);
    type = AddressType::P2PKH;
}

std::string Address::ToString() const {
    byte version = PUBKEY_ADDRESS_VERSION;

    if (type == AddressType::P2SH) {
        version = SCRIPT_ADDRESS_VERSION;
    }

    return crypto::Base58::EncodeAddress(hash, version);
}

bool Address::IsValid() const {
    // Check if hash is not all zeros
    for (byte b : hash) {
        if (b != 0) {
            return true;
        }
    }
    return false;
}

bool Address::Parse(const std::string& addr) {
    byte version;
    if (!crypto::Base58::DecodeAddress(addr, hash, version)) {
        return false;
    }

    if (version == PUBKEY_ADDRESS_VERSION) {
        type = AddressType::P2PKH;
        return true;
    } else if (version == SCRIPT_ADDRESS_VERSION) {
        type = AddressType::P2SH;
        return true;
    }

    return false;
}

Address Address::FromPubKey(const bytes& pubKey) {
    Hash160 hash = crypto::Hash::Hash160(pubKey);
    return Address(hash, AddressType::P2PKH);
}

Address Address::FromScript(const bytes& script) {
    Hash160 hash = crypto::Hash::Hash160(script);
    return Address(hash, AddressType::P2SH);
}

bool Address::Validate(const std::string& addr) {
    Address address;
    return address.Parse(addr) && address.IsValid();
}

// AddressBook implementation

AddressBook::AddressBook() {
}

bool AddressBook::AddAddress(const Address& addr, const AddressMetadata& metadata) {
    if (!addr.IsValid()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    addresses[addr] = metadata;

    LOG_DEBUG("AddressBook", "Added address: " + addr.ToString());

    return true;
}

bool AddressBook::AddAddress(const Address& addr, const std::string& label) {
    AddressMetadata metadata;
    metadata.label = label;
    metadata.creationTime = std::time(nullptr);
    metadata.type = addr.GetType();

    return AddAddress(addr, metadata);
}

bool AddressBook::RemoveAddress(const Address& addr) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return false;
    }

    addresses.erase(it);

    LOG_DEBUG("AddressBook", "Removed address: " + addr.ToString());

    return true;
}

bool AddressBook::HaveAddress(const Address& addr) const {
    std::lock_guard<std::mutex> lock(mutex);
    return addresses.count(addr) > 0;
}

bool AddressBook::GetMetadata(const Address& addr, AddressMetadata& metadata) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return false;
    }

    metadata = it->second;
    return true;
}

bool AddressBook::SetLabel(const Address& addr, const std::string& label) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return false;
    }

    it->second.label = label;
    return true;
}

std::string AddressBook::GetLabel(const Address& addr) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return "";
    }

    return it->second.label;
}

bool AddressBook::SetIsMine(const Address& addr, bool isMine) {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return false;
    }

    it->second.isMine = isMine;
    return true;
}

bool AddressBook::IsMine(const Address& addr) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = addresses.find(addr);
    if (it == addresses.end()) {
        return false;
    }

    return it->second.isMine;
}

std::vector<Address> AddressBook::GetAllAddresses() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Address> result;
    result.reserve(addresses.size());

    for (const auto& pair : addresses) {
        result.push_back(pair.first);
    }

    return result;
}

std::vector<Address> AddressBook::GetAddressesByType(AddressType type) const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Address> result;

    for (const auto& pair : addresses) {
        if (pair.second.type == type) {
            result.push_back(pair.first);
        }
    }

    return result;
}

std::vector<Address> AddressBook::GetMyAddresses() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Address> result;

    for (const auto& pair : addresses) {
        if (pair.second.isMine) {
            result.push_back(pair.first);
        }
    }

    return result;
}

std::vector<Address> AddressBook::GetChangeAddresses() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Address> result;

    for (const auto& pair : addresses) {
        if (pair.second.isMine && pair.second.isChange) {
            result.push_back(pair.first);
        }
    }

    return result;
}

std::vector<Address> AddressBook::GetReceivingAddresses() const {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<Address> result;

    for (const auto& pair : addresses) {
        if (pair.second.isMine && !pair.second.isChange) {
            result.push_back(pair.first);
        }
    }

    return result;
}

size_t AddressBook::GetAddressCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return addresses.size();
}

void AddressBook::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    addresses.clear();
    LOG_INFO("AddressBook", "Cleared all addresses");
}

// AddressGenerator implementation

Address AddressGenerator::GenerateP2PKH(const bytes& pubKey) {
    return Address::FromPubKey(pubKey);
}

Address AddressGenerator::GenerateP2SH(const bytes& script) {
    return Address::FromScript(script);
}

Address AddressGenerator::GenerateFromPrivateKey(const Hash256& privKey) {
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
    return GenerateP2PKH(pubKey);
}

bytes AddressGenerator::GenerateScriptPubKey(const Address& addr) {
    bytes script;

    if (addr.GetType() == AddressType::P2PKH) {
        // P2PKH script: OP_DUP OP_HASH160 <hash> OP_EQUALVERIFY OP_CHECKSIG
        script.push_back(static_cast<byte>(OpCode::OP_DUP));
        script.push_back(static_cast<byte>(OpCode::OP_HASH160));
        script.push_back(20);  // Hash length

        const Hash160& hash = addr.GetHash();
        script.insert(script.end(), hash.begin(), hash.end());

        script.push_back(static_cast<byte>(OpCode::OP_EQUALVERIFY));
        script.push_back(static_cast<byte>(OpCode::OP_CHECKSIG));
    } else if (addr.GetType() == AddressType::P2SH) {
        // P2SH script: OP_HASH160 <hash> OP_EQUAL
        script.push_back(static_cast<byte>(OpCode::OP_HASH160));
        script.push_back(20);  // Hash length

        const Hash160& hash = addr.GetHash();
        script.insert(script.end(), hash.begin(), hash.end());

        script.push_back(static_cast<byte>(OpCode::OP_EQUAL));
    }

    return script;
}

bool AddressGenerator::ExtractAddress(const bytes& scriptPubKey, Address& addr) {
    if (scriptPubKey.empty()) {
        return false;
    }

    // Check for P2PKH pattern
    if (scriptPubKey.size() == 25 &&
        scriptPubKey[0] == static_cast<byte>(OpCode::OP_DUP) &&
        scriptPubKey[1] == static_cast<byte>(OpCode::OP_HASH160) &&
        scriptPubKey[2] == 20 &&
        scriptPubKey[23] == static_cast<byte>(OpCode::OP_EQUALVERIFY) &&
        scriptPubKey[24] == static_cast<byte>(OpCode::OP_CHECKSIG)) {

        Hash160 hash;
        std::copy(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23, hash.begin());

        addr = Address(hash, AddressType::P2PKH);
        return true;
    }

    // Check for P2SH pattern
    if (scriptPubKey.size() == 23 &&
        scriptPubKey[0] == static_cast<byte>(OpCode::OP_HASH160) &&
        scriptPubKey[1] == 20 &&
        scriptPubKey[22] == static_cast<byte>(OpCode::OP_EQUAL)) {

        Hash160 hash;
        std::copy(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22, hash.begin());

        addr = Address(hash, AddressType::P2SH);
        return true;
    }

    // Check for P2PK pattern (rare, legacy)
    if (scriptPubKey.size() == 35 || scriptPubKey.size() == 67) {
        size_t pubKeyLen = scriptPubKey[0];
        if (pubKeyLen == scriptPubKey.size() - 2 &&
            scriptPubKey[scriptPubKey.size() - 1] == static_cast<byte>(OpCode::OP_CHECKSIG)) {

            bytes pubKey(scriptPubKey.begin() + 1, scriptPubKey.end() - 1);
            Hash160 hash = crypto::Hash::Hash160(pubKey);

            addr = Address(hash, AddressType::P2PK);
            return true;
        }
    }

    return false;
}

} // namespace dinari
