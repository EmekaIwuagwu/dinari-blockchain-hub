#include "script.h"
#include "transaction.h"
#include "crypto/hash.h"
#include "crypto/ecdsa.h"
#include "crypto/base58.h"
#include "util/logger.h"
#include <sstream>
#include <algorithm>

namespace dinari {

// Script implementation

std::string Script::ToString() const {
    std::ostringstream oss;

    for (size_t i = 0; i < code.size(); ) {
        OpCode op = static_cast<OpCode>(code[i]);

        // Check if it's a data push
        if (code[i] >= 1 && code[i] <= 75) {
            size_t len = code[i];
            oss << "PUSH(" << len << " bytes) ";
            i += len + 1;
        } else {
            // Opcode
            switch (op) {
                case OpCode::OP_DUP: oss << "OP_DUP "; break;
                case OpCode::OP_HASH160: oss << "OP_HASH160 "; break;
                case OpCode::OP_EQUALVERIFY: oss << "OP_EQUALVERIFY "; break;
                case OpCode::OP_CHECKSIG: oss << "OP_CHECKSIG "; break;
                case OpCode::OP_EQUAL: oss << "OP_EQUAL "; break;
                case OpCode::OP_VERIFY: oss << "OP_VERIFY "; break;
                case OpCode::OP_RETURN: oss << "OP_RETURN "; break;
                default: oss << "OP_" << static_cast<int>(op) << " "; break;
            }
            i++;
        }
    }

    return oss.str();
}

Script::Type Script::GetType() const {
    if (code.empty()) {
        return Type::UNKNOWN;
    }

    // P2PKH: OP_DUP OP_HASH160 <20 bytes> OP_EQUALVERIFY OP_CHECKSIG
    if (code.size() == 25 &&
        code[0] == static_cast<uint8_t>(OpCode::OP_DUP) &&
        code[1] == static_cast<uint8_t>(OpCode::OP_HASH160) &&
        code[2] == 20 &&
        code[23] == static_cast<uint8_t>(OpCode::OP_EQUALVERIFY) &&
        code[24] == static_cast<uint8_t>(OpCode::OP_CHECKSIG)) {
        return Type::P2PKH;
    }

    // P2SH: OP_HASH160 <20 bytes> OP_EQUAL
    if (code.size() == 23 &&
        code[0] == static_cast<uint8_t>(OpCode::OP_HASH160) &&
        code[1] == 20 &&
        code[22] == static_cast<uint8_t>(OpCode::OP_EQUAL)) {
        return Type::P2SH;
    }

    // P2PK: <pubkey> OP_CHECKSIG
    if (code.size() == 35 || code.size() == 67) {
        if (code[code.size() - 1] == static_cast<uint8_t>(OpCode::OP_CHECKSIG)) {
            return Type::P2PK;
        }
    }

    // NULL_DATA: OP_RETURN <data>
    if (code.size() > 1 && code[0] == static_cast<uint8_t>(OpCode::OP_RETURN)) {
        return Type::NULL_DATA;
    }

    return Type::UNKNOWN;
}

bool Script::IsStandard() const {
    Type type = GetType();
    return type == Type::P2PKH ||
           type == Type::P2SH ||
           type == Type::P2PK ||
           type == Type::MULTISIG ||
           type == Type::NULL_DATA;
}

bool Script::ExtractDestination(Hash160& dest) const {
    Type type = GetType();

    if (type == Type::P2PKH) {
        // Extract hash from P2PKH script
        std::copy(code.begin() + 3, code.begin() + 23, dest.begin());
        return true;
    }

    if (type == Type::P2SH) {
        // Extract hash from P2SH script
        std::copy(code.begin() + 2, code.begin() + 22, dest.begin());
        return true;
    }

    return false;
}

Script Script::CreateP2PKH(const Hash160& pubkeyHash) {
    bytes script;
    script.push_back(static_cast<uint8_t>(OpCode::OP_DUP));
    script.push_back(static_cast<uint8_t>(OpCode::OP_HASH160));
    script.push_back(20);  // Push 20 bytes
    script.insert(script.end(), pubkeyHash.begin(), pubkeyHash.end());
    script.push_back(static_cast<uint8_t>(OpCode::OP_EQUALVERIFY));
    script.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));
    return Script(script);
}

