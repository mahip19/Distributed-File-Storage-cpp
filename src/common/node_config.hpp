#pragma once

#include "common/node_info.hpp"
#include <string>
#include <vector>

namespace dfs {
namespace common {

class NodeConfig {
public:
    explicit NodeConfig(const std::string& configFilePath, int myNodeId = -1);
    NodeInfo getMyNode() const;
    std::vector<NodeInfo> getAllNodes() const;
    std::vector<NodeInfo> getPeerNodes() const;
    NodeInfo getNodeById(int id) const;

private:
    void loadConfig(const std::string& configFilePath);
    std::vector<NodeInfo> nodes_;
    int myNodeId_;
};

}  // namespace common
}  // namespace dfs
