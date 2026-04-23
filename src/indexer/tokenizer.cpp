#include "search/indexer/tokenizer.h"

#include <cctype>
#include <fstream>
#include <string>
#include <vector>

#include "search/common/utils.h"

namespace search {

Tokenizer::Tokenizer(const std::string& stopwords_path) {
  LoadStopwords(stopwords_path);
}

bool Tokenizer::IsStopword(const std::string& token) const {
  return stopwords_.count(token) > 0;
}

std::vector<std::string> Tokenizer::Tokenize(const std::string& text) const {
  std::string normalized = utils::Normalize(text);
  std::vector<std::string> all_tokens = utils::SplitTokens(normalized);
  std::vector<std::string> result;

  for (std::string token : all_tokens) {
    if (!IsStopword(token) && !token.empty()) {
      result.push_back(token);
    }
  }

  return result;
}

void Tokenizer::LoadStopwords(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return;
  }

  std::string line;
  while (std::getline(file, line)) {
    while (!line.empty() && std::isspace(line.front())) {
      line.erase(line.begin());
    }

    while (!line.empty() && std::isspace(line.back())) {
      line.pop_back();
    }

    if (!line.empty()) {
      stopwords_.insert(line);
    }
  }
}

}  // namespace search
