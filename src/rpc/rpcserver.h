#ifndef DINARI_RPC_RPCSERVER_H
#define DINARI_RPC_RPCSERVER_H

#include "dinari/types.h"
#include "blockchain/blockchain.h"
#include "wallet/wallet.h"
#include "network/node.h"
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

namespace dinari {

// Forward declarations
class JSONValue;
class JSONObject;
class JSONArray;

/**
 * @brief Simple JSON value types
 */
enum class JSONType {
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};

/**
 * @brief Simple JSON value
 */
class JSONValue {
public:
    JSONValue();
    explicit JSONValue(bool b);
    explicit JSONValue(int64_t n);
    explicit JSONValue(double d);
    explicit JSONValue(const std::string& s);
    explicit JSONValue(const char* s);

    JSONType GetType() const { return type; }

    bool IsNull() const { return type == JSONType::Null; }
    bool IsBool() const { return type == JSONType::Bool; }
    bool IsNumber() const { return type == JSONType::Number; }
    bool IsString() const { return type == JSONType::String; }
    bool IsArray() const { return type == JSONType::Array; }
    bool IsObject() const { return type == JSONType::Object; }

    bool GetBool() const;
    int64_t GetInt() const;
    double GetDouble() const;
    std::string GetString() const;

    std::string Serialize() const;
    static JSONValue Parse(const std::string& json);

private:
    JSONType type;
    bool boolValue;
    int64_t intValue;
    double doubleValue;
    std::string stringValue;
};

/**
 * @brief JSON object
 */
class JSONObject {
public:
    JSONObject() = default;

    void Set(const std::string& key, const JSONValue& value);
    void SetNull(const std::string& key);
    void SetBool(const std::string& key, bool value);
    void SetInt(const std::string& key, int64_t value);
    void SetDouble(const std::string& key, double value);
    void SetString(const std::string& key, const std::string& value);
    void SetObject(const std::string& key, const JSONObject& value);
    void SetArray(const std::string& key, const std::vector<JSONValue>& value);

    bool Has(const std::string& key) const;
    JSONValue Get(const std::string& key) const;

    std::string Serialize() const;
    static JSONObject Parse(const std::string& json);

    const std::map<std::string, JSONValue>& GetData() const { return data; }

private:
    std::map<std::string, JSONValue> data;
};

/**
 * @brief RPC request
 */
struct RPCRequest {
    std::string jsonrpc;  // Should be "2.0"
    std::string method;
    std::vector<JSONValue> params;
    JSONValue id;

    static RPCRequest Parse(const std::string& json);
};

/**
 * @brief RPC response
 */
struct RPCResponse {
    std::string jsonrpc;  // "2.0"
    JSONValue result;
    JSONObject error;
    JSONValue id;
    bool isError;

    RPCResponse() : jsonrpc("2.0"), isError(false) {}

    std::string Serialize() const;
};

/**
 * @brief RPC error codes (Bitcoin-compatible)
 */
enum RPCErrorCode {
    RPC_INVALID_REQUEST = -32600,
    RPC_METHOD_NOT_FOUND = -32601,
    RPC_INVALID_PARAMS = -32602,
    RPC_INTERNAL_ERROR = -32603,
    RPC_PARSE_ERROR = -32700,

    // General application errors
    RPC_MISC_ERROR = -1,
    RPC_TYPE_ERROR = -3,
    RPC_INVALID_ADDRESS_OR_KEY = -5,
    RPC_OUT_OF_MEMORY = -7,
    RPC_INVALID_PARAMETER = -8,
    RPC_DATABASE_ERROR = -20,
    RPC_DESERIALIZATION_ERROR = -22,
    RPC_VERIFY_ERROR = -25,
    RPC_VERIFY_REJECTED = -26,
    RPC_VERIFY_ALREADY_IN_CHAIN = -27,
    RPC_IN_WARMUP = -28,

