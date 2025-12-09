// Copyright (c) 2025 X-CASH Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "temp_consensus_validator.h"
#include "cryptonote_basic/cryptonote_format_utils.h"

#undef XCASH_DEFAULT_LOG_CATEGORY
#define XCASH_DEFAULT_LOG_CATEGORY "temp_consensus"

namespace cryptonote
{

temp_consensus_validator::temp_consensus_validator(const config& cfg)
  : m_config(cfg)
  , m_enabled(false)
{
  MINFO("Temporary consensus validator initialized");
  MINFO("Expected leader ID: " << m_config.expected_leader_id);
}

bool temp_consensus_validator::validate_leader_block(const block& bl, uint64_t height)
{
  if (!m_enabled)
  {
    MWARNING("Validator called but not enabled");
    return false;
  }

  // Special case: allow genesis block (height 0)
  if (height == 0)
  {
    MINFO("=== Genesis block (height 0) - ALLOWED ===");
    return true;
  }

  // Phase 3: Full validation implementation
  MINFO("=== Validating leader block (Phase 3) ===");
  MINFO("Block height: " << height);
  
  crypto::hash block_hash = get_block_hash(bl);
  MINFO("Block hash: " << block_hash);
  MINFO("Expected leader: " << m_config.expected_leader_id);
  
  // Step 1: Extract leader metadata from miner_tx.extra
  std::string leader_id;
  crypto::signature sig;
  
  if (!cryptonote::get_leader_info_from_tx_extra(bl.miner_tx.extra, leader_id, sig))
  {
    MERROR("REJECT: No leader metadata found in miner tx extra");
    return false;
  }
  
  MINFO("Extracted leader_id: " << leader_id);
  
  // Step 2: Verify leader_id matches expected leader
  if (leader_id != m_config.expected_leader_id)
  {
    MERROR("REJECT: Leader ID mismatch");
    MERROR("  Expected: " << m_config.expected_leader_id);
    MERROR("  Got:      " << leader_id);
    return false;
  }
  
  MINFO("✓ Leader ID verified");
  
  // Step 3: Verify signature using leader_pubkey
  if (!crypto::check_signature(block_hash, m_config.leader_pubkey, sig))
  {
    MERROR("REJECT: Invalid signature");
    MERROR("  Block hash: " << block_hash);
    MERROR("  Leader pubkey: " << m_config.leader_pubkey);
    return false;
  }
  
  MINFO("✓ Signature verified");
  MINFO("=== Block ACCEPTED ===");
  
  return true;
}

} // namespace cryptonote
