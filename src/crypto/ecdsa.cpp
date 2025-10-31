#include "ecdsa.h"
#include "hash.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>
#include <mutex>

namespace dinari {
namespace crypto {

// Static context for secp256k1
static EC_GROUP* secp256k1_group = nullptr;
static std::once_flag init_flag;

bool ECDSA::InitializeContext() {
    std::call_once(init_flag, []() {
        secp256k1_group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        if (!secp256k1_group) {
            throw std::runtime_error("Failed to initialize secp256k1 context");
        }
    });
    return secp256k1_group != nullptr;
}

void* ECDSA::GetContext() {
    InitializeContext();
    return secp256k1_group;
}

Hash256 ECDSA::GeneratePrivateKey() {
    InitializeContext();

    Hash256 privkey;

    // Generate random private key until we get a valid one
    do {
        if (RAND_bytes(privkey.data(), 32) != 1) {
            throw std::runtime_error("Failed to generate random bytes");
        }
    } while (!IsValidPrivateKey(privkey));

    return privkey;
}

bool ECDSA::IsValidPrivateKey(const Hash256& privkey) {
    InitializeContext();

    // Check if private key is in valid range (0 < key < n)
    // where n is the order of the secp256k1 curve

    BIGNUM* bn_key = BN_bin2bn(privkey.data(), 32, nullptr);
    if (!bn_key) return false;

    BIGNUM* bn_order = BN_new();
    EC_GROUP_get_order(secp256k1_group, bn_order, nullptr);

    bool valid = (BN_cmp(bn_key, BN_value_one()) >= 0) &&
                 (BN_cmp(bn_key, bn_order) < 0);

    BN_free(bn_key);
    BN_free(bn_order);

    return valid;
}

bytes ECDSA::GetPublicKey(const Hash256& privkey, bool compressed) {
    InitializeContext();

    // Create EC_KEY
    EC_KEY* key = EC_KEY_new();
    if (!key) {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    EC_KEY_set_group(key, secp256k1_group);

    // Set private key
    BIGNUM* bn_privkey = BN_bin2bn(privkey.data(), 32, nullptr);
    if (!bn_privkey || !EC_KEY_set_private_key(key, bn_privkey)) {
        BN_free(bn_privkey);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }
    BN_free(bn_privkey);

    // Compute public key
    EC_POINT* pub_key = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_mul(secp256k1_group, pub_key, bn_privkey, nullptr, nullptr, nullptr)) {
        EC_POINT_free(pub_key);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to compute public key");
    }

    EC_KEY_set_public_key(key, pub_key);

    // Convert to bytes
    point_conversion_form_t form = compressed ? POINT_CONVERSION_COMPRESSED : POINT_CONVERSION_UNCOMPRESSED;
    size_t len = EC_POINT_point2oct(secp256k1_group, pub_key, form, nullptr, 0, nullptr);

    bytes pubkey(len);
    EC_POINT_point2oct(secp256k1_group, pub_key, form, pubkey.data(), len, nullptr);

    EC_POINT_free(pub_key);
    EC_KEY_free(key);

    return pubkey;
}

bytes ECDSA::CompressPublicKey(const bytes& pubkey) {
    if (pubkey.size() != PUBKEY_UNCOMPRESSED_SIZE) {
        throw std::invalid_argument("Invalid uncompressed public key size");
    }

    InitializeContext();

    EC_POINT* point = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_oct2point(secp256k1_group, point, pubkey.data(), pubkey.size(), nullptr)) {
        EC_POINT_free(point);
        throw std::runtime_error("Failed to parse public key");
    }

    bytes compressed(PUBKEY_COMPRESSED_SIZE);
    EC_POINT_point2oct(secp256k1_group, point, POINT_CONVERSION_COMPRESSED,
                      compressed.data(), compressed.size(), nullptr);

    EC_POINT_free(point);
    return compressed;
}

bytes ECDSA::DecompressPublicKey(const bytes& pubkey) {
    if (pubkey.size() != PUBKEY_COMPRESSED_SIZE) {
        throw std::invalid_argument("Invalid compressed public key size");
    }

    InitializeContext();

    EC_POINT* point = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_oct2point(secp256k1_group, point, pubkey.data(), pubkey.size(), nullptr)) {
        EC_POINT_free(point);
        throw std::runtime_error("Failed to parse public key");
    }

    bytes uncompressed(PUBKEY_UNCOMPRESSED_SIZE);
    EC_POINT_point2oct(secp256k1_group, point, POINT_CONVERSION_UNCOMPRESSED,
                      uncompressed.data(), uncompressed.size(), nullptr);

    EC_POINT_free(point);
    return uncompressed;
}

bool ECDSA::IsValidPublicKey(const bytes& pubkey) {
    InitializeContext();

    if (pubkey.size() != PUBKEY_COMPRESSED_SIZE && pubkey.size() != PUBKEY_UNCOMPRESSED_SIZE) {
        return false;
    }

    EC_POINT* point = EC_POINT_new(secp256k1_group);
    if (!point) return false;

    bool valid = EC_POINT_oct2point(secp256k1_group, point, pubkey.data(), pubkey.size(), nullptr) != 0;

    if (valid) {
        valid = EC_POINT_is_on_curve(secp256k1_group, point, nullptr) == 1;
    }

    EC_POINT_free(point);
    return valid;
}

