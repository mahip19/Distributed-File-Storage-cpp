#include "common/hash_utils.hpp"
#include "common/sha256.hpp"

namespace dfs {
namespace common {

std::string computeSHA256(const uint8_t* data, size_t len) {
    return sha256(data, len);
}

std::string computeSHA256(const std::vector<uint8_t>& data) {
    return sha256(data);
}

void hashChunk(Chunk& chunk) {
    std::vector<uint8_t> validData(chunk.data.begin(), chunk.data.begin() + chunk.size);
    chunk.hash = computeSHA256(validData);
}

void hashAllChunks(std::vector<Chunk>& chunks) {
    for (auto& chunk : chunks) {
        hashChunk(chunk);
    }
}

std::string computeRootHash(const std::vector<std::string>& chunkHashes) {
    std::string combined;
    for (const auto& h : chunkHashes) {
        combined += h;
    }
    return computeSHA256(reinterpret_cast<const uint8_t*>(combined.data()), combined.size());
}

}  // namespace common
}  // namespace dfs
