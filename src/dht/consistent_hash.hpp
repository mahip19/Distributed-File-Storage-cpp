#pragma once

#include <map>
#include <string>
#include <vector>

namespace dfs {
namespace dht {

class ConsistentHash {
public:
    void addNode(const std::string& nodeAddress);
    void removeNode(const std::string& nodeAddress);
    std::string getNodeForKey(const std::string& key) const;
    std::vector<std::string> getNodesForKey(const std::string& key, int k) const;
    std::vector<std::string> getAllNodes() const;
    int getNodeCount() const;
    bool hasNode(const std::string& nodeAddress) const;
    void printRing() const;

private:
    static int hashKey(const std::string& key);
    std::map<int, std::string> ring_;
};

}  // namespace dht
}  // namespace dfs
