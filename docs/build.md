# Building Xcash Tech Core from Source

This guide provides detailed instructions for building Xcash Tech Core on various platforms.

## Platform Support

- **Linux**: x86_64, ARM64, ARMv7/v8
- **macOS**: Intel x86_64, Apple Silicon (M1/M2/M3/M4)
- **Windows**: x86_64, x86 (via MSYS2)

## Dependencies

| Dependency | Min. Version | Ubuntu/Debian | macOS (Homebrew) | Purpose |
|------------|--------------|---------------|------------------|---------|  
| GCC/Clang | 4.7.3+ | `build-essential` | Xcode CLI tools | C++ compiler |
| CMake | 3.0.0 | `cmake` | `cmake` | Build system |
| Boost | 1.58 | `libboost-all-dev` | `boost` | C++ libraries |
| OpenSSL | 1.1+ | `libssl-dev` | `openssl` | Cryptography |
| libsodium | 1.0+ | `libsodium-dev` | `libsodium` | Ed25519 signatures |
| libzmq | 3.0.0 | `libzmq3-dev` | `zeromq` | ZeroMQ messaging |
| libunbound | 1.4.16 | `libunbound-dev` | `unbound` | DNS resolution |

## Build Instructions

### Linux (Ubuntu/Debian)

```bash
# Install dependencies
sudo apt update && sudo apt install -y \
  build-essential cmake pkg-config \
  libboost-all-dev libssl-dev libzmq3-dev \
  libsodium-dev libunbound-dev

# Clone and build
git clone https://github.com/Xcash-Tech/xcash-tech-core.git
cd xcash-tech-core
make release -j$(nproc)

# Binaries in: build/release/bin/xcashd
```

#### Static Build (Linux)

For portable binaries that don't depend on system libraries:

```bash
make release-static -j$(nproc)
```

### macOS (Intel & Apple Silicon M1/M2/M3/M4)

#### Install Xcode Command Line Tools

```bash
xcode-select --install
```

#### Install Homebrew (if not installed)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Install Dependencies and Build

```bash
# Install dependencies
brew install cmake boost openssl@3 zeromq libsodium unbound

# Clone and build
git clone https://github.com/Xcash-Tech/xcash-tech-core.git
cd xcash-tech-core
make release -j$(sysctl -n hw.ncpu)

# Binaries in: build/release/bin/xcashd
```

> **Note for M1/M2/M3/M4**: Native ARM64 build is automatically detected and optimized for Apple Silicon.

#### Troubleshooting on macOS

If you encounter OpenSSL linking issues:

```bash
export OPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
make release -j$(sysctl -n hw.ncpu)
```

If CMake cannot find `zmq.hpp`:

```bash
brew reinstall zeromq cppzmq
```

### Windows (MSYS2)

#### Install MSYS2

1. Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/)
2. Open MSYS2 MinGW 64-bit terminal

#### Install Dependencies

```bash
# Update package database
pacman -Syu

# Install build tools and dependencies
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-boost mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-zeromq mingw-w64-x86_64-libsodium
```

#### Build

```bash
# Clone repository
git clone https://github.com/Xcash-Tech/xcash-tech-core.git
cd xcash-tech-core

# Build for 64-bit Windows
make release-static-win64

# Binaries in: build/release/bin/
```

For 32-bit Windows:

```bash
make release-static-win32
```

## Advanced Build Options

### Building with Tests

```bash
make release-test
```

> **Note**: `core_tests` may take several hours to complete.

### Debug Build

For development and debugging purposes:

```bash
make debug
```

### Portable Builds

Build binaries that work across different systems:

```bash
# Linux x86_64
make release-static-linux-x86_64

# Linux ARM64
make release-static-linux-armv8

# Linux ARMv7
make release-static-linux-armv7

# Windows 64-bit
make release-static-win64

# Windows 32-bit
make release-static-win32
```

### Custom CMake Configuration

For advanced users who need custom build settings:

```bash
mkdir -p build/release
cd build/release

cmake -D STATIC=ON \
      -D ARCH="x86-64" \
      -D BUILD_64=ON \
      -D CMAKE_BUILD_TYPE=release \
      ../..

make -j$(nproc)
```

#### Common CMake Options

| Option | Values | Description |
|--------|--------|-------------|
| `STATIC` | ON/OFF | Build static binaries |
| `BUILD_64` | ON/OFF | Build 64-bit binaries |
| `BUILD_TESTS` | ON/OFF | Build test suite |
| `ARCH` | "x86-64", "armv8-a", etc. | Target architecture |
| `CMAKE_BUILD_TYPE` | release/debug | Build type |

## Running the Daemon

After building, the `xcashd` daemon binary will be in `build/release/bin/`.

### Basic Usage

```bash
# Run in foreground
./build/release/bin/xcashd

# Run in background
./build/release/bin/xcashd --detach --log-file xcashd.log

# Show all options
./build/release/bin/xcashd --help
```

### Using Configuration File

Create a configuration file (e.g., `xcashd.conf`):

```
data-dir=/var/lib/xcash
log-file=/var/log/xcash/xcashd.log
log-level=1
max-concurrency=4
```

Run with configuration:

```bash
./build/release/bin/xcashd --config-file xcashd.conf
```

### SystemD Service (Linux)

1. Copy service file:
```bash
sudo cp utils/systemd/xcashd.service /etc/systemd/system/
```

2. Copy configuration:
```bash
sudo cp utils/conf/xcashd.conf /etc/
```

3. Create xcash user:
```bash
sudo useradd -r -s /bin/false xcash
sudo mkdir -p /var/lib/xcash
sudo chown xcash:xcash /var/lib/xcash
```

4. Enable and start service:
```bash
sudo systemctl enable xcashd
sudo systemctl start xcashd
sudo systemctl status xcashd
```

## Verification

After building, verify the binaries:

```bash
# Check version
./build/release/bin/xcashd --version

# Run basic tests
make release-test
```

## Troubleshooting

### Common Issues

**Issue**: `fatal error: zmq.hpp: No such file or directory`

**Solution**: Install cppzmq headers:
```bash
# Ubuntu/Debian
sudo apt install libzmq3-dev libcppzmq-dev

# macOS
brew install cppzmq
```

---

**Issue**: Build fails with "Boost not found"

**Solution**: Ensure Boost is properly installed:
```bash
# Ubuntu/Debian
sudo apt install libboost-all-dev

# macOS
brew install boost
```

---

**Issue**: OpenSSL linking errors on macOS

**Solution**: Specify OpenSSL path:
```bash
export OPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl@3
```

---

**Issue**: Insufficient memory during compilation

**Solution**: Reduce parallel jobs:
```bash
make release -j2  # Use only 2 parallel jobs
```

## Performance Tips

- Use `-j$(nproc)` for parallel compilation (Linux)
- Use `-j$(sysctl -n hw.ncpu)` for parallel compilation (macOS)
- Allocate at least 2GB RAM per compilation thread
- SSD storage significantly speeds up compilation

## Getting Help

If you encounter build issues:

1. Check [GitHub Issues](https://github.com/Xcash-Tech/xcash-tech-core/issues)
2. Join [Discord](https://discord.gg/4CAahnd) for community support
3. Review CMake output for specific error messages
4. Ensure all dependencies are correctly installed

---

**Last Updated**: December 2025  
**Xcash Tech Core Version**: Migration Network Branch
