#include "message.h"
#include "crypto/hash.h"
#include "util/logger.h"
#include <cstring>

namespace dinari {

// MessageHeader implementation

bool MessageHeader::IsValid(uint32_t expectedMagic) const {
    return magic == expectedMagic && payloadSize <= MAX_MESSAGE_SIZE;
}

// VersionMessage implementation

VersionMessage::VersionMessage()
    : version(PROTOCOL_VERSION)
    , services(NODE_NETWORK)
    , timestamp(0)
    , nonce(0)
    , userAgent("/Dinari:1.0.0/")
    , startHeight(0)
    , relay(true) {}

bytes VersionMessage::Serialize() const {
    Serializer s;
    s.WriteUInt32(version);
    s.WriteUInt64(services);
    s.WriteInt64(timestamp);

    // addrRecv
    s.WriteUInt64(addrRecv.services);
    s.Write(addrRecv.ip.data(), 16);
    s.WriteUInt16BE(addrRecv.port);

    // addrFrom
    s.WriteUInt64(addrFrom.services);
    s.Write(addrFrom.ip.data(), 16);
    s.WriteUInt16BE(addrFrom.port);

    s.WriteUInt64(nonce);
    s.WriteString(userAgent);
    s.WriteInt32(startHeight);
    s.WriteByte(relay ? 1 : 0);

    return s.GetBytes();
}

bool VersionMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);

        version = d.ReadUInt32();
        services = d.ReadUInt64();
        timestamp = d.ReadInt64();

        // addrRecv
        addrRecv.services = d.ReadUInt64();
        d.Read(addrRecv.ip.data(), 16);
        addrRecv.port = d.ReadUInt16BE();

        // addrFrom
        addrFrom.services = d.ReadUInt64();
        d.Read(addrFrom.ip.data(), 16);
        addrFrom.port = d.ReadUInt16BE();

        nonce = d.ReadUInt64();
        userAgent = d.ReadString();
        startHeight = d.ReadInt32();

        if (d.Available() > 0) {
            relay = d.ReadByte() != 0;
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize VERSION: " + std::string(e.what()));
        return false;
    }
}

// PingMessage implementation

bytes PingMessage::Serialize() const {
    Serializer s;
    s.WriteUInt64(nonce);
    return s.GetBytes();
}

bool PingMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        nonce = d.ReadUInt64();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize PING: " + std::string(e.what()));
        return false;
    }
}

// PongMessage implementation

bytes PongMessage::Serialize() const {
    Serializer s;
    s.WriteUInt64(nonce);
    return s.GetBytes();
}

bool PongMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        nonce = d.ReadUInt64();
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize PONG: " + std::string(e.what()));
        return false;
    }
}

// AddrMessage implementation

bytes AddrMessage::Serialize() const {
    Serializer s;
    s.WriteVarInt(addresses.size());

    for (const auto& addr : addresses) {
        s.WriteUInt32(addr.timestamp);
        s.WriteUInt64(addr.services);
        s.Write(addr.ip.data(), 16);
        s.WriteUInt16BE(addr.port);
    }

    return s.GetBytes();
}

bool AddrMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        uint64_t count = d.ReadVarInt();

        if (count > MAX_ADDRS_PER_MESSAGE) {
            LOG_ERROR("Message", "Too many addresses in ADDR message");
            return false;
        }

        addresses.clear();
        addresses.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            NetworkAddress addr;
            addr.timestamp = d.ReadUInt32();
            addr.services = d.ReadUInt64();
            d.Read(addr.ip.data(), 16);
            addr.port = d.ReadUInt16BE();

            addresses.push_back(addr);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize ADDR: " + std::string(e.what()));
        return false;
    }
}

// InvMessage implementation

bytes InvMessage::Serialize() const {
    Serializer s;
    s.WriteVarInt(inventory.size());

    for (const auto& item : inventory) {
        s.WriteUInt32(static_cast<uint32_t>(item.type));
        s.WriteHash256(item.hash);
    }

    return s.GetBytes();
}

