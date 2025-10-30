"# Dinari Blockchain

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0--alpha-blue)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-in--progress-yellow)

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

### 🚧 In Development (Phase 4 - Networking)

- P2P protocol implementation
- Block propagation
- Transaction relay
- Peer discovery and management

### 📋 Planned (Phase 4+)

- P2P networking
- HD wallet (BIP32/39/44)
- JSON-RPC API
- REST API
- Storage layer (LevelDB/RocksDB)
- Comprehensive test suite

---

## Project Structure

```
DinariBlockchain/
├── src/
│   ├── blockchain/     # Block and blockchain ✅ (blocks, chain, merkle)
│   ├── consensus/      # Difficulty & validation ✅
│   ├── core/           # Transactions, UTXO, scripts, mempool ✅
│   ├── crypto/         # Cryptographic primitives ✅
│   ├── wallet/         # Wallet and key management
│   ├── network/        # P2P networking
│   ├── mining/         # Mining functionality
│   ├── rpc/            # RPC server
│   ├── storage/        # Database abstraction
│   ├── util/           # Utilities ✅
│   └── main.cpp        # Entry point ✅
├── include/dinari/     # Public headers ✅
├── tests/              # Test suite
├── config/             # Configuration files ✅
├── docs/               # Documentation
└── CMakeLists.txt      # Build configuration ✅
```

---

## Development Status

**Current Phase:** Phase 3 (Consensus & Blockchain) - ✅ Complete
**Next Phase:** Phase 4 (P2P Networking) - 🚧 Ready to Start

### Roadmap

- [x] **Phase 1:** Foundation (Crypto, Serialization, Utilities) ✅
- [x] **Phase 2:** Core Blockchain (Transactions, Blocks, UTXO) ✅
- [x] **Phase 3:** Consensus & Blockchain (Difficulty, Validation, Chain Management) ✅
- [ ] **Phase 4:** Networking (P2P, Block Propagation, Peer Management)
- [ ] **Phase 5:** Wallet (HD Wallet, Key Management, Transaction Creation)
- [ ] **Phase 6:** APIs (RPC, REST, CLI)
- [ ] **Phase 7:** Testing & Security (Unit Tests, Integration Tests, Security Audit)
- [ ] **Phase 8:** Advanced Features (Mining Pools, SPV, KYC Integration)

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

## Security Features

- ✅ Memory-safe C++ practices (RAII, smart pointers)
- ✅ Cryptographically secure random number generation
- ✅ Constant-time operations in crypto code
- ✅ Input validation and sanitization
- ✅ PBKDF2 key derivation with 100,000 iterations
- ✅ AES-256-CBC encryption for wallet data

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
