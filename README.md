# Distributed File Storage — C++ Port

This directory contains a full C++ reimplementation of the Java distributed file storage system. The design matches the original: **consistent hashing** for the storage DHT, **chain replication** for metadata, and the same TCP protocol so components are interchangeable.

## Requirements

- **C++17** compiler (GCC, Clang, or MSVC)
- No external dependencies (SHA-256 is implemented in-tree)

## Build

### Using Make (recommended)

```bash
cd cpp
make
```

Executables are produced in `out/`:

- `out/storage_node` — storage node
- `out/metadata_node` — metadata node (chain replication)
- `out/client` — upload/download client
- `out/verify_files` — compare CIDs of two files
- `out/system_tests` — automated system tests
- `out/performance_experiments` — scalability/throughput experiments
- `out/performance_evaluation` — performance evaluation report

### Using CMake

```bash
cd cpp
mkdir build && cd build
cmake ..
make
```

Binaries will be in `build/` (no OpenSSL required).

## Usage

### 1. Start storage nodes (e.g. 2 nodes)

Using config file (see `../nodes.conf`):

```bash
./out/storage_node ../nodes.conf 1 &
./out/storage_node ../nodes.conf 2 &
```

Or run on specific ports (no config):

- Storage: 8001, 8002 (match client defaults)
- Metadata: 9001, 9002, 9003

### 2. Start metadata chain (Tail → Mid → Head)

```bash
./out/metadata_node ../nodes.conf 13 &   # Tail (9003)
./out/metadata_node ../nodes.conf 12 &   # Mid (9002)
./out/metadata_node ../nodes.conf 11 &   # Head (9001)
```

### 3. Client

```bash
# Upload
./out/client upload /path/to/file

# Download
./out/client download <filename> /path/to/output
```

### 4. Verify two files (e.g. original vs downloaded)

```bash
./out/verify_files original.bin downloaded.bin
```

### 5. Run system tests (starts nodes internally)

```bash
./out/system_tests
```

### 6. Performance experiments (writes `results.csv`)

```bash
./out/performance_experiments
python3 ../plot_results.py   # if pandas available
```

## Project layout

```
cpp/
├── CMakeLists.txt
├── Makefile
├── README.md
├── apps/                    # Main entry points
│   ├── main_client.cpp
│   ├── main_metadata_node.cpp
│   ├── main_performance_evaluation.cpp
│   ├── main_performance_experiments.cpp
│   ├── main_storage_node.cpp
│   ├── main_system_tests.cpp
│   └── main_verify_files.cpp
└── src/
    ├── common/              # Chunk, FileMetadata, hashing, file I/O, config
    ├── network/             # TCP client/server (length-prefixed messages)
    ├── dht/                 # Consistent hash ring
    ├── storage/             # Storage node (STORE/GET)
    ├── metadata/            # Metadata node (chain replication, PUT/GET)
    └── client/              # Client and verify_files
```

## Protocol compatibility

The wire format is unchanged from the Java version:

- **Messages**: 4-byte big-endian length + UTF-8 payload.
- **Storage**: `STORE <hash>`, `GET <hash>`, `READY`/`ACK`, `FOUND`/`NOT_FOUND`.
- **Metadata**: `PUT filename size chunkSize totalChunks rootHash hash1,hash2,...`, `GET filename`, `FOUND ...` / `NOT_FOUND`, `PING`/`PONG`, `DIE`, etc.

So a Java client can talk to C++ nodes and vice versa.

## Notes

- **Config**: Storage/metadata node IDs and ports follow `nodes.conf` (e.g. IDs 1–10 storage, 11–20 metadata). The client defaults to `127.0.0.1:8001`, `8002` and `127.0.0.1:9001`–`9003`.
- **Replication**: Storage replication factor is 2; metadata uses a 3-node chain (Head → Mid → Tail).
- **Integrity**: Upload uses SHA-256 per chunk and a root hash; download can be checked with `verify_files` or by recomputing the CID.
