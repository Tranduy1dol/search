#include "server/search_service.h"

#include <grpcpp/support/status.h>

#include <chrono>
#include <cstdint>
#include <ratio>
#include <string>

#include "search/indexer/disk_index.h"

namespace search {

SearchServerImpl::SearchServerImpl(Searcher* searcher, DiskIndex* index)
    : searcher_(searcher), disk_index_(index) {}

grpc::Status SearchServerImpl::Search(
    grpc::ServerContext* context,
    const grpc_service::v1::SearchRequest* request,
    grpc_service::v1::SearchResponse* response) {
  uint32_t top_k = request->top_k() > 0 ? request->top_k() : 10;

  auto start = std::chrono::steady_clock::now();
  auto results = searcher_->Search(request->query(), top_k);
  auto end = std::chrono::steady_clock::now();

  double elapsed_ms =
      std::chrono::duration<double, std::milli>(end - start).count();
  response->set_query_time_ms(elapsed_ms);
  response->set_total_hits(results.size());

  for (const auto& r : results) {
    auto* hit = response->add_hits();
    hit->set_doc_id(std::to_string(r.doc_id_));
    hit->set_score(r.score_);
    hit->set_title(r.title_);
    hit->set_snippet(r.snippet_);
  }

  return grpc::Status::OK;
}

grpc::Status SearchServerImpl::GetStats(
    grpc::ServerContext* context,
    const grpc_service::v1::GetStatsRequest* request,
    grpc_service::v1::GetStatsResponse* response) {
  response->set_total_documents(disk_index_->TotalDocs());
  response->set_avg_document_length(disk_index_->AvgDocLength());

  return grpc::Status::OK;
}

}  // namespace search
