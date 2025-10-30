#include "protocol.h"
#include <sstream>
#include <iomanip>

namespace dinari {

std::string NetworkAddress::ToString() const {
    std::ostringstream oss;

    if (IsIPv4()) {
        // IPv4 format
        oss << static_cast<int>(ip[12]) << "."
            << static_cast<int>(ip[13]) << "."
            << static_cast<int>(ip[14]) << "."
            << static_cast<int>(ip[15]);
    } else {
        // IPv6 format
        oss << std::hex;
        for (size_t i = 0; i < 16; i += 2) {
            if (i > 0) oss << ":";
            oss << std::setw(2) << std::setfill('0') << static_cast<int>(ip[i])
                << std::setw(2) << std::setfill('0') << static_cast<int>(ip[i + 1]);
        }
    }

    oss << ":" << port;

    return oss.str();
}

bool NetworkAddress::IsValid() const {
    // Check if not all zeros
    bool allZero = true;
    for (const auto& byte : ip) {
        if (byte != 0) {
            allZero = false;
            break;
        }
    }

    if (allZero || port == 0) {
        return false;
    }

    return true;
}

bool NetworkAddress::IsRoutable() const {
    if (!IsValid()) {
        return false;
    }

    // Check for local/private addresses
    if (IsLocal()) {
        return false;
    }

    if (IsIPv4()) {
        uint8_t b1 = ip[12];
        uint8_t b2 = ip[13];

        // Private networks (RFC 1918)
        if (b1 == 10) return false;
        if (b1 == 172 && b2 >= 16 && b2 <= 31) return false;
        if (b1 == 192 && b2 == 168) return false;

        // Loopback
        if (b1 == 127) return false;

        // Link-local
        if (b1 == 169 && b2 == 254) return false;

        // Multicast
        if (b1 >= 224) return false;
    }

    return true;
}

bool NetworkAddress::IsLocal() const {
    if (IsIPv4()) {
        uint8_t b1 = ip[12];
        return b1 == 127 || b1 == 0;
    }

    // IPv6 loopback (::1)
    bool isLoopback = true;
    for (size_t i = 0; i < 15; ++i) {
        if (ip[i] != 0) {
            isLoopback = false;
            break;
        }
    }
    if (isLoopback && ip[15] == 1) {
        return true;
    }

    return false;
}

} // namespace dinari
