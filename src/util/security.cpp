#include "security.h"
#include <openssl/rand.h>
#include <algorithm>
#include <stdexcept>

namespace dinari {

// Base64 character set
const std::string Security::base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

bool Security::IsBase64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string Security::Base64Decode(const std::string& encoded) {
    size_t in_len = encoded.size();
    size_t i = 0;
    size_t j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && (encoded[in_] != '=') && IsBase64(encoded[in_])) {
        char_array_4[i++] = encoded[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}

std::string Security::Base64Encode(const std::string& data) {
    unsigned char const* bytes_to_encode = reinterpret_cast<const unsigned char*>(data.c_str());
    size_t in_len = data.size();
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i < 4); i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

bool Security::ConstantTimeCompare(const std::string& a, const std::string& b) {
    // Constant-time comparison to prevent timing attacks
    if (a.length() != b.length()) {
        return false;
    }

    volatile unsigned char result = 0;
    for (size_t i = 0; i < a.length(); ++i) {
        result |= static_cast<unsigned char>(a[i] ^ b[i]);
    }

    return result == 0;
}

bytes Security::SecureRandomBytes(size_t length) {
    bytes result(length);
    if (RAND_bytes(result.data(), length) != 1) {
        throw std::runtime_error("Failed to generate secure random bytes");
    }
    return result;
}

// RateLimiter implementation

bool RateLimiter::CheckLimit(const std::string& key, size_t maxRequests, size_t windowSeconds) {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    auto& record = history[key];

    // Check if banned
    if (now < record.banUntil) {
        return false;
    }

    // Remove timestamps outside the window
    auto cutoff = now - std::chrono::seconds(windowSeconds);
    record.timestamps.erase(
        std::remove_if(record.timestamps.begin(), record.timestamps.end(),
            [cutoff](const auto& timestamp) { return timestamp < cutoff; }),
        record.timestamps.end()
    );

    // Check if limit exceeded
    if (record.timestamps.size() >= maxRequests) {
        // Auto-ban if excessive requests
        if (record.timestamps.size() >= maxRequests * 2) {
            record.banUntil = now + std::chrono::hours(1);
        }
        return false;
    }

    // Record this request
    record.timestamps.push_back(now);
    return true;
}

void RateLimiter::CleanupOldEntries() {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::hours(24);

    for (auto it = history.begin(); it != history.end(); ) {
        // Remove if no recent activity and not banned
        if (it->second.timestamps.empty() && it->second.banUntil < now) {
            it = history.erase(it);
        } else {
            ++it;
        }
    }
}

void RateLimiter::Ban(const std::string& key, size_t durationSeconds) {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    history[key].banUntil = now + std::chrono::seconds(durationSeconds);
}

bool RateLimiter::IsBanned(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex);

    auto now = std::chrono::steady_clock::now();
    auto it = history.find(key);

    if (it == history.end()) {
        return false;
    }

    return now < it->second.banUntil;
}

} // namespace dinari
