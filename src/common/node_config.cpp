#include "common/node_config.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace dfs {
namespace common {

NodeConfig::NodeConfig(const std::string& configFilePath, int myNodeId)
    : myNodeId_(myNodeId) {
    loadConfig(configFilePath);
}

void NodeConfig::loadConfig(const std::string& configFilePath) {
    std::ifstream file(configFilePath);
    if (!file) {
        std::cerr << "Error loading config file: " << configFilePath << std::endl;
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        if (line[start] == '#') continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end == std::string::npos ? line.size() : end - start + 1);
        if (line.empty()) continue;

        std::istringstream iss(line);
        int id;
        std::string host;
        int port;
        if (iss >> id >> host >> port) {
            nodes_.emplace_back(id, host, port);
        }
    }
}

NodeInfo NodeConfig::getMyNode() const {
    for (const auto& n : nodes_) {
        if (n.id == myNodeId_) return n;
    }
    return NodeInfo{};
}

std::vector<NodeInfo> NodeConfig::getAllNodes() const {
    return nodes_;
}

std::vector<NodeInfo> NodeConfig::getPeerNodes() const {
    std::vector<NodeInfo> out;
    for (const auto& n : nodes_) {
        if (n.id != myNodeId_) out.push_back(n);
    }
    return out;
}

NodeInfo NodeConfig::getNodeById(int id) const {
    for (const auto& n : nodes_) {
        if (n.id == id) return n;
    }
    return NodeInfo{};
}

}  // namespace common
}  // namespace dfs
