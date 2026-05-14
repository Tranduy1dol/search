# Search Engine — Architecture

## Overview

A modular C++17 search engine with an inverted index, BM25 ranking, and a gRPC API layer. The engine supports multi-threaded web crawling, binary disk serialization via `mmap`, and Vietnamese stopword filtering. A gRPC service exposes the search capabilities over the network, designed to be consumed by a Go-based Japanese learning app.

```
┌─────────────────────────────────────────────────────────┐
│  Go Japanese Learning App (client)                      │
│  ┌───────────────────────────────────────────────────┐  │
│  │  searchgrpc/client.go  (generated from proto)     │  │
│  └────────────────────────┬──────────────────────────┘  │
└───────────────────────────┼─────────────────────────────┘
                            │ gRPC (HTTP/2)
┌───────────────────────────┼─────────────────────────────┐
│  C++ Search Service       ▼                             │
│  ┌────────────────────────────────────────────────────┐ │
│  │  gRPC Server (grpc::Server)                        │ │
│  │  ├── SearchServiceImpl                             │ │
│  │  │     Search(query, top_k, filters) → results     │ │
│  │  │     GetStats() → index statistics               │ │
│  │  └── Graceful shutdown (SIGINT/SIGTERM)            │ │
│  └────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Core Engine                                       │ │
│  │  ├── Tokenizer (Vietnamese stopwords)              │ │
│  │  ├── InvertedIndex (in-memory postings)            │ │
│  │  ├── DiskIndex (mmap binary serialization)         │ │
│  │  ├── Searcher (BM25 ranking)                       │ │
│  │  └── Crawler (libcurl + lexbor, multi-threaded)    │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

---

## Current Status

| Component | Status | Location |
|-----------|--------|----------|
| Crawler (libcurl + lexbor) | ✅ Done | `src/crawler/crawler.cpp` |
| Tokenizer (whitespace + stopwords) | ✅ Done | `src/indexer/tokenizer.cpp` |
| InvertedIndex (in-memory) | ✅ Done | `src/indexer/inverted_index.cpp` |
| DiskIndex (mmap binary format) | ✅ Done | `src/indexer/disk_index.cpp` |
| Searcher (BM25) | ✅ Done | `src/searcher/searcher.cpp` |
| Multi-threaded indexer CLI | ✅ Done | `app/index_main.cpp` |
| Interactive search CLI | ✅ Done | `app/search_main.cpp` |
| Proto definition (Search, GetStats) | ✅ Done | `proto/grpc_service/v1/search.proto` |
| Protobuf code generation (buf) | ✅ Done | `generated/grpc_service/v1/` |
| gRPC SearchServiceImpl | ✅ Done | `src/server/search_service.cpp` |
| gRPC server entry point | ✅ Done | `app/server_main.cpp` |
| Dockerfile (multi-stage) | ✅ Done | `Dockerfile` |
| CI pipeline (build + lint + docker) | ✅ Done | `.github/workflows/ci.yml` |
| clang-tidy / clang-format | ✅ Done | `.clang-tidy`, `.clang-format` |
| IndexDocument RPC | Not started | — |
| BulkIndex RPC (client streaming) | Not started | — |
| DeleteDocument RPC | Not started | — |
| Live index merge at query time | Not started | — |
| Background flush thread | Not started | — |
| Japanese tokenization (MeCab) | Not started | — |
| Health check service | Not started | — |

---

## Project Structure

```
search/
├── app/                           # Executable entry points
│   ├── index_main.cpp             # Multi-threaded crawler + indexer CLI
│   ├── search_main.cpp            # Interactive search CLI
│   └── server_main.cpp            # gRPC server bootstrap
├── proto/
│   └── grpc_service/v1/
│       └── search.proto           # gRPC service definition
├── generated/
│   └── grpc_service/v1/           # protoc output (search.pb.cc, search.grpc.pb.cc)
├── include/
│   ├── search/                    # Core engine headers
│   │   ├── common/
│   │   │   ├── types.h            # Document, Posting, SearchResult, IndexHeader
│   │   │   └── utils.h            # SplitTokens, Normalize, WriteString, ReadString
│   │   ├── crawler/
│   │   │   └── crawler.h          # CrawledPage, Crawler
│   │   ├── indexer/
│   │   │   ├── tokenizer.h        # Tokenizer (stopword filtering)
│   │   │   ├── inverted_index.h   # InvertedIndex (in-memory)
│   │   │   └── disk_index.h       # DiskIndex (mmap serialization)
│   │   └── searcher/
│   │       └── searcher.h         # Searcher (BM25 ranking)
│   └── server/
│       └── search_service.h       # SearchServiceImpl (gRPC)
├── src/                           # Implementation files (mirrors include/)
│   ├── common/utils.cpp
│   ├── crawler/crawler.cpp
│   ├── indexer/
│   │   ├── tokenizer.cpp
│   │   ├── inverted_index.cpp
│   │   └── disk_index.cpp
│   ├── searcher/searcher.cpp
│   └── server/search_service.cpp
├── tests/                         # GoogleTest unit tests
│   ├── test_utils.cpp
│   ├── test_tokenizer.cpp
│   ├── test_inverted_index.cpp
│   ├── test_disk_index.cpp
│   ├── test_searcher.cpp
│   └── test_crawler.cpp
├── data/
│   ├── stopwords_vi.txt           # Vietnamese stopword dictionary
│   └── seed_urls.txt              # Seed URLs for the crawler
├── cmake/FindLibcurl.cmake
├── docs/architecture.md           # This file
├── .github/workflows/ci.yml      # CI: build, lint, docker
├── .clang-tidy                    # Google style naming conventions
├── .clang-format
├── buf.yaml                       # Buf protobuf tooling config
├── buf.gen.yaml                   # Buf code generation config
├── Dockerfile                     # Multi-stage build → search_server
├── CMakeLists.txt                 # Build configuration
├── Makefile                       # Developer convenience wrapper
└── README.md
```

---

## Proto Definition

```protobuf
// proto/grpc_service/v1/search.proto
syntax = "proto3";
package grpc_service.v1;

