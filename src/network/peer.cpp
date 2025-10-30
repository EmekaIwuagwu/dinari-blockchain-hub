#include "peer.h"
#include "util/logger.h"
#include "util/time.h"
#include "crypto/hash.h"
#include <random>

namespace dinari {

// Helper to generate random nonce
static uint64_t GenerateNonce() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
}

Peer::Peer(SOCKET s, const NetworkAddress& addr, uint64_t peerId)
    : id(peerId)
    , address(addr)
    , socket(s)
    , inbound(true)
    , state(PeerState::CONNECTED)
    , version(0)
    , services(0)
    , startHeight(0)
    , nonce(GenerateNonce())
    , lastPingNonce(0) {

    NetBase::SetSocketOptions(socket.Get());
    NetBase::SetNonBlocking(socket.Get(), true);

    LOG_INFO("Peer", "New inbound peer " + std::to_string(id) + " from " + address.ToString());
}

Peer::Peer(const NetworkAddress& addr, uint64_t peerId)
    : id(peerId)
    , address(addr)
    , socket(INVALID_SOCKET_VALUE)
    , inbound(false)
    , state(PeerState::DISCONNECTED)
    , version(0)
    , services(0)
    , startHeight(0)
    , nonce(GenerateNonce())
    , lastPingNonce(0) {

    LOG_INFO("Peer", "Created outbound peer " + std::to_string(id) + " to " + address.ToString());
}

Peer::~Peer() {
    Disconnect();
    LOG_DEBUG("Peer", "Peer " + std::to_string(id) + " destroyed");
}

bool Peer::Connect() {
    std::lock_guard<std::mutex> lock(mutex);

    if (state != PeerState::DISCONNECTED) {
        return false;
    }

    LOG_INFO("Peer", "Connecting to " + address.ToString());

    UpdateState(PeerState::CONNECTING);

    // Create socket
    SOCKET s = NetBase::CreateSocket();
    if (!NetBase::IsValid(s)) {
        UpdateState(PeerState::DISCONNECTED);
        return false;
    }

    socket = SocketRAII(s);
    NetBase::SetSocketOptions(socket.Get());

    // Connect
    if (!NetBase::Connect(socket.Get(), address, 5000)) {
        LOG_ERROR("Peer", "Failed to connect to " + address.ToString());
        socket = SocketRAII(INVALID_SOCKET_VALUE);
        UpdateState(PeerState::DISCONNECTED);
        return false;
    }

    NetBase::SetNonBlocking(socket.Get(), true);
    UpdateState(PeerState::CONNECTED);

    LOG_INFO("Peer", "Connected to " + address.ToString());

    // Send VERSION message
    SendVersionMessage();

    return true;
}

void Peer::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex);

    if (state == PeerState::DISCONNECTED) {
        return;
    }

    LOG_INFO("Peer", "Disconnecting peer " + std::to_string(id));

    UpdateState(PeerState::DISCONNECTING);

    // Close socket
    socket = SocketRAII(INVALID_SOCKET_VALUE);

    // Clear buffers
    recvBuffer.clear();
    sendQueue = std::queue<bytes>();
    messageQueue = std::queue<std::unique_ptr<NetworkMessage>>();

    UpdateState(PeerState::DISCONNECTED);
}

bool Peer::SendMessage(const NetworkMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex);

    if (!IsConnected()) {
        return false;
    }

    // Serialize message
    bytes data = MessageSerializer::SerializeMessage(msg, MAINNET_MAGIC);

    // Add to send queue
    sendQueue.push(std::move(data));

    LOG_DEBUG("Peer", "Queued " + std::string(GetMessageTypeName(msg.GetType())) +
             " message to peer " + std::to_string(id));

    stats.messagesSent++;

    return true;
}

bool Peer::ProcessIncoming() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!IsConnected() || !socket.IsValid()) {
        return false;
    }

    // Receive data
    if (!ReceiveData()) {
        if (state != PeerState::DISCONNECTING) {
            LOG_WARNING("Peer", "Failed to receive from peer " + std::to_string(id));
            Disconnect();
        }
        return false;
    }

    // Process messages from buffer
    ProcessMessages();

    return true;
}

bool Peer::ProcessOutgoing() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!IsConnected() || !socket.IsValid()) {
        return false;
    }

    // Send queued data
    while (!sendQueue.empty()) {
        const bytes& data = sendQueue.front();

        int sent = NetBase::Send(socket.Get(), data.data(), data.size());
        if (sent < 0) {
#ifdef _WIN32
            int error = NetBase::GetLastError();
            if (error != WSAEWOULDBLOCK) {
                LOG_ERROR("Peer", "Send error: " + NetBase::GetErrorString(error));
                Disconnect();
                return false;
            }
#else
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                LOG_ERROR("Peer", "Send error: " + NetBase::GetErrorString(errno));
                Disconnect();
                return false;
            }
#endif
            break;  // Would block, try again later
        }

        stats.bytesSent += sent;
        stats.lastSend = Time::GetCurrentTime();

        sendQueue.pop();
    }

    return true;
}

void Peer::Update() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!IsConnected()) {
        return;
    }

    Timestamp now = Time::GetCurrentTime();

    // Send periodic pings
    if (state == PeerState::ACTIVE && now - stats.lastPing > PING_INTERVAL) {
        SendPingMessage();
    }

    // Check for timeout
    if (ShouldDisconnect()) {
        LOG_WARNING("Peer", "Peer " + std::to_string(id) + " timed out");
        Disconnect();
    }
}

PeerStats Peer::GetStats() const {
    std::lock_guard<std::mutex> lock(mutex);
    return stats;
}

Timestamp Peer::GetLastActivity() const {
    std::lock_guard<std::mutex> lock(mutex);
    return std::max(stats.lastSend, stats.lastRecv);
}