    // Wallet errors
    RPC_WALLET_ERROR = -4,
    RPC_WALLET_INSUFFICIENT_FUNDS = -6,
    RPC_WALLET_INVALID_LABEL_NAME = -11,
    RPC_WALLET_KEYPOOL_RAN_OUT = -12,
    RPC_WALLET_UNLOCK_NEEDED = -13,
    RPC_WALLET_PASSPHRASE_INCORRECT = -14,
    RPC_WALLET_WRONG_ENC_STATE = -15,
    RPC_WALLET_ENCRYPTION_FAILED = -16,
    RPC_WALLET_ALREADY_UNLOCKED = -17
};

/**
 * @brief RPC command handler
 */
using RPCCommandHandler = std::function<JSONValue(const RPCRequest&,
                                                  Blockchain&,
                                                  Wallet*,
                                                  NetworkNode*)>;

/**
 * @brief RPC command definition
 */
struct RPCCommand {
    std::string name;
    RPCCommandHandler handler;
    std::string category;
    std::string description;
    std::string usage;
    bool requiresWallet;

    RPCCommand(const std::string& n,
               RPCCommandHandler h,
               const std::string& cat,
               const std::string& desc,
               const std::string& use,
               bool wallet = false)
        : name(n)
        , handler(h)
        , category(cat)
        , description(desc)
        , usage(use)
        , requiresWallet(wallet) {}
};

/**
 * @brief RPC server configuration
 */
struct RPCServerConfig {
    std::string bindAddress;
    uint16_t port;
    std::string rpcUser;
    std::string rpcPassword;
    bool allowFromAll;

    RPCServerConfig()
        : bindAddress("127.0.0.1")
        , port(9334)
        , rpcUser("dinariuser")
        , rpcPassword("")
        , allowFromAll(false) {}
};

/**
 * @brief JSON-RPC 2.0 server
 *
 * Implements Bitcoin-compatible JSON-RPC server:
 * - HTTP basic authentication
 * - JSON-RPC 2.0 protocol
 * - Blockchain, wallet, and network commands
 * - Thread-safe operation
 */
class RPCServer {
public:
    RPCServer(Blockchain& chain, Wallet* wallet, NetworkNode* node);
    ~RPCServer();

    /**
     * @brief Initialize server
     */
    bool Initialize(const RPCServerConfig& config);

    /**
     * @brief Start server
     */
    bool Start();

    /**
     * @brief Stop server
     */
    void Stop();

    /**
     * @brief Check if running
     */
    bool IsRunning() const { return running.load(); }

    /**
     * @brief Register RPC command
     */
    void RegisterCommand(const RPCCommand& command);

    /**
     * @brief Execute RPC command
     */
    RPCResponse ExecuteCommand(const RPCRequest& request);

    /**
     * @brief Get available commands
     */
    std::vector<RPCCommand> GetCommands() const;

private:
    Blockchain& blockchain;
    Wallet* wallet;
    NetworkNode* networkNode;

    RPCServerConfig config;

    // Command registry
    std::map<std::string, RPCCommand> commands;
    mutable std::mutex commandsMutex;

    // Server state
    std::atomic<bool> running;
    std::atomic<bool> shouldStop;
    std::thread serverThread;

    // Initialize command registry
    void RegisterDefaultCommands();

    // Server thread function
    void ServerThreadFunc();

    // Handle HTTP request
    std::string HandleHTTPRequest(const std::string& request);

    // Authenticate request
    bool Authenticate(const std::string& authHeader);

    // Helper to create error response
    static RPCResponse CreateErrorResponse(const JSONValue& id,
                                          int code,
                                          const std::string& message);
};

/**
 * @brief RPC helper functions
 */
class RPCHelper {
public:
    /**
     * @brief Check parameter count
     */
    static void CheckParams(const RPCRequest& request, size_t expected);
    static void CheckParamsAtLeast(const RPCRequest& request, size_t minimum);
    static void CheckParamsRange(const RPCRequest& request, size_t min, size_t max);

    /**
     * @brief Get parameter as specific type
     */
    static std::string GetStringParam(const RPCRequest& request, size_t index);
    static int64_t GetIntParam(const RPCRequest& request, size_t index);
    static bool GetBoolParam(const RPCRequest& request, size_t index);
    static double GetDoubleParam(const RPCRequest& request, size_t index);

    /**
     * @brief Convert types to JSON
     */
    static JSONObject BlockToJSON(const Block& block, const Blockchain& blockchain);
    static JSONObject TransactionToJSON(const Transaction& tx);
    static JSONObject AddressInfoToJSON(const Address& addr, const Wallet& wallet);

    /**
     * @brief Throw RPC error
     */
    [[noreturn]] static void ThrowError(int code, const std::string& message);
};

} // namespace dinari

#endif // DINARI_RPC_RPCSERVER_H