service SearchService {
  rpc Search(SearchRequest) returns (SearchResponse);
  rpc GetStats(GetStatsRequest) returns (GetStatsResponse);
}

message SearchRequest {
  string query = 1;
  uint32 top_k = 2;
  ContentFilter filter = 3;
}

message ContentFilter {
  repeated ContentType content_types = 1;
  int32 level = 2;
}

enum ContentType {
  CONTENT_TYPE_UNSPECIFIED = 0;
  CONTENT_TYPE_WORD = 1;
  CONTENT_TYPE_GRAMMAR = 2;
  CONTENT_TYPE_PARAGRAPH = 3;
}

message SearchResponse {
  repeated SearchHit hits = 1;
  uint32 total_hits = 2;
  double query_time_ms = 3;
}

message SearchHit {
  string doc_id = 1;
  double score = 2;
  string title = 3;
  string snippet = 4;
  ContentType content_type = 5;
  int32 level = 6;
}

message GetStatsRequest {}

message GetStatsResponse {
  uint32 total_documents = 1;
  uint32 total_term = 2;
  double avg_document_length = 3;
}
```

---

## Binary Index Format

The `DiskIndex` uses a custom binary format serialized with `fwrite` and read via Linux `mmap`:

```
┌─────────────────────────────────────────────────┐
│ IndexHeader (40 bytes)                          │
│   magic_  (uint32)  = 0x434F4343 ("COCC")      │
│   version_ (uint32) = 1                        │
│   num_docs_ (uint32)                            │
│   num_terms_ (uint32)                           │
│   avg_doc_length_ (double)                      │
│   doc_table_offset_ (uint64)                    │
│   dict_offset_ (uint64)                         │
├─────────────────────────────────────────────────┤
│ Postings Lists (variable length)                │
│   For each term:                                │
│     num_postings (uint32)                       │
│     For each posting:                           │
│       doc_id (uint32)                           │
│       term_freq (uint32)                        │
│       num_positions (uint32)                    │
│       positions[] (uint32 × num_positions)      │
├─────────────────────────────────────────────────┤
│ Document Table (at doc_table_offset_)           │
│   For each document:                            │
│     doc_id (uint32)                             │
│     length (uint32)                             │
│     url (length-prefixed string)                │
│     title (length-prefixed string)              │
├─────────────────────────────────────────────────┤
│ Term Dictionary (at dict_offset_)               │
│   For each term:                                │
│     term (length-prefixed string)               │
│     doc_freq (uint32)                           │
│     postings_offset (uint64)                    │
└─────────────────────────────────────────────────┘
```

The `DiskIndex` constructor reads only the Header, Term Dictionary, and Document Table offsets into memory. Postings lists are read on-demand from the memory-mapped region during query time.

---

## Server Lifecycle

```
server_main.cpp
    │
    ├── Read env vars (INDEX_PATH, STOPWORD_PATH, SEARCH_PORT / PORT)
    ├── Open DiskIndex via mmap
    ├── Create Searcher (owns DiskIndex, uses Tokenizer)
    ├── Create SearchServiceImpl (borrows Searcher* and DiskIndex*)
    ├── Build gRPC server on 0.0.0.0:{port}
    ├── Register signal handlers (SIGINT, SIGTERM → Shutdown)
    └── server->Wait()
