/**
 * @file test_hash.cpp
 * @brief Unit tests for cryptographic hash functions
 */

#include "crypto/hash.h"
#include <gtest/gtest.h>

using namespace dinari::crypto;

TEST(HashTest, SHA256_BasicTest) {
    std::string data = "Hello, Dinari!";
    auto hash = Hash::SHA256(data);

    // Verify hash length
    EXPECT_EQ(hash.size(), 32);

    // Verify deterministic (same input = same output)
    auto hash2 = Hash::SHA256(data);
    EXPECT_EQ(hash, hash2);
}

TEST(HashTest, SHA256_EmptyString) {
    std::string empty = "";
    auto hash = Hash::SHA256(empty);

    // SHA-256 of empty string is well-known
    std::string expected = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    EXPECT_EQ(Hash::ToHex(hash), expected);
}

TEST(HashTest, DoubleSHA256) {
    std::string data = "Test data";
    auto singleHash = Hash::SHA256(data);
    auto doubleHash = Hash::DoubleSHA256(data);

    // Double hash should be different from single hash
    EXPECT_NE(singleHash, doubleHash);

    // Verify it's actually double hashing
    auto manualDouble = Hash::SHA256(std::vector<uint8_t>(singleHash.begin(), singleHash.end()));
    EXPECT_EQ(doubleHash, manualDouble);
}

TEST(HashTest, RIPEMD160_BasicTest) {
    std::string data = "Test";
    auto hash = Hash::RIPEMD160(data);

    // Verify hash length
    EXPECT_EQ(hash.size(), 20);
}

TEST(HashTest, Hash160_PublicKeyHash) {
    // Simulate public key
    std::vector<uint8_t> pubkey(33, 0x02);  // Compressed pubkey starting with 0x02

    auto hash160 = Hash::Hash160(pubkey);

    // Verify length
    EXPECT_EQ(hash160.size(), 20);

    // Verify it's deterministic
    auto hash160_2 = Hash::Hash160(pubkey);
    EXPECT_EQ(hash160, hash160_2);
}

TEST(HashTest, HexConversion) {
    std::string data = "Test";
    auto hash = Hash::SHA256(data);

    std::string hex = Hash::ToHex(hash);

    // Verify hex length (32 bytes = 64 hex chars)
    EXPECT_EQ(hex.length(), 64);

    // Verify round-trip
    auto hash2 = Hash::FromHex256(hex);
    EXPECT_EQ(hash, hash2);
}

TEST(HashTest, MerkleRoot_SingleTransaction) {
    std::vector<dinari::Hash256> hashes;
    hashes.push_back(Hash::SHA256("tx1"));

    auto root = Hash::ComputeMerkleRoot(hashes);

    // Single tx merkle root should be the tx hash itself
    EXPECT_EQ(root, hashes[0]);
}

TEST(HashTest, MerkleRoot_TwoTransactions) {
    std::vector<dinari::Hash256> hashes;
    hashes.push_back(Hash::SHA256("tx1"));
    hashes.push_back(Hash::SHA256("tx2"));

    auto root = Hash::ComputeMerkleRoot(hashes);

    // Root should be different from individual hashes
    EXPECT_NE(root, hashes[0]);
    EXPECT_NE(root, hashes[1]);

    // Should be deterministic
    auto root2 = Hash::ComputeMerkleRoot(hashes);
    EXPECT_EQ(root, root2);
}

TEST(HashTest, MerkleRoot_Empty) {
    std::vector<dinari::Hash256> empty;
    auto root = Hash::ComputeMerkleRoot(empty);

    // Empty merkle root should be all zeros
    dinari::Hash256 zero{};
    EXPECT_EQ(root, zero);
}

TEST(HashTest, HMAC_SHA256) {
    std::vector<uint8_t> key = {0x01, 0x02, 0x03};
    std::vector<uint8_t> data = {0x04, 0x05, 0x06};

    auto hmac = Hash::HMAC_SHA256(key, data);

    // Verify length
    EXPECT_EQ(hmac.size(), 32);

    // Verify deterministic
    auto hmac2 = Hash::HMAC_SHA256(key, data);
    EXPECT_EQ(hmac, hmac2);
}

TEST(HashTest, PBKDF2_KeyDerivation) {
    std::string password = "testpassword";
    std::vector<uint8_t> salt = {0x01, 0x02, 0x03, 0x04};
    int iterations = 1000;
    size_t keylen = 32;

    auto key = Hash::PBKDF2_SHA256(password, salt, iterations, keylen);

    // Verify length
    EXPECT_EQ(key.size(), keylen);

    // Verify deterministic
    auto key2 = Hash::PBKDF2_SHA256(password, salt, iterations, keylen);
    EXPECT_EQ(key, key2);

    // Different password should give different key
    auto key3 = Hash::PBKDF2_SHA256("different", salt, iterations, keylen);
    EXPECT_NE(key, key3);
}

TEST(HashTest, ProofOfWork_CheckTarget) {
    // Create a hash that should pass an easy target
    dinari::Hash256 easyHash{};
    easyHash[31] = 0x01;  // Very small number

    uint32_t easyTarget = 0x207fffff;  // Very easy target

    EXPECT_TRUE(Hash::CheckProofOfWork(easyHash, easyTarget));

    // Create a hash that should fail
    dinari::Hash256 hardHash{};
    for (auto& b : hardHash) b = 0xFF;  // Very large number

    EXPECT_FALSE(Hash::CheckProofOfWork(hardHash, easyTarget));
}

TEST(HashTest, TargetConversion) {
    uint32_t compact = 0x1d00ffff;

    // Convert to full target
    auto target = Hash::CompactToTarget(compact);

    // Convert back
    uint32_t compact2 = Hash::TargetToCompact(target);

    // Should be same (or very close due to rounding)
    EXPECT_EQ(compact, compact2);
}

// Main function
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
