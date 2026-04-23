#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace search {

class Tokenizer {
 public:
  explicit Tokenizer(const std::string& stopwords_path);
  std::vector<std::string> Tokenize(const std::string& text) const;
  bool IsStopword(const std::string& token) const;

 private:
  std::unordered_set<std::string> stopwords_;
  void LoadStopwords(const std::string& path);
};

}  // namespace search
