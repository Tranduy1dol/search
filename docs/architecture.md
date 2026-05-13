# Search Engine — gRPC Service Architecture

## Overview

This project is a C++ search engine with an inverted index and BM25 ranking. It will be extended into a **gRPC service** that the Go-based Japanese learning app consumes for full-text search over dictionary entries, grammar points, and reading passages.

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
│  │  │     IndexDocument(doc) → status                 │ │
│  │  │     DeleteDocument(doc_id) → status             │ │
│  │  │     GetStats() → index statistics               │ │
│  │  └── HealthServiceImpl (grpc.health.v1)            │ │
│  └────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Core Engine (existing)                            │ │
│  │  ├── Tokenizer (Japanese + Vietnamese stopwords)   │ │
│  │  ├── InvertedIndex (in-memory postings)            │ │
│  │  ├── DiskIndex (mmap serialization)                │ │
│  │  └── Searcher (BM25 ranking)                       │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

---

## Current State

| Component | Status | Location |
|-----------|--------|----------|
| Crawler (libcurl + lexbor) | Done | `src/crawler/` |
| Tokenizer (whitespace + stopwords) | Done | `src/indexer/tokenizer.cpp` |
| InvertedIndex (in-memory) | Done | `src/indexer/inverted_index.cpp` |
| DiskIndex (mmap binary format) | Done | `src/indexer/disk_index.cpp` |
| Searcher (BM25) | Done | `src/searcher/searcher.cpp` |
| CLI entry points | Done | `app/index_main.cpp`, `app/search_main.cpp` |
| gRPC server | **Not started** | — |
| Japanese tokenization | **Not started** | — |

---

## Changes Required

### 1. Proto Definition

```protobuf
syntax = "proto3";
package search.v1;

service SearchService {
  rpc Search(SearchRequest) returns (SearchResponse);
  rpc IndexDocument(IndexDocumentRequest) returns (IndexDocumentResponse);
  rpc BulkIndex(stream IndexDocumentRequest) returns (BulkIndexResponse);
  rpc DeleteDocument(DeleteDocumentRequest) returns (DeleteDocumentResponse);
  rpc GetStats(GetStatsRequest) returns (GetStatsResponse);
}

message SearchRequest {
  string query = 1;
  uint32 top_k = 2;
  ContentFilter filter = 3;
}

message ContentFilter {
  repeated ContentType content_types = 1;  // word, grammar, paragraph
  int32 jlpt_level = 2;                   // 0 = any, 1-5 = specific level
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
  int32 jlpt_level = 6;
}

message IndexDocumentRequest {
  string doc_id = 1;
  string title = 2;
  string content = 3;
  ContentType content_type = 4;
  int32 jlpt_level = 5;
}

message IndexDocumentResponse {
  bool success = 1;
}

message BulkIndexResponse {
  uint32 indexed_count = 1;
  uint32 failed_count = 2;
}

message DeleteDocumentRequest {
  string doc_id = 1;
}

message DeleteDocumentResponse {
  bool success = 1;
}

message GetStatsRequest {}

message GetStatsResponse {
  uint32 total_documents = 1;
  uint32 total_terms = 2;
  double avg_document_length = 3;
}
```

### 2. Japanese Tokenization

The current tokenizer splits on whitespace — fine for Vietnamese/English but not for Japanese (no spaces between words). Options:

| Approach | Pros | Cons |
|----------|------|------|
| **MeCab** (libmecab) | Industry standard, accurate, fast | External dependency, dictionary files |
| **ICU BreakIterator** | Unicode-aware, no extra dict | Less accurate for Japanese compounds |
| **Character n-grams** | Zero dependencies, simple | Larger index, lower precision |

**Recommendation:** MeCab for production accuracy. Fall back to bigram tokenization as a simpler first step that still works for Japanese search.

### 3. Project Structure (after changes)

```
search/
├── proto/
│   └── search/v1/search.proto
├── src/
│   ├── crawler/                    # (existing)
│   ├── indexer/                    # (existing)
│   │   ├── inverted_index.cpp
│   │   ├── disk_index.cpp
│   │   └── tokenizer.cpp          # extend with Japanese support
│   ├── searcher/                   # (existing)
│   └── server/                     # NEW
│       ├── search_service.cpp      # SearchServiceImpl
│       ├── search_service.h
│       └── server.cpp              # gRPC server bootstrap
├── app/
│   ├── index_main.cpp             # (existing)
│   ├── search_main.cpp            # (existing)
│   └── server_main.cpp            # NEW — gRPC server entry point
├── include/search/
│   ├── server/
│   │   └── search_service.h
│   └── ...                        # (existing headers)
├── tests/
│   ├── ...                        # (existing)
│   └── test_grpc_service.cpp      # NEW
├── CMakeLists.txt                 # updated with gRPC/protobuf
├── Makefile                       # add run-server target
├── Dockerfile                     # NEW
└── docs/
```

### 4. Build System Updates

Add to CMakeLists.txt:
- `find_package(Protobuf REQUIRED)`
- `find_package(gRPC REQUIRED)`
- Proto compilation via `protobuf_generate_cpp` + `grpc_cpp_plugin`
- New executable target: `search-server`
- Optional: MeCab linkage

### 5. Server Lifecycle

