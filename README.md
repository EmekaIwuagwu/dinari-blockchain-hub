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

### 🚧 In Development (Phase 2 - Core Blockchain)

- Transaction structure and validation
- Block structure and Merkle trees
- UTXO set management
- Blockchain data structure
- Genesis block creation

### 📋 Planned (Phase 3+)

- Proof of Work mining
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
│   ├── blockchain/     # Block and blockchain logic
│   ├── consensus/      # PoW and validation
│   ├── core/           # Transactions, UTXO, mempool
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

**Current Phase:** Phase 1 (Foundation) - ✅ Complete
**Next Phase:** Phase 2 (Core Blockchain) - 🚧 Starting

### Roadmap

- [x] **Phase 1:** Foundation (Crypto, Serialization, Utilities)
- [ ] **Phase 2:** Core Blockchain (Transactions, Blocks, UTXO)
- [ ] **Phase 3:** Consensus (PoW, Difficulty, Mining)
- [ ] **Phase 4:** Networking (P2P, Block Propagation)
- [ ] **Phase 5:** Wallet (HD Wallet, Key Management)
- [ ] **Phase 6:** APIs (RPC, REST, CLI)
- [ ] **Phase 7:** Testing & Security (Unit Tests, Integration Tests)
- [ ] **Phase 8:** Advanced Features (Mining Pools, SPV, KYC)

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
