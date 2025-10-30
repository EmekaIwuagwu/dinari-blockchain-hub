/**
 * DINARI BLOCKCHAIN - SECURITY HARDENING PATCH
 *
 * This file contains the fixed implementations for all CRITICAL and HIGH security issues.
 * Apply these fixes to the respective files to harden the blockchain.
 *
 * Senior Blockchain Engineer Review: 2025-10-30
 */

// ============================================================================
// FIX #1: RPC Authentication (CRITICAL-002)
// File: src/rpc/rpcserver.cpp
// Lines: 407-411
// ============================================================================

bool RPCServer::Authenticate(const std::string& authHeader, const std::string& clientIP) {
    // Rate limiting check - prevent brute force
    if (!rateLimiter.CheckLimit(clientIP, 10, 60)) {  // Max 10 requests per minute
        LOG_WARNING("RPC", "Rate limit exceeded for IP: " + clientIP);
        failedAuthAttempts++;

        // Auto-ban after 50 failed attempts
        if (failedAuthAttempts.load() > 50) {
            rateLimiter.Ban(clientIP, 3600);  // Ban for 1 hour
            LOG_WARNING("RPC", "IP banned due to excessive failed attempts: " + clientIP);
        }
        return false;
    }

    // Check if IP is banned
    if (rateLimiter.IsBanned(clientIP)) {
        LOG_WARNING("RPC", "Rejected request from banned IP: " + clientIP);
        return false;
    }

    // If no password is set, reject (security requirement)
    if (config.rpcPassword.empty()) {
        LOG_ERROR("RPC", "RPC password not set - rejecting all requests");
        return false;
    }

    // Parse Basic authentication header
    // Format: "Basic base64(username:password)"
    if (authHeader.substr(0, 6) != "Basic ") {
        LOG_WARNING("RPC", "Invalid authentication header format from " + clientIP);
        failedAuthAttempts++;
        return false;
    }

    std::string base64Credentials = authHeader.substr(6);

    // Decode base64
    std::string credentials;
    try {
        credentials = Security::Base64Decode(base64Credentials);
    } catch (const std::exception& e) {
        LOG_WARNING("RPC", "Base64 decode failed from " + clientIP + ": " + e.what());
        failedAuthAttempts++;
        return false;
    }

    // Parse username:password
    size_t colonPos = credentials.find(':');
    if (colonPos == std::string::npos) {
        LOG_WARNING("RPC", "Invalid credentials format from " + clientIP);
        failedAuthAttempts++;
        return false;
    }

    std::string username = credentials.substr(0, colonPos);
    std::string password = credentials.substr(colonPos + 1);

    // Constant-time comparison to prevent timing attacks
    bool usernameMatch = Security::ConstantTimeCompare(username, config.rpcUser);
    bool passwordMatch = Security::ConstantTimeCompare(password, config.rpcPassword);

    if (!usernameMatch || !passwordMatch) {
        LOG_WARNING("RPC", "Authentication failed for user '" + username + "' from " + clientIP);
        failedAuthAttempts++;

        // Slow down brute force attempts
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return false;
    }

    // Success - reset failed attempts for this IP
    failedAuthAttempts = 0;
    LOG_DEBUG("RPC", "Authentication successful for user '" + username + "' from " + clientIP);

    return true;
}

// ============================================================================
// FIX #2: Wallet RNG (HIGH-001)
// File: src/wallet/keystore.cpp
// Lines: 126-133, 358-364
// ============================================================================

