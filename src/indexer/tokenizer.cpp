#include "search/indexer/tokenizer.h"

#include <cctype>
#include <cstdint>
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
  if (ConstainsJapanese(text)) {
    return japanese_tokenizer_.Tokenize(text);
  }

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

bool Tokenizer::ConstainsJapanese(const std::string& text) const {
  for (size_t i = 0; i < text.size();) {
    unsigned char c = text[i];
    if (c >= 0xE0 && i + 2 < text.size()) {
      uint32_t codepoint = ((c & 0x0f) << 12) | ((text[i + 1] & 0x3F) << 6) |
                           (text[i + 2] & 0x3F);
      if (codepoint >= 0x3000 && codepoint <= 0x9FFF) {
        return true;
      }
      i += 3;
    } else if (c >= 0xC0) {
      i += 2;
    } else {
      i += 1;
    }
  }

  return false;
}

}  // namespace search
