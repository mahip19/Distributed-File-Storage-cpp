#include "common/file_utils.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace dfs {
namespace common {

std::vector<Chunk> splitFileIntoChunks(const std::string& filepath) {
    std::vector<Chunk> chunks;
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        std::cerr << "Error, file cannot be opened " << filepath << std::endl;
        return chunks;
    }

    std::vector<uint8_t> buffer(CHUNK_SIZE);
    int index = 0;

    while (file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE) || file.gcount() > 0) {
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            Chunk chunk;
            chunk.index = index++;
            chunk.size = static_cast<int>(bytesRead);
            chunk.data.assign(buffer.begin(), buffer.begin() + bytesRead);
            chunks.push_back(std::move(chunk));
        }
    }
    return chunks;
}

bool reconstructFile(const std::vector<Chunk>& chunks, const std::string& outputPath) {
    if (chunks.empty()) {
        std::cerr << "Empty chunks. Can't reconstruct." << std::endl;
        return false;
    }

    std::vector<Chunk> sortedChunks = chunks;
    std::sort(sortedChunks.begin(), sortedChunks.end(),
              [](const Chunk& a, const Chunk& b) { return a.index < b.index; });

    for (size_t i = 0; i < sortedChunks.size(); ++i) {
        if (sortedChunks[i].index != static_cast<int>(i)) {
            std::cerr << "Error: missing chunk " << i << std::endl;
            return false;
        }
    }

    std::ofstream out(outputPath, std::ios::binary);
    if (!out) {
        std::cerr << "Error: file could not be created " << outputPath << std::endl;
        return false;
    }

    for (const auto& chunk : sortedChunks) {
        out.write(reinterpret_cast<const char*>(chunk.data.data()), chunk.size);
    }
    return true;
}

}  // namespace common
}  // namespace dfs
