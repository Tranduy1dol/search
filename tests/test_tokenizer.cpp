#include <gtest/gtest.h>

#include <vector>

#include "search/indexer/tokenizer.h"

#ifndef TEST_DATA_DIR
#define TEST_DATA_DIR "."
#endif

static const std::string kStopwordsPath =
    std::string(TEST_DATA_DIR) + "/stopwords_vi.txt";

TEST(TokenizeTest, NoStopwordRemoved) {
  search::Tokenizer tokenizer(kStopwordsPath);
  std::vector<std::string> expected{"hello", "world"};

  EXPECT_EQ(tokenizer.Tokenize("Hello World"), expected);
}

TEST(TokenizeTest, StopwordRemoved) {
  search::Tokenizer tokenizer(kStopwordsPath);
  std::vector<std::string> expected{"khó", "vl"};

  EXPECT_EQ(tokenizer.Tokenize("tôi khó vl"), expected);
}

TEST(IsStopwordTest, IsStopword) {
  search::Tokenizer tokenizer(kStopwordsPath);

  EXPECT_TRUE(tokenizer.IsStopword("và"));
}

TEST(IsStopwordTest, NotStopword) {
  search::Tokenizer tokenizer(kStopwordsPath);

  EXPECT_FALSE(tokenizer.IsStopword("tao"));
}
