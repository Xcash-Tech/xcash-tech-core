# Getting Started with Xcash Tech Core Migration Network

This guide covers the architecture, configuration, monitoring, and security of the Xcash Tech Core Migration Network.

## 🏗️ Architecture

### Overview

The Migration Network uses a **temporary leader-based consensus** mechanism for controlled block production during the blockchain migration process. This ensures:

- **External compatibility** with existing mainnet infrastructure
- **No P2P cross-talk** with test/migration networks
- **Shared LMDB** with legacy chain
- **Controlled consensus** via leader/follower model

### Consensus Model

#### Leader Node

The leader node is responsible for generating new blocks:

- **Block Generation**: Creates blocks on fixed 30-second intervals (configurable)
- **Block Signing**: Signs each block with Ed25519 private key
- **Authorization**: Must be one of 4 authorized seed addresses
- **Requirements**: 
  - `DELEGATE_PUBLIC_ADDRESS` - Public address of the delegate
  - `DELEGATE_SECRET_KEY` - Private key for signing blocks

#### Follower Nodes

Follower nodes validate and sync blocks from the leader:

- **Block Validation**: Receive and verify blocks from leader
- **Signature Verification**: Verify Ed25519 signatures against authorized public keys
- **Security**: Reject blocks from unauthorized leaders
- **Synchronization**: Sync blockchain from leader and other peers
- **No Credentials Required**: Follower nodes don't need delegate keys

### Network Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| **Network ID** | MAINNET | Production network identifier |
| **P2P Port** | 18280 | Peer-to-peer communication |
| **RPC Port** | 18281 | RPC API endpoint |
| **ZMQ Port** | 18282 | ZeroMQ messaging |
| **Activation Height** | 1085612 | Block height when consensus activates |
| **Block Time** | 30 seconds | Time between blocks (leader) |

### Authorized Leaders

Only these seed addresses can act as leaders (defined in `src/cryptonote_config.h`):

1. **seed1**: `NETWORK_DATA_NODE_PUBLIC_ADDRESS_1`
2. **seed2**: `NETWORK_DATA_NODE_PUBLIC_ADDRESS_2`
3. **seed3**: `NETWORK_DATA_NODE_PUBLIC_ADDRESS_3`
4. **seed4**: `NETWORK_DATA_NODE_PUBLIC_ADDRESS_4`

These addresses are hardcoded in the consensus validation logic to prevent unauthorized block production.

## ⚙️ Configuration

### Environment Variables

Configuration is managed through environment variables in the `.env` file.

#### Leader Node Configuration

```bash
# Node identification
NODE_NAME=seed1
NODE_ROLE=leader

# Delegate credentials (REQUIRED for leader)
DELEGATE_PUBLIC_ADDRESS=<your-delegate-public-address>
DELEGATE_SECRET_KEY=<your-delegate-secret-key>

# Data directories
DATA_DIR=/var/lib/xcash
LOG_DIR=/var/log/xcash

# Optional: Customize ports if needed
# P2P_PORT=18280
# RPC_PORT=18281
# ZMQ_PORT=18282
```

#### Follower Node Configuration

```bash
# Node identification
NODE_NAME=seed2
NODE_ROLE=follower

# Data directories
DATA_DIR=/var/lib/xcash
LOG_DIR=/var/log/xcash

# No delegate credentials needed for followers
```

### Configuration Files

#### Docker Compose

For production deployment, use `docker-compose.migration.prod.yml`:

```bash
# Start node
docker compose -f docker-compose.migration.prod.yml up -d

# View logs
docker compose -f docker-compose.migration.prod.yml logs -f

# Stop node
docker compose -f docker-compose.migration.prod.yml down
```

#### xcashd Configuration

Direct `xcashd` configuration file (optional):

```conf
# /etc/xcashd.conf
data-dir=/var/lib/xcash
log-file=/var/log/xcash/xcashd.log
log-level=1
max-concurrency=4
p2p-bind-port=18280
rpc-bind-port=18281
zmq-rpc-bind-port=18282
```

Run with config file:

```bash
./xcashd --config-file /etc/xcashd.conf
```

