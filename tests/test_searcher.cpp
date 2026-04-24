#include <gtest/gtest.h>

#include <filesystem>

#include "search/indexer/disk_index.h"
#include "search/indexer/inverted_index.h"
#include "search/searcher/searcher.h"

using search::DiskIndex;
using search::InvertedIndex;
using search::Searcher;

TEST(SearcherTest, BM25RankingOrder) {
  std::string test_file = "test_searcher_index.bin";

  InvertedIndex mem_index{""};

  mem_index.AddDocument(1, "apple orange banana", "Fruit Mix", "url1");
  mem_index.AddDocument(2, "apple apple apple grape", "Only Apples", "url2");
  mem_index.AddDocument(3, "grape orange banana", "No Apples", "url3");

  DiskIndex::Serialize(mem_index, test_file);

  auto disk_index = std::make_unique<DiskIndex>(test_file);
  Searcher searcher(std::move(disk_index), "");

  auto results = searcher.Search("apple", 10);

  ASSERT_EQ(results.size(), 2u);

  EXPECT_EQ(results[0].doc_id_, 2u);
  EXPECT_EQ(results[0].title_, "Only Apples");
  EXPECT_GT(results[0].score_, results[1].score_);

  std::filesystem::remove(test_file);
}
