# Makefile for C++ Distributed File Storage (no CMake required)
# Requires: C++17 compiler, OpenSSL (openssl-dev / libssl-dev)

CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -I src -O2
LDFLAGS =

SRC = src
COMMON = $(SRC)/common/chunk.cpp $(SRC)/common/file_utils.cpp $(SRC)/common/hash_utils.cpp $(SRC)/common/node_config.cpp $(SRC)/common/sha256.cpp
NETWORK = $(SRC)/network/tcp_client.cpp $(SRC)/network/tcp_server.cpp
DHT = $(SRC)/dht/consistent_hash.cpp
CORE_OBJS = $(COMMON:.cpp=.o) $(NETWORK:.cpp=.o) $(DHT:.cpp=.o)
NODES_OBJS = $(SRC)/storage/storage_node.o $(SRC)/metadata/metadata_node.o
CLIENT_OBJS = $(SRC)/client/client.o $(SRC)/client/verify_files.o

all: build_dir storage_node metadata_node client verify_files system_tests performance_experiments performance_evaluation

build_dir:
	@mkdir -p out

# Core library objects
$(CORE_OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NODES_OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CLIENT_OBJS): %.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

storage_node: $(CORE_OBJS) $(NODES_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_storage_node.cpp $(CORE_OBJS) $(NODES_OBJS) -o out/storage_node $(LDFLAGS)

metadata_node: $(CORE_OBJS) $(NODES_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_metadata_node.cpp $(CORE_OBJS) $(NODES_OBJS) -o out/metadata_node $(LDFLAGS)

client: $(CORE_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_client.cpp $(CORE_OBJS) $(CLIENT_OBJS) -o out/client $(LDFLAGS)

verify_files: $(CORE_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_verify_files.cpp $(CORE_OBJS) $(CLIENT_OBJS) -o out/verify_files $(LDFLAGS)

system_tests: $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_system_tests.cpp $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS) -o out/system_tests $(LDFLAGS) -pthread

performance_experiments: $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_performance_experiments.cpp $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS) -o out/performance_experiments $(LDFLAGS) -pthread

performance_evaluation: $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) apps/main_performance_evaluation.cpp $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS) -o out/performance_evaluation $(LDFLAGS) -pthread

clean:
	rm -f $(CORE_OBJS) $(NODES_OBJS) $(CLIENT_OBJS) out/storage_node out/metadata_node out/client out/verify_files out/system_tests out/performance_experiments out/performance_evaluation

test: system_tests
	./out/system_tests

.PHONY: all build_dir clean test storage_node metadata_node client verify_files system_tests performance_experiments performance_evaluation
