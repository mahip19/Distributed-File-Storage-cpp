#include "common/node_config.hpp"
#include "metadata/metadata_node.hpp"
#include <algorithm>
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <config_file> <node_id>" << std::endl;
        return 1;
    }
    std::string configFile = argv[1];
    int nodeId = std::stoi(argv[2]);
    dfs::common::NodeConfig config(configFile, nodeId);
    dfs::common::NodeInfo myNode = config.getMyNode();
    if (myNode.port == 0 && myNode.host.empty()) {
        std::cerr << "Error: Node ID " << nodeId << " not found in config file." << std::endl;
        return 1;
    }

    auto allNodes = config.getAllNodes();
    std::vector<dfs::common::NodeInfo> metadataNodes;
    for (const auto& n : allNodes) {
        if (n.id >= 11) metadataNodes.push_back(n);
    }
    std::sort(metadataNodes.begin(), metadataNodes.end(),
              [](const dfs::common::NodeInfo& a, const dfs::common::NodeInfo& b) { return a.id < b.id; });

    std::string nextIp;
    int nextPort = -1;
    for (size_t i = 0; i < metadataNodes.size(); ++i) {
        if (metadataNodes[i].id == nodeId) {
            if (i + 1 < metadataNodes.size()) {
                nextIp = metadataNodes[i + 1].host;
                nextPort = metadataNodes[i + 1].port;
            }
            break;
        }
    }

    dfs::metadata::MetadataNode node(nextIp, nextPort);
    node.start(myNode.port);
    return 0;
}
