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

#include <memory>
#include <boost/program_options.hpp>
#include "cryptonote_core/temp_consensus_leader_service.h"
#include "cryptonote_core/temp_consensus_validator.h"
#include "daemon/command_line_args.h"

#undef XCASH_DEFAULT_LOG_CATEGORY
#define XCASH_DEFAULT_LOG_CATEGORY "daemon"

namespace daemonize
{
  // Forward declaration
  class t_core;

  /**
   * @brief Temporary consensus integration for daemon
   * 
   * Manages leader service and validator based on command-line flags.
   * Phase 2: Service runs but doesn't generate blocks yet (stub mode).
   */
  class t_temp_consensus final
  {
  public:
    /**
     * @brief Initialize with command-line variables
     * @param vm Variables map from command line
     * @param core Reference to core
     */
    t_temp_consensus(
      boost::program_options::variables_map const & vm,
      t_core & core
    );

    ~t_temp_consensus();

    /**
     * @brief Start temporary consensus services
     * @return true if started successfully (or disabled)
     */
    bool run();

    /**
     * @brief Stop temporary consensus services
     */
    void stop();

    /**
     * @brief Check if temporary consensus is enabled
     */
    bool is_enabled() const { return m_enabled; }

    /**
     * @brief Check if this node is configured as leader
     */
    bool is_leader() const { return m_is_leader; }

    /**
     * @brief Get validator instance (may be null if not enabled)
     */
    cryptonote::temp_consensus_validator* get_validator() { return m_validator.get(); }

  private:
    bool m_enabled;
    bool m_is_leader;
    bool m_config_error;  // Set to true if configuration failed
    
    std::unique_ptr<cryptonote::temp_consensus_leader_service> m_leader_service;
    std::unique_ptr<cryptonote::temp_consensus_validator> m_validator;
  };

} // namespace daemonize
