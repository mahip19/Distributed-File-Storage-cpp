#include "metadata/metadata_node.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

namespace dfs {
namespace metadata {

MetadataNode::MetadataNode(const std::string& nextNodeIp, int nextNodePort)
    : nextNodeIp_(nextNodeIp), nextNodePort_(nextNodePort) {
    if (nextNodePort == -1) {
        role_ = Role::TAIL;
    }
}

void MetadataNode::start(int port) {
    myPort_ = port;
    if (!server_.start(port)) {
        std::cerr << "Failed to start metadata node on port " << port << std::endl;
        return;
    }
    running_ = true;
    std::cout << "Metadata Node started on port " << port << " Role: " << (role_ == Role::TAIL ? "TAIL" : "HEAD")
              << " Next: " << nextNodePort_ << std::endl;

    std::thread healthThread([this]() { healthCheckLoop(); });
    healthThread.detach();

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
    std::unique_lock<std::mutex> lock(handlersMutex_);
    handlersCv_.wait(lock, [this]() { return activeHandlers_.load() == 0; });
}

void MetadataNode::healthCheckLoop() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        if (nextNodePort_ != -1) {
            if (!pingNext()) {
                std::cout << "Port " << myPort_ << ": Next node " << nextNodePort_ << " failed!" << std::endl;
                handleNextNodeFailure();
            }
        }
    }
}

bool MetadataNode::pingNext() {
    dfs::network::TCPClient client;
    if (!client.connect(nextNodeIp_, nextNodePort_)) return false;
    if (!client.sendMessage("PING")) {
        client.close();
        return false;
    }
    std::string resp = client.recvMessage();
    client.close();
    return resp == "PONG";
}

void MetadataNode::handleNextNodeFailure() {
    std::lock_guard<std::mutex> lock(chainMutex_);
    if (skipToPort_ != -1) {
        std::cout << "Port " << myPort_ << ": Recovering using skip node -> " << skipToPort_ << std::endl;
        nextNodeIp_ = skipToIp_;
        nextNodePort_ = skipToPort_;
        skipToIp_.clear();
        skipToPort_ = -1;
        notifyNextOfPredecessor();
    } else {
        std::cout << "Port " << myPort_ << ": No skip node. Becoming TAIL." << std::endl;
        nextNodeIp_.clear();
        nextNodePort_ = -1;
        role_ = (prevNodePort_ == -1) ? Role::SINGLE : Role::TAIL;
    }
}

void MetadataNode::notifyNextOfPredecessor() {
    dfs::network::TCPClient client;
    if (client.connect(nextNodeIp_, nextNodePort_)) {
        client.sendMessage("UPDATE_PREV 127.0.0.1 " + std::to_string(myPort_));
        client.close();
    }
}

void MetadataNode::handleClient(int clientId) {
    while (running_) {
        std::string command = server_.recvMessage(clientId);
        if (command.empty()) break;

        std::istringstream iss(command);
        std::string op;
        iss >> op;

        if (op == "PUT") {
            handlePut(clientId, command);
        } else if (op == "GET") {
            std::string filename;
            if (iss >> filename) handleGet(clientId, filename);
        } else if (op == "PING") {
            server_.sendMessage(clientId, "PONG");
        } else if (op == "UPDATE_PREV") {
            std::string ip;
            int port;
            if (iss >> ip >> port) {
                std::lock_guard<std::mutex> lock(chainMutex_);
                prevNodeIp_ = ip;
                prevNodePort_ = port;
                if (role_ == Role::HEAD) role_ = Role::MIDDLE;
                if (role_ == Role::SINGLE) role_ = Role::TAIL;
                std::cout << "Port " << myPort_ << ": Updated prev to " << prevNodePort_ << ". New Role: MIDDLE/TAIL" << std::endl;
                server_.sendMessage(clientId, "ACK");
            }
        } else if (op == "UPDATE_NEXT") {
            std::string ip;
            int port;
            if (iss >> ip >> port) {
                std::lock_guard<std::mutex> lock(chainMutex_);
                nextNodeIp_ = ip;
                nextNodePort_ = port;
                if (role_ == Role::TAIL) role_ = Role::MIDDLE;
                if (role_ == Role::SINGLE) role_ = Role::HEAD;
                std::cout << "Port " << myPort_ << ": Updated next to " << nextNodePort_ << std::endl;
                server_.sendMessage(clientId, "ACK");
            }
        } else if (op == "SET_SKIP") {
            std::string ip;
            int port;
            if (iss >> ip >> port) {
                std::lock_guard<std::mutex> lock(chainMutex_);
                skipToIp_ = ip;
                skipToPort_ = port;
                std::cout << "Port " << myPort_ << ": Set skip node to " << skipToPort_ << std::endl;
                server_.sendMessage(clientId, "ACK");
            }
        } else if (op == "GET_STATUS") {
            std::string roleStr = (role_ == Role::HEAD) ? "HEAD" : (role_ == Role::MIDDLE) ? "MIDDLE" : (role_ == Role::TAIL) ? "TAIL" : "SINGLE";
            server_.sendMessage(clientId, "ROLE=" + roleStr + " NEXT=" + std::to_string(nextNodePort_) + " PREV=" + std::to_string(prevNodePort_));
        } else if (op == "DIE") {
            std::cout << "Port " << myPort_ << ": Received DIE command. Stopping..." << std::endl;
            running_ = false;
            server_.stop();
            break;
        } else {
            server_.sendMessage(clientId, "ERROR");
        }
    }
    server_.closeClient(clientId);
}

