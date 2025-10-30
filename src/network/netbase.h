#ifndef DINARI_NETWORK_NETBASE_H
#define DINARI_NETWORK_NETBASE_H

#include "protocol.h"
#include "dinari/types.h"
#include <string>
#include <vector>

namespace dinari {

/**
 * @brief Socket handle type
 */
#ifdef _WIN32
typedef uintptr_t SOCKET;
constexpr SOCKET INVALID_SOCKET_VALUE = ~0;
#else
typedef int SOCKET;
constexpr SOCKET INVALID_SOCKET_VALUE = -1;
#endif

/**
 * @brief Socket operations wrapper
 */
class NetBase {
public:
    /**
     * @brief Initialize networking (WSA on Windows)
     */
    static bool Initialize();

    /**
     * @brief Cleanup networking
     */
    static void Cleanup();

    /**
     * @brief Create TCP socket
     */
    static SOCKET CreateSocket();

    /**
     * @brief Close socket
     */
    static void CloseSocket(SOCKET socket);

    /**
     * @brief Connect to address
     */
    static bool Connect(SOCKET socket, const NetworkAddress& addr, int timeout = 5000);

    /**
     * @brief Bind socket to address
     */
    static bool Bind(SOCKET socket, const NetworkAddress& addr);

    /**
     * @brief Listen for connections
     */
    static bool Listen(SOCKET socket, int backlog = 10);

    /**
     * @brief Accept incoming connection
     */
    static SOCKET Accept(SOCKET socket, NetworkAddress& addr);

    /**
     * @brief Send data
     */
    static int Send(SOCKET socket, const byte* data, size_t length);

    /**
     * @brief Receive data
     */
    static int Receive(SOCKET socket, byte* buffer, size_t length);

    /**
     * @brief Set socket to non-blocking mode
     */
    static bool SetNonBlocking(SOCKET socket, bool nonBlocking = true);

    /**
     * @brief Set socket options
     */
    static bool SetSocketOptions(SOCKET socket);

    /**
     * @brief Check if socket is valid
     */
    static bool IsValid(SOCKET socket);

    /**
     * @brief Get last socket error
     */
    static int GetLastError();

    /**
     * @brief Get error string
     */
    static std::string GetErrorString(int error);

    /**
     * @brief Resolve hostname to addresses
     */
    static std::vector<NetworkAddress> LookupHost(const std::string& hostname,
                                                   uint16_t port,
                                                   bool allowIPv6 = true);

    /**
     * @brief Parse address string (ip:port or hostname:port)
     */
    static bool ParseAddress(const std::string& addrStr,
                            NetworkAddress& addr,
                            uint16_t defaultPort = DEFAULT_PORT);

    /**
     * @brief Convert string IP to NetworkAddress
     */
    static bool StringToIP(const std::string& ip, NetworkAddress& addr);

private:
    static bool initialized;
};

/**
 * @brief RAII socket wrapper
 */
class SocketRAII {
public:
    explicit SocketRAII(SOCKET s = INVALID_SOCKET_VALUE) : socket(s) {}

    ~SocketRAII() {
        if (NetBase::IsValid(socket)) {
            NetBase::CloseSocket(socket);
        }
    }

    // No copy
    SocketRAII(const SocketRAII&) = delete;
    SocketRAII& operator=(const SocketRAII&) = delete;

    // Move semantics
    SocketRAII(SocketRAII&& other) noexcept : socket(other.socket) {
        other.socket = INVALID_SOCKET_VALUE;
    }

    SocketRAII& operator=(SocketRAII&& other) noexcept {
        if (this != &other) {
            if (NetBase::IsValid(socket)) {
                NetBase::CloseSocket(socket);
            }
            socket = other.socket;
            other.socket = INVALID_SOCKET_VALUE;
        }
        return *this;
    }

    SOCKET Get() const { return socket; }
    SOCKET Release() {
        SOCKET s = socket;
        socket = INVALID_SOCKET_VALUE;
        return s;
    }

    bool IsValid() const { return NetBase::IsValid(socket); }

private:
    SOCKET socket;
};

} // namespace dinari

#endif // DINARI_NETWORK_NETBASE_H
