#include "search/indexer/inverted_index.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
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

  for (const auto& token : tokens) {
    doc_terms_[doc_id].insert(token);
  }
}

void InvertedIndex::RemoveDocument(uint32_t doc_id) {
  auto doc_it = doc_table_.find(doc_id);
  if (doc_it == doc_table_.end()) return;

  total_tokens_ -= doc_it->second.length_;
  doc_table_.erase(doc_it);

  auto terms_it = doc_terms_.find(doc_id);
  if (terms_it != doc_terms_.end()) {
    for (const auto& term : terms_it->second) {
      auto idx_it = index_.find(term);
      if (idx_it == index_.end()) continue;

      auto& postings = idx_it->second;
      postings.erase(std::remove_if(postings.begin(), postings.end(),
                                    [doc_id](const Posting& p) {
                                      return p.doc_id_ == doc_id;
                                    }),
                     postings.end());

      if (postings.empty()) {
        index_.erase(idx_it);
      }
    }

    doc_terms_.erase(terms_it);
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

const Document& InvertedIndex::GetDocument(uint32_t doc_id) const {
  auto it = doc_table_.find(doc_id);
  if (it == doc_table_.end()) {
    throw std::runtime_error("document not found");
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
