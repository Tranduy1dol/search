# Deployment Guide — Azure Container Apps

## Context

The C++ search engine serves as a gRPC backend for a Go-based Japanese learning app. Both services are deployed on **Azure Container Apps (ACA)** using $100 Azure student credits.

### Why ACA over alternatives

| Option | Monthly Cost | Verdict |
|--------|-------------|---------|
| **Container Apps** | ~$7–13 (scale to zero) | ✅ Best fit |
| AKS (Kubernetes) | ~$70+ (control plane alone) | ❌ Overkill, burns credits in 6 weeks |
| Single VM (B2s) | ~$15 (always running) | ⚠️ Works but no auto-scaling |
| Container Instances | ~$20+ (no scale to zero) | ❌ No service mesh |

ACA provides scale-to-zero, native gRPC/HTTP2 support, internal service discovery, and built-in TLS — all critical for this use case.

---

## Infrastructure Overview

```
Internet (HTTPS)
    │
    ▼
┌──────────────────────────────────────────┐
│         Azure Container Apps Env         │
│         (shared virtual network)         │
│                                          │
│  ┌────────────────────────┐              │
│  │  go-backend            │              │
│  │  Ingress: external     │              │
│  │  Port: 8080            │              │
│  │  REST API + MongoDB    │              │
│  └──────────┬─────────────┘              │
│             │ gRPC (internal DNS)        │
│             │ search-grpc:50051          │
│             ▼                            │
│  ┌────────────────────────┐              │
│  │  search-grpc           │              │
│  │  Ingress: internal     │  ┌────────┐  │
│  │  Port: 50051 (HTTP/2)  │──│ Azure  │  │
│  │  C++ search_server     │  │ Files  │  │
│  └────────────────────────┘  │ Volume │  │
│                              │(index) │  │
│                              └────────┘  │
└──────────────────────────────────────────┘
```

**Key points:**
- Go backend has **external** ingress (public-facing REST API)
- Search service has **internal** ingress only (no public exposure)
- Both share the same environment → Go reaches search via `search-grpc:50051`
- Index file persists on Azure Files volume across container restarts

---

## Azure Resources

| Resource | Name | Purpose |
|----------|------|---------|
| Resource Group | `jlearn-rg` | Groups all resources |
| Container Registry | `jlearnacr` | Stores Docker images |
| Container Apps Environment | `jlearn-env` | Shared network for containers |
| Container App | `search-grpc` | C++ gRPC search service |
| Container App | `go-backend` | Go REST API |
| Storage Account | `jlearnstorage` | Backs Azure Files |
| File Share | `search-index` | Persistent storage for `index.bin` |

---

## Initial Setup (One-time)

### 1. Create resource group and registry

```bash
az group create -n jlearn-rg -l southeastasia
az acr create -n jlearnacr -g jlearn-rg --sku Basic --admin-enabled true
```

### 2. Create the Container Apps environment

```bash
az containerapp env create \
  --name jlearn-env \
  --resource-group jlearn-rg \
  --location southeastasia
```

### 3. Create persistent storage for the index

```bash
# Storage account
az storage account create \
  -n jlearnstorage \
  -g jlearn-rg \
  --sku Standard_LRS \
  --location southeastasia

# File share
az storage share create \
  --name search-index \
  --account-name jlearnstorage

# Get the storage key
STORAGE_KEY=$(az storage account keys list \
  -n jlearnstorage -g jlearn-rg \
  --query '[0].value' -o tsv)

# Mount into the Container Apps environment
az containerapp env storage set \
  --name jlearn-env \
  --resource-group jlearn-rg \
  --storage-name indexvol \
  --azure-file-account-name jlearnstorage \
  --azure-file-share-name search-index \
  --azure-file-account-key "$STORAGE_KEY" \
  --access-mode ReadWrite
```

### 4. Build and push Docker images

```bash
az acr build -r jlearnacr -t search-grpc:latest ./search/
az acr build -r jlearnacr -t go-backend:latest ./go-app/
```

### 5. Deploy the search service (internal only)

```bash
az containerapp create \
  --name search-grpc \
  --resource-group jlearn-rg \
  --environment jlearn-env \
  --image jlearnacr.azurecr.io/search-grpc:latest \
  --registry-server jlearnacr.azurecr.io \
  --target-port 50051 \
  --transport http2 \
  --ingress internal \
  --cpu 0.5 --memory 1Gi \
  --min-replicas 1 \
  --max-replicas 2 \
  --env-vars \
    INDEX_PATH=/data/index/index.bin \
    STOPWORD_PATH=/data/stopwords_vi.txt \
    SEARCH_PORT=50051
```