Script Script::CreateP2SH(const Hash160& scriptHash) {
    bytes script;
    script.push_back(static_cast<uint8_t>(OpCode::OP_HASH160));
    script.push_back(20);  // Push 20 bytes
    script.insert(script.end(), scriptHash.begin(), scriptHash.end());
    script.push_back(static_cast<uint8_t>(OpCode::OP_EQUAL));
    return Script(script);
}

Script Script::CreateP2PK(const bytes& pubkey) {
    bytes script;
    script.push_back(static_cast<uint8_t>(pubkey.size()));
    script.insert(script.end(), pubkey.begin(), pubkey.end());
    script.push_back(static_cast<uint8_t>(OpCode::OP_CHECKSIG));
    return Script(script);
}

Script Script::CreateMultisig(int nRequired, const std::vector<bytes>& pubkeys) {
    bytes script;

    // OP_N (number required)
    script.push_back(static_cast<uint8_t>(OpCode::OP_1) + nRequired - 1);

    // Push public keys
    for (const auto& pubkey : pubkeys) {
        script.push_back(static_cast<uint8_t>(pubkey.size()));
        script.insert(script.end(), pubkey.begin(), pubkey.end());
    }

    // OP_M (number of keys)
    script.push_back(static_cast<uint8_t>(OpCode::OP_1) + pubkeys.size() - 1);

    // OP_CHECKMULTISIG
    script.push_back(static_cast<uint8_t>(OpCode::OP_CHECKMULTISIG));

    return Script(script);
}

Script Script::CreateNullData(const bytes& data) {
    bytes script;
    script.push_back(static_cast<uint8_t>(OpCode::OP_RETURN));

    if (data.size() < 76) {
        script.push_back(static_cast<uint8_t>(data.size()));
    } else {
        // Use OP_PUSHDATA1 for larger data
        script.push_back(static_cast<uint8_t>(OpCode::OP_PUSHDATA1));
        script.push_back(static_cast<uint8_t>(data.size()));
    }

    script.insert(script.end(), data.begin(), data.end());
    return Script(script);
}

// ScriptEngine implementation

ScriptEngine::ScriptEngine() : currentScriptCode(nullptr) {}

bool ScriptEngine::Verify(const bytes& scriptSig, const bytes& scriptPubKey,
                          const Transaction& tx, size_t inputIndex) {
    // Reset interpreter state
    std::stack<bytes> emptyStack;
    std::stack<bytes> emptyAltStack;
    stack.swap(emptyStack);
    altStack.swap(emptyAltStack);
    lastError.clear();
    currentScriptCode = nullptr;

    // Execute scriptSig first
    if (!ExecuteScript(scriptSig, tx, inputIndex)) {
        return false;
    }

    // Execute scriptPubKey with the resulting stack
    if (!ExecuteScript(scriptPubKey, tx, inputIndex, &scriptPubKey)) {
        return false;
    }

    // Stack must have at least one element and top must be true
    if (stack.empty()) {
        lastError = "Stack empty after execution";
        return false;
    }

    bytes top;
    if (!PopStack(top)) {
        return false;
    }

    if (!StackBool(top)) {
        lastError = "Script verification failed: top of stack is false";
        return false;
    }

    return true;
}

bool ScriptEngine::ExecuteScript(const bytes& script, const Transaction& tx,
                                 size_t inputIndex, const bytes* scriptCode) {
    const bytes* previousScriptCode = currentScriptCode;
    if (scriptCode) {
        currentScriptCode = scriptCode;
    }
    bool success = true;

    for (size_t pc = 0; pc < script.size(); ) {
        uint8_t op = script[pc];

        if (op >= 1 && op <= 75) {
            size_t len = op;
            if (pc + 1 + len > script.size()) {
                lastError = "Script push exceeds script length";
                success = false;
                break;
            }
            bytes data(script.begin() + pc + 1, script.begin() + pc + 1 + len);
            PushStack(data);
            pc += len + 1;
        } else {
            if (!ExecuteOpcode(static_cast<OpCode>(op), tx, inputIndex)) {
                success = false;
                break;
            }
            pc++;
        }
    }

    currentScriptCode = previousScriptCode;
    return success;
}

