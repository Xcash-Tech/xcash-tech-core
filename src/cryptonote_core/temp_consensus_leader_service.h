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

#include <atomic>
#include <thread>
#include <memory>
#include "cryptonote_basic/cryptonote_basic.h"

namespace cryptonote
{
  // Forward declaration
  class cryptonote_core;

  /**
   * @brief Temporary leader-based block generation service
   * 
   * This service implements Phase 2 of the temporary consensus:
   * - Generates blocks at 5-minute time slots (timestamp % 300 == 0)
   * - Forces block timestamp = slot timestamp
   * - Uses deterministic nonce when PoW is disabled
   * 
   * This is a TEMPORARY implementation for migration period only.
   */
  class temp_consensus_leader_service
  {
  public:
    /**
     * @brief Configuration for leader service
     */
    struct config
    {
      std::string leader_id;                    // Leader identifier
      crypto::public_key leader_pubkey;         // Leader public key for signing
      crypto::secret_key leader_seckey;         // Leader secret key for signing
      account_public_address miner_address;     // Address to receive block rewards
      bool enable_pow;                          // Whether to perform PoW (default: false)
      uint64_t slot_duration_seconds;           // Time slot duration (default: 300 = 5 minutes)
      
      config() 
        : enable_pow(false)
        , slot_duration_seconds(300)
      {}
    };

    /**
     * @brief Construct leader service
     * @param core Reference to cryptonote core
     * @param cfg Leader service configuration
     */
    temp_consensus_leader_service(cryptonote_core& core, const config& cfg);
    
    /**
     * @brief Destructor - stops service if running
     */
    ~temp_consensus_leader_service();

    /**
     * @brief Start the leader service
     * @return true if started successfully
     */
    bool start();

    /**
     * @brief Stop the leader service
     */
    void stop();

    /**
     * @brief Check if service is running
     */
    bool is_running() const { return m_running.load(); }

    /**
     * @brief Calculate next slot timestamp
     * @param current_time Current Unix timestamp
     * @return Next slot timestamp aligned to slot_duration_seconds boundary
     */
    uint64_t next_slot_timestamp(uint64_t current_time) const;

    /**
     * @brief Check if given timestamp is a valid slot boundary
     * @param timestamp Unix timestamp to check
     * @return true if timestamp % slot_duration_seconds == 0
     */
    bool is_slot_boundary(uint64_t timestamp) const;

  private:
    /**
     * @brief Main service loop (runs in separate thread)
     */
    void service_loop();

    /**
     * @brief Generate and submit a block for the current slot
     * @param slot_timestamp The slot timestamp for this block
     * @return true if block was generated and submitted successfully
     */
    bool generate_block(uint64_t slot_timestamp);

    /**
     * @brief Generate deterministic nonce for block (when PoW disabled)
     * @param slot_timestamp The slot timestamp
     * @return Deterministic nonce value
     */
    uint32_t generate_deterministic_nonce(uint64_t slot_timestamp) const;

  private:
    cryptonote_core& m_core;
    config m_config;
    
    std::atomic<bool> m_running;
    std::atomic<bool> m_stop_requested;
    std::unique_ptr<std::thread> m_service_thread;
    
    uint64_t m_last_generated_slot;  // Track last slot we generated to avoid duplicates
  };

} // namespace cryptonote