### Delegate Key Generation

> ⚠️ **Security Warning**: Generate keys securely and store them safely.

For testing purposes, keys can be generated using the provided utilities. In production, use hardware security modules (HSM) or secure key management systems.

## 📊 Monitoring

### Health Checks

#### RPC Endpoint

Check node status via RPC:

```bash
# Get general node information
curl http://localhost:18281/get_info

# Example response:
# {
#   "height": 1085620,
#   "target_height": 1085620,
#   "difficulty": 0,
#   "tx_count": 12345,
#   "tx_pool_size": 0,
#   ...
# }
```

#### Daemon Status

Check daemon status directly:

```bash
# If running from source
./build/release/bin/xcashd status

# Docker
docker exec xcash-migration-seed1 xcashd status
```

#### Log Monitoring

Monitor logs in real-time:

```bash
# Docker logs
docker logs -f xcash-migration-seed1

# File logs
tail -f /var/log/xcash/xcash-migration.log

# Filter for errors
tail -f /var/log/xcash/xcash-migration.log | grep ERROR

# Filter for consensus events
tail -f /var/log/xcash/xcash-migration.log | grep "CONSENSUS\|LEADER\|SIGNATURE"
```

### Key Metrics to Monitor

#### Block Production (Leader)

- **Block generation interval**: Should be ~30 seconds
- **Missed blocks**: Any gaps indicate issues
- **Signature test results**: Should always pass

```bash
# Watch for block generation
tail -f /var/log/xcash/xcash-migration.log | grep "Block generated"
```

#### Block Validation (Follower)

- **Block reception**: Receiving blocks from leader
- **Signature validation**: All blocks should validate successfully
- **Rejected blocks**: Should be zero from authorized leader

```bash
# Watch for block validation
tail -f /var/log/xcash/xcash-migration.log | grep "Block validated\|REJECT"
```

#### Network Health

- **Peer connections**: Should maintain stable connections to seed nodes
- **Sync status**: Height should match network
- **Network latency**: Monitor P2P communication delays

```bash
# Check peer connections
curl http://localhost:18281/get_connections | jq
```

#### System Resources

- **Memory usage**: Monitor for memory leaks
- **CPU usage**: Should be moderate and stable
- **Disk I/O**: LMDB operations
- **Disk space**: Blockchain data growth

```bash
# Docker stats
docker stats xcash-migration-seed1

# System resources
top -p $(pgrep xcashd)
```

### Monitoring Tools

#### Prometheus + Grafana (Recommended)

Set up monitoring stack for production:

```bash
# Example prometheus configuration
# Add to prometheus.yml
scrape_configs:
  - job_name: 'xcash-node'
    static_configs:
      - targets: ['localhost:18281']
```

#### Simple Health Check Script

```bash
#!/bin/bash
# health-check.sh

RESPONSE=$(curl -s http://localhost:18281/get_info)
HEIGHT=$(echo $RESPONSE | jq -r '.height')
TARGET=$(echo $RESPONSE | jq -r '.target_height')

if [ "$HEIGHT" -eq "$TARGET" ]; then
    echo "✓ Node is synced: $HEIGHT"
    exit 0
else
    echo "✗ Node is syncing: $HEIGHT / $TARGET"
    exit 1
fi
```

## 🔒 Security

### Protect Secret Keys

#### Storage Best Practices

1. **Never commit to version control**
   - Add `.env` to `.gitignore` (already done)
   - Never hardcode keys in source code or config files
   - Use separate configurations for different environments

2. **Use secure key management systems**
   - **HashiCorp Vault**: For enterprise deployments
   - **AWS Secrets Manager**: For AWS infrastructure
   - **Azure Key Vault**: For Azure infrastructure
   - **Hardware Security Modules (HSM)**: For maximum security

3. **Environment-specific configurations**
   - Production keys separate from development/test
   - Limit key access to essential personnel only
   - Use role-based access control (RBAC)

4. **Key rotation**
   - Rotate keys periodically (e.g., every 90 days)
   - Have key rotation procedure documented
   - Plan for emergency key rotation

