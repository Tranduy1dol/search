#include "search/indexer/inverted_index.h"

#include <string>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/tokenizer.h"

namespace search {

InvertedIndex::InvertedIndex(const std::string& stopwords_path)
    : tokenizer_(stopwords_path) {}

void InvertedIndex::AddDocument(uint32_t doc_id, const std::string& text,
                                const std::string& title,
                                const std::string& url) {
  std::vector<std::string> tokens = tokenizer_.Tokenize(text);

  doc_table_[doc_id] = {doc_id, url, title,
                        static_cast<uint32_t>(tokens.size())};
  total_tokens_ += tokens.size();

  for (int i = 0; i < static_cast<int>(tokens.size()); i++) {
    std::string term = tokens[i];
    std::vector<Posting>& posts = index_[term];
    bool exist = false;

    for (Posting& posting : posts) {
      if (posting.doc_id_ == doc_id) {
        exist = true;
        posting.term_freq_++;
        posting.positions_.push_back(i);
      }
    }

    if (exist == false) {
      Posting posting{doc_id, 1, {static_cast<uint32_t>(i)}};
      posts.push_back(posting);
    }
  }
}

const std::vector<Posting>& InvertedIndex::GetPostings(
    const std::string& term) const {
  static const std::vector<Posting> empty{};
  auto it = index_.find(term);
  if (it == index_.end()) {
    return empty;
  }
  return it->second;
}

uint32_t InvertedIndex::TotalDocs() const { return doc_table_.size(); }
double InvertedIndex::AvgDocLength() const {
  return total_tokens_ / TotalDocs();
}

uint32_t InvertedIndex::DocFreq(const std::string& term) const {
  auto it = index_.find(term);
  if (it == index_.end()) {
    return 0;
  }
  return static_cast<uint32_t>(it->second.size());
}

}  // namespace search
