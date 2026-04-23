#include <gtest/gtest.h>

#include "search/indexer/inverted_index.h"

class InvertedIndexTest : public ::testing::Test {
 protected:
  search::InvertedIndex index_{""};

  void SetUp() override {
    index_.AddDocument(1, "hello world hello", "Doc1", "url1");
    index_.AddDocument(2, "world foo", "Doc2", "url2");
  }
};

TEST_F(InvertedIndexTest, HelloPostingsCount) {
  auto& postings = index_.GetPostings("hello");
  EXPECT_EQ(postings.size(), 1u);  // "hello" only in doc 1
  EXPECT_EQ(postings[0].doc_id_, 1u);
  EXPECT_EQ(postings[0].term_freq_, 2u);  // appears twice
}

TEST_F(InvertedIndexTest, WorldInBothDocs) {
  EXPECT_EQ(index_.DocFreq("world"), 2u);
}

TEST_F(InvertedIndexTest, MissingTermEmpty) {
  auto& postings = index_.GetPostings("missing");
  EXPECT_TRUE(postings.empty());
}

TEST_F(InvertedIndexTest, TotalDocs) { EXPECT_EQ(index_.TotalDocs(), 2u); }
