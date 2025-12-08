# XCash Migration Network - Docker Deployment

This directory contains Docker configuration for running a 3-node XCash migration network cluster.

## Overview

The migration network is a testnet-based isolated network with:
- **Unique network_id**: `0xA0B1C2D3E4F5A6B7C8D9EAFBACBDCE93`
- **Migration ports**: P2P:58280, RPC:58281, ZMQ:58282
- **3 seed nodes**: seed1.xcash.testnet, seed2.xcash.testnet, seed3.xcash.testnet
- **Complete P2P isolation**: Cannot connect to mainnet or legacy testnet
- **Modern Ubuntu 22.04 base** with optimized multi-stage build

## Quick Start

### 1. Build and Start the Cluster

```bash
# Build images and start all 3 nodes
docker-compose -f docker-compose.migration.yml build
docker-compose -f docker-compose.migration.yml up -d

# View logs from all nodes
docker-compose -f docker-compose.migration.yml logs -f

# View logs from specific node
docker-compose -f docker-compose.migration.yml logs -f seed1
```

### 2. Check Node Status

```bash
# Check if nodes are running
docker ps

# Check network connectivity (using xcash user inside container)
docker exec -u xcash xcash-migration-seed1 curl -s http://localhost:58281/get_info | jq

# Enter container shell
docker exec -it -u xcash xcash-migration-seed1 /bin/bash
```

### 3. Stop the Cluster

```bash
# Stop all nodes (preserves data)
docker-compose -f docker-compose.migration.yml stop

# Stop and remove containers (preserves volumes)
docker-compose -f docker-compose.migration.yml down

# Stop and remove everything including volumes
docker-compose -f docker-compose.migration.yml down -v
```

## Network Architecture

```
┌─────────────────────────────────────────────────────┐
│        XCash Migration Network (Isolated)           │
│                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────┐  │
│  │   seed1      │←→│   seed2      │←→│  seed3   │  │
│  │ :58280/81/82 │  │ :58280/81/82 │  │:58280/81 │  │
│  └──────────────┘  └──────────────┘  └──────────┘  │
│                                                      │
│  Host Ports:       Host Ports:       Host Ports:    │
│  58280-58282       58290-58292       58300-58302    │
└─────────────────────────────────────────────────────┘
```

## Port Mapping

| Service | Container Ports   | Host Ports     | Purpose         |
|---------|-------------------|----------------|-----------------|
| seed1   | 58280/58281/58282 | 58280-58282    | P2P/RPC/ZMQ     |
| seed2   | 58280/58281/58282 | 58290-58292    | P2P/RPC/ZMQ     |
| seed3   | 58280/58281/58282 | 58300-58302    | P2P/RPC/ZMQ     |

## Data Volumes

Each node has persistent storage mounted at `/data`:
- `xcash-migration-seed1-data` → `/data` in seed1 (blockchain in `/data/bc`, logs in `/data/logs`)
- `xcash-migration-seed2-data` → `/data` in seed2
- `xcash-migration-seed3-data` → `/data` in seed3

## RPC Examples

### Connect to seed1 RPC:
```bash
curl -X POST http://localhost:58281/json_rpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":"0","method":"get_info"}'
```

### Connect to seed2 RPC:
```bash
curl -X POST http://localhost:58291/json_rpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":"0","method":"get_info"}'
```

### Connect to seed3 RPC:
```bash
curl -X POST http://localhost:58301/json_rpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":"0","method":"get_info"}'
```

## Build Configuration

The Dockerfile supports build arguments:

```bash
docker-compose -f docker-compose.migration.yml build \
  --build-arg BUILD_THREADS=8 \
  --build-arg CMAKE_BUILD_TYPE=Release
```

- `BUILD_THREADS`: Number of threads for compilation (0=auto-detect)
- `CMAKE_BUILD_TYPE`: Release (default), Debug, or RelWithDebInfo

## Troubleshooting

### View detailed logs:
```bash
docker logs xcash-migration-seed1 -f --tail 100
```

### Enter container shell as xcash user:
```bash
docker exec -it -u xcash xcash-migration-seed1 /bin/bash
```

### Enter as root (if needed):
```bash
docker exec -it -u root xcash-migration-seed1 /bin/bash
```

### Check health status:
```bash
docker inspect xcash-migration-seed1 | jq '.[0].State.Health'
```

### View blockchain data:
```bash
docker exec -u xcash xcash-migration-seed1 ls -lah /data/bc/testnet
```

### Reset everything:
```bash
docker-compose -f docker-compose.migration.yml down -v
docker volume prune -f
docker system prune -a -f
```

## Security Features

- **Non-root user**: Daemon runs as `xcash` user (UID 1000)
- **Restricted RPC**: Enabled by default for security
- **Health checks**: Automatic container health monitoring
- **Network isolation**: Bridge network with no external exposure except mapped ports

## Next Steps - Phase 2

Once Phase 1 is validated:
1. Add `--temp-consensus-enabled` flag
2. Add `--temp-consensus-leader` flag to seed1
3. Implement leader service with 5-minute slot scheduler
4. Add stub validation (reject mode)

See `openspec/changes/add-temp-leader-consensus-migration-net/tasks.md` for full roadmap.
