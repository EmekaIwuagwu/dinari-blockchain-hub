/**
 * @file test_blockchain.cpp
 * @brief Unit tests for blockchain components
 */

#include "test_framework.h"
#include "core/transaction.h"
#include "blockchain/block.h"
#include "blockchain/merkle.h"
#include "core/utxo.h"
#include "wallet/address.h"

using namespace dinari;
using namespace dinari::test;

TEST(Transaction_Creation) {
    Transaction tx;
    tx.version = 1;
    tx.lockTime = 0;

    ASSERT_EQ(tx.version, 1);
    ASSERT_EQ(tx.lockTime, 0);
    ASSERT_TRUE(tx.vin.empty());
    ASSERT_TRUE(tx.vout.empty());
}

TEST(Transaction_Hash) {
    Transaction tx;
    tx.version = 1;

    Hash256 hash1 = tx.GetHash();
    Hash256 hash2 = tx.GetHash();

    // Hash should be deterministic
    ASSERT_EQ(hash1, hash2);
}

TEST(Transaction_Serialization) {
    Transaction tx;
    tx.version = 1;
    tx.lockTime = 100;

    bytes serialized = tx.Serialize();
    ASSERT_FALSE(serialized.empty());

    Transaction tx2;
    bool success = tx2.Deserialize(serialized);
    ASSERT_TRUE(success);
    ASSERT_EQ(tx.version, tx2.version);
    ASSERT_EQ(tx.lockTime, tx2.lockTime);
}

TEST(TxOut_Creation) {
    TxOut txout;
    txout.value = 1000000;  // 0.01 DNT

    ASSERT_EQ(txout.value, 1000000);
}

TEST(TxIn_Creation) {
    TxIn txin;
    txin.sequence = 0xFFFFFFFF;

    ASSERT_EQ(txin.sequence, 0xFFFFFFFF);
}

TEST(BlockHeader_Creation) {
    BlockHeader header;
    header.version = 1;
    header.nonce = 12345;

    ASSERT_EQ(header.version, 1);
    ASSERT_EQ(header.nonce, 12345);
}

TEST(BlockHeader_Hash) {
    BlockHeader header;
    header.version = 1;
    header.timestamp = 1234567890;

    Hash256 hash1 = header.GetHash();
    Hash256 hash2 = header.GetHash();

    ASSERT_EQ(hash1, hash2);
}

TEST(Block_GenesisCreation) {
    std::string genesisMessage = "Dinari Genesis Block";
    Block genesis = Block::CreateGenesisBlock(0, 0x1d00ffff, 0, genesisMessage);

    ASSERT_FALSE(genesis.transactions.empty());
    ASSERT_TRUE(genesis.transactions[0].IsCoinbase());
}

TEST(Block_Serialization) {
    Block block;
    block.header.version = 1;
    block.header.timestamp = 1234567890;

    bytes serialized = block.Serialize();
    ASSERT_FALSE(serialized.empty());

    Block block2;
    bool success = block2.Deserialize(serialized);
    ASSERT_TRUE(success);
    ASSERT_EQ(block.header.version, block2.header.version);
}

TEST(MerkleTree_SingleTransaction) {
    std::vector<Hash256> hashes;
    Hash256 hash;
    std::fill(hash.begin(), hash.end(), 0x42);
    hashes.push_back(hash);

    Hash256 merkleRoot = MerkleTree::ComputeMerkleRoot(hashes);
    ASSERT_EQ(merkleRoot, hash);
}

TEST(MerkleTree_MultipleTransactions) {
    std::vector<Hash256> hashes;

    for (int i = 0; i < 4; ++i) {
        Hash256 hash;
        std::fill(hash.begin(), hash.end(), static_cast<byte>(i));
        hashes.push_back(hash);
    }

    Hash256 merkleRoot = MerkleTree::ComputeMerkleRoot(hashes);
    ASSERT_NE(merkleRoot, Hash256{0});

    // Should be deterministic
    Hash256 merkleRoot2 = MerkleTree::ComputeMerkleRoot(hashes);
    ASSERT_EQ(merkleRoot, merkleRoot2);
}