void MetadataNode::handlePut(int clientId, const std::string& command) {
    std::istringstream iss(command);
    std::string op, filename, rootHash, hashesStr;
    int64_t fileSize;
    int chunkSize, totalChunks;
    if (!(iss >> op >> filename >> fileSize >> chunkSize >> totalChunks >> rootHash >> hashesStr)) {
        server_.sendMessage(clientId, "ERROR_ARGS");
        return;
    }
    common::FileMetadata meta;
    meta.filename = filename;
    meta.fileSize = fileSize;
    meta.chunkSize = chunkSize;
    meta.totalChunks = totalChunks;
    meta.rootHash = rootHash;
    meta.chunkHashes.clear();
    std::string h;
    std::istringstream hs(hashesStr);
    while (std::getline(hs, h, ',')) {
        if (!h.empty()) meta.chunkHashes.push_back(h);
    }

    {
        std::lock_guard<std::mutex> lock(storeMutex_);
        metadataStore_[filename] = meta;
    }
    std::cout << "Port " << myPort_ << ": Stored metadata for " << filename << std::endl;

    bool success = true;
    if (role_ != Role::TAIL && role_ != Role::SINGLE && nextNodePort_ != -1) {
        success = forwardPut(command);
    }

    if (success) {
        server_.sendMessage(clientId, "ACK");
    } else {
        server_.sendMessage(clientId, "ERROR_FORWARD");
    }
}

bool MetadataNode::forwardPut(const std::string& command) {
    dfs::network::TCPClient client;
    if (!client.connect(nextNodeIp_, nextNodePort_)) {
        std::cerr << "Port " << myPort_ << ": Failed to forward to " << nextNodePort_ << std::endl;
        return false;
    }
    if (!client.sendMessage(command)) {
        client.close();
        return false;
    }
    std::string response = client.recvMessage();
    client.close();
    return response == "ACK";
}

void MetadataNode::handleGet(int clientId, const std::string& filename) {
    if (role_ != Role::TAIL && role_ != Role::SINGLE) {
        server_.sendMessage(clientId, "REDIRECT_TO_TAIL");
        return;
    }
    common::FileMetadata meta;
    {
        std::lock_guard<std::mutex> lock(storeMutex_);
        auto it = metadataStore_.find(filename);
        if (it == metadataStore_.end()) {
            server_.sendMessage(clientId, "NOT_FOUND");
            return;
        }
        meta = it->second;
    }
    std::string hashesStr;
    for (size_t i = 0; i < meta.chunkHashes.size(); ++i) {
        if (i > 0) hashesStr += ",";
        hashesStr += meta.chunkHashes[i];
    }
    std::string msg = "FOUND " + std::to_string(meta.fileSize) + " " + std::to_string(meta.chunkSize) + " " +
                      std::to_string(meta.totalChunks) + " " + meta.rootHash + " " + hashesStr;
    server_.sendMessage(clientId, msg);
}

}  // namespace metadata
}  // namespace dfs
