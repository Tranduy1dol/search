#include <cctype>
#include <string>
#include <vector>

namespace search::utils {

bool IsDelimiter(char c) {
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '.' ||
          c == ',' || c == '!' || c == '?' || c == ';' || c == ':' ||
          c == '(' || c == ')' || c == '"' || c == '\'' || c == '-' ||
          c == '/' || c == '\\' || c == '[' || c == ']');
}

std::vector<std::string> SplitTokens(const std::string& text) {
  std::vector<std::string> result;
  std::string current_word;

  for (char c : text) {
    if (IsDelimiter(c)) {
      if (!current_word.empty()) {
        result.push_back(current_word);
        current_word = "";
      }
    } else {
      current_word.push_back(c);
    }
  }

  if (!current_word.empty()) {
    result.push_back(current_word);
  }

  return result;
}

std::string Normalize(const std::string& utf8_text) {
  std::string result = utf8_text;

  for (char& c : result) {
    if ('A' <= c && c <= 'Z') {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
  }

  return result;
}

}  // namespace search::utils