#### Example: Using HashiCorp Vault

```bash
# Store secret in Vault
vault kv put secret/xcash/seed1 \
  delegate_public_address="XCA1..." \
  delegate_secret_key="7a88c31f..."

# Retrieve and use in environment
export DELEGATE_SECRET_KEY=$(vault kv get -field=delegate_secret_key secret/xcash/seed1)
```

### Network Security

#### Firewall Configuration

Restrict access to sensitive ports:

```bash
# Ubuntu/Debian with UFW
sudo ufw default deny incoming
sudo ufw default allow outgoing

# Allow P2P from anywhere (required for blockchain sync)
sudo ufw allow 18280/tcp comment "Xcash P2P"

# Allow RPC only from trusted IPs
sudo ufw allow from 10.0.0.0/8 to any port 18281 comment "Xcash RPC - Internal"
sudo ufw allow from 203.0.113.0/24 to any port 18281 comment "Xcash RPC - Office"

# Allow ZMQ only from monitoring/internal systems
sudo ufw allow from 10.0.0.0/8 to any port 18282 comment "Xcash ZMQ - Internal"

# Enable firewall
sudo ufw enable
```

#### CentOS/RHEL with firewalld

```bash
# Allow P2P
firewall-cmd --permanent --add-port=18280/tcp
firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="10.0.0.0/8" port protocol="tcp" port="18281" accept'
firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="10.0.0.0/8" port protocol="tcp" port="18282" accept'
firewall-cmd --reload
```

#### TLS/SSL for RPC

> Note: Native TLS support is planned. Currently, use reverse proxy.

```nginx
# Nginx reverse proxy with TLS
server {
    listen 443 ssl http2;
    server_name rpc.example.com;

    ssl_certificate /etc/letsencrypt/live/rpc.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/rpc.example.com/privkey.pem;

    location / {
        proxy_pass http://127.0.0.1:18281;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

### Access Control

#### SSH Hardening

```bash
# /etc/ssh/sshd_config
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
Port 2222  # Non-standard port
AllowUsers xcash-admin
```

#### Audit Logging

Enable system audit logging:

```bash
# Install auditd
sudo apt install auditd

# Monitor xcashd binary
sudo auditctl -w /usr/local/bin/xcashd -p x -k xcash-execution

# Monitor config files
sudo auditctl -w /etc/xcashd.conf -p wa -k xcash-config
```

### Security Monitoring

#### Failed Connection Attempts

Monitor for suspicious connection patterns:

```bash
# Watch for connection failures
tail -f /var/log/xcash/xcash-migration.log | grep "connection failed\|refused"
```

#### Unauthorized Block Attempts

Alert on unauthorized leader blocks:

```bash
# Monitor for signature validation failures
tail -f /var/log/xcash/xcash-migration.log | grep "REJECT\|Invalid signature"
```

### Incident Response

#### Compromise Detection

Signs of potential compromise:

- Unexpected block rejections
- Unusual network traffic patterns
- Unauthorized configuration changes
- Suspicious log entries

#### Response Procedure

1. **Immediate Actions**
   - Isolate affected node from network
   - Stop the `xcashd` process
   - Preserve logs for forensic analysis

2. **Investigation**
   - Review system logs
   - Check for unauthorized access
   - Verify blockchain integrity

3. **Recovery**
   - Rotate all credentials
   - Rebuild system from known-good state
   - Restore from backup if necessary
   - Update security measures

### Reporting Vulnerabilities

If you discover a security vulnerability:

1. **Do NOT** disclose publicly
2. Email: **security@xcash.tech**
3. Include:
   - Description of the vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fixes (if any)

We aim to respond within 48 hours and will work with you to address the issue.

## 🚀 Next Steps

- **Production Deployment**: See [deployment.md](deployment.md)
- **Build from Source**: See [build.md](build.md)
- **Troubleshooting**: Check logs and monitoring metrics
- **Community Support**: Join [Discord](https://discord.gg/4CAahnd)

---

**Last Updated**: December 2025  
**Xcash Tech Core Version**: Migration Network Branch
