# Production Deployment Guide - X-CASH Migration Network

This guide explains how to deploy X-CASH nodes with temporary leader consensus on production servers.

## Architecture Overview

**IMPORTANT**: Production nodes use **MAINNET** configuration (network_id, ports, seeds) but with temporary consensus enabled internally. This ensures:
- External compatibility with existing mainnet infrastructure
- No P2P cross-talk with test/migration networks
- Shared LMDB with legacy chain
- Controlled consensus via leader/follower model

## Prerequisites

- Docker and Docker Compose installed
- Access to existing MAINNET blockchain data (LMDB)
- For leader node: DPoS Ed25519 keypair
- Minimum 4GB RAM, 2 CPU cores
- 150GB+ disk space for blockchain data

## Quick Start

### 1. Prepare Environment

```bash
# Clone repository
git clone https://github.com/Xcash-Tech/xcash-tech-core.git
cd xcash-tech-core/migration

# Copy environment template
cp .env.prod.example .env

# Edit .env with your configuration
nano .env
```

### 2. Configure Node

Edit `.env` file:

**For Leader Node (seed1):**
```bash
NODE_NAME=seed1
NODE_ROLE=leader
DELEGATE_PUBLIC_ADDRESS=XCA1SkZTKvC1vMR3vzdsBiFY69RXHJYw2TZ7jPTtsFGCUj7JfEaDXuSMBXrhaGHf7QJH5PyBxQK2K7gNs9pYGBGs7zfS7cPHd9
DELEGATE_SECRET_KEY=7a88c31fdb91e596400173a39ac60f048063bd4b3e137001d8efa2e5fcb2ee27707a57d0ec077e1ae594c6c7afac203aa8a720713e5ce7258b0d6cd33923e1e2
DATA_DIR=/var/lib/xcash
LOG_DIR=/var/log/xcash
```

**For Follower Node (seed2/3/4):**
```bash
NODE_NAME=seed2
NODE_ROLE=follower
DATA_DIR=/var/lib/xcash
LOG_DIR=/var/log/xcash
```

### 3. Start Node

```bash
# Pull latest image
docker pull xcashtech/xcash-core-migration:latest

# Start node
docker compose -f docker-compose.migration.prod.yml up -d

# View logs
docker compose -f docker-compose.migration.prod.yml logs -f
```

## Configuration Details

### Network Settings (MAINNET)

| Parameter | Value | Description |
|-----------|-------|-------------|
| Network ID | MAINNET | Uses production network identifier |
| P2P Port | 18280 | Peer-to-peer communication |
| RPC Port | 18281 | RPC API endpoint |
| ZMQ Port | 18282 | ZeroMQ messaging |

### Node Roles

#### Leader Node
- Generates blocks on 30-second slots (configurable)
- Signs blocks with Ed25519 keypair
- Requires `DELEGATE_PUBLIC_ADDRESS` and `DELEGATE_SECRET_KEY`
- Must be one of authorized seed addresses (1-4)

#### Follower Node
- Validates blocks from leader
- Verifies Ed25519 signatures
- Syncs blockchain from leader and peers
- No credentials required

### Authorized Leader Addresses

Only these addresses can act as leaders (defined in `cryptonote_config.h`):

1. `NETWORK_DATA_NODE_PUBLIC_ADDRESS_1` - seed1
2. `NETWORK_DATA_NODE_PUBLIC_ADDRESS_2` - seed2
3. `NETWORK_DATA_NODE_PUBLIC_ADDRESS_3` - seed3
4. `NETWORK_DATA_NODE_PUBLIC_ADDRESS_4` - seed4

## Building Custom Images

### Build Multi-Architecture Image

```bash
# Build and push to Docker Hub
./build-and-push.sh

# Build with custom tag
./build-and-push.sh --tag v1.0.0

# Build without cache
./build-and-push.sh --no-cache
```

### Build for Single Architecture

```bash
# For amd64 only
./build-and-push.sh --platform linux/amd64

# For arm64 only (e.g., AWS Graviton)
./build-and-push.sh --platform linux/arm64
```

## Monitoring

### Health Checks

```bash
# Check node status
docker compose -f docker-compose.migration.prod.yml ps

# Check RPC endpoint
curl http://localhost:58281/get_info
```

### Log Monitoring

```bash
# Follow logs
docker logs -f xcash-migration-seed1

# View specific log files
tail -f /var/log/xcash/xcash-migration.log
```

### Key Metrics to Monitor

- Block height synchronization
- Block generation timing (leader: every 30s)
- Signature validation success rate
- Peer connection count
- Memory and CPU usage

## Troubleshooting

### Leader Not Generating Blocks

1. Check delegate credentials in `.env`
2. Verify address is authorized (one of seed1-4)
3. Check system time synchronization (NTP)
4. Review logs for signature test failures

```bash
docker logs xcash-migration-seed1 | grep "Signature test"
```

### Follower Rejecting Blocks

1. Check `TEMPORARY_CONSENSUS_ACTIVATION_HEIGHT` (default: 1085612)
2. Verify Ed25519 pubkeys match in `cryptonote_config.h`
3. Check logs for signature verification errors

```bash
docker logs xcash-migration-seed2 | grep "REJECT"
```

### Data Directory Issues

1. Ensure LMDB data exists at `DATA_DIR`
2. Check directory permissions (Docker needs read/write)
3. Verify sufficient disk space

```bash
# Check disk usage
df -h /var/lib/xcash

# Check permissions
ls -la /var/lib/xcash
```

## Security Considerations

### Protect Secret Keys

1. **Never commit `.env` to git** (already in `.gitignore`)
2. Store secret keys in secure vault (e.g., HashiCorp Vault, AWS Secrets Manager)
3. Use environment-specific configurations
4. Rotate keys periodically

### Network Security

1. Use firewall rules to restrict RPC access
2. Enable TLS for RPC endpoints (not yet implemented)
3. Monitor for unauthorized access attempts
4. Keep Docker images updated

### Recommended Firewall Rules

```bash
# P2P (required for all peers)
ufw allow 18280/tcp

# RPC (restrict to trusted IPs)
ufw allow from TRUSTED_IP to any port 18281

# ZMQ (restrict to trusted IPs)
ufw allow from TRUSTED_IP to any port 18282
```

## Backup and Recovery

### Blockchain Data

```bash
# Stop node
docker compose -f docker-compose.migration.prod.yml down

# Backup LMDB
tar -czf xcash-lmdb-backup-$(date +%Y%m%d).tar.gz -C /var/lib/xcash .

# Restore LMDB
tar -xzf xcash-lmdb-backup-YYYYMMDD.tar.gz -C /var/lib/xcash

# Start node
docker compose -f docker-compose.migration.prod.yml up -d
```

### Configuration Backup

```bash
# Backup environment config
cp .env .env.backup-$(date +%Y%m%d)
```

## Upgrading

### Upgrade Docker Image

```bash
# Pull latest image
docker pull xcashtech/xcash-core-migration:latest

# Recreate container
docker compose -f docker-compose.migration.prod.yml up -d --force-recreate
```

### Rollback

```bash
# Use specific version
docker pull xcashtech/xcash-core-migration:COMMIT_HASH

# Update docker-compose.yml to use specific tag
# Then recreate container
docker compose -f docker-compose.migration.prod.yml up -d --force-recreate
```

## Support

- **Documentation**: See `openspec/changes/add-temp-leader-consensus-migration-net/`
- **Issues**: https://github.com/Xcash-Tech/xcash-tech-core/issues
- **Discord**: https://discord.gg/xcash

## License

MIT License - See LICENSE file for details
