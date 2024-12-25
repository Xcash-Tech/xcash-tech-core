#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <iostream>
 #include <vector>
#include <string>
#include <memory> // For std::shared_ptr


#include "common/send_and_receive_data.h"
#include "common/blocking_tcp_client.h"
#include "send_and_receive_data.h"


std::string send_and_receive_data(std::string IP_address,std::string data2, int send_or_receive_socket_data_timeout_settings)
{
  // Variables
  std::string string;
  auto connection_timeout = boost::posix_time::milliseconds(CONNECTION_TIMEOUT_SETTINGS);
  auto send_and_receive_data_timeout = boost::posix_time::milliseconds(send_or_receive_socket_data_timeout_settings);

  try
  {
    // add the end string to the data
    data2 += SOCKET_END_STRING;

    client c;
    c.connect(IP_address, SEND_DATA_PORT, connection_timeout);
    
    // send the message and read the response
    c.write_line(data2, send_and_receive_data_timeout);
    string = c.read_until('}', send_and_receive_data_timeout);
  }
  catch (std::exception &ex)
  {
    return "";
  }
  return string;
}


namespace xcash_net {
// Function to send a message and receive a reply from a single server asynchronously
void xcash_send_msg_async(
    const std::string& server,
    const std::string& port,
    const std::string& message,
    boost::asio::io_context& io_context,
    std::vector<XcashResult>* results,
    std::function<void()> on_complete,
    const std::string& message_ender = "|END|"
) {
    auto socket = std::make_shared<tcp::socket>(io_context);
    auto timer = std::make_shared<boost::asio::steady_timer>(io_context);
    auto reply_buffer = std::make_shared<boost::asio::streambuf>();
    auto result = std::make_shared<XcashResult>();

    result->server_info = server + ":" + port;

    const std::string xcash_message = message + "|END|";

    try {
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve(server, port);
    
        // Start timer for timeout
        timer->expires_after(std::chrono::milliseconds(300));
        timer->async_wait([=](const boost::system::error_code& ec) mutable {
            if (!ec) {
                socket->close(); // Timeout occurred
                result->reply = "Error: Connection Timeout occurred";
                results->push_back(*result);
                on_complete();
            }
        });


        // Start connection
        socket->async_connect(*endpoints.begin(), [=](const boost::system::error_code& ec) mutable {
            timer->cancel(); // Cancel timer on successful connection

            if (ec) {
                result->reply = "Error: " + ec.message();
                results->push_back(*result);
                on_complete();
                return;
            }

            timer->expires_after(std::chrono::milliseconds(600));
            timer->async_wait([=](const boost::system::error_code& ec) mutable {
                if (!ec) {
                    socket->close(); // Timeout occurred
                    result->reply = "Error: Write Timeout occurred";
                    results->push_back(*result);
                    on_complete();
                }
            });

            // Send message
            boost::asio::async_write(*socket, boost::asio::buffer(xcash_message), [=](const boost::system::error_code& ec, std::size_t) mutable {
                timer->cancel(); // Cancel timer on successful read
                if (ec) {
                    result->reply = "Error: " + ec.message();
                    results->push_back(*result);
                    on_complete();
                    return;
                }

                // Start timer for timeout
                timer->expires_after(std::chrono::seconds(6));
                timer->async_wait([=](const boost::system::error_code& ec) mutable {
                    if (!ec) {
                        socket->close(); // Timeout occurred
                        result->reply = "Error: Read Timeout occurred";
                        results->push_back(*result);
                        on_complete();
                    }
                });

                // Receive reply
                boost::asio::async_read_until(*socket, *reply_buffer, message_ender, [=](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
                    timer->cancel(); // Cancel timer on successful read
                    if (!ec) {
                        result->reply = std::string(boost::asio::buffers_begin(reply_buffer->data()),
                                                    boost::asio::buffers_begin(reply_buffer->data()) + bytes_transferred);
                    } else {
                        result->reply = "Error: " + ec.message();
                    }
                    results->push_back(*result);
                    on_complete();
                });
            });
        });
    } catch (const std::exception& ex) {
        result->reply = "Error: " + std::string(ex.what());
        results->push_back(*result);
        on_complete();
    }
}


void xcash_send_multi_msg_async(
    const std::vector<std::string>& servers,
    const std::string& message,
    std::vector<XcashResult>* results,
    const std::string& message_ender = "|END|"
    ) {
    boost::asio::io_context io_context;
    std::size_t pending_operations = servers.size();
    std::vector<XcashResult> all_results;

    if (servers.empty()) {
        std::cerr << "Error: No servers provided." << std::endl;
        return;
    }
    bool all_done = false;

    // Completion handler
    auto on_complete = [&]() {
        if (--pending_operations == 0) {
            all_done = true;
            io_context.stop();
        }
    };

    // Start sending messages asynchronously
    for (const auto& server : servers) {
        xcash_send_msg_async(server, "18283", message, io_context, &all_results, on_complete, message_ender);
    }

    io_context.run();
    while (!all_done) {
        // io_context.run_one(); // Run only one asynchronous operation at a time
        // Optionally, add a small delay to avoid tight looping
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (auto result : all_results) {
    // std::cout << "Server: " << result.server_info << "\n";
    if (result.reply.rfind("Error:", 0) != 0) {
        // Remove message ender
        if (result.reply.size() >= message_ender.size() && 
            result.reply.compare(result.reply.size() - message_ender.size(), message_ender.size(), message_ender) == 0) {
            result.reply.erase(result.reply.size() - message_ender.size());
    
            results->push_back(result);
        }
        // std::cout << "Reply: " << result.reply << "\n";
    }else{
        // std::cout << result.server_info << " : " << result.reply << "\n";
    }
}

}

std::vector<std::string> extract_block_verifiers_IP_address_list(const std::string& message) {
    std::vector<std::string> ip_list;
    std::string key = "block_verifiers_IP_address_list";
    std::size_t start_pos = message.find(key);

    if (start_pos != std::string::npos) {
        start_pos = message.find(":", start_pos + key.length());
        start_pos = message.find("\"", start_pos + 1);
        std::size_t end_pos = message.find("\"", start_pos + 1);
        if (start_pos != std::string::npos && end_pos != std::string::npos) {
            std::string ip_list_str = message.substr(start_pos + 1, end_pos - start_pos - 1);
            std::stringstream ss(ip_list_str);
            std::string ip;
            while (std::getline(ss, ip, '|')) {
                ip_list.push_back(ip);
            }
        }
    }

    return ip_list;
}


void get_block_hashes(std::size_t block_height, std::vector<std::string>& servers, std::vector<std::string>& hashes) {
    std::string message_str = "{\r\n \"message_settings\": \"XCASH_GET_BLOCK_HASH\",\r\n\"block_height\": " + std::to_string(block_height) + "\r\n}";

    std::vector<XcashResult> results;
    xcash_send_multi_msg_async(servers, message_str, &results);

    // Populate the cache with the results
    for (const auto& result : results) {
        if (!result.reply.empty()) {
            std::size_t start_pos = result.reply.find("\"block_hash\":\"");
            if (start_pos != std::string::npos) {
                start_pos += std::string("\"block_hash\":\"").length();
                std::size_t end_pos = result.reply.find("\"", start_pos);
                if (end_pos != std::string::npos) {
                    std::string hash = result.reply.substr(start_pos, end_pos - start_pos);
                    hashes.push_back({hash});
                }
            }
        }
    }
}


} // namespace xcash_net


