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
#include "cryptonote_basic/tx_extra.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include <sodium.h>
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
        MINFO("=== DEBUG: Starting sleep loop, m_stop_requested=" << m_stop_requested.load());
        
        // Sleep in small increments to check stop flag
        for (uint64_t i = 0; i < wait_seconds && !m_stop_requested.load(); ++i)
        {
          std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        MINFO("=== DEBUG: Sleep loop finished, m_stop_requested=" << m_stop_requested.load());
        
        if (m_stop_requested.load())
          break;
      }
      
      MINFO("=== DEBUG: About to generate block for slot " << next_slot);
      // Generate block for this slot
      MINFO("Generating block for slot timestamp: " << next_slot);
      bool success = false;
      try
      {
        success = generate_block(next_slot);
      }
      catch (const std::exception& e)
      {
        MERROR("Exception in generate_block(): " << e.what());
        success = false;
      }
      
      if (success)
      {
        MINFO("Block generated successfully for slot " << next_slot);
        m_last_generated_slot = next_slot;
        MINFO("=== Continuing to next iteration ===");
      }
      else
      {
        MWARNING("Failed to generate block for slot " << next_slot);
      }
      
      // Small delay before next iteration
      MINFO("=== Sleeping 1 second before next loop iteration ===");
      std::this_thread::sleep_for(std::chrono::seconds(1));
      MINFO("=== Woke up, checking stop flag: m_stop_requested=" << m_stop_requested << " ===");
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
  
  // Calculate the size of leader_info that will be added to miner_tx.extra later
  // This is used to reserve space via extra_nonce so that get_block_template
  // calculates the correct block weight and reward
  //
  // leader_info size breakdown:
  // - tag (TX_EXTRA_TAG_LEADER_INFO): 1 byte
  // - size varint: ~2 bytes (for sizes < 253)
  // - leader_id string length varint: 1-2 bytes
  // - leader_id string: 97 bytes (X-CASH public address)
  // - signature: 64 bytes (Ed25519)
  // Total: ~165 bytes, use 170 to be safe
  const size_t LEADER_INFO_RESERVED_SIZE = 170;
  
  // Step 1: Get block template from core
  // We pass extra_nonce with reserved space for leader_info to ensure
  // correct weight calculation for block reward
  block bl;
  difficulty_type difficulty;
  uint64_t height;
  uint64_t expected_reward;
  blobdata extra_nonce(LEADER_INFO_RESERVED_SIZE, '\0');  // Reserve space for leader metadata
  
  if (!m_core.get_block_template(bl, m_config.miner_address, difficulty, height, expected_reward, extra_nonce))
  {
    MERROR("Failed to get block template from core");
    return false;
  }
  
  MINFO("Reserved " << LEADER_INFO_RESERVED_SIZE << " bytes for leader metadata in extra_nonce");
  
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
  
  // Step 4: Extract existing tx_pub_key from miner tx extra
  crypto::public_key tx_pub_key = get_tx_pub_key_from_extra(bl.miner_tx.extra);
  if (tx_pub_key == crypto::null_pkey)
  {
    MERROR("Failed to extract tx_pub_key from miner tx extra");
    return false;
  }
  MINFO("Extracted tx_pub_key from template: " << tx_pub_key);
  
  // Step 5: Remove placeholder nonce AND padding from extra BEFORE calculating hash for signing
  // This is critical: the hash we sign must match the hash verifier will calculate
  // (which is hash of block without leader_info, without nonce, and without padding)
  size_t extra_size_with_placeholder = bl.miner_tx.extra.size();
  MINFO("DEBUG: miner_tx.extra size WITH placeholder: " << extra_size_with_placeholder);
  
  if (!remove_field_from_tx_extra(bl.miner_tx.extra, typeid(tx_extra_nonce)))
  {
    MWARNING("Failed to remove placeholder nonce from miner tx extra (may not exist)");
  }
  
  size_t extra_size_after_nonce = bl.miner_tx.extra.size();
  MINFO("DEBUG: miner_tx.extra size after removing nonce: " << extra_size_after_nonce);
  
  // Also remove padding - it was added by get_block_template for size alignment
  // Padding must be removed before signing because verifier won't have it
  if (!remove_field_from_tx_extra(bl.miner_tx.extra, typeid(tx_extra_padding)))
  {
    MWARNING("Failed to remove padding from miner tx extra (may not exist)");
  }
  
  size_t extra_size_without_placeholder = bl.miner_tx.extra.size();
  MINFO("DEBUG: miner_tx.extra size after removing padding: " << extra_size_without_placeholder);
  MINFO("DEBUG: Removed " << (extra_size_with_placeholder - extra_size_without_placeholder) << " bytes total (nonce + padding)");
  
  // Step 6: Serialize block to blob BEFORE signing (to ensure consistent state)
  blobdata blockblob_unsigned = t_serializable_object_to_blob(bl);
  MINFO("DEBUG: Serialized unsigned block to blob, size: " << blockblob_unsigned.size() << " bytes");
  
  // Step 7: Parse block from blob to get consistent block representation
  block bl_unsigned = AUTO_VAL_INIT(bl_unsigned);
  if (!parse_and_validate_block_from_blob(blockblob_unsigned, bl_unsigned))
  {
    MERROR("Failed to parse unsigned block from blob");
    return false;
  }
  MINFO("DEBUG: Unsigned block parsed from blob");
  
  // Step 8: Calculate block hash for signing (WITHOUT leader metadata AND without placeholder)
  // This hash will match what the verifier calculates after removing leader_info
  crypto::hash block_hash = get_block_hash(bl_unsigned);
  MINFO("Block hash (without leader metadata): " << block_hash);
  
  // Step 9: Sign block hash with Ed25519 secret key using libsodium
  // DPoS keys are in libsodium format, so we must use libsodium for signing
  unsigned char sig_bytes[64];
  unsigned long long sig_len = 64;
  
  MINFO("Signing with libsodium seckey (first 32 bytes): " << epee::string_tools::buff_to_hex_nodelimer(std::string((char*)m_config.libsodium_seckey, 32)));
  MINFO("Expected pubkey: " << epee::string_tools::pod_to_hex(m_config.leader_ed25519_pubkey));
  
  if (crypto_sign_detached(sig_bytes, &sig_len,
                          reinterpret_cast<unsigned char*>(&block_hash), 32,
                          m_config.libsodium_seckey) != 0)
  {
    MERROR("Failed to sign block with libsodium");
    return false;
  }
  
  crypto::signature sig;
  memcpy(&sig, sig_bytes, 64);
  
  MINFO("Generated signature: " << epee::string_tools::pod_to_hex(sig));
  MINFO("Block signed with Ed25519 key (libsodium)");
  
  // Step 10: Add leader metadata to block
  // Padding and nonce were already removed in Step 5, so extra should only contain pubkey
  size_t extra_size_before = bl_unsigned.miner_tx.extra.size();
  MINFO("DEBUG: miner_tx.extra size BEFORE adding leader metadata: " << extra_size_before);
  
  if (!add_leader_info_to_tx_extra(bl_unsigned.miner_tx.extra, m_config.leader_id, sig))
  {
    MERROR("Failed to add leader metadata to miner tx extra");
    return false;
  }
  
  size_t extra_size_after = bl_unsigned.miner_tx.extra.size();
  MINFO("DEBUG: miner_tx.extra size AFTER adding leader metadata: " << extra_size_after);
  MINFO("DEBUG: Added " << (extra_size_after - extra_size_before) << " bytes of leader metadata");
  
  // Step 11: Invalidate cached block hash (critical - hash changed!)
  MINFO("DEBUG: About to call bl_unsigned.invalidate_hashes()");
  bl_unsigned.invalidate_hashes();
  MINFO("DEBUG: After bl_unsigned.invalidate_hashes() - hash_valid should be false now");
  
  // Step 13: Serialize final block to blob (with leader metadata)
  blobdata blockblob = t_serializable_object_to_blob(bl_unsigned);
  MINFO("DEBUG: Serialized final block to blob, size: " << blockblob.size() << " bytes");
  
  // Step 12: Parse block from blob (recreate block object like RPC does)
  block b_parsed = AUTO_VAL_INIT(b_parsed);
  if (!parse_and_validate_block_from_blob(blockblob, b_parsed))
  {
    MERROR("Failed to parse and validate block from blob");
    return false;
  }
  MINFO("DEBUG: Block parsed and validated from blob successfully");
  MINFO("DEBUG: b_parsed.miner_tx.extra size = " << b_parsed.miner_tx.extra.size());
  
  // Verify leader_info is present in parsed block
  std::string test_leader_id;
  crypto::signature test_sig;
  if (!get_leader_info_from_tx_extra(b_parsed.miner_tx.extra, test_leader_id, test_sig))
  {
    MERROR("CRITICAL: leader_info NOT found in b_parsed after parsing! This is a bug.");
    // Dump first 50 bytes for debug
    std::string extra_hex;
    for (size_t i = 0; i < std::min<size_t>(50, b_parsed.miner_tx.extra.size()); i++)
    {
      char buf[4];
      snprintf(buf, sizeof(buf), "%02x", b_parsed.miner_tx.extra[i]);
      extra_hex += buf;
    }
    MERROR("DEBUG: First 50 bytes of b_parsed extra: " << extra_hex);
  }
  else
  {
    MINFO("DEBUG: leader_info FOUND in b_parsed, leader_id = " << test_leader_id);
  }
  
  // Step 13: Check block size (like RPC submitblock does)
  if (!m_core.check_incoming_block_size(blockblob))
  {
    MERROR("Block size too big, rejecting block");
    return false;
  }
  MINFO("DEBUG: Block size check passed");
  
  // Step 14: Submit block to core using proper batch handling (same as RPC submitblock)
  MINFO("=== About to call handle_block_found() ===");
  
  bool result = false;
  try {
    result = m_core.handle_block_found(b_parsed);
  } catch (const std::exception& e) {
    MERROR("EXCEPTION in handle_block_found(): " << e.what());
    return false;
  } catch (...) {
    MERROR("UNKNOWN EXCEPTION in handle_block_found()");
    return false;
  }
  
  MINFO("=== handle_block_found() returned: " << (result ? "true" : "false") << " ===");
  
  if (!result)
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