> **Note:** `min-replicas 1` (not 0) because the search service must be warm
> to receive `BulkIndex` calls from the Go backend on startup.

### 6. Deploy the Go backend (public)

```bash
az containerapp create \
  --name go-backend \
  --resource-group jlearn-rg \
  --environment jlearn-env \
  --image jlearnacr.azurecr.io/go-backend:latest \
  --registry-server jlearnacr.azurecr.io \
  --target-port 8080 \
  --ingress external \
  --cpu 0.5 --memory 1Gi \
  --min-replicas 0 \
  --max-replicas 3 \
  --env-vars \
    SEARCH_GRPC_ADDR=search-grpc:50051
```

### 7. Grant ACR pull permissions

```bash
az containerapp registry set \
  --name search-grpc \
  --resource-group jlearn-rg \
  --server jlearnacr.azurecr.io \
  --identity system
```

---

## CI/CD — GitHub Actions Auto-Deploy

### Setup: Create Azure service principal

```bash
az ad sp create-for-rbac \
  --name "github-deploy-jlearn" \
  --role contributor \
  --scopes /subscriptions/<SUBSCRIPTION_ID>/resourceGroups/jlearn-rg \
  --json-auth
```

Copy the entire JSON output → GitHub repo → Settings → Secrets → `AZURE_CREDENTIALS`

### Workflow: `.github/workflows/ci.yml`

Add a `deploy` job after the existing `build-and-test` and `docker` jobs:

```yaml
  deploy:
    needs: [build-and-test, docker]
    runs-on: ubuntu-24.04
    if: github.ref == 'refs/heads/main' && github.event_name == 'push'

    steps:
      - uses: actions/checkout@v4

      - name: Login to Azure
        uses: azure/login@v2
        with:
          creds: ${{ secrets.AZURE_CREDENTIALS }}

      - name: Login to ACR
        run: az acr login --name jlearnacr

      - name: Build and push image
        run: |
          az acr build \
            --registry jlearnacr \
            --image search-grpc:${{ github.sha }} \
            --image search-grpc:latest \
            .

      - name: Deploy to Container Apps
        uses: azure/container-apps-deploy-action@v1
        with:
          resourceGroup: jlearn-rg
          containerAppName: search-grpc
          imageToDeploy: jlearnacr.azurecr.io/search-grpc:${{ github.sha }}
```

### Deployment flow

```
git push to main
  → CI: build + test + lint (must pass)
  → CI: docker build (must pass)
  → Deploy:
      → az acr build → push search-grpc:{git-sha}
      → az containerapp update → new revision goes live
      → Container starts → LoadFromDisk (recover index)
      → Go backend detects restart → BulkIndex sync
```

Images are tagged with the git SHA for easy rollback:
```bash
# Roll back to a specific commit
az containerapp update \
  --name search-grpc -g jlearn-rg \
  --image jlearnacr.azurecr.io/search-grpc:<previous-sha>
```

---

## Live Indexing — How the Index Gets Built

The index is **not** baked into the Docker image. Instead, it is built at runtime
from data managed by the Go backend.

### Data flow

```
Go backend starts
  → Loads all words/grammar from MongoDB
  → Calls search-grpc BulkIndex(stream of documents)
  → Search service tokenizes + indexes in memory
  → Immediately searchable

User adds a new word via the app
  → Go backend saves to MongoDB
  → Go backend calls search-grpc IndexDocument(word)
  → Search service adds to live in-memory index
  → Background thread flushes to Azure Files every 5 min
```

### Search service lifecycle

```
Container starts
  │
  ├── Check INDEX_PATH for existing index.bin
  │   ├── Found → LoadFromDisk() → recover last flushed state
  │   └── Not found → start with empty index (Go will sync)
  │
  ├── Start gRPC server on :50051
  ├── Start background flush thread (every 5 min → serialize to disk)
  │
  │   ... serving requests ...
  │
  ├── SIGTERM received
  ├── Stop accepting new RPCs
  ├── Final flush to disk
  └── Exit
```

### Required proto additions (Phase B)

```protobuf
service SearchService {
  rpc Search(SearchRequest) returns (SearchResponse);                      // existing
  rpc GetStats(GetStatsRequest) returns (GetStatsResponse);                // existing
  rpc IndexDocument(IndexDocumentRequest) returns (IndexDocumentResponse);  // new
  rpc BulkIndex(stream IndexDocumentRequest) returns (BulkIndexResponse);   // new
}

message IndexDocumentRequest {
  string doc_id = 1;       // MongoDB ObjectID as string
  string title = 2;
  string content = 3;
  ContentType content_type = 4;
  int32 level = 5;
}

message IndexDocumentResponse {
  bool success = 1;
}

message BulkIndexResponse {
  uint32 indexed_count = 1;
  uint32 failed_count = 2;
}
```

