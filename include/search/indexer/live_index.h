#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/inverted_index.h"

namespace search {

class LiveIndex {
 public:
  LiveIndex(const std::string& index_path, const std::string& stopwords_path,
            uint32_t flush_interval_seconds = 60);
  ~LiveIndex();

  LiveIndex(const LiveIndex&) = delete;
  LiveIndex& operator=(const LiveIndex&) = delete;

  void IndexDocument(const std::string& external_id, const std::string& text,
                     const std::string& title, const std::string& url);
  void RemoveDocument(const std::string& external_id);

  std::vector<Posting> GetPostings(const std::string& term) const;
  Document GetDocument(uint32_t doc_id) const;
  uint32_t TotalDocs() const;
  double AvgDocLength() const;
  uint32_t DocFreq(const std::string& term) const;

  uint32_t GetInternalId(const std::string& external_id) const;
  std::string GetExternalId(uint32_t internal_id) const;

  void Flush();

 private:
  void LoadFromDisk();
  void FlushLoop();
  void SaveIdMapping() const;
  void LoadIdMapping();

  std::string index_path_;
  std::string stopwords_path_;
  uint32_t flush_interval_seconds_;

  mutable std::shared_mutex mutex_;
  std::unique_ptr<InvertedIndex> index_;

  std::unordered_map<std::string, uint32_t> external_to_internal_;
  std::unordered_map<uint32_t, std::string> internal_to_external_;
  uint32_t next_internal_id_ = 1;

  std::thread flush_thread_;
  std::atomic_bool running_{true};
};

}  // namespace search
