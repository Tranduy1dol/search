#pragma once

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

#include "grpc_service/v1/search.grpc.pb.h"
#include "search/indexer/disk_index.h"
#include "search/searcher/searcher.h"

namespace search {
class SearchServerImpl final : public grpc_service::v1::SearchService::Service {
 public:
  explicit SearchServerImpl(Searcher* searcher, DiskIndex* index);

  grpc::Status Search(grpc::ServerContext* context,
                      const grpc_service::v1::SearchRequest* request,
                      grpc_service::v1::SearchResponse* response) override;

  grpc::Status GetStats(grpc::ServerContext* context,
                        const grpc_service::v1::GetStatsRequest* request,
                        grpc_service::v1::GetStatsResponse* response) override;

 private:
  Searcher* searcher_;
  DiskIndex* disk_index_;
};
}  // namespace search
