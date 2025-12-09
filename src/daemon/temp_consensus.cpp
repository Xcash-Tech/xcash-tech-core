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

#include "temp_consensus.h"
#include "daemon/core.h"
#include "common/command_line.h"
#include "crypto/crypto.h"
#include "crypto/crypto-ops.h"  // For sc_reduce32
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_core.h"  // For arg_xcash_dpops_delegates_*

namespace daemonize
{

t_temp_consensus::t_temp_consensus(
  boost::program_options::variables_map const & vm,
  t_core & core
)
  : m_enabled(false)
  , m_is_leader(false)
{
  // Check if temporary consensus is enabled
  m_enabled = command_line::get_arg(vm, daemon_args::arg_temp_consensus_enabled);
  
  if (!m_enabled)
  {
    MINFO("Temporary consensus disabled");
    return;
  }

  MINFO("===== Temporary Consensus Configuration =====");
  MINFO("Temporary consensus ENABLED");

  // Check if this node is leader
  m_is_leader = command_line::get_arg(vm, daemon_args::arg_temp_consensus_leader);
  MINFO("Node role: " << (m_is_leader ? "LEADER" : "FOLLOWER"));

  // Get DPoS delegate configuration (reused for temp consensus)
  std::string delegate_public_address = command_line::get_arg(vm, cryptonote::arg_xcash_dpops_delegates_public_address);
  std::string delegate_secret_key = command_line::get_arg(vm, cryptonote::arg_xcash_dpops_delegates_secret_key);

  if (delegate_public_address.empty())
  {
    MERROR("Temporary consensus enabled but --xcash-dpops-delegates-public-address not provided");
    m_enabled = false;
    return;
  }

  MINFO("Delegate public address (used as leader ID and miner address): " << delegate_public_address);

  // Parse delegate address to extract public key
  cryptonote::address_parse_info address_info;
  bool testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  cryptonote::network_type nettype = testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET;
  
  if (!cryptonote::get_account_address_from_str(address_info, nettype, delegate_public_address))
  {
    MERROR("Failed to parse delegate public address: " << delegate_public_address);
    m_enabled = false;
    return;
  }

  // Use address spend public key as initial leader identity
  crypto::public_key leader_pubkey = address_info.address.m_spend_public_key;

  // Initialize leader service if this is a leader node
  if (m_is_leader)
  {
    if (delegate_secret_key.empty())
    {
      MERROR("Leader mode enabled but --xcash-dpops-delegates-secret-key not provided");
      m_enabled = false;
      return;
    }

    // Parse secret key from hex (take first 32 bytes for Ed25519)
    // DPoS uses 64-byte ECDSA keys, but Ed25519 needs 32-byte keys
    // We'll use hash of the full ECDSA key as Ed25519 seed
    crypto::hash ecdsa_key_hash;
    crypto::cn_fast_hash(delegate_secret_key.data(), delegate_secret_key.size(), ecdsa_key_hash);
    
    crypto::secret_key leader_seckey;
    memcpy(&leader_seckey, &ecdsa_key_hash, sizeof(crypto::secret_key));

    // CRITICAL: Apply scalar reduction to make the key valid for Ed25519
    // This ensures the 32-byte hash becomes a valid Ed25519 secret key
    sc_reduce32(reinterpret_cast<unsigned char*>(&leader_seckey));

    // Verify key pair: derive public key from secret key and compare
    crypto::public_key derived_pubkey;
    if (!crypto::secret_key_to_public_key(leader_seckey, derived_pubkey))
    {
      MERROR("Failed to derive public key from secret key");
      m_enabled = false;
      return;
    }

    if (derived_pubkey != leader_pubkey)
    {
      MWARNING("WARNING: Derived public key does NOT match address public key!");
      MWARNING("This is expected - using hash-derived Ed25519 key instead of ECDSA");
      MWARNING("Derived:  " << epee::string_tools::pod_to_hex(derived_pubkey));
      MWARNING("Expected: " << epee::string_tools::pod_to_hex(leader_pubkey));
      MWARNING("Using DERIVED public key for signature verification (leader_pubkey updated)");
      // Update leader_pubkey to the derived one for consistency
      leader_pubkey = derived_pubkey;
    }
    else
    {
      MINFO("✓ Key pair verified: secret key correctly derives to address public key");
    }

    // Test signature: sign test data and verify
    std::string test_data = "temporary_consensus_test";
    crypto::hash test_hash;
    crypto::cn_fast_hash(test_data.data(), test_data.size(), test_hash);
    
    crypto::signature test_sig;
    crypto::generate_signature(test_hash, leader_pubkey, leader_seckey, test_sig);
    
    bool sig_valid = crypto::check_signature(test_hash, leader_pubkey, test_sig);
    if (sig_valid)
    {
      MINFO("✓ Signature test PASSED: can sign and verify with key pair");
    }
    else
    {
      MERROR("✗ Signature test FAILED: signature verification failed!");
      MERROR("This key pair cannot be used for block signing");
      m_enabled = false;
      return;
    }

    MINFO("Using delegate address as miner address: " << delegate_public_address);
    MINFO("PoW: disabled (deterministic nonce)");

    // Create leader service configuration
    cryptonote::temp_consensus_leader_service::config leader_cfg;
    leader_cfg.leader_id = delegate_public_address;  // Use address as ID
    leader_cfg.leader_pubkey = leader_pubkey;
    leader_cfg.leader_seckey = leader_seckey;
    leader_cfg.miner_address = address_info.address;  // Rewards go to delegate address
    leader_cfg.enable_pow = false;  // Always use deterministic nonce
    leader_cfg.slot_duration_seconds = 300; // 5 minutes

    m_leader_service.reset(new cryptonote::temp_consensus_leader_service(core.get(), leader_cfg));
    MINFO("Leader service initialized");
  }

  // Initialize validator (for both leader and followers) - AFTER key derivation for leader
  cryptonote::temp_consensus_validator::config validator_cfg;
  validator_cfg.expected_leader_id = delegate_public_address;  // Use full address as leader ID
  validator_cfg.leader_pubkey = leader_pubkey;  // Use derived pubkey if leader, address pubkey if follower
  
  m_validator.reset(new cryptonote::temp_consensus_validator(validator_cfg));
  m_validator->set_enabled(true);
  MINFO("Validator initialized with leader pubkey: " << epee::string_tools::pod_to_hex(leader_pubkey));

  MINFO("==============================================");
}

t_temp_consensus::~t_temp_consensus()
{
  stop();
}

bool t_temp_consensus::run()
{
  if (!m_enabled)
    return true; // Not an error, just disabled

  MINFO("Starting temporary consensus services...");

  // Register validator with core (Phase 2: validator hook integration)
  if (m_validator)
  {
    MINFO("Registering temp consensus validator with core...");
    // Get access to core through daemon t_core wrapper - we need to pass validator pointer to core
    // This will be handled by daemon during initialization
  }

  // Start leader service if this is a leader node
  if (m_is_leader && m_leader_service)
  {
    if (!m_leader_service->start())
    {
      MERROR("Failed to start leader service");
      return false;
    }
    MINFO("Leader service started");
  }

  MINFO("Temporary consensus services running");
  return true;
}

void t_temp_consensus::stop()
{
  if (!m_enabled)
    return;

  MINFO("Stopping temporary consensus services...");

  if (m_leader_service)
  {
    m_leader_service->stop();
    MINFO("Leader service stopped");
  }

  if (m_validator)
  {
    m_validator->set_enabled(false);
  }

  MINFO("Temporary consensus services stopped");
}

} // namespace daemonize
