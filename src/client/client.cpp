#include "client/client.hpp"
#include "common/file_utils.hpp"
#include "common/hash_utils.hpp"
#include "network/tcp_client.hpp"
#include <chrono>
#include <iostream>
#include <sstream>
#include <sys/stat.h>

namespace dfs {
namespace client {

static int64_t getFileSize(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return 0;
    return static_cast<int64_t>(st.st_size);
}

Client::Client(const std::vector<std::string>& storageNodes,
               const std::vector<std::string>& metadataNodes)
    : metadataNodes_(metadataNodes) {
    for (const auto& node : storageNodes) {
        dht_.addNode(node);
    }
}

void Client::uploadFile(const std::string& filepath) {
    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Uploading " << filepath << std::endl;

    auto chunks = common::splitFileIntoChunks(filepath);
    if (chunks.empty()) {
        std::cerr << "File is empty or not found" << std::endl;
        return;
    }
    common::hashAllChunks(chunks);

    std::vector<std::string> hashes;
    for (const auto& c : chunks) hashes.push_back(c.hash);
    std::string rootHash = common::computeRootHash(hashes);
    std::cout << "Root Hash (CID): " << rootHash << std::endl;

    const int replicationFactor = 2;
    auto startChunkUpload = std::chrono::steady_clock::now();
    for (const auto& chunk : chunks) {
        auto nodes = dht_.getNodesForKey(chunk.hash, replicationFactor);
        std::cout << "Chunk " << chunk.index << " -> ";
        for (const auto& n : nodes) std::cout << n << " ";
        std::cout << std::endl;

        int successCount = 0;
        for (const auto& nodeAddr : nodes) {
            if (uploadChunkToNode(chunk, nodeAddr)) {
                successCount++;
            } else {
                std::cerr << "  Failed to upload to " << nodeAddr << std::endl;
            }
        }
        if (successCount == 0) {
            std::cerr << "Failed to upload chunk " << chunk.index << " to any node!" << std::endl;
            return;
        }
    }
    lastChunkUploadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startChunkUpload).count();

    auto startMetadataUpload = std::chrono::steady_clock::now();
    bool metadataSuccess = false;
    for (const auto& nodeAddr : metadataNodes_) {
        std::cout << "Trying to put metadata to " << nodeAddr << std::endl;
        if (putMetadataToNode(nodeAddr, filepath, chunks, rootHash)) {
            std::cout << "Metadata uploaded successfully to " << nodeAddr << std::endl;
            metadataSuccess = true;
            break;
        }
        std::cerr << "Failed to connect/write to " << nodeAddr << ". Trying next..." << std::endl;
    }
    lastMetadataUploadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startMetadataUpload).count();

    if (!metadataSuccess) {
        std::cerr << "Failed to upload metadata to any node!" << std::endl;
    } else {
        std::cout << "Upload complete." << std::endl;
    }
    lastTotalUploadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
}

bool Client::putMetadataToNode(const std::string& nodeAddr, const std::string& filepath,
                               const std::vector<common::Chunk>& chunks, const std::string& rootHash) {
    size_t colon = nodeAddr.find(':');
    if (colon == std::string::npos) return false;
    std::string ip = nodeAddr.substr(0, colon);
    int port = std::stoi(nodeAddr.substr(colon + 1));

    network::TCPClient client;
    if (!client.connect(ip, port)) return false;

    int64_t size = getFileSize(filepath);
    size_t slash = filepath.find_last_of("/\\");
    std::string filename = (slash != std::string::npos) ? filepath.substr(slash + 1) : filepath;
    int chunkSize = common::CHUNK_SIZE;
    int totalChunks = static_cast<int>(chunks.size());

    std::string hashesStr;
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i > 0) hashesStr += ",";
        hashesStr += chunks[i].hash;
    }
    std::string cmd = "PUT " + filename + " " + std::to_string(size) + " " + std::to_string(chunkSize) +
                      " " + std::to_string(totalChunks) + " " + rootHash + " " + hashesStr;

    if (!client.sendMessage(cmd)) {
        client.close();
        return false;
    }
    std::string response = client.recvMessage();
    client.close();
    return response == "ACK";
}

