###########################
# Builder image
###########################
FROM ubuntu:22.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_THREADS=0
ARG CMAKE_BUILD_TYPE=Release

ENV CCACHE_DIR=/ccache

WORKDIR /build

# Install build dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        pkg-config \
        git \
        ccache \
        # Core dependencies
        libzmq3-dev \
        libssl-dev \
        libunbound-dev \
        libsodium-dev \
        libunwind-dev \
        liblzma-dev \
        libreadline-dev \
        libldns-dev \
        libexpat1-dev \
        libpgm-dev \
        # Hardware wallet support
        libhidapi-dev \
        libusb-1.0-0-dev \
        libprotobuf-dev \
        protobuf-compiler \
        libudev-dev \
        # Boost libraries
        libboost-chrono-dev \
        libboost-date-time-dev \
        libboost-filesystem-dev \
        libboost-locale-dev \
        libboost-program-options-dev \
        libboost-regex-dev \
        libboost-serialization-dev \
        libboost-system-dev \
        libboost-thread-dev \
        libboost-context-dev \
        libboost-coroutine-dev \
        # Additional tools
        doxygen \
        graphviz && \
    rm -rf /var/lib/apt/lists/*

# Build and install gtest
RUN apt-get update && \
    apt-get install -y --no-install-recommends libgtest-dev && \
    cd /usr/src/gtest && \
    cmake . && \
    make -j$(nproc) && \
    mv lib/*.a /usr/lib/ || mv libg* /usr/lib/ && \
    rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /build/xcash-core-migration

# Build X-CASH
RUN cd /build/xcash-core-migration && \
    # Remove git references if present
    rm -rf .git .gitmodules && \
    # Determine build threads
    BUILD_THREADS=${BUILD_THREADS:-0} && \
    if [ "${BUILD_THREADS}" -le 0 ]; then BUILD_THREADS=$(nproc); fi && \
    echo "Building with ${BUILD_THREADS} threads..." && \
    # Build
    make release -j${BUILD_THREADS}

###########################
# Production image
###########################
FROM ubuntu:22.04

LABEL maintainer="X-CASH Tech"
LABEL description="X-CASH Core Daemon - Migration Network"

# Install runtime dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        jq \
        netcat-traditional \
        tzdata \
        # Runtime libraries
        libstdc++6 \
        libc6 \
        libgcc-s1 \
        libssl3 \
        libzmq5 \
        libsodium23 \
        libunbound8 \
        libreadline8 \
        libhidapi-libusb0 \
        libusb-1.0-0 \
        # Boost runtime
        libboost-chrono1.74.0 \
        libboost-date-time1.74.0 \
        libboost-filesystem1.74.0 \
        libboost-program-options1.74.0 \
        libboost-regex1.74.0 \
        libboost-serialization1.74.0 \
        libboost-system1.74.0 \
        libboost-thread1.74.0 \
        libboost-context1.74.0 \
        libboost-coroutine1.74.0 && \
    rm -rf /var/lib/apt/lists/*

# Copy binaries from builder
COPY --from=builder /build/xcash-core-migration/build/release/bin/xcashd /usr/local/bin/
COPY --from=builder /build/xcash-core-migration/build/release/bin/xcash-wallet-cli /usr/local/bin/
COPY --from=builder /build/xcash-core-migration/build/release/bin/xcash-wallet-rpc /usr/local/bin/

# Create directories
RUN mkdir -p /data/bc /data/logs && \
    chmod -R 755 /data

# Create non-root user
RUN useradd -m -u 1000 -s /bin/bash xcash && \
    chown -R xcash:xcash /data

WORKDIR /data

# Expose migration network ports
# 58280: P2P
# 58281: RPC
# 58282: ZMQ RPC
EXPOSE 58280 58281 58282

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=120s --retries=3 \
    CMD curl -sf http://localhost:58281/get_info || exit 1

# Default user
USER xcash

# Default command for migration network (testnet mode)
CMD ["xcashd", \
     "--testnet", \
     "--data-dir", "/data/bc", \
     "--rpc-bind-ip", "0.0.0.0", \
     "--rpc-bind-port", "58281", \
     "--p2p-bind-ip", "0.0.0.0", \
     "--p2p-bind-port", "58280", \
     "--zmq-rpc-bind-ip", "0.0.0.0", \
     "--zmq-rpc-bind-port", "58282", \
     "--restricted-rpc", \
     "--confirm-external-bind", \
     "--log-file", "/data/logs/xcash-daemon.log", \
     "--log-level", "1", \
     "--non-interactive"]
