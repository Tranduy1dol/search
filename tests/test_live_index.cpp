#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "search/indexer/live_index.h"

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "."
#endif

static const std::string kStopwordsPath =
    std::string(TEST_DATA_DIR) + "/stopwords_vi.txt";
static const std::string kIndexPath =
    std::string(TEST_DATA_DIR) + "/test_live_index.bin";

class LiveIndexTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::remove(kIndexPath.c_str());
    std::remove((kIndexPath + ".idmap").c_str());
  }
  void TearDown() override {
    std::remove(kIndexPath.c_str());
    std::remove((kIndexPath + ".idmap").c_str());
  }
};

TEST_F(LiveIndexTest, IndexAndSearch) {
  search::LiveIndex index(kIndexPath, kStopwordsPath, 9999);
  index.IndexDocument("doc1", "hello world", "Hello", "http://example.com");

  auto postings = index.GetPostings("hello");
  ASSERT_EQ(postings.size(), 1);
  EXPECT_EQ(postings[0].term_freq_, 1);
}

TEST_F(LiveIndexTest, UpdateDocument) {
  search::LiveIndex index(kIndexPath, kStopwordsPath, 9999);
  index.IndexDocument("doc1", "hello world", "Hello", "http://example.com");
  index.IndexDocument("doc1", "goodbye world", "Goodbye", "http://example.com");

  auto hello_postings = index.GetPostings("hello");
  EXPECT_TRUE(hello_postings.empty());

  auto goodbye_postings = index.GetPostings("goodbye");
  ASSERT_EQ(goodbye_postings.size(), 1);
}

TEST_F(LiveIndexTest, IdMapping) {
  search::LiveIndex index(kIndexPath, kStopwordsPath, 9999);
  index.IndexDocument("mongo_abc123", "test content", "Test",
                      "http://test.com");

  uint32_t internal_id = index.GetInternalId("mongo_abc123");
  EXPECT_NE(internal_id, 0);

  std::string external_id = index.GetExternalId(internal_id);
  EXPECT_EQ(external_id, "mongo_abc123");
}

TEST_F(LiveIndexTest, FlushAndReloadMapping) {
  {
    search::LiveIndex index(kIndexPath, kStopwordsPath, 9999);
    index.IndexDocument("doc1", "hello world", "Hello", "http://example.com");
    index.Flush();
  }

  search::LiveIndex index2(kIndexPath, kStopwordsPath, 9999);
  uint32_t internal_id = index2.GetInternalId("doc1");
  EXPECT_NE(internal_id, 0);
}

TEST_F(LiveIndexTest, EmptyIndexStats) {
  search::LiveIndex index(kIndexPath, kStopwordsPath, 9999);
  EXPECT_EQ(index.TotalDocs(), 0);
  EXPECT_DOUBLE_EQ(index.AvgDocLength(), 0.0);
}