bytes ECDSA::Sign(const Hash256& hash, const Hash256& privkey) {
    InitializeContext();

    // Create EC_KEY
    EC_KEY* key = EC_KEY_new();
    if (!key) {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    EC_KEY_set_group(key, secp256k1_group);

    // Set private key
    BIGNUM* bn_privkey = BN_bin2bn(privkey.data(), 32, nullptr);
    if (!bn_privkey || !EC_KEY_set_private_key(key, bn_privkey)) {
        BN_free(bn_privkey);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }
    BN_free(bn_privkey);

    // Sign
    ECDSA_SIG* sig = ECDSA_do_sign(hash.data(), hash.size(), key);
    if (!sig) {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to sign");
    }

    // Convert to compact format (r || s)
    bytes signature(SIGNATURE_SIZE);

    const BIGNUM* r;
    const BIGNUM* s;
    ECDSA_SIG_get0(sig, &r, &s);

    BN_bn2binpad(r, signature.data(), 32);
    BN_bn2binpad(s, signature.data() + 32, 32);

    ECDSA_SIG_free(sig);
    EC_KEY_free(key);

    // Normalize to low S
    return NormalizeSignature(signature);
}

bytes ECDSA::SignDER(const Hash256& hash, const Hash256& privkey) {
    InitializeContext();

    EC_KEY* key = EC_KEY_new();
    if (!key) {
        throw std::runtime_error("Failed to create EC_KEY");
    }

    EC_KEY_set_group(key, secp256k1_group);

    BIGNUM* bn_privkey = BN_bin2bn(privkey.data(), 32, nullptr);
    if (!bn_privkey || !EC_KEY_set_private_key(key, bn_privkey)) {
        BN_free(bn_privkey);
        EC_KEY_free(key);
        throw std::runtime_error("Failed to set private key");
    }
    BN_free(bn_privkey);

    unsigned int sig_len = 0;
    bytes der_sig(DER_SIGNATURE_MAX_SIZE);

    if (ECDSA_sign(0, hash.data(), hash.size(), der_sig.data(), &sig_len, key) != 1) {
        EC_KEY_free(key);
        throw std::runtime_error("Failed to sign");
    }

    der_sig.resize(sig_len);
    EC_KEY_free(key);

    return der_sig;
}

bool ECDSA::Verify(const Hash256& hash, const bytes& signature, const bytes& pubkey) {
    InitializeContext();

    if (!IsValidPublicKey(pubkey)) {
        return false;
    }

    EC_KEY* key = EC_KEY_new();
    if (!key) return false;

    EC_KEY_set_group(key, secp256k1_group);

    // Set public key
    EC_POINT* pub_point = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_oct2point(secp256k1_group, pub_point, pubkey.data(), pubkey.size(), nullptr)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(key);
        return false;
    }

    EC_KEY_set_public_key(key, pub_point);

    bool result = false;

    // Try as DER signature first
    if (signature.size() <= DER_SIGNATURE_MAX_SIZE) {
        result = ECDSA_verify(0, hash.data(), hash.size(),
                             signature.data(), signature.size(), key) == 1;
    }

    // If DER fails and size is 64, try as compact signature
    if (!result && signature.size() == SIGNATURE_SIZE) {
        ECDSA_SIG* sig = ECDSA_SIG_new();
        BIGNUM* r = BN_bin2bn(signature.data(), 32, nullptr);
        BIGNUM* s = BN_bin2bn(signature.data() + 32, 32, nullptr);
        ECDSA_SIG_set0(sig, r, s);

        result = ECDSA_do_verify(hash.data(), hash.size(), sig, key) == 1;
        ECDSA_SIG_free(sig);
    }

    EC_POINT_free(pub_point);
    EC_KEY_free(key);

    return result;
}

bool ECDSA::IsLowS(const bytes& signature) {
    if (signature.size() != SIGNATURE_SIZE) {
        return false;
    }

    InitializeContext();

    BIGNUM* s = BN_bin2bn(signature.data() + 32, 32, nullptr);
    BIGNUM* order = BN_new();
    BIGNUM* half_order = BN_new();

    EC_GROUP_get_order(secp256k1_group, order, nullptr);
    BN_rshift1(half_order, order);

    int cmp = BN_cmp(s, half_order);

    BN_free(s);
    BN_free(order);
    BN_free(half_order);

    return cmp <= 0;
}

bytes ECDSA::NormalizeSignature(const bytes& signature) {
    if (signature.size() != SIGNATURE_SIZE) {
        return signature;
    }

    if (IsLowS(signature)) {
        return signature;
    }

    InitializeContext();

    bytes normalized = signature;

    BIGNUM* s = BN_bin2bn(signature.data() + 32, 32, nullptr);
    BIGNUM* order = BN_new();

    EC_GROUP_get_order(secp256k1_group, order, nullptr);
    BN_sub(s, order, s);  // s = order - s

    BN_bn2binpad(s, normalized.data() + 32, 32);

    BN_free(s);
    BN_free(order);

    return normalized;
}

