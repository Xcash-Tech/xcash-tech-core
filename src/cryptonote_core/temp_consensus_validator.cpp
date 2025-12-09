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

  // Phase 2 stub implementation: log and always reject
  MINFO("=== Block validation stub (Phase 2) ===");
  MINFO("Block height: " << height);
  MINFO("Block hash: " << get_block_hash(bl));
  MINFO("Expected leader: " << m_config.expected_leader_id);
  
  // Log miner tx info
  if (bl.miner_tx.vin.size() > 0)
  {
    MINFO("Miner tx has " << bl.miner_tx.vin.size() << " input(s)");
  }
  
  if (bl.miner_tx.extra.size() > 0)
  {
    MINFO("Miner tx extra size: " << bl.miner_tx.extra.size() << " bytes");
  }
  
  // Phase 2: Always reject (return false)
  // Phase 3 will implement:
  // - Extract leader metadata from miner_tx.extra
  // - Verify leader_id matches expected
  // - Verify signature using leader_pubkey
  MINFO("Phase 2 stub: REJECTING block (validation not implemented yet)");
  
  return false;
}

} // namespace cryptonote
