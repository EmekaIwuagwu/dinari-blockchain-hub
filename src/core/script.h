#ifndef DINARI_CORE_SCRIPT_H
#define DINARI_CORE_SCRIPT_H

#include "dinari/types.h"
#include <vector>
#include <string>
#include <stack>

namespace dinari {

/**
 * @brief Script opcodes (subset of Bitcoin opcodes)
 *
 * These are the opcodes used in transaction scripts for validation.
 */
enum class OpCode : uint8_t {
    // Constants
    OP_0 = 0x00,
    OP_FALSE = 0x00,
    OP_PUSHDATA1 = 0x4c,
    OP_PUSHDATA2 = 0x4d,
    OP_PUSHDATA4 = 0x4e,
    OP_1NEGATE = 0x4f,
    OP_1 = 0x51,
    OP_TRUE = 0x51,
    OP_2 = 0x52,
    OP_3 = 0x53,
    OP_4 = 0x54,
    OP_5 = 0x55,
    OP_6 = 0x56,
    OP_7 = 0x57,
    OP_8 = 0x58,
    OP_9 = 0x59,
    OP_10 = 0x5a,
    OP_11 = 0x5b,
    OP_12 = 0x5c,
    OP_13 = 0x5d,
    OP_14 = 0x5e,
    OP_15 = 0x5f,
    OP_16 = 0x60,

    // Flow control
    OP_NOP = 0x61,
    OP_IF = 0x63,
    OP_NOTIF = 0x64,
    OP_ELSE = 0x67,
    OP_ENDIF = 0x68,
    OP_VERIFY = 0x69,
    OP_RETURN = 0x6a,

    // Stack operations
    OP_TOALTSTACK = 0x6b,
    OP_FROMALTSTACK = 0x6c,
    OP_2DROP = 0x6d,
    OP_2DUP = 0x6e,
    OP_3DUP = 0x6f,
    OP_2OVER = 0x70,
    OP_2ROT = 0x71,
    OP_2SWAP = 0x72,
    OP_IFDUP = 0x73,
    OP_DEPTH = 0x74,
    OP_DROP = 0x75,
    OP_DUP = 0x76,
    OP_NIP = 0x77,
    OP_OVER = 0x78,
    OP_PICK = 0x79,
    OP_ROLL = 0x7a,
    OP_ROT = 0x7b,
    OP_SWAP = 0x7c,
    OP_TUCK = 0x7d,

    // Splice operations
    OP_SIZE = 0x82,

    // Bitwise logic
    OP_EQUAL = 0x87,
    OP_EQUALVERIFY = 0x88,

    // Arithmetic
    OP_1ADD = 0x8b,
    OP_1SUB = 0x8c,
    OP_NEGATE = 0x8f,
    OP_ABS = 0x90,
    OP_NOT = 0x91,
    OP_0NOTEQUAL = 0x92,
    OP_ADD = 0x93,
    OP_SUB = 0x94,
    OP_BOOLAND = 0x9a,
    OP_BOOLOR = 0x9b,
    OP_NUMEQUAL = 0x9c,
    OP_NUMEQUALVERIFY = 0x9d,
    OP_NUMNOTEQUAL = 0x9e,
    OP_LESSTHAN = 0x9f,
    OP_GREATERTHAN = 0xa0,
    OP_LESSTHANOREQUAL = 0xa1,
    OP_GREATERTHANOREQUAL = 0xa2,
    OP_MIN = 0xa3,
    OP_MAX = 0xa4,
    OP_WITHIN = 0xa5,

    // Crypto
    OP_RIPEMD160 = 0xa6,
    OP_SHA1 = 0xa7,
    OP_SHA256 = 0xa8,
    OP_HASH160 = 0xa9,
    OP_HASH256 = 0xaa,
    OP_CODESEPARATOR = 0xab,
    OP_CHECKSIG = 0xac,
    OP_CHECKSIGVERIFY = 0xad,
    OP_CHECKMULTISIG = 0xae,
    OP_CHECKMULTISIGVERIFY = 0xaf,

