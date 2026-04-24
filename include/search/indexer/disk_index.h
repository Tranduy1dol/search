#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/indexer/inverted_index.h"

namespace search {

class DiskIndex {
 public:
  static constexpr uint32_t kMagicNumber = 0x434F4343;
  static constexpr uint32_t kVersion = 1;

  static void Serialize(const InvertedIndex& index, const std::string& path);
  explicit DiskIndex(const std::string& path);
  ~DiskIndex();

  DiskIndex(const DiskIndex&) = delete;
  DiskIndex& operator=(const DiskIndex&) = delete;

  std::vector<Posting> GetPostings(const std::string& term) const;
  Document GetDocument(uint32_t doc_id) const;
  uint32_t TotalDocs() const;
  double AvgDocLength() const;

 private:
  int fd_ = -1;
  void* mapped_data_ = nullptr;
  size_t file_size_ = 0;

  uint32_t total_docs_ = 0;
  double avg_doc_length_ = 0.0;

  std::unordered_map<std::string, uint64_t> term_offsets_;
  std::unordered_map<uint32_t, uint64_t> doc_offsets_;
};

}  // namespace search
