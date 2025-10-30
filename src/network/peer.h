#ifndef DINARI_NETWORK_PEER_H
#define DINARI_NETWORK_PEER_H

#include "protocol.h"
#include "message.h"
#include "netbase.h"
#include "dinari/types.h"
#include <memory>
#include <mutex>
#include <queue>
#include <atomic>

namespace dinari {

/**
 * @brief Peer connection state
 */
enum class PeerState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    VERSION_SENT,
    VERSION_RECEIVED,
    ACTIVE,          // Fully handshaked
    DISCONNECTING
};

/**
 * @brief Peer connection statistics
 */
struct PeerStats {
    size_t bytesSent;
    size_t bytesReceived;
    size_t messagesSent;
    size_t messagesReceived;
    Timestamp lastSend;
    Timestamp lastRecv;
    Timestamp lastPing;
    Timestamp lastPong;
    uint64_t pingTime;  // milliseconds

    PeerStats()
        : bytesSent(0)
        , bytesReceived(0)
        , messagesSent(0)
        , messagesReceived(0)
        , lastSend(0)
        , lastRecv(0)
        , lastPing(0)
        , lastPong(0)
        , pingTime(0) {}
};

/**
 * @brief Peer connection
 *
 * Manages a single peer connection including:
 * - Connection lifecycle
 * - Message sending/receiving
 * - Protocol handshake
 * - Statistics tracking
 */
class Peer {
public:
    /**
     * @brief Construct peer from socket (inbound)
     */
    Peer(SOCKET socket, const NetworkAddress& addr, uint64_t id);

    /**
     * @brief Construct peer for outbound connection
     */
    Peer(const NetworkAddress& addr, uint64_t id);

    ~Peer();

    // No copy
    Peer(const Peer&) = delete;
    Peer& operator=(const Peer&) = delete;

    /**
     * @brief Connect to peer (outbound)
     */
    bool Connect();

    /**
     * @brief Disconnect from peer
     */
    void Disconnect();

    /**
     * @brief Send message to peer
     */
    bool SendMessage(const NetworkMessage& msg);

    /**
     * @brief Process incoming data
     *
     * @return true if should continue processing
     */
    bool ProcessIncoming();

    /**
     * @brief Process outgoing data
     */
    bool ProcessOutgoing();

    /**
     * @brief Update peer state (call periodically)
     */
    void Update();

    /**
     * @brief Get peer ID
     */
    uint64_t GetId() const { return id; }

    /**
     * @brief Get peer address
     */
    const NetworkAddress& GetAddress() const { return address; }

    /**
     * @brief Get peer state
     */
    PeerState GetState() const { return state.load(); }

    /**
     * @brief Check if peer is connected
     */
    bool IsConnected() const {
        auto s = state.load();
        return s != PeerState::DISCONNECTED && s != PeerState::DISCONNECTING;
    }

    /**
     * @brief Check if peer is fully active
     */
    bool IsActive() const { return state.load() == PeerState::ACTIVE; }

    /**
     * @brief Get peer statistics
     */
    PeerStats GetStats() const;

    /**
     * @brief Get peer version info
     */
    uint32_t GetVersion() const { return version; }
    BlockHeight GetStartHeight() const { return startHeight; }
    const std::string& GetUserAgent() const { return userAgent; }

    /**
     * @brief Check if inbound connection
     */
    bool IsInbound() const { return inbound; }

    /**
     * @brief Get last activity timestamp
     */
    Timestamp GetLastActivity() const;

    /**
     * @brief Check if peer should be disconnected (timeout, etc.)
     */
    bool ShouldDisconnect() const;

    /**
     * @brief Fetch received messages
     */
    std::vector<std::unique_ptr<NetworkMessage>> FetchMessages();

    /**
     * @brief Increase misbehavior score
     * @param howMuch Amount to increase score
     */
    void Misbehaving(int howMuch);

    /**
     * @brief Check if peer should be banned
     */
    bool ShouldBan() const;

    /**
     * @brief Get misbehavior score
     */
    int GetMisbehaviorScore() const { return misbehaviorScore.load(); }

private:
    // Connection info
    uint64_t id;
    NetworkAddress address;
    SocketRAII socket;
    bool inbound;
    std::atomic<PeerState> state;

    // Protocol version info
    uint32_t version;
    uint64_t services;
    BlockHeight startHeight;
    std::string userAgent;
    uint64_t nonce;  // For version handshake

    // Buffers
    bytes recvBuffer;
    std::queue<bytes> sendQueue;
    std::queue<std::unique_ptr<NetworkMessage>> messageQueue;

    // Statistics
    PeerStats stats;

    // Synchronization
    mutable std::mutex mutex;

    // Ping/pong
    uint64_t lastPingNonce;

    // Misbehavior tracking
    std::atomic<int> misbehaviorScore;
    static constexpr int BAN_THRESHOLD = 100;

    // Internal methods
    bool SendRaw(const bytes& data);
    bool ReceiveData();
    void ProcessMessages();
    void HandleMessage(std::unique_ptr<NetworkMessage> msg);
    void HandleVersionMessage(const VersionMessage& msg);
    void HandleVerackMessage();
    void HandlePingMessage(const PingMessage& msg);
    void HandlePongMessage(const PongMessage& msg);
    void SendVersionMessage();
    void SendVerackMessage();
    void SendPongMessage(uint64_t nonce);
    void SendPingMessage();
    void UpdateState(PeerState newState);
};

/**
 * @brief Peer pointer type
 */
using PeerPtr = std::shared_ptr<Peer>;

} // namespace dinari

#endif // DINARI_NETWORK_PEER_H
