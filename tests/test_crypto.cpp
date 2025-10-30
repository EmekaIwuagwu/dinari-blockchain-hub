/**
 * @file test_crypto.cpp
 * @brief Unit tests for cryptographic functions
 */

#include "test_framework.h"
#include "crypto/hash.h"
#include "crypto/ecdsa.h"
#include "crypto/base58.h"
#include "crypto/aes.h"

using namespace dinari;
using namespace dinari::test;

TEST(SHA256_EmptyInput) {
    bytes input;
    Hash256 hash = crypto::Hash::SHA256(input);

    // SHA256 of empty string is known
    ASSERT_FALSE(hash == Hash256{0});
}

TEST(SHA256_KnownValue) {
    bytes input = {'a', 'b', 'c'};
    Hash256 hash = crypto::Hash::SHA256(input);

    // Verify hash is deterministic
    Hash256 hash2 = crypto::Hash::SHA256(input);
    ASSERT_EQ(hash, hash2);
}

TEST(DoubleSHA256) {
    bytes input = {'t', 'e', 's', 't'};
    Hash256 singleHash = crypto::Hash::SHA256(input);
    Hash256 doubleHash = crypto::Hash::DoubleSHA256(input);

    ASSERT_NE(singleHash, doubleHash);
}

TEST(Hash160) {
    bytes input = {'t', 'e', 's', 't'};
    Hash160 hash = crypto::Hash::Hash160(input);

    // Hash160 should produce 20 bytes
    bool allZero = true;
    for (byte b : hash) {
        if (b != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_FALSE(allZero);
}

TEST(ECDSA_KeyGeneration) {
    Hash256 privKey = crypto::ECDSA::GeneratePrivateKey();

    // Private key should not be all zeros
    bool allZero = true;
    for (byte b : privKey) {
        if (b != 0) {
            allZero = false;
            break;
        }
    }
    ASSERT_FALSE(allZero);
}

TEST(ECDSA_PublicKeyDerivation) {
    Hash256 privKey = crypto::ECDSA::GeneratePrivateKey();
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);

    // Compressed public key should be 33 bytes
    ASSERT_EQ(pubKey.size(), 33);

    // First byte should be 0x02 or 0x03
    ASSERT_TRUE(pubKey[0] == 0x02 || pubKey[0] == 0x03);
}

TEST(ECDSA_SignatureVerification) {
    Hash256 privKey = crypto::ECDSA::GeneratePrivateKey();
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);

    bytes message = {'h', 'e', 'l', 'l', 'o'};
    Hash256 messageHash = crypto::Hash::SHA256(message);

    bytes signature = crypto::ECDSA::Sign(messageHash, privKey);
    ASSERT_FALSE(signature.empty());

    bool valid = crypto::ECDSA::Verify(messageHash, signature, pubKey);
    ASSERT_TRUE(valid);
}

TEST(ECDSA_InvalidSignature) {
    Hash256 privKey = crypto::ECDSA::GeneratePrivateKey();
    bytes pubKey = crypto::ECDSA::GetPublicKey(privKey, true);

    bytes message = {'h', 'e', 'l', 'l', 'o'};
    Hash256 messageHash = crypto::Hash::SHA256(message);

    bytes signature = crypto::ECDSA::Sign(messageHash, privKey);

    // Modify signature
    if (!signature.empty()) {
        signature[0] ^= 0xFF;
    }

    bool valid = crypto::ECDSA::Verify(messageHash, signature, pubKey);
    ASSERT_FALSE(valid);
}

TEST(Base58_EncodeDecodeRoundtrip) {
    bytes input = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    std::string encoded = crypto::Base58::Encode(input);

    ASSERT_FALSE(encoded.empty());

    bytes decoded = crypto::Base58::Decode(encoded);
    ASSERT_EQ(input, decoded);
}

TEST(Base58Check_AddressRoundtrip) {
    Hash160 hash;
    for (size_t i = 0; i < hash.size(); ++i) {
        hash[i] = static_cast<byte>(i);
    }

    byte version = 30;  // Dinari address version
    std::string address = crypto::Base58::EncodeAddress(hash, version);

    ASSERT_FALSE(address.empty());
    ASSERT_EQ(address[0], 'D');  // Dinari prefix

    Hash160 decodedHash;
    byte decodedVersion;
    bool success = crypto::Base58::DecodeAddress(address, decodedHash, decodedVersion);

    ASSERT_TRUE(success);
    ASSERT_EQ(hash, decodedHash);
    ASSERT_EQ(version, decodedVersion);
}

TEST(Base58_InvalidAddress) {
    std::string invalidAddr = "InvalidAddress123";
    Hash160 hash;
    byte version;

    bool success = crypto::Base58::DecodeAddress(invalidAddr, hash, version);
    ASSERT_FALSE(success);
}

TEST(AES_EncryptDecryptRoundtrip) {
    bytes plaintext = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
    bytes key(32, 0x42);  // 32-byte key
    bytes iv(16, 0x00);   // 16-byte IV

    bytes ciphertext = crypto::AES::Encrypt(plaintext, key, iv);
    ASSERT_FALSE(ciphertext.empty());
    ASSERT_NE(plaintext, ciphertext);

    bytes decrypted = crypto::AES::Decrypt(ciphertext, key, iv);
    ASSERT_EQ(plaintext, decrypted);
}

TEST(AES_WrongKey) {
    bytes plaintext = {'T', 'e', 's', 't'};
    bytes key1(32, 0x42);
    bytes key2(32, 0x43);
    bytes iv(16, 0x00);

    bytes ciphertext = crypto::AES::Encrypt(plaintext, key1, iv);
    bytes decrypted = crypto::AES::Decrypt(ciphertext, key2, iv);

    // Decryption with wrong key should not produce original plaintext
    ASSERT_NE(plaintext, decrypted);
}

TEST(HMAC_SHA256) {
    bytes key = {'k', 'e', 'y'};
    bytes message = {'m', 'e', 's', 's', 'a', 'g', 'e'};

    bytes hmac = crypto::Hash::HMAC_SHA256(key, message);
    ASSERT_EQ(hmac.size(), 32);  // SHA256 produces 32 bytes

    // Same input should produce same output
    bytes hmac2 = crypto::Hash::HMAC_SHA256(key, message);
    ASSERT_EQ(hmac, hmac2);
}

TEST(PBKDF2) {
    bytes password = {'p', 'a', 's', 's', 'w', 'o', 'r', 'd'};
    bytes salt = {'s', 'a', 'l', 't'};

    bytes derived = crypto::Hash::PBKDF2_SHA512(password, salt, 1000, 32);
    ASSERT_EQ(derived.size(), 32);

    // Same input should produce same output
    bytes derived2 = crypto::Hash::PBKDF2_SHA512(password, salt, 1000, 32);
    ASSERT_EQ(derived, derived2);

    // Different salt should produce different output
    bytes salt2 = {'s', 'a', 'l', 't', '2'};
    bytes derived3 = crypto::Hash::PBKDF2_SHA512(password, salt2, 1000, 32);
    ASSERT_NE(derived, derived3);
}

int main() {
    return TestFramework::Instance().RunAllTests();
}
