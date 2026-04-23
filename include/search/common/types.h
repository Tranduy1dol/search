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