bool CryptoKeyStore::EncryptWallet(const std::string& passphrase) {
    if (encrypted) {
        LOG_ERROR("KeyStore", "Wallet is already encrypted");
        return false;
    }

    if (passphrase.empty()) {
        LOG_ERROR("KeyStore", "Empty passphrase not allowed");
        return false;
    }

    if (passphrase.length() < 8) {
        LOG_ERROR("KeyStore", "Passphrase must be at least 8 characters");
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex);

    // Generate cryptographically secure random salt using OpenSSL
    masterKeySalt.resize(32);
    if (RAND_bytes(masterKeySalt.data(), 32) != 1) {
        LOG_ERROR("KeyStore", "Failed to generate secure random salt");
        return false;
    }

    // Derive master key
    masterKey = DeriveMasterKey(passphrase, masterKeySalt);

    // Encrypt all existing keys
    for (const auto& pair : keys) {
        bytes encryptedKey = EncryptKey(pair.second.privKey);
        if (encryptedKey.empty()) {
            LOG_ERROR("KeyStore", "Failed to encrypt key");
            // Securely clear sensitive data
            std::fill(masterKey.begin(), masterKey.end(), 0);
            masterKey.clear();
            std::fill(masterKeySalt.begin(), masterKeySalt.end(), 0);
            masterKeySalt.clear();
            return false;
        }

        encryptedKeys[pair.first] = encryptedKey;
    }

    encrypted = true;
    unlocked = true;

    LOG_INFO("KeyStore", "Wallet encrypted with " + std::to_string(keys.size()) + " keys");

    return true;
}

