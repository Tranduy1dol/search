#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Document {
  uint32_t doc_id;
  std::string url;
  std::string title;
  uint32_t length;
};

struct Posting {
  uint32_t doc_id;
  uint32_t term_freq;
  std::vector<uint32_t> positions;
};

struct SearchResult {
  uint32_t doc_id;
  double score;
  std::string title;
  std::string snippet;
};
