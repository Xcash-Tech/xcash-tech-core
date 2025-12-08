# Design: Temporary Leader-Based Consensus & Migration Network for XCash

This document describes the implementation design for the OpenSpec change  
`add-temp-leader-consensus-migration-net`.

The goal is to introduce a **temporary, single-leader consensus** and an **isolated migration network** (for local testing), while keeping the production network compatible with the existing XCash mainnet identifiers and LMDB.

---

## 1. Context

XCash is a Monero-based chain with a patched `xcashd` that currently relies on an **external consensus module** for block validation and production:

- The external service chooses a block producer.
- It builds a block from a template.
- It pushes the block into the network.
- Nodes call the module before accepting blocks.

This external module is unstable and is scheduled for replacement. We need a **minimal, temporary in-daemon consensus** implementation to:

- keep a live network for migration and testing,
- avoid depending on the external module,
- maintain compatibility with the existing chain and infrastructure.

The design supports two main environments:

1. **Local MIGRATION_NET** (Docker, 3 nodes) for development and testing.  
2. **Production-like deployment** using MAINNET identifiers on real servers.

---

## 2. Goals & Non-Goals

### 2.1 Goals

- Introduce a **MIGRATION_NET** network type for local testing:
  - distinct `network_id`, ports, and seed nodes (`seed1/2/3.xcash.tech`),
  - completely isolated from MAINNET.
- Implement a **temporary leader-based consensus**:
  - exactly one configured leader node produces blocks,
  - followers validate blocks using leader metadata and signature.
- Add a **leader block generator service** inside the daemon:
  - uses existing `get_block_template` and `handle_block_found`,
  - produces blocks strictly at 5-minute global time slots.
- Use **leader metadata in coinbase extra**:
  - `leader_id` and `leader_signature` encoded via a dedicated extra tag.
- Provide a **phased rollout**:
  - Phase 1: local MIGRATION_NET bootstrap.
  - Phase 2: leader service + validation stub (blocks always rejected).
  - Phase 3: full leader consensus enforcement.
  - Phase 4: server deployment with MAINNET identifiers.

### 2.2 Non-Goals

- No new long-term consensus algorithm (e.g., DPoS, PBFT).
- No multi-leader rotation or automatic failover.
- No new state export/import mechanisms beyond copying LMDB directories.
- No changes to monetary policy, transaction format, or core economic rules.
- No changes to mainnet consensus behavior when temporary consensus is disabled.

---

## 3. High-Level Architecture

The design adds:

1. A new **MIGRATION_NET** network type:
   - unique `network_id` (magic bytes),
   - dedicated P2P/RPC/ZMQ ports,
   - fixed seed nodes (`seed1/2/3.xcash.tech`).
2. A set of **temporary consensus configuration flags**:
   - `--temp-consensus-enabled`
   - `--temp-consensus-leader`
   - `--leader-id`
   - `--leader-pubkey`
   - `--leader-miner-address`
   - optional: `--temp-consensus-with-pow`
3. **Leader metadata** encoded in `miner_tx.extra` using a new extra tag.
4. A **leader block generator service**:
   - runs only on nodes with `--temp-consensus-leader`,
   - generates blocks at 5-minute boundaries (`timestamp % 300 == 0`).
5. A **leader-based validation hook**:
   - integrated into `Blockchain::add_new_block`,
   - bypasses existing PoS `check_block_validity` on MIGRATION_NET (and later in mainnet temp-consensus mode),
   - enforces `check_temp_leader_consensus`.

Production nodes can run with MAINNET identifiers and still use temporary consensus internally by enabling flags.

---

## 4. MIGRATION_NET (Phase 1)

### 4.1 Network Type and IDs

We add a new network type, e.g.:

- `MIGRATION_NET` alongside `MAINNET`, `TESTNET`, `STAGENET`.

Configuration:

- **Network ID**:  
  A dedicated magic value distinct from MAINNET to guarantee P2P incompatibility.
- **Ports**:
  - P2P port for MIGRATION_NET (e.g., `18290`).
  - RPC port (e.g., `18291`).
  - ZMQ or additional ports as needed.
- **Seed nodes**:
  - `seed1.xcash.tech`
  - `seed2.xcash.tech`
  - `seed3.xcash.tech`
  These are intended as hostnames reachable inside a local Docker network.

### 4.2 P2P Isolation

On startup:

- A node started with `--network-type=migration`:
  - uses MIGRATION_NET ports,
  - uses MIGRATION_NET `network_id`,
  - connects only to MIGRATION_NET seeds.

Handshake behavior:

- If the peer’s `network_id` does not match MIGRATION_NET:
  - the connection is rejected,
  - a clear message is logged.

This ensures MIGRATION_NET cannot accidentally connect to MAINNET or other networks.

### 4.3 Data Directory

