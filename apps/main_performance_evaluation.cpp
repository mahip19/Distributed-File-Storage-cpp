#include "client/client.hpp"
#include "metadata/metadata_node.hpp"
#include "storage/storage_node.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <thread>

static const char* OUTPUT_FILE = "performance_evaluation.txt";
static const char* TEST_DIR = "test_data";

static void startStorageNode(int port) {
    std::thread([port]() {
        dfs::storage::StorageNode node;
        node.start(port);
    }).detach();
}

static void startMetadataNode(int port, const std::string& nextIp, int nextPort) {
    std::thread([port, nextIp, nextPort]() {
        dfs::metadata::MetadataNode node(nextIp, nextPort);
        node.start(port);
    }).detach();
}

static void startCluster() {
    startStorageNode(8001);
    startStorageNode(8002);
    startMetadataNode(9003, "", -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9002, "127.0.0.1", 9003);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9001, "127.0.0.1", 9002);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

static void createDummyFile(const std::string& filepath, int size) {
    std::vector<uint8_t> data(static_cast<size_t>(size));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& b : data) b = static_cast<uint8_t>(dis(gen));
    std::ofstream f(filepath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()), data.size());
}

static void runTest(std::ofstream& writer, dfs::client::Client& client,
                    const std::string& filename, int sizeBytes) {
    std::cout << "\nRunning Test: " << filename << " (" << sizeBytes << " bytes)\n";
    std::string filepath = std::string(TEST_DIR) + "/" + filename;
    createDummyFile(filepath, sizeBytes);

    writer << "Test Case: " << filename << "\n-------------------------------\nFile Size: " << sizeBytes << " bytes\n";

    auto startUpload = std::chrono::steady_clock::now();
    client.uploadFile(filepath);
    auto endUpload = std::chrono::steady_clock::now();
    long uploadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endUpload - startUpload).count();
    double uploadThroughput = sizeBytes / 1024.0 / 1024.0 / (uploadLatency / 1000.0);

    writer << "Total Upload Latency: " << uploadLatency << " ms\n";
    writer << "Upload Throughput: " << std::fixed << std::setprecision(2) << uploadThroughput << " MB/s\n";
    writer << "Chunk Upload Latency: " << client.lastChunkUploadDuration << " ms\n";
    writer << "Metadata Chain Latency: " << client.lastMetadataUploadDuration << " ms\n";

    std::string downloadPath = std::string(TEST_DIR) + "/downloaded_" + filename;
    auto startDownload = std::chrono::steady_clock::now();
    client.downloadFile(filename, downloadPath);
    auto endDownload = std::chrono::steady_clock::now();
    long downloadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(endDownload - startDownload).count();
    double downloadThroughput = sizeBytes / 1024.0 / 1024.0 / (downloadLatency / 1000.0);

    writer << "Total Download Latency: " << downloadLatency << " ms\n";
    writer << "Download Throughput: " << std::fixed << std::setprecision(2) << downloadThroughput << " MB/s\n\n";
    writer.flush();

    remove(downloadPath.c_str());
}

int main(int argc, char* argv[]) {
    mkdir(TEST_DIR, 0755);
    std::ofstream writer(OUTPUT_FILE);
    if (!writer) {
        std::cerr << "Cannot open " << OUTPUT_FILE << std::endl;
        return 1;
    }

    writer << "Performance Evaluation Results\n==============================\n\n";
    std::cout << "Starting Cluster...\n";
    startCluster();

    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};
    dfs::client::Client client(storageNodes, metadataNodes);

    runTest(writer, client, "small_file.dat", 100 * 1024);
    runTest(writer, client, "medium_file.dat", 5 * 1024 * 1024);
    runTest(writer, client, "large_file.dat", 20 * 1024 * 1024);

    std::cout << "Performance evaluation complete. Results saved to " << OUTPUT_FILE << "\n";
    return 0;
}
