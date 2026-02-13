#include "network/tcp_server.hpp"
#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace dfs {
namespace network {

TCPServer::TCPServer() = default;

TCPServer::~TCPServer() {
    stop();
}

bool TCPServer::start(int port) {
    serverSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock_ < 0) {
        std::cerr << "Error: failed to create socket" << std::endl;
        return false;
    }
    int opt = 1;
    if (setsockopt(serverSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        ::close(serverSock_);
        serverSock_ = -1;
        return false;
    }
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (bind(serverSock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Error: failed to bind" << std::endl;
        ::close(serverSock_);
        serverSock_ = -1;
        return false;
    }
    if (listen(serverSock_, 50) < 0) {
        ::close(serverSock_);
        serverSock_ = -1;
        return false;
    }
    running_ = true;
    return true;
}

int TCPServer::acceptClient() {
    if (!running_ || serverSock_ < 0) return -1;
    int clientSock = accept(serverSock_, nullptr, nullptr);
    if (clientSock < 0) {
        std::cerr << "Error: accept failed" << std::endl;
        return -1;
    }
    std::lock_guard<std::mutex> lock(clientsMutex_);
    int id = nextClientId_++;
    clients_[id] = SocketContext{clientSock, true};
    return id;
}

bool TCPServer::sendData(int clientId, const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it == clients_.end() || !it->second.valid) return false;
    int sock = it->second.socket;
    uint32_t len32 = htonl(static_cast<uint32_t>(len));
    if (::send(sock, &len32, 4, 0) != 4) return false;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sock, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool TCPServer::sendData(int clientId, const std::vector<uint8_t>& data) {
    return sendData(clientId, data.data(), data.size());
}

std::vector<uint8_t> TCPServer::recvData(int clientId) {
    int sock = -1;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = clients_.find(clientId);
        if (it == clients_.end() || !it->second.valid) return {};
        sock = it->second.socket;
    }
    uint32_t len32;
    if (::recv(sock, &len32, 4, MSG_WAITALL) != 4) return {};
    size_t len = ntohl(len32);
    std::vector<uint8_t> result(len);
    size_t got = 0;
    while (got < len) {
        ssize_t n = ::recv(sock, result.data() + got, len - got, 0);
        if (n <= 0) return {};
        got += static_cast<size_t>(n);
    }
    return result;
}

bool TCPServer::sendMessage(int clientId, const std::string& message) {
    return sendData(clientId, reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

std::string TCPServer::recvMessage(int clientId) {
    auto data = recvData(clientId);
    if (data.empty()) return "";
    return std::string(data.begin(), data.end());
}

void TCPServer::closeClient(int clientId) {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = clients_.find(clientId);
    if (it != clients_.end()) {
        if (it->second.socket >= 0) ::close(it->second.socket);
        it->second.valid = false;
        it->second.socket = -1;
        clients_.erase(it);
    }
}

void TCPServer::stop() {
    if (running_) {
        running_ = false;
        if (serverSock_ >= 0) {
            ::close(serverSock_);
            serverSock_ = -1;
        }
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& p : clients_) {
            if (p.second.socket >= 0) ::close(p.second.socket);
        }
        clients_.clear();
    }
}

}  // namespace network
}  // namespace dfs