MIGRATION_NET nodes use a dedicated data directory, e.g.:

- mainnet: `~/.xcash/mainnet`
- migration: `~/.xcash/migration`

Operators may:

- copy an existing mainnet LMDB into the migration directory before starting MIGRATION_NET nodes,
- enabling tests against real chain data without touching production.

---

## 5. Temporary Leader-Based Consensus

We introduce a simple leader/follower model.

### 5.1 Configuration Flags

New CLI options:

- `--temp-consensus-enabled`  
  Enables leader-based consensus checks and replaces external module logic (in supported modes).

- `--temp-consensus-leader`  
  Marks this node as the leader; only one node should use this flag for the initial deployment.

- `--leader-id`  
  Short identifier for the leader, used in block metadata (e.g., string or fixed-length bytes).

- `--leader-pubkey`  
  Public key used by followers to verify leader signatures.

- `--leader-miner-address`  
  Address receiving block rewards in leader-produced blocks.

- `--temp-consensus-with-pow` (optional)  
  If true, leader blocks must satisfy PoW difficulty; if false, difficulty checks can be bypassed (for MIGRATION_NET) and a deterministic nonce used.

### 5.2 Leader vs Follower Behavior

- **Leader node**:
  - `--network-type=migration` (Phase 1–3),
  - `--temp-consensus-enabled`,
  - `--temp-consensus-leader`,
  - must be fully synchronized before producing blocks.

- **Follower node**:
  - `--network-type=migration`,
  - `--temp-consensus-enabled`,
  - no `--temp-consensus-leader` flag.

Leader runs the block generator. Followers never produce blocks via this mechanism; they only validate.

---

## 6. Leader Metadata in Blocks

### 6.1 Data Format

We use a new extra tag in `miner_tx.extra`, e.g.:

- `TX_EXTRA_TAG_LEADER_INFO` (1-byte tag value).

Binary layout:

```text
[tag:1][leader_id_len:1][leader_id:leader_id_len][signature:fixed_len]
```

- `tag`: constant `TX_EXTRA_TAG_LEADER_INFO`.
- `leader_id_len`: length of `leader_id` (1 byte).
- `leader_id`: encoded identifier (e.g., short string or pubkey hash).
- `signature`: fixed-length `crypto::signature` over the block hash or header hash.

### 6.2 Writing Metadata (Leader)

On the leader:

1. After `get_block_template`, compute `leader_id` from configuration.
2. Compute the block hash or header hash to sign.
3. Sign the hash using the leader private key.
4. Encode `[tag, leader_id_len, leader_id, signature]` and append to `miner_tx.extra`.

### 6.3 Reading Metadata (Followers)

On all nodes (leader + followers):

1. When validating a block, parse `miner_tx.extra`.
2. Locate `TX_EXTRA_TAG_LEADER_INFO`.
3. If missing or malformed, leader consensus validation fails.
4. Extract `leader_id` and `signature` for use in `check_temp_leader_consensus`.

---

## 7. Leader Block Generator Service

The leader block generator is a background component responsible for producing blocks at 5-minute intervals.

### 7.1 Service Responsibilities

The service:

- runs only on nodes with `--temp-consensus-leader` and `--temp-consensus-enabled`,
- enforces 5-minute slot scheduling: `timestamp % 300 == 0`,
- constructs block templates via `core.get_block_template`,
- injects leader metadata,
- optionally computes PoW,
- submits blocks via `core.handle_block_found`.

### 7.2 5-minute Slot Scheduling

All nodes share the same global 5-minute slots:

- …:00  
- …:05  
- …:10  
- …:15  
- …:20  
- etc.

Pseudocode:

```text
while (!shutdown_requested) {
  uint64_t now = current_time();
  uint64_t next_slot = compute_next_slot(now); // aligned to 300 seconds

  if (now < next_slot) {
    sleep_until(next_slot);
    continue;
  }

  if (!node_is_synchronized()) {
    log("leader: waiting for sync");
    sleep(1);
    continue;
  }

  block b;
  difficulty_type diff;
  uint64_t height;
  hash seed_hash;

  if (!core.get_block_template(b, leader_miner_address, diff, height, seed_hash)) {
    log("leader: failed to get block template");
    sleep(1);
    continue;
  }

  // Force slot timestamp
  b.timestamp = next_slot;

  add_leader_metadata(b);

  if (temp_consensus_with_pow) {
    run_pow_until_satisfied(b, diff, height);
  } else {
    b.nonce = deterministic_nonce(height, leader_id);
  }

  bool accepted = false;
  core.handle_block_found(b, accepted);
  log("leader: block submitted", height, accepted);

  // Next slot = current slot + 300 seconds
  sleep_until(next_slot + 300);
}
```

`compute_next_slot(now)`:

```text
next_slot = ((now / 300) * 300) + 300
```

