#include "common/chunk.hpp"
#include <sstream>

namespace dfs {
namespace common {

std::string Chunk::to_string() const {
    std::ostringstream oss;
    oss << "Chunk{index=" << index << ", hash='" << hash << "', size=" << size << "}";
    return oss.str();
}

}  // namespace common
}  // namespace dfs
