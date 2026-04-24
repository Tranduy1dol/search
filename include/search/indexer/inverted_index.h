#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/tokenizer.h"

namespace search {

class InvertedIndex {
 public:
  explicit InvertedIndex(const std::string& stopwords_path);
  void AddDocument(uint32_t doc_id, const std::string& text,
                   const std::string& title, const std::string& url);

  const std::vector<Posting>& GetPostings(const std::string& term) const;
  const Document& GetDocument(uint32_t doc_id) const;

  uint32_t TotalDocs() const;
  double AvgDocLength() const;
  uint32_t DocFreq(const std::string& term) const;

  const std::unordered_map<std::string, std::vector<Posting>>& GetIndex()
      const {
    return index_;
  }
  const std::unordered_map<uint32_t, Document>& GetDocTable() const {
    return doc_table_;
  }

 private:
  Tokenizer tokenizer_;
  std::unordered_map<std::string, std::vector<Posting>> index_;
  std::unordered_map<uint32_t, Document> doc_table_;
  uint64_t total_tokens_ = 0;
};

}  // namespace search
