# Xcash Tech Core

**Xcash Tech Core Migration** is a temporary blockchain network implementation with **leader-based consensus** for controlled migration from legacy chain to new infrastructure.

## Overview

This branch implements a **temporary consensus mechanism** that allows controlled block production during the X-CASH blockchain migration process:

- **Leader/Follower Architecture**: One designated leader node generates blocks, follower nodes validate
- **Ed25519 Signature Validation**: All blocks are cryptographically signed and verified
- **MAINNET Compatible**: Uses production network configuration with internal consensus override
- **Time-Based Block Generation**: Leader produces blocks on fixed 30-second intervals
- **Authorized Seed Nodes**: Only pre-configured seed addresses can act as leaders

> ⚠️ **IMPORTANT**: This is a temporary migration solution. The production network will use full DPoPS consensus.

## Quick Start

### Using Docker (Recommended)

#### Prerequisites
- Docker and Docker Compose
- Existing MAINNET blockchain data (LMDB)
- 4GB+ RAM, 2+ CPU cores, 150GB+ disk space
- **Supports**: Linux (x86_64, ARM64), macOS (Intel, Apple Silicon M1/M2/M3/M4)

#### Deploy Leader Node

```bash
# Clone repository
git clone https://github.com/Xcash-Tech/xcash-tech-core.git
cd xcash-tech-core/scripts

# Configure environment
cp .env.prod.example .env
nano .env  # Set NODE_ROLE=leader and add your credentials

# Start node
docker compose -f docker-compose.migration.prod.yml up -d

# View logs
docker compose logs -f
```

#### Deploy Follower Node

```bash
# Configure as follower
cp .env.prod.example .env
nano .env  # Set NODE_ROLE=follower

# Start node
docker compose -f docker-compose.migration.prod.yml up -d
```

See [docs/deployment.md](docs/deployment.md) for detailed deployment instructions.

## 📖 Documentation

For detailed information about the Migration Network:

- **[Getting Started Guide](docs/start.md)** - Architecture, configuration, monitoring, and security
- **[Building from Source](docs/build.md)** - Platform-specific build instructions
- **[Production Deployment](docs/deployment.md)** - Deployment guide for production servers

## 🤝 Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on how to contribute.

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 💬 Support

- **Documentation**: [docs/deployment.md](docs/deployment.md)
- **Issues**: [GitHub Issues](https://github.com/Xcash-Tech/xcash-tech-core/issues)
- **Discord**: [discord.gg/4CAahnd](https://discord.gg/4CAahnd)
- **Email**: support@xcash.tech

## 🔗 Resources

### Documentation
- **Official Website**: [xcash.tech](https://xcash.tech)
- **Documentation**: [docs.xcash.tech](https://docs.xcash.tech)
- **Block Explorer**: [explorer.xcash.tech](https://explorer.xcash.tech)

### Community
- **Discord**: [discord.gg/4CAahnd](https://discord.gg/4CAahnd)
- **Twitter**: [@XcashTech](https://twitter.com/XcashTech)
- **Telegram**: [t.me/xcashtech](https://t.me/xcashtech)
- **GitHub**: [github.com/Xcash-Tech](https://github.com/orgs/Xcash-Tech/repositories)

---

<div align="center">
**Built with ❤️ by the Xcash Tech Team**
</div>
