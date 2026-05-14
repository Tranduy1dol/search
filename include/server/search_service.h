#pragma once

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/sync_stream.h>

#include "grpc_service/v1/search.grpc.pb.h"
#include "grpc_service/v1/search.pb.h"
#include "search/indexer/live_index.h"
#include "search/searcher/live_searcher.h"

namespace search {
class SearchServerImpl final : public grpc_service::v1::SearchService::Service {
 public:
  explicit SearchServerImpl(LiveSearcher* searcher, LiveIndex* index);

  grpc::Status Search(grpc::ServerContext* context,
                      const grpc_service::v1::SearchRequest* request,
                      grpc_service::v1::SearchResponse* response) override;

  grpc::Status GetStats(grpc::ServerContext* context,
                        const grpc_service::v1::GetStatsRequest* request,
                        grpc_service::v1::GetStatsResponse* response) override;

  grpc::Status IndexDocument(
      grpc::ServerContext* context,
      const grpc_service::v1::IndexDocumentRequest* request,
      grpc_service::v1::IndexDocumentResponse* response) override;

  grpc::Status BulkIndex(
      grpc::ServerContext* context,
      grpc::ServerReader<grpc_service::v1::IndexDocumentRequest>* reader,
      grpc_service::v1::BulkIndexResponse* response) override;

 private:
  LiveSearcher* searcher_;
  LiveIndex* index_;
};
}  // namespace search
