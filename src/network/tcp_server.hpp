#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace dfs {
namespace network {

struct SocketContext {
    int socket{-1};
    bool valid{false};
};

class TCPServer {
public:
    TCPServer();
    ~TCPServer();
    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;

    bool start(int port);
    int acceptClient();
    bool sendData(int clientId, const uint8_t* data, size_t len);
    bool sendData(int clientId, const std::vector<uint8_t>& data);
    std::vector<uint8_t> recvData(int clientId);
    bool sendMessage(int clientId, const std::string& message);
    std::string recvMessage(int clientId);
    void closeClient(int clientId);
    void stop();

private:
    int serverSock_{-1};
    bool running_{false};
    std::map<int, SocketContext> clients_;
    int nextClientId_{1};
    std::mutex clientsMutex_;
};

}  // namespace network
}  // namespace dfs