bool ScriptEngine::ExecuteOpcode(OpCode opcode, const Transaction& tx,
                                 size_t inputIndex) {
    switch (opcode) {
        case OpCode::OP_0:  // OP_FALSE is same value
            PushStack(bytes());
            return true;

        case OpCode::OP_1:  // OP_TRUE is same value
            PushStack(IntToBytes(1));
            return true;

        case OpCode::OP_DUP: {
            bytes value;
            if (!PeekStack(value)) return false;
            PushStack(value);
            return true;
        }

        case OpCode::OP_HASH160: {
            bytes value;
            if (!PopStack(value)) return false;
            Hash160 hash = crypto::Hash::ComputeHash160(value);
            PushStack(bytes(hash.begin(), hash.end()));
            return true;
        }

        case OpCode::OP_SHA256: {
            bytes value;
            if (!PopStack(value)) return false;
            Hash256 hash = crypto::Hash::SHA256(value);
            PushStack(bytes(hash.begin(), hash.end()));
            return true;
        }

        case OpCode::OP_EQUAL:
        case OpCode::OP_EQUALVERIFY: {
            bytes a, b;
            if (!PopStack(b) || !PopStack(a)) return false;
            bool equal = (a == b);

            if (opcode == OpCode::OP_EQUAL) {
                PushStack(IntToBytes(equal ? 1 : 0));
            } else {
                if (!equal) {
                    lastError = "OP_EQUALVERIFY failed";
                    return false;
                }
            }
            return true;
        }

        case OpCode::OP_CHECKSIG:
        case OpCode::OP_CHECKSIGVERIFY:
            return OpCheckSig(tx, inputIndex);

        case OpCode::OP_VERIFY: {
            bytes value;
            if (!PopStack(value)) return false;
            if (!StackBool(value)) {
                lastError = "OP_VERIFY failed";
                return false;
            }
            return true;
        }

        case OpCode::OP_RETURN:
            lastError = "OP_RETURN executed";
            return false;

        case OpCode::OP_DROP: {
            bytes value;
            return PopStack(value);
        }

        case OpCode::OP_NOP:
            return true;

        default:
            lastError = "Unknown opcode: " + std::to_string(static_cast<int>(opcode));
            return false;
    }
}

bool ScriptEngine::PopStack(bytes& value) {
    if (stack.empty()) {
        lastError = "Stack underflow";
        return false;
    }
    value = stack.top();
    stack.pop();
    return true;
}

bool ScriptEngine::PeekStack(bytes& value) {
    if (stack.empty()) {
        lastError = "Stack empty";
        return false;
    }
    value = stack.top();
    return true;
}

void ScriptEngine::PushStack(const bytes& value) {
    stack.push(value);
}

bool ScriptEngine::StackBool(const bytes& value) {
    // Empty or all zeros is false
    return !value.empty() && !std::all_of(value.begin(), value.end(),
                                          [](byte b) { return b == 0; });
}

int64_t ScriptEngine::BytesToInt(const bytes& b) {
    if (b.empty()) return 0;

    int64_t result = 0;
    bool negative = (b.back() & 0x80) != 0;

    for (size_t i = 0; i < b.size(); ++i) {
        result |= static_cast<int64_t>(b[i] & (i == b.size() - 1 ? 0x7f : 0xff)) << (8 * i);
    }

    return negative ? -result : result;
}

bytes ScriptEngine::IntToBytes(int64_t n) {
    if (n == 0) return bytes();

    bytes result;
    bool negative = n < 0;
    uint64_t absValue = negative ? -n : n;

    while (absValue) {
        result.push_back(absValue & 0xff);
        absValue >>= 8;
    }

    if (result.back() & 0x80) {
        result.push_back(negative ? 0x80 : 0x00);
    } else if (negative) {
        result.back() |= 0x80;
    }

    return result;
}

