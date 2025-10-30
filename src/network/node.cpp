#include "node.h"
#include "util/logger.h"
#include "util/time.h"
#include <algorithm>
#include <chrono>

namespace dinari {

NetworkNode::NetworkNode(Blockchain& chain)
    : blockchain(chain)
    , nextPeerId(1)
    , running(false)
    , shouldStop(false) {
}

NetworkNode::~NetworkNode() {
    Stop();
}

bool NetworkNode::Initialize(const NetworkConfig& cfg) {
    config = cfg;

    LOG_INFO("Network", "Initializing network node");

    // Initialize networking
    if (!NetBase::Initialize()) {
        LOG_ERROR("Network", "Failed to initialize network");
        return false;
    }

    // Initialize address manager
    if (!addrman.Initialize(config.testnet)) {
        LOG_WARNING("Network", "Failed to initialize address manager");
    }

    // Load saved addresses
    std::string addrFile = config.dataDir + "/peers.dat";
    addrman.LoadFromFile(addrFile);

    LOG_INFO("Network", "Network node initialized");

    return true;
}

bool NetworkNode::Start() {
    if (running.load()) {
        return true;
    }

    LOG_INFO("Network", "Starting network node");

    shouldStop.store(false);

    // Start listen socket if enabled
    if (config.listen) {
        SOCKET sock = NetBase::CreateSocket();
        if (!NetBase::IsValid(sock)) {
            LOG_ERROR("Network", "Failed to create listen socket");
            return false;
        }

        listenSocket = SocketRAII(sock);
        NetBase::SetSocketOptions(listenSocket.Get());
        NetBase::SetNonBlocking(listenSocket.Get(), true);

        NetworkAddress bindAddr;
        bindAddr.port = config.port;
        bindAddr.ip.fill(0);  // Bind to all interfaces

        if (!NetBase::Bind(listenSocket.Get(), bindAddr)) {
            LOG_ERROR("Network", "Failed to bind to port " + std::to_string(config.port));
            return false;
        }

        if (!NetBase::Listen(listenSocket.Get())) {
            LOG_ERROR("Network", "Failed to listen on socket");
            return false;
        }

        LOG_INFO("Network", "Listening on port " + std::to_string(config.port));

        // Start listen thread
        listenThread = std::thread(&NetworkNode::ListenThreadFunc, this);
    }

    // Start network thread
    networkThread = std::thread(&NetworkNode::NetworkThreadFunc, this);

    // Start discovery thread
    discoveryThread = std::thread(&NetworkNode::DiscoveryThreadFunc, this);

    running.store(true);

    LOG_INFO("Network", "Network node started");

    return true;
}

void NetworkNode::Stop() {
    if (!running.load()) {
        return;
    }

    LOG_INFO("Network", "Stopping network node");

    shouldStop.store(true);
    running.store(false);

    // Close listen socket
    listenSocket = SocketRAII(INVALID_SOCKET_VALUE);

    // Wait for threads
    if (listenThread.joinable()) {
        listenThread.join();
    }
    if (networkThread.joinable()) {
        networkThread.join();
    }
    if (discoveryThread.joinable()) {
        discoveryThread.join();
    }

    // Disconnect all peers
    {
        std::lock_guard<std::mutex> lock(peersMutex);
        peers.clear();
    }

    // Save addresses
    std::string addrFile = config.dataDir + "/peers.dat";
    addrman.SaveToFile(addrFile);

    NetBase::Cleanup();

    LOG_INFO("Network", "Network node stopped");
}

NetworkStats NetworkNode::GetStats() const {
    NetworkStats stats;

    std::lock_guard<std::mutex> lock(peersMutex);

    stats.totalPeers = peers.size();
    stats.knownAddresses = addrman.GetAddressCount();

    for (const auto& pair : peers) {
        const PeerPtr& peer = pair.second;

        if (peer->IsInbound()) {
            stats.inboundPeers++;
        } else {
            stats.outboundPeers++;
        }

        if (peer->IsActive()) {
            stats.activePeers++;
        }

        PeerStats peerStats = peer->GetStats();
        stats.totalBytesSent += peerStats.bytesSent;
        stats.totalBytesReceived += peerStats.bytesReceived;
    }

    return stats;
}

std::vector<PeerPtr> NetworkNode::GetPeers() const {
    std::lock_guard<std::mutex> lock(peersMutex);

    std::vector<PeerPtr> result;
    result.reserve(peers.size());

    for (const auto& pair : peers) {
        result.push_back(pair.second);
    }

    return result;
}

size_t NetworkNode::GetPeerCount() const {
    std::lock_guard<std::mutex> lock(peersMutex);
    return peers.size();
}

bool NetworkNode::ConnectToPeer(const NetworkAddress& addr) {
    if (IsBanned(addr)) {
        LOG_WARNING("Network", "Cannot connect to banned address: " + addr.ToString());
        return false;
    }

    LOG_INFO("Network", "Connecting to peer: " + addr.ToString());

    uint64_t peerId;
    PeerPtr peer;

    {
        std::lock_guard<std::mutex> lock(peersMutex);
        peerId = nextPeerId++;
        peer = std::make_shared<Peer>(addr, peerId);
        peers[peerId] = peer;
    }

    addrman.MarkAttempt(addr);

    if (!peer->Connect()) {
        LOG_ERROR("Network", "Failed to connect to peer: " + addr.ToString());
        RemovePeer(peerId);
        addrman.MarkFailed(addr);
        return false;
    }

    addrman.MarkGood(addr);
    addrman.MarkConnected(addr, true);

    LOG_INFO("Network", "Successfully connected to peer: " + addr.ToString());

    return true;
}

void NetworkNode::DisconnectPeer(uint64_t peerId) {
    std::lock_guard<std::mutex> lock(peersMutex);

    auto it = peers.find(peerId);
    if (it != peers.end()) {
        it->second->Disconnect();
        peers.erase(it);
    }
}

void NetworkNode::BroadcastBlock(const Block& block) {
    LOG_INFO("Network", "Broadcasting block " + block.GetHash().ToHex());

    InvItem item;
    item.type = InvType::BLOCK;
    item.hash = block.GetHash();

    std::vector<InvItem> inv = {item};

    auto peerList = GetPeers();
    for (const auto& peer : peerList) {
        if (peer->IsActive()) {
            InvMessage msg(inv);
            peer->SendMessage(msg);
        }
    }
}

void NetworkNode::BroadcastTransaction(const Transaction& tx) {
    LOG_INFO("Network", "Broadcasting transaction " + tx.GetHash().ToHex());

    InvItem item;
    item.type = InvType::TX;
    item.hash = tx.GetHash();

    std::vector<InvItem> inv = {item};

    auto peerList = GetPeers();
    for (const auto& peer : peerList) {
        if (peer->IsActive()) {
            InvMessage msg(inv);
            peer->SendMessage(msg);
        }
    }
}

void NetworkNode::RequestBlocks(const BlockLocator& locator, const Hash256& hashStop) {
    LOG_INFO("Network", "Requesting blocks from peers");

    GetBlocksMessage msg;
    msg.locator = locator;
    msg.hashStop = hashStop;

    auto peerList = GetPeers();
    for (const auto& peer : peerList) {
        if (peer->IsActive()) {
            peer->SendMessage(msg);
            break;  // Request from one peer at a time
        }
    }
}

void NetworkNode::BanAddress(const NetworkAddress& addr, Timestamp duration) {
    std::lock_guard<std::mutex> lock(bannedMutex);

    std::string key = addr.ToString();
    Timestamp until = Time::GetCurrentTime() + duration;

    banned[key] = until;

    LOG_INFO("Network", "Banned address: " + addr.ToString() +
             " for " + std::to_string(duration) + " seconds");
}

bool NetworkNode::IsBanned(const NetworkAddress& addr) const {
    std::lock_guard<std::mutex> lock(bannedMutex);

    std::string key = addr.ToString();
    auto it = banned.find(key);

    if (it == banned.end()) {
        return false;
    }

    Timestamp now = Time::GetCurrentTime();
    return now < it->second;
}

// Private methods

void NetworkNode::ListenThreadFunc() {
    LOG_INFO("Network", "Listen thread started");

    while (!shouldStop.load()) {
        AcceptConnections();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("Network", "Listen thread stopped");
}

void NetworkNode::NetworkThreadFunc() {
    LOG_INFO("Network", "Network thread started");

    while (!shouldStop.load()) {
        ProcessPeers();
        CleanupPeers();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    LOG_INFO("Network", "Network thread stopped");
}

void NetworkNode::DiscoveryThreadFunc() {
    LOG_INFO("Network", "Discovery thread started");

    while (!shouldStop.load()) {
        DiscoverPeers();
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    LOG_INFO("Network", "Discovery thread stopped");
}

void NetworkNode::AcceptConnections() {
    if (!listenSocket.IsValid()) {
        return;
    }

    NetworkAddress addr;
    SOCKET clientSock = NetBase::Accept(listenSocket.Get(), addr);

    if (!NetBase::IsValid(clientSock)) {
        return;
    }

    // Check if banned
    if (IsBanned(addr)) {
        LOG_WARNING("Network", "Rejected connection from banned address: " + addr.ToString());
        NetBase::CloseSocket(clientSock);
        return;
    }

    // Check connection limits
    size_t inbound = GetInboundCount();
    if (inbound >= config.maxInbound) {
        LOG_WARNING("Network", "Rejected connection: max inbound limit reached");
        NetBase::CloseSocket(clientSock);
        return;
    }

    LOG_INFO("Network", "Accepted connection from " + addr.ToString());

    AddPeer(clientSock, addr, true);
}

void NetworkNode::ProcessPeers() {
    auto peerList = GetPeers();

    for (const auto& peer : peerList) {
        if (!peer->IsConnected()) {
            continue;
        }

        // Process I/O
        peer->ProcessIncoming();
        peer->ProcessOutgoing();

        // Update state
        peer->Update();

        // Process messages
        ProcessPeerMessages(peer);
    }
}

void NetworkNode::DiscoverPeers() {
    if (!ShouldConnectMore()) {
        return;
    }

    NetworkAddress addr;
    if (addrman.GetAddress(addr)) {
        ConnectToPeer(addr);
    }
}

PeerPtr NetworkNode::AddPeer(SOCKET socket, const NetworkAddress& addr, bool inbound) {
    std::lock_guard<std::mutex> lock(peersMutex);

    uint64_t peerId = nextPeerId++;
    auto peer = std::make_shared<Peer>(socket, addr, peerId);

    peers[peerId] = peer;

    if (!inbound) {
        addrman.MarkConnected(addr, true);
    }

    LOG_INFO("Network", "Added " + std::string(inbound ? "inbound" : "outbound") +
             " peer " + std::to_string(peerId));

    return peer;
}

void NetworkNode::RemovePeer(uint64_t peerId) {
    std::lock_guard<std::mutex> lock(peersMutex);

    auto it = peers.find(peerId);
    if (it != peers.end()) {
        addrman.MarkConnected(it->second->GetAddress(), false);
        peers.erase(it);

        LOG_DEBUG("Network", "Removed peer " + std::to_string(peerId));
    }
}

void NetworkNode::CleanupPeers() {
    std::vector<uint64_t> toRemove;

    {
        std::lock_guard<std::mutex> lock(peersMutex);

        for (const auto& pair : peers) {
            if (!pair.second->IsConnected()) {
                toRemove.push_back(pair.first);
            }
        }
    }

    for (uint64_t peerId : toRemove) {
        RemovePeer(peerId);
    }
}

void NetworkNode::ProcessPeerMessages(PeerPtr peer) {
    auto messages = peer->FetchMessages();

    for (auto& msg : messages) {
        switch (msg->GetType()) {
            case NetMsgType::INV:
                HandleInvMessage(peer, *static_cast<InvMessage*>(msg.get()));
                break;

            case NetMsgType::GETDATA:
                HandleGetDataMessage(peer, *static_cast<GetDataMessage*>(msg.get()));
                break;

            case NetMsgType::BLOCK:
                HandleBlockMessage(peer, *static_cast<BlockMessage*>(msg.get()));
                break;

            case NetMsgType::TX:
                HandleTxMessage(peer, *static_cast<TxMessage*>(msg.get()));
                break;

            case NetMsgType::GETBLOCKS:
                HandleGetBlocksMessage(peer, *static_cast<GetBlocksMessage*>(msg.get()));
                break;

            case NetMsgType::GETHEADERS:
                HandleGetHeadersMessage(peer, *static_cast<GetHeadersMessage*>(msg.get()));
                break;

            case NetMsgType::ADDR:
                HandleAddrMessage(peer, *static_cast<AddrMessage*>(msg.get()));
                break;

            case NetMsgType::GETADDR:
                HandleGetAddrMessage(peer);
                break;

            default:
                break;
        }
    }
}

void NetworkNode::HandleInvMessage(PeerPtr peer, const InvMessage& msg) {
    LOG_DEBUG("Network", "Received INV with " + std::to_string(msg.inventory.size()) + " items");

    std::vector<InvItem> toRequest;

    for (const auto& item : msg.inventory) {
        if (item.type == InvType::BLOCK) {
            // Check if we need this block
            if (!blockchain.HasBlock(item.hash)) {
                toRequest.push_back(item);
            }
        } else if (item.type == InvType::TX) {
            // Check if we need this transaction
            // TODO: Check mempool
            toRequest.push_back(item);
        }
    }

    if (!toRequest.empty()) {
        GetDataMessage getData(toRequest);
        peer->SendMessage(getData);
    }
}

void NetworkNode::HandleGetDataMessage(PeerPtr peer, const GetDataMessage& msg) {
    LOG_DEBUG("Network", "Received GETDATA with " + std::to_string(msg.inventory.size()) + " items");

    for (const auto& item : msg.inventory) {
        if (item.type == InvType::BLOCK) {
            SendBlock(peer, item.hash);
        } else if (item.type == InvType::TX) {
            SendTransaction(peer, item.hash);
        }
    }
}

void NetworkNode::HandleBlockMessage(PeerPtr peer, const BlockMessage& msg) {
    LOG_INFO("Network", "Received block " + msg.block.GetHash().ToHex());

    // Process block
    if (blockchain.AcceptBlock(msg.block)) {
        LOG_INFO("Network", "Accepted block from peer");
    } else {
        LOG_WARNING("Network", "Rejected block from peer");
    }
}

void NetworkNode::HandleTxMessage(PeerPtr peer, const TxMessage& msg) {
    LOG_DEBUG("Network", "Received transaction " + msg.tx.GetHash().ToHex());

    // Add to mempool
    // TODO: blockchain.GetMemPool().AddTransaction(msg.tx, ...);
}

void NetworkNode::HandleGetBlocksMessage(PeerPtr peer, const GetBlocksMessage& msg) {
    LOG_DEBUG("Network", "Received GETBLOCKS request");

    // Find blocks to send
    std::vector<InvItem> inventory;

    // TODO: Implement block locator processing
    // For now, just send current tip
    const BlockIndex* tip = blockchain.GetBestBlock();
    if (tip) {
        InvItem item;
        item.type = InvType::BLOCK;
        item.hash = tip->hash;
        inventory.push_back(item);
    }

    if (!inventory.empty()) {
        InvMessage invMsg(inventory);
        peer->SendMessage(invMsg);
    }
}

void NetworkNode::HandleGetHeadersMessage(PeerPtr peer, const GetHeadersMessage& msg) {
    LOG_DEBUG("Network", "Received GETHEADERS request");

    // TODO: Implement header sending
    std::vector<BlockHeader> headers;
    SendHeaders(peer, headers);
}

void NetworkNode::HandleAddrMessage(PeerPtr peer, const AddrMessage& msg) {
    LOG_DEBUG("Network", "Received ADDR with " + std::to_string(msg.addresses.size()) + " addresses");

    addrman.Add(msg.addresses);
}

void NetworkNode::HandleGetAddrMessage(PeerPtr peer) {
    LOG_DEBUG("Network", "Received GETADDR request");

    auto addrs = addrman.GetAddresses(1000);
    SendAddresses(peer, addrs);
}

void NetworkNode::SendBlock(PeerPtr peer, const Hash256& blockHash) {
    auto block = blockchain.GetBlock(blockHash);
    if (block) {
        BlockMessage msg(*block);
        peer->SendMessage(msg);

        LOG_DEBUG("Network", "Sent block " + blockHash.ToHex() + " to peer");
    } else {
        // Send NOTFOUND
        InvItem item;
        item.type = InvType::BLOCK;
        item.hash = blockHash;

        NotFoundMessage msg({item});
        peer->SendMessage(msg);
    }
}

void NetworkNode::SendTransaction(PeerPtr peer, const Hash256& txHash) {
    // TODO: Get transaction from mempool/blockchain
    // For now, send NOTFOUND
    InvItem item;
    item.type = InvType::TX;
    item.hash = txHash;

    NotFoundMessage msg({item});
    peer->SendMessage(msg);
}

void NetworkNode::SendHeaders(PeerPtr peer, const std::vector<BlockHeader>& headers) {
    HeadersMessage msg(headers);
    peer->SendMessage(msg);
}

void NetworkNode::SendAddresses(PeerPtr peer, const std::vector<NetworkAddress>& addrs) {
    if (addrs.empty()) {
        return;
    }

    AddrMessage msg(addrs);
    peer->SendMessage(msg);

    LOG_DEBUG("Network", "Sent " + std::to_string(addrs.size()) + " addresses to peer");
}

bool NetworkNode::ShouldConnectMore() const {
    size_t outbound = GetOutboundCount();
    return outbound < config.maxOutbound;
}

size_t NetworkNode::GetOutboundCount() const {
    std::lock_guard<std::mutex> lock(peersMutex);

    size_t count = 0;
    for (const auto& pair : peers) {
        if (!pair.second->IsInbound()) {
            count++;
        }
    }

    return count;
}

size_t NetworkNode::GetInboundCount() const {
    std::lock_guard<std::mutex> lock(peersMutex);

    size_t count = 0;
    for (const auto& pair : peers) {
        if (pair.second->IsInbound()) {
            count++;
        }
    }

    return count;
}

} // namespace dinari
