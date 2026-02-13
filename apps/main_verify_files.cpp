#include "client/verify_files.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <original_file> <reconstructed_file>" << std::endl;
        return 1;
    }
    std::string originalPath = argv[1];
    std::string reconstructedPath = argv[2];

    std::cout << "Computing CID for original file..." << std::endl;
    std::string originalCID = dfs::client::computeCID(originalPath);
    if (originalCID.empty()) {
        std::cerr << "Error: Could not process original file" << std::endl;
        return 1;
    }

    std::cout << "Computing CID for reconstructed file..." << std::endl;
    std::string reconstructedCID = dfs::client::computeCID(reconstructedPath);
    if (reconstructedCID.empty()) {
        std::cerr << "Error: Could not process reconstructed file" << std::endl;
        return 1;
    }

    std::cout << "\n--- Results ---\nOriginal CID:      " << originalCID
              << "\nReconstructed CID: " << reconstructedCID << std::endl;

    if (originalCID == reconstructedCID) {
        std::cout << "\nVERIFIED: Files are identical" << std::endl;
        return 0;
    } else {
        std::cout << "\nMISMATCH: Files differ" << std::endl;
        return 1;
    }
}