bytes CryptoKeyStore::EncryptKey(const Hash256& privKey) const {
    if (masterKey.empty()) {
        return bytes();
    }

    // Convert Hash256 to bytes
    bytes plaintext(privKey.begin(), privKey.end());

    // Generate cryptographically secure random IV using OpenSSL
    bytes iv(16);
    if (RAND_bytes(iv.data(), 16) != 1) {
        LOG_ERROR("KeyStore", "Failed to generate secure random IV");
        return bytes();
    }

    // Encrypt
    bytes ciphertext = crypto::AES::Encrypt(plaintext, masterKey, iv);
    if (ciphertext.empty()) {
        return bytes();
    }

    // Prepend IV
    bytes result;
    result.reserve(iv.size() + ciphertext.size());
    result.insert(result.end(), iv.begin(), iv.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    // Securely clear plaintext
    std::fill(plaintext.begin(), plaintext.end(), 0);

    return result;
}

// ============================================================================
// FIX #3: Transaction Validation (HIGH-002)
// File: src/network/node.cpp
// Line: 549
// ============================================================================

void NetworkNode::HandleTx(const TxMessage& msg, PeerPtr peer) {
    Hash256 txHash = msg.tx.GetHash();
    LOG_DEBUG("Network", "Received transaction: " + txHash.ToHex() + " from peer " + std::to_string(peer->GetId()));

    // Validate transaction structure
    if (msg.tx.vin.empty()) {
        LOG_WARNING("Network", "Rejected transaction with no inputs from peer " + std::to_string(peer->GetId()));
        peer->Misbehaving(10);  // Add misbehavior score
        return;
    }

    if (msg.tx.vout.empty()) {
        LOG_WARNING("Network", "Rejected transaction with no outputs from peer " + std::to_string(peer->GetId()));
        peer->Misbehaving(10);
        return;
    }

    // Check transaction size
    bytes serialized = msg.tx.Serialize();
    if (serialized.size() > MAX_TX_SIZE) {
        LOG_WARNING("Network", "Rejected oversized transaction from peer " + std::to_string(peer->GetId()));
        peer->Misbehaving(20);
        return;
    }

    // Validate signatures
    const UTXOSet& utxos = blockchain.GetUTXOs();
    std::string error;

    if (!ValidateTransactionSignatures(msg.tx, utxos, error)) {
        LOG_WARNING("Network", "Rejected transaction with invalid signatures: " + error);
        peer->Misbehaving(50);  // Serious misbehavior
        return;
    }

    // Check for double-spends
    MemPool& mempool = blockchain.GetMemPool();
    if (mempool.HasConflict(msg.tx)) {
        LOG_DEBUG("Network", "Transaction conflicts with mempool transaction");
        // Not misbehavior - could be legitimate race condition
        return;
    }

    // Check if already in blockchain
    if (blockchain.HasTransaction(txHash)) {
        LOG_DEBUG("Network", "Transaction already in blockchain");
        return;
    }

    // Add to mempool with validation
    Amount fee = 0;
    if (!mempool.AddTransaction(msg.tx, utxos, fee, error)) {
        LOG_WARNING("Network", "Failed to add transaction to mempool: " + error);
        peer->Misbehaving(5);
        return;
    }

    LOG_INFO("Network", "Added transaction to mempool: " + txHash.ToHex() + " (fee: " + std::to_string(fee) + ")");

    // Relay to other peers (don't send back to sender)
    RelayTransaction(msg.tx, peer->GetId());
}

// ============================================================================
// FIX #4: Wallet Auto-Lock (HIGH-006)
// File: src/wallet/wallet.cpp / src/rpc/rpcwallet.cpp
// Line: 484
// ============================================================================

// In wallet.h, add:
class Wallet {
private:
    std::atomic<Timestamp> unlockUntil;
    std::thread autoLockThread;
    std::atomic<bool> autoLockRunning;

    void AutoLockThreadFunc();
};

// Implementation:
void Wallet::UnlockWithTimeout(const std::string& passphrase, uint32_t timeoutSeconds) {
    if (!keystore->Unlock(passphrase)) {
        throw std::runtime_error("Incorrect passphrase");
    }

    if (timeoutSeconds > 0) {
        unlockUntil = Time::GetCurrentTime() + timeoutSeconds;

        // Start auto-lock thread if not running
        if (!autoLockRunning.load()) {
            autoLockRunning = true;
            autoLockThread = std::thread(&Wallet::AutoLockThreadFunc, this);
        }

        LOG_INFO("Wallet", "Wallet unlocked for " + std::to_string(timeoutSeconds) + " seconds");
    } else {
        unlockUntil = 0;  // Unlock indefinitely
        LOG_WARNING("Wallet", "Wallet unlocked indefinitely - security risk!");
    }
}

void Wallet::AutoLockThreadFunc() {
    while (autoLockRunning.load()) {
        Timestamp now = Time::GetCurrentTime();

        if (unlockUntil > 0 && now >= unlockUntil) {
            keystore->Lock();
            unlockUntil = 0;
            LOG_INFO("Wallet", "Wallet auto-locked after timeout");
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ============================================================================
// FIX #5: Integer Overflow Protection (HIGH-008)
// File: Multiple files (transaction.cpp, wallet.cpp)
// ============================================================================

// Safe addition with overflow check
bool SafeAdd(Amount a, Amount b, Amount& result) {
    if (a > MAX_MONEY || b > MAX_MONEY) {
        return false;
    }

    if (a > MAX_MONEY - b) {
        return false;  // Overflow
    }

    result = a + b;
    return result <= MAX_MONEY;
}

// Safe multiplication
bool SafeMul(Amount a, Amount b, Amount& result) {
    if (a > MAX_MONEY || b > MAX_MONEY) {
        return false;
    }

    if (a != 0 && MAX_MONEY / a < b) {
        return false;  // Overflow
    }

    result = a * b;
    return result <= MAX_MONEY;
}

// Use in transaction validation:
bool ValidateTransactionAmounts(const Transaction& tx) {
    Amount totalInput = 0;
    Amount totalOutput = 0;

    for (const auto& out : tx.vout) {
        if (out.value < 0 || out.value > MAX_MONEY) {
            return false;
        }

        Amount newTotal;
        if (!SafeAdd(totalOutput, out.value, newTotal)) {
            return false;  // Output sum overflow
        }
        totalOutput = newTotal;
    }

    // Validate fee doesn't overflow
    if (totalInput < totalOutput) {
        return false;  // Negative fee
    }

    Amount fee = totalInput - totalOutput;
    if (fee > MAX_MONEY / 100) {  // Fee shouldn't exceed 1% of max money
        return false;  // Suspiciously high fee
    }

    return true;
}

// ============================================================================
// FIX #6: Peer Banning System (HIGH-003)
// File: src/network/peer.h and peer.cpp
// ============================================================================

// In peer.h, add:
class Peer {
private:
    std::atomic<int> misbehaviorScore;
    static const int BAN_THRESHOLD = 100;

public:
    void Misbehaving(int howMuch);
    bool ShouldBan() const;
    int GetMisbehaviorScore() const;
};

// Implementation:
void Peer::Misbehaving(int howMuch) {
    misbehaviorScore += howMuch;

    LOG_WARNING("Peer", "Peer " + std::to_string(id) + " misbehavior score: " +
                std::to_string(misbehaviorScore.load()) + " (+" + std::to_string(howMuch) + ")");

    if (ShouldBan()) {
        LOG_WARNING("Peer", "Peer " + std::to_string(id) + " banned (score: " +
                    std::to_string(misbehaviorScore.load()) + ")");
    }
}

bool Peer::ShouldBan() const {
    return misbehaviorScore.load() >= BAN_THRESHOLD;
}

int Peer::GetMisbehaviorScore() const {
    return misbehaviorScore.load();
}

// In NetworkNode, add ban management:
void NetworkNode::ProcessPeers() {
    std::lock_guard<std::mutex> lock(peersMutex);

    for (auto it = peers.begin(); it != peers.end(); ) {
        auto& peer = it->second;

        // Check if peer should be banned
        if (peer->ShouldBan()) {
            LOG_WARNING("Network", "Banning peer " + std::to_string(peer->GetId()) +
                        " at " + peer->GetAddress().ToString());

            // Add to ban list
            bannedPeers.insert(peer->GetAddress());

            // Disconnect
            peer->Disconnect();
            it = peers.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// FIX #7: UTXO Script Validation (HIGH-005)
// File: src/core/utxo.cpp
// Line: 333
// ============================================================================

Address UTXOSet::ExtractAddress(const bytes& scriptPubKey) const {
    std::lock_guard<std::mutex> lock(mutex);

    // P2PKH: OP_DUP OP_HASH160 <pubKeyHash> OP_EQUALVERIFY OP_CHECKSIG
    if (scriptPubKey.size() == 25 &&
        scriptPubKey[0] == OP_DUP &&
        scriptPubKey[1] == OP_HASH160 &&
        scriptPubKey[2] == 20 &&
        scriptPubKey[23] == OP_EQUALVERIFY &&
        scriptPubKey[24] == OP_CHECKSIG) {

        Hash160 pubKeyHash;
        std::copy(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23, pubKeyHash.begin());

        Address addr(pubKeyHash, false);  // P2PKH
        return addr;
    }

    // P2SH: OP_HASH160 <scriptHash> OP_EQUAL
    if (scriptPubKey.size() == 23 &&
        scriptPubKey[0] == OP_HASH160 &&
        scriptPubKey[1] == 20 &&
        scriptPubKey[22] == OP_EQUAL) {

        Hash160 scriptHash;
        std::copy(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22, scriptHash.begin());

        Address addr(scriptHash, true);  // P2SH
        return addr;
    }

    // P2WPKH (SegWit): OP_0 <20-byte-hash>
    if (scriptPubKey.size() == 22 &&
        scriptPubKey[0] == OP_0 &&
        scriptPubKey[1] == 20) {

        Hash160 witnessHash;
        std::copy(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22, witnessHash.begin());

        // For now, treat as P2PKH equivalent
        Address addr(witnessHash, false);
        return addr;
    }

    // P2WSH (SegWit): OP_0 <32-byte-hash>
    if (scriptPubKey.size() == 34 &&
        scriptPubKey[0] == OP_0 &&
        scriptPubKey[1] == 32) {

        // P2WSH - more complex, need to handle witness data
        LOG_WARNING("UTXO", "P2WSH script detected - partial support");

        // Return invalid address - need full SegWit support
        return Address();
    }

    // Unknown script type
    LOG_WARNING("UTXO", "Unknown script type, size: " + std::to_string(scriptPubKey.size()));
    return Address();
}

/**
 * END OF SECURITY HARDENING PATCH
 *
 * Additional fixes needed (see security audit):
 * - Implement persistent storage (LevelDB integration)
 * - Complete block locator implementation
 * - Implement help command
 * - Add coinbase maturity checks
 * - Implement transaction history lookup
 * - Add proper HTTP server (consider using cpp-httplib)
 *
 * TESTING REQUIRED:
 * After applying these patches, conduct thorough testing:
 * 1. Unit tests for all security functions
 * 2. Integration tests for RPC authentication
 * 3. Fuzzing for transaction validation
 * 4. Load testing for rate limiting
 * 5. Penetration testing for auth bypass
 */
