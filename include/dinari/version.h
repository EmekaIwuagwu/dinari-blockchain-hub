#ifndef DINARI_VERSION_H
#define DINARI_VERSION_H

#include <string>

namespace dinari {

// Version information
constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;
constexpr const char* VERSION_BUILD = "alpha";

// Client version
constexpr int CLIENT_VERSION = (VERSION_MAJOR * 10000) + (VERSION_MINOR * 100) + VERSION_PATCH;

// Protocol version
constexpr int PROTOCOL_VERSION_NUMBER = 100000;

// Minimum supported version for peers
constexpr int MIN_PROTOCOL_VERSION = 100000;

// Format version string
inline std::string GetVersionString() {
    return std::to_string(VERSION_MAJOR) + "." +
           std::to_string(VERSION_MINOR) + "." +
           std::to_string(VERSION_PATCH) +
           (VERSION_BUILD[0] ? std::string("-") + VERSION_BUILD : "");
}

// Get full version information
inline std::string GetFullVersionString() {
    return "Dinari Core version v" + GetVersionString();
}

// Get copyright string
inline std::string GetCopyrightString() {
    return "Copyright (C) 2025 The Dinari Blockchain Developers";
}

// Get build information
inline std::string GetBuildInfo() {
    std::string info = GetFullVersionString() + "\n";
    info += GetCopyrightString() + "\n";
    info += "Protocol version: " + std::to_string(PROTOCOL_VERSION_NUMBER) + "\n";

#ifdef _WIN32
    info += "Platform: Windows\n";
#elif __linux__
    info += "Platform: Linux\n";
#elif __APPLE__
    info += "Platform: macOS\n";
#else
    info += "Platform: Unknown\n";
#endif

#ifdef _DEBUG
    info += "Build type: Debug\n";
#else
    info += "Build type: Release\n";
#endif

    return info;
}

} // namespace dinari

#endif // DINARI_VERSION_H
