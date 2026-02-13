#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dfs {
namespace network {

class TCPClient {
public:
    TCPClient();
    ~TCPClient();
    TCPClient(const TCPClient&) = delete;
    TCPClient& operator=(const TCPClient&) = delete;

    bool connect(const std::string& ip, int port);
    bool sendData(const uint8_t* data, size_t len);
    bool sendData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> recvData();
    bool sendMessage(const std::string& message);
    std::string recvMessage();
    void close();
    bool isConnected() const { return connected_; }

private:
    int sock_{-1};
    bool connected_{false};
};

}  // namespace network
}  // namespace dfs
