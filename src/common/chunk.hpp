#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dfs {
namespace common {

struct Chunk {
    int index{0};
    std::string hash;
    std::vector<uint8_t> data;
    int size{0};

    std::string to_string() const;
};

}  // namespace common
}  // namespace dfs
