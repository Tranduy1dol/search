#include "search/searcher/live_searcher.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/tokenizer.h"

namespace search {

LiveSearcher::LiveSearcher(LiveIndex* index, const std::string& stopwords_path)
    : index_(index), tokenizer_(stopwords_path) {}

double LiveSearcher::CalculateBM25(uint32_t term_freq, uint32_t doc_freq,
                                   uint32_t doc_length) const {
  double big_n = static_cast<double>(index_->TotalDocs());
  double n = static_cast<double>(doc_freq);
  double avgdl = index_->AvgDocLength();

  double idf = std::log(1.0 + (big_n - n + 0.5) / (n + 0.5));

  double tf_num = term_freq * (k1 + 1.0);
  double tf_den = term_freq + k1 * (1.0 - kB + kB * (doc_length / avgdl));

  return idf * (tf_num / tf_den);
}

std::vector<SearchResult> LiveSearcher::Search(const std::string& query,
                                               size_t top_k) const {
  if (index_->TotalDocs() == 0) {
    return {};
  }

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
    results.push_back({doc_id, score, doc.title_, doc.url_});
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
