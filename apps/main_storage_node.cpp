#include "common/node_config.hpp"
#include "storage/storage_node.hpp"
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
    dfs::storage::StorageNode node;
    node.start(myNode.port);
    return 0;
}
