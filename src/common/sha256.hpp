#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace dfs {
namespace common {

// Self-contained SHA-256 (no OpenSSL). Returns 64-char hex string.
std::string sha256(const uint8_t* data, size_t len);
std::string sha256(const std::vector<uint8_t>& data);

}  // namespace common
}  // namespace dfs
