#include "dht/consistent_hash.hpp"
#include <algorithm>
#include <iostream>

namespace dfs {
namespace dht {

int ConsistentHash::hashKey(const std::string& key) {
    int h = 0;
    for (unsigned char c : key) {
        h = 31 * h + static_cast<int>(c);
    }
    return h;
}

void ConsistentHash::addNode(const std::string& nodeAddress) {
    ring_[hashKey(nodeAddress)] = nodeAddress;
}

void ConsistentHash::removeNode(const std::string& nodeAddress) {
    ring_.erase(hashKey(nodeAddress));
}

std::string ConsistentHash::getNodeForKey(const std::string& key) const {
    if (ring_.empty()) return "";
    int pos = hashKey(key);
    auto it = ring_.lower_bound(pos);
    if (it == ring_.end()) it = ring_.begin();
    return it->second;
}

std::vector<std::string> ConsistentHash::getNodesForKey(const std::string& key, int k) const {
    std::vector<std::string> nodes;
    if (ring_.empty()) return nodes;
    int pos = hashKey(key);
    auto it = ring_.lower_bound(pos);
    if (it == ring_.end()) it = ring_.begin();
    std::vector<int> keys;
    for (const auto& p : ring_) keys.push_back(p.first);
    size_t startIndex = 0;
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == it->first) { startIndex = i; break; }
    }
    for (size_t i = 0; i < keys.size() && static_cast<int>(nodes.size()) < k; ++i) {
        size_t idx = (startIndex + i) % keys.size();
        std::string node = ring_.at(keys[idx]);
        if (std::find(nodes.begin(), nodes.end(), node) == nodes.end()) {
            nodes.push_back(node);
        }
    }
    return nodes;
}

std::vector<std::string> ConsistentHash::getAllNodes() const {
    std::vector<std::string> out;
    for (const auto& p : ring_) out.push_back(p.second);
    return out;
}

int ConsistentHash::getNodeCount() const {
    return static_cast<int>(ring_.size());
}

bool ConsistentHash::hasNode(const std::string& nodeAddress) const {
    return ring_.count(hashKey(nodeAddress)) != 0;
}

void ConsistentHash::printRing() const {
    std::cout << "=== Consistent Hash Ring ===\nNodes: " << ring_.size() << "\n";
    for (const auto& p : ring_) {
        std::cout << "  Position " << p.first << " -> " << p.second << "\n";
    }
    std::cout << "=============================\n";
}

}  // namespace dht
}  // namespace dfs