If the node misses a slot (e.g., offline or restarted late), it simply waits for the next available slot.

### 7.3 Stub Mode vs Full Mode

- **Phase 2 (stub)**:
  - Leader still generates blocks on slots.
  - Followers see blocks but **always reject** them (no state change).
  - `check_temp_leader_consensus` can run in “log-only” mode.

- **Phase 3 (full)**:
  - Leader’s blocks are fully validated and accepted by followers if signatures and metadata are valid.

---

## 8. Block Validation Integration

### 8.1 Integration Point in Blockchain

The existing PoS validation hook in `Blockchain::add_new_block` looks like:

```cpp
if (version >= HF_VERSION_PROOF_OF_STAKE &&
    !check_block_validity(block, height))
{
  // reject
}
```

On MIGRATION_NET (and later, in MAINNET with temp-consensus enabled), this hook is replaced for new blocks:

- `check_block_validity` is **not** called.
- `check_temp_leader_consensus(block, height)` becomes the authoritative check for leader consensus.

### 8.2 `check_temp_leader_consensus` Behavior

`check_temp_leader_consensus(block, height)`:

1. If `temp-consensus-enabled` is false, return `true` (no-op).
2. Extract leader metadata:
   - if missing or malformed, return `false`.
3. Verify leader identity:
   - compare `leader_id` from metadata with configured leader id.
4. Verify signature:
   - compute block hash,
   - verify signature with `leader-pubkey`.
5. Enforce timing:
   - reject if `block.timestamp % 300 != 0`.
6. If all checks pass: return `true`.

The block validation flow:

- If `check_temp_leader_consensus` returns `false`:
  - block is rejected,
  - a detailed error is logged.
- If `true`:
  - normal block/transaction validation proceeds (sizes, fees, etc.).

On MAINNET with `--temp-consensus-enabled=false`, legacy behavior remains unchanged.

---

## 9. Rollout Phases & Environments

### 9.1 Phase 1 — Local MIGRATION_NET Bootstrap

- Implement MIGRATION_NET network type, ports, seeds.
- Bring up 3-node local cluster via Docker Compose.
- Use copied mainnet LMDB for initial state.
- Verify:
  - nodes connect only to MIGRATION_NET peers,
  - seeds `seed1/2/3.xcash.tech` resolve in Docker network,
  - no handshake with MAINNET.

### 9.2 Phase 2 — Leader + Validation Stub

- Enable leader service on MIGRATION_NET leader node.
- Implement validation stub on followers:
  - log block arrival,
  - always reject (no chain advancement).
- Validate:
  - leader produces blocks every 5 minutes,
  - followers see blocks and emit stub logs.

### 9.3 Phase 3 — Full Leader Validation

- Implement leader metadata and `check_temp_leader_consensus`.
- Replace PoS hook for MIGRATION_NET in `Blockchain::add_new_block`.
- Validate:
  - followers accept correctly signed leader blocks,
  - invalid metadata/signatures are rejected,
  - slot enforcement works.

### 9.4 Phase 4 — Production Deployment with MAINNET Identifiers

On real servers:

- One server = one node (Docker or systemd).
- Nodes use existing MAINNET `network_id`, ports, seeds, and LMDB.
- Temporary consensus is activated via CLI flags:
  - leader on one node,
  - followers on others.
- From the outside:
  - network looks identical to legacy XCash,
  - internal consensus is now leader-based.

---

## 10. Risks & Mitigations

### Wrong Leader Configuration

- **Risk**: Followers reject all blocks due to ID or key mismatch.
- **Mitigation**:
  - strict config validation at startup,
  - clear error logs for leader-metadata failures,
  - provide example configs.

### Leader Downtime

- **Risk**: No new blocks produced while leader is offline.
- **Mitigation**:
  - acceptable for migration network and temporary phase,
  - manual promotion of a different node as leader if needed.

### Network Misconfiguration

- **Risk**: MIGRATION_NET accidentally connects to MAINNET.
- **Mitigation**:
  - unique `network_id` and ports,
  - isolated seed hosts,
  - strong logging of rejected handshakes.

### Future Consensus Interaction

- **Risk**: Temporary code pollutes future design.
- **Mitigation**:
  - isolate temporary consensus to a dedicated module,
  - mark as deprecated once new consensus is ready,
  - avoid cross-dependencies into long-term consensus code.

---

## 11. Summary

This design introduces:

- a **MIGRATION_NET** for safe local testing,
- a **single-leader temporary consensus** model,
- a **leader block generator** with strict 5-minute slot timing,
- **leader metadata** for block authenticity,
- a clear **integration point** in `Blockchain::add_new_block`,
- a **phased rollout** from local testing to production with MAINNET identifiers.

It provides a minimal, controlled way to keep XCash running on a temporary leader-based consensus while the external consensus module is replaced, without breaking compatibility with existing infrastructure and chain data.
