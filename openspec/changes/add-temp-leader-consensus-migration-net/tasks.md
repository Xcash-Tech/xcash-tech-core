## 0. Rollout Phases (High‑level execution order)

### Phase 1 — Bring up isolated MIGRATION_NET locally (Docker, 3 nodes)
- [ ] 1.1 Add new `MIGRATION_NET` network type to daemon.
- [ ] 1.2 Define **unique** MIGRATION_NET magic bytes (`network_id`), P2P, RPC, ZMQ ports.
- [ ] 1.3 Add isolated seed nodes:
      - `seed1.xcash.tech`
      - `seed2.xcash.tech`
      - `seed3.xcash.tech`
      (Docker hostnames inside compose)
- [ ] 1.4 Ensure MIGRATION_NET **cannot** connect to MAINNET via P2P handshake.
- [ ] 1.5 Add CLI flag `--network-type=migration`.
- [ ] 1.6 Validate 3-node local cluster:
      - nodes resolve seeds,
      - nodes sync with each other,
      - nodes do **not** connect to mainnet,
      - LMDB loads from copied DB.
- [ ] 1.7 Finalize minimal migration docker-compose for 3 nodes.

---

## Phase 2 — Implement leader service + validation stub (block rejection mode)

### Leader service (scheduling only)
- [ ] 2.1 Create `temp_consensus_leader_service`.
- [ ] 2.2 Implement **5-minute slot scheduler**:
      - `timestamp % 300 == 0`
      - leader only generates blocks at these global time slots.
- [ ] 2.3 Implement `next_slot_timestamp()`.
- [ ] 2.4 Force block timestamp = slot timestamp.
- [ ] 2.5 Implement deterministic nonce when PoW disabled.

### Validation stub mode
- [ ] 2.6 Add flag `--temp-consensus-enabled`.
- [ ] 2.7 Add flag `--temp-consensus-leader`.
- [ ] 2.8 Add stub validator:
      - follower logs that block arrived,
      - but *always rejects* it,
      - no signature checks yet.
- [ ] 2.9 Ensure external consensus module is fully bypassed on MIGRATION_NET.
- [ ] 2.10 Validate local cluster:
       - leader generates slot-timed blocks,
       - followers log stub events,
       - no chain advancement occurs.

---

## Phase 3 — Full leader consensus enforcement

### Leader metadata
- [ ] 3.1 Define extra tag `TX_EXTRA_TAG_LEADER_INFO`.
- [ ] 3.2 Implement serialization:
      `[tag][leader_id_len][leader_id][signature]`.
- [ ] 3.3 Implement metadata extraction.
- [ ] 3.4 Add unit tests for malformed metadata.

### Real validation logic
- [ ] 3.5 Implement `check_temp_leader_consensus(block, height)`:
      - extract metadata,
      - verify leader_id,
      - verify signature using provided `--leader-pubkey`,
      - reject if invalid.

### Blockchain hook
- [ ] 3.6 Replace PoS hook in `Blockchain::add_new_block`:
      - MIGRATION_NET: bypass `check_block_validity`,
      - instead call `check_temp_leader_consensus`,
      - MAINNET: preserve original behavior.
- [ ] 3.7 Ensure block acceptance works end‑to‑end.

### Local functional test
- [ ] 3.8 Run 3-node local network:
      - leader produces valid blocks on 5-minute slots,
      - followers accept and sync,
      - missed-slot handling works.

---

## Phase 4 — Server deployment using MAINNET identifiers

**Important:** On servers we do *not* use MIGRATION_NET.  
Nodes must use **MAINNET IDs, ports, seeds**, but rely on temporary leader consensus internally.

### Tasks
- [ ] 4.1 Prepare production docker config: one server = one Docker node.
- [ ] 4.2 Use MAINNET `network_id`, ports, seeds.
- [ ] 4.3 Use same LMDB as legacy chain.
- [ ] 4.4 Enable temporary leader consensus flags.
- [ ] 4.5 Validate:
      - chain behaves identically externally,
      - consensus follows slot schedule internally,
      - no P2P cross-talk with MIGRATION_NET.

---

## Phase 5 — Stability monitoring (optional but recommended)
- [ ] 5.1 Add logs for slot scheduling events.
- [ ] 5.2 Add logs for metadata parsing failures.
- [ ] 5.3 Observe multi-day operation of leader/followers.
- [ ] 5.4 Prepare migration report for the next major consensus upgrade.