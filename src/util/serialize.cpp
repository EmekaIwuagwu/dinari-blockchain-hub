#include "serialize.h"
#include <cstring>
#include <stdexcept>

namespace dinari {

// Serializer implementations

void Serializer::WriteUInt8(uint8_t value) {
    data.push_back(value);
}

void Serializer::WriteUInt16(uint16_t value) {
    value = htole16(value);
    const byte* ptr = reinterpret_cast<const byte*>(&value);
    data.insert(data.end(), ptr, ptr + sizeof(value));
}

void Serializer::WriteUInt32(uint32_t value) {
    value = htole32(value);
    const byte* ptr = reinterpret_cast<const byte*>(&value);
    data.insert(data.end(), ptr, ptr + sizeof(value));
}

void Serializer::WriteUInt64(uint64_t value) {
    value = htole64(value);
    const byte* ptr = reinterpret_cast<const byte*>(&value);
    data.insert(data.end(), ptr, ptr + sizeof(value));
}

void Serializer::WriteInt32(int32_t value) {
    WriteUInt32(static_cast<uint32_t>(value));
}

void Serializer::WriteInt64(int64_t value) {
    WriteUInt64(static_cast<uint64_t>(value));
}

void Serializer::WriteBool(bool value) {
    WriteUInt8(value ? 1 : 0);
}

void Serializer::WriteVarInt(uint64_t value) {
    // Bitcoin VarInt format
    if (value < 0xFD) {
        WriteUInt8(static_cast<uint8_t>(value));
    } else if (value <= 0xFFFF) {
        WriteUInt8(0xFD);
        WriteUInt16(static_cast<uint16_t>(value));
    } else if (value <= 0xFFFFFFFF) {
        WriteUInt8(0xFE);
        WriteUInt32(static_cast<uint32_t>(value));
    } else {
        WriteUInt8(0xFF);
        WriteUInt64(value);
    }
}

void Serializer::WriteCompactSize(uint64_t size) {
    WriteVarInt(size);
}

void Serializer::WriteBytes(const bytes& value) {
    data.insert(data.end(), value.begin(), value.end());
}

void Serializer::WriteBytes(const byte* ptr, size_t len) {
    data.insert(data.end(), ptr, ptr + len);
}

void Serializer::WriteString(const std::string& value) {
    WriteCompactSize(value.size());
    data.insert(data.end(), value.begin(), value.end());
}

void Serializer::WriteHash256(const Hash256& hash) {
    data.insert(data.end(), hash.begin(), hash.end());
}

void Serializer::WriteHash160(const Hash160& hash) {
    data.insert(data.end(), hash.begin(), hash.end());
}

// Deserializer implementations

void Deserializer::CheckAvailable(size_t size) const {
    if (pos + size > data.size()) {
        throw std::runtime_error("Deserializer: not enough data");
    }
}

void Deserializer::Skip(size_t count) {
    CheckAvailable(count);
    pos += count;
}

uint8_t Deserializer::ReadUInt8() {
    CheckAvailable(1);
    return data[pos++];
}

uint16_t Deserializer::ReadUInt16() {
    CheckAvailable(2);
    uint16_t value;
    std::memcpy(&value, &data[pos], sizeof(value));
    pos += sizeof(value);
    return le16toh(value);
}

uint32_t Deserializer::ReadUInt32() {
    CheckAvailable(4);
    uint32_t value;
    std::memcpy(&value, &data[pos], sizeof(value));
    pos += sizeof(value);
    return le32toh(value);
}

uint64_t Deserializer::ReadUInt64() {
    CheckAvailable(8);
    uint64_t value;
    std::memcpy(&value, &data[pos], sizeof(value));
    pos += sizeof(value);
    return le64toh(value);
}

int32_t Deserializer::ReadInt32() {
    return static_cast<int32_t>(ReadUInt32());
}

int64_t Deserializer::ReadInt64() {
    return static_cast<int64_t>(ReadUInt64());
}

bool Deserializer::ReadBool() {
    return ReadUInt8() != 0;
}

uint64_t Deserializer::ReadVarInt() {
    uint8_t first = ReadUInt8();

    if (first < 0xFD) {
        return first;
    } else if (first == 0xFD) {
        return ReadUInt16();
    } else if (first == 0xFE) {
        return ReadUInt32();
    } else {
        return ReadUInt64();
    }
}

uint64_t Deserializer::ReadCompactSize() {
    return ReadVarInt();
}

bytes Deserializer::ReadBytes(size_t len) {
    CheckAvailable(len);
    bytes result(data.begin() + pos, data.begin() + pos + len);
    pos += len;
    return result;
}

std::string Deserializer::ReadString(size_t len) {
    CheckAvailable(len);
    std::string result(data.begin() + pos, data.begin() + pos + len);
    pos += len;
    return result;
}

Hash256 Deserializer::ReadHash256() {
    CheckAvailable(32);
    Hash256 hash;
    std::memcpy(hash.data(), &data[pos], 32);
    pos += 32;
    return hash;
}

Hash160 Deserializer::ReadHash160() {
    CheckAvailable(20);
    Hash160 hash;
    std::memcpy(hash.data(), &data[pos], 20);
    pos += 20;
    return hash;
}

bytes Deserializer::ReadRemaining() {
    bytes result(data.begin() + pos, data.end());
    pos = data.size();
    return result;
}

} // namespace dinari