void Client::downloadFile(const std::string& filename, const std::string& outputPath) {
    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Downloading " << filename << std::endl;

    common::FileMetadata meta;
    bool found = false;
    for (int i = static_cast<int>(metadataNodes_.size()) - 1; i >= 0; --i) {
        meta = getMetadataFromNode(metadataNodes_[i], filename);
        if (!meta.filename.empty() || !meta.chunkHashes.empty()) {
            std::cout << "Retrieved metadata from " << metadataNodes_[i] << std::endl;
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "File not found in metadata (or all nodes down)." << std::endl;
        return;
    }
    std::cout << "Metadata found. Root: " << meta.rootHash << std::endl;

    std::vector<common::Chunk> chunks;
    for (size_t i = 0; i < meta.chunkHashes.size(); ++i) {
        const std::string& hash = meta.chunkHashes[i];
        auto nodes = dht_.getNodesForKey(hash, 2);
        std::vector<uint8_t> data;
        for (const auto& node : nodes) {
            data = downloadChunkFromNode(hash, node);
            if (!data.empty()) {
                std::cout << "Retrieved chunk " << i << " from " << node << std::endl;
                break;
            }
        }
        if (data.empty()) {
            std::cerr << "Failed to retrieve chunk " << i << std::endl;
            return;
        }
        common::Chunk c;
        c.index = static_cast<int>(i);
        c.hash = hash;
        c.data = std::move(data);
        c.size = static_cast<int>(c.data.size());
        chunks.push_back(std::move(c));
    }

    if (common::reconstructFile(chunks, outputPath)) {
        std::cout << "File reconstructed at " << outputPath << std::endl;
        std::cout << "Verifying integrity... (use verify_files tool to check)" << std::endl;
    } else {
        std::cerr << "Reconstruction failed." << std::endl;
    }
    lastTotalDownloadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
}

common::FileMetadata Client::getMetadataFromNode(const std::string& nodeAddr, const std::string& filename) {
    common::FileMetadata meta;
    size_t colon = nodeAddr.find(':');
    if (colon == std::string::npos) return meta;
    std::string ip = nodeAddr.substr(0, colon);
    int port = std::stoi(nodeAddr.substr(colon + 1));

    network::TCPClient client;
    if (!client.connect(ip, port)) return meta;
    if (!client.sendMessage("GET " + filename)) {
        client.close();
        return meta;
    }
    std::string response = client.recvMessage();
    client.close();

    if (response.size() > 6 && response.substr(0, 6) == "FOUND ") {
        std::istringstream iss(response.substr(6));
        std::string hashesStr;
        iss >> meta.fileSize >> meta.chunkSize >> meta.totalChunks >> meta.rootHash;
        if (iss >> hashesStr) {
            meta.filename = filename;
            meta.chunkHashes.clear();
            std::istringstream hs(hashesStr);
            std::string h;
            while (std::getline(hs, h, ',')) {
                if (!h.empty()) meta.chunkHashes.push_back(h);
            }
        }
    }
    return meta;
}

bool Client::uploadChunkToNode(const common::Chunk& chunk, const std::string& nodeAddr) {
    size_t colon = nodeAddr.find(':');
    if (colon == std::string::npos) return false;
    std::string ip = nodeAddr.substr(0, colon);
    int port = std::stoi(nodeAddr.substr(colon + 1));

    network::TCPClient client;
    if (!client.connect(ip, port)) return false;
    if (!client.sendMessage("STORE " + chunk.hash)) {
        client.close();
        return false;
    }
    std::string response = client.recvMessage();
    if (response != "READY") {
        client.close();
        return false;
    }
    if (!client.sendData(chunk.data)) {
        client.close();
        return false;
    }
    response = client.recvMessage();
    client.close();
    return response == "ACK";
}

std::vector<uint8_t> Client::downloadChunkFromNode(const std::string& hash, const std::string& nodeAddr) {
    size_t colon = nodeAddr.find(':');
    if (colon == std::string::npos) return {};
    std::string ip = nodeAddr.substr(0, colon);
    int port = std::stoi(nodeAddr.substr(colon + 1));

    network::TCPClient client;
    if (!client.connect(ip, port)) return {};
    if (!client.sendMessage("GET " + hash)) {
        client.close();
        return {};
    }
    std::string response = client.recvMessage();
    if (response == "FOUND") {
        auto data = client.recvData();
        client.close();
        return data;
    }
    client.close();
    return {};
}

}  // namespace client
}  // namespace dfs
