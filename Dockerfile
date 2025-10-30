# Dinari Blockchain - Production Dockerfile
# Multi-stage build for optimized image size

# Stage 1: Build
FROM ubuntu:22.04 AS builder

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Create build directory and compile
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Create dinari user for security
RUN useradd -m -u 1000 -s /bin/bash dinari

# Create necessary directories
RUN mkdir -p /data/dinari && \
    chown -R dinari:dinari /data/dinari

# Copy binary from builder
COPY --from=builder /build/build/dinarid /usr/local/bin/
COPY --from=builder /build/config /etc/dinari/config

# Set permissions
RUN chmod +x /usr/local/bin/dinarid && \
    chown dinari:dinari /usr/local/bin/dinarid

# Switch to dinari user
USER dinari
WORKDIR /home/dinari

# Expose ports
# 9333 - P2P Network (mainnet)
# 9334 - RPC Server (mainnet)
# 19333 - P2P Network (testnet)
# 19334 - RPC Server (testnet)
EXPOSE 9333 9334 19333 19334

# Volume for blockchain data
VOLUME ["/data/dinari"]

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD /usr/local/bin/dinarid --help > /dev/null || exit 1

# Default command
ENTRYPOINT ["/usr/local/bin/dinarid"]
CMD ["--datadir=/data/dinari", "--printtoconsole=1"]
