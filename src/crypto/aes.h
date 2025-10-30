#ifndef DINARI_CRYPTO_AES_H
#define DINARI_CRYPTO_AES_H

#include "dinari/types.h"
#include <string>

namespace dinari {
namespace crypto {

/**
 * @brief AES-256-CBC encryption for wallet data
 *
 * Used to encrypt sensitive data like private keys and wallet files.
 * Uses AES-256 in CBC mode with PKCS7 padding.
 */

class AES {
public:
    /**
     * @brief Encrypt data with AES-256-CBC
     * @param plaintext Data to encrypt
     * @param key 32-byte encryption key
     * @param iv 16-byte initialization vector
     * @return Encrypted data
     */
    static bytes Encrypt(const bytes& plaintext, const Hash256& key, const bytes& iv);

    /**
     * @brief Decrypt data with AES-256-CBC
     * @param ciphertext Encrypted data
     * @param key 32-byte encryption key
     * @param iv 16-byte initialization vector
     * @return Decrypted data (empty on failure)
     */
    static bytes Decrypt(const bytes& ciphertext, const Hash256& key, const bytes& iv);

    /**
     * @brief Encrypt string with password
     * Derives key from password using PBKDF2
     * @param plaintext String to encrypt
     * @param password Password for encryption
     * @return Encrypted data with salt and IV prepended
     */
    static bytes EncryptWithPassword(const std::string& plaintext, const std::string& password);

    /**
     * @brief Decrypt data with password
     * @param ciphertext Encrypted data (with salt and IV)
     * @param password Password for decryption
     * @return Decrypted string (empty on failure)
     */
    static std::string DecryptWithPassword(const bytes& ciphertext, const std::string& password);

    /**
     * @brief Generate random IV
     * @return 16-byte random IV
     */
    static bytes GenerateIV();

    /**
     * @brief Generate random salt
     * @return 32-byte random salt
     */
    static bytes GenerateSalt();

private:
    static constexpr size_t IV_SIZE = 16;
    static constexpr size_t SALT_SIZE = 32;
    static constexpr size_t KEY_SIZE = 32;
    static constexpr int PBKDF2_ITERATIONS = 100000;
};

} // namespace crypto
} // namespace dinari

#endif // DINARI_CRYPTO_AES_H
