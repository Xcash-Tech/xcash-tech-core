# Temporary Consensus Configuration

## Overview
Temporary leader-based consensus for migration period uses existing DPoS delegate infrastructure.

## Required Parameters

### Temporary Consensus Control (2 parameters)
```bash
--temp-consensus-enabled=1          # Enable temporary consensus
--temp-consensus-leader=1           # Set this node as leader (generates blocks)
```

### DPoS Delegate Parameters (reused)
```bash
--xcash-dpops-delegates-public-address=<address>   # Delegate public address
--xcash-dpops-delegates-secret-key=<hex>           # Delegate secret key (leader only)
```

## Usage

### Leader Node
```bash
xcashd \
  --temp-consensus-enabled=1 \
  --temp-consensus-leader=1 \
  --xcash-dpops-delegates-public-address=XCA1... \
  --xcash-dpops-delegates-secret-key=abcdef123...
```

### Follower Node
```bash
xcashd \
  --temp-consensus-enabled=1 \
  --temp-consensus-leader=0 \
  --xcash-dpops-delegates-public-address=XCA1...
```

## Parameter Mapping

| Function | Parameter | Source |
|----------|-----------|--------|
| Leader ID | `expected_leader_id` | `--xcash-dpops-delegates-public-address` |
| Leader Public Key | `leader_pubkey` | Extracted from delegate address (spend key) |
| Leader Secret Key | `leader_seckey` | `--xcash-dpops-delegates-secret-key` |
| Miner Address | `miner_address` | `--xcash-dpops-delegates-public-address` |
| PoW Mode | `enable_pow` | Always `false` (deterministic nonce) |
| Slot Duration | `slot_duration_seconds` | Fixed: 300 seconds (5 minutes) |

## Block Generation

- **Slot boundary**: `timestamp % 300 == 0` (every 5 minutes)
- **Nonce**: Deterministic (`slot_timestamp` as nonce)
- **Rewards**: All block rewards go to delegate address
- **Signature**: Leader signs block with delegate secret key

## Validation

- **Followers**: Validate leader signature using public key from delegate address
- **Rejection**: Any block not signed by configured leader is rejected (Phase 2 stub)
- **External consensus**: Fully bypassed when temp consensus enabled

## Phase 2 vs Phase 3

### Phase 2 (Current - Stub)
- Validator **logs and rejects** all blocks
- No actual block generation (stub)
- Used for infrastructure testing

### Phase 3 (Planned)
- Real signature verification
- Actual block generation via `get_block_template` + `handle_block_found`
- Leader metadata in `TX_EXTRA_TAG_LEADER_INFO`
- Full consensus enforcement
