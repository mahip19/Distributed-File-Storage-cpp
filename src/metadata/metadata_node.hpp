#pragma once

#include "common/file_metadata.hpp"
#include "network/tcp_client.hpp"
#include "network/tcp_server.hpp"
#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace dfs {
namespace metadata {

enum class Role { HEAD, MIDDLE, TAIL, SINGLE };

class MetadataNode {
public:
    MetadataNode(const std::string& nextNodeIp, int nextNodePort);
    void start(int port);

private:
    void handleClient(int clientId);
    void healthCheckLoop();
    bool pingNext();
    void handleNextNodeFailure();
    void notifyNextOfPredecessor();
    void handlePut(int clientId, const std::string& command);
    bool forwardPut(const std::string& command);
    void handleGet(int clientId, const std::string& filename);

    dfs::network::TCPServer server_;
    std::map<std::string, common::FileMetadata> metadataStore_;
    std::mutex storeMutex_;
    std::mutex chainMutex_;
    std::atomic<bool> running_{false};

    std::string nextNodeIp_;
    int nextNodePort_{-1};
    std::string prevNodeIp_;
    int prevNodePort_{-1};
    std::string skipToIp_;
    int skipToPort_{-1};
    Role role_{Role::HEAD};
    int myPort_{0};
    std::atomic<int> activeHandlers_{0};
    std::condition_variable handlersCv_;
    std::mutex handlersMutex_;
};

}  // namespace metadata
}  // namespace dfs
