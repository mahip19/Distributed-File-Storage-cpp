#pragma once

#include "common/chunk.hpp"
#include <string>
#include <vector>

namespace dfs {
namespace common {

constexpr int CHUNK_SIZE = 1048576;  // 1MB

std::vector<Chunk> splitFileIntoChunks(const std::string& filepath);
bool reconstructFile(const std::vector<Chunk>& chunks, const std::string& outputPath);

}  // namespace common
}  // namespace dfs