bool InvMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        uint64_t count = d.ReadVarInt();

        if (count > MAX_INV_PER_MESSAGE) {
            LOG_ERROR("Message", "Too many items in INV message");
            return false;
        }

        inventory.clear();
        inventory.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            InvItem item;
            item.type = static_cast<InvType>(d.ReadUInt32());
            item.hash = d.ReadHash256();

            inventory.push_back(item);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize INV: " + std::string(e.what()));
        return false;
    }
}

// GetDataMessage implementation

bytes GetDataMessage::Serialize() const {
    Serializer s;
    s.WriteVarInt(inventory.size());

    for (const auto& item : inventory) {
        s.WriteUInt32(static_cast<uint32_t>(item.type));
        s.WriteHash256(item.hash);
    }

    return s.GetBytes();
}

bool GetDataMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        uint64_t count = d.ReadVarInt();

        if (count > MAX_INV_PER_MESSAGE) {
            LOG_ERROR("Message", "Too many items in GETDATA message");
            return false;
        }

        inventory.clear();
        inventory.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            InvItem item;
            item.type = static_cast<InvType>(d.ReadUInt32());
            item.hash = d.ReadHash256();

            inventory.push_back(item);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize GETDATA: " + std::string(e.what()));
        return false;
    }
}

// NotFoundMessage implementation

bytes NotFoundMessage::Serialize() const {
    Serializer s;
    s.WriteVarInt(inventory.size());

    for (const auto& item : inventory) {
        s.WriteUInt32(static_cast<uint32_t>(item.type));
        s.WriteHash256(item.hash);
    }

    return s.GetBytes();
}

bool NotFoundMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        uint64_t count = d.ReadVarInt();

        inventory.clear();
        inventory.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            InvItem item;
            item.type = static_cast<InvType>(d.ReadUInt32());
            item.hash = d.ReadHash256();

            inventory.push_back(item);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize NOTFOUND: " + std::string(e.what()));
        return false;
    }
}

// GetBlocksMessage implementation

bytes GetBlocksMessage::Serialize() const {
    Serializer s;
    s.WriteUInt32(version);
    s.WriteVarInt(locator.hashes.size());

    for (const auto& hash : locator.hashes) {
        s.WriteHash256(hash);
    }

    s.WriteHash256(hashStop);

    return s.GetBytes();
}

bool GetBlocksMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);

        version = d.ReadUInt32();
        uint64_t count = d.ReadVarInt();

        locator.hashes.clear();
        locator.hashes.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            locator.hashes.push_back(d.ReadHash256());
        }

        hashStop = d.ReadHash256();

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize GETBLOCKS: " + std::string(e.what()));
        return false;
    }
}

// GetHeadersMessage implementation

bytes GetHeadersMessage::Serialize() const {
    Serializer s;
    s.WriteUInt32(version);
    s.WriteVarInt(locator.hashes.size());

    for (const auto& hash : locator.hashes) {
        s.WriteHash256(hash);
    }

    s.WriteHash256(hashStop);

    return s.GetBytes();
}

bool GetHeadersMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);

        version = d.ReadUInt32();
        uint64_t count = d.ReadVarInt();

        locator.hashes.clear();
        locator.hashes.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            locator.hashes.push_back(d.ReadHash256());
        }

        hashStop = d.ReadHash256();

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize GETHEADERS: " + std::string(e.what()));
        return false;
    }
}

// BlockMessage implementation

bytes BlockMessage::Serialize() const {
    return block.Serialize();
}

bool BlockMessage::Deserialize(const bytes& data) {
    return block.Deserialize(data);
}

// HeadersMessage implementation

bytes HeadersMessage::Serialize() const {
    Serializer s;
    s.WriteVarInt(headers.size());

    for (const auto& header : headers) {
        bytes headerData = header.Serialize();
        s.Write(headerData.data(), headerData.size());
        s.WriteVarInt(0);  // Transaction count (0 for headers)
    }

    return s.GetBytes();
}

