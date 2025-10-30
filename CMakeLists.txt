cmake_minimum_required(VERSION 3.15)
project(DinariBlockchain VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_MINING "Build mining components" ON)
option(BUILD_KYC "Build KYC integration" OFF)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${OPENSSL_INCLUDE_DIR}
)

# Source files - Core
set(CORE_SOURCES
    src/core/transaction.cpp
    src/core/script.cpp
    src/core/utxo.cpp
    src/core/mempool.cpp
)

# Source files - Blockchain
set(BLOCKCHAIN_SOURCES
    src/blockchain/block.cpp
    src/blockchain/blockchain.cpp
    src/blockchain/merkle.cpp
)

# Source files - Consensus
set(CONSENSUS_SOURCES
    src/consensus/difficulty.cpp
    src/consensus/validation.cpp
)

# Source files - Crypto
set(CRYPTO_SOURCES
    src/crypto/hash.cpp
    src/crypto/ecdsa.cpp
    src/crypto/aes.cpp
    src/crypto/base58.cpp
)

# Source files - Wallet
set(WALLET_SOURCES
    src/wallet/wallet.cpp
    src/wallet/hdwallet.cpp
    src/wallet/keystore.cpp
    src/wallet/address.cpp
)

# Source files - Network
set(NETWORK_SOURCES
    src/network/protocol.cpp
    src/network/message.cpp
    src/network/netbase.cpp
    src/network/peer.cpp
    src/network/addrman.cpp
    src/network/node.cpp
)

# Source files - Mining
set(MINING_SOURCES
    src/mining/miner.cpp
    src/mining/hasher.cpp
    src/mining/pool.cpp
)

# Source files - RPC
set(RPC_SOURCES
    src/rpc/rpcserver.cpp
    src/rpc/rpcwallet.cpp
    src/rpc/rpcblockchain.cpp
)

# Source files - Storage
set(STORAGE_SOURCES
    src/storage/database.cpp
    src/storage/blockstore.cpp
    src/storage/txindex.cpp
)

# Source files - Utilities
set(UTIL_SOURCES
    src/util/logger.cpp
    src/util/config.cpp
    src/util/serialize.cpp
    src/util/time.cpp
)

# KYC sources (optional)
if(BUILD_KYC)
    set(KYC_SOURCES
        src/kyc/kyc.cpp
        src/kyc/compliance.cpp
    )
endif()

# Combine all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${BLOCKCHAIN_SOURCES}
    ${CONSENSUS_SOURCES}
    ${CRYPTO_SOURCES}
    ${WALLET_SOURCES}
    ${NETWORK_SOURCES}
    ${RPC_SOURCES}
    ${STORAGE_SOURCES}
    ${UTIL_SOURCES}
)

if(BUILD_MINING)
    list(APPEND ALL_SOURCES ${MINING_SOURCES})
endif()

if(BUILD_KYC)
    list(APPEND ALL_SOURCES ${KYC_SOURCES})
endif()

# Create static library
add_library(dinari_core STATIC ${ALL_SOURCES})

target_link_libraries(dinari_core
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
        Threads::Threads
)

# Main executable
add_executable(dinarid src/main.cpp)
target_link_libraries(dinarid PRIVATE dinari_core)

# Testing
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
install(TARGETS dinarid DESTINATION bin)
install(DIRECTORY include/dinari DESTINATION include)
install(FILES config/mainnet.conf config/testnet.conf DESTINATION etc/dinari)

# Documentation
add_custom_target(docs
    COMMAND doxygen ${CMAKE_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

# Print configuration summary
message(STATUS "")
message(STATUS "Dinari Blockchain Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build mining: ${BUILD_MINING}")
message(STATUS "  Build KYC: ${BUILD_KYC}")
message(STATUS "")
