#include <gtest/gtest.h>

#include "search/common/utils.h"

using search::utils::IsDelimiter;
using search::utils::Normalize;
using search::utils::SplitTokens;

TEST(IsDelimiterTest, SpaceIsDelimiter) { EXPECT_TRUE(IsDelimiter(' ')); }

TEST(IsDelimiterTest, LetterIsNotDelimiter) { EXPECT_FALSE(IsDelimiter('a')); }

TEST(IsDelimiterTest, CommaIsDelimiter) { EXPECT_TRUE(IsDelimiter('.')); }

TEST(IsDelimiterTest, DigitIsNotDelimiter) {
  EXPECT_FALSE(search::utils::IsDelimiter('5'));
}

TEST(SplitTokensTest, BasicTwoWords) {
  auto tokens = SplitTokens("hello world");
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

TEST(SplitTokensTest, CommaDelimited) {
  auto tokens = SplitTokens("hello,world");
  EXPECT_EQ(tokens.size(), 2u);
  EXPECT_EQ(tokens[0], "hello");
  EXPECT_EQ(tokens[1], "world");
}

TEST(SplitTokensTest, LeadingTrailingSpaces) {
  auto tokens = SplitTokens("  hello  ");
  EXPECT_EQ(tokens.size(), 1u);
  EXPECT_EQ(tokens[0], "hello");
}

TEST(SplitTokensTest, EmptyString) {
  auto tokens = SplitTokens("");
  EXPECT_EQ(tokens.size(), 0u);
}

TEST(SplitTokensTest, ThreeWords) {
  auto tokens = SplitTokens("one two three");
  EXPECT_EQ(tokens.size(), 3u);
  EXPECT_EQ(tokens[0], "one");
  EXPECT_EQ(tokens[1], "two");
  EXPECT_EQ(tokens[2], "three");
}

TEST(NormalizeTest, UpperToLower) { EXPECT_EQ(Normalize("Hello"), "hello"); }

TEST(NormalizeTest, AllUpper) { EXPECT_EQ(Normalize("WORLD"), "world"); }

TEST(NormalizeTest, AlreadyLower) {
  EXPECT_EQ(Normalize("already lowercase"), "already lowercase");
}

TEST(NormalizeTest, DigitsUntouched) {
  EXPECT_EQ(Normalize("abc123"), "abc123");
}