bool HeadersMessage::Deserialize(const bytes& data) {
    try {
        Deserializer d(data);
        uint64_t count = d.ReadVarInt();

        if (count > MAX_HEADERS_PER_MESSAGE) {
            LOG_ERROR("Message", "Too many headers in HEADERS message");
            return false;
        }

        headers.clear();
        headers.reserve(count);

        for (uint64_t i = 0; i < count; ++i) {
            BlockHeader header;
            bytes headerData = d.ReadBytes(80);  // Header is 80 bytes
            if (!header.Deserialize(headerData)) {
                return false;
            }

            uint64_t txCount = d.ReadVarInt();
            if (txCount != 0) {
                LOG_WARNING("Message", "HEADERS message contains transaction count");
            }

            headers.push_back(header);
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize HEADERS: " + std::string(e.what()));
        return false;
    }
}

// TxMessage implementation

bytes TxMessage::Serialize() const {
    return tx.Serialize();
}

bool TxMessage::Deserialize(const bytes& data) {
    return tx.Deserialize(data);
}

// RejectMessage implementation

bytes RejectMessage::Serialize() const {
    Serializer s;
    s.WriteString(message);
    s.WriteByte(static_cast<uint8_t>(code));
    s.WriteString(reason);

    if (!data.empty()) {
        s.Write(data.data(), data.size());
    }

    return s.GetBytes();
}

bool RejectMessage::Deserialize(const bytes& bytes) {
    try {
        Deserializer d(bytes);

        message = d.ReadString();
        code = static_cast<RejectCode>(d.ReadByte());
        reason = d.ReadString();

        if (d.Available() > 0) {
            data = d.ReadBytes(d.Available());
        }

        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Message", "Failed to deserialize REJECT: " + std::string(e.what()));
        return false;
    }
}

// NetworkMessage factory

std::unique_ptr<NetworkMessage> NetworkMessage::CreateFromType(NetMsgType type) {
    switch (type) {
        case NetMsgType::VERSION: return std::make_unique<VersionMessage>();
        case NetMsgType::VERACK: return std::make_unique<VerackMessage>();
        case NetMsgType::PING: return std::make_unique<PingMessage>();
        case NetMsgType::PONG: return std::make_unique<PongMessage>();
        case NetMsgType::ADDR: return std::make_unique<AddrMessage>();
        case NetMsgType::GETADDR: return std::make_unique<GetAddrMessage>();
        case NetMsgType::INV: return std::make_unique<InvMessage>();
        case NetMsgType::GETDATA: return std::make_unique<GetDataMessage>();
        case NetMsgType::NOTFOUND: return std::make_unique<NotFoundMessage>();
        case NetMsgType::GETBLOCKS: return std::make_unique<GetBlocksMessage>();
        case NetMsgType::GETHEADERS: return std::make_unique<GetHeadersMessage>();
        case NetMsgType::BLOCK: return std::make_unique<BlockMessage>();
        case NetMsgType::HEADERS: return std::make_unique<HeadersMessage>();
        case NetMsgType::TX: return std::make_unique<TxMessage>();
        case NetMsgType::MEMPOOL: return std::make_unique<MempoolMessage>();
        case NetMsgType::REJECT: return std::make_unique<RejectMessage>();
        default:
            LOG_WARNING("Message", "Unknown message type");
            return nullptr;
    }
}

// MessageSerializer implementation

bytes MessageSerializer::SerializeMessage(const NetworkMessage& msg, uint32_t magic) {
    // Serialize payload
    bytes payload = msg.Serialize();

    // Create header
    MessageHeader header;
    header.magic = magic;
    std::strncpy(header.command, GetMessageTypeName(msg.GetType()), 11);
    header.command[11] = '\0';
    header.payloadSize = static_cast<uint32_t>(payload.size());
    header.checksum = CalculateChecksum(payload);

    // Serialize header
    bytes headerBytes = SerializeHeader(header);

    // Combine
    bytes result;
    result.reserve(headerBytes.size() + payload.size());
    result.insert(result.end(), headerBytes.begin(), headerBytes.end());
    result.insert(result.end(), payload.begin(), payload.end());

    return result;
}

std::unique_ptr<NetworkMessage> MessageSerializer::DeserializeMessage(
    const bytes& data, uint32_t expectedMagic, size_t& bytesConsumed) {

    bytesConsumed = 0;

    // Need at least header
    if (data.size() < 24) {
        return nullptr;
    }

    // Deserialize header
    MessageHeader header;
    if (!DeserializeHeader(data, header)) {
        LOG_ERROR("Message", "Failed to deserialize message header");
        return nullptr;
    }

    if (!header.IsValid(expectedMagic)) {
        LOG_ERROR("Message", "Invalid message header");
        return nullptr;
    }

    // Check if we have full message
    if (data.size() < 24 + header.payloadSize) {
        return nullptr;  // Need more data
    }

    // Extract payload
    bytes payload(data.begin() + 24, data.begin() + 24 + header.payloadSize);

    // Verify checksum
    uint32_t calculatedChecksum = CalculateChecksum(payload);
    if (calculatedChecksum != header.checksum) {
        LOG_ERROR("Message", "Message checksum mismatch");
        return nullptr;
    }

    // Find message type from command string
    NetMsgType msgType = NetMsgType::VERSION;  // Default
    std::string command(header.command);

    if (command == "version") msgType = NetMsgType::VERSION;
    else if (command == "verack") msgType = NetMsgType::VERACK;
    else if (command == "ping") msgType = NetMsgType::PING;
    else if (command == "pong") msgType = NetMsgType::PONG;
    else if (command == "addr") msgType = NetMsgType::ADDR;
    else if (command == "getaddr") msgType = NetMsgType::GETADDR;
    else if (command == "inv") msgType = NetMsgType::INV;
    else if (command == "getdata") msgType = NetMsgType::GETDATA;
    else if (command == "notfound") msgType = NetMsgType::NOTFOUND;
    else if (command == "getblocks") msgType = NetMsgType::GETBLOCKS;
    else if (command == "getheaders") msgType = NetMsgType::GETHEADERS;
    else if (command == "block") msgType = NetMsgType::BLOCK;
    else if (command == "headers") msgType = NetMsgType::HEADERS;
    else if (command == "tx") msgType = NetMsgType::TX;
    else if (command == "mempool") msgType = NetMsgType::MEMPOOL;
    else if (command == "reject") msgType = NetMsgType::REJECT;
    else {
        LOG_WARNING("Message", "Unknown message command: " + command);
        bytesConsumed = 24 + header.payloadSize;
        return nullptr;
    }

    // Create and deserialize message
    auto msg = NetworkMessage::CreateFromType(msgType);
    if (!msg) {
        bytesConsumed = 24 + header.payloadSize;
        return nullptr;
    }

    if (!msg->Deserialize(payload)) {
        LOG_ERROR("Message", "Failed to deserialize message payload");
        bytesConsumed = 24 + header.payloadSize;
        return nullptr;
    }

    bytesConsumed = 24 + header.payloadSize;
    return msg;
}

uint32_t MessageSerializer::CalculateChecksum(const bytes& payload) {
    Hash256 hash = crypto::Hash::DoubleSHA256(payload);
    return *reinterpret_cast<const uint32_t*>(hash.data());
}

bytes MessageSerializer::SerializeHeader(const MessageHeader& header) {
    Serializer s;
    s.WriteUInt32(header.magic);
    s.Write(reinterpret_cast<const byte*>(header.command), 12);
    s.WriteUInt32(header.payloadSize);
    s.WriteUInt32(header.checksum);
    return s.GetBytes();
}

bool MessageSerializer::DeserializeHeader(const bytes& data, MessageHeader& header) {
    if (data.size() < 24) {
        return false;
    }

    try {
        Deserializer d(data);
        header.magic = d.ReadUInt32();
        d.Read(reinterpret_cast<byte*>(header.command), 12);
        header.payloadSize = d.ReadUInt32();
        header.checksum = d.ReadUInt32();

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace dinari
