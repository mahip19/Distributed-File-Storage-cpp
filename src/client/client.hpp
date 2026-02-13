#pragma once

#include "common/chunk.hpp"
#include "common/file_metadata.hpp"
#include "dht/consistent_hash.hpp"
#include <string>
#include <vector>

namespace dfs {
namespace client {

class Client {
public:
    Client(const std::vector<std::string>& storageNodes,
           const std::vector<std::string>& metadataNodes);

    void uploadFile(const std::string& filepath);
    void downloadFile(const std::string& filename, const std::string& outputPath);
    std::vector<uint8_t> downloadChunkFromNode(const std::string& hash, const std::string& nodeAddr);

    long lastMetadataUploadDuration{0};
    long lastChunkUploadDuration{0};
    long lastTotalUploadDuration{0};
    long lastTotalDownloadDuration{0};

private:
    bool putMetadataToNode(const std::string& nodeAddr, const std::string& filepath,
                           const std::vector<common::Chunk>& chunks, const std::string& rootHash);
    common::FileMetadata getMetadataFromNode(const std::string& nodeAddr, const std::string& filename);
    bool uploadChunkToNode(const common::Chunk& chunk, const std::string& nodeAddr);

    dht::ConsistentHash dht_;
    std::vector<std::string> metadataNodes_;
};

}  // namespace client
}  // namespace dfs
