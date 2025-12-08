# Migration Network Configuration Update

## Summary of Changes (Dec 8, 2025)

### 1. Domain Change: xcash.tech → xcash.testnet ✅

**Files modified:**
- `src/cryptonote_config.h`
  - SEED_NODE_1-5: `seed*.xcash.tech` → `seed*.xcash.testnet`
  - NETWORK_DATA_NODE_IP_ADDRESS_1-5: Updated to xcash.testnet
- `src/p2p/net_node.inl`
  - Testnet seed nodes: Updated to xcash.testnet:58280

### 2. Port Reconfiguration: 18290-18292 → 58280-58282 ✅

**Migration network ports:**
- P2P: 18290 → **58280**
- RPC: 18291 → **58281**
- ZMQ: 18292 → **58282**

**Files modified:**
- `src/cryptonote_config.h` - testnet namespace port constants
- `src/p2p/net_node.inl` - seed node addresses
- `docker-compose.migration.yml` - all port mappings
- `Dockerfile` - default CMD ports
- `MIGRATION_NETWORK.md` - documentation

### 3. Dockerfile Modernization ✅

**Complete rewrite based on reference Dockerfile:**

**Old (Ubuntu 18.04):**
- Single-stage build
- Clones from GitHub during build
- Downloads external blockchain
- Runs as root
- No health checks
- Basic package management

**New (Ubuntu 22.04):**
- **Multi-stage build** (builder + production)
- Uses local source code
- Optimized layer caching with ccache
- **Non-root user** (xcash:1000)
- **Health checks** integrated
- Modern dependencies (Boost 1.74, libssl3)
- Structured data directories (`/data/bc`, `/data/logs`)
- Build arguments support (BUILD_THREADS, CMAKE_BUILD_TYPE)

### 4. Docker Compose Enhancements ✅

**Updates:**
- Modern hostnames: `seed*.xcash.testnet`
- Host port mapping:
  - seed1: 58280-58282
  - seed2: 58290-58292
  - seed3: 58300-58302
- Volume paths: `/root/.xcash` → `/data`
- Added health checks
- Added build args
- Explicit data-dir configuration
- Improved command structure

### 5. Documentation Update ✅

**MIGRATION_NETWORK.md:**
- Updated all port references
- Updated domain names
- Added security features section
- Added build configuration guide
- Enhanced troubleshooting with non-root user examples
- Updated network architecture diagram

### 6. Proposal Update ✅

**tasks.md:**
- Updated task 1.2 with new ports (58280/58281/58282)
- Updated task 1.3 with xcash.testnet domains
- Added task 1.8 for Dockerfile modernization
- Updated task 1.6 data path reference

## Validation ✅

```bash
openspec validate add-temp-leader-consensus-migration-net --strict
# Result: Change 'add-temp-leader-consensus-migration-net' is valid
```

## Testing Instructions

### Build and Run:
```bash
cd /Users/mike/Documents/proj/xcash/src/xcash-core-migration

# Build all images
docker-compose -f docker-compose.migration.yml build

# Start cluster
docker-compose -f docker-compose.migration.yml up -d

# Check status
docker ps
docker logs xcash-migration-seed1 -f
```

### Verify Configuration:
```bash
# Check seed1 RPC on new port
curl http://localhost:58281/get_info

# Check seed2 RPC on new port
curl http://localhost:58291/get_info

# Check seed3 RPC on new port
curl http://localhost:58301/get_info
```

### Verify P2P Connectivity:
```bash
# Enter seed1
docker exec -u xcash -it xcash-migration-seed1 /bin/bash

# Inside container, check peers (should see seed2 and seed3)
curl -s http://localhost:58281/get_info | jq '.outgoing_connections_count, .incoming_connections_count'
```

## Next Phase

Phase 1 is complete. Ready to proceed with **Phase 2**:
- Implement `--temp-consensus-enabled` flag
- Implement `--temp-consensus-leader` flag
- Create leader service with 5-minute slot scheduler
- Add stub validation (reject mode)

## Files Changed

```
modified:   Dockerfile
modified:   MIGRATION_NETWORK.md
modified:   docker-compose.migration.yml
modified:   openspec/changes/add-temp-leader-consensus-migration-net/tasks.md
modified:   src/cryptonote_config.h
modified:   src/p2p/net_node.inl
```
