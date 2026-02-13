#include "network/tcp_client.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace dfs {
namespace network {

TCPClient::TCPClient() = default;

TCPClient::~TCPClient() {
    close();
}

bool TCPClient::connect(const std::string& ip, int port) {
    if (connected_) close();
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0) {
        std::cerr << "Error: socket failed" << std::endl;
        return false;
    }
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Error: invalid address " << ip << std::endl;
        ::close(sock_);
        sock_ = -1;
        return false;
    }
    if (::connect(sock_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Error: connection failed" << std::endl;
        ::close(sock_);
        sock_ = -1;
        return false;
    }
    connected_ = true;
    return true;
}

bool TCPClient::sendData(const uint8_t* data, size_t len) {
    if (!connected_ || sock_ < 0) return false;
    uint32_t len32 = htonl(static_cast<uint32_t>(len));
    if (::send(sock_, &len32, 4, 0) != 4) return false;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(sock_, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool TCPClient::sendData(const std::vector<uint8_t>& data) {
    return sendData(data.data(), data.size());
}

std::vector<uint8_t> TCPClient::recvData() {
    std::vector<uint8_t> result;
    if (!connected_ || sock_ < 0) return result;
    uint32_t len32;
    if (::recv(sock_, &len32, 4, MSG_WAITALL) != 4) return result;
    size_t len = ntohl(len32);
    result.resize(len);
    size_t got = 0;
    while (got < len) {
        ssize_t n = ::recv(sock_, result.data() + got, len - got, 0);
        if (n <= 0) return {};
        got += static_cast<size_t>(n);
    }
    return result;
}

bool TCPClient::sendMessage(const std::string& message) {
    return sendData(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

std::string TCPClient::recvMessage() {
    auto data = recvData();
    if (data.empty()) return "";
    return std::string(data.begin(), data.end());
}

void TCPClient::close() {
    if (connected_ && sock_ >= 0) {
        ::close(sock_);
        sock_ = -1;
        connected_ = false;
    }
}

}  // namespace network
}  // namespace dfs
