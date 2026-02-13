#include "client/client.hpp"
#include "client/verify_files.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage:\n  " << argv[0] << " upload <filepath>\n  "
                  << argv[0] << " download <filename> <output_path>" << std::endl;
        return 1;
    }

    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};

    dfs::client::Client client(storageNodes, metadataNodes);
    std::string command = argv[1];
    std::string arg1 = argv[2];

    if (command == "upload") {
        client.uploadFile(arg1);
    } else if (command == "download") {
        if (argc < 4) {
            std::cout << "Usage: download <filename> <output_path>" << std::endl;
            return 1;
        }
        std::string outputPath = argv[3];
        client.downloadFile(arg1, outputPath);
        std::cout << "Verifying integrity..." << std::endl;
        std::string computedCID = dfs::client::computeCID(outputPath);
        std::cout << "Integrity CID: " << computedCID << std::endl;
    } else {
        std::cout << "Unknown command: " << command << std::endl;
        return 1;
    }
    return 0;
}
