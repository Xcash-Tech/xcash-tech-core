# Change: Temporary Leader-Based Consensus & Migration Network for XCash

## Why
The XCash blockchain is transitioning from the legacy Monero-based CryptoNote implementation to a new modern architecture. To ensure a safe, transparent, and auditable migration without disrupting the existing network, we need a temporary consensus mechanism and parallel migration network that allows for thorough testing and gradual transition of state while maintaining the integrity of the current production blockchain.

## What Changes
- **Add temporary leader-based consensus mechanism** that allows designated trusted nodes to produce blocks during the migration phase
- **Add migration network infrastructure** running in parallel to the production network with controlled state synchronization
- **Add state verification and checkpoint system** to validate migrated blockchain state against the original chain
- **Add leader rotation and failover** mechanisms to ensure continued operation during migration
- **Add migration monitoring and auditing** tools for transparency and verification
- **Add rollback capabilities** for safe recovery if issues are detected

## Impact
- **Affected specs:**
  - New: `consensus` (temporary leader-based consensus)
  - New: `migration-network` (parallel network infrastructure)
  
- **Affected code:**
  - `src/cryptonote_core/` - Core consensus logic modifications
  - `src/cryptonote_protocol/` - Protocol additions for migration network
  - `src/blockchain_db/` - State export/import capabilities
  - `src/p2p/` - Network segregation and dual-network support
  - `src/rpc/` - Migration monitoring endpoints
  - `src/daemon/` - Migration mode support
  - New: `src/migration/` - Migration orchestration and validation

- **Deployment phases:**
  1. Phase 1: Deploy migration network infrastructure alongside production
  2. Phase 2: Begin state synchronization and validation
  3. Phase 3: Run parallel networks with cross-validation
  4. Phase 4: Gradual traffic migration with monitoring
  5. Phase 5: Full cutover and decommission of legacy network

## Non-Goals
- This is **NOT** the final DPOPS consensus mechanism (that comes later)
- This is **NOT** a permanent solution (temporary migration tool only)
- This does **NOT** change the production network until fully validated
- This does **NOT** require consensus changes on the legacy network

## Success Criteria
- Migration network runs stably for extended test period (30+ days)
- State synchronization maintains 100% accuracy with production chain
- Leader consensus produces blocks reliably with <60 second block time
- All stakeholders can independently verify migration progress
- Rollback mechanism tested and proven functional
- Performance benchmarks meet or exceed legacy network

# Change Proposal: Temporary Leader-Based Consensus & Migration Network for XCash

- Change ID: `add-temp-leader-consensus-migration-net`
- Status: Draft
- Owners: TBD
- Related Components: `daemon`, `core`, `p2p`, consensus integration, deployment tooling

## 1. Summary

We want to spin up a temporary, leader-based consensus on top of our Monero-derived XCash daemon and run it on a dedicated migration network that uses a copy of the mainnet LMDB but is completely isolated at the P2P level.

The goal is to keep a “live” chain for migration and testing while we replace the existing external consensus module, without any risk that newly produced blocks escape into the production network.

## 2. Background & Motivation

Today:

- XCash is a Monero fork.
- Block acceptance in `monerod` is patched to call an external consensus module:
  - the external service chooses a block producer,
  - builds a block from a template,
  - pushes the block into the network,
  - nodes call the module before accepting blocks.
- The external consensus module is unstable and will be replaced by a different implementation.

We need:

- A temporary, simpler consensus mechanism implemented inside the daemon.
- A separate migration network (`MIGRATION_NET`) running off a snapshot/copy of the mainnet LMDB.
- A safe, isolated environment to test and gradually transition state.

## 3. Goals

- Define a new network type (`MIGRATION_NET`) with a unique network_id, ports, and seed nodes.
- Introduce temporary leader-based consensus:
  - a fixed leader node produces blocks,
  - followers verify leader signatures instead of calling the external module.
- Implement leader metadata in blocks: leader_id + leader_signature.
- Add an internal block generator service running only on the leader.
- Add block validation logic for migration-mode nodes.
- Support deployment using a copied LMDB snapshot.

## 4. High-Level Design

### 4.1 MIGRATION_NET

- New network type with a distinct network_id and unique ports.
- Dedicated seed nodes (internal only).
- Reject P2P connections from any node not matching migration network_id.
- Runs on separate data-dir (`~/.xcash/migration`).

