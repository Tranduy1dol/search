#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Document {
  uint32_t doc_id_;
  std::string url_;
  std::string title_;
  uint32_t length_;
};

struct Posting {
  uint32_t doc_id_;
  uint32_t term_freq_;
  std::vector<uint32_t> positions_;
};

struct SearchResult {
  uint32_t doc_id_;
  double score_;
  std::string title_;
  std::string snippet_;
};

struct IndexHeader {
  uint32_t magic_;
  uint32_t version_;
  uint32_t num_docs_;
  uint32_t num_terms_;
  double avg_doc_length_;
  uint64_t doc_table_offset_;
  uint64_t dict_offset_;
};
