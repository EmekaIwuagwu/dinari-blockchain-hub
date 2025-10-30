#ifndef DINARI_UTIL_SECURITY_H
#define DINARI_UTIL_SECURITY_H

#include "dinari/types.h"
#include <string>
#include <cstring>
#include <map>
#include <mutex>
#include <chrono>

namespace dinari {

/**
 * @brief Security utilities for cryptographic operations
 */
class Security {
public:
    /**
     * @brief Base64 decode a string
     */
    static std::string Base64Decode(const std::string& encoded);

    /**
     * @brief Base64 encode a string
     */
    static std::string Base64Encode(const std::string& data);

    /**
     * @brief Constant-time string comparison (prevents timing attacks)
     */
    static bool ConstantTimeCompare(const std::string& a, const std::string& b);

    /**
     * @brief Generate cryptographically secure random bytes
     */
    static bytes SecureRandomBytes(size_t length);

private:
    static const std::string base64_chars;
    static bool IsBase64(unsigned char c);
};

/**
 * @brief Rate limiter for preventing DoS attacks
 */
class RateLimiter {
public:
    /**
     * @brief Check if request from IP is allowed
     * @param key Identifier (e.g., IP address)
     * @param maxRequests Maximum requests allowed
     * @param windowSeconds Time window in seconds
     * @return true if request is allowed, false if rate limit exceeded
     */
    bool CheckLimit(const std::string& key, size_t maxRequests, size_t windowSeconds);

    /**
     * @brief Clear old entries to prevent memory bloat
     */
    void CleanupOldEntries();

    /**
     * @brief Ban an identifier
     */
    void Ban(const std::string& key, size_t durationSeconds = 3600);

    /**
     * @brief Check if an identifier is banned
     */
    bool IsBanned(const std::string& key);

private:
    struct RequestHistory {
        std::vector<std::chrono::steady_clock::time_point> timestamps;
        std::chrono::steady_clock::time_point banUntil;
    };

    std::map<std::string, RequestHistory> history;
    mutable std::mutex mutex;
};

} // namespace dinari

#endif // DINARI_UTIL_SECURITY_H
