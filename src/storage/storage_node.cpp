#include "storage/storage_node.hpp"
#include <iostream>
#include <sstream>

namespace dfs {
namespace storage {

StorageNode::StorageNode() = default;

StorageNode::~StorageNode() {
    running_ = false;
    server_.stop();
}

void StorageNode::start(int port) {
    if (!server_.start(port)) {
        std::cerr << "Failed to start storage node on port " << port << std::endl;
        return;
    }
    running_ = true;
    std::cout << "Storage Node started on port " << port << std::endl;

    while (running_) {
        int clientId = server_.acceptClient();
        if (clientId != -1) {
            activeHandlers_++;
            std::thread([this, clientId]() {
                handleClient(clientId);
                activeHandlers_--;
                handlersCv_.notify_one();
            }).detach();
        }
    }
    // Wait for all client handlers to finish before destroying server
    std::unique_lock<std::mutex> lock(handlersMutex_);
    handlersCv_.wait(lock, [this]() { return activeHandlers_.load() == 0; });
}

void StorageNode::handleClient(int clientId) {
    while (running_) {
        std::string command = server_.recvMessage(clientId);
        if (command.empty()) break;

        std::istringstream iss(command);
        std::string op;
        iss >> op;

        if (op == "STORE") {
            std::string hash;
            if (!(iss >> hash)) {
                server_.sendMessage(clientId, "ERROR");
                continue;
            }
            server_.sendMessage(clientId, "READY");
            auto data = server_.recvData(clientId);
            if (!data.empty()) {
                size_t sz = data.size();
                {
                    std::lock_guard<std::mutex> lock(storageMutex_);
                    storage_[hash] = std::move(data);
                }
                server_.sendMessage(clientId, "ACK");
                std::cout << "Stored chunk: " << hash << " (" << sz << " bytes)" << std::endl;
            } else {
                break;
            }
        } else if (op == "GET") {
            std::string hash;
            if (!(iss >> hash)) {
                server_.sendMessage(clientId, "ERROR");
                continue;
            }
            std::vector<uint8_t> data;
            {
                std::lock_guard<std::mutex> lock(storageMutex_);
                auto it = storage_.find(hash);
                if (it != storage_.end()) data = it->second;
            }
            if (!data.empty()) {
                server_.sendMessage(clientId, "FOUND");
                server_.sendData(clientId, data);
                std::cout << "Served chunk: " << hash << std::endl;
            } else {
                server_.sendMessage(clientId, "NOT_FOUND");
            }
        } else if (op == "DIE") {
            std::cout << "Received DIE command. Stopping..." << std::endl;
            running_ = false;
            server_.stop();
            break;
        } else {
            server_.sendMessage(clientId, "ERROR");
        }
    }
    server_.closeClient(clientId);
}

}  // namespace storage
}  // namespace dfs
