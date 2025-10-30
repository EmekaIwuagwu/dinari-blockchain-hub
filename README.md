# Dinari Blockchain

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0--alpha-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![Security](https://img.shields.io/badge/security-hardened-success)

**A Production-Ready, Security-Hardened, Bitcoin-Style Proof-of-Work Blockchain**

[Features](#features) • [Build](#building) • [Usage](#usage) • [API](#api-documentation) • [Deployment](#deployment) • [Security](#security) • [Documentation](#documentation)

</div>

---

## Overview

**Dinari Blockchain** is a production-grade, Bitcoin-style Proof-of-Work blockchain system implemented entirely in C++17. Designed for real-world financial transactions, every component has been built with security, performance, and correctness as top priorities. The codebase has undergone comprehensive security hardening and is ready for deployment.

### Key Specifications

- **Blockchain Name:** Dinari Blockchain
- **Native Token:** Dinari (DNT)
- **Initial Supply:** 700 Trillion DNT (700,000,000,000,000)
- **Address Prefix:** `D` (e.g., D1a2b3c4...)
- **Consensus:** Proof of Work (PoW) - SHA-256
- **Block Time:** 10 minutes (600 seconds)
- **Difficulty Adjustment:** Every 2,016 blocks
- **Block Reward Halving:** Every 210,000 blocks
- **Max Block Size:** 2 MB
- **Initial Block Reward:** 50 DNT

---

## Features

### ✅ Implemented (Phase 1 - Foundation)

- **Cryptographic Primitives**
  - ✅ SHA-256 and double SHA-256 hashing
  - ✅ RIPEMD-160 for address generation
  - ✅ ECDSA (secp256k1) for signatures
  - ✅ AES-256-CBC for wallet encryption
  - ✅ Base58Check encoding with 'D' prefix
  - ✅ HMAC, PBKDF2, Scrypt key derivation

- **Core Infrastructure**
  - ✅ Binary serialization framework
  - ✅ Thread-safe logging system
  - ✅ Configuration management
  - ✅ Time utilities and timers
  - ✅ Type system with safety checks

- **Build System**
  - ✅ CMake build configuration
  - ✅ Cross-platform support (Linux, macOS, Windows)
  - ✅ Visual Studio 2022 support

### ✅ Implemented (Phase 2 - Core Blockchain)

- **Transaction System**
  - ✅ Complete UTXO transaction model (TxIn, TxOut, OutPoint)
  - ✅ Transaction validation and signature verification
  - ✅ Coinbase transaction support
  - ✅ Transaction fee calculation
  - ✅ Transaction builder with fluent API
  - ✅ Block reward with halving schedule (50 DNT initial)

- **UTXO Management**
  - ✅ Thread-safe UTXO set with address indexing
  - ✅ Coinbase maturity tracking (100 blocks)
  - ✅ UTXO validation for transactions
  - ✅ Coin selection algorithms (4 strategies)
  - ✅ UTXO cache for performance
  - ✅ Chain reorganization support
  - ✅ Multi-script type support (P2PKH, P2SH, P2WPKH, P2WSH)

- **Script System**
  - ✅ Stack-based script execution engine
  - ✅ Standard script types (P2PKH, P2SH, P2PK, Multisig)
  - ✅ OpCode implementation (DUP, HASH160, CHECKSIG, etc.)
  - ✅ Script verification and validation
  - ✅ Signature creation and verification
  - ✅ SegWit script support (P2WPKH, P2WSH)

- **Block Structure**
  - ✅ BlockHeader with PoW (nonce, bits, merkle root)
  - ✅ Block validation (size, transactions, merkle root)
  - ✅ Merkle tree implementation
  - ✅ Genesis block creation (700T DNT)
  - ✅ Block mining functionality
  - ✅ BlockIndex for chain management

### ✅ Implemented (Phase 3 - Consensus & Blockchain)

- **Difficulty Adjustment**
  - ✅ Bitcoin-style adjustment every 2,016 blocks
  - ✅ Maintains 10-minute block time
  - ✅ Limits adjustments to 4x (prevents manipulation)
  - ✅ Timespan calculation and validation
  - ✅ Testnet and mainnet support

- **Consensus Validation**
  - ✅ Comprehensive block validation rules
  - ✅ Transaction validation in context
  - ✅ Coinbase validation (reward limits)
  - ✅ Block size and sigop limits (2MB, 20K)
  - ✅ Timestamp validation
  - ✅ Money supply enforcement (700T DNT)
  - ✅ UTXO-based input validation

- **MemPool (Transaction Pool)**
  - ✅ Thread-safe transaction storage
  - ✅ Priority-based selection (fee rate)
  - ✅ Double-spend conflict detection
  - ✅ Auto-trimming when full (300MB max)
  - ✅ Mining template generation
  - ✅ Standard transaction enforcement
  - ✅ Mempool statistics

- **Blockchain Management**
  - ✅ Complete blockchain state management
  - ✅ Block acceptance and validation flow
  - ✅ Chain reorganization logic
  - ✅ Fork detection and resolution
  - ✅ Orphan block handling
  - ✅ Best chain selection (most work)
  - ✅ UTXO set integration
  - ✅ Height and hash indexing

### ✅ Implemented (Phase 4 - Networking)

- **P2P Protocol**
  - ✅ Complete Bitcoin-compatible protocol (version 70001)
  - ✅ Network message types (VERSION, VERACK, PING, PONG, INV, GETDATA, etc.)
  - ✅ Message serialization/deserialization with checksums
  - ✅ Protocol handshake (version exchange)
  - ✅ Keepalive mechanism (ping/pong)

- **Peer Management**
  - ✅ Connection lifecycle management
  - ✅ Inbound/outbound connection handling
  - ✅ Peer state machine (connecting, handshaking, active)
  - ✅ Connection statistics and monitoring
  - ✅ Automatic peer discovery
  - ✅ Connection limits (8 outbound, 125 inbound)
  - ✅ **Peer misbehavior scoring system with automatic banning**

- **Network Node**
  - ✅ Multi-threaded network I/O
  - ✅ Listen for incoming connections
  - ✅ Automatic peer connection management
  - ✅ Message routing and processing
  - ✅ Network statistics tracking
  - ✅ **Full transaction validation before relay**

- **Address Manager**
  - ✅ Peer address storage and management
  - ✅ DNS seed integration
  - ✅ Hardcoded seed peers
  - ✅ Address quality scoring
  - ✅ Connection retry logic with exponential backoff
  - ✅ Address persistence to disk
  - ✅ Ban management for misbehaving peers

- **Block & Transaction Propagation**
  - ✅ Inventory announcement (INV messages)
  - ✅ Block request/response (GETDATA/BLOCK)
  - ✅ Transaction relay with validation (TX messages)
  - ✅ Block header synchronization
  - ✅ Address sharing (ADDR messages)
  - ✅ Not found handling (NOTFOUND)

- **Network Infrastructure**
  - ✅ Cross-platform socket abstraction (Windows/Linux)
  - ✅ Non-blocking I/O
  - ✅ TCP socket operations
  - ✅ DNS resolution
  - ✅ IPv4 support (IPv6-ready structure)
  - ✅ Network address validation

### ✅ Implemented (Phase 5 - Wallet)

- **Key Management**
  - ✅ Encrypted key storage with AES-256-CBC
  - ✅ Key store interface (BasicKeyStore, CryptoKeyStore)
  - ✅ PBKDF2 key derivation (100,000 iterations)
  - ✅ Wallet encryption with passphrase
  - ✅ Lock/unlock functionality
  - ✅ **Auto-lock with configurable timeout**
  - ✅ Private key import/export
  - ✅ **Cryptographically secure RNG (OpenSSL RAND_bytes)**

- **HD Wallet (BIP32)**
  - ✅ Master key generation from seed
  - ✅ Hierarchical deterministic key derivation
  - ✅ Child key derivation (hardened and normal)
  - ✅ Extended key serialization (xprv/xpub)
  - ✅ Derivation path parsing (m/44'/0'/0'/0/0)
  - ✅ Public key derivation from extended keys

- **BIP39 Mnemonic**
  - ✅ Mnemonic generation from entropy (12/15/18/21/24 words)
  - ✅ Random mnemonic generation
  - ✅ Mnemonic to seed conversion (PBKDF2-HMAC-SHA512)
  - ✅ Mnemonic validation with checksum
  - ✅ Passphrase support

- **BIP44 Account Structure**
  - ✅ Standard derivation path (m/44'/0'/account'/change/index)
  - ✅ Account derivation
  - ✅ Change address management
  - ✅ Address index tracking

- **Address Management**
  - ✅ P2PKH address generation with 'D' prefix
  - ✅ P2SH address support
  - ✅ Address book with labels and metadata
  - ✅ Address validation and parsing
  - ✅ Receiving and change address separation
  - ✅ Address derivation tracking

- **Transaction Building**
  - ✅ Transaction builder with fluent API
  - ✅ Coin selection (largest-first strategy)
  - ✅ Automatic fee calculation
  - ✅ Change output handling
  - ✅ Transaction signing with private keys
  - ✅ Multi-input/output support
  - ✅ UTXO management

- **Wallet Core**
  - ✅ Comprehensive wallet class
  - ✅ Balance tracking (confirmed, unconfirmed, available)
  - ✅ Transaction creation and signing
  - ✅ UTXO tracking and management
  - ✅ Transaction history
  - ✅ Wallet persistence (save/load)
  - ✅ Thread-safe operations

### ✅ Implemented (Phase 6 - APIs)

- **JSON-RPC Server**
  - ✅ JSON-RPC 2.0 protocol implementation
  - ✅ HTTP server with Basic authentication
  - ✅ **Secure authentication with Base64 and constant-time comparison**
  - ✅ **Rate limiting (10 requests per 60 seconds)**
  - ✅ **IP banning for brute force protection**
  - ✅ Command registration and routing
  - ✅ Error handling with Bitcoin-compatible codes
  - ✅ Request/response serialization
  - ✅ Thread-safe operation

- **Blockchain RPC Commands**
  - ✅ `getblockcount` - Get current block height
  - ✅ `getblockhash` - Get block hash at height
  - ✅ `getblock` - Get block information
  - ✅ `getbestblockhash` - Get best block hash
  - ✅ `getdifficulty` - Get current difficulty
  - ✅ `getblockchaininfo` - Comprehensive blockchain info
  - ✅ `gettxout` - Get UTXO information
  - ✅ `getmempoolinfo` - Mempool statistics
  - ✅ `getrawmempool` - List mempool transactions

- **Blockchain Explorer RPC Commands** ⭐ NEW
  - ✅ `getrawtransaction` - Get transaction by hash with confirmations
  - ✅ `listblocks` - List blocks with height, hash, timestamp, transactions, and miner
  - ✅ Transaction lookup in mempool and blockchain
  - ✅ Confirmation count calculation
  - ✅ Miner address extraction from coinbase
  - ✅ Pagination support (max 100 blocks)

- **Wallet RPC Commands**
  - ✅ `getnewaddress` - Generate new receiving address
  - ✅ `getbalance` - Get wallet balance
  - ✅ `sendtoaddress` - Send DNT to address
  - ✅ `listaddresses` - List all wallet addresses
  - ✅ `listtransactions` - List transaction history
  - ✅ `listunspent` - List unspent outputs
  - ✅ `getwalletinfo` - Wallet state information
  - ✅ `encryptwallet` - Encrypt wallet with passphrase
  - ✅ `walletlock` - Lock encrypted wallet
  - ✅ `walletpassphrase` - **Unlock wallet with timeout (auto-lock)**
  - ✅ `walletpassphrasechange` - Change wallet passphrase
  - ✅ `importmnemonic` - Import HD wallet from mnemonic
  - ✅ `importprivkey` - Import private key
  - ✅ `validateaddress` - Validate Dinari address
  - ✅ `getaddressinfo` - Get address information

- **Command-Line Interface**
  - ✅ dinari-cli tool for RPC interaction
  - ✅ Command-line argument parsing
  - ✅ RPC connection configuration
  - ✅ Help system with usage examples
  - ✅ Support for all RPC commands

- **API Infrastructure**
  - ✅ Simplified JSON parsing/serialization
  - ✅ RPC helper functions for parameter validation
  - ✅ Type conversion utilities
  - ✅ Error response generation
  - ✅ **Secure authentication framework with rate limiting**

### ✅ Implemented (Phase 7 - Testing & Security)

- **Test Framework**
  - ✅ Custom lightweight test framework with macros
  - ✅ Assertion helpers (ASSERT_TRUE, ASSERT_FALSE, ASSERT_EQ)
  - ✅ Test registration system
  - ✅ Automatic test discovery

- **Cryptographic Tests**
  - ✅ SHA-256 hash function tests
  - ✅ Double SHA-256 verification
  - ✅ Hash160 (RIPEMD-160) tests
  - ✅ ECDSA key generation tests
  - ✅ Signature creation and verification
  - ✅ Base58 encoding/decoding tests
  - ✅ AES encryption/decryption tests
  - ✅ HMAC-SHA256 and HMAC-SHA512 tests
  - ✅ PBKDF2 key derivation tests

- **Blockchain Tests**
  - ✅ Transaction creation and serialization
  - ✅ Block construction and validation
  - ✅ Merkle tree computation
  - ✅ UTXO management tests
  - ✅ Address generation and validation
  - ✅ Block reward calculation and halving
  - ✅ Script execution tests
  - ✅ Signature verification in context

- **Security Audit**
  - ✅ Comprehensive security checklist (13 categories)
  - ✅ Cryptographic security review
  - ✅ Wallet security analysis
  - ✅ Transaction security validation
  - ✅ Consensus security verification
  - ✅ Network security assessment
  - ✅ RPC security documentation
  - ✅ Memory safety review
  - ✅ Thread safety analysis
  - ✅ Known limitations documented
  - ✅ Production readiness checklist
  - ✅ Incident response procedures

### ✅ Implemented (Phase 8 - Advanced Features)

- **Mining System**
  - ✅ Multi-threaded CPU mining
  - ✅ Configurable thread count
  - ✅ Mining statistics and hashrate calculation
  - ✅ Block template creation
  - ✅ Nonce range distribution across threads
  - ✅ PoW verification (bits to target conversion)
  - ✅ Hash target checking
  - ✅ Mining start/stop controls
  - ✅ Callback system for found blocks
  - ✅ Coinbase transaction creation

- **Mining Configuration**
  - ✅ Configurable coinbase address
  - ✅ Max nonce limit settings
  - ✅ Thread pool management
  - ✅ Mining timeout controls

### ✅ Implemented (Phase 9 - Production Deployment) ⭐ NEW

- **Main Application Integration**
  - ✅ Fully integrated main.cpp with all components
  - ✅ Genesis block creation and initialization
  - ✅ Wallet, network, RPC, and mining coordination
  - ✅ Graceful shutdown handling
  - ✅ Periodic statistics logging (60-second intervals)
  - ✅ Signal handling (SIGINT, SIGTERM)

- **Docker Support**
  - ✅ Multi-stage production Dockerfile
  - ✅ Docker Compose with 3 profiles (mainnet, testnet, mining)
  - ✅ Volume persistence for blockchain data
  - ✅ Health checks and auto-restart
  - ✅ Environment-based configuration
  - ✅ Non-root user execution

- **Azure Cloud Deployment**
  - ✅ Automated VM deployment script (deploy.sh)
  - ✅ Azure Container Instances configuration
  - ✅ Network security group setup
  - ✅ Resource group management
  - ✅ Firewall configuration (P2P + RPC ports)
  - ✅ Load balancer ready

- **Comprehensive Documentation**
  - ✅ Complete setup guide (SETUP_GUIDE.md)
  - ✅ Postman API documentation (POSTMAN_API_DOCUMENTATION.md)
  - ✅ Postman collection with all 30+ endpoints
  - ✅ Docker deployment instructions
  - ✅ Azure deployment guide
  - ✅ Troubleshooting section
  - ✅ Quick command reference

### ✅ Implemented (Phase 10 - Security Hardening) ⭐ NEW

- **Critical Security Fixes**
  - ✅ **CRITICAL-002**: Fixed RPC authentication bypass
    - Proper Base64 decoding for HTTP Basic Auth
    - Constant-time string comparison (timing attack prevention)
    - Rate limiting (10 req/60s) with IP banning
    - Brute force protection (2-second delays)
    - Auto-ban after 10 failed attempts

  - ✅ **HIGH-001**: Fixed weak wallet encryption RNG
    - Replaced std::mt19937 with OpenSSL RAND_bytes
    - Cryptographically secure salt/IV generation
    - Applied to all wallet encryption operations

  - ✅ **HIGH-002**: Implemented transaction validation
    - Full structure validation (inputs/outputs presence)
    - Transaction size limits (max 1MB)
    - UTXO existence verification
    - Signature validation with misbehavior scoring

  - ✅ **HIGH-003**: Implemented peer misbehavior system
    - Misbehavior score tracking per peer
    - Automatic banning at threshold (100 points)
    - Graduated penalties for violations

  - ✅ **HIGH-005**: Multi-script UTXO validation
    - P2PKH (Pay to Public Key Hash)
    - P2SH (Pay to Script Hash)
    - P2WPKH (SegWit witness key hash)
    - P2WSH (SegWit witness script hash)

  - ✅ **HIGH-006**: Wallet auto-lock feature
    - UnlockWithTimeout() method
    - Background thread monitoring
    - Automatic wallet locking after timeout

  - ✅ **HIGH-008**: Integer overflow protection
    - SafeAdd() with overflow detection
    - SafeSub() with underflow detection
    - SafeMul() with multiplication checks
    - MAX_MONEY validation

- **Security Infrastructure**
  - ✅ Security utility library (src/util/security.h/cpp)
  - ✅ Base64 encoding/decoding
  - ✅ Constant-time comparison functions
  - ✅ Cryptographically secure RNG wrapper
  - ✅ Rate limiting with IP banning
  - ✅ All TODOs eliminated from codebase (0 remaining)

---

## Project Structure

```
dinari-blockchain-hub/
├── src/
│   ├── blockchain/     # Block and blockchain ✅
│   ├── consensus/      # Difficulty & validation ✅
│   ├── core/           # Transactions, UTXO, scripts, mempool ✅
│   ├── crypto/         # Cryptographic primitives ✅
│   ├── network/        # P2P networking ✅
│   ├── wallet/         # Wallet and key management ✅
│   ├── mining/         # Mining functionality ✅
│   ├── rpc/            # JSON-RPC 2.0 server ✅
│   ├── storage/        # Database abstraction ✅
│   ├── util/           # Utilities (including security) ✅
│   ├── cli/            # Command-line tools ✅
│   └── main.cpp        # Fully integrated entry point ✅
├── include/dinari/     # Public headers ✅
├── tests/              # Test suite ✅
├── config/             # Configuration files ✅
├── docs/               # Documentation ✅
│   ├── SECURITY_AUDIT.md
│   ├── SETUP_GUIDE.md
│   └── POSTMAN_API_DOCUMENTATION.md
├── postman/            # Postman collection ✅
├── docker/             # Docker configuration ✅
├── azure/              # Azure deployment scripts ✅
├── Dockerfile          # Production Docker image ✅
├── docker-compose.yml  # Multi-profile compose ✅
└── CMakeLists.txt      # Build configuration ✅
```

---

## Development Status

**Current Phase:** Phase 10 (Security Hardening) - ✅ Complete
**Status:** Production-ready with comprehensive security hardening!

### Roadmap

- [x] **Phase 1:** Foundation (Crypto, Serialization, Utilities) ✅
- [x] **Phase 2:** Core Blockchain (Transactions, Blocks, UTXO) ✅
- [x] **Phase 3:** Consensus & Blockchain (Difficulty, Validation, Chain Management) ✅
- [x] **Phase 4:** Networking (P2P, Block Propagation, Peer Management) ✅
- [x] **Phase 5:** Wallet (HD Wallet, Key Management, Transaction Creation) ✅
- [x] **Phase 6:** APIs (JSON-RPC, CLI) ✅
- [x] **Phase 7:** Testing & Security (Unit Tests, Security Audit) ✅
- [x] **Phase 8:** Advanced Features (Multi-threaded Mining) ✅
- [x] **Phase 9:** Production Deployment (Docker, Azure, Documentation) ✅
- [x] **Phase 10:** Security Hardening (All vulnerabilities fixed) ✅

**Progress:** 10/10 phases complete (100%)

---

## Building

### Prerequisites

**Required:**
- CMake 3.15 or higher
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- OpenSSL 1.1.1+ or 3.0+

**Optional:**
- Docker (for containerized deployment)
- Azure CLI (for cloud deployment)

### Linux / macOS

```bash
# Clone the repository
git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git
cd dinari-blockchain-hub

# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake libssl-dev

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j$(nproc)

# Run
./dinarid --help
```

### Windows (Visual Studio 2022)

```powershell
# Clone the repository
git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git
cd dinari-blockchain-hub

# Create build directory
mkdir build
cd build

# Configure for Visual Studio
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release

# Run
Release\dinarid.exe --help
```

---

## Deployment

### Docker Deployment

**Quick Start:**
```bash
# Build and run mainnet node
docker-compose --profile mainnet up -d

# Build and run testnet node
docker-compose --profile testnet up -d

# Build and run mining node
docker-compose --profile mining up -d

# View logs
docker-compose logs -f

# Stop
docker-compose down
```

**Manual Docker Build:**
```bash
# Build image
docker build -t dinari-blockchain .

# Run container
docker run -d \
  --name dinarid \
  -p 9333:9333 \
  -p 9334:9334 \
  -v dinari-data:/data \
  dinari-blockchain
```

### Azure Deployment

**Automated VM Deployment:**
```bash
cd azure
chmod +x deploy.sh
./deploy.sh
```

**Azure Container Instances:**
```bash
az container create \
  --resource-group dinari-rg \
  --file container-deploy.yml
```

For detailed deployment instructions, see [docs/SETUP_GUIDE.md](docs/SETUP_GUIDE.md).

---

## API Documentation

### Postman Collection

A complete Postman collection with all 30+ RPC endpoints is available:
- **Collection:** `postman/Dinari_Blockchain_API.postman_collection.json`
- **Documentation:** `docs/POSTMAN_API_DOCUMENTATION.md`

### Quick API Examples

**Get Blockchain Info:**
```bash
curl -u user:pass http://localhost:9334 \
  -d '{"jsonrpc":"2.0","method":"getblockchaininfo","params":[],"id":1}'
```

**Get Transaction by Hash (Explorer API):**
```bash
curl -u user:pass http://localhost:9334 \
  -d '{"jsonrpc":"2.0","method":"getrawtransaction","params":["<txid>",true],"id":1}'
```

**List Recent Blocks (Explorer API):**
```bash
curl -u user:pass http://localhost:9334 \
  -d '{"jsonrpc":"2.0","method":"listblocks","params":[0,10],"id":1}'
```

**Send Transaction:**
```bash
curl -u user:pass http://localhost:9334 \
  -d '{"jsonrpc":"2.0","method":"sendtoaddress","params":["D1abc...",100.0],"id":1}'
```

For complete API documentation with all methods, see [docs/POSTMAN_API_DOCUMENTATION.md](docs/POSTMAN_API_DOCUMENTATION.md).

---

## Testing

### Running Tests

```bash
# Build with tests enabled (default)
cd build
cmake .. -DBUILD_TESTS=ON
cmake --build .

# Run all tests
ctest --verbose

# Or run test executables directly
./tests/test_crypto
./tests/test_blockchain
```

### Test Coverage

- **Cryptographic Tests** (tests/test_crypto.cpp)
  - SHA-256, Double SHA-256, Hash160
  - ECDSA key generation and signature verification
  - Base58 encoding/decoding
  - AES encryption/decryption
  - HMAC and PBKDF2

- **Blockchain Tests** (tests/test_blockchain.cpp)
  - Transaction creation and validation
  - Block construction and merkle trees
  - UTXO management
  - Address generation
  - Block rewards and halving

---

## Configuration

Edit `config/mainnet.conf` or `config/testnet.conf`:

```ini
# Network
testnet=0
port=9333
rpcport=9334

# RPC Authentication (CHANGE FOR PRODUCTION!)
rpcuser=yoursecureusername
rpcpassword=yoursecurepassword

# Data Directory
datadir=/path/to/blockchain/data

# Mining (optional)
enablemining=1
miningthreads=4
miningaddress=D1YourMiningAddress...

# Logging
loglevel=info
printtoconsole=1
```

---

## Security

The Dinari blockchain has been designed and hardened with security as the absolute top priority. All critical vulnerabilities identified in the security audit have been fixed.

### Security Features

- ✅ Memory-safe C++ practices (RAII, smart pointers)
- ✅ **Cryptographically secure random number generation (OpenSSL RAND_bytes)**
- ✅ **Constant-time operations in authentication (timing attack prevention)**
- ✅ Input validation and sanitization
- ✅ PBKDF2 key derivation with 100,000 iterations
- ✅ AES-256-CBC encryption for wallet data
- ✅ Thread-safe operations with mutex protection
- ✅ **DoS protection (rate limiting, IP banning, connection limits)**
- ✅ Private key wiping from memory
- ✅ Signature verification for all transactions
- ✅ **Transaction validation before network relay**
- ✅ **Peer misbehavior scoring and automatic banning**
- ✅ **Integer overflow protection with safe arithmetic**
- ✅ **Multi-script type validation (P2PKH, P2SH, P2WPKH, P2WSH)**
- ✅ **Wallet auto-lock with configurable timeout**
- ✅ **Zero TODO items in codebase (all implemented or documented)**

### Security Audit Status

| Vulnerability | Severity | Status |
|---------------|----------|--------|
| Main application integration | CRITICAL | ✅ FIXED |
| RPC authentication bypass | CRITICAL | ✅ FIXED |
| Weak wallet encryption RNG | HIGH | ✅ FIXED |
| No transaction validation | HIGH | ✅ FIXED |
| No peer banning system | HIGH | ✅ FIXED |
| Incomplete UTXO validation | HIGH | ✅ FIXED |
| No wallet auto-lock | HIGH | ✅ FIXED |
| Integer overflow risks | HIGH | ✅ FIXED |

A comprehensive security audit covering 13 major categories is available in [docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md).

**Note:** While all identified vulnerabilities have been addressed, a professional third-party security audit is strongly recommended before mainnet deployment.

---

## Documentation

- **[Setup Guide](docs/SETUP_GUIDE.md)** - Complete setup and deployment guide
- **[Postman API Documentation](docs/POSTMAN_API_DOCUMENTATION.md)** - Full API reference with examples
- **[Security Audit](docs/SECURITY_AUDIT.md)** - Comprehensive security checklist and audit
- **[Postman Collection](postman/Dinari_Blockchain_API.postman_collection.json)** - Import into Postman for testing

---

## Usage

### Starting the Node

```bash
# Start mainnet node
./dinarid --config=../config/mainnet.conf

# Start testnet node
./dinarid --config=../config/testnet.conf

# Enable mining
./dinarid --mining --miningthreads=4 --miningaddress=D1YourAddress...
```

### Using the CLI

```bash
# Get blockchain info
./dinari-cli getblockchaininfo

# Get new address
./dinari-cli getnewaddress

# Send transaction
./dinari-cli sendtoaddress D1recipient... 100.0

# List recent blocks (Explorer API)
./dinari-cli listblocks 0 10

# Get transaction details (Explorer API)
./dinari-cli getrawtransaction <txid> true
```

### Systemd Service

```bash
# Copy service file
sudo cp dinari-blockchain.service /etc/systemd/system/

# Enable and start
sudo systemctl enable dinari-blockchain
sudo systemctl start dinari-blockchain

# Check status
sudo systemctl status dinari-blockchain
```

---

## Contributing

Contributions are welcome! Please feel free to submit pull requests or open issues for bugs and feature requests.

### Development Guidelines

1. Follow the existing code style (C++17 standards)
2. Add tests for new features
3. Update documentation as needed
4. Ensure all tests pass before submitting PR
5. Security-critical changes require thorough review

---

## License

This project is licensed under the MIT License. See LICENSE file for details.

---

## Contact

- **GitHub:** [EmekaIwuagwu/dinari-blockchain-hub](https://github.com/EmekaIwuagwu/dinari-blockchain-hub)
- **Issues:** [Report bugs and request features](https://github.com/EmekaIwuagwu/dinari-blockchain-hub/issues)

---

<div align="center">

**Built with ❤️ for Production-Grade, Security-Hardened Blockchain Infrastructure**

*Ready for deployment with comprehensive security, Docker support, and cloud-ready architecture.*

</div>
