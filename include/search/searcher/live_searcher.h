#pragma once

#include <string>
#include <vector>

#include "search/indexer/live_index.h"
#include "search/indexer/tokenizer.h"

namespace search {

class LiveSearcher {
 public:
  LiveSearcher(LiveIndex* index, const std::string& stopwords_path);
  std::vector<SearchResult> Search(const std::string& query,
                                   size_t top_k = 10) const;

 private:
  double CalculateBM25(uint32_t term_freq, uint32_t doc_freq,
                       uint32_t doc_length) const;

  LiveIndex* index_;
  Tokenizer tokenizer_;

  static constexpr double k1 = 1.5;
  static constexpr double kB = 0.75;
};

}  // namespace search
