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

  // Use address spend public key as leader identity
  crypto::public_key leader_pubkey = address_info.address.m_spend_public_key;

  // Initialize validator (for both leader and followers)
  cryptonote::temp_consensus_validator::config validator_cfg;
  validator_cfg.expected_leader_id = delegate_public_address;  // Use full address as leader ID
  validator_cfg.leader_pubkey = leader_pubkey;
  
  m_validator.reset(new cryptonote::temp_consensus_validator(validator_cfg));
  m_validator->set_enabled(true);
  MINFO("Validator initialized");

  // Initialize leader service if this is a leader node
  if (m_is_leader)
  {
    if (delegate_secret_key.empty())
    {
      MERROR("Leader mode enabled but --xcash-dpops-delegates-secret-key not provided");
      m_enabled = false;
      return;
    }

    // Parse secret key from hex
    crypto::secret_key leader_seckey;
    if (!epee::string_tools::hex_to_pod(delegate_secret_key, leader_seckey))
    {
      MERROR("Failed to parse delegate secret key from hex");
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
