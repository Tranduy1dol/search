#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "search/indexer/disk_index.h"
#include "search/indexer/tokenizer.h"

namespace search {

class Searcher {
 public:
  Searcher(std::unique_ptr<DiskIndex> index, const std::string& stopwords_path);
  std::vector<SearchResult> Search(const std::string& query,
                                   size_t top_k = 10) const;

 private:
  double CalculateBM25(uint32_t term_freq, uint32_t doc_freq,
                       uint32_t doc_length) const;

  std::unique_ptr<DiskIndex> index_;
  Tokenizer tokenizer_;

  const double k1_ = 1.5;
  const double b_ = 0.75;
};
}  // namespace search
