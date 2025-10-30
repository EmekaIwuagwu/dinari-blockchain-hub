"# Dinari Blockchain

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0--alpha-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-passing-brightgreen)

**A Production-Ready, Bitcoin-Style Proof-of-Work Blockchain**

[Features](#features) • [Build](#building) • [Usage](#usage) • [Documentation](#documentation) • [Contributing](#contributing)

</div>

---

## Overview

**Dinari Blockchain** is a production-grade, Bitcoin-style Proof-of-Work blockchain system implemented entirely in C++. Designed for real-world financial transactions on mainnet, every component has been built with security, performance, and correctness as top priorities.

### Key Specifications

- **Blockchain Name:** Dinari Blockchain
- **Native Token:** Dinari (DNT)
- **Initial Supply:** 700 Trillion DNT (700,000,000,000,000)
- **Address Prefix:** `D` (e.g., D1a2b3c4...)
- **Consensus:** Proof of Work (PoW)
- **Block Time:** 10 minutes (600 seconds)
- **Difficulty Adjustment:** Every 2,016 blocks
- **Block Reward Halving:** Every 210,000 blocks
- **Max Block Size:** 2 MB

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
  - ✅ Visual Studio 2022 support
  - ✅ Cross-platform considerations

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

- **Script System**
  - ✅ Stack-based script execution engine
  - ✅ Standard script types (P2PKH, P2SH, P2PK, Multisig)
  - ✅ OpCode implementation (DUP, HASH160, CHECKSIG, etc.)
  - ✅ Script verification and validation
  - ✅ Signature creation and verification

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

- **Network Node**
  - ✅ Multi-threaded network I/O
  - ✅ Listen for incoming connections
  - ✅ Automatic peer connection management
  - ✅ Message routing and processing
  - ✅ Network statistics tracking

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
  - ✅ Transaction relay (TX messages)
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
  - ✅ Private key import/export

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
  - ✅ HTTP server with basic authentication
  - ✅ Command registration and routing
  - ✅ Error handling with Bitcoin-compatible codes
  - ✅ Request/response serialization
  - ✅ Thread-safe operation

- **Blockchain RPC Commands**
  - ✅ getblockcount - Get current block height
  - ✅ getblockhash - Get block hash at height
  - ✅ getblock - Get block information
  - ✅ getbestblockhash - Get best block hash
  - ✅ getdifficulty - Get current difficulty
  - ✅ getblockchaininfo - Comprehensive blockchain info
  - ✅ gettxout - Get UTXO information
  - ✅ getmempoolinfo - Mempool statistics
  - ✅ getrawmempool - List mempool transactions

- **Wallet RPC Commands**
  - ✅ getnewaddress - Generate new receiving address
  - ✅ getbalance - Get wallet balance
  - ✅ sendtoaddress - Send DNT to address
  - ✅ listaddresses - List all wallet addresses
  - ✅ listtransactions - List transaction history
  - ✅ listunspent - List unspent outputs
  - ✅ getwalletinfo - Wallet state information
  - ✅ encryptwallet - Encrypt wallet with passphrase
  - ✅ walletlock - Lock encrypted wallet
  - ✅ walletpassphrase - Unlock wallet temporarily
  - ✅ walletpassphrasechange - Change wallet passphrase
  - ✅ importmnemonic - Import HD wallet from mnemonic
  - ✅ importprivkey - Import private key
  - ✅ validateaddress - Validate Dinari address
  - ✅ getaddressinfo - Get address information

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
  - ✅ Authentication framework

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

---

## Project Structure

```
DinariBlockchain/
├── src/
│   ├── blockchain/     # Block and blockchain ✅ (blocks, chain, merkle)
│   ├── consensus/      # Difficulty & validation ✅
│   ├── core/           # Transactions, UTXO, scripts, mempool ✅
│   ├── crypto/         # Cryptographic primitives ✅
│   ├── network/        # P2P networking ✅ (protocol, peers, messages, node)
│   ├── wallet/         # Wallet and key management ✅ (HD wallet, BIP32/39/44)
│   ├── mining/         # Mining functionality ✅ (multi-threaded CPU mining)
│   ├── rpc/            # RPC server ✅ (JSON-RPC 2.0)
│   ├── storage/        # Database abstraction ✅
│   ├── util/           # Utilities ✅
│   ├── cli/            # Command-line tools ✅
│   └── main.cpp        # Entry point ✅
├── include/dinari/     # Public headers ✅
├── tests/              # Test suite ✅ (crypto, blockchain, framework)
├── config/             # Configuration files ✅
├── docs/               # Documentation ✅ (security audit)
└── CMakeLists.txt      # Build configuration ✅
```

---

## Development Status

**Current Phase:** Phase 8 (Advanced Features) - ✅ Complete
**Status:** All core phases complete! Ready for production hardening.

### Roadmap

- [x] **Phase 1:** Foundation (Crypto, Serialization, Utilities) ✅
- [x] **Phase 2:** Core Blockchain (Transactions, Blocks, UTXO) ✅
- [x] **Phase 3:** Consensus & Blockchain (Difficulty, Validation, Chain Management) ✅
- [x] **Phase 4:** Networking (P2P, Block Propagation, Peer Management) ✅
- [x] **Phase 5:** Wallet (HD Wallet, Key Management, Transaction Creation) ✅
- [x] **Phase 6:** APIs (JSON-RPC, CLI) ✅
- [x] **Phase 7:** Testing & Security (Unit Tests, Security Audit) ✅
- [x] **Phase 8:** Advanced Features (Multi-threaded Mining) ✅

**Progress:** 8/8 phases complete (100%)

---

## Building

### Prerequisites

**Required:**
- CMake 3.15 or higher
- C++17 compatible compiler (GCC 9+, Clang 10+, MSVC 2019+)
- OpenSSL 1.1.1+ or 3.0+

**Optional (for future phases):**
- LevelDB or RocksDB (for blockchain storage)
- Boost 1.70+ (for networking)
- Google Test (for unit tests)

### Linux / macOS

```bash
# Clone the repository
git clone https://github.com/EmekaIwuagwu/dinari-blockchain-hub.git
cd dinari-blockchain-hub

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
```

---

## Testing

The project includes a comprehensive test suite covering cryptographic functions and blockchain components.

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
rpcuser=youruser
rpcpassword=yourpassword

# Data Directory
datadir=/path/to/blockchain/data

# Logging
loglevel=info
printtoconsole=1
```

---

## Security

The Dinari blockchain has been designed with security as a top priority. For a comprehensive security review, see [SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md).

### Security Features

- ✅ Memory-safe C++ practices (RAII, smart pointers)
- ✅ Cryptographically secure random number generation
- ✅ Constant-time operations in crypto code
- ✅ Input validation and sanitization
- ✅ PBKDF2 key derivation with 100,000 iterations
- ✅ AES-256-CBC encryption for wallet data
- ✅ Thread-safe operations with mutex protection
- ✅ DoS protection (connection limits, message size limits)
- ✅ Private key wiping from memory
- ✅ Signature verification for all transactions

### Security Audit

A comprehensive security audit checklist covering 13 major categories is available in [docs/SECURITY_AUDIT.md](docs/SECURITY_AUDIT.md):

1. Cryptographic Security (Hash functions, ECDSA, Encryption)
2. Wallet Security (Key storage, HD wallet, Address generation)
3. Transaction Security (Validation, Script security)
4. Consensus Security (PoW, Difficulty adjustment, Chain validation)
5. Network Security (P2P protocol, DoS protection)
6. RPC Security (Authentication, Input validation)
7. Memory Safety (RAII, Bounds checking, Thread safety)
8. Data Integrity (Serialization, Checksums)
9. Privacy Considerations
10. Known Limitations & TODOs
11. Security Best Practices
12. Incident Response
13. Audit Status

**Note:** A professional security audit is strongly recommended before mainnet deployment.

---

## License

This project is licensed under the MIT License.

---

## Contact

- **GitHub:** [EmekaIwuagwu/dinari-blockchain-hub](https://github.com/EmekaIwuagwu/dinari-blockchain-hub)

---

<div align="center">

**Built with ❤️ for Production-Grade Blockchain Infrastructure**

</div>" 
