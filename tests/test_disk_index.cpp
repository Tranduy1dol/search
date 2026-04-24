#include <gtest/gtest.h>

#include <filesystem>

#include "search/indexer/disk_index.h"
#include "search/indexer/inverted_index.h"

using search::DiskIndex;
using search::InvertedIndex;

class DiskIndexTest : public ::testing::Test {
 protected:
  InvertedIndex memory_index_{""};
  std::string test_file_path_ = "test_index.bin";

  void SetUp() override {
    memory_index_.AddDocument(1, "hello world hello", "Title 1", "http://url1");
    memory_index_.AddDocument(2, "world foo bar", "Title 2", "http://url2");
    DiskIndex::Serialize(memory_index_, test_file_path_);
  }

  void TearDown() override {
    if (std::filesystem::exists(test_file_path_)) {
      std::filesystem::remove(test_file_path_);
    }
  }
};

TEST_F(DiskIndexTest, HeaderStatsMatch) {
  DiskIndex disk_index(test_file_path_);

  EXPECT_EQ(disk_index.TotalDocs(), memory_index_.TotalDocs());
  EXPECT_DOUBLE_EQ(disk_index.AvgDocLength(), memory_index_.AvgDocLength());
}

TEST_F(DiskIndexTest, DocumentMetadataMatches) {
  DiskIndex disk_index(test_file_path_);

  Document mem_doc = memory_index_.GetDocument(1u);
  Document disk_doc = disk_index.GetDocument(1u);

  EXPECT_EQ(disk_doc.doc_id_, mem_doc.doc_id_);
  EXPECT_EQ(disk_doc.title_, mem_doc.title_);
  EXPECT_EQ(disk_doc.url_, mem_doc.url_);
  EXPECT_EQ(disk_doc.length_, mem_doc.length_);
}

TEST_F(DiskIndexTest, PostingsMatch) {
  DiskIndex disk_index(test_file_path_);

  auto mem_postings = memory_index_.GetPostings("world");
  auto disk_postings = disk_index.GetPostings("world");

  ASSERT_EQ(disk_postings.size(), mem_postings.size());

  EXPECT_EQ(disk_postings[0].doc_id_, mem_postings[0].doc_id_);
  EXPECT_EQ(disk_postings[0].term_freq_, mem_postings[0].term_freq_);

  ASSERT_EQ(disk_postings[0].positions_.size(), 1u);
  EXPECT_EQ(disk_postings[0].positions_[0], mem_postings[0].positions_[0]);
}

TEST_F(DiskIndexTest, MissingTermReturnsEmpty) {
  DiskIndex disk_index(test_file_path_);

  auto disk_postings = disk_index.GetPostings("missingterm");
  EXPECT_TRUE(disk_postings.empty());
}
