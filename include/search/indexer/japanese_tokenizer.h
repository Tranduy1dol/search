#pragma once

#include <mecab.h>

#include <string>
#include <vector>

namespace search {

class JapaneseTokenizer {
 public:
  JapaneseTokenizer();
  ~JapaneseTokenizer();

  JapaneseTokenizer(const JapaneseTokenizer&) = delete;
  JapaneseTokenizer& operator=(const JapaneseTokenizer&) = delete;

  std::vector<std::string> Tokenize(const std::string& text) const;

 private:
  MeCab::Tagger* tagger_;
};

}  // namespace search
