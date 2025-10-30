#include "netbase.h"
#include "util/logger.h"
#include <cstring>
#include <sstream>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace dinari {

bool NetBase::initialized = false;

bool NetBase::Initialize() {
#ifdef _WIN32
    if (initialized) {
        return true;
    }

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        LOG_ERROR("NetBase", "WSAStartup failed: " + std::to_string(result));
        return false;
    }

    initialized = true;
    LOG_INFO("NetBase", "Network initialized");
    return true;
#else
    initialized = true;
    return true;
#endif
}

void NetBase::Cleanup() {
#ifdef _WIN32
    if (initialized) {
        WSACleanup();
        initialized = false;
    }
#endif
}

SOCKET NetBase::CreateSocket() {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!IsValid(s)) {
        LOG_ERROR("NetBase", "Failed to create socket: " + GetErrorString(GetLastError()));
    }
    return s;
}

void NetBase::CloseSocket(SOCKET socket) {
    if (!IsValid(socket)) {
        return;
    }

#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}

bool NetBase::Connect(SOCKET socket, const NetworkAddress& addr, int timeout) {
    if (!IsValid(socket)) {
        return false;
    }

    // Set non-blocking for timeout
    SetNonBlocking(socket, true);

    struct sockaddr_in sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(addr.port);

    // Convert IP
    if (addr.IsIPv4()) {
        std::memcpy(&sa.sin_addr.s_addr, &addr.ip[12], 4);
    } else {
        LOG_ERROR("NetBase", "IPv6 not fully supported yet");
        return false;
    }

    int result = connect(socket, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));

#ifdef _WIN32
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            LOG_ERROR("NetBase", "Connect failed: " + GetErrorString(error));
            return false;
        }
    }
#else
    if (result < 0) {
        if (errno != EINPROGRESS) {
            LOG_ERROR("NetBase", "Connect failed: " + GetErrorString(errno));
            return false;
        }
    }
#endif

    // Wait for connection with timeout
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socket, &writefds);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    result = select(static_cast<int>(socket) + 1, nullptr, &writefds, nullptr, &tv);
    if (result <= 0) {
        LOG_ERROR("NetBase", "Connect timeout or error");
        return false;
    }

    // Check connection status
    int error = 0;
    socklen_t len = sizeof(error);
#ifdef _WIN32
    getsockopt(socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len);
#else
    getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &len);
#endif

    if (error != 0) {
        LOG_ERROR("NetBase", "Connection failed: " + GetErrorString(error));
        return false;
    }

    // Set back to blocking
    SetNonBlocking(socket, false);

    return true;
}

bool NetBase::Bind(SOCKET socket, const NetworkAddress& addr) {
    if (!IsValid(socket)) {
        return false;
    }

    struct sockaddr_in sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(addr.port);

    if (addr.IsIPv4()) {
        std::memcpy(&sa.sin_addr.s_addr, &addr.ip[12], 4);
    } else {
        sa.sin_addr.s_addr = INADDR_ANY;
    }

    int result = bind(socket, reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
    if (result != 0) {
        LOG_ERROR("NetBase", "Bind failed: " + GetErrorString(GetLastError()));
        return false;
    }

    return true;
}

bool NetBase::Listen(SOCKET socket, int backlog) {
    if (!IsValid(socket)) {
        return false;
    }

    int result = listen(socket, backlog);
    if (result != 0) {
        LOG_ERROR("NetBase", "Listen failed: " + GetErrorString(GetLastError()));
        return false;
    }

    return true;
}

SOCKET NetBase::Accept(SOCKET socket, NetworkAddress& addr) {
    if (!IsValid(socket)) {
        return INVALID_SOCKET_VALUE;
    }

    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);

    SOCKET client = accept(socket, reinterpret_cast<struct sockaddr*>(&sa), &len);
    if (!IsValid(client)) {
        return INVALID_SOCKET_VALUE;
    }

    // Fill address
    addr.port = ntohs(sa.sin_port);
    addr.ip.fill(0);
    addr.ip[10] = 0xff;
    addr.ip[11] = 0xff;
    std::memcpy(&addr.ip[12], &sa.sin_addr.s_addr, 4);

    return client;
}

int NetBase::Send(SOCKET socket, const byte* data, size_t length) {
    if (!IsValid(socket)) {
        return -1;
    }

#ifdef _WIN32
    return send(socket, reinterpret_cast<const char*>(data), static_cast<int>(length), 0);
#else
    return send(socket, data, length, MSG_NOSIGNAL);
#endif
}

