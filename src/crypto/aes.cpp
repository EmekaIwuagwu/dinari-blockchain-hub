#include "aes.h"
#include "hash.h"
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>

namespace dinari {
namespace crypto {

bytes AES::Encrypt(const bytes& plaintext, const Hash256& key, const bytes& iv) {
    if (iv.size() != IV_SIZE) {
        throw std::invalid_argument("IV must be 16 bytes");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create cipher context");
    }

    // Initialize encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize encryption");
    }

    // Allocate output buffer (plaintext size + block size for padding)
    bytes ciphertext(plaintext.size() + AES_BLOCK_SIZE);
    int len = 0;
    int ciphertext_len = 0;

    // Encrypt
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption failed");
    }
    ciphertext_len = len;

    // Finalize (adds padding)
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Encryption finalization failed");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);
    return ciphertext;
}

bytes AES::Decrypt(const bytes& ciphertext, const Hash256& key, const bytes& iv) {
    if (iv.size() != IV_SIZE) {
        return bytes();  // Invalid IV
    }

    if (ciphertext.empty()) {
        return bytes();
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return bytes();
    }

    // Initialize decryption
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return bytes();
    }

    // Allocate output buffer
    bytes plaintext(ciphertext.size());
    int len = 0;
    int plaintext_len = 0;

    // Decrypt
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return bytes();
    }
    plaintext_len = len;

    // Finalize (removes padding)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return bytes();  // Padding error or wrong key
    }
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(plaintext_len);
    return plaintext;
}

bytes AES::GenerateIV() {
    bytes iv(IV_SIZE);
    if (RAND_bytes(iv.data(), IV_SIZE) != 1) {
        throw std::runtime_error("Failed to generate random IV");
    }
    return iv;
}

bytes AES::GenerateSalt() {
    bytes salt(SALT_SIZE);
    if (RAND_bytes(salt.data(), SALT_SIZE) != 1) {
        throw std::runtime_error("Failed to generate random salt");
    }
    return salt;
}

bytes AES::EncryptWithPassword(const std::string& plaintext, const std::string& password) {
    // Generate random salt and IV
    bytes salt = GenerateSalt();
    bytes iv = GenerateIV();

    // Derive key from password using PBKDF2
    bytes key = Hash::PBKDF2_SHA256(password, salt, PBKDF2_ITERATIONS, KEY_SIZE);
    Hash256 key256;
    std::copy(key.begin(), key.end(), key256.begin());

    // Encrypt
    bytes plaintext_bytes(plaintext.begin(), plaintext.end());
    bytes ciphertext = Encrypt(plaintext_bytes, key256, iv);

    // Combine: salt || iv || ciphertext
    bytes result;
    result.reserve(salt.size() + iv.size() + ciphertext.size());
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

std::string AES::DecryptWithPassword(const bytes& data, const std::string& password) {
    // Check minimum size (salt + iv)
    if (data.size() < SALT_SIZE + IV_SIZE) {
        return "";
    }

    // Extract salt, IV, and ciphertext
    bytes salt(data.begin(), data.begin() + SALT_SIZE);
    bytes iv(data.begin() + SALT_SIZE, data.begin() + SALT_SIZE + IV_SIZE);
    bytes ciphertext(data.begin() + SALT_SIZE + IV_SIZE, data.end());

    // Derive key from password
    bytes key = Hash::PBKDF2_SHA256(password, salt, PBKDF2_ITERATIONS, KEY_SIZE);
    Hash256 key256;
    std::copy(key.begin(), key.end(), key256.begin());

    // Decrypt
    bytes plaintext = Decrypt(ciphertext, key256, iv);
    if (plaintext.empty()) {
        return "";
    }

    return std::string(plaintext.begin(), plaintext.end());
}

} // namespace crypto
} // namespace dinari
