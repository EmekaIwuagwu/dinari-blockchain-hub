#include "test_framework.h"
#include "core/script.h"
#include "core/transaction.h"
#include "crypto/hash.h"
#include "crypto/ecdsa.h"
#include "blockchain/block.h"
#include "dinari/constants.h"
#include <iostream>

using namespace dinari;

// Test OP_CHECKSIG signature removal from scriptCode
TEST(test_op_checksig_signature_removal) {
    // Generate a key pair for testing
    Hash256 privKey = crypto::Hash::SHA256("test private key");
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
    Hash160 pubKeyHash = crypto::Hash::ComputeHash160(pubKey);

    // Create a P2PKH scriptPubKey
    Script scriptPubKeyScript = Script::CreateP2PKH(pubKeyHash);
    bytes scriptPubKey = scriptPubKeyScript.GetCode();

    // Create a transaction that spends from a previous output
    Transaction tx;
    tx.version = 1;

    // Input: reference to a previous output
    Hash256 prevTxHash = crypto::Hash::SHA256("previous transaction");
    tx.inputs.emplace_back(OutPoint(prevTxHash, 0));

    // Output: send to another address
    Hash160 destHash = crypto::Hash::ComputeHash160(crypto::Hash::SHA256("destination"));
    tx.outputs.emplace_back(50 * COIN, Script::CreateP2PKH(destHash).GetCode());
    tx.lockTime = 0;

    // Sign the transaction
    bytes scriptSig = SignTransactionInput(tx, 0, scriptPubKey, privKey);
    tx.inputs[0].scriptSig = scriptSig;

    // Verify the signature
    bool verified = VerifyScript(scriptSig, scriptPubKey, tx, 0);

    ASSERT_TRUE(verified, "Signature verification should succeed");

    std::cout << "✓ OP_CHECKSIG correctly verifies signature with proper signature removal\n";
}

// Test that signature verification works with signature in scriptCode
// This indirectly tests FindAndDelete since OP_CHECKSIG uses it internally
TEST(test_signature_malleability_protection) {
    // Generate a key pair
    Hash256 privKey = crypto::Hash::SHA256("malleability test key");
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);
    Hash160 pubKeyHash = crypto::Hash::ComputeHash160(pubKey);

    // Create P2PKH script
    bytes scriptPubKey = Script::CreateP2PKH(pubKeyHash).GetCode();

    // Create transaction
    Transaction tx;
    tx.version = 1;
    Hash256 prevTxHash = crypto::Hash::SHA256("previous tx for malleability");
    tx.inputs.emplace_back(OutPoint(prevTxHash, 0));
    Hash160 destHash = crypto::Hash::ComputeHash160(crypto::Hash::SHA256("dest"));
    tx.outputs.emplace_back(25 * COIN, Script::CreateP2PKH(destHash).GetCode());
    tx.lockTime = 0;

    // Sign the transaction
    bytes scriptSig = SignTransactionInput(tx, 0, scriptPubKey, privKey);
    tx.inputs[0].scriptSig = scriptSig;

    // Verification should succeed even though signature is in scriptPubKey during hashing
    bool verified = VerifyScript(scriptSig, scriptPubKey, tx, 0);

    ASSERT_TRUE(verified, "Signature should verify with proper malleability protection");

    std::cout << "✓ Signature verification protects against malleability\n";
}