int NetBase::Receive(SOCKET socket, byte* buffer, size_t length) {
    if (!IsValid(socket)) {
        return -1;
    }

#ifdef _WIN32
    return recv(socket, reinterpret_cast<char*>(buffer), static_cast<int>(length), 0);
#else
    return recv(socket, buffer, length, 0);
#endif
}

bool NetBase::SetNonBlocking(SOCKET socket, bool nonBlocking) {
    if (!IsValid(socket)) {
        return false;
    }

#ifdef _WIN32
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1) return false;

    if (nonBlocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }

    return fcntl(socket, F_SETFL, flags) == 0;
#endif
}

bool NetBase::SetSocketOptions(SOCKET socket) {
    if (!IsValid(socket)) {
        return false;
    }

    // Set TCP_NODELAY (disable Nagle's algorithm)
    int one = 1;
#ifdef _WIN32
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&one), sizeof(one));
#else
    setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#endif

    // Set SO_KEEPALIVE
#ifdef _WIN32
    setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char*>(&one), sizeof(one));
#else
    setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one));
#endif

    // Set SO_REUSEADDR
#ifdef _WIN32
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof(one));
#else
    setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
#endif

    return true;
}

bool NetBase::IsValid(SOCKET socket) {
    return socket != INVALID_SOCKET_VALUE;
}

int NetBase::GetLastError() {
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

std::string NetBase::GetErrorString(int error) {
#ifdef _WIN32
    char* msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, error, 0, reinterpret_cast<char*>(&msg), 0, nullptr);
    std::string result = msg ? msg : "Unknown error";
    if (msg) LocalFree(msg);
    return result;
#else
    return std::strerror(error);
#endif
}

std::vector<NetworkAddress> NetBase::LookupHost(const std::string& hostname,
                                                uint16_t port,
                                                bool allowIPv6) {
    std::vector<NetworkAddress> addresses;

    struct addrinfo hints, *result = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = allowIPv6 ? AF_UNSPEC : AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0) {
        LOG_WARNING("NetBase", "DNS lookup failed for " + hostname);
        return addresses;
    }

    for (struct addrinfo* ai = result; ai != nullptr; ai = ai->ai_next) {
        NetworkAddress addr;
        addr.port = port;

        if (ai->ai_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ai->ai_addr);
            addr.ip.fill(0);
            addr.ip[10] = 0xff;
            addr.ip[11] = 0xff;
            std::memcpy(&addr.ip[12], &sa->sin_addr, 4);
            addresses.push_back(addr);
        } else if (ai->ai_family == AF_INET6 && allowIPv6) {
            struct sockaddr_in6* sa = reinterpret_cast<struct sockaddr_in6*>(ai->ai_addr);
            std::memcpy(addr.ip.data(), &sa->sin6_addr, 16);
            addresses.push_back(addr);
        }
    }

    freeaddrinfo(result);

    return addresses;
}

bool NetBase::ParseAddress(const std::string& addrStr,
                          NetworkAddress& addr,
                          uint16_t defaultPort) {
    // Parse format: "ip:port" or "hostname:port" or "ip"
    size_t colon = addrStr.find(':');

    std::string host;
    uint16_t port = defaultPort;

    if (colon != std::string::npos) {
        host = addrStr.substr(0, colon);
        try {
            port = static_cast<uint16_t>(std::stoi(addrStr.substr(colon + 1)));
        } catch (...) {
            return false;
        }
    } else {
        host = addrStr;
    }

    // Try parse as IP first
    if (StringToIP(host, addr)) {
        addr.port = port;
        return true;
    }

    // Try DNS lookup
    auto addresses = LookupHost(host, port, false);
    if (!addresses.empty()) {
        addr = addresses[0];
        return true;
    }

    return false;
}

bool NetBase::StringToIP(const std::string& ip, NetworkAddress& addr) {
    struct in_addr ipv4;
    if (inet_pton(AF_INET, ip.c_str(), &ipv4) == 1) {
        addr.ip.fill(0);
        addr.ip[10] = 0xff;
        addr.ip[11] = 0xff;
        std::memcpy(&addr.ip[12], &ipv4, 4);
        return true;
    }

    struct in6_addr ipv6;
    if (inet_pton(AF_INET6, ip.c_str(), &ipv6) == 1) {
        std::memcpy(addr.ip.data(), &ipv6, 16);
        return true;
    }

    return false;
}

} // namespace dinari
