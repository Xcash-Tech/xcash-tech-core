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

#include "temp_consensus_leader_service.h"
#include "cryptonote_core/cryptonote_core.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include <chrono>
#include <thread>

#undef XCASH_DEFAULT_LOG_CATEGORY
#define XCASH_DEFAULT_LOG_CATEGORY "temp_consensus"

namespace cryptonote
{

temp_consensus_leader_service::temp_consensus_leader_service(cryptonote_core& core, const config& cfg)
  : m_core(core)
  , m_config(cfg)
  , m_running(false)
  , m_stop_requested(false)
  , m_last_generated_slot(0)
{
  MINFO("Temporary leader service initialized");
  MINFO("Leader ID: " << m_config.leader_id);
  MINFO("Slot duration: " << m_config.slot_duration_seconds << " seconds");
  MINFO("PoW enabled: " << (m_config.enable_pow ? "yes" : "no"));
}

temp_consensus_leader_service::~temp_consensus_leader_service()
{
  stop();
}

bool temp_consensus_leader_service::start()
{
  if (m_running.load())
  {
    MWARNING("Leader service already running");
    return false;
  }

  MINFO("Starting temporary leader service...");
  m_stop_requested.store(false);
  m_running.store(true);
  
  m_service_thread.reset(new std::thread(&temp_consensus_leader_service::service_loop, this));
  
  MINFO("Leader service started successfully");
  return true;
}

void temp_consensus_leader_service::stop()
{
  if (!m_running.load())
    return;

  MINFO("Stopping temporary leader service...");
  m_stop_requested.store(true);
  
  if (m_service_thread && m_service_thread->joinable())
  {
    m_service_thread->join();
  }
  
  m_running.store(false);
  MINFO("Leader service stopped");
}

uint64_t temp_consensus_leader_service::next_slot_timestamp(uint64_t current_time) const
{
  // Round up to next slot boundary
  uint64_t remainder = current_time % m_config.slot_duration_seconds;
  if (remainder == 0)
    return current_time; // Already on boundary
  
  return current_time + (m_config.slot_duration_seconds - remainder);
}

bool temp_consensus_leader_service::is_slot_boundary(uint64_t timestamp) const
{
  return (timestamp % m_config.slot_duration_seconds) == 0;
}

void temp_consensus_leader_service::service_loop()
{
  MINFO("Leader service loop started");
  
  while (!m_stop_requested.load())
  {
    try
    {
      // Get current time
      uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
      ).count();
      
      // Calculate next slot
      uint64_t next_slot = next_slot_timestamp(now);
      
      // Check if we've already generated for this slot
      if (next_slot <= m_last_generated_slot)
      {
        // Wait a bit before checking again
        std::this_thread::sleep_for(std::chrono::seconds(1));
        continue;
      }
      
      // Wait until slot time
      if (now < next_slot)
      {
        uint64_t wait_seconds = next_slot - now;
        MINFO("Next slot in " << wait_seconds << " seconds (slot time: " << next_slot << ")");
        
        // Sleep in small increments to check stop flag
        for (uint64_t i = 0; i < wait_seconds && !m_stop_requested.load(); ++i)
        {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        if (m_stop_requested.load())
          break;
      }
      
      // Generate block for this slot
      MINFO("Generating block for slot timestamp: " << next_slot);
      bool success = generate_block(next_slot);
      
      if (success)
      {
        MINFO("Block generated successfully for slot " << next_slot);
        m_last_generated_slot = next_slot;
      }
      else
      {
        MWARNING("Failed to generate block for slot " << next_slot);
      }
      
      // Small delay before next iteration
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    catch (const std::exception& e)
    {
      MERROR("Exception in leader service loop: " << e.what());
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
  
  MINFO("Leader service loop stopped");
}

bool temp_consensus_leader_service::generate_block(uint64_t slot_timestamp)
{
  // Phase 3 implementation: Full block generation with leader metadata
  
  MINFO("=== Generating leader block (Phase 3) ===");
  MINFO("Slot timestamp: " << slot_timestamp);
  MINFO("Leader ID: " << m_config.leader_id);
  
  // Step 1: Get block template from core
  block bl;
  difficulty_type difficulty;
  uint64_t height;
  uint64_t expected_reward;
  blobdata extra_nonce;  // Empty for now
  
  if (!m_core.get_block_template(bl, m_config.miner_address, difficulty, height, expected_reward, extra_nonce))
  {
    MERROR("Failed to get block template from core");
    return false;
  }
  
  MINFO("Block template obtained: height=" << height << " difficulty=" << difficulty);
  
  // Step 2: Force block timestamp to slot timestamp
  bl.timestamp = slot_timestamp;
  MINFO("Set block timestamp to slot: " << slot_timestamp);
  
  // Step 3: Set deterministic nonce (no PoW)
  if (!m_config.enable_pow)
  {
    bl.nonce = generate_deterministic_nonce(slot_timestamp);
    MINFO("Set deterministic nonce: " << bl.nonce);
  }
  
  // Step 4: Calculate block hash for signing
  crypto::hash block_hash = get_block_hash(bl);
  MINFO("Block hash: " << block_hash);
  
  // Step 5: Sign block hash with leader secret key
  crypto::signature sig;
  crypto::generate_signature(block_hash, m_config.leader_pubkey, m_config.leader_seckey, sig);
  MINFO("Block signed with leader key");
  
  // Step 6: Add leader metadata to miner tx extra
  if (!add_leader_info_to_tx_extra(bl.miner_tx.extra, m_config.leader_id, sig))
  {
    MERROR("Failed to add leader metadata to miner tx");
    return false;
  }
  
  MINFO("Leader metadata added to miner tx");
  
  // Step 7: Submit block to core
  if (!m_core.handle_block_found(bl))
  {
    MERROR("Core rejected block");
    return false;
  }
  
  MINFO("✓ Block generated and submitted successfully");
  MINFO("  Height: " << height);
  MINFO("  Hash: " << block_hash);
  MINFO("  Timestamp: " << slot_timestamp);
  
  return true;
}

uint32_t temp_consensus_leader_service::generate_deterministic_nonce(uint64_t slot_timestamp) const
{
  // Simple deterministic nonce: hash(leader_id + slot_timestamp)
  // This ensures different nonces for different slots but same nonce for same slot
  
  crypto::hash h;
  std::string data = m_config.leader_id + std::to_string(slot_timestamp);
  crypto::cn_fast_hash(data.data(), data.size(), h);
  
  // Use first 4 bytes as nonce
  uint32_t nonce;
  memcpy(&nonce, &h, sizeof(nonce));
  
  return nonce;
}

} // namespace cryptonote