Hash256 ECDSA::ECDH(const Hash256& privkey, const bytes& peerPubkey) {
    InitializeContext();

    // Parse peer's public key
    EC_POINT* peer_point = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_oct2point(secp256k1_group, peer_point, peerPubkey.data(), peerPubkey.size(), nullptr)) {
        EC_POINT_free(peer_point);
        throw std::runtime_error("Invalid peer public key");
    }

    // Multiply peer's public key with our private key
    BIGNUM* bn_privkey = BN_bin2bn(privkey.data(), 32, nullptr);
    EC_POINT* shared_point = EC_POINT_new(secp256k1_group);

    if (!EC_POINT_mul(secp256k1_group, shared_point, nullptr, peer_point, bn_privkey, nullptr)) {
        BN_free(bn_privkey);
        EC_POINT_free(peer_point);
        EC_POINT_free(shared_point);
        throw std::runtime_error("ECDH multiplication failed");
    }

    // Get x-coordinate as shared secret
    BIGNUM* shared_x = BN_new();
    EC_POINT_get_affine_coordinates(secp256k1_group, shared_point, shared_x, nullptr, nullptr);

    Hash256 shared_secret;
    BN_bn2binpad(shared_x, shared_secret.data(), 32);

    BN_free(shared_x);
    BN_free(bn_privkey);
    EC_POINT_free(peer_point);
    EC_POINT_free(shared_point);

    return shared_secret;
}

bytes ECDSA::RecoverPublicKey(const Hash256& hash, const bytes& signature, int recoveryId) {
    // Recovery not fully implemented in basic OpenSSL
    // For production, use libsecp256k1 which has better recovery support
    throw std::runtime_error("Public key recovery not implemented. Use libsecp256k1 for this feature.");
}

Hash256 ECDSA::PrivKeyAdd(const Hash256& key1, const Hash256& key2) {
    InitializeContext();

    BIGNUM* bn_key1 = BN_bin2bn(key1.data(), 32, nullptr);
    BIGNUM* bn_key2 = BN_bin2bn(key2.data(), 32, nullptr);
    BIGNUM* bn_order = BN_new();
    BIGNUM* bn_result = BN_new();

    EC_GROUP_get_order(secp256k1_group, bn_order, nullptr);

    // result = (key1 + key2) mod order
    BN_mod_add(bn_result, bn_key1, bn_key2, bn_order, nullptr);

    Hash256 result;
    BN_bn2binpad(bn_result, result.data(), 32);

    BN_free(bn_key1);
    BN_free(bn_key2);
    BN_free(bn_order);
    BN_free(bn_result);

    return result;
}

bytes ECDSA::PubKeyAdd(const bytes& pubkey, const Hash256& tweak) {
    InitializeContext();

    // Parse public key
    EC_POINT* point = EC_POINT_new(secp256k1_group);
    if (!EC_POINT_oct2point(secp256k1_group, point, pubkey.data(), pubkey.size(), nullptr)) {
        EC_POINT_free(point);
        throw std::runtime_error("Invalid public key");
    }

    // Get tweak point (G * tweak)
    BIGNUM* bn_tweak = BN_bin2bn(tweak.data(), 32, nullptr);
    EC_POINT* tweak_point = EC_POINT_new(secp256k1_group);
    EC_POINT_mul(secp256k1_group, tweak_point, bn_tweak, nullptr, nullptr, nullptr);

    // Add points
    EC_POINT* result_point = EC_POINT_new(secp256k1_group);
    EC_POINT_add(secp256k1_group, result_point, point, tweak_point, nullptr);

    // Convert to bytes (compressed)
    bytes result(PUBKEY_COMPRESSED_SIZE);
    EC_POINT_point2oct(secp256k1_group, result_point, POINT_CONVERSION_COMPRESSED,
                      result.data(), result.size(), nullptr);

    BN_free(bn_tweak);
    EC_POINT_free(point);
    EC_POINT_free(tweak_point);
    EC_POINT_free(result_point);

    return result;
}

Hash256 ECDSA::PrivKeyNegate(const Hash256& privkey) {
    InitializeContext();

    BIGNUM* bn_key = BN_bin2bn(privkey.data(), 32, nullptr);
    BIGNUM* bn_order = BN_new();
    BIGNUM* bn_result = BN_new();

    EC_GROUP_get_order(secp256k1_group, bn_order, nullptr);

    // result = order - key
    BN_sub(bn_result, bn_order, bn_key);

    Hash256 result;
    BN_bn2binpad(bn_result, result.data(), 32);

    BN_free(bn_key);
    BN_free(bn_order);
    BN_free(bn_result);

    return result;
}

// KeyPair methods
Hash160 KeyPair::GetHash160() const {
    return Hash::ComputeHash160(publicKey);
}

} // namespace crypto
} // namespace dinari
