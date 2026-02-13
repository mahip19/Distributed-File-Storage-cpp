#pragma once

#include "common/chunk.hpp"
#include <string>
#include <vector>

namespace dfs {
namespace common {

std::string computeSHA256(const uint8_t* data, size_t len);
std::string computeSHA256(const std::vector<uint8_t>& data);
void hashChunk(Chunk& chunk);
void hashAllChunks(std::vector<Chunk>& chunks);
std::string computeRootHash(const std::vector<std::string>& chunkHashes);

}  // namespace common
}  // namespace dfs