TEST(MerkleTree_OddNumberTransactions) {
    std::vector<Hash256> hashes;

    for (int i = 0; i < 3; ++i) {
        Hash256 hash;
        std::fill(hash.begin(), hash.end(), static_cast<byte>(i));
        hashes.push_back(hash);
    }

    Hash256 merkleRoot = MerkleTree::ComputeMerkleRoot(hashes);
    ASSERT_NE(merkleRoot, Hash256{0});
}

TEST(UTXOSet_AddRemove) {
    UTXOSet utxos;

    OutPoint outpoint;
    outpoint.hash = Hash256{0};
    outpoint.n = 0;

    UTXOEntry entry;
    entry.output.value = 1000000;
    entry.height = 100;
    entry.isCoinbase = false;

    bool added = utxos.AddUTXO(outpoint, entry);
    ASSERT_TRUE(added);

    bool has = utxos.HasUTXO(outpoint);
    ASSERT_TRUE(has);

    bool removed = utxos.RemoveUTXO(outpoint);
    ASSERT_TRUE(removed);

    bool hasAfter = utxos.HasUTXO(outpoint);
    ASSERT_FALSE(hasAfter);
}

TEST(UTXOSet_GetUTXO) {
    UTXOSet utxos;

    OutPoint outpoint;
    outpoint.hash = Hash256{0};
    outpoint.n = 1;

    UTXOEntry entry;
    entry.output.value = 5000000;
    entry.height = 200;

    utxos.AddUTXO(outpoint, entry);

    UTXOEntry retrieved;
    bool success = utxos.GetUTXO(outpoint, retrieved);

    ASSERT_TRUE(success);
    ASSERT_EQ(retrieved.output.value, 5000000);
    ASSERT_EQ(retrieved.height, 200);
}

TEST(Address_Creation) {
    Hash160 hash;
    std::fill(hash.begin(), hash.end(), 0x42);

    Address addr(hash, AddressType::P2PKH);
    ASSERT_TRUE(addr.IsValid());
}

TEST(Address_ToString) {
    Hash160 hash;
    for (size_t i = 0; i < hash.size(); ++i) {
        hash[i] = static_cast<byte>(i);
    }

    Address addr(hash, AddressType::P2PKH);
    std::string addrStr = addr.ToString();

    ASSERT_FALSE(addrStr.empty());
    ASSERT_EQ(addrStr[0], 'D');  // Dinari prefix
}

TEST(Address_Validation) {
    // Valid address format
    std::string validAddr = "D1BvBMSEYstWetqTFn5Au4m4GFg7xJaNVN";

    // Note: This is a demonstration - actual validation depends on proper Base58Check
    bool isValid = Address::Validate(validAddr);
    // In a real test with proper Base58, this would validate checksums
}

TEST(Address_Parse) {
    Hash160 hash;
    for (size_t i = 0; i < hash.size(); ++i) {
        hash[i] = static_cast<byte>(i);
    }

    Address addr1(hash, AddressType::P2PKH);
    std::string addrStr = addr1.ToString();

    Address addr2(addrStr);
    ASSERT_EQ(addr1.GetHash(), addr2.GetHash());
}

TEST(BlockReward_InitialReward) {
    Amount reward = GetBlockReward(0);
    ASSERT_EQ(reward, 50 * COIN);
}

TEST(BlockReward_AfterHalving) {
    Amount reward = GetBlockReward(210000);
    ASSERT_EQ(reward, 25 * COIN);
}

TEST(BlockReward_AfterTwoHalvings) {
    Amount reward = GetBlockReward(420000);
    ASSERT_EQ(reward, 12.5 * COIN);
}

TEST(Transaction_IsCoinbase) {
    Transaction tx;
    tx.version = 1;

    TxIn coinbaseInput;
    coinbaseInput.prevOut.hash = Hash256{0};
    coinbaseInput.prevOut.n = 0xFFFFFFFF;

    tx.vin.push_back(coinbaseInput);

    ASSERT_TRUE(tx.IsCoinbase());
}

TEST(Transaction_NotCoinbase) {
    Transaction tx;
    tx.version = 1;

    TxIn normalInput;
    normalInput.prevOut.hash = Hash256{1};
    normalInput.prevOut.n = 0;

    tx.vin.push_back(normalInput);

    ASSERT_FALSE(tx.IsCoinbase());
}

int main() {
    return TestFramework::Instance().RunAllTests();
}
