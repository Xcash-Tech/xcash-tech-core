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

  // Get common configuration
  std::string leader_id = command_line::get_arg(vm, daemon_args::arg_temp_consensus_leader_id);
  std::string leader_pubkey_hex = command_line::get_arg(vm, daemon_args::arg_temp_consensus_leader_pubkey);

  if (leader_id.empty())
  {
    MERROR("Temporary consensus enabled but --temp-consensus-leader-id not provided");
    m_enabled = false;
    return;
  }

  if (leader_pubkey_hex.empty())
  {
    MERROR("Temporary consensus enabled but --temp-consensus-leader-pubkey not provided");
    m_enabled = false;
    return;
  }

  MINFO("Leader ID: " << leader_id);
  MINFO("Leader pubkey: " << leader_pubkey_hex);

  // Parse public key
  crypto::public_key leader_pubkey;
  if (!epee::string_tools::hex_to_pod(leader_pubkey_hex, leader_pubkey))
  {
    MERROR("Failed to parse leader public key from hex");
    m_enabled = false;
    return;
  }

  // Initialize validator (for both leader and followers)
  cryptonote::temp_consensus_validator::config validator_cfg;
  validator_cfg.expected_leader_id = leader_id;
  validator_cfg.leader_pubkey = leader_pubkey;
  
  m_validator.reset(new cryptonote::temp_consensus_validator(validator_cfg));
  m_validator->set_enabled(true);
  MINFO("Validator initialized");

  // Initialize leader service if this is a leader node
  if (m_is_leader)
  {
    std::string leader_seckey_hex = command_line::get_arg(vm, daemon_args::arg_temp_consensus_leader_seckey);
    std::string miner_address_str = command_line::get_arg(vm, daemon_args::arg_temp_consensus_miner_address);
    bool with_pow = command_line::get_arg(vm, daemon_args::arg_temp_consensus_with_pow);

    if (leader_seckey_hex.empty())
    {
      MERROR("Leader mode enabled but --temp-consensus-leader-seckey not provided");
      m_enabled = false;
      return;
    }

    if (miner_address_str.empty())
    {
      MERROR("Leader mode enabled but --temp-consensus-miner-address not provided");
      m_enabled = false;
      return;
    }

    // Parse secret key
    crypto::secret_key leader_seckey;
    if (!epee::string_tools::hex_to_pod(leader_seckey_hex, leader_seckey))
    {
      MERROR("Failed to parse leader secret key from hex");
      m_enabled = false;
      return;
    }

    // Parse miner address
    cryptonote::address_parse_info miner_address_info;
    bool testnet = command_line::get_arg(vm, cryptonote::arg_testnet_on);
    bool stagenet = command_line::get_arg(vm, cryptonote::arg_stagenet_on);
    cryptonote::network_type nettype = testnet ? cryptonote::TESTNET : stagenet ? cryptonote::STAGENET : cryptonote::MAINNET;
    
    if (!cryptonote::get_account_address_from_str(miner_address_info, nettype, miner_address_str))
    {
      MERROR("Failed to parse miner address: " << miner_address_str);
      m_enabled = false;
      return;
    }

    MINFO("Miner address: " << miner_address_str);
    MINFO("PoW enabled: " << (with_pow ? "yes" : "no"));

    // Create leader service configuration
    cryptonote::temp_consensus_leader_service::config leader_cfg;
    leader_cfg.leader_id = leader_id;
    leader_cfg.leader_pubkey = leader_pubkey;
    leader_cfg.leader_seckey = leader_seckey;
    leader_cfg.miner_address = miner_address_info.address;
    leader_cfg.enable_pow = with_pow;
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