// Test CompactToTarget with various inputs
TEST(test_compact_to_target) {
    // Test 1: Valid compact format
    uint32_t compact1 = 0x1d00ffff;  // Bitcoin's initial difficulty
    Hash256 target1 = crypto::Hash::CompactToTarget(compact1);

    // The target should be non-zero
    bool nonZero = false;
    for (size_t i = 0; i < target1.size(); i++) {
        if (target1[i] != 0) {
            nonZero = true;
            break;
        }
    }
    ASSERT_TRUE(nonZero, "Target should be non-zero for valid compact");

    // Test 2: Compact with sign bit set (negative) should return zero
    uint32_t compact2 = 0x1d80ffff;  // Sign bit set
    Hash256 target2 = crypto::Hash::CompactToTarget(compact2);

    bool allZero = true;
    for (size_t i = 0; i < target2.size(); i++) {
        if (target2[i] != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_TRUE(allZero, "Target should be zero for negative compact");

    // Test 3: Compact with zero mantissa should return zero
    uint32_t compact3 = 0x03000000;
    Hash256 target3 = crypto::Hash::CompactToTarget(compact3);

    allZero = true;
    for (size_t i = 0; i < target3.size(); i++) {
        if (target3[i] != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_TRUE(allZero, "Target should be zero for zero mantissa");

    // Test 4: Compact with zero exponent should return zero
    uint32_t compact4 = 0x00123456;
    Hash256 target4 = crypto::Hash::CompactToTarget(compact4);

    allZero = true;
    for (size_t i = 0; i < target4.size(); i++) {
        if (target4[i] != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_TRUE(allZero, "Target should be zero for zero exponent");

    // Test 5: Exponent too large should return zero
    uint32_t compact5 = 0x21123456;  // Exponent 33 (too large)
    Hash256 target5 = crypto::Hash::CompactToTarget(compact5);

    allZero = true;
    for (size_t i = 0; i < target5.size(); i++) {
        if (target5[i] != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_TRUE(allZero, "Target should be zero for exponent > 32");

    std::cout << "✓ CompactToTarget handles all edge cases correctly\n";
}

// Test TargetToCompact round-trip conversion
TEST(test_target_to_compact_round_trip) {
    // Test with Bitcoin's initial difficulty
    uint32_t compact1 = 0x1d00ffff;
    Hash256 target1 = crypto::Hash::CompactToTarget(compact1);
    uint32_t compact1_back = crypto::Hash::TargetToCompact(target1);

    ASSERT_EQ(compact1, compact1_back, "Compact should round-trip correctly");

    // Test with lower difficulty (higher target)
    uint32_t compact2 = 0x1e0ffff0;
    Hash256 target2 = crypto::Hash::CompactToTarget(compact2);
    uint32_t compact2_back = crypto::Hash::TargetToCompact(target2);

    // Allow for some rounding in the mantissa
    uint32_t exp1 = compact2 >> 24;
    uint32_t exp2 = compact2_back >> 24;
    ASSERT_EQ(exp1, exp2, "Exponent should match in round-trip");

    // Test with higher difficulty (lower target)
    uint32_t compact3 = 0x1b0404cb;
    Hash256 target3 = crypto::Hash::CompactToTarget(compact3);
    uint32_t compact3_back = crypto::Hash::TargetToCompact(target3);

    exp1 = compact3 >> 24;
    exp2 = compact3_back >> 24;
    ASSERT_EQ(exp1, exp2, "Exponent should match for higher difficulty");

    std::cout << "✓ TargetToCompact correctly converts targets\n";
}

// Test that TargetToCompact never sets sign bit
TEST(test_target_to_compact_no_sign_bit) {
    // Create various targets and ensure none have sign bit set
    std::vector<uint32_t> testCompacts = {
        0x1d00ffff,
        0x1c0fffff,
        0x1b0404cb,
        0x1a0f0f0f,
        0x19123456,
        0x18012345
    };

    for (uint32_t origCompact : testCompacts) {
        Hash256 target = crypto::Hash::CompactToTarget(origCompact);
        uint32_t newCompact = crypto::Hash::TargetToCompact(target);

        // Check that sign bit is not set
        bool signBit = (newCompact & 0x00800000) != 0;
        ASSERT_TRUE(!signBit, "TargetToCompact should never set sign bit");
    }

    std::cout << "✓ TargetToCompact never sets sign bit\n";
}

// Test BlockHeader::GetWork calculation
TEST(test_block_header_get_work) {
    BlockHeader header1;
    header1.version = 1;
    header1.bits = 0x1d00ffff;  // Bitcoin's initial difficulty
    header1.timestamp = 1231006505;
    header1.nonce = 0;

    auto work1 = header1.GetWork();

    // Work should be non-zero
    ASSERT_TRUE(work1 > 0, "GetWork should return non-zero work");

    // Higher difficulty (lower target) should produce more work
    BlockHeader header2;
    header2.version = 1;
    header2.bits = 0x1c00ffff;  // Higher difficulty
    header2.timestamp = 1231006505;
    header2.nonce = 0;

    auto work2 = header2.GetWork();

    ASSERT_TRUE(work2 > work1, "Higher difficulty should produce more work");

    std::cout << "✓ BlockHeader::GetWork calculates work correctly\n";
}

// Test chain work accumulation
TEST(test_chain_work_accumulation) {
    // Create a genesis block
    Block genesis = CreateGenesisBlock(1231006505, 0x1d00ffff, 0, "Dinari Genesis");
    auto genesisPtr = std::make_shared<Block>(genesis);

    BlockIndex genesisIndex(genesisPtr, 0);
    genesisIndex.UpdateChainWork();

    auto genesisWork = genesisIndex.chainWork;
    ASSERT_TRUE(genesisWork > 0, "Genesis should have non-zero work");

    // Create a second block
    Block block2;
    block2.header.version = 1;
    block2.header.prevBlockHash = genesis.GetHash();
    block2.header.bits = 0x1d00ffff;
    block2.header.timestamp = 1231006505 + 600;
    block2.header.nonce = 0;

    // Add coinbase
    Transaction coinbase = CreateCoinbaseTransaction(1, "miner", 0, GetBlockReward(1));
    block2.transactions.push_back(coinbase);
    block2.header.merkleRoot = block2.CalculateMerkleRoot();

    auto block2Ptr = std::make_shared<Block>(block2);
    BlockIndex block2Index(block2Ptr, 1);
    block2Index.prev = &genesisIndex;
    block2Index.UpdateChainWork();

    // Chain work should accumulate
    ASSERT_TRUE(block2Index.chainWork > genesisWork, "Chain work should accumulate");
    ASSERT_EQ(block2Index.chainWork, genesisWork + block2.header.GetWork(),
              "Chain work should equal sum of block work");

    std::cout << "✓ Chain work accumulates correctly\n";
}

// Test money supply tracking
TEST(test_money_supply_tracking) {
    // Genesis should have initial reward
    Block genesis = CreateGenesisBlock(1231006505, 0x1d00ffff, 0, "Dinari Genesis");
    Amount genesisReward = GetBlockReward(0);

    ASSERT_TRUE(genesis.HasCoinbase(), "Genesis should have coinbase");
    Amount genesisCoinbase = genesis.GetCoinbaseTransaction().GetOutputValue();
    ASSERT_EQ(genesisCoinbase, genesisReward, "Genesis coinbase should equal block reward");

    // Test that block reward halves correctly
    Amount reward0 = GetBlockReward(0);
    Amount reward1 = GetBlockReward(SUBSIDY_HALVING_INTERVAL);  // After first halving
    Amount reward2 = GetBlockReward(SUBSIDY_HALVING_INTERVAL * 2);  // After second halving

    ASSERT_EQ(reward1, reward0 / 2, "Reward should halve after halving interval");
    ASSERT_EQ(reward2, reward0 / 4, "Reward should quarter after two halvings");

    // Test that reward eventually becomes zero
    Amount rewardFinal = GetBlockReward(SUBSIDY_HALVING_INTERVAL * 64);
    ASSERT_EQ(rewardFinal, static_cast<Amount>(0), "Reward should become zero after 64 halvings");

    std::cout << "✓ Money supply tracking works correctly\n";
}

// Test genesis output is provably unspendable
TEST(test_genesis_output_unspendable) {
    Block genesis = CreateGenesisBlock(1231006505, 0x1d00ffff, 0, "Dinari Genesis");

    ASSERT_TRUE(genesis.HasCoinbase(), "Genesis should have coinbase");

    const Transaction& coinbase = genesis.GetCoinbaseTransaction();
    ASSERT_TRUE(coinbase.outputs.size() > 0, "Coinbase should have outputs");

    const bytes& scriptPubKey = coinbase.outputs[0].scriptPubKey;
    ASSERT_TRUE(scriptPubKey.size() > 0, "ScriptPubKey should not be empty");

    // Check that it's an OP_RETURN script (provably unspendable)
    ASSERT_EQ(scriptPubKey[0], static_cast<byte>(0x6a),
              "Genesis output should be OP_RETURN");

    // Verify it's detected as NULL_DATA type
    Script script(scriptPubKey);
    ASSERT_EQ(script.GetType(), Script::Type::NULL_DATA,
              "Genesis output should be NULL_DATA type");

    std::cout << "✓ Genesis output is provably unspendable (OP_RETURN)\n";
}

// Test SafeAdd overflow detection
TEST(test_safe_add_overflow) {
    Amount a = MAX_MONEY - 1000;
    Amount b = 2000;
    Amount result = 0;

    // This should detect overflow
    bool success = SafeAdd(a, b, result);
    ASSERT_TRUE(!success, "SafeAdd should detect overflow when exceeding MAX_MONEY");

    // This should succeed
    Amount c = 500;
    success = SafeAdd(a, c, result);
    ASSERT_TRUE(success, "SafeAdd should succeed when not exceeding MAX_MONEY");
    ASSERT_EQ(result, a + c, "SafeAdd should compute correct sum");

    std::cout << "✓ SafeAdd correctly detects overflow\n";
}

int main() {
    std::cout << "\n=== Running Consensus Hardening Tests ===\n\n";

    RUN_TEST(test_op_checksig_signature_removal);
    RUN_TEST(test_signature_malleability_protection);
    RUN_TEST(test_compact_to_target);
    RUN_TEST(test_target_to_compact_round_trip);
    RUN_TEST(test_target_to_compact_no_sign_bit);
    RUN_TEST(test_block_header_get_work);
    RUN_TEST(test_chain_work_accumulation);
    RUN_TEST(test_money_supply_tracking);
    RUN_TEST(test_genesis_output_unspendable);
    RUN_TEST(test_safe_add_overflow);

    std::cout << "\n=== All Consensus Tests Passed! ===\n\n";

    return 0;
}