```

### Ownership Graph

```
server_main.cpp
  ├── unique_ptr<DiskIndex> ──→ moved into Searcher
  ├── Searcher (owns DiskIndex, owns Tokenizer)
  └── SearchServiceImpl
        ├── Searcher*     (non-owning, borrows from main)
        └── DiskIndex*    (non-owning, raw ptr saved before move)
```

---

## Data Flow: Search Query

```
gRPC SearchRequest { query="食べる", top_k=10 }
    │
    ▼
SearchServiceImpl::Search()
    ├── Start timer
    ├── Searcher::Search(query, top_k)
    │     ├── Tokenizer::Tokenize(query) → ["食べる"] or bigrams
    │     ├── For each token:
    │     │     ├── DiskIndex::GetPostings(token)  ← mmap read
    │     │     └── CalculateBM25(tf, df, doc_len)
    │     ├── Accumulate scores per doc_id
    │     ├── Sort by score (descending)
    │     └── Return top_k SearchResults
    ├── Stop timer → query_time_ms
    ├── Map SearchResult → SearchHit (doc_id as string)
    └── Return SearchResponse
```

---

## Multi-threaded Crawler

```
index_main.cpp
    │
    ├── LoadSeedUrls("data/seed_urls.txt")
    ├── Spawn min(hardware_concurrency, 10) worker threads
    │
    │   Each worker thread:
    │     ├── Creates own Crawler (own libcurl handle)
    │     ├── Lock queue_mutex → grab next URL → unlock
    │     ├── Crawl(url) → CrawledPage  (no lock held during I/O)
    │     └── Lock results_mutex → push page → unlock
    │
    ├── Join all threads
    ├── Build InvertedIndex sequentially from CrawledPages
    └── DiskIndex::Serialize() → data/index/index.bin
```

---

## Build & Dependencies

| Dependency | Source | Purpose |
|------------|--------|---------|
| `libcurl` | System (`apt`/`pacman`) | HTTP downloads in Crawler |
| `lexbor` v2.3.0 | CMake FetchContent (auto) | HTML parsing, DOM text extraction |
| `gRPC` + `Protobuf` | System (`apt`/`pacman`) | RPC framework |
| `GoogleTest` | CMake FetchContent (auto) | Unit testing |

### Build Commands

```bash
# Generate protobuf C++ code (if not already generated)
mkdir -p generated/grpc_service/v1
protoc --proto_path=proto \
  --cpp_out=generated \
  --grpc_out=generated \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) \
  grpc_service/v1/search.proto