```
server_main.cpp
    │
    ├── Load config (port, index path, stopwords)
    ├── Open DiskIndex (mmap)
    ├── Create Searcher instance
    ├── Create InvertedIndex (for live indexing)
    ├── Register SearchServiceImpl with gRPC server
    ├── Register health check service
    ├── Start listening on 0.0.0.0:50051
    └── Graceful shutdown on SIGINT/SIGTERM
```

The server holds both:
- A **DiskIndex** for the bulk pre-built index (read-only, fast)
- An **InvertedIndex** for live-indexed documents via `IndexDocument` RPC (merged at query time)

### 6. Live Indexing Strategy

When the Go app adds/updates content (words, grammar, paragraphs), it calls `IndexDocument`. The server:
1. Tokenizes the content
2. Adds to the in-memory InvertedIndex
3. Periodically flushes to disk (background thread, every N minutes or on shutdown)

Search queries merge results from both DiskIndex and live InvertedIndex.

---

## Data Flow: Japanese Learning App → Search

```
User types "食べる" in the app
    │
    ▼
Go API receives GET /api/v1/words/search?q=食べる
    │
    ▼
Go SearchUseCase calls gRPC client
    │
    ▼
C++ SearchService.Search(query="食べる", top_k=10, filter={WORD, jlpt=3})
    │
    ├── Tokenize query (MeCab or bigram: ["食べ", "べる"] or ["食べる"])
    ├── Lookup postings in DiskIndex + InvertedIndex
    ├── Score with BM25
    ├── Filter by content_type and jlpt_level
    ├── Sort by score, take top_k
    │
    ▼
SearchResponse { hits: [{doc_id, score, title, snippet, ...}] }
    │
    ▼
Go app fetches full documents from MongoDB by doc_id
    │
    ▼
Returns enriched results to user
```

---

## Deployment

```yaml
# docker-compose.yml (search service addition)
services:
  search-grpc:
    build:
      context: .
      dockerfile: Dockerfile
    ports: ["50051:50051"]
    volumes:
      - index_data:/data/index
      - ./data/stopwords_vi.txt:/data/stopwords_vi.txt:ro
    environment:
      SEARCH_PORT: "50051"
      INDEX_PATH: "/data/index/index.bin"
      STOPWORDS_PATH: "/data/stopwords_vi.txt"
    healthcheck:
      test: ["CMD", "grpc_health_probe", "-addr=:50051"]
      interval: 10s
      timeout: 3s
```

The Go app's `docker-compose.yml` references this service:
```yaml
services:
  api:
    depends_on:
      search-grpc:
        condition: service_healthy
    environment:
      SEARCH_GRPC_ADDR: "search-grpc:50051"
```

---

## Implementation Phases

### Phase A: gRPC Foundation
1. Add `proto/search/v1/search.proto`
2. Update CMakeLists.txt with gRPC + Protobuf
3. Implement `SearchServiceImpl::Search` (wraps existing Searcher)
4. Implement `SearchServiceImpl::GetStats`
5. Add `app/server_main.cpp`
6. Add health check service
7. Makefile target: `make run-server`

### Phase B: Live Indexing
1. Implement `IndexDocument` RPC (add to in-memory index)
2. Implement `BulkIndex` RPC (client streaming)
3. Implement `DeleteDocument` RPC
4. Background flush thread (in-memory → disk)
5. Merge logic: query both DiskIndex + live InvertedIndex

### Phase C: Japanese Support
1. Add Japanese tokenizer (MeCab or bigram)
2. Auto-detect language in tokenizer (Japanese vs Vietnamese/English)
3. Add Japanese stopwords list
4. Update index format if needed (store content_type + jlpt metadata)

### Phase D: Production Readiness
1. Dockerfile (multi-stage build)
2. Graceful shutdown handling
3. Metrics / observability (request count, latency histogram)
4. Connection pooling guidance for Go client
5. Load testing

---

## Configuration

Environment variables for the gRPC server:

| Variable | Default | Description |
|----------|---------|-------------|
| `SEARCH_PORT` | `50051` | gRPC listen port |
| `INDEX_PATH` | `data/index/index.bin` | Path to serialized index |
| `STOPWORDS_PATH` | `data/stopwords_vi.txt` | Stopwords file |
| `FLUSH_INTERVAL_SEC` | `300` | How often to flush live index to disk |
| `MAX_RESULTS` | `100` | Hard cap on top_k |

---

## Key Design Decisions

1. **String doc_id in proto, uint32 internally** — The Go app uses MongoDB ObjectIDs (strings). The search engine maps them to internal uint32 doc_ids for compact postings. A bidirectional mapping is maintained.

2. **Filter after scoring** — Content type and JLPT filters are applied post-BM25 scoring rather than pre-filtering postings. This keeps the index structure simple. If performance becomes an issue, partitioned indexes per content_type can be added later.

3. **Separate DiskIndex + live InvertedIndex** — Avoids rebuilding the entire index when new content is added. The bulk of the dictionary (JMdict ~180k entries) lives in DiskIndex; admin-added content goes to the live index until the next full rebuild.

4. **No snippet generation initially** — The search engine returns doc_id + title. The Go app fetches full content from MongoDB. Snippet generation (with highlight) can be added later as an optimization.
