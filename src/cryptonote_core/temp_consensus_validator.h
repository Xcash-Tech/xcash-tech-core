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

#pragma once

#include "cryptonote_basic/cryptonote_basic.h"
#include "crypto/crypto.h"

namespace cryptonote
{
  /**
   * @brief Temporary consensus validator (Phase 2 stub)
   * 
   * Phase 2 implementation:
   * - Logs when blocks arrive from leader
   * - Always rejects blocks (returns false)
   * - No signature verification yet
   * 
   * Phase 3 will implement full validation with signature checks.
   * 
   * This is a TEMPORARY implementation for migration period only.
   */
  class temp_consensus_validator
  {
  public:
    /**
     * @brief Configuration for validator
     */
    struct config
    {
      std::string expected_leader_id;       // Expected leader identifier
      crypto::public_key leader_pubkey;     // Leader public key for signature verification
      
      config() {}
    };

    /**
     * @brief Construct validator
     * @param cfg Validator configuration
     */
    temp_consensus_validator(const config& cfg);

    /**
     * @brief Validate block from leader (Phase 2 stub)
     * 
     * Phase 2: Always logs and returns false (reject)
     * Phase 3: Will extract metadata and verify signature
     * 
     * @param bl Block to validate
     * @param height Block height
     * @return true if block is valid (Phase 2: always false)
     */
    bool validate_leader_block(const block& bl, uint64_t height);

    /**
     * @brief Check if temporary consensus is enabled for this validator
     * @return true if enabled
     */
    bool is_enabled() const { return m_enabled; }

    /**
     * @brief Enable/disable validator
     */
    void set_enabled(bool enabled) { m_enabled = enabled; }

  private:
    config m_config;
    bool m_enabled;
  };

} // namespace cryptonote
