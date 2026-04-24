#include "search/searcher/searcher.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/disk_index.h"

namespace search {

Searcher::Searcher(std::unique_ptr<DiskIndex> index,
                   const std::string& stopwords_path)
    : index_(std::move(index)), tokenizer_(stopwords_path) {}

double Searcher::CalculateBM25(uint32_t term_freq, uint32_t doc_freq,
                               uint32_t doc_length) const {
  double big_n = static_cast<double>(index_->TotalDocs());
  double n = static_cast<double>(doc_freq);
  double avgdl = index_->AvgDocLength();

  double idf = std::log(1.0 + (big_n - n + 0.5) / (n + 0.5));

  double tf_numerator = term_freq * (k1_ + 1.0);
  double tf_denominaotr =
      term_freq + k1_ * (1.0 - b_ + b_ * (doc_length / avgdl));

  return idf * (tf_numerator / tf_denominaotr);
}

std::vector<SearchResult> Searcher::Search(const std::string& query,
                                           size_t top_k) const {
  std::vector<std::string> query_tokens = tokenizer_.Tokenize(query);
  std::unordered_map<uint32_t, double> doc_scores;

  for (const std::string& term : query_tokens) {
    std::vector<Posting> postings = index_->GetPostings(term);
    uint32_t doc_freq = postings.size();

    for (const Posting& p : postings) {
      Document doc = index_->GetDocument(p.doc_id_);
      double score = CalculateBM25(p.term_freq_, doc_freq, doc.length_);
      doc_scores[p.doc_id_] += score;
    }
  }

  std::vector<SearchResult> results;
  results.reserve(doc_scores.size());

  for (const auto& [doc_id, score] : doc_scores) {
    Document doc = index_->GetDocument(doc_id);

    SearchResult res;
    res.doc_id_ = doc_id;
    res.score_ = score;
    res.title_ = doc.title_;
    res.snippet_ = doc.url_;

    results.push_back(res);
  }

  std::sort(results.begin(), results.end(),
            [](const SearchResult& a, const SearchResult& b) {
              return a.score_ > b.score_;
            });

  if (results.size() > top_k) {
    results.resize(top_k);
  }

  return results;
}

}  // namespace search
