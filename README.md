# search

A high-performance, modular search engine built in modern C++17. Designed to demonstrate core search engineering competencies, including concurrent web crawling, binary disk serialization, memory-mapped I/O, and BM25 ranking. 

## Key Features & Architecture

The system is separated into highly cohesive, decoupled modules:

### 1. Multi-threaded Web Crawler (`libcurl` + `liblexbor`)
- **Concurrency:** Uses a thread pool (`std::thread`, `std::mutex`, `std::atomic`) to isolate I/O-bound network requests from CPU-bound indexing.
- **Connection Pooling:** Each thread maintains its own `libcurl` handle, preventing race conditions and maximizing throughput.
- **Fast Parsing:** Utilizes the `lexbor` engine (a production-grade HTML parser) to cleanly strip HTML tags, CSS, and JS, extracting only raw semantic text and `<title>` metadata.

### 2. Vietnamese-Aware Tokenizer
- Performs Unicode-safe string normalization.
- Tokenizes text using custom delimiter logic.
- Implements a fast `std::unordered_set` based stopword filter designed specifically for Vietnamese (e.g., filtering *bị*, *bởi*, *các* but keeping critical pronouns like *tôi*).

### 3. In-Memory & Binary Disk Index (`mmap`)
- **In-Memory:** Builds a dense Inverted Index mapping Terms -> Postings (DocID, Term Frequency, Positions).
- **Binary Serialization:** Uses pure C-style `fwrite` to pack the index into a strict binary format on disk, significantly outperforming JSON/XML.
- **Memory-Mapped Deserialization:** The `DiskIndex` uses Linux `mmap` to map the binary index file directly into the process's virtual memory space. This allows the OS to page the index in and out of RAM on demand, enabling the engine to search datasets much larger than available physical memory.

### 4. Okapi BM25 Ranking Engine
- Industry-standard ranking algorithm (used by Lucene/Elasticsearch).
- Scores documents based on Term Frequency (TF), Inverse Document Frequency (IDF), and Length Normalization to ensure short documents aren't penalized and long documents don't dominate.

## Build Instructions

### Dependencies
The project requires a modern C++17 compiler and CMake.
* `libcurl` (must be installed via your system package manager)
* `lexbor` (automatically fetched and built from source via CMake `FetchContent`)
* `GoogleTest` (automatically fetched via CMake)

For Arch Linux:
```bash
sudo pacman -S curl cmake make gcc
```

### Building the Project
A wrapper `Makefile` is provided for convenience. It automatically exports `compile_commands.json` for LSP support.

```bash
# Generate build files and compile the project
make build

# Run the unit test suite
make test
```

## Usage

### `indexer`
Crawls a list of seed URLs, extracts their content, builds the inverted index, and serializes it to `index.bin`.
```bash
# Creates or updates data/urls.txt with seed URLs, then runs the indexer
./build/indexer data/urls.txt
```

### `search`
Loads `index.bin` using `mmap` and opens an interactive prompt. Type any query to get BM25-ranked results instantly.
```bash
./build/search

> Search> search engine architecture
> Found 2 results:
> 1. Search engine - Wikipedia
>    Score: 1.842
>    URL: https://en.wikipedia.org/wiki/Search_engine
```
