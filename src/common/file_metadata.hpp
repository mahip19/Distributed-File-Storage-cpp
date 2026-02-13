#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dfs {
namespace common {

struct FileMetadata {
    std::string filename;
    std::string rootHash;
    int64_t fileSize{0};
    int chunkSize{0};
    int totalChunks{0};
    std::vector<std::string> chunkHashes;
};

}  // namespace common
}  // namespace dfs
