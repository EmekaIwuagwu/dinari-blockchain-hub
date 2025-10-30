#include "rpcserver.h"
#include "util/logger.h"
#include "util/serialize.h"
#include "util/security.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <chrono>

namespace dinari {

// JSONValue implementation

JSONValue::JSONValue()
    : type(JSONType::Null)
    , boolValue(false)
    , intValue(0)
    , doubleValue(0.0) {
}

JSONValue::JSONValue(bool b)
    : type(JSONType::Bool)
    , boolValue(b)
    , intValue(0)
    , doubleValue(0.0) {
}

JSONValue::JSONValue(int64_t n)
    : type(JSONType::Number)
    , boolValue(false)
    , intValue(n)
    , doubleValue(static_cast<double>(n)) {
}

JSONValue::JSONValue(double d)
    : type(JSONType::Number)
    , boolValue(false)
    , intValue(static_cast<int64_t>(d))
    , doubleValue(d) {
}

JSONValue::JSONValue(const std::string& s)
    : type(JSONType::String)
    , boolValue(false)
    , intValue(0)
    , doubleValue(0.0)
    , stringValue(s) {
}

JSONValue::JSONValue(const char* s)
    : type(JSONType::String)
    , boolValue(false)
    , intValue(0)
    , doubleValue(0.0)
    , stringValue(s) {
}

bool JSONValue::GetBool() const {
    return boolValue;
}

int64_t JSONValue::GetInt() const {
    return intValue;
}

double JSONValue::GetDouble() const {
    return doubleValue;
}

std::string JSONValue::GetString() const {
    return stringValue;
}

std::string JSONValue::Serialize() const {
    std::ostringstream oss;

    switch (type) {
        case JSONType::Null:
            oss << "null";
            break;
        case JSONType::Bool:
            oss << (boolValue ? "true" : "false");
            break;
        case JSONType::Number:
            if (doubleValue == static_cast<double>(intValue)) {
                oss << intValue;
            } else {
                oss << doubleValue;
            }
            break;
        case JSONType::String:
            oss << "\"";
            // Escape special characters
            for (char c : stringValue) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            oss << "\"";
            break;
        default:
            oss << "null";
            break;
    }

    return oss.str();
}

// JSONObject implementation

void JSONObject::Set(const std::string& key, const JSONValue& value) {
    data[key] = value;
}

void JSONObject::SetNull(const std::string& key) {
    data[key] = JSONValue();
}

void JSONObject::SetBool(const std::string& key, bool value) {
    data[key] = JSONValue(value);
}

void JSONObject::SetInt(const std::string& key, int64_t value) {
    data[key] = JSONValue(value);
}

void JSONObject::SetDouble(const std::string& key, double value) {
    data[key] = JSONValue(value);
}

void JSONObject::SetString(const std::string& key, const std::string& value) {
    data[key] = JSONValue(value);
}

bool JSONObject::Has(const std::string& key) const {
    return data.count(key) > 0;
}

JSONValue JSONObject::Get(const std::string& key) const {
    auto it = data.find(key);
    if (it == data.end()) {
        return JSONValue();
    }
    return it->second;
}

std::string JSONObject::Serialize() const {
    std::ostringstream oss;
    oss << "{";

    bool first = true;
    for (const auto& pair : data) {
        if (!first) oss << ",";
        first = false;

        oss << "\"" << pair.first << "\":" << pair.second.Serialize();
    }

    oss << "}";
    return oss.str();
}

// RPCRequest implementation

RPCRequest RPCRequest::Parse(const std::string& json) {
    // Simplified JSON parsing - production code should use a proper JSON library
    RPCRequest request;
    request.jsonrpc = "2.0";

    // Extract method (very simplified)
    size_t methodPos = json.find("\"method\"");
    if (methodPos != std::string::npos) {
        size_t colonPos = json.find(":", methodPos);
        size_t quoteStart = json.find("\"", colonPos);
        size_t quoteEnd = json.find("\"", quoteStart + 1);
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            request.method = json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
    }

    // Extract id (simplified)
    size_t idPos = json.find("\"id\"");
    if (idPos != std::string::npos) {
        size_t colonPos = json.find(":", idPos);
        size_t numStart = colonPos + 1;
        while (numStart < json.length() && std::isspace(json[numStart])) numStart++;

        if (json[numStart] == '"') {
            // String ID
            size_t quoteEnd = json.find("\"", numStart + 1);
            request.id = JSONValue(json.substr(numStart + 1, quoteEnd - numStart - 1));
        } else if (std::isdigit(json[numStart]) || json[numStart] == '-') {
            // Numeric ID
            size_t numEnd = numStart;
            while (numEnd < json.length() && (std::isdigit(json[numEnd]) || json[numEnd] == '-' || json[numEnd] == '.')) {
                numEnd++;
            }
            int64_t id = std::stoll(json.substr(numStart, numEnd - numStart));
            request.id = JSONValue(id);
        }
    }

    return request;
}

// RPCResponse implementation

std::string RPCResponse::Serialize() const {
    JSONObject obj;
    obj.SetString("jsonrpc", jsonrpc);

    if (isError) {
        obj.SetObject("error", error);
        obj.SetNull("result");
    } else {
        obj.Set("result", result);
        obj.SetNull("error");
    }

    obj.Set("id", id);

    return obj.Serialize();
}

// RPCServer implementation

RPCServer::RPCServer(Blockchain& chain, Wallet* w, NetworkNode* node)
    : blockchain(chain)
    , wallet(w)
    , networkNode(node)
    , running(false)
    , shouldStop(false)
    , failedAuthAttempts(0) {
}

RPCServer::~RPCServer() {
    Stop();
}

bool RPCServer::Initialize(const RPCServerConfig& cfg) {
    config = cfg;

    LOG_INFO("RPC", "Initializing RPC server on " + config.bindAddress +
             ":" + std::to_string(config.port));

    // Register default commands
    RegisterDefaultCommands();

    return true;
}

bool RPCServer::Start() {
    if (running.load()) {
        return true;
    }

    LOG_INFO("RPC", "Starting RPC server");

    shouldStop.store(false);

    // Start server thread
    serverThread = std::thread(&RPCServer::ServerThreadFunc, this);

    running.store(true);

    LOG_INFO("RPC", "RPC server started");

    return true;
}

void RPCServer::Stop() {
    if (!running.load()) {
        return;
    }

    LOG_INFO("RPC", "Stopping RPC server");

    shouldStop.store(true);
    running.store(false);

    if (serverThread.joinable()) {
        serverThread.join();
    }

    LOG_INFO("RPC", "RPC server stopped");
}

void RPCServer::RegisterCommand(const RPCCommand& command) {
    std::lock_guard<std::mutex> lock(commandsMutex);

    commands[command.name] = command;

    LOG_DEBUG("RPC", "Registered command: " + command.name);
}

RPCResponse RPCServer::ExecuteCommand(const RPCRequest& request) {
    RPCResponse response;
    response.id = request.id;

    try {
        // Find command
        std::lock_guard<std::mutex> lock(commandsMutex);

        auto it = commands.find(request.method);
        if (it == commands.end()) {
            return CreateErrorResponse(request.id, RPC_METHOD_NOT_FOUND,
                                      "Method not found: " + request.method);
        }

        const RPCCommand& command = it->second;

        // Check wallet requirement
        if (command.requiresWallet && !wallet) {
            return CreateErrorResponse(request.id, RPC_WALLET_ERROR,
                                      "Wallet not loaded");
        }

        // Execute command
        response.result = command.handler(request, blockchain, wallet, networkNode);
        response.isError = false;

        LOG_DEBUG("RPC", "Executed command: " + request.method);

    } catch (const std::exception& e) {
        LOG_ERROR("RPC", "Command execution error: " + std::string(e.what()));
        return CreateErrorResponse(request.id, RPC_INTERNAL_ERROR, e.what());
    }

    return response;
}

std::vector<RPCCommand> RPCServer::GetCommands() const {
    std::lock_guard<std::mutex> lock(commandsMutex);

    std::vector<RPCCommand> result;
    result.reserve(commands.size());

    for (const auto& pair : commands) {
        result.push_back(pair.second);
    }

    return result;
}

void RPCServer::RegisterDefaultCommands() {
    // This is a placeholder - actual command implementations are in other files
    LOG_INFO("RPC", "Registered " + std::to_string(commands.size()) + " RPC commands");
}

void RPCServer::ServerThreadFunc() {
    LOG_INFO("RPC", "RPC server thread started");

    // Simplified server loop - production code should use a proper HTTP server library (e.g., cpp-httplib)
    // In production, this would listen on a socket and handle HTTP requests
    // For now, this serves as a placeholder for the server infrastructure
    while (!shouldStop.load()) {
        // Production implementation would:
        // 1. Accept incoming HTTP connections
        // 2. Parse HTTP requests
        // 3. Call HandleHTTPRequest()
        // 4. Send HTTP responses back to clients
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    LOG_INFO("RPC", "RPC server thread stopped");
}

std::string RPCServer::HandleHTTPRequest(const std::string& request, const std::string& clientIP) {
    // Extract HTTP headers
    size_t headerEnd = request.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }

    std::string headers = request.substr(0, headerEnd);
    std::string body = request.substr(headerEnd + 4);

    // Check authentication
    size_t authPos = headers.find("Authorization:");
    if (authPos != std::string::npos) {
        size_t lineEnd = headers.find("\r\n", authPos);
        std::string authHeader = headers.substr(authPos + 15, lineEnd - authPos - 15);

        if (!Authenticate(authHeader, clientIP)) {
            return "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"dinari-rpc\"\r\n\r\n";
        }
    } else if (!config.rpcPassword.empty()) {
        return "HTTP/1.1 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"dinari-rpc\"\r\n\r\n";
    }

    // Parse JSON-RPC request
    RPCRequest rpcRequest = RPCRequest::Parse(body);

    // Execute command
    RPCResponse rpcResponse = ExecuteCommand(rpcRequest);

    // Build HTTP response
    std::string responseBody = rpcResponse.Serialize();

    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Type: application/json\r\n";
    oss << "Content-Length: " << responseBody.length() << "\r\n";
    oss << "\r\n";
    oss << responseBody;

    return oss.str();
}

bool RPCServer::Authenticate(const std::string& authHeader, const std::string& clientIP) {
    // Check if IP is banned
    if (rateLimiter.IsBanned(clientIP)) {
        LOG_WARNING("RPC", "Rejected request from banned IP: " + clientIP);
        return false;
    }

    // Rate limiting: 10 requests per 60 seconds
    if (!rateLimiter.CheckLimit(clientIP, 10, 60)) {
        failedAuthAttempts++;
        if (failedAuthAttempts.load() > 50) {
            rateLimiter.Ban(clientIP, 3600);  // Ban for 1 hour
            LOG_WARNING("RPC", "Banned IP due to excessive requests: " + clientIP);
        }
        return false;
    }

    // Trim whitespace from auth header
    std::string auth = authHeader;
    size_t start = auth.find_first_not_of(" \t");
    size_t end = auth.find_last_not_of(" \t\r\n");
    if (start != std::string::npos && end != std::string::npos) {
        auth = auth.substr(start, end - start + 1);
    }

    // Check for "Basic " prefix
    if (auth.size() < 6 || auth.substr(0, 6) != "Basic ") {
        LOG_WARNING("RPC", "Invalid auth header format from " + clientIP);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return false;
    }

    // Decode Base64 credentials
    std::string base64Credentials = auth.substr(6);
    std::string credentials;

    try {
        credentials = Security::Base64Decode(base64Credentials);
    } catch (const std::exception& e) {
        LOG_WARNING("RPC", "Failed to decode auth credentials from " + clientIP);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return false;
    }

    // Split into username:password
    size_t colonPos = credentials.find(':');
    if (colonPos == std::string::npos) {
        LOG_WARNING("RPC", "Invalid credentials format from " + clientIP);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return false;
    }

    std::string username = credentials.substr(0, colonPos);
    std::string password = credentials.substr(colonPos + 1);

    // Use constant-time comparison to prevent timing attacks
    bool usernameMatch = Security::ConstantTimeCompare(username, config.rpcUser);
    bool passwordMatch = Security::ConstantTimeCompare(password, config.rpcPassword);

    if (!usernameMatch || !passwordMatch) {
        failedAuthAttempts++;
        LOG_WARNING("RPC", "Authentication failed for " + clientIP +
                          " (attempt #" + std::to_string(failedAuthAttempts.load()) + ")");

        // Slow down brute force attacks
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Ban after many failed attempts
        if (failedAuthAttempts.load() > 10) {
            rateLimiter.Ban(clientIP, 3600);
            LOG_WARNING("RPC", "Banned IP due to failed authentication: " + clientIP);
        }

        return false;
    }

    // Authentication successful
    LOG_DEBUG("RPC", "Authentication successful for " + clientIP);
    return true;
}

RPCResponse RPCServer::CreateErrorResponse(const JSONValue& id,
                                          int code,
                                          const std::string& message) {
    RPCResponse response;
    response.id = id;
    response.isError = true;

    response.error.SetInt("code", code);
    response.error.SetString("message", message);

    return response;
}

// RPCHelper implementation

void RPCHelper::CheckParams(const RPCRequest& request, size_t expected) {
    if (request.params.size() != expected) {
        ThrowError(RPC_INVALID_PARAMS,
                  "Expected " + std::to_string(expected) + " parameters, got " +
                  std::to_string(request.params.size()));
    }
}

void RPCHelper::CheckParamsAtLeast(const RPCRequest& request, size_t minimum) {
    if (request.params.size() < minimum) {
        ThrowError(RPC_INVALID_PARAMS,
                  "Expected at least " + std::to_string(minimum) + " parameters, got " +
                  std::to_string(request.params.size()));
    }
}

void RPCHelper::CheckParamsRange(const RPCRequest& request, size_t min, size_t max) {
    if (request.params.size() < min || request.params.size() > max) {
        ThrowError(RPC_INVALID_PARAMS,
                  "Expected " + std::to_string(min) + "-" + std::to_string(max) +
                  " parameters, got " + std::to_string(request.params.size()));
    }
}

std::string RPCHelper::GetStringParam(const RPCRequest& request, size_t index) {
    if (index >= request.params.size()) {
        ThrowError(RPC_INVALID_PARAMS, "Parameter index out of range");
    }

    if (!request.params[index].IsString()) {
        ThrowError(RPC_TYPE_ERROR, "Parameter " + std::to_string(index) + " must be a string");
    }

    return request.params[index].GetString();
}

int64_t RPCHelper::GetIntParam(const RPCRequest& request, size_t index) {
    if (index >= request.params.size()) {
        ThrowError(RPC_INVALID_PARAMS, "Parameter index out of range");
    }

    if (!request.params[index].IsNumber()) {
        ThrowError(RPC_TYPE_ERROR, "Parameter " + std::to_string(index) + " must be a number");
    }

    return request.params[index].GetInt();
}

bool RPCHelper::GetBoolParam(const RPCRequest& request, size_t index) {
    if (index >= request.params.size()) {
        ThrowError(RPC_INVALID_PARAMS, "Parameter index out of range");
    }

    if (!request.params[index].IsBool()) {
        ThrowError(RPC_TYPE_ERROR, "Parameter " + std::to_string(index) + " must be a boolean");
    }

    return request.params[index].GetBool();
}

double RPCHelper::GetDoubleParam(const RPCRequest& request, size_t index) {
    if (index >= request.params.size()) {
        ThrowError(RPC_INVALID_PARAMS, "Parameter index out of range");
    }

    if (!request.params[index].IsNumber()) {
        ThrowError(RPC_TYPE_ERROR, "Parameter " + std::to_string(index) + " must be a number");
    }

    return request.params[index].GetDouble();
}

JSONObject RPCHelper::BlockToJSON(const Block& block, const Blockchain& blockchain) {
    JSONObject obj;

    obj.SetString("hash", block.GetHash().ToHex());
    obj.SetInt("height", 0);  // Note: Block height lookup requires blockchain index
    obj.SetInt("version", block.header.version);
    obj.SetString("previousblockhash", block.header.prevBlockHash.ToHex());
    obj.SetString("merkleroot", block.header.merkleRoot.ToHex());
    obj.SetInt("time", block.header.timestamp);
    obj.SetInt("nonce", block.header.nonce);
    obj.SetInt("bits", block.header.bits);
    obj.SetInt("ntx", block.transactions.size());

    return obj;
}

JSONObject RPCHelper::TransactionToJSON(const Transaction& tx) {
    JSONObject obj;

    obj.SetString("txid", tx.GetHash().ToHex());
    obj.SetInt("version", tx.version);
    obj.SetInt("locktime", tx.lockTime);
    obj.SetInt("vin_count", tx.vin.size());
    obj.SetInt("vout_count", tx.vout.size());

    return obj;
}

JSONObject RPCHelper::AddressInfoToJSON(const Address& addr, const Wallet& wallet) {
    JSONObject obj;

    obj.SetString("address", addr.ToString());
    obj.SetBool("ismine", wallet.IsMine(addr));

    return obj;
}

[[noreturn]] void RPCHelper::ThrowError(int code, const std::string& message) {
    // Create error and throw as exception
    // The exception will be caught by ExecuteCommand
    throw std::runtime_error("RPC Error " + std::to_string(code) + ": " + message);
}

} // namespace dinari
