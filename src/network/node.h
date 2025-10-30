#ifndef DINARI_NETWORK_NODE_H
#define DINARI_NETWORK_NODE_H

#include "protocol.h"
#include "peer.h"
#include "addrman.h"
#include "netbase.h"
#include "blockchain/blockchain.h"
#include "core/mempool.h"
#include <thread>
#include <atomic>
#include <map>
#include <vector>

namespace dinari {

/**
 * @brief Network node configuration
 */
struct NetworkConfig {
    bool listen;
    uint16_t port;
    uint32_t maxOutbound;
    uint32_t maxInbound;
    bool testnet;
    std::string dataDir;

    NetworkConfig()
        : listen(true)
        , port(DEFAULT_PORT)
        , maxOutbound(MAX_OUTBOUND_CONNECTIONS)
        , maxInbound(MAX_INBOUND_CONNECTIONS)
        , testnet(false)
        , dataDir(".") {}
};

/**
 * @brief Network node statistics
 */
struct NetworkStats {
    size_t totalPeers;
    size_t inboundPeers;
    size_t outboundPeers;
    size_t activePeers;
    size_t knownAddresses;
    uint64_t totalBytesSent;
    uint64_t totalBytesReceived;

    NetworkStats()
        : totalPeers(0)
        , inboundPeers(0)
        , outboundPeers(0)
        , activePeers(0)
        , knownAddresses(0)
        , totalBytesSent(0)
        , totalBytesReceived(0) {}
};

/**
 * @brief Network node
 *
 * Main networking component that:
 * - Manages peer connections
 * - Handles block and transaction propagation
 * - Coordinates with blockchain
 * - Provides network API
 */
class NetworkNode {
public:
    explicit NetworkNode(Blockchain& chain);
    ~NetworkNode();

    /**
     * @brief Initialize network
     */
    bool Initialize(const NetworkConfig& config);

    /**
     * @brief Start network threads
     */
    bool Start();

    /**
     * @brief Stop network
     */
    void Stop();

    /**
     * @brief Check if running
     */
    bool IsRunning() const { return running.load(); }

    /**
     * @brief Get network statistics
     */
    NetworkStats GetStats() const;

    /**
     * @brief Get connected peers
     */
    std::vector<PeerPtr> GetPeers() const;

    /**
     * @brief Get peer count
     */
    size_t GetPeerCount() const;

    /**
     * @brief Connect to peer
     */
    bool ConnectToPeer(const NetworkAddress& addr);

    /**
     * @brief Disconnect peer
     */
    void DisconnectPeer(uint64_t peerId);

    /**
     * @brief Broadcast block to peers
     */
    void BroadcastBlock(const Block& block);

    /**
     * @brief Broadcast transaction to peers
     */
    void BroadcastTransaction(const Transaction& tx);

    /**
     * @brief Request blocks from peers
     */
    void RequestBlocks(const BlockLocator& locator, const Hash256& hashStop = Hash256{0});

    /**
     * @brief Ban peer address
     */
    void BanAddress(const NetworkAddress& addr, Timestamp duration = 86400);

    /**
     * @brief Check if address is banned
     */
    bool IsBanned(const NetworkAddress& addr) const;

private:
    // Components
    Blockchain& blockchain;
    AddressManager addrman;
    NetworkConfig config;

    // Peers
    std::map<uint64_t, PeerPtr> peers;
    uint64_t nextPeerId;
    mutable std::mutex peersMutex;

    // Listen socket
    SocketRAII listenSocket;

    // Threads
    std::atomic<bool> running;
    std::atomic<bool> shouldStop;
    std::thread listenThread;
    std::thread networkThread;
    std::thread discoveryThread;

    // Banned addresses
    std::map<std::string, Timestamp> banned;
    mutable std::mutex bannedMutex;

    // Internal methods
    void ListenThreadFunc();
    void NetworkThreadFunc();
    void DiscoveryThreadFunc();

    void AcceptConnections();
    void ProcessPeers();
    void DiscoverPeers();

    PeerPtr AddPeer(SOCKET socket, const NetworkAddress& addr, bool inbound);
    void RemovePeer(uint64_t peerId);
    void CleanupPeers();

    void ProcessPeerMessages(PeerPtr peer);
    void HandleInvMessage(PeerPtr peer, const InvMessage& msg);
    void HandleGetDataMessage(PeerPtr peer, const GetDataMessage& msg);
    void HandleBlockMessage(PeerPtr peer, const BlockMessage& msg);
    void HandleTxMessage(PeerPtr peer, const TxMessage& msg);
    void HandleGetBlocksMessage(PeerPtr peer, const GetBlocksMessage& msg);
    void HandleGetHeadersMessage(PeerPtr peer, const GetHeadersMessage& msg);
    void HandleAddrMessage(PeerPtr peer, const AddrMessage& msg);
    void HandleGetAddrMessage(PeerPtr peer);

    void SendInventory(PeerPtr peer, const std::vector<InvItem>& items);
    void SendBlock(PeerPtr peer, const Hash256& blockHash);
    void SendTransaction(PeerPtr peer, const Hash256& txHash);
    void SendHeaders(PeerPtr peer, const std::vector<BlockHeader>& headers);
    void SendAddresses(PeerPtr peer, const std::vector<NetworkAddress>& addrs);

    bool ShouldConnectMore() const;
    size_t GetOutboundCount() const;
    size_t GetInboundCount() const;
};

} // namespace dinari

#endif // DINARI_NETWORK_NODE_H
