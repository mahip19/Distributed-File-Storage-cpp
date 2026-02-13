cmake_minimum_required(VERSION 3.14)
project(DistributedFileStorage CXX)

set(CMAKE_CXX_STANDARD 17)

# Library: core (common + network + dht). SHA-256 is self-contained (no OpenSSL).
add_library(dfs_core
  src/common/chunk.cpp
  src/common/file_utils.cpp
  src/common/hash_utils.cpp
  src/common/node_config.cpp
  src/common/sha256.cpp
  src/network/tcp_client.cpp
  src/network/tcp_server.cpp
  src/dht/consistent_hash.cpp
)
target_include_directories(dfs_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Storage Node executable
add_executable(storage_node apps/main_storage_node.cpp)
target_link_libraries(storage_node PRIVATE dfs_core)

# Metadata Node executable
add_executable(metadata_node apps/main_metadata_node.cpp)
target_link_libraries(metadata_node PRIVATE dfs_core)

# Library: metadata + storage nodes (depend on core)
add_library(dfs_nodes
  src/storage/storage_node.cpp
  src/metadata/metadata_node.cpp
)
target_link_libraries(dfs_nodes PRIVATE dfs_core)

# Re-link storage_node and metadata_node to use dfs_nodes (they include the logic)
# Actually we have main_* that just start the nodes - the node logic is in dfs_nodes.
# So storage_node needs to link dfs_nodes. Let me fix CMake: main_storage_node.cpp
# will call StorageNode::run(), so we need to link storage_node to dfs_nodes.
target_link_libraries(storage_node PRIVATE dfs_nodes)
target_link_libraries(metadata_node PRIVATE dfs_nodes)

# Client library (client + verify)
add_library(dfs_client
  src/client/client.cpp
  src/client/verify_files.cpp
)
target_link_libraries(dfs_client PRIVATE dfs_core)

# Client executable
add_executable(client apps/main_client.cpp)
target_link_libraries(client PRIVATE dfs_client)

# Verify executable
add_executable(verify_files apps/main_verify_files.cpp)
target_link_libraries(verify_files PRIVATE dfs_client)

# System tests
add_executable(system_tests apps/main_system_tests.cpp)
target_link_libraries(system_tests PRIVATE dfs_client dfs_nodes)

# Performance experiments
add_executable(performance_experiments apps/main_performance_experiments.cpp)
target_link_libraries(performance_experiments PRIVATE dfs_client dfs_nodes)

# Performance evaluation
add_executable(performance_evaluation apps/main_performance_evaluation.cpp)
target_link_libraries(performance_evaluation PRIVATE dfs_client dfs_nodes)