bool Peer::ShouldDisconnect() const {
    Timestamp now = Time::GetCurrentTime();
    Timestamp lastActivity = GetLastActivity();

    // Timeout check
    if (lastActivity > 0 && now - lastActivity > TIMEOUT_INTERVAL) {
        return true;
    }

    return false;
}

std::vector<std::unique_ptr<NetworkMessage>> Peer::FetchMessages() {
    std::lock_guard<std::mutex> lock(mutex);

    std::vector<std::unique_ptr<NetworkMessage>> messages;

    while (!messageQueue.empty()) {
        messages.push_back(std::move(messageQueue.front()));
        messageQueue.pop();
    }

    return messages;
}

// Private methods

bool Peer::ReceiveData() {
    byte buffer[8192];

    int received = NetBase::Receive(socket.Get(), buffer, sizeof(buffer));
    if (received < 0) {
#ifdef _WIN32
        int error = NetBase::GetLastError();
        if (error == WSAEWOULDBLOCK) {
            return true;  // No data available
        }
#else
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return true;  // No data available
        }
#endif
        return false;
    }

    if (received == 0) {
        // Connection closed
        return false;
    }

    // Add to receive buffer
    recvBuffer.insert(recvBuffer.end(), buffer, buffer + received);

    stats.bytesReceived += received;
    stats.lastRecv = Time::GetCurrentTime();

    return true;
}

void Peer::ProcessMessages() {
    while (recvBuffer.size() >= 24) {  // Minimum message size (header)
        size_t bytesConsumed = 0;

        auto msg = MessageSerializer::DeserializeMessage(
            recvBuffer, MAINNET_MAGIC, bytesConsumed);

        if (bytesConsumed == 0) {
            // Need more data
            break;
        }

        // Remove consumed bytes
        recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + bytesConsumed);

        if (msg) {
            LOG_DEBUG("Peer", "Received " + std::string(GetMessageTypeName(msg->GetType())) +
                     " from peer " + std::to_string(id));

            stats.messagesReceived++;
            HandleMessage(std::move(msg));
        }
    }
}

void Peer::HandleMessage(std::unique_ptr<NetworkMessage> msg) {
    switch (msg->GetType()) {
        case NetMsgType::VERSION:
            HandleVersionMessage(*static_cast<VersionMessage*>(msg.get()));
            break;

        case NetMsgType::VERACK:
            HandleVerackMessage();
            break;

        case NetMsgType::PING:
            HandlePingMessage(*static_cast<PingMessage*>(msg.get()));
            break;

        case NetMsgType::PONG:
            HandlePongMessage(*static_cast<PongMessage*>(msg.get()));
            break;

        default:
            // Queue for application to process
            messageQueue.push(std::move(msg));
            break;
    }
}

void Peer::HandleVersionMessage(const VersionMessage& msg) {
    LOG_INFO("Peer", "Received VERSION from peer " + std::to_string(id) +
             " (v" + std::to_string(msg.version) + ", " + msg.userAgent + ")");

    version = msg.version;
    services = msg.services;
    startHeight = msg.startHeight;
    userAgent = msg.userAgent;

    if (state == PeerState::CONNECTED) {
        // We're inbound, send our VERSION
        SendVersionMessage();
        UpdateState(PeerState::VERSION_RECEIVED);
    } else if (state == PeerState::VERSION_SENT) {
        UpdateState(PeerState::VERSION_RECEIVED);
    }

    // Send VERACK
    SendVerackMessage();
}

void Peer::HandleVerackMessage() {
    LOG_DEBUG("Peer", "Received VERACK from peer " + std::to_string(id));

    if (state == PeerState::VERSION_RECEIVED) {
        UpdateState(PeerState::ACTIVE);
        LOG_INFO("Peer", "Peer " + std::to_string(id) + " is now active");
    }
}

void Peer::HandlePingMessage(const PingMessage& msg) {
    LOG_DEBUG("Peer", "Received PING from peer " + std::to_string(id));
    SendPongMessage(msg.nonce);
}

void Peer::HandlePongMessage(const PongMessage& msg) {
    LOG_DEBUG("Peer", "Received PONG from peer " + std::to_string(id));

    if (msg.nonce == lastPingNonce) {
        Timestamp now = Time::GetCurrentTime();
        stats.pingTime = now - stats.lastPing;
        stats.lastPong = now;

        LOG_DEBUG("Peer", "Peer " + std::to_string(id) + " ping: " +
                 std::to_string(stats.pingTime) + "ms");
    }
}

void Peer::SendVersionMessage() {
    VersionMessage msg;
    msg.version = PROTOCOL_VERSION;
    msg.services = NODE_NETWORK;
    msg.timestamp = Time::GetCurrentTime();
    msg.addrRecv = address;
    msg.nonce = nonce;
    msg.startHeight = 0;  // TODO: Get from blockchain

    SendMessage(msg);
    UpdateState(PeerState::VERSION_SENT);

    LOG_DEBUG("Peer", "Sent VERSION to peer " + std::to_string(id));
}

void Peer::SendVerackMessage() {
    VerackMessage msg;
    SendMessage(msg);

    LOG_DEBUG("Peer", "Sent VERACK to peer " + std::to_string(id));
}

void Peer::SendPongMessage(uint64_t nonce) {
    PongMessage msg(nonce);
    SendMessage(msg);
}

void Peer::SendPingMessage() {
    lastPingNonce = GenerateNonce();
    PingMessage msg(lastPingNonce);
    SendMessage(msg);

    stats.lastPing = Time::GetCurrentTime();

    LOG_DEBUG("Peer", "Sent PING to peer " + std::to_string(id));
}

void Peer::UpdateState(PeerState newState) {
    state.store(newState);
}

} // namespace dinari