### 4.2 Temporary Leader-Based Consensus

- Enabled only when `--temp-consensus-enabled` AND `--network-type=migration`.
- One leader node configured with:
  - `--temp-consensus-leader`
  - `--leader-id`
  - `--leader-pubkey`
  - `--leader-miner-address`
- Leader produces blocks using `get_block_template` + `handle_block_found`.
- Followers check leader metadata and accept/reject blocks accordingly.

### 4.3 Leader Metadata Encoding

`miner_tx.extra` extended with a new extra-tag:

```
[tag:1][leader_id_len:1][leader_id:leader_id_len][signature:fixed_len]
```

- leader_id: short identifier or pubkey hash  
- signature: crypto::signature over block hash

### 4.4 Leader Block Generator Service

A persistent background service:

1. Waits until node is synchronized.
2. Periodically calls `get_block_template`.
3. Adds leader metadata.
4. Performs PoW if configured (`--temp-consensus-with-pow`), otherwise adds deterministic nonce.
5. Submits block via `handle_block_found`.

### 4.5 Block Validation

When in MIGRATION_NET + temp consensus:

- Extract leader metadata.
- Validate leader_id.
- Verify signature using leader pubkey.
- Skip external consensus module call.

Blocks failing validation are rejected.

### 4.6 Deployment Overview

- Copy mainnet LMDB to migration data directory.
- Start leader and follower nodes with migration network config.
- Ensure P2P isolation using network_id + ports + firewall rules.
- Migration network produces blocks safely isolated from production.

## 5. Non-Goals

- Not a replacement for the future DPOPS consensus.
- Not intended for production use.
- Does not modify mainnet consensus.
- No changes to monetary policy or block format beyond extra metadata.

## 6. Success Metrics

- Migration network runs stably for extended test periods.
- Leader produces blocks consistently (<60s block time target).
- All nodes validate and sync correctly.
- No accidental connections to mainnet.
- Migration audits are transparent and reproducible.

## 7. Risks

- Wrong leader key configuration → all blocks rejected.
- Misconfigured network_id could accidentally bridge networks.
- Leader downtime halts block production.

## 8. Rollout Plan (Prioritized & Environment-Specific)

### Phase 1 — Local MIGRATION_NET bootstrap (Docker, 3 nodes)
- Define MIGRATION_NET with unique network_id, ports, and seed nodes.
- Configure three local seeds: `seed1.xcash.tech`, `seed2.xcash.tech`, `seed3.xcash.tech` (Docker hostnames).
- Ensure MIGRATION_NET cannot handshake with MAINNET (magic bytes + ports).
- Bring up a 3-node local cluster using Docker Compose.
- Validate:
  - nodes discover each other via the three seed hosts,
  - no accidental links to mainnet,
  - LMDB loads correctly from copied state.

### Phase 2 — Leader mode with validation stub
- Implement leader service (no real validation yet).
- Implement block validation stub:
  - follower nodes log receipt of a block,
  - block is always rejected (not added to chain),
  - no signature verification yet.
- Run local MIGRATION_NET:
  - ensure block generator triggers on 5-minute slots,
  - confirm followers see blocks and log stub messages.

### Phase 3 — Enable real leader consensus validation
- Replace stub with actual `check_temp_leader_consensus`.
- Followers must:
  - extract leader metadata,
  - verify leader_id,
  - verify leader signature,
  - accept block only if valid.
- Run multi-node local network tests:
  - leader produces blocks,
  - followers accept and sync,
  - slot-missed behavior is correct.

### Phase 4 — Server deployment (production-like environment)
**Important:**  
On servers the network MUST use **existing mainnet identifiers, ports, and seeds**, preserving compatibility with historical XCash infrastructure.

- One server = one node (Docker or systemd).
- Use the same LMDB as legacy nodes.
- Enable temporary consensus flags on leader/followers.
- MIGRATION_NET is NOT used on servers — they run MAINNET identifiers with temporary consensus enabled.
- Validate:
  - nodes operate identically to the old network from the outside,
  - block production follows 5-minute schedule,
  - temporary consensus is fully functional and isolated to internal logic.

### Phase 5 — Monitoring & stability verification
- Observe cluster for multi-day operation.
- Validate logs, signature checks, block timings.
- Prepare the network for final migration phase (outside of this proposal).