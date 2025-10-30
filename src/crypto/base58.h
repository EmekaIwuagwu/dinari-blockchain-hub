#ifndef DINARI_CRYPTO_BASE58_H
#define DINARI_CRYPTO_BASE58_H

#include "dinari/types.h"
#include <string>

namespace dinari {
namespace crypto {

/**
 * @brief Base58 and Base58Check encoding/decoding
 *
 * Base58 is used for encoding addresses and private keys in a human-readable format.
 * Base58Check adds a checksum for error detection.
 *
 * The Base58 alphabet excludes similar-looking characters (0, O, I, l) to prevent
 * confusion when manually transcribing addresses.
 */

class Base58 {
public:
    /**
     * @brief Encode data to Base58
     * @param data Data to encode
     * @return Base58 encoded string
     */
    static std::string Encode(const bytes& data);

    /**
     * @brief Decode Base58 string
     * @param encoded Base58 encoded string
     * @return Decoded data (empty if invalid)
     */
    static bytes Decode(const std::string& encoded);

    /**
     * @brief Encode data with checksum (Base58Check)
     * @param data Data to encode
     * @return Base58Check encoded string
     */
    static std::string EncodeCheck(const bytes& data);

    /**
     * @brief Decode Base58Check string and verify checksum
     * @param encoded Base58Check encoded string
     * @param result Output decoded data
     * @return true if valid and decoded successfully
     */
    static bool DecodeCheck(const std::string& encoded, bytes& result);

    /**
     * @brief Encode address with version byte
     * Creates a Dinari address with 'D' prefix
     * @param hash Public key hash (Hash160)
     * @param version Version byte (determines address type)
     * @return Address string (e.g., D1a2b3c4...)
     */
    static std::string EncodeAddress(const Hash160& hash, byte version);

    /**
     * @brief Decode Dinari address
     * @param address Address string
     * @param hash Output public key hash
     * @param version Output version byte
     * @return true if valid address
     */
    static bool DecodeAddress(const std::string& address, Hash160& hash, byte& version);

    /**
     * @brief Validate Dinari address format
     * @param address Address to validate
     * @return true if valid format and checksum
     */
    static bool IsValidAddress(const std::string& address);

    /**
     * @brief Encode private key to WIF (Wallet Import Format)
     * @param privkey 32-byte private key
     * @param compressed Whether public key is compressed
     * @param testnet Whether this is for testnet
     * @return WIF encoded private key
     */
    static std::string EncodePrivateKey(const Hash256& privkey,
                                        bool compressed = true,
                                        bool testnet = false);

    /**
     * @brief Decode WIF private key
     * @param wif WIF encoded private key
     * @param privkey Output private key
     * @param compressed Output whether compressed
     * @param testnet Output whether testnet
     * @return true if valid WIF
     */
    static bool DecodePrivateKey(const std::string& wif,
                                 Hash256& privkey,
                                 bool& compressed,
                                 bool& testnet);

private:
    // Base58 alphabet (Bitcoin/Dinari standard)
    static const char* ALPHABET;

    // Character map for decoding
    static const int8_t MAP[256];

    // Calculate checksum (first 4 bytes of double SHA-256)
    static bytes CalculateChecksum(const bytes& data);

    // Initialize character map
    static void InitializeMap();
};

} // namespace crypto
} // namespace dinari

#endif // DINARI_CRYPTO_BASE58_H