# Build everything
make build          # cmake configure + compile + link compile_commands.json

# Run tests
make test

# Run executables
./build/indexer data/seed_urls.txt    # Crawl + build index
./build/search                        # Interactive CLI search
./build/search_server                 # Start gRPC server on :50051
```

### Docker

```bash
docker build -t search-server .
docker run -p 50051:50051 -v ./data/index:/data/index search-server
```

---

## CI Pipeline

`.github/workflows/ci.yml` runs three jobs on every push/PR to `main`:

| Job | Steps |
|-----|-------|
| **build-and-test** | Install deps → generate protobuf → cmake configure → build → ctest |
| **lint** | Install clang-tidy → build → `find src app -name '*.cpp' \| xargs clang-tidy` |
| **docker** | `docker build -t search-server .` |

---

## Configuration

Environment variables for `search_server`:

| Variable | Default | Description |
|----------|---------|-------------|
| `INDEX_PATH` | `data/index/index.bin` | Path to serialized binary index |
| `STOPWORD_PATH` | `data/stopword_vi.txt` | Vietnamese stopwords file |
| `SEARCH_PORT` | `50051` | gRPC listen port |
| `PORT` | — | Fallback port (if `SEARCH_PORT` not set) |

---

## Implementation Phases

### Phase A: gRPC Foundation ✅
1. ~~Add `proto/grpc_service/v1/search.proto`~~
2. ~~Update CMakeLists.txt with gRPC + Protobuf~~
3. ~~Implement `SearchServiceImpl::Search`~~
4. ~~Implement `SearchServiceImpl::GetStats`~~
5. ~~Add `app/server_main.cpp` with signal handling~~
6. ~~Dockerfile (multi-stage build)~~
7. ~~CI pipeline (build + lint + docker)~~

### Phase B: Live Indexing
1. Add `IndexDocument` RPC to proto and `SearchServiceImpl`
2. Add `BulkIndex` RPC (client streaming)
3. Add `DeleteDocument` RPC
4. Hold a second `InvertedIndex` in `SearchServiceImpl` for live docs
5. Merge results from `DiskIndex` + live `InvertedIndex` at query time
6. Background flush thread (live index → disk periodically)

### Phase C: Japanese Support
1. Add Japanese tokenizer (MeCab or character bigrams)
2. Auto-detect language in `Tokenizer` (Japanese vs Vietnamese/English)
3. Add Japanese stopwords list
4. Extend `Document` struct with `content_type` and `level` metadata
5. Implement post-scoring filtering by `ContentFilter`

### Phase D: Production Readiness
1. gRPC health check service (`grpc.health.v1`)
2. Graceful drain (stop accepting new RPCs, finish in-flight)
3. Metrics / observability (request count, latency histogram)
4. Connection pooling guidance for Go client
5. Load testing with `ghz`

---

## Key Design Decisions

1. **String doc_id in proto, uint32 internally** — The Go app uses MongoDB ObjectIDs (strings). The search engine maps them to internal `uint32_t` doc_ids for compact postings. A bidirectional mapping will be needed for Phase B.

2. **Separate header paths for server vs engine** — Engine headers live under `include/search/`, server headers under `include/server/`. This keeps the gRPC layer cleanly separated from the core engine, which has no gRPC dependency.

3. **Raw pointer borrowing in SearchServiceImpl** — The service borrows `Searcher*` and `DiskIndex*` from `server_main.cpp`. The objects are stack-allocated in `main()` and outlive the gRPC server, so this is safe. No shared_ptr overhead needed.

4. **No snippet generation yet** — The search engine returns `doc_id` + `title` + URL. The Go app fetches full content from MongoDB. Snippet generation with keyword highlighting can be added as a later optimization.

5. **Pre-generated protobuf** — The `generated/` directory is checked into the repo (via `buf gen`). This avoids requiring `protoc` and `grpc_cpp_plugin` on every developer machine. CI regenerates from source to verify consistency.
