# Project Context

## Purpose
X-Cash Core is a community-driven, open-source blockchain project developing a new standard for digital payments. The project provides:
- **FlexPrivacy**: Toggle between public and private transactions without compromising privacy
- **Delegated Proof of Private Stake (DPOPS)**: Custom consensus mechanism for scalability and reduced energy consumption
- **Sidechains**: Enable delegates to host customizable blockchain solutions

Based on the CryptoNote protocol (forked from Monero), X-Cash aims to be the standard in digital payment and transaction settlement.

## Tech Stack
- **Primary Language**: C++ (C++11/14 standard)
- **Build System**: CMake 3.5+, GNU Make
- **Core Dependencies**:
  - Boost (1.58+) - C++ libraries
  - OpenSSL - Cryptography and SHA256
  - libsodium - Cryptographic library
  - libzmq (3.0.0+) - ZeroMQ for messaging
  - libunbound (1.4.16+) - DNS resolver
- **Optional Dependencies**:
  - libminiupnpc (2.0+) - NAT punching
  - libunwind - Stack traces
  - libreadline (6.3.0+) - Input editing
  - GTest (1.5+) - Test suite
  - Doxygen + Graphviz - Documentation
- **Cryptography**: CryptoNote protocol, Ring Signatures, VRF functions
- **Networking**: P2P networking, RPC server/client
- **Database**: Custom blockchain database implementation
- **Platforms**: Linux, macOS, Windows, iOS (via CMakeLists_IOS.txt)

## Project Conventions

### Code Style
- **C++ Standard**: C++11/14 features used throughout
- **Naming Conventions**:
  - Namespaces: `cryptonote`, `rpc`, `nodetool`, `epee`
  - Snake_case for variables and functions
  - UPPER_CASE for preprocessor defines and constants
  - Class names in CamelCase
- **Header Guards**: `#pragma once` used consistently
- **Copyright**: MIT License with X-Cash Foundation and Monero Project attribution
- **File Organization**: Clear separation between blockchain_db, cryptonote_core, daemon, wallet, RPC, P2P modules

### Architecture Patterns
- **Modular Architecture**:
  - `src/blockchain_db/` - Blockchain database abstraction
  - `src/blockchain_utilities/` - Import/export/maintenance tools
  - `src/cryptonote_core/` - Core blockchain logic
  - `src/cryptonote_protocol/` - Network protocol
  - `src/daemon/` - Node daemon
  - `src/wallet/` - Wallet implementation
  - `src/simplewallet/` - CLI wallet
  - `src/rpc/` - RPC server and client
  - `src/p2p/` - Peer-to-peer networking
  - `src/crypto/` - Cryptographic primitives
- **Separation of Concerns**: Clear boundaries between consensus, networking, storage, and application layers
- **Template-heavy design**: Extensive use of C++ templates for type safety and performance
- **External vendored libraries**: Located in `external/` and `contrib/` directories

### Testing Strategy
- **Test Framework**: Google Test (GTest)
- **Test Organization**:
  - `tests/unit_tests/` - Unit tests
  - `tests/core_tests/` - Core blockchain logic tests
  - `tests/crypto/` - Cryptography tests
  - `tests/performance_tests/` - Performance benchmarks
  - `tests/functional_tests/` - End-to-end functional tests
  - `tests/fuzz/` - Fuzzing tests
  - `tests/difficulty/` - Difficulty algorithm tests
- **Make targets**: `make debug-test` for running test suite
- **Known Issues**: libwallet_api_tests currently excluded from test runs (Issue #895)

### Git Workflow
- **Repository**: Xcash-Tech/xcash-tech-core
- **Main Branch**: `master`
- **License**: MIT License
- **Build Configuration**: Multiple targets (debug, release, static builds)
- **Platform Support**: Makefile wrappers for CMake build system

## Domain Context

### Blockchain Specifics
- **Currency**: X-CASH (XCASH)
- **Decimal Precision**: 6 decimal places (CRYPTONOTE_DISPLAY_DECIMAL_POINT)
- **Smallest Unit**: 1 COIN = 1,000,000 base units
- **Total Supply**: 100,000,000,000 XCASH (MONEY_SUPPLY)
- **Block Time**: 60 seconds (DIFFICULTY_TARGET)
- **Premine**: 40,000,000,000 XCASH at block height 1
- **Emission**: 
  - Speed factor: 19 per minute
  - Final subsidy: 2000 XCASH/minute (~1.05% annual inflation starting ~2025)
- **Transaction Fees**:
  - Base fee per KB: 2000 units
  - Dynamic fee calculation based on block size
- **Block Size Limits**:
  - Max block size: 196,608 bytes (miner production limit)
  - Full reward zone: 300,000 bytes (V5)

### Consensus Evolution
- **Original**: CryptoNight Proof-of-Work
- **Current**: Transitioning to Delegated Proof-of-Private-Stake (DPOPS)
- **Hard Forks**:
  - HF V8: LWMA difficulty algorithm (block 95085)
  - HF V9: Further difficulty adjustments
- **Difficulty Algorithm**: LWMA (Linearly Weighted Moving Average) with 120-block window

### Privacy Features
- **Ring Signatures**: Obfuscates transaction sources
- **Stealth Addresses**: Protects recipient privacy
- **Hybrid Transactions**: Toggle between public and private modes
- **Mined Money Unlock Window**: 60 blocks

## Important Constraints
- **Monero Codebase Heritage**: Code derived from Monero/CryptoNote, maintains compatibility considerations
- **Consensus Rules**: Strict adherence to block validation and difficulty adjustment algorithms
- **Network Protocol**: P2P protocol must remain compatible with existing nodes
- **Cryptographic Security**: No modifications to core cryptographic primitives without thorough review
- **API Stability**: RPC interfaces used by external services (wallets, exchanges)
- **Performance**: Blockchain verification and synchronization must remain performant
- **Storage**: Efficient blockchain database design for growing chain size

## External Dependencies
- **Build Dependencies**: See dependency table in README (GCC, CMake, pkg-config, Boost, OpenSSL, etc.)
- **Vendored Libraries** (in `external/` and `contrib/`):
  - `easylogging++` - Logging framework
  - `rapidjson` - JSON parsing
  - `miniupnp` - UPnP implementation
  - `unbound` - DNS resolver
  - `VRF_functions` - Verifiable Random Functions
  - Boost subset
  - Berkeley DB drivers
- **Network Services**:
  - DNS for seed nodes
  - P2P network for blockchain synchronization
  - RPC endpoints for wallet and mining software
- **Documentation**: GitBook at docs.xcash.foundation
- **Community**: Discord, Gitter for support and collaboration
