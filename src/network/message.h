#ifndef DINARI_NETWORK_MESSAGE_H
#define DINARI_NETWORK_MESSAGE_H

#include "protocol.h"
#include "dinari/types.h"
#include "blockchain/block.h"
#include "core/transaction.h"
#include "util/serialize.h"
#include <vector>
#include <memory>

namespace dinari {

/**
 * @brief Network message header
 */
struct MessageHeader {
    uint32_t magic;           // Network magic bytes
    char command[12];         // Command name (null-terminated)
    uint32_t payloadSize;     // Payload size in bytes
    uint32_t checksum;        // First 4 bytes of double SHA-256

    MessageHeader()
        : magic(MAINNET_MAGIC)
        , command{0}
        , payloadSize(0)
        , checksum(0) {}

    bool IsValid(uint32_t expectedMagic) const;
};

/**
 * @brief Base network message
 */
class NetworkMessage {
public:
    virtual ~NetworkMessage() = default;

    virtual NetMsgType GetType() const = 0;
    virtual bytes Serialize() const = 0;
    virtual bool Deserialize(const bytes& data) = 0;

    static std::unique_ptr<NetworkMessage> CreateFromType(NetMsgType type);
};

/**
 * @brief VERSION message
 */
class VersionMessage : public NetworkMessage {
public:
    uint32_t version;
    uint64_t services;
    Timestamp timestamp;
    NetworkAddress addrRecv;
    NetworkAddress addrFrom;
    uint64_t nonce;
    std::string userAgent;
    BlockHeight startHeight;
    bool relay;

    VersionMessage();

    NetMsgType GetType() const override { return NetMsgType::VERSION; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief VERACK message (empty)
 */
class VerackMessage : public NetworkMessage {
public:
    NetMsgType GetType() const override { return NetMsgType::VERACK; }
    bytes Serialize() const override { return bytes(); }
    bool Deserialize(const bytes& data) override { return true; }
};

/**
 * @brief PING message
 */
class PingMessage : public NetworkMessage {
public:
    uint64_t nonce;

    PingMessage() : nonce(0) {}
    explicit PingMessage(uint64_t n) : nonce(n) {}

    NetMsgType GetType() const override { return NetMsgType::PING; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief PONG message
 */
class PongMessage : public NetworkMessage {
public:
    uint64_t nonce;

    PongMessage() : nonce(0) {}
    explicit PongMessage(uint64_t n) : nonce(n) {}

    NetMsgType GetType() const override { return NetMsgType::PONG; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief ADDR message
 */
class AddrMessage : public NetworkMessage {
public:
    std::vector<NetworkAddress> addresses;

    AddrMessage() {}
    explicit AddrMessage(const std::vector<NetworkAddress>& addrs) : addresses(addrs) {}

    NetMsgType GetType() const override { return NetMsgType::ADDR; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief GETADDR message (empty)
 */
class GetAddrMessage : public NetworkMessage {
public:
    NetMsgType GetType() const override { return NetMsgType::GETADDR; }
    bytes Serialize() const override { return bytes(); }
    bool Deserialize(const bytes& data) override { return true; }
};

/**
 * @brief INV message
 */
class InvMessage : public NetworkMessage {
public:
    std::vector<InvItem> inventory;

    InvMessage() {}
    explicit InvMessage(const std::vector<InvItem>& inv) : inventory(inv) {}

    NetMsgType GetType() const override { return NetMsgType::INV; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief GETDATA message
 */
class GetDataMessage : public NetworkMessage {
public:
    std::vector<InvItem> inventory;

    GetDataMessage() {}
    explicit GetDataMessage(const std::vector<InvItem>& inv) : inventory(inv) {}

    NetMsgType GetType() const override { return NetMsgType::GETDATA; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief NOTFOUND message
 */
class NotFoundMessage : public NetworkMessage {
public:
    std::vector<InvItem> inventory;

    NotFoundMessage() {}
    explicit NotFoundMessage(const std::vector<InvItem>& inv) : inventory(inv) {}

    NetMsgType GetType() const override { return NetMsgType::NOTFOUND; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief GETBLOCKS message
 */
class GetBlocksMessage : public NetworkMessage {
public:
    uint32_t version;
    BlockLocator locator;
    Hash256 hashStop;

    GetBlocksMessage() : version(PROTOCOL_VERSION), hashStop({0}) {}

    NetMsgType GetType() const override { return NetMsgType::GETBLOCKS; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief GETHEADERS message
 */
class GetHeadersMessage : public NetworkMessage {
public:
    uint32_t version;
    BlockLocator locator;
    Hash256 hashStop;

    GetHeadersMessage() : version(PROTOCOL_VERSION), hashStop({0}) {}

    NetMsgType GetType() const override { return NetMsgType::GETHEADERS; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief BLOCK message
 */
class BlockMessage : public NetworkMessage {
public:
    Block block;

    BlockMessage() {}
    explicit BlockMessage(const Block& b) : block(b) {}

    NetMsgType GetType() const override { return NetMsgType::BLOCK; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief HEADERS message
 */
class HeadersMessage : public NetworkMessage {
public:
    std::vector<BlockHeader> headers;

    HeadersMessage() {}
    explicit HeadersMessage(const std::vector<BlockHeader>& h) : headers(h) {}

    NetMsgType GetType() const override { return NetMsgType::HEADERS; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief TX message
 */
class TxMessage : public NetworkMessage {
public:
    Transaction tx;

    TxMessage() {}
    explicit TxMessage(const Transaction& t) : tx(t) {}

    NetMsgType GetType() const override { return NetMsgType::TX; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief MEMPOOL message (empty)
 */
class MempoolMessage : public NetworkMessage {
public:
    NetMsgType GetType() const override { return NetMsgType::MEMPOOL; }
    bytes Serialize() const override { return bytes(); }
    bool Deserialize(const bytes& data) override { return true; }
};

/**
 * @brief REJECT message
 */
class RejectMessage : public NetworkMessage {
public:
    std::string message;
    RejectCode code;
    std::string reason;
    bytes data;  // Optional

    RejectMessage() : code(RejectCode::INVALID) {}

    NetMsgType GetType() const override { return NetMsgType::REJECT; }
    bytes Serialize() const override;
    bool Deserialize(const bytes& data) override;
};

/**
 * @brief Message builder and parser
 */
class MessageSerializer {
public:
    /**
     * @brief Serialize message with header
     */
    static bytes SerializeMessage(const NetworkMessage& msg, uint32_t magic);

    /**
     * @brief Parse message from raw data
     */
    static std::unique_ptr<NetworkMessage> DeserializeMessage(
        const bytes& data, uint32_t expectedMagic, size_t& bytesConsumed);

    /**
     * @brief Calculate message checksum
     */
    static uint32_t CalculateChecksum(const bytes& payload);

private:
    static bytes SerializeHeader(const MessageHeader& header);
    static bool DeserializeHeader(const bytes& data, MessageHeader& header);
};

} // namespace dinari

#endif // DINARI_NETWORK_MESSAGE_H
