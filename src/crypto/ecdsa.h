#ifndef DINARI_CRYPTO_ECDSA_H
#define DINARI_CRYPTO_ECDSA_H

#include "dinari/types.h"
#include <string>

namespace dinari {
namespace crypto {

/**
 * @brief ECDSA (Elliptic Curve Digital Signature Algorithm) using secp256k1
 *
 * This class provides cryptographic key generation, signing, and verification
 * using the secp256k1 elliptic curve (same as Bitcoin).
 *
 * Security considerations:
 * - Private keys must be kept secure and never exposed
 * - Use cryptographically secure random number generation
 * - Implement constant-time operations where possible
 */

// Public key can be compressed (33 bytes) or uncompressed (65 bytes)
constexpr size_t PUBKEY_COMPRESSED_SIZE = 33;
constexpr size_t PUBKEY_UNCOMPRESSED_SIZE = 65;
constexpr size_t PRIVKEY_SIZE = 32;
constexpr size_t SIGNATURE_SIZE = 64;  // Compact signature
constexpr size_t DER_SIGNATURE_MAX_SIZE = 72;  // DER encoded

class ECDSA {
public:
    /**
     * @brief Generate a new random private key
     * @return 32-byte private key
     */
    static Hash256 GeneratePrivateKey();

    /**
     * @brief Derive public key from private key
     * @param privkey Private key
     * @param compressed Whether to use compressed format (default: true)
     * @return Public key (33 or 65 bytes)
     */
    static bytes GetPublicKey(const Hash256& privkey, bool compressed = true);

    /**
     * @brief Compress an uncompressed public key
     * @param pubkey Uncompressed public key (65 bytes)
     * @return Compressed public key (33 bytes)
     */
    static bytes CompressPublicKey(const bytes& pubkey);

    /**
     * @brief Decompress a compressed public key
     * @param pubkey Compressed public key (33 bytes)
     * @return Uncompressed public key (65 bytes)
     */
    static bytes DecompressPublicKey(const bytes& pubkey);

    /**
     * @brief Sign a message hash with private key
     * @param hash Message hash (usually transaction hash)
     * @param privkey Private key
     * @return 64-byte compact signature
     */
    static bytes Sign(const Hash256& hash, const Hash256& privkey);

    /**
     * @brief Sign a message hash and return DER encoded signature
     * @param hash Message hash
     * @param privkey Private key
     * @return DER encoded signature (variable length, max 72 bytes)
     */
    static bytes SignDER(const Hash256& hash, const Hash256& privkey);

    /**
     * @brief Verify signature
     * @param hash Message hash
     * @param signature Signature (compact or DER format)
     * @param pubkey Public key
     * @return true if signature is valid
     */
    static bool Verify(const Hash256& hash, const bytes& signature, const bytes& pubkey);

    /**
     * @brief Recover public key from signature (if possible)
     * @param hash Message hash
     * @param signature Signature with recovery ID
     * @param recoveryId Recovery ID (0-3)
     * @return Public key if recovery successful
     */
    static bytes RecoverPublicKey(const Hash256& hash, const bytes& signature, int recoveryId);

    /**
     * @brief Validate private key
     * @param privkey Private key to validate
     * @return true if valid
     */
    static bool IsValidPrivateKey(const Hash256& privkey);

    /**
     * @brief Validate public key
     * @param pubkey Public key to validate
     * @return true if valid
     */
    static bool IsValidPublicKey(const bytes& pubkey);

    /**
     * @brief Perform ECDH (Elliptic Curve Diffie-Hellman)
     * Derive shared secret from private key and peer's public key
     * @param privkey Our private key
     * @param peerPubkey Peer's public key
     * @return Shared secret (32 bytes)
     */
    static Hash256 ECDH(const Hash256& privkey, const bytes& peerPubkey);

    /**
     * @brief Add two private keys (for HD wallet derivation)
     * @param key1 First private key
     * @param key2 Second private key (tweak)
     * @return Result key
     */
    static Hash256 PrivKeyAdd(const Hash256& key1, const Hash256& key2);

    /**
     * @brief Add a tweak to public key (for HD wallet derivation)
     * @param pubkey Public key
     * @param tweak Tweak to add
     * @return Result public key
     */
    static bytes PubKeyAdd(const bytes& pubkey, const Hash256& tweak);

    /**
     * @brief Negate a private key
     * @param privkey Private key
     * @return Negated private key
     */
    static Hash256 PrivKeyNegate(const Hash256& privkey);

    /**
     * @brief Check if signature uses low S value (BIP62)
     * Low S values prevent signature malleability
     * @param signature Signature to check
     * @return true if S is low
     */
    static bool IsLowS(const bytes& signature);

    /**
     * @brief Normalize signature to low S (BIP62)
     * @param signature Signature to normalize
     * @return Normalized signature
     */
    static bytes NormalizeSignature(const bytes& signature);

private:
    // Helper functions for secp256k1 operations
    static bool InitializeContext();
    static void* GetContext();
};

/**
 * @brief Key pair container
 */
struct KeyPair {
    Hash256 privateKey;
    bytes publicKey;  // Compressed by default

    KeyPair() = default;

    KeyPair(const Hash256& privkey, bool compressed = true)
        : privateKey(privkey), publicKey(ECDSA::GetPublicKey(privkey, compressed)) {}

    // Generate new random key pair
    static KeyPair Generate(bool compressed = true) {
        Hash256 privkey = ECDSA::GeneratePrivateKey();
        return KeyPair(privkey, compressed);
    }

    // Get address from public key
    Hash160 GetHash160() const;

    // Sign message
    bytes Sign(const Hash256& hash) const {
        return ECDSA::Sign(hash, privateKey);
    }

    // Verify with this key pair's public key
    bool Verify(const Hash256& hash, const bytes& signature) const {
        return ECDSA::Verify(hash, signature, publicKey);
    }
};

} // namespace crypto
} // namespace dinari

#endif // DINARI_CRYPTO_ECDSA_H
