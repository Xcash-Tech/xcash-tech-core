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
#include "cryptonote_config.h"  // For NETWORK_DATA_NODE_PUBLIC_ADDRESS_*
#include <sodium.h>  // For crypto_sign_seed_keypair

namespace daemonize
{

t_temp_consensus::t_temp_consensus(
  boost::program_options::variables_map const & vm,
  t_core & core
)
  : m_enabled(false)
  , m_is_leader(false)
  , m_config_error(false)
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
    m_config_error = true;
    return;
  }

  MINFO("Delegate public address (used as leader ID and miner address): " << delegate_public_address);

  // Security: Verify delegate address is one of the authorized seed nodes (first 4 only)
  const std::string authorized_seeds[4] = {
    NETWORK_DATA_NODE_PUBLIC_ADDRESS_1,
    NETWORK_DATA_NODE_PUBLIC_ADDRESS_2,
    NETWORK_DATA_NODE_PUBLIC_ADDRESS_3,
    NETWORK_DATA_NODE_PUBLIC_ADDRESS_4
  };
  
  bool is_authorized = false;
  for (size_t i = 0; i < 4; i++)
  {
    if (delegate_public_address == authorized_seeds[i])
    {
      is_authorized = true;
      MINFO("✓ Address authorized as seed node #" << (i+1));
      break;
    }
  }
  
  if (!is_authorized)
  {
    MERROR("SECURITY: Delegate address is NOT one of the authorized seed nodes!");
    MERROR("Only the 5 hardcoded seed nodes can act as temporary consensus leader");
    m_enabled = false;
    m_config_error = true;
    return;
  }

  // Parse delegate address to extract public key
  cryptonote::address_parse_info address_info;
  bool testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
  bool stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
  cryptonote::network_type nettype = testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET;
  
  if (!cryptonote::get_account_address_from_str(address_info, nettype, delegate_public_address))
  {
    MERROR("Failed to parse delegate public address: " << delegate_public_address);
    m_enabled = false;
    m_config_error = true;
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
      m_config_error = true;
      return;
    }

    // Parse secret key from hex - DPoS uses 64-byte Ed25519 keypair (32 secret + 32 public)
    if (delegate_secret_key.size() != 128)
    {
      MERROR("Invalid delegate secret key length: " << delegate_secret_key.size() << " (expected 128 hex chars)");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    std::string keypair_binary;
    if (!epee::string_tools::parse_hexstr_to_binbuff(delegate_secret_key, keypair_binary))
    {
      MERROR("Failed to parse delegate secret key as hex");
      m_enabled = false;
      m_config_error = true;
      return;
    }

    // DPoS keypair format: [secret_key_32_bytes][public_key_32_bytes]
    // BUT: Need to check if this is actually Ed25519 keypair or seed-based format
    if (keypair_binary.size() != 64)
    {
      MERROR("Invalid keypair size: " << keypair_binary.size() << " (expected 64 bytes)");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    crypto::secret_key leader_seckey;
    crypto::public_key expected_pubkey;
    
    // Copy first 32 bytes as secret key (or seed)
    memcpy(leader_seckey.data, keypair_binary.data(), 32);
    
    // Copy last 32 bytes as expected public key
    memcpy(expected_pubkey.data, keypair_binary.data() + 32, 32);
    
    MINFO("DPoS keypair public key (last 32 bytes): " << epee::string_tools::pod_to_hex(expected_pubkey));
    
    // Debug: print secret key
    MINFO("DPoS secret key (first 32 bytes): " << epee::string_tools::pod_to_hex(leader_seckey));
    
    // DPoS uses seed-based keypair generation via libsodium
    // First 32 bytes is a seed, not a raw Ed25519 secret key
    unsigned char libsodium_seckey[64];  // libsodium format: [secret(32)][public(32)]
    unsigned char libsodium_pubkey[32];
    unsigned char* seed = reinterpret_cast<unsigned char*>(leader_seckey.data);
    
    // Derive keypair from seed using libsodium
    if (crypto_sign_seed_keypair(libsodium_pubkey, libsodium_seckey, seed) != 0)
    {
      MERROR("Failed to derive Ed25519 keypair from seed");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    crypto::public_key derived_pubkey;
    memcpy(derived_pubkey.data, libsodium_pubkey, 32);
    
    MINFO("Libsodium derived pubkey: " << epee::string_tools::pod_to_hex(derived_pubkey));
    
    // Verify derived pubkey matches the one in DPoS keypair
    if (derived_pubkey != expected_pubkey)
    {
      MERROR("SECURITY: Libsodium derived wrong pubkey from seed!");
      MERROR("  Derived:  " << epee::string_tools::pod_to_hex(derived_pubkey));
      MERROR("  Expected: " << epee::string_tools::pod_to_hex(expected_pubkey));
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    MINFO("✓ DPoS seed-based keypair verified");
    
    // CRITICAL: libsodium's 64-byte secret key format is [secret(32)][public(32)]
    // But we need to store the FULL 64-byte key to use with libsodium signing
    // Monero's crypto structures expect 32 bytes, so we'll store the seed part
    // and use libsodium functions directly for signing
    
    // Store the original seed (used to derive the keypair)
    // This is what we'll use with crypto_sign_detached later
    memcpy(leader_seckey.data, seed, 32);
    
    MINFO("Stored seed for libsodium signing: " << epee::string_tools::pod_to_hex(leader_seckey));
    MINFO("Full libsodium seckey (64 bytes): " << epee::string_tools::buff_to_hex_nodelimer(std::string((char*)libsodium_seckey, 64)));
    
    // Note: We'll need to use libsodium's crypto_sign_detached with the full 64-byte key
    // for actual block signing, not Monero's crypto::generate_signature
    MINFO("✓ Ed25519 keypair ready for signing (via libsodium)");
    MINFO("X-CASH address pubkey: " << epee::string_tools::pod_to_hex(leader_pubkey));
    MINFO("(Address pubkey is different - used only for miner rewards)");
    
    // Security: Verify that the derived Ed25519 pubkey matches the expected one for this seed node
    // This prevents using wrong secret key with authorized address (first 4 seeds only)
    const std::string expected_ed25519_pubkeys[4] = {
      NETWORK_DATA_NODE_ED25519_PUBKEY_1,
      NETWORK_DATA_NODE_ED25519_PUBKEY_2,
      NETWORK_DATA_NODE_ED25519_PUBKEY_3,
      NETWORK_DATA_NODE_ED25519_PUBKEY_4
    };
    
    std::string derived_pubkey_hex = epee::string_tools::pod_to_hex(derived_pubkey);
    bool pubkey_matches = false;
    
    for (size_t i = 0; i < 4; i++)
    {
      if (delegate_public_address == authorized_seeds[i])
      {
        if (expected_ed25519_pubkeys[i].empty())
        {
          MWARNING("WARNING: No Ed25519 pubkey configured for seed node #" << (i+1));
          MWARNING("Skipping pubkey verification (development mode)");
          MWARNING("COPY THIS FOR cryptonote_config.h: NETWORK_DATA_NODE_ED25519_PUBKEY_" << (i+1) << " \"" << derived_pubkey_hex << "\"");
          pubkey_matches = true;  // Allow in development
        }
        else if (derived_pubkey_hex == expected_ed25519_pubkeys[i])
        {
          MINFO("✓ Ed25519 pubkey verified for seed node #" << (i+1));
          pubkey_matches = true;
        }
        else
        {
          MERROR("SECURITY: Ed25519 public key mismatch for seed node #" << (i+1));
          MERROR("  Expected: " << expected_ed25519_pubkeys[i]);
          MERROR("  Derived:  " << derived_pubkey_hex);
          MERROR("You are using the WRONG secret key for address: " << delegate_public_address);
          pubkey_matches = false;
        }
        break;
      }
    }
    
    if (!pubkey_matches)
    {
      MERROR("Ed25519 public key verification FAILED");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    // Use the derived Ed25519 public key for consensus (not the address key)
    leader_pubkey = derived_pubkey;

    // Test signature using libsodium directly (DPoS uses libsodium format keys)
    std::string test_data = "temporary_consensus_test";
    crypto::hash test_hash;
    crypto::cn_fast_hash(test_data.data(), test_data.size(), test_hash);
    
    MINFO("Test hash: " << epee::string_tools::pod_to_hex(test_hash));
    MINFO("Signing with libsodium (64-byte seckey)...");
    
    // Sign using libsodium with the full 64-byte secret key
    unsigned char sig_bytes[64];
    unsigned long long sig_len = 64;
    
    if (crypto_sign_detached(sig_bytes, &sig_len, 
                            reinterpret_cast<unsigned char*>(&test_hash), 32,
                            libsodium_seckey) != 0)
    {
      MERROR("✗ Libsodium signing FAILED!");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    MINFO("Generated signature (libsodium): " << epee::string_tools::buff_to_hex_nodelimer(std::string((char*)sig_bytes, 64)));
    
    // Verify using libsodium
    if (crypto_sign_verify_detached(sig_bytes, 
                                    reinterpret_cast<unsigned char*>(&test_hash), 32,
                                    libsodium_pubkey) != 0)
    {
      MERROR("✗ Signature verification FAILED!");
      MERROR("This key pair cannot be used for block signing");
      m_enabled = false;
      m_config_error = true;
      return;
    }
    
    MINFO("✓ Signature test PASSED with libsodium!");
    
    // Also test with Monero's crypto::check_signature
    crypto::signature monero_sig;
    memcpy(&monero_sig, sig_bytes, 64);
    
    if (crypto::check_signature(test_hash, expected_pubkey, monero_sig))
    {
      MINFO("✓ Monero crypto can also verify libsodium signatures!");
    }
    else
    {
      MWARNING("⚠ Monero crypto cannot verify libsodium signatures");
      MWARNING("Will need to use libsodium for both signing and verification");
    }

    MINFO("Using delegate address as miner address: " << delegate_public_address);
    MINFO("PoW: disabled (deterministic nonce)");

    // Create leader service configuration
    cryptonote::temp_consensus_leader_service::config leader_cfg;
    leader_cfg.leader_id = delegate_public_address;  // Use address as ID
    leader_cfg.leader_pubkey = leader_pubkey;        // X-CASH address pubkey (for rewards)
    leader_cfg.leader_ed25519_pubkey = expected_pubkey; // Ed25519 pubkey (for signatures)
    leader_cfg.leader_seckey = leader_seckey;        // Ed25519 secret key (seed, 32 bytes)
    memcpy(leader_cfg.libsodium_seckey, libsodium_seckey, 64); // Full 64-byte libsodium key
    leader_cfg.miner_address = address_info.address;  // Rewards go to delegate address
    leader_cfg.enable_pow = false;  // Always use deterministic nonce
    leader_cfg.slot_duration_seconds = 30; // 30 seconds for testing (was 300 = 5 minutes)

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
  MINFO("=== TEMP CONSENSUS RUN() CALLED ===");
  MINFO("m_enabled = " << (m_enabled ? "true" : "false"));
  MINFO("m_is_leader = " << (m_is_leader ? "true" : "false"));
  MINFO("m_config_error = " << (m_config_error ? "true" : "false"));
  
  // If configuration error occurred, daemon must not start
  if (m_config_error)
  {
    MERROR("Temporary consensus configuration FAILED - daemon cannot start");
    return false;
  }
  
  if (!m_enabled)
  {
    MINFO("Temporary consensus disabled, exiting run()");
    return true; // Not an error, just disabled
  }

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
