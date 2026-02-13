#include "client/verify_files.hpp"
#include "common/file_utils.hpp"
#include "common/hash_utils.hpp"

namespace dfs {
namespace client {

std::string computeCID(const std::string& filepath) {
    auto chunks = common::splitFileIntoChunks(filepath);
    if (chunks.empty()) return "";
    common::hashAllChunks(chunks);
    std::vector<std::string> chunkHashes;
    for (const auto& c : chunks) {
        chunkHashes.push_back(c.hash);
    }
    return common::computeRootHash(chunkHashes);
}

}  // namespace client
}  // namespace dfs