    // Invalid
    OP_INVALIDOPCODE = 0xff,
};

/**
 * @brief Script interpreter
 *
 * Executes transaction validation scripts.
 * Implements a stack-based virtual machine.
 */
class Script {
public:
    Script() = default;
    explicit Script(const bytes& script) : code(script) {}

    // Get script bytes
    const bytes& GetCode() const { return code; }

    // Check if script is empty
    bool IsEmpty() const { return code.empty(); }

    // Get script size
    size_t Size() const { return code.size(); }

    // Parse script to human-readable format
    std::string ToString() const;

    // Standard script types
    enum class Type {
        UNKNOWN,
        P2PKH,          // Pay to Public Key Hash
        P2SH,           // Pay to Script Hash
        P2PK,           // Pay to Public Key
        MULTISIG,       // Multisig
        NULL_DATA,      // OP_RETURN data
        WITNESS_V0_KEYHASH,  // P2WPKH (future)
        WITNESS_V0_SCRIPTHASH // P2WSH (future)
    };

    // Determine script type
    Type GetType() const;

    // Check if script is standard
    bool IsStandard() const;

    // Extract destination address (for P2PKH, P2SH)
    bool ExtractDestination(Hash160& dest) const;

    // Create standard scripts
    static Script CreateP2PKH(const Hash160& pubkeyHash);
    static Script CreateP2SH(const Hash160& scriptHash);
    static Script CreateP2PK(const bytes& pubkey);
    static Script CreateMultisig(int nRequired, const std::vector<bytes>& pubkeys);
    static Script CreateNullData(const bytes& data);

private:
    bytes code;
};

/**
 * @brief Script execution engine
 */
class ScriptEngine {
public:
    ScriptEngine();

    // Execute and verify script
    bool Verify(const bytes& scriptSig, const bytes& scriptPubKey,
               const class Transaction& tx, size_t inputIndex);

    // Get last error
    std::string GetLastError() const { return lastError; }

private:
    std::stack<bytes> stack;
    std::stack<bytes> altStack;
    std::string lastError;

    // Execute script
    bool ExecuteScript(const bytes& script, const Transaction& tx, size_t inputIndex);

    // Execute individual opcode
    bool ExecuteOpcode(OpCode opcode, const Transaction& tx, size_t inputIndex);

    // Stack operations
    bool PopStack(bytes& value);
    bool PeekStack(bytes& value);
    void PushStack(const bytes& value);
    bool StackBool(const bytes& value);

    // Convert bytes to number
    int64_t BytesToInt(const bytes& b);
    bytes IntToBytes(int64_t n);

    // Check if stack size is sufficient
    bool CheckStackSize(size_t required);

    // Crypto operations
    bool OpCheckSig(const Transaction& tx, size_t inputIndex);

    // Flow control
    bool OpIf();
    bool OpNotIf();
    bool OpElse();
    bool OpEndIf();

    // Arithmetic
    bool OpAdd();
    bool OpSub();
    bool OpEqual();
};

/**
 * @brief Script verification
 *
 * Verify transaction script (scriptSig + scriptPubKey)
 */
bool VerifyScript(const bytes& scriptSig, const bytes& scriptPubKey,
                 const Transaction& tx, size_t inputIndex);

/**
 * @brief Sign transaction input
 *
 * Create scriptSig for spending a P2PKH output
 */
bytes SignTransactionInput(const Transaction& tx, size_t inputIndex,
                          const bytes& scriptPubKey, const Hash256& privKey);

/**
 * @brief Create scriptPubKey for address
 */
bytes CreateScriptPubKeyForAddress(const std::string& address);

/**
 * @brief Extract address from scriptPubKey
 */
bool ExtractAddressFromScript(const bytes& script, std::string& address);

} // namespace dinari

#endif // DINARI_CORE_SCRIPT_H