### Required C++ changes (Phase B)

`SearchServiceImpl` needs to be redesigned to hold a live index:

```cpp
class SearchServiceImpl final : public grpc_service::v1::SearchService::Service {
 public:
  SearchServiceImpl(const std::string& stopwords_path,
                    const std::string& index_path);
  ~SearchServiceImpl();

  grpc::Status Search(...) override;
  grpc::Status GetStats(...) override;
  grpc::Status IndexDocument(...) override;    // new
  grpc::Status BulkIndex(...) override;        // new (client streaming)

 private:
  void FlushLoop();       // background thread: serialize to disk every 5 min
  void LoadFromDisk();    // startup: recover from last flush

  Tokenizer tokenizer_;
  InvertedIndex live_index_;                   // single source of truth
  std::string index_path_;

  mutable std::shared_mutex index_mutex_;      // readers-writer lock
  std::thread flush_thread_;
  std::atomic<bool> running_{true};

  // MongoDB string ID ↔ internal uint32_t mapping
  std::unordered_map<std::string, uint32_t> id_map_;
  uint32_t next_doc_id_ = 1;
};
```

**Concurrency model:**
- `IndexDocument` / `BulkIndex` → acquire **exclusive** lock on `index_mutex_`
- `Search` / `GetStats` → acquire **shared** (read) lock on `index_mutex_`
- `FlushLoop` → acquire **shared** lock, serialize to disk

This allows multiple concurrent searches while blocking only during writes.

### Go client example

```go
// On startup — full sync
func (s *SearchService) SyncAllWords(ctx context.Context) error {
    words, _ := s.wordRepo.FindAll(ctx)

    stream, _ := s.grpcClient.BulkIndex(ctx)
    for _, w := range words {
        stream.Send(&pb.IndexDocumentRequest{
            DocId:       w.ID.Hex(),
            Title:       w.Reading,
            Content:     w.Meaning + " " + w.Reading + " " + w.Kanji,
            ContentType: pb.ContentType_CONTENT_TYPE_WORD,
            Level:       int32(w.JLPTLevel),
        })
    }
    resp, _ := stream.CloseAndRecv()
    log.Printf("synced %d words to search", resp.IndexedCount)
    return nil
}

// On single word create/update
func (s *SearchService) IndexWord(ctx context.Context, w *Word) error {
    _, err := s.grpcClient.IndexDocument(ctx, &pb.IndexDocumentRequest{
        DocId:       w.ID.Hex(),
        Title:       w.Reading,
        Content:     w.Meaning + " " + w.Reading + " " + w.Kanji,
        ContentType: pb.ContentType_CONTENT_TYPE_WORD,
        Level:       int32(w.JLPTLevel),
    })
    return err
}
```

---

## Cost Estimate

| Resource | Monthly Cost |
|----------|-------------|
| Container Registry (Basic) | ~$5 |
| Container Apps (search-grpc, min 1 replica) | ~$5–10 |
| Container Apps (go-backend, scale to zero) | ~$0–5 |
| Azure Files (< 1 GB) | ~$0.05 |
| **Total** | **~$10–20/month** |

With $100 credits: **5–10 months** of runway.

---

## Checklist

### One-time infra setup
- [ ] Create resource group (`jlearn-rg`)
- [ ] Create container registry (`jlearnacr`)
- [ ] Create container apps environment (`jlearn-env`)
- [ ] Create storage account + file share for index persistence
- [ ] Mount file share into environment
- [ ] Create Azure service principal for GitHub Actions
- [ ] Add `AZURE_CREDENTIALS` secret to GitHub repo

### Code changes (Phase B)
- [ ] Add `IndexDocument` and `BulkIndex` RPCs to proto
- [ ] Regenerate protobuf (`buf generate`)
- [ ] Redesign `SearchServiceImpl` with live `InvertedIndex`
- [ ] Add `std::shared_mutex` for concurrent read/write safety
- [ ] Implement background flush thread
- [ ] Implement `LoadFromDisk` startup recovery
- [ ] Add graceful shutdown (final flush on SIGTERM)
- [ ] Add string ↔ uint32 doc_id bidirectional mapping
- [ ] Update `server_main.cpp` with new constructor
- [ ] Write Go client `SyncAllWords` and `IndexWord` methods
- [ ] Update CI workflow with deploy job

### Testing
- [ ] Unit test: IndexDocument adds to live index, immediately searchable
- [ ] Unit test: BulkIndex streams N docs, all searchable
- [ ] Unit test: FlushLoop serializes, LoadFromDisk recovers
- [ ] Integration test: Go client → BulkIndex → Search → correct results
