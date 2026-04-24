#include "search/indexer/disk_index.h"

#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "search/common/types.h"
#include "search/common/utils.h"

namespace search {

void DiskIndex::Serialize(const InvertedIndex& index, const std::string& path) {
  FILE* fp = fopen(path.c_str(), "wb");
  if (!fp) {
    throw std::runtime_error("failed to open file for writing");
  }

  const auto& in_memory_index = index.GetIndex();
  const auto& doc_table = index.GetDocTable();

  IndexHeader header;
  header.magic_ = kMagicNumber;
  header.version_ = kVersion;
  header.num_docs_ = index.TotalDocs();
  header.num_terms_ = in_memory_index.size();
  header.avg_doc_length_ = index.AvgDocLength();
  header.doc_table_offset_ = 0;
  header.dict_offset_ = 0;
  fwrite(&header, sizeof(IndexHeader), 1, fp);

  std::unordered_map<std::string, uint64_t> term_offsets;
  for (const auto& [term, posting] : in_memory_index) {
    term_offsets[term] = ftell(fp);
    uint32_t num_postings = posting.size();
    fwrite(&num_postings, sizeof(uint32_t), 1, fp);

    for (const auto& p : posting) {
      fwrite(&p.doc_id_, sizeof(uint32_t), 1, fp);
      fwrite(&p.term_freq_, sizeof(uint32_t), 1, fp);

      uint32_t num_pos = p.positions_.size();
      fwrite(&num_pos, sizeof(uint32_t), 1, fp);
      fwrite(p.positions_.data(), sizeof(uint32_t), num_pos, fp);
    }
  }

  header.doc_table_offset_ = ftell(fp);
  for (const auto& [doc_id, doc] : doc_table) {
    fwrite(&doc_id, sizeof(uint32_t), 1, fp);
    fwrite(&doc.length_, sizeof(uint32_t), 1, fp);
    utils::WriteString(fp, doc.url_);
    utils::WriteString(fp, doc.title_);
  }

  header.dict_offset_ = ftell(fp);
  for (const auto& [term, postings] : in_memory_index) {
    utils::WriteString(fp, term);

    uint32_t doc_freq = postings.size();
    fwrite(&doc_freq, sizeof(uint32_t), 1, fp);

    uint64_t offset = term_offsets[term];
    fwrite(&offset, sizeof(uint64_t), 1, fp);
  }

  fseek(fp, 0, SEEK_SET);
  fwrite(&header, sizeof(IndexHeader), 1, fp);

  fclose(fp);
}

DiskIndex::DiskIndex(const std::string& path) {
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ < 0) {
    throw std::runtime_error("failed to open index file");
  }

  struct stat sb;
  if (fstat(fd_, &sb) < 0) {
    throw std::runtime_error("failed to get file size");
  }
  file_size_ = sb.st_size;

  mapped_data_ = mmap(nullptr, file_size_, PROT_READ, MAP_SHARED, fd_, 0);
  if (mapped_data_ == MAP_FAILED) {
    throw std::runtime_error("mmap failed");
  }

  const char* base = static_cast<const char*>(mapped_data_);
  IndexHeader header;
  std::memcpy(&header, base, sizeof(IndexHeader));
  if (header.magic_ != kMagicNumber) {
    throw std::runtime_error("invalid index magic number");
  }

  if (header.version_ != kVersion) {
    throw std::runtime_error("unsupported index version_");
  }

  total_docs_ = header.num_docs_;
  avg_doc_length_ = header.avg_doc_length_;
  const char* dict_ptr = base + header.dict_offset_;
  for (uint32_t i = 0; i < header.num_terms_; i++) {
    std::string term = utils::ReadString(dict_ptr);

    uint32_t doc_freq;
    std::memcpy(&doc_freq, dict_ptr, sizeof(uint32_t));
    dict_ptr += sizeof(uint32_t);

    uint64_t offset;
    std::memcpy(&offset, dict_ptr, sizeof(uint64_t));
    dict_ptr += sizeof(uint64_t);

    term_offsets_[term] = offset;
  }

  const char* doc_ptr = base + header.doc_table_offset_;
  for (uint32_t i = 0; i < header.num_docs_; i++) {
    uint32_t doc_id;
    std::memcpy(&doc_id, doc_ptr, sizeof(uint32_t));
    doc_offsets_[doc_id] = doc_ptr - base;

    doc_ptr += sizeof(uint32_t);
    doc_ptr += sizeof(uint32_t);
    utils::ReadString(doc_ptr);
    utils::ReadString(doc_ptr);
  }
}

DiskIndex::~DiskIndex() {
  if (mapped_data_ != MAP_FAILED && mapped_data_ != nullptr) {
    munmap(mapped_data_, file_size_);
  }

  if (fd_ >= 0) {
    close(fd_);
  }
}

uint32_t DiskIndex::TotalDocs() const { return total_docs_; }
double DiskIndex::AvgDocLength() const { return avg_doc_length_; }

std::vector<Posting> DiskIndex::GetPostings(const std::string& term) const {
  auto it = term_offsets_.find(term);
  if (it == term_offsets_.end()) {
    return {};
  }

  const char* ptr = static_cast<const char*>(mapped_data_) + it->second;

  uint32_t num_postings;
  std::memcpy(&num_postings, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);

  std::vector<Posting> postings;
  postings.reserve(num_postings);
  for (uint32_t i = 0; i < num_postings; i++) {
    Posting p;

    std::memcpy(&p.doc_id_, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    std::memcpy(&p.term_freq_, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    uint32_t num_pos;
    std::memcpy(&num_pos, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    p.positions_.resize(num_pos);
    std::memcpy(p.positions_.data(), ptr, num_pos * sizeof(uint32_t));
    ptr += num_pos * sizeof(uint32_t);

    postings.push_back(p);
  }

  return postings;
}

Document DiskIndex::GetDocument(uint32_t doc_id) const {
  auto it = doc_offsets_.find(doc_id);
  if (it == doc_offsets_.end()) {
    throw std::runtime_error("document not found");
  }

  const char* ptr = static_cast<const char*>(mapped_data_) + it->second;

  Document doc;
  std::memcpy(&doc.doc_id_, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);

  std::memcpy(&doc.length_, ptr, sizeof(uint32_t));
  ptr += sizeof(uint32_t);

  doc.url_ = utils::ReadString(ptr);
  doc.title_ = utils::ReadString(ptr);

  return doc;
}

}  // namespace search
