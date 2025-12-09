## 0. Rollout Phases (High‑level execution order)

### Phase 1 — Configure testnet as isolated migration network (Docker, 3 nodes)
- [x] 1.1 Replace testnet `network_id` with unique migration network_id.
- [x] 1.2 Update testnet ports to migration-specific values (P2P:58280, RPC:58281, ZMQ:58282).
- [x] 1.3 Add isolated seed nodes in testnet config:
      - `seed1.xcash.testnet`
      - `seed2.xcash.testnet`
      - `seed3.xcash.testnet`
      (Docker hostnames inside compose)
- [x] 1.4 Ensure testnet with new network_id **cannot** connect to MAINNET or legacy testnet via P2P handshake.
- [x] 1.5 Launch nodes with `--testnet` flag (automatically uses new network_id).
- [x] 1.6 Validate 3-node local cluster:
      - nodes resolve seeds,
      - nodes sync with each other,
      - nodes do **not** connect to mainnet or legacy testnet,
      - LMDB loads from copied DB into `/data/bc/testnet`.
- [x] 1.7 Finalize minimal migration docker-compose for 3 nodes.
- [x] 1.8 Modernize Dockerfile with Ubuntu 22.04, multi-stage build, non-root user.

---

## Phase 2 — Implement leader service + validation stub (block rejection mode)

### Leader service (scheduling only)
- [x] 2.1 Create `temp_consensus_leader_service`.
- [x] 2.2 Implement **5-minute slot scheduler**:
      - `timestamp % 300 == 0`
      - leader only generates blocks at these global time slots.
- [x] 2.3 Implement `next_slot_timestamp()`.
- [x] 2.4 Force block timestamp = slot timestamp.
- [x] 2.5 Implement deterministic nonce when PoW disabled.

### Validation stub mode
- [x] 2.6 Add flag `--temp-consensus-enabled`.
- [x] 2.7 Add flag `--temp-consensus-leader`.
- [x] 2.8 Add stub validator:
      - follower logs that block arrived,
      - but *always rejects* it,
      - no signature checks yet.
- [x] 2.9 Ensure external consensus module is fully bypassed when testnet runs with temp consensus.
- [x] 2.10 Validate local cluster:
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
      - testnet with temp consensus enabled: bypass `check_block_validity`,
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

**Important:** On production servers we do *not* use the migration network (testnet with unique network_id).  
Nodes must use **MAINNET IDs, ports, seeds**, but rely on temporary leader consensus internally.

### Tasks
- [ ] 4.1 Prepare production docker config: one server = one Docker node.
- [ ] 4.2 Use MAINNET `network_id`, ports, seeds.
- [ ] 4.3 Use same LMDB as legacy chain.
- [ ] 4.4 Enable temporary leader consensus flags.
- [ ] 4.5 Validate:
      - chain behaves identically externally,
      - consensus follows slot schedule internally,
      - no P2P cross-talk with migration network (testnet).

---

## Phase 5 — Stability monitoring (optional but recommended)
- [ ] 5.1 Add logs for slot scheduling events.
- [ ] 5.2 Add logs for metadata parsing failures.
- [ ] 5.3 Observe multi-day operation of leader/followers.
- [ ] 5.4 Prepare migration report for the next major consensus upgrade.