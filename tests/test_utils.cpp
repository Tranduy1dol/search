#include "search/common/utils.h"
#include <gtest/gtest.h>

using namespace search::utils;

TEST(IsDelimiterTest, SpaceIsDelimiter) { EXPECT_TRUE(is_delimiter(' ')); }

TEST(IsDelimiterTest, LetterIsNotDelimiter) { EXPECT_FALSE(is_delimiter('a')); }

TEST(IsDelimiterTest, CommaIsDelimiter) { EXPECT_TRUE(is_delimiter('.')); }

TEST(IsDelimiterTest, DigitIsNotDelimiter) {
  EXPECT_FALSE(search::utils::is_delimiter('5'));
}

TEST(SplitTokensTest, BasicTwoWords) {
  auto tokens = split_tokens("hello world");
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

TEST(SplitTokensTest, CommaDelimited) {
  auto tokens = split_tokens("hello,world");
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

TEST(SplitTokensTest, LeadingTrailingSpaces) {
  auto tokens = split_tokens("  hello  ");
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], "hello");
}

TEST(SplitTokensTest, EmptyString) {
  auto tokens = split_tokens("");
  EXPECT_EQ(tokens.size(), 0u);
}

TEST(SplitTokensTest, ThreeWords) {
  auto tokens = split_tokens("one two three");
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], "one");
  EXPECT_EQ(tokens[1], "two");
  EXPECT_EQ(tokens[2], "three");
}

TEST(NormalizeTest, UpperToLower) { EXPECT_EQ(normalize("Hello"), "hello"); }

TEST(NormalizeTest, AllUpper) { EXPECT_EQ(normalize("WORLD"), "world"); }

TEST(NormalizeTest, AlreadyLower) {
  EXPECT_EQ(normalize("already lowercase"), "already lowercase");
}

TEST(NormalizeTest, DigitsUntouched) {
  EXPECT_EQ(normalize("abc123"), "abc123");
}
