#include "client/client.hpp"
#include "client/verify_files.hpp"
#include "metadata/metadata_node.hpp"
#include "network/tcp_client.hpp"
#include "storage/storage_node.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

static void startStorageNode(int port) {
    std::thread([](int p) {
        dfs::storage::StorageNode node;
        node.start(p);
    }, port).detach();
}

static void startMetadataNode(int port, const std::string& nextIp, int nextPort) {
    std::thread([port, nextIp, nextPort]() {
        dfs::metadata::MetadataNode node(nextIp, nextPort);
        node.start(port);
    }).detach();
}

static void killNode(int port) {
    dfs::network::TCPClient client;
    if (client.connect("127.0.0.1", port)) {
        client.sendMessage("DIE");
        client.close();
    }
}

static void testStorageFailure() {
    std::cout << "\n[TEST] Storage Node Failure (Replication)\n";
    startStorageNode(8001);
    startStorageNode(8002);
    startMetadataNode(9003, "", -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9002, "127.0.0.1", 9003);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9001, "127.0.0.1", 9002);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};
    dfs::client::Client client(storageNodes, metadataNodes);

    std::string filename = "test_storage_fail.txt";
    {
        std::ofstream f(filename);
        f << "Data that should survive storage node failure.";
    }
    client.uploadFile(filename);

    std::cout << ">>> Killing Storage Node 8001...\n";
    killNode(8001);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string outFilename = "test_storage_fail_out.txt";
    client.downloadFile(filename, outFilename);

    std::string originalCID = dfs::client::computeCID(filename);
    std::string downloadedCID = dfs::client::computeCID(outFilename);
    if (originalCID == downloadedCID) {
        std::cout << "[PASS] Storage Failure Test: Integrity Verified.\n";
    } else {
        std::cerr << "[FAIL] Storage Failure Test: Integrity Mismatch!\n";
    }

    killNode(8002);
    killNode(9001);
    killNode(9002);
    killNode(9003);
    remove(filename.c_str());
    remove(outFilename.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

static void testConcurrentClients() {
    std::cout << "\n[TEST] Concurrent Clients (Thread Pool)\n";
    startStorageNode(8001);
    startStorageNode(8002);
    startMetadataNode(9003, "", -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9002, "127.0.0.1", 9003);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9001, "127.0.0.1", 9002);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};

    const int clientCount = 10;
    std::vector<std::string> failures;
    std::mutex failuresMutex;
    std::vector<std::thread> threads;
    for (int i = 0; i < clientCount; ++i) {
        threads.emplace_back([&, i]() {
            try {
                std::string fname = "concurrent_" + std::to_string(i) + ".txt";
                {
                    std::ofstream f(fname);
                    f << "Concurrent data " << i;
                }
                dfs::client::Client c(storageNodes, metadataNodes);
                c.uploadFile(fname);
                std::string outName = "concurrent_out_" + std::to_string(i) + ".txt";
                c.downloadFile(fname, outName);
                if (dfs::client::computeCID(fname) != dfs::client::computeCID(outName)) {
                    std::lock_guard<std::mutex> lock(failuresMutex);
                    failures.push_back("Integrity fail client " + std::to_string(i));
                }
                remove(fname.c_str());
                remove(outName.c_str());
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(failuresMutex);
                failures.push_back(std::string("Exception client ") + std::to_string(i) + ": " + e.what());
            }
        });
    }
    for (auto& t : threads) t.join();

    if (failures.empty()) {
        std::cout << "[PASS] Concurrent Clients Test: All " << clientCount << " clients succeeded.\n";
    } else {
        std::cerr << "[FAIL] Concurrent Clients Test: " << failures.size() << " failures.\n";
        for (const auto& f : failures) std::cerr << f << "\n";
    }

    killNode(8001);
    killNode(8002);
    killNode(9001);
    killNode(9002);
    killNode(9003);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

static void testBinaryFiles() {
    std::cout << "\n[TEST] Binary Files (Image & PDF)\n";
    startStorageNode(8001);
    startStorageNode(8002);
    startMetadataNode(9003, "", -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9002, "127.0.0.1", 9003);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9001, "127.0.0.1", 9002);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};
    dfs::client::Client client(storageNodes, metadataNodes);

    std::vector<std::string> files = {"test_image.jpg", "test_pdf.pdf"};
    for (const auto& filename : files) {
        std::ifstream check(filename);
        if (!check) {
            std::cout << "Skipping " << filename << " (File not found)\n";
            continue;
        }
        std::cout << ">>> Testing " << filename << "\n";
        client.uploadFile(filename);
        std::string outFilename = "restored_" + filename;
        client.downloadFile(filename, outFilename);
        std::string originalCID = dfs::client::computeCID(filename);
        std::string downloadedCID = dfs::client::computeCID(outFilename);
        if (originalCID == downloadedCID) {
            std::cout << "[PASS] " << filename << ": Integrity Verified (" << originalCID << ")\n";
        } else {
            std::cerr << "[FAIL] " << filename << ": Integrity Mismatch!\n";
        }
        remove(outFilename.c_str());
    }
    killNode(8001);
    killNode(8002);
    killNode(9001);
    killNode(9002);
    killNode(9003);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

int main(int argc, char* argv[]) {
    std::cout << "=== STARTING COMPREHENSIVE SYSTEM TESTS ===\n";
    try {
        testStorageFailure();
        testConcurrentClients();
        testBinaryFiles();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    std::cout << "=== ALL TESTS COMPLETED ===\n";
    return 0;
}
