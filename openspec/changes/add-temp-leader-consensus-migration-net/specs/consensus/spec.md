# Consensus Specification — Temporary Leader-Based Consensus & Migration Network
(Aligned with Rollout Phases 1–4)

**Note**: The migration network reuses **testnet** with a unique `network_id` to ensure isolation, rather than creating a new network type.

---

## ADDED Requirements

### Requirement: Dedicated migration-only network

The system SHALL support an isolated migration network by repurposing testnet with a unique network_id, completely separate from MAINNET, STAGENET, and legacy testnet.

#### Scenario: MIGRATION_NET initialization
- **GIVEN** a node is started with `--testnet`
- **WHEN** the networking subsystem initializes
- **THEN** the node SHALL use the unique migration network `network_id`: `0xA0 0xB1 0xC2 0xD3 0xE4 0xF5 0xA6 0xB7 0xC8 0xD9 0xEA 0xFB 0xAC 0xBD 0xCE 0x93`
- **AND** SHALL bind to migration-specific P2P (18290), RPC (18291), and ZMQ (18292) ports
- **AND** SHALL load only migration network seeds

#### Scenario: MIGRATION_NET seeds
- **GIVEN** migration network is active (testnet with unique network_id)
- **THEN** the node SHALL include the following mandatory seed hosts:
  - `seed1.xcash.tech`
  - `seed2.xcash.tech`
  - `seed3.xcash.tech`
- **AND** these hosts SHALL be discoverable inside local Docker networks

#### Scenario: Reject connections from other networks
- **WHEN** a P2P handshake is received
- **IF** the peer’s `network_id` differs
- **THEN** the node SHALL reject the connection
- **AND** SHALL log the mismatch

#### Scenario: Migration LMDB usage
- **GIVEN** migration network mode (testnet with unique network_id)
- **WHEN** the daemon loads the blockchain
- **THEN** it SHALL use the testnet data directory (`~/.xcash/testnet`)
- **AND** MAY load from a copied mainnet LMDB snapshot

### Requirement: Leader scheduling without accepting blocks

In Phase 2, the leader service SHALL generate blocks at scheduled intervals, but follower nodes SHALL NOT yet accept blocks into their blockchain state.

#### Scenario: Leader service startup
- **GIVEN** testnet + `--temp-consensus-enabled` + `--temp-consensus-leader`
- **WHEN** startup completes
- **THEN** the leader SHALL start the leader block generator thread

#### Scenario: 5-minute slot scheduling
- **GIVEN** the leader service runs
- **WHEN** system time reaches a slot (`timestamp % 300 == 0`)
- **THEN** the leader SHALL construct a block template
- **AND** SHALL set the block timestamp to the exact slot boundary
- **AND** SHALL insert metadata (even in stub mode)
- **BUT** PoW SHALL run only if enabled

#### Scenario: Validation stub on followers
- **GIVEN** migration network follower nodes (testnet)
- **WHEN** they receive a block
- **THEN** they SHALL log receipt of the block
- **AND** they SHALL reject the block unconditionally
- **AND** SHALL NOT advance the blockchain

#### Scenario: External consensus disabled
- **GIVEN** testnet with migration network_id
- **THEN** the external consensus module SHALL NOT be called

### Requirement: Leader metadata format

All blocks produced by the leader SHALL include metadata (leader ID and signature) encoded in the coinbase transaction's extra field using a dedicated tag.

#### Scenario: Leader metadata encoding
- **GIVEN** a leader finalizes a block
- **THEN** it SHALL serialize metadata using:

```
[tag:1][leader_id_len:1][leader_id:leader_id_len][signature:fixed_len]
```

#### Scenario: Metadata extraction
- **GIVEN** a follower receives a block
- **WHEN** parsing `miner_tx.extra`
- **THEN** it SHALL locate `TX_EXTRA_TAG_LEADER_INFO`
- **AND** SHALL reject blocks if:
  - tag missing,
  - structure malformed,
  - lengths invalid.

### Requirement: Actual validation rules

In Phase 3, follower nodes SHALL enforce full leader-based consensus validation, replacing the legacy PoS validation hook with temporary consensus checks.

#### Scenario: Replace PoS hook
- **GIVEN** testnet + temp consensus enabled
- **WHEN** `Blockchain::add_new_block` runs
- **THEN** the PoS validation hook (`check_block_validity`) SHALL be bypassed
- **AND** the node SHALL run `check_temp_leader_consensus` instead

#### Scenario: Valid block acceptance
- **GIVEN** metadata leader_id matches configured leader_id
- **AND** signature validates using `--leader-pubkey`
- **AND** block passes normal size/tx checks
- **THEN** the block SHALL be accepted

#### Scenario: Invalid signature
- **WHEN** signature verification fails
- **THEN** the block SHALL be rejected
- **AND** SHALL log the failure reason

- **IF** a block timestamp is not aligned to a slot (`timestamp % 300 != 0`)
- **THEN** the follower SHALL reject the block

### Requirement: Temporary consensus active on MAINNET identifiers

In Phase 4, production nodes SHALL use MAINNET network identifiers while internally enforcing temporary leader-based consensus when enabled via configuration flags. This is separate from the local migration network which uses testnet with a unique network_id.

#### Scenario: Production node uses legacy network identifiers
- **GIVEN** a node is deployed on a production server
- **THEN** it SHALL use MAINNET:
  - `network_id`,
  - ports,
  - seed nodes,
  - LMDB directory.

#### Scenario: Temporary consensus activated internally
- **GIVEN** MAINNET + `--temp-consensus-enabled`
- **THEN** the node SHALL enforce leader-based consensus
- **AND** bypass external consensus hooks
- **WHILE** maintaining full compatibility with legacy P2P layout

#### Scenario: Leader service in production
- **GIVEN** a single designated leader node on MAINNET
- **THEN** it SHALL produce blocks at 5-minute intervals
- **AND** followers SHALL validate using leader metadata exactly as in the migration network (testnet)

#### Scenario: Mainnet fallback
- **IF** `--temp-consensus-enabled` is disabled
- **THEN** nodes SHALL operate identically to the legacy network behavior

### Requirement: Optional Stability & Monitoring

The system SHALL support multi-day continuous operation and provide comprehensive logging for monitoring and debugging temporary consensus behavior.

#### Scenario: Multi-day stability
- **The system SHALL support** continuous operation for multiple days without slot drift or chain divergence.

#### Scenario: Logging
- **The system SHALL log**:
  - slot scheduling events,
  - metadata parsing failures,
  - signature validation errors,
  - rejected blocks with reasons.