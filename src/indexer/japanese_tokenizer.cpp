#include "search/indexer/japanese_tokenizer.h"

#include <mecab.h>

#include <stdexcept>
#include <vector>

namespace search {

JapaneseTokenizer::JapaneseTokenizer() {
  tagger_ = MeCab::createTagger("");
  if (!tagger_) {
    throw std::runtime_error("failed to create MeCab tagger");
  }
}

JapaneseTokenizer::~JapaneseTokenizer() { delete tagger_; }

std::vector<std::string> JapaneseTokenizer::Tokenize(
    const std::string& text) const {
  std::vector<std::string> tokens;

  const MeCab::Node* node = tagger_->parseToNode(text.c_str());
  if (!node) {
    return tokens;
  }

  for (; node; node = node->next) {
    if (node->stat == MECAB_BOS_NODE || node->stat == MECAB_EOS_NODE) {
      continue;
    }

    std::string surface(node->surface, node->length);
    if (surface.empty()) {
      continue;
    }

    char first = surface[0];
    if (first == ' ' || first == '\t' || first == '\n') {
      continue;
    }

    tokens.push_back(surface);
  }

  return tokens;
}

}  // namespace search
