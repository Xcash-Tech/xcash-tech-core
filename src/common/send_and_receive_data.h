#pragma once

#include <thread>
#include <future>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <stdlib.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/asio.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/thread.hpp>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/system/system_error.hpp>
#include <boost/asio/write.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using boost::asio::deadline_timer;
using boost::asio::ip::tcp;



#include "cryptonote_config.h"

std::string send_and_receive_data(std::string IP_address,std::string data2, int send_or_receive_socket_data_timeout_settings = SEND_OR_RECEIVE_SOCKET_DATA_TIMEOUT_SETTINGS);

namespace xcash_net {
// Structure to store results
struct XcashResult {
    std::string server_info;
    std::string reply;
};
void xcash_send_msg_async(const std::string &server, const std::string &port, const std::string &message, boost::asio::io_context &io_context, std::vector<XcashResult> *results, std::function<void()> on_complete, const std::string &message_ender);
void xcash_send_multi_msg_async(const std::vector<std::string> &servers, const std::string &message, std::vector<XcashResult> *results, const std::string &message_ender);
std::vector<std::string> extract_block_verifiers_IP_address_list(const std::string &message);
void get_block_hashes(std::size_t block_height, std::vector<std::string> &servers, std::vector<std::string> &hashes);
}