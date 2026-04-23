#include <gtest/gtest.h>

#include <vector>

#include "search/indexer/tokenizer.h"

TEST(TokenizeTest, NoStopwordRemoved) {
  search::Tokenizer tokenizer(
      "/home/tranduy/Workspace/search/data/stopwords_vi.txt");
  std::vector<std::string> expected{"hello", "world"};

  EXPECT_EQ(tokenizer.Tokenize("Hello World"), expected);
}

TEST(TokenizeTest, StopwordRemoved) {
  search::Tokenizer tokenizer(
      "/home/tranduy/Workspace/search/data/stopwords_vi.txt");
  std::vector<std::string> expected{"khó", "vl"};

  EXPECT_EQ(tokenizer.Tokenize("tôi khó vl"), expected);
}

TEST(IsStopwordTest, IsStopword) {
  search::Tokenizer tokenizer(
      "/home/tranduy/Workspace/search/data/stopwords_vi.txt");

  EXPECT_TRUE(tokenizer.IsStopword("và"));
}

TEST(IsStopwordTest, NotStopword) {
  search::Tokenizer tokenizer(
      "/home/tranduy/Workspace/search/data/stopwords_vi.txt");

  EXPECT_FALSE(tokenizer.IsStopword("tao"));
}
