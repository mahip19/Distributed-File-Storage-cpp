#pragma once

#include "network/tcp_server.hpp"
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace dfs {
namespace storage {

class StorageNode {
public:
    StorageNode();
    ~StorageNode();
    void start(int port);

private:
    void handleClient(int clientId);
    dfs::network::TCPServer server_;
    std::map<std::string, std::vector<uint8_t>> storage_;
    std::mutex storageMutex_;
    std::atomic<bool> running_{false};
    std::atomic<int> activeHandlers_{0};
    std::condition_variable handlersCv_;
    std::mutex handlersMutex_;
};

}  // namespace storage
}  // namespace dfs
