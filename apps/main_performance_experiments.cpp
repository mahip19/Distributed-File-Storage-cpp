#include "client/client.hpp"
#include "metadata/metadata_node.hpp"
#include "network/tcp_client.hpp"
#include "storage/storage_node.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

static const char* RESULTS_FILE = "results.csv";

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

static void killNode(int port) {
    dfs::network::TCPClient client;
    if (client.connect("127.0.0.1", port)) {
        client.sendMessage("DIE");
        client.close();
    }
}

static void startCluster(int storageCount) {
    for (int i = 1; i <= storageCount; ++i) {
        startStorageNode(8000 + i);
    }
    startMetadataNode(9003, "", -1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9002, "127.0.0.1", 9003);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    startMetadataNode(9001, "127.0.0.1", 9002);
}

static void stopCluster(int storageCount) {
    for (int i = 1; i <= storageCount; ++i) {
        killNode(8000 + i);
    }
    killNode(9001);
    killNode(9002);
    killNode(9003);
}

struct ExperimentResult {
    double avgUpload = 0, avgDownload = 0, successRate = 0;
};

static ExperimentResult runWorkload(int clientCount, int fileSize) {
    std::vector<std::string> storageNodes = {"127.0.0.1:8001", "127.0.0.1:8002"};
    std::vector<std::string> metadataNodes = {"127.0.0.1:9001", "127.0.0.1:9002", "127.0.0.1:9003"};

    std::vector<long> uploadLatencies;
    std::vector<long> downloadLatencies;
    std::vector<bool> successes;
    std::mutex mtx;

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto& b : data) b = static_cast<uint8_t>(dis(gen));

    std::vector<std::thread> threads;
    for (int i = 0; i < clientCount; ++i) {
        threads.emplace_back([&, i]() {
            try {
                std::string fname = "perf_test_" + std::to_string(i) + ".bin";
                {
                    std::ofstream f(fname, std::ios::binary);
                    f.write(reinterpret_cast<const char*>(data.data()), data.size());
                }
                dfs::client::Client c(storageNodes, metadataNodes);
                c.uploadFile(fname);
                long up = c.lastTotalUploadDuration;
                std::string outName = "perf_out_" + std::to_string(i) + ".bin";
                c.downloadFile(fname, outName);
                long down = c.lastTotalDownloadDuration;
                remove(fname.c_str());
                remove(outName.c_str());
                std::lock_guard<std::mutex> lock(mtx);
                uploadLatencies.push_back(up);
                downloadLatencies.push_back(down);
                successes.push_back(true);
            } catch (...) {
                std::lock_guard<std::mutex> lock(mtx);
                successes.push_back(false);
            }
        });
    }
    for (auto& t : threads) t.join();

    ExperimentResult r;
    if (!uploadLatencies.empty()) {
        double sumUp = 0, sumDown = 0;
        for (size_t j = 0; j < uploadLatencies.size(); ++j) {
            sumUp += uploadLatencies[j];
            sumDown += downloadLatencies[j];
        }
        r.avgUpload = sumUp / uploadLatencies.size();
        r.avgDownload = sumDown / downloadLatencies.size();
    }
    int ok = 0;
    for (bool b : successes) if (b) ++ok;
    r.successRate = clientCount > 0 ? (100.0 * ok / clientCount) : 0;
    return r;
}

int main(int argc, char* argv[]) {
    std::cout << "=== STARTING PERFORMANCE EXPERIMENTS ===\n";
    std::ofstream writer(RESULTS_FILE);
    if (!writer) {
        std::cerr << "Cannot open " << RESULTS_FILE << std::endl;
        return 1;
    }
    writer << "Experiment,Variable,Value,AvgUploadLatency,AvgDownloadLatency,SuccessRate\n";

    // Scalability
    std::cout << "\n[Experiment A] Scalability (Varying Clients)\n";
    std::vector<int> clientCounts = {1, 5, 10, 20, 50};
    int fileSize = 100 * 1024;
    for (int clients : clientCounts) {
        std::cout << "  Running with " << clients << " clients...\n";
        startCluster(2);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ExperimentResult result = runWorkload(clients, fileSize);
        writer << "Scalability,Clients," << clients << ","
               << result.avgUpload << "," << result.avgDownload << "," << result.successRate << "\n";
        writer.flush();
        std::cout << "    -> Avg Upload: " << result.avgUpload << " ms, Avg Download: " << result.avgDownload
                  << " ms, Success: " << result.successRate << "%\n";
        stopCluster(2);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Throughput
    std::cout << "\n[Experiment B] Throughput (Varying File Size)\n";
    std::vector<int> fileSizes = {10 * 1024, 100 * 1024, 1024 * 1024, 10 * 1024 * 1024};
    for (int size : fileSizes) {
        std::cout << "  Running with file size " << size << " bytes...\n";
        startCluster(2);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ExperimentResult result = runWorkload(1, size);
        writer << "Throughput,FileSize," << size << ","
               << result.avgUpload << "," << result.avgDownload << "," << result.successRate << "\n";
        writer.flush();
        std::cout << "    -> Avg Upload: " << result.avgUpload << " ms, Avg Download: " << result.avgDownload
                  << " ms, Success: " << result.successRate << "%\n";
        stopCluster(2);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "=== EXPERIMENTS COMPLETED. Results saved to " << RESULTS_FILE << " ===\n";
    return 0;
}