bool ScriptEngine::CheckStackSize(size_t required) {
    if (stack.size() < required) {
        lastError = "Stack size insufficient";
        return false;
    }
    return true;
}

bool ScriptEngine::OpCheckSig(const Transaction& tx, size_t inputIndex) {
    if (!CheckStackSize(2)) return false;

    bytes pubkey, signature;
    if (!PopStack(pubkey) || !PopStack(signature)) {
        return false;
    }

    // Verify signature
    // Extract hash type from signature (last byte)
    if (signature.empty()) {
        lastError = "Empty signature";
        return false;
    }

    uint32_t hashType = signature.back();
    signature.pop_back();  // Remove hash type byte

    // Get signature hash
    const bytes* scriptCode = currentScriptCode;
    const bytes& scriptForHash = scriptCode ? *scriptCode : EmptyScript();
    Hash256 sigHash = tx.GetSignatureHash(inputIndex, scriptForHash, hashType);

    // Verify using ECDSA
    bool valid = crypto::ECDSA::Verify(sigHash, signature, pubkey);

    PushStack(IntToBytes(valid ? 1 : 0));

    return true;
}

const bytes& ScriptEngine::EmptyScript() {
    static const bytes empty{};
    return empty;
}

bool ScriptEngine::OpAdd() {
    if (!CheckStackSize(2)) return false;

    bytes a, b;
    if (!PopStack(b) || !PopStack(a)) return false;

    int64_t result = BytesToInt(a) + BytesToInt(b);
    PushStack(IntToBytes(result));

    return true;
}

bool ScriptEngine::OpSub() {
    if (!CheckStackSize(2)) return false;

    bytes a, b;
    if (!PopStack(b) || !PopStack(a)) return false;

    int64_t result = BytesToInt(a) - BytesToInt(b);
    PushStack(IntToBytes(result));

    return true;
}

bool ScriptEngine::OpEqual() {
    if (!CheckStackSize(2)) return false;

    bytes a, b;
    if (!PopStack(b) || !PopStack(a)) return false;

    PushStack(IntToBytes(a == b ? 1 : 0));
    return true;
}

// Global functions

bool VerifyScript(const bytes& scriptSig, const bytes& scriptPubKey,
                 const Transaction& tx, size_t inputIndex) {
    ScriptEngine engine;
    return engine.Verify(scriptSig, scriptPubKey, tx, inputIndex);
}

bytes SignTransactionInput(const Transaction& tx, size_t inputIndex,
                          const bytes& scriptPubKey, const Hash256& privKey) {
    // Get signature hash
    uint32_t hashType = 1;  // SIGHASH_ALL
    Hash256 sigHash = tx.GetSignatureHash(inputIndex, scriptPubKey, hashType);

    // Sign with private key
    bytes signature = crypto::ECDSA::Sign(sigHash, privKey);

    // Append hash type
    signature.push_back(static_cast<byte>(hashType));

    // Get public key
    bytes pubkey = crypto::ECDSA::GetPublicKey(privKey, true);

    // Create scriptSig: <signature> <pubkey>
    bytes scriptSig;
    scriptSig.push_back(static_cast<byte>(signature.size()));
    scriptSig.insert(scriptSig.end(), signature.begin(), signature.end());
    scriptSig.push_back(static_cast<byte>(pubkey.size()));
    scriptSig.insert(scriptSig.end(), pubkey.begin(), pubkey.end());

    return scriptSig;
}

bytes CreateScriptPubKeyForAddress(const std::string& address) {
    Hash160 hash;
    byte version;

    if (!crypto::Base58::DecodeAddress(address, hash, version)) {
        return bytes();
    }

    return Script::CreateP2PKH(hash).GetCode();
}

bool ExtractAddressFromScript(const bytes& script, std::string& address) {
    Script s(script);
    Hash160 hash;

    if (!s.ExtractDestination(hash)) {
        return false;
    }

    // Create address with proper version
    address = crypto::Base58::EncodeAddress(hash, PUBKEY_ADDRESS_VERSION);
    return true;
}

} // namespace dinari
