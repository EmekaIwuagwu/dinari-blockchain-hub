#ifndef DINARI_UTIL_SERIALIZE_H
#define DINARI_UTIL_SERIALIZE_H

#include "dinari/types.h"
#include <vector>
#include <string>
#include <cstring>
#include <stdexcept>

namespace dinari {

/**
 * @brief Serialization and deserialization for blockchain data
 *
 * Provides efficient binary serialization for all blockchain data structures.
 * Uses little-endian byte order (consistent with Bitcoin).
 *
 * Supports:
 * - Basic types (integers, booleans)
 * - Variable-length integers (VarInt)
 * - Vectors and strings
 * - Custom types with Serialize/Deserialize methods
 */

class Serializer {
public:
    Serializer() = default;

    // Get serialized data
    const bytes& GetData() const { return data; }
    bytes&& MoveData() { return std::move(data); }

    // Get current size
    size_t Size() const { return data.size(); }

    // Clear data
    void Clear() { data.clear(); }

    // Reserve space
    void Reserve(size_t size) { data.reserve(size); }

    // Write basic types
    void WriteUInt8(uint8_t value);
    void WriteUInt16(uint16_t value);
    void WriteUInt32(uint32_t value);
    void WriteUInt64(uint64_t value);
    void WriteInt32(int32_t value);
    void WriteInt64(int64_t value);
    void WriteBool(bool value);

    // Write variable-length integer (Bitcoin VarInt format)
    void WriteVarInt(uint64_t value);

    // Write bytes and strings
    void WriteBytes(const bytes& value);
    void WriteBytes(const byte* data, size_t len);
    void WriteString(const std::string& value);

    // Write hash types
    void WriteHash256(const Hash256& hash);
    void WriteHash160(const Hash160& hash);

    // Write vector with size prefix
    template<typename T>
    void WriteVector(const std::vector<T>& vec);

    // Write compact size (used for vector sizes)
    void WriteCompactSize(uint64_t size);

    // Serialize any object with Serialize method
    template<typename T>
    void WriteObject(const T& obj);

private:
    bytes data;
};

class Deserializer {
public:
    explicit Deserializer(const bytes& data) : data(data), pos(0) {}
    explicit Deserializer(bytes&& data) : data(std::move(data)), pos(0) {}

    // Check if more data available
    bool Available() const { return pos < data.size(); }
    size_t Remaining() const { return data.size() - pos; }
    size_t Position() const { return pos; }

    // Skip bytes
    void Skip(size_t count);

    // Reset position
    void Reset() { pos = 0; }

    // Read basic types
    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    int32_t ReadInt32();
    int64_t ReadInt64();
    bool ReadBool();

    // Read variable-length integer
    uint64_t ReadVarInt();

    // Read bytes and strings
    bytes ReadBytes(size_t len);
    std::string ReadString(size_t len);

    // Read hash types
    Hash256 ReadHash256();
    Hash160 ReadHash160();

    // Read vector with size prefix
    template<typename T>
    std::vector<T> ReadVector();

    // Read compact size
    uint64_t ReadCompactSize();

    // Deserialize any object with Deserialize method
    template<typename T>
    T ReadObject();

    // Read remaining data
    bytes ReadRemaining();

private:
    bytes data;
    size_t pos;

    void CheckAvailable(size_t size) const;
};

// Helper functions for quick serialization/deserialization

template<typename T>
bytes Serialize(const T& obj) {
    Serializer s;
    s.WriteObject(obj);
    return s.MoveData();
}

template<typename T>
T Deserialize(const bytes& data) {
    Deserializer d(data);
    return d.ReadObject<T>();
}

// Serializable base class (CRTP pattern)
template<typename Derived>
class Serializable {
public:
    bytes Serialize() const {
        Serializer s;
        static_cast<const Derived*>(this)->SerializeImpl(s);
        return s.MoveData();
    }

    void Deserialize(const bytes& data) {
        Deserializer d(data);
        static_cast<Derived*>(this)->DeserializeImpl(d);
    }

    size_t GetSerializedSize() const {
        Serializer s;
        static_cast<const Derived*>(this)->SerializeImpl(s);
        return s.Size();
    }
};

// Template implementations

template<typename T>
void Serializer::WriteVector(const std::vector<T>& vec) {
    WriteCompactSize(vec.size());
    for (const auto& item : vec) {
        WriteObject(item);
    }
}

template<typename T>
void Serializer::WriteObject(const T& obj) {
    // If T has a Serialize method, use it
    if constexpr (std::is_invocable_v<decltype(&T::SerializeImpl), T, Serializer&>) {
        obj.SerializeImpl(*this);
    } else {
        // Otherwise, T must be a basic type already handled
        static_assert(sizeof(T) == 0, "Type must have SerializeImpl method");
    }
}

template<typename T>
std::vector<T> Deserializer::ReadVector() {
    uint64_t size = ReadCompactSize();
    if (size > 1000000) {  // Sanity check
        throw std::runtime_error("Vector size too large");
    }

    std::vector<T> vec;
    vec.reserve(size);
    for (uint64_t i = 0; i < size; ++i) {
        vec.push_back(ReadObject<T>());
    }
    return vec;
}

template<typename T>
T Deserializer::ReadObject() {
    T obj;
    // If T has a Deserialize method, use it
    if constexpr (requires { obj.DeserializeImpl(std::declval<Deserializer&>()); }) {
        obj.DeserializeImpl(*this);
        return obj;
    } else {
        static_assert(sizeof(T) == 0, "Type must have DeserializeImpl method");
    }
}

// Utility functions for endianness conversion
inline uint16_t htole16(uint16_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return __builtin_bswap16(x);
#endif
}

inline uint32_t htole32(uint32_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return __builtin_bswap32(x);
#endif
}

inline uint64_t htole64(uint64_t x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return __builtin_bswap64(x);
#endif
}

inline uint16_t le16toh(uint16_t x) { return htole16(x); }
inline uint32_t le32toh(uint32_t x) { return htole32(x); }
inline uint64_t le64toh(uint64_t x) { return htole64(x); }

} // namespace dinari

#endif // DINARI_UTIL_SERIALIZE_H
