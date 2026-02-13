#pragma once

#include <string>

namespace dfs {
namespace common {

struct NodeInfo {
    int id{0};
    std::string host;
    int port{0};

    NodeInfo() = default;
    NodeInfo(int id_, const std::string& host_, int port_)
        : id(id_), host(host_), port(port_) {}

    std::string getAddress() const { return host + ":" + std::to_string(port); }
};

}  // namespace common
}  // namespace dfs
