#include "search/indexer/live_index.h"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

#include "search/indexer/disk_index.h"

namespace search {

LiveIndex::LiveIndex(const std::string& index_path,
                     const std::string& stopwords_path,
                     uint32_t flush_interval_seconds)
    : index_path_(index_path),
      stopwords_path_(stopwords_path),
      flush_interval_seconds_(flush_interval_seconds),
      index_(std::make_unique<InvertedIndex>(stopwords_path)) {
  LoadFromDisk();
  flush_thread_ = std::thread(&LiveIndex::FlushLoop, this);
}

LiveIndex::~LiveIndex() {
  running_ = false;
  if (flush_thread_.joinable()) {
    flush_thread_.join();
  }

  Flush();
}

void LiveIndex::IndexDocument(const std::string& external_id,
                              const std::string& text, const std::string& title,
                              const std::string& url) {
  std::unique_lock lock(mutex_);

  auto it = external_to_internal_.find(external_id);
  if (it != external_to_internal_.end()) {
    index_->RemoveDocument(it->second);
  }

  uint32_t internal_id;
  if (it != external_to_internal_.end()) {
    internal_id = it->second;
  } else {
    internal_id = next_internal_id_++;
    external_to_internal_[external_id] = internal_id;
    internal_to_external_[internal_id] = external_id;
  }

  index_->AddDocument(internal_id, text, title, url);
}

void LiveIndex::RemoveDocument(const std::string& external_id) {
  std::unique_lock lock(mutex_);

  auto it = external_to_internal_.find(external_id);
  if (it == external_to_internal_.end()) return;

  uint32_t internal_id = it->second;
  index_->RemoveDocument(internal_id);

  internal_to_external_.erase(internal_id);
  external_to_internal_.erase(it);
}

std::vector<Posting> LiveIndex::GetPostings(const std::string& term) const {
  std::shared_lock lock(mutex_);
  return index_->GetPostings(term);
}

Document LiveIndex::GetDocument(uint32_t doc_id) const {
  std::shared_lock lock(mutex_);
  return index_->GetDocument(doc_id);
}

uint32_t LiveIndex::TotalDocs() const {
  std::shared_lock lock(mutex_);
  return index_->TotalDocs();
}

double LiveIndex::AvgDocLength() const {
  std::shared_lock lock(mutex_);
  return index_->AvgDocLength();
}

uint32_t LiveIndex::DocFreq(const std::string& term) const {
  std::shared_lock lock(mutex_);
  return index_->DocFreq(term);
}

uint32_t LiveIndex::GetInternalId(const std::string& external_id) const {
  std::shared_lock lock(mutex_);

  auto it = external_to_internal_.find(external_id);
  if (it == external_to_internal_.end()) {
    return 0;
  }

  return it->second;
}

std::string LiveIndex::GetExternalId(uint32_t internal_id) const {
  std::shared_lock lock(mutex_);

  auto it = internal_to_external_.find(internal_id);
  if (it == internal_to_external_.end()) {
    return "";
  }

  return it->second;
}

void LiveIndex::Flush() {
  std::unique_lock lock(mutex_);

  if (index_->TotalDocs() == 0) {
    return;
  }

  DiskIndex::Serialize(*index_, index_path_);
  SaveIdMapping();
}

void LiveIndex::LoadFromDisk() {
  std::ifstream file(index_path_);
  if (!file.good()) {
    return;
  }

  file.close();
  auto disk_index = std::make_unique<DiskIndex>(index_path_);
  LoadIdMapping();
}

void LiveIndex::FlushLoop() {
  while (running_) {
    for (uint32_t i = 0; i < flush_interval_seconds_ && running_; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (running_) {
      Flush();
    }
  }
}

void LiveIndex::SaveIdMapping() const {
  std::string path = index_path_ + ".idmap";
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return;
  }

  uint32_t count = external_to_internal_.size();
  out.write(reinterpret_cast<const char*>(&count), sizeof(count));

  for (const auto& [ext_id, int_id] : external_to_internal_) {
    uint32_t len = ext_id.size();
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    out.write(ext_id.data(), len);
    out.write(reinterpret_cast<const char*>(&int_id), sizeof(int_id));
  }
}

void LiveIndex::LoadIdMapping() {
  std::string path = index_path_ + ".idmap";
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    return;
  }

  uint32_t count = 0;
  in.read(reinterpret_cast<char*>(&count), sizeof(count));

  for (uint32_t i = 0; i < count; ++i) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));

    std::string ext_id(len, '\0');
    in.read(ext_id.data(), len);

    uint32_t int_id = 0;
    in.read(reinterpret_cast<char*>(&int_id), sizeof(int_id));

    external_to_internal_[ext_id] = int_id;
    internal_to_external_[int_id] = ext_id;

    if (int_id >= next_internal_id_) {
      next_internal_id_ = int_id + 1;
    }
  }
}

}  // namespace search
