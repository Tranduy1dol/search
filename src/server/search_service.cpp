#include "server/search_service.h"

#include <grpcpp/support/status.h>

#include <chrono>
#include <cstdint>
#include <ratio>
#include <string>

#include "search/indexer/live_index.h"
#include "search/searcher/live_searcher.h"

namespace search {

SearchServerImpl::SearchServerImpl(LiveSearcher* searcher, LiveIndex* index)
    : searcher_(searcher), index_(index) {}

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
  response->set_total_documents(index_->TotalDocs());
  response->set_avg_document_length(index_->AvgDocLength());

  return grpc::Status::OK;
}

grpc::Status SearchServerImpl::IndexDocument(
    grpc::ServerContext* context,
    const grpc_service::v1::IndexDocumentRequest* request,
    grpc_service::v1::IndexDocumentResponse* response) {
  index_->IndexDocument(request->doc_id(), request->text(), request->title(),
                        request->url());
  response->set_success(true);
  return grpc::Status::OK;
}

grpc::Status SearchServerImpl::BulkIndex(
    grpc::ServerContext* context,
    grpc::ServerReader<grpc_service::v1::IndexDocumentRequest>* reader,
    grpc_service::v1::BulkIndexResponse* response) {
  grpc_service::v1::IndexDocumentRequest request;
  uint32_t indexed = 0;
  uint32_t failed = 0;

  while (reader->Read(&request)) {
    try {
      index_->IndexDocument(request.doc_id(), request.text(), request.title(),

                            request.url());
      indexed++;
    } catch (...) {
      failed++;
    }
  }

  response->set_failed_count(failed);
  response->set_indexed_count(indexed);

  return grpc::Status::OK;
}

grpc::Status SearchServerImpl::DeleteDocument(
    grpc::ServerContext* context,
    const grpc_service::v1::DeleteDocumentRequest* request,
    grpc_service::v1::DeleteDocumentResponse* response) {
  if (request->doc_id().empty()) {
    response->set_success(false);
    response->set_message("doc_id cannot be empty");
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "doc_id cannot be empty");
  }

  index_->RemoveDocument(request->doc_id());
  response->set_success(true);

  return grpc::Status::OK;
}

}  // namespace search
